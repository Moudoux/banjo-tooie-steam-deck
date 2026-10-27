// Included at the end of bk_menu_thunks.hpp (TOOIE_BK_PIPELINE_SELECTED), after bk_menu_sprite_thunks.hpp.
/* ---- Option-box text (BK dbd52154 patches/text_transform_tagging.c parity, option boxes only) ---------------
 * zoombox entry2 calls 80800FE0 (a0 = box) at 80801920, returning at 80801928 before its model. 80800FE0 enqueues its
 * text lines with core B798C (JALs 808012C8 / 80801314): a vector whose pointer is D_80128180 (+0 stride 0x14, +4 begin,
 * +8 end), record +0 s16 x, +2 s16 y, +C string. B798C stores x (B79E4), y (B79F0) and the string (B79FC) after
 * push_back returns, so the record is identified at B7A84 (non-null path only, before the final alpha store to
 * record+13; v0 still the record). Core B5E04 frees the vector (vector_free at B5E1C, global zeroed at B5E28); B5E3C
 * defrags it keeping contents and indices. Core B7448 (sole caller A73F4, gfx = 80127658) traverses it (record s1 at
 * the B74EC loop header), calls B62CC per glyph (B78E8), and on a consumed vector cleans up at B7940 (E7A4C) and clears
 * it before the common exit B7950. B62CC writes each glyph's G_TEXRECT triple by B713C (v1 = triple + 16) and
 * publishes the cursor at B7160; B7164 is its common exit (early outs included). RT64 never matches or interpolates
 * rectangles, so option text stepped at the game rate while its box model and sprite glide.
 * BK stores the drawer's text ID per print-buffer slot at enqueue and redraws each letter as an ortho quad with its own
 * MODELVIEW load and SimpleVerts group. @tooie L14-VR03: the guest owner (bk_text.inc) makes every decision and write
 * described below; these thunks forward registers (and the native menu occurrence identity) and count outcomes.
 * Here: owned records are those pushed while 80800FE0 runs for the current
 * option box occurrence; each is keyed by its vector index in its construction and vector lifetime and authenticated
 * by its own words. An owned record is admitted at B74EC with the bracket plus one glyph per string byte charged
 * first; its normally emitted rectangles are decoded from their own words, silenced in place and redrawn as the
 * equivalent quads (menu_text.hpp) while that charge lasts. B7448 may call B62CC beyond the string length (format-state
 * continuation), so a formatted record is not necessarily converted whole: past the charge, or past 128 glyphs, its
 * rectangles stay original. Unowned records, early-out glyphs, refused admission and any unexpected word stay exactly
 * original; an original rectangle left inside a bracket draws identically (the bracket changes no rectangle state). */
