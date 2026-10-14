"""Mechanical local PE/map/selected-guest equality gate; metadata output only."""
import argparse, hashlib, json, re, struct, sys
from pathlib import Path

def digest(data): return hashlib.sha256(data).hexdigest()
def check(exe_path, map_path, guest_path, layout_path):
    exe, mapping, guest, layout = (Path(x) for x in (exe_path,map_path,guest_path,layout_path))
    pe=exe.read_bytes(); text=mapping.read_text(); payload=guest.read_bytes()
    def need(test, why):
        if not test: raise ValueError(why)
    def take(offset, size):
        need(0 <= offset <= len(pe) and 0 <= size <= len(pe)-offset, "PE file span outside bounds")
        return pe[offset:offset+size]
    need(take(0,2)==b'MZ', "DOS signature")
    nt=struct.unpack('<I',take(0x3c,4))[0]; need(take(nt,4)==b'PE\0\0', "PE signature")
    count=struct.unpack('<H',take(nt+6,2))[0]; opt_size=struct.unpack('<H',take(nt+20,2))[0]
    opt=nt+24; need(struct.unpack('<H',take(opt,2))[0]==0x20b, "PE32+ required")
    need(opt_size>=32 and 0<count<=96, "PE section/optional-header bounds")
    image_base=struct.unpack('<Q',take(opt+24,8))[0]; sections=[]
    for i in range(count):
        raw=take(opt+opt_size+i*40,40)
        virtual_size,rva,raw_size,raw_offset=struct.unpack_from('<IIII',raw,8)
        take(raw_offset,raw_size); sections.append((rva,raw_size,raw_offset))
    def from_va(va, size):
        need(va>=image_base, "symbol below PE image base")
        rva=va-image_base; matches=[off+rva-start for start,n,off in sections if start<=rva and size<=n and rva-start<=n-size]
        need(len(matches)==1, "symbol span must have exactly one file-backed section")
        return take(matches[0],size)
    addresses={}; owners={}
    for name in ('baseline_boot_patch_bin','baseline_boot_patch_bin_size'):
        matches=re.findall(r'^\s*[0-9A-Fa-f]{4}:[0-9A-Fa-f]+\s+'+name+r'\s+([0-9A-Fa-f]{16})\s+(\S+)\s*$',text,re.M)
        need(len(matches)==1, "map symbol must have exactly one owner: "+name)
        address,owner=matches[0]; need(owner=='baseline_boot_patch_bin.c.obj', "unexpected embedded symbol owner")
        addresses[name]=int(address,16); owners[name]=owner
    size=struct.unpack('<Q',from_va(addresses['baseline_boot_patch_bin_size'],8))[0]
    need(size==len(payload), "linked embedded size differs from selected guest")
    linked=from_va(addresses['baseline_boot_patch_bin'],size)
    need(linked==payload, "linked embedded payload differs from selected guest")
    macros={name:int(value,0) for name,value in re.findall(r'^#define\s+(TOOIE_BRIDGE_\w+)\s+(0x[0-9A-Fa-f]+|[0-9]+)u\s*$',layout.read_text(),re.M)}
    image_bytes=macros['TOOIE_BRIDGE_IMAGE_BYTES']; offset=macros['TOOIE_BRIDGE_STATE_IMAGE_OFFSET']; vram=macros['TOOIE_BRIDGE_STATE_VRAM']
    need(size==image_bytes, "linked size differs from selected layout")
    need(offset<=size and size-offset>=18844, "selected layout state span outside image")
    need(vram==0x80801000+offset and vram+18844<=0x81000000, "selected layout VRAM extent")
    return {'status':'PASS','exe_sha256':digest(pe),'map_sha256':digest(mapping.read_bytes()),'guest_sha256':digest(payload),'layout_sha256':digest(layout.read_bytes()),'embedded_bytes':size,'embedded_sha256':digest(linked),'linked_payload_equals_selected_guest':True,'state_offset':offset,'state_bytes':18844,'state_vram':vram,'symbol_addresses':addresses,'symbol_owners':owners}

if __name__=='__main__':
    parser=argparse.ArgumentParser(); parser.add_argument('exe');parser.add_argument('map');parser.add_argument('guest');parser.add_argument('layout');args=parser.parse_args()
    try: print(json.dumps(check(args.exe,args.map,args.guest,args.layout)))
    except (OSError,ValueError,KeyError,struct.error) as error:
        print(json.dumps({'status':'FAIL','error':str(error)}));sys.exit(1)
