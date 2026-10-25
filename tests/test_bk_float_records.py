"""Compile the actual bounded BK float-record lookup; no game or shared build.

Usage: python tests/test_bk_float_records.py [--source path/to/bk_float.inc]
       [--compiler path/to/clang++]
Only the production lookup, its record type and record bounds are extracted.
Temporary C++ and executable files are confined to TemporaryDirectory.
"""
import argparse
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

PRELUDE = r"""#include <cstdint>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <array>
#include <vector>
using u32=uint32_t;
"""
CASES = r"""// Included after extracted actual helper and fixture-specific call_record adapter.
static uint32_t bucket(uint32_t id) { return ((id >> 8)*2654435761u) >> 22; }
static void reset(uint32_t frame) { std::memset((void*)&BKF,0,sizeof(BKF)); BKF.frame=frame; }
static void assign(volatile TooieBkRecord* r,uint32_t id,uint32_t frame,uint32_t model,uint32_t start) {
 assert(r);r->id=id;r->frame=frame;r->model=model;r->start=start;
}
int main(){
 const uint32_t ids[]={0x50000500,0x50B52500,0x516A4500};
 reset(10);for(auto id:ids) {auto* r=call_record(id);assign(r,id,10,0x12340000,id);}
 for(uint32_t f=11;f<=13;f++) {BKF.frame=f;for(int i=2;i>=0;i--) {auto* r=call_record(ids[i]);assert(r && r->id==ids[i] && r->frame==f-1 && r->model==0x12340000 && r->start==ids[i]);r->frame=f;}}
 // Stale first probe cannot hide an existing exact ID farther along its cluster.
 reset(20);assign(&BKF.records[bucket(ids[0])],0x77770000,1,1,1);
 assign(&BKF.records[(bucket(ids[0])+7)%1024],ids[0],19,77,88);
 auto* found=call_record(ids[0]);assert(found==&BKF.records[(bucket(ids[0])+7)%1024] && found->model==77 && found->start==88);
 // More than32 legal same-bucket actor samples must retain all if capacity is1024.
 reset(30);std::vector<uint32_t> colliders;
 for(uint32_t birth=1;birth<=37449 && colliders.size()<40;birth++) {uint32_t id=0x50000000+(birth-1)*112*256+5*256;if(bucket(id)==875)colliders.push_back(id);}
 assert(colliders.size()==40);for(auto id:colliders) {auto* r=call_record(id);assert(r);assign(r,id,30,9,id);}
 BKF.frame=31;for(auto i=colliders.rbegin();i!=colliders.rend();i++){auto* r=call_record(*i);assert(r && r->id==*i && r->frame==30 && r->start==*i);r->frame=31;}
 // True full live table declines without changing any record.
 reset(40);for(uint32_t i=0;i<1024;i++)assign(&BKF.records[i],0x01000000+i*256,39+(i&1),7,i);
 std::array<unsigned char,sizeof(BKF.records)> before;std::memcpy(before.data(),(const void*)BKF.records,before.size());
 assert(call_record(0x70000000)==nullptr);assert(std::memcmp(before.data(),(const void*)BKF.records,before.size())==0);
 // Once stale, capacity is reusable without inheriting a live model/start history.
 BKF.frame=43;auto* fresh=call_record(0x70000000);assert(fresh);assert(fresh->frame!=42);
 // Wrap: exact immediately previous uint32 frame remains discoverable.
 reset(0);assign(&BKF.records[bucket(ids[0])],ids[0],0xFFFFFFFFu,91,92);
 auto* wrapped=call_record(ids[0]);assert(wrapped && wrapped->frame==0xFFFFFFFFu && wrapped->model==91);
 reset(0);for(uint32_t i=0;i<1024;i++)assign(&BKF.records[i],0x01000000+i*256,0,7,i);
 assert(call_record(0x70000000)==nullptr);
 BKF.frame=1;assert(call_record(0x70000000)==nullptr);
 // Empty id sentinel with a misleading recent frame is still free storage.
 reset(1);for(auto& r:BKF.records)r.frame=0;assert(call_record(ids[0]));
 puts("extracted bk_record: triple/40-collision/full-capacity/stale/wrap PASS");
}
"""

def extract_function(text, name):
    match = re.search(r"SI\s+volatile\s+struct\s+TooieBkRecord\*\s+" + re.escape(name) + r"\(u32 id\)\s*\{", text)
    if not match:
        raise AssertionError("production bk_record definition missing or changed; review fixture binding")
    start = match.start()
    depth = 1
    end = match.end()
    while depth and end < len(text):
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    assert depth == 0, "unterminated production helper"
    return text[start:end].replace("SI ", "", 1)

def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=root / "tools/baseline/guest/silo-candidate/bk_float.inc")
    parser.add_argument("--compiler", type=Path)
    args = parser.parse_args()
    compiler = args.compiler
    if compiler is None:
        bundled = root / "deps/LLVM-19.1.3-Windows-X64/bin/clang++.exe"
        compiler = bundled if bundled.is_file() else shutil.which("clang++")
    if not compiler:
        raise SystemExit("clang++ unavailable; pass --compiler")
    text = args.source.read_text(encoding="utf-8")
    bounds = re.findall(r"^#define TOOIE_BK_RECORD(?:S|_PROBE)\b.*$", text, re.M)
    assert any(line.startswith("#define TOOIE_BK_RECORDS ") for line in bounds)
    record = re.search(r"struct TooieBkRecord \{[^}]*\};", text)
    assert record, "production record schema changed"
    code = (PRELUDE + "\n".join(bounds) + "\n" + record.group() +
            "\nstruct {uint32_t frame;TooieBkRecord records[TOOIE_BK_RECORDS];} BKF{};\n" +
            extract_function(text, "bk_record") +
            "\nauto call_record(uint32_t id){return bk_record(id); }\n" + CASES)
    with tempfile.TemporaryDirectory(prefix="bk-float-records-") as temporary:
        directory = Path(temporary)
        source = directory / "test.cpp"
        binary = directory / "test.exe"
        source.write_text(code, encoding="utf-8")
        subprocess.run([str(compiler), "-std=c++20", str(source), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
    print("test_bk_float_records: ok (actual production helper)")

if __name__ == "__main__":
    main()
