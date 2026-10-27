// Option-box text owner (menu_text.hpp): ownership identity across constructions, vector lifetimes and reused vector
// indices, the push seam order, the bounded string scan, exact rectangle -> quad words, packet/stack balance and state
// restoration. Synthetic words only; no game data, renderer or GPU. The last group is an explicit MODEL of RT64's
// consumer (rectangles are never interpolated; an explicit-ID quad interpolates its vertices), not a visual claim.
#include "../tools/baseline/menu_text.hpp"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace tooie_menu_text;
namespace {
uint32_t rect_word0(int lrx,int lry){return 0xE4000000u|(uint32_t(lrx)<<12)|uint32_t(lry);}
uint32_t rect_word1(unsigned tile,int ulx,int uly){return (tile<<24)|(uint32_t(ulx)<<12)|uint32_t(uly);}
int16_t hi(uint32_t w){return int16_t(w>>16);} int16_t lo(uint32_t w){return int16_t(w&0xFFFFu);}
struct Stacks { int viewport=0, projection=0, proj_group=0, geometry=0, modelview=0, mv_group=0; bool persp=false;
    int textures=0; std::vector<uint32_t> ids; std::vector<std::vector<int>> quads; };
// Interpret our own words (skipping branch-over data) and track every state stack they touch.
void run(const std::vector<uint32_t>& w, uint32_t base, Stacks& s){
    size_t i=0;
    while(i<w.size()){
        const uint32_t w0=w[i],w1=w[i+1];
        if(w0==0xDE010000u){ i=(w1-base)/4u; continue; }
        if(w0==0x64000015u)++s.viewport; else if(w0==0x64000016u)--s.viewport;
        else if(w0==0x6400001Du)++s.projection; else if(w0==0x6400001Eu)--s.projection;
        else if(w0==0x64000029u)++s.geometry; else if(w0==0x6400002Au)--s.geometry;
        else if(w0==0xDA380002u)++s.modelview; else if(w0==0xD8380002u){assert(w1==64u);--s.modelview;}
        else if(w0==0x6400000Cu){ (w[i+2]&2u)?++s.proj_group:++s.mv_group; if(!(w[i+2]&2u))s.ids.push_back(w1); i+=4; continue; }
        else if(w0==0x6400000Du){ (w1&0x100u)?--s.proj_group:--s.mv_group; }
        else if(w0==0xE3000C00u){ s.persp=(w1==0x00080000u); }
        else if((w0>>24)==0xD7u)++s.textures;
        else if(w0==0x01004008u){ // gSPVertex(4): keep the four x positions of this quad
            const size_t v=(w1-base)/4u; s.quads.push_back({hi(w[v]),hi(w[v+4]),hi(w[v+8]),hi(w[v+12])}); }
        else if(w0==0x64000030u){ i+=4; continue; }
        i+=2;
    }
}
// A queue record's identity words (+0 x/y, +C string) in a synthetic vector.
struct Record { uint32_t xy, string; };
}
int main(){
    // IDs: one block per box occurrence; inactive or foreign gfx gives none.
    tooie_menu::State s;
    assert(tooie_menu::initialize(s,0x100,1));
    auto outer=tooie_menu::enter(s,0x100,0x1000,1);
    assert(tooie_menu::occurrence(s,2,tooie_menu::primary));
    const uint32_t a=occurrence_base(s,0x1000);
    assert(a>=first_id && a+block<=end_id && !(a&0xFFu) && !occurrence_base(s,0x2000));
    assert(tooie_menu::occurrence(s,2,tooie_menu::upper_wrap));
    const uint32_t b=occurrence_base(s,0x1000);assert(b && b!=a && (b>a?b-a:a-b)>=block);
    tooie_menu::leave(s,outer);
    assert(!occurrence_base(s,0x1000));

    // Ownership: only pushes inside the zoombox text scope, two lines, keyed by vector index in one construction and
    // authenticated by the record's own string and x/y words.
    Queue q; construction(q,7);
    pushed(q,0,0x80400000u,0x00100020u);               // outside any scope: not owned
    scope_begin(q,a,0);
    pushed(q,1,0x80400100u,0x00300040u); pushed(q,2,0x80400200u,0x00300050u);
    pushed(q,3,0x80400300u,0x00300060u);               // a third line: not owned
    scope_end(q);
    assert(!owner(q,0,7,0x80400000u,0x00100020u) && owner(q,1,7,0x80400100u,0x00300040u)->base==a);
    assert(owner(q,2,7,0x80400200u,0x00300050u)->base==a+glyphs && !owner(q,3,7,0x80400300u,0x00300060u));
    // A reused vector slot with another record's words is never owned.
    assert(!owner(q,1,7,0x80400999u,0x00300040u) && !owner(q,1,7,0x80400100u,0x00310040u));
    assert(!owner(q,1,8,0x80400100u,0x00300040u));     // another construction
    // The same index pushed again in this construction replaces its stale owner.
    scope_begin(q,b,1); pushed(q,1,0x80400700u,0x00700070u); scope_end(q);
    assert(!owner(q,1,7,0x80400100u,0x00300040u) && owner(q,1,7,0x80400700u,0x00700070u)->base==b &&
        owner(q,1,7,0x80400700u,0x00700070u)->cut==1);
    // A new construction or a consumed vector owns nothing.
    construction(q,8); assert(!owner(q,2,8,0x80400200u,0x00300050u));
    construction(q,9); scope_begin(q,a,0); pushed(q,0,1,2); scope_end(q); cleared(q); assert(!owner(q,0,9,1,2));

    // Push seam order (B798C): push_back returns a slot still holding its previous occupant's words; x (B79E4),
    // y (B79F0) and the string (B79FC) are stored after it. Identity read at the old B79D8 point is stale and the
    // traversal refuses it; read at B7A84 it is the record's own and is owned.
    {
        Record slot{0x00110022u,0x80410000u};                       // the previous occupant of index 0
        const Record final_words{0x00500060u,0x80420000u};
        Queue early; construction(early,20); scope_begin(early,a,0);
        pushed(early,0,slot.string,slot.xy);                         // B79D8: before the identity stores
        slot=final_words;                                            // B79E4/B79F0/B79FC
        scope_end(early);
        assert(!owner(early,0,20,slot.string,slot.xy));
        Queue late; construction(late,20); scope_begin(late,a,0);
        Record slot2{0x00110022u,0x80410000u}; slot2=final_words;    // the stores, then B7A84
        pushed(late,0,slot2.string,slot2.xy); scope_end(late);
        assert(owner(late,0,20,final_words.string,final_words.xy) && owner(late,0,20,final_words.string,final_words.xy)->base==a);
    }
    // Vector lifetime (B5E04): a same-frame free and recreate, then an unowned record at the same index with the same
    // string and x/y, never inherits the old owner; a push into the new vector is owned again.
    {
        Queue v; construction(v,30); scope_begin(v,a,0); pushed(v,0,0x80430000u,0x00010002u); scope_end(v);
        assert(owner(v,0,30,0x80430000u,0x00010002u));
        freed(v); construction(v,30);                                 // same construction
        assert(!owner(v,0,30,0x80430000u,0x00010002u));
        scope_begin(v,b,0); pushed(v,0,0x80430000u,0x00010002u); scope_end(v);
        assert(owner(v,0,30,0x80430000u,0x00010002u) && owner(v,0,30,0x80430000u,0x00010002u)->base==b);
        // B5E3C defrag relocates the vector with its contents: begin moves, the index (record - begin) / stride and
        // the words are unchanged, so ownership survives (no queue event).
        const uint32_t old_begin=0x80440000u,new_begin=0x80450000u,stride=0x14u;
        const uint32_t before=(old_begin+stride*0-old_begin)/stride,after=(new_begin+stride*0-new_begin)/stride;
        assert(before==after && owner(v,after,30,0x80430000u,0x00010002u));
    }
    // Bounded string scan: one glyph per byte, at most one line, never reading at or past 0x81000000.
    {
        uint32_t top=0;   // highest address read
        auto at=[&](uint32_t base,const char* text){
            return [&top,base,text](uint32_t addr){ assert(addr<0x81000000u); top=addr>top?addr:top;
                return uint32_t(uint8_t(text[addr-base])); }; };
        assert(string_bytes(0x80400000u,at(0x80400000u,"WARP"))==4);
        assert(string_bytes(0x80FFFFF0u,at(0x80FFFFF0u,"abc"))==3);
        assert(!string_bytes(0x80400000u,at(0x80400000u,"")));                       // empty
        const char tail[5]={'a','b','c','d','e'};                                     // no NUL before 0x81000000
        assert(!string_bytes(0x80FFFFFCu,at(0x80FFFFFCu,tail)) && top==0x80FFFFFFu);
        assert(!string_bytes(0x81000000u,at(0x81000000u,"x")) && !string_bytes(0x7FFFFFF0u,at(0x7FFFFFF0u,"x")));
        std::vector<char> line(glyphs+2,'a'); line[glyphs]=0;
        assert(string_bytes(0x80400000u,at(0x80400000u,line.data()))==glyphs);       // a full line
        line[glyphs]='a'; line[glyphs+1]=0;
        assert(!string_bytes(0x80400000u,at(0x80400000u,line.data())));              // longer than one line
        assert(record_bytes(glyphs)==4u*(open_words+close_words+glyphs*glyph_words));
    }

    // Rectangle decode and its quad: 8x8 px glyph at (100,50), 1 texel/px, tile 0; texcoords twice s10.5
    // (texture scale 0x8000, BK's 64 units per texel).
    const uint32_t w[6]={rect_word0(432,232),rect_word1(0,400,200),0xE1000000u,0x00000000u,0xF1000000u,0x04000400u};
    Rect r; assert(decode(w,r));
    assert(r.ulx==400 && r.uly==200 && r.lrx==432 && r.lry==232 && r.s==0 && r.t==0 && r.dsdx==0x400 && r.dtdy==0x400);
    uint32_t v[16]; assert(quad(r,320,240,v));
    assert(hi(v[0])==-240 && lo(v[0])==280 && hi(v[2])==0 && lo(v[2])==0 && v[3]==0xFFFFFFFFu);
    assert(hi(v[4])==-208 && lo(v[4])==280 && hi(v[6])==512 && lo(v[6])==0);
    assert(hi(v[8])==-240 && lo(v[8])==248 && hi(v[10])==0 && lo(v[10])==512);
    assert(hi(v[12])==-208 && lo(v[12])==248 && hi(v[14])==512 && lo(v[14])==512);
    // Viewport scale = translate = 2W,2H and ortho 1/2W,1/2H map each corner to its original quarter pixel.
    for(int i=0;i<4;++i){const int x=hi(v[i*4]),y=lo(v[i*4]); assert(640+x==(i&1?432:400) && 480-y==(i&2?232:200));}
    // Original clipping/scale stay authoritative: a clipped glyph keeps its adjusted S/T; scale 0.5 halves the span.
    const uint32_t clipped[6]={w[0],w[1],w[2],0x00400020u,w[4],0x02000400u};
    Rect rc; assert(decode(clipped,rc) && quad(rc,320,240,v) && hi(v[2])==128 && lo(v[2])==64 && hi(v[6])==128+256);
    // Shape refusals stay original: another opcode, a non-zero tile, an empty rectangle.
    uint32_t bad[6]={(w[0]&0x00FFFFFFu)|0xE5000000u,w[1],w[2],w[3],w[4],w[5]}; assert(!decode(bad,r));
    uint32_t tile2[6]={w[0],rect_word1(2,400,200),w[2],w[3],w[4],w[5]}; assert(!decode(tile2,r));
    uint32_t empty[6]={rect_word0(400,232),w[1],w[2],w[3],w[4],w[5]}; assert(!decode(empty,r));

    assert(decode(w,r)); // Restore the valid rectangle after the refusal cases below.

    // A whole admitted record: open + three glyphs + close, laid out contiguously; balanced stacks, no texture
    // command, G_TP_NONE restored after every quad, each glyph its own ID.
    const uint32_t base=0x80300000u;std::vector<uint32_t> list(4u*512u,0u);uint32_t at=base,n=0;
    n=open(&list[(at-base)/4u],at,320,240); assert(n==open_words); at+=4u*n;
    assert(list[0]==0xDE010000u && list[1]==base+8u+16u+64u && list[2]==((640u<<16)|480u) && list[3]==0x01FF0000u);
    assert(list[6]==f32_bits(1.0f/640.0f) && list[11]==f32_bits(1.0f/480.0f) && list[29]==projection_id && list[30]==projection_adjust);
    for(uint32_t g=0;g<3;++g){
        assert(quad(r,320,240,v)); n=glyph(&list[(at-base)/4u],at,v,0x80500000u,a+g,0); assert(n==glyph_words); at+=4u*n;
    }
    n=close(&list[(at-base)/4u]); assert(n==close_words); at+=4u*n;
    assert(at-base==record_bytes(3));
    list.resize((at-base)/4u);
    Stacks st; run(list,base,st);
    // Every pushed stack is popped; RSP texture state untouched; G_TP_NONE restored.
    assert(!st.viewport && !st.projection && !st.proj_group && !st.geometry && !st.modelview && !st.mv_group &&
        !st.persp && !st.textures);
    assert(st.ids==(std::vector<uint32_t>{a,a+1,a+2}) && st.quads.size()==3);
    uint32_t cut[glyph_words]; assert(glyph(cut,base,v,0x80500000u,a,1)==glyph_words && cut[21]==a && cut[22]==skip_all);
    uint32_t sil[6]={1,2,3,4,5,6}; silence(sil); for(uint32_t x:sil)assert(!x);

    // MODEL of the consumer across two constructions: the glyph slides 8 px right (32 quarter pixels).
    // Rectangle path (current): no transform and no ID, so every presentation draws the latest rectangle.
    // Quad path: the same explicit ID in both frames with four vertices; matching by ID gives vertex velocity, so a
    // presentation at weight 0.5 is the midpoint.
    const uint32_t w2[6]={rect_word0(464,232),rect_word1(0,432,200),w[2],w[3],w[4],w[5]};
    Rect r2; assert(decode(w2,r2)); uint32_t v1[16],v2[16]; assert(quad(r,320,240,v1) && quad(r2,320,240,v2));
    uint32_t g1[glyph_words],g2[glyph_words]; glyph(g1,base,v1,0x80500000u,a,0); glyph(g2,base,v2,0x80500000u,a,0);
    assert(g1[21]==g2[21] && g1[21]==a && g1[22]==simple_verts);        // same ID, vertex interpolation on
    const int rect_at_half=432;                                         // rectangle: latest position
    const int quad_at_half=640+(hi(v1[0])+(hi(v2[0])-hi(v1[0]))/2);     // quad: prior + velocity * 0.5
    assert(rect_at_half==432 && quad_at_half==416 && hi(v2[0])-hi(v1[0])==32);
    std::printf("baseline_menu_text: ok\n");
    return 0;
}