#include "menu_text.hpp"
// L14-VR03: guest option-box text owner (bk_text.inc). Each thunk forwards its seam's registers once.
extern "C" void tooie_bk_text_scope(uint8_t*,recomp_context*);
extern "C" void tooie_bk_text_freed(uint8_t*,recomp_context*);
extern "C" void tooie_bk_text_pushed(uint8_t*,recomp_context*);
extern "C" void tooie_bk_text_enter(uint8_t*,recomp_context*);
extern "C" void tooie_bk_text_record(uint8_t*,recomp_context*);
extern "C" void tooie_bk_text_mark(uint8_t*,recomp_context*);
extern "C" void tooie_bk_text_glyph(uint8_t*,recomp_context*);
extern "C" void tooie_bk_text_close(uint8_t*,recomp_context*);
namespace bk_text {
static uint32_t forward(uint8_t* rdram,recomp_context* ctx,void(*helper)(uint8_t*,recomp_context*),
                        uint32_t a0=0,uint32_t a1=0,uint32_t a2=0){
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=(gpr)(int32_t)a0;clone.r5=(gpr)(int32_t)a1;clone.r6=(gpr)(int32_t)a2;
    helper(rdram,&clone);
    return uint32_t(clone.r2);
}
}
// gczoombox 80800FE0 entry (a0 = box): its B798C pushes until 80801928 are this box occurrence's text lines. The native
// menu owner supplies this occurrence's text ID base and cut (base 0 when it is not the admitted option box).
extern "C" void baseline_bk_menu_text_begin(uint8_t* rdram,recomp_context* ctx){
    const auto& m=bk_menu::live;
    const bool box=m.box && m.box==uint32_t(ctx->r4) && m.state.active.valid;
    const uint32_t base=box?tooie_menu_text::occurrence_base(m.state,m.state.active.gfx):0u;
    if(bk_text::forward(rdram,ctx,tooie_bk_text_scope,1u,base,box?uint32_t(m.state.active.cut):0u))
        tooie_menu_witness::hit(tooie_menu_witness::TextScope);
}
// gczoombox entry2 at 80801928, 80800FE0's return continuation: the text scope ends before the model.
extern "C" void baseline_bk_menu_text_end(uint8_t* rdram,recomp_context* ctx){bk_text::forward(rdram,ctx,tooie_bk_text_scope,0u);}
// B5E04 entry, before vector_free (B5E1C) and the global's zeroing (B5E28).
extern "C" void baseline_bk_text_freed(uint8_t* rdram,recomp_context* ctx){bk_text::forward(rdram,ctx,tooie_bk_text_freed);}
// B798C at B7A84 (non-null string path, before the final alpha store): v0 is still the new record.
extern "C" void baseline_bk_text_pushed(uint8_t* rdram,recomp_context* ctx){
    if(bk_text::forward(rdram,ctx,tooie_bk_text_pushed,uint32_t(ctx->r2)))tooie_menu_witness::hit(tooie_menu_witness::TextPushed);
}
// B7448 entry (a0 = gfx buffers).
extern "C" void baseline_bk_text_enter(uint8_t* rdram,recomp_context* ctx){bk_text::forward(rdram,ctx,tooie_bk_text_enter,uint32_t(ctx->r4));}
// B74EC, the loop header reached once per record (record = s1): 1 owned, 2 owned and admitted.
extern "C" void baseline_bk_text_record(uint8_t* rdram,recomp_context* ctx){
    const uint32_t status=bk_text::forward(rdram,ctx,tooie_bk_text_record,uint32_t(ctx->r17));
    if(status)tooie_menu_witness::hit(tooie_menu_witness::TextOwned);
    if(status==2u)tooie_menu_witness::hit(tooie_menu_witness::TextAdmitted);
}
// B62CC at B7144 (normal emission only): v1 = triple + 16.
extern "C" void baseline_bk_text_glyph_emitted(uint8_t* rdram,recomp_context* ctx){
    bk_text::forward(rdram,ctx,tooie_bk_text_mark,uint32_t(ctx->r29),uint32_t(ctx->r3));
}
// B62CC at B7164 (also every early out): its own sp and its gfx buffers argument (frame word sp+0x150).
extern "C" void baseline_bk_text_glyph(uint8_t* rdram,recomp_context* ctx){
    if(bk_text::forward(rdram,ctx,tooie_bk_text_glyph,uint32_t(ctx->r29),uint32_t(MEM_W(0x150,ctx->r29))))
        tooie_menu_witness::hit(tooie_menu_witness::TextConverted);
}
// B7448 at B7940 (consumed non-empty vector, before its own cleanup): close the last bracket.
extern "C" void baseline_bk_text_flush(uint8_t* rdram,recomp_context* ctx){bk_text::forward(rdram,ctx,tooie_bk_text_close,0u);}
// B7448 common exit B7950: indices restart; no bracket survives.
extern "C" void baseline_bk_text_leave(uint8_t* rdram,recomp_context* ctx){bk_text::forward(rdram,ctx,tooie_bk_text_close,1u);}
