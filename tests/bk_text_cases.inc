// Included in baseline_bk_boundaries_test.cpp's anonymous namespace after bk_sprite_group_cases.inc.
// L14-VR03: the actual translated guest option-box text owner (bk_text.inc) emits exactly the words of the former
// native owner (menu_text.hpp, the reference). The ortho reciprocal is guest integer arithmetic: every framebuffer
// width and height the owner admits (1..2048) is compared bit for bit with the native float division. Glyph quads
// are compared over signed span x scale products (arithmetic right shift) and every s16 refusal edge, with fallback
// and close order. Synthetic words and strings only.
namespace text {
namespace mt=tooie_menu_text;
constexpr uint32_t frames=bk+TOOIE_BK_DIAGNOSTICS_OFFSET;
constexpr uint32_t cc=TOOIE_CAMERA_STATE_VRAM+offsetof(TooieCameraState,control);
constexpr uint32_t vector_global=0x80128180u, vec=0x80330000u, first=0x80330100u, record=first, chars=0x80331000u;
constexpr uint32_t fb_width=0x80127750u, fb_height=0x80127752u;
constexpr uint32_t list=cursor0, base=mt::first_id, glyph_sp=0x807F0000u;
static_assert(chars+0x200u<0x80801000u && list>=0x80300000u,"synthetic vector, string and list lie below the patch image");
uint32_t frame_value=0;
void put_half(uint8_t* rdram,uint32_t a,uint16_t v){MEM_H(0,guest(a))=int16_t(v);}
void setup(uint8_t* rdram){
    put(rdram,cc+offsetof(TooieCameraControl,active),1);
    put(rdram,cc+offsetof(TooieCameraControl,main_base),0x80300000u);
    put(rdram,cc+offsetof(TooieCameraControl,main_end),0x80340000u);
    put(rdram,vector_global,vec);put(rdram,vec,0x14u);put(rdram,vec+4,first);put(rdram,vec+8,first+4u*0x14u);
    put(rdram,record,0x00100020u);put(rdram,record+0xC,chars);
}
void set_string(uint8_t* rdram,uint32_t bytes){for(uint32_t i=0;i<bytes;++i)put_byte(rdram,chars+i,'A');put_byte(rdram,chars+bytes,0);}
// One owned record admitted in a fresh construction; returns the cursor after its open bracket.
uint32_t admit(uint8_t* rdram,uint32_t width,uint32_t height,uint32_t bytes,uint32_t cut=0){
    put(rdram,frames,++frame_value);put_half(rdram,fb_width,uint16_t(width));put_half(rdram,fb_height,uint16_t(height));
    set_string(rdram,bytes);
    require(helper(rdram,candidate::tooie_bk_text_scope,1,base,cut)==1,"admitted option box opens the text scope");
    require(helper(rdram,candidate::tooie_bk_text_pushed,record)==1,"a push inside the scope is owned");
    helper(rdram,candidate::tooie_bk_text_scope,0);
    put(rdram,buffers,list);
    helper(rdram,candidate::tooie_bk_text_enter,buffers);
    require(helper(rdram,candidate::tooie_bk_text_record,record)==2,"the owned record is admitted");
    return word(rdram,buffers);
}
unsigned reciprocal_cases(uint8_t* rdram){
    for(uint32_t w=1;w<=2048u;++w){
        const uint32_t h=2049u-w, after=admit(rdram,w,h,1);
        require(after==list+4u*mt::open_words,"open bracket length");
        uint32_t ref[mt::open_words];mt::open(ref,list,w,h);
        for(uint32_t i=0;i<mt::open_words;++i)
            require(word(rdram,list+4u*i)==ref[i],"guest bracket equals the native words, ortho reciprocal bits included");
        helper(rdram,candidate::tooie_bk_text_close,1);
        uint32_t closing[mt::close_words];mt::close(closing);
        for(uint32_t i=0;i<mt::close_words;++i)require(word(rdram,after+4u*i)==closing[i],"guest close words");
        require(word(rdram,buffers)==after+4u*mt::close_words,"one close");
    }
    return 1;
}
unsigned glyph_cases(uint8_t* rdram){
    const int32_t spans[4]={1,7,32,4095};
    const int32_t scales[11]={-32768,-1025,-129,-128,-1,0,1,127,128,1024,32767};
    const int32_t sts[5]={-32768,-1,0,1,32767};
    unsigned converted=0,refused=0;
    for(int32_t span:spans)for(uint32_t di=0;di<11u;++di)for(uint32_t si=0;si<5u;++si){
        const int32_t origin=span==4095?0:400,ulx=origin,uly=origin,lrx=ulx+span,lry=uly+span;
        const int32_t s=sts[si],t=sts[(si+2u)%5u],dsdx=scales[di],dtdy=scales[(di+5u)%11u];
        const uint32_t rect=admit(rdram,320,240,1);
        const uint32_t w[6]={0xE4000000u|(uint32_t(lrx)<<12)|uint32_t(lry),(uint32_t(ulx)<<12)|uint32_t(uly),0xE1000000u,
            (uint32_t(uint16_t(int16_t(s)))<<16)|uint16_t(int16_t(t)),0xF1000000u,
            (uint32_t(uint16_t(int16_t(dsdx)))<<16)|uint16_t(int16_t(dtdy))};
        for(uint32_t i=0;i<6u;++i)put(rdram,rect+4u*i,w[i]);
        put(rdram,buffers,rect+24u);                                   // B62CC emitted its triple
        helper(rdram,candidate::tooie_bk_text_mark,glyph_sp,rect+16u);
        const uint32_t done=helper(rdram,candidate::tooie_bk_text_glyph,glyph_sp,buffers);
        mt::Rect r;uint32_t v[16];
        const bool ok=mt::decode(w,r)&&mt::quad(r,320,240,v);
        require(done==uint32_t(ok),"guest conversion decision equals the native decode/quad decision");
        if(ok){
            uint32_t ref[mt::glyph_words];mt::glyph(ref,rect+24u,v,TOOIE_BK_FLOAT,base,0);
            for(uint32_t i=0;i<6u;++i)require(!word(rdram,rect+4u*i),"original triple silenced in place");
            for(uint32_t i=0;i<mt::glyph_words;++i)require(word(rdram,rect+24u+4u*i)==ref[i],"guest glyph words equal native");
            require(word(rdram,buffers)==rect+24u+4u*mt::glyph_words,"cursor after one glyph");++converted;
        }else{
            for(uint32_t i=0;i<6u;++i)require(word(rdram,rect+4u*i)==w[i],"refused rectangle stays original");
            require(word(rdram,buffers)==rect+24u,"refused glyph emits nothing");++refused;
        }
        helper(rdram,candidate::tooie_bk_text_close,1);
    }
    require(converted&&refused,"the sweep covers converted and s16-refused rectangles");
    return 1;
}
unsigned fallback_cases(uint8_t* rdram){
    // Unowned: a push outside any scope is not owned and its record stays original.
    put(rdram,frames,++frame_value);set_string(rdram,1);
    require(!helper(rdram,candidate::tooie_bk_text_pushed,record),"a push outside any scope is not owned");
    put(rdram,buffers,list);helper(rdram,candidate::tooie_bk_text_enter,buffers);
    require(!helper(rdram,candidate::tooie_bk_text_record,record)&&word(rdram,buffers)==list,"an unowned record emits nothing");
    helper(rdram,candidate::tooie_bk_text_close,1);
    // A foreign gfx argument converts nothing; the record's own converts, with the cut SkipAll policy.
    const uint32_t at=admit(rdram,320,240,1,1);
    const uint32_t w[6]={0xE4000000u|(432u<<12)|232u,(400u<<12)|200u,0xE1000000u,0u,0xF1000000u,0x04000400u};
    for(uint32_t i=0;i<6u;++i)put(rdram,at+4u*i,w[i]);
    put(rdram,buffers,at+24u);
    helper(rdram,candidate::tooie_bk_text_mark,glyph_sp,at+16u);
    require(!helper(rdram,candidate::tooie_bk_text_glyph,glyph_sp,buffers+8u)&&word(rdram,at)==w[0]&&word(rdram,buffers)==at+24u,
        "a foreign gfx buffers argument converts nothing");
    helper(rdram,candidate::tooie_bk_text_mark,glyph_sp,at+16u);
    require(helper(rdram,candidate::tooie_bk_text_glyph,glyph_sp,buffers)==1,"its own gfx buffers argument converts");
    mt::Rect r;uint32_t v[16];require(mt::decode(w,r)&&mt::quad(r,320,240,v),"reference rectangle");
    uint32_t ref[mt::glyph_words];mt::glyph(ref,at+24u,v,TOOIE_BK_FLOAT,base,1);
    for(uint32_t i=0;i<mt::glyph_words;++i)require(word(rdram,at+24u+4u*i)==ref[i],"cut record glyph equals the native SkipAll words");
    // Flush (B7940) closes exactly once with the native close words; leave (B7950) adds nothing.
    const uint32_t tail=word(rdram,buffers);
    helper(rdram,candidate::tooie_bk_text_close,0);
    uint32_t closing[mt::close_words];mt::close(closing);
    for(uint32_t i=0;i<mt::close_words;++i)require(word(rdram,tail+4u*i)==closing[i],"flush close words");
    require(word(rdram,buffers)==tail+32u,"flush closes once");
    helper(rdram,candidate::tooie_bk_text_close,1);
    require(word(rdram,buffers)==tail+32u,"a closed bracket is not closed again");
    return 1;
}
unsigned text_cases(){
    uint8_t* rdram=fresh();setup(rdram);
    return reciprocal_cases(rdram)+glyph_cases(rdram)+fallback_cases(rdram);
}
}
