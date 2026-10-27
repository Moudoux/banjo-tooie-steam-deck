/* ---- Option-box text owner (L14-VR03) -------------------------------------------------------------------
 * BK dbd52154 patches/text_transform_tagging.c: the drawer's text ID is stored per print-buffer slot at enqueue
 * (_printbuffer_push_new 71-93); each letter is drawn as an orthographic quad with its own MODELVIEW load and
 * SimpleVerts group (_printbuffer_draw_letter 337-385) instead of the original texture rectangle (388), with
 * G_TP_PERSP for the quads (printbuffer_draw 418) and 64 texcoord units per texel (354-372).
 * @tooie Seams, identities, bounds and words are the former native owner's: menu_text.hpp is its reference model and
 * bk_text_cases.inc proves these bodies emit the same words. The native thunks (bk_menu_text_thunks.hpp) forward
 * registers and the native menu occurrence identity only. Records are owned by vector index within one construction
 * and one vector lifetime (B5E04 free; B5E3C defrag keeps indices), authenticated by their own words; each rectangle
 * is decoded from its emitted words, so B62CC's clip/scale stays authoritative. No float arithmetic (this object has
 * no FP constant pool): the ortho reciprocal is formed exactly with integers. Defined last (BK_HELPERS order). */
#define TOOIE_BK_TEXT_MAGIC 0x424B5431u      /* 'BKT1' */
#define TOOIE_BK_TEXT_LAYOUT 1u
#define BKT_RECORDS 64u                      /* menu_text.hpp owned_records */
#define BKT_LINES 2u
#define BKT_GLYPHS 128u
#define BKT_VECTOR 0x80128180u               /* D_80128180: +0 stride 0x14, +4 begin, +8 end */
#define BKT_STRIDE 0x14u
#define BKT_FB_WIDTH 0x80127750u             /* s16 framebuffer dimensions (SetColorImage/scissor) */
#define BKT_FB_HEIGHT 0x80127752u
#define BKT_BUFFERS 0x80127658u              /* A7180 global graphics buffers */
#define BKT_FRAME_CAP 24576u                 /* converted bytes per construction */
#define BKT_PROJECTION_ID 0x30230000u        /* menu_text.hpp projection_id */
#define BKT_OPEN_WORDS 40u
#define BKT_GLYPH_WORDS 36u
#define BKT_CLOSE_WORDS 8u
#define BKT_SIMPLE_VERTS 0x0240AAA9u         /* menu_text.hpp simple_verts */
#define BKT_SKIP_ALL 1u
#define BKT_PROJECTION_ADJUST 0x00200003u
struct TooieBkTextOwner { u32 index, frame, generation, string, xy, base, cut; };
struct TooieBkText {
    u32 magic, layout;
    struct TooieBkTextOwner owners[BKT_RECORDS];
    u32 count, frame, generation, pending_base, pending_cut, pending_lines;   /* enqueue ownership, open scope */
    u32 draw_base, draw_cut, draw_glyph, draw_open, mark_sp, mark_rect;      /* one B7448 traversal */
    u32 buffers, spent, spent_frame, width, height, left;
};
_Static_assert(sizeof(struct TooieBkText) == 1872u, "BK option-text owner layout1");
volatile struct TooieBkText tooie_bk_text __attribute__((section(".tooie_bk_text"), aligned(4))) = {
    .magic = TOOIE_BK_TEXT_MAGIC, .layout = TOOIE_BK_TEXT_LAYOUT,
};
#define BKT tooie_bk_text

/* bk_menu_thunks.hpp camera_main: the camera construction is active and unlatched, no redirected fill. */
SI u32 bkt_camera_main(void) {
    return tooie_camera_state.control.active && !tooie_camera_state.control.latch && !*(volatile u32*)0x8012D508u;
}
SI void bkt_construction(u32 frame) { if (frame != BKT.frame) { BKT.frame = frame; BKT.count = 0; } }
SI void bkt_copy(u32 to, u32 from) {
    volatile struct TooieBkTextOwner* a = &BKT.owners[to];
    volatile struct TooieBkTextOwner* b = &BKT.owners[from];
    a->index = b->index; a->frame = b->frame; a->generation = b->generation; a->string = b->string;
    a->xy = b->xy; a->base = b->base; a->cut = b->cut;
}
/* The vector index of `record`, or 0xFFFFFFFF when it is not an element of the live vector. */
SI u32 bkt_index(u32 record) {
    u32 vec = *(volatile u32*)BKT_VECTOR, stride, begin, end;
    if (vec < 0x80000000u || vec > 0x80FFFFF0u || (vec & 3u)) return 0xFFFFFFFFu;
    stride = *(volatile u32*)vec; begin = *(volatile u32*)(vec + 4u); end = *(volatile u32*)(vec + 8u);
    if (stride != BKT_STRIDE || begin < 0x80000000u || end < begin || end > 0x81000000u ||
        record < begin || record >= end || (record - begin) % BKT_STRIDE) return 0xFFFFFFFFu;
    return (record - begin) / BKT_STRIDE;
}
/* menu_text.hpp string_bytes: one glyph per byte, at most one line, never reading at or past 0x81000000. */
SI u32 bkt_string_bytes(u32 string) {
    u32 n = 0;
    if (string < 0x80000000u || string >= 0x81000000u) return 0;
    while (n <= BKT_GLYPHS) {
        if (string + n >= 0x81000000u) return 0;
        if (!*(volatile u8*)(string + n)) return n;
        ++n;
    }
    return 0;
}
/* bk_menu_text_thunks.hpp room: the camera adapters' allowance and the full original MAIN allowance stay behind it. */
SI u32 bkt_room(u32 cursor, u32 bytes) {
    u32 base = tooie_camera_state.control.main_base, end = tooie_camera_state.control.main_end;
    u32 keep = TOOIE_CAMERA_EXTRA_BYTES + TOOIE_BRIDGE_ORIGINAL_MAIN_BYTES;
    return !(cursor & 7u) && cursor >= base && end >= base + keep && cursor <= end - keep &&
        bytes <= end - keep - cursor && BKT.spent <= BKT_FRAME_CAP && bytes <= BKT_FRAME_CAP - BKT.spent;
}
/* IEEE-754 single bits of 1/d, 2 <= d <= 4096: the correctly rounded quotient that the former native
 * 1.0f/float(d) produced (exact operand, one round-to-nearest division). Integer restoring division only. */
SI u32 bkt_reciprocal(u32 d) {
    u32 e = 0, rem = 1u, q = 0, i, twice;
    while ((2u << e) <= d) ++e;                       /* 2^e <= d < 2^(e+1) */
    if (d == (1u << e)) return (127u - e) << 23;      /* exact power of two */
    for (i = 0; i < e + 24u; ++i) {                   /* q = floor(2^(e+24) / d), 2^23 < q < 2^24 */
        rem <<= 1; q <<= 1;
        if (rem >= d) { rem -= d; q |= 1u; }
    }
    twice = rem << 1;
    if (twice > d || (twice == d && (q & 1u))) ++q;  /* nearest, ties to even */
    if (q == (1u << 24)) return (127u - e) << 23;
    return ((126u - e) << 23) | (q & 0x7FFFFFu);
}
SI u32 bkt_s16(s32 v) { return v >= -32768 && v <= 32767; }
/* menu_text.hpp open(): the record bracket with its inline viewport and float ortho matrix. */
SI void bkt_open(u32 at, u32 width, u32 height) {
    volatile u32* w = (volatile u32*)at;
    u32 vp = at + 8u, mtx = vp + 16u, after = mtx + 64u, sx = 2u * width, sy = 2u * height;
    w[0] = 0xDE010000u; w[1] = after;
    w[2] = (sx << 16) | sy; w[3] = 0x01FF0000u; w[4] = (sx << 16) | sy; w[5] = 0x01FF0000u;
    w[6] = bkt_reciprocal(sx); w[7] = 0; w[8] = 0; w[9] = 0;
    w[10] = 0; w[11] = bkt_reciprocal(sy); w[12] = 0; w[13] = 0;
    w[14] = 0; w[15] = 0; w[16] = 0x3F800000u; w[17] = 0;
    w[18] = 0; w[19] = 0; w[20] = 0; w[21] = 0x3F800000u;
    w[22] = 0x64000015u; w[23] = 0;
    w[24] = 0xDC080008u; w[25] = vp;
    w[26] = 0x6400001Du; w[27] = 0;
    w[28] = 0x6400000Cu; w[29] = BKT_PROJECTION_ID; w[30] = BKT_PROJECTION_ADJUST; w[31] = 0;
    w[32] = 0x64000030u; w[33] = 7u; w[34] = 0; w[35] = mtx;
    w[36] = 0x64000029u; w[37] = 0;
    w[38] = 0xD9F0F9FEu; w[39] = 0x00200004u;
}
/* bk_menu_text_thunks.hpp close_open with menu_text.hpp close(): charged when the record was admitted. */
SI void bkt_close_open(void) {
    u32 cursor; volatile u32* w;
    if (!BKT.draw_open || !BKT.buffers) return;
    cursor = *(volatile u32*)BKT.buffers;
    w = (volatile u32*)cursor;
    w[0] = 0x6400002Au; w[1] = 0; w[2] = 0x6400000Du; w[3] = 1u | (1u << 8);
    w[4] = 0x6400001Eu; w[5] = 0; w[6] = 0x64000016u; w[7] = 0;
    *(volatile u32*)BKT.buffers = cursor + 4u * BKT_CLOSE_WORDS;
    BKT.draw_open = 0; BKT.draw_base = 0;
}

/* gczoombox 80800FE0 entry (opening) and entry2 80801928 (closing). `base`/`cut` are the native menu owner's
 * occurrence identity (0 when not its admitted option box). Returns 1 when the text scope opened. */
BK_HELPER u32 tooie_bk_text_scope(u32 opening, u32 base, u32 cut) {
    BKT.pending_base = 0; BKT.pending_cut = 0; BKT.pending_lines = 0;
    if (!opening) return 0;
    bkt_construction(BK.diagnostics.frames);
    if (!base || !bkt_camera_main() || BK.owner.kind != TOOIE_BK_OWNER_NONE || BK.draw_depth || BK.root_depth) return 0;
    BKT.pending_base = base; BKT.pending_cut = cut; BKT.pending_lines = 0;
    return 1;
}
/* B5E04 entry: no ownership survives into the next vector, even at the same index with the same words. */
BK_HELPER void tooie_bk_text_freed(void) { BKT.count = 0; BKT.generation += 1u; }
/* B798C at B7A84 (record = v0, its x/y and string stored). Returns 1 when a line was owned. */
BK_HELPER u32 tooie_bk_text_pushed(u32 record) {
    u32 index, i;
    volatile struct TooieBkTextOwner* o;
    if (!BKT.pending_base) return 0;
    bkt_construction(BK.diagnostics.frames);
    index = bkt_index(record);
    if (index == 0xFFFFFFFFu || BKT.pending_lines >= BKT_LINES) return 0;
    for (i = 0; i < BKT.count; ++i)   /* an index reused within the construction replaces its stale owner */
        if (BKT.owners[i].index == index) { BKT.count -= 1u; bkt_copy(i, BKT.count); break; }
    if (BKT.count >= BKT_RECORDS) return 0;
    o = &BKT.owners[BKT.count];
    o->index = index; o->frame = BKT.frame; o->generation = BKT.generation;
    o->string = *(volatile u32*)(record + 0xCu); o->xy = *(volatile u32*)record;
    o->base = BKT.pending_base + BKT.pending_lines * BKT_GLYPHS; o->cut = BKT.pending_cut;
    BKT.count += 1u; BKT.pending_lines += 1u;
    return 1;
}
/* B7448 entry (a0 = gfx buffers): a new traversal of the camera MAIN list only. */
BK_HELPER void tooie_bk_text_enter(u32 buffers) {
    u32 f = BK.diagnostics.frames;
    if (f != BKT.spent_frame) { BKT.spent_frame = f; BKT.spent = 0; }
    BKT.draw_base = 0; BKT.draw_cut = 0; BKT.draw_glyph = 0; BKT.draw_open = 0; BKT.mark_sp = 0; BKT.mark_rect = 0;
    BKT.left = 0;
    BKT.buffers = buffers == BKT_BUFFERS && bkt_camera_main() ? buffers : 0u;
}
/* B74EC loop header (record = s1): close the previous bracket, then admit this record or leave it original.
 * Returns 0 unowned, 1 owned but not admitted, 2 admitted (bracket opened, whole record charged). */
BK_HELPER u32 tooie_bk_text_record(u32 record) {
    u32 index, string, i, bytes, cursor, need;
    s32 w, h;
    volatile struct TooieBkTextOwner* o = 0;
    bkt_close_open();
    BKT.draw_base = 0; BKT.draw_cut = 0; BKT.draw_glyph = 0; BKT.left = 0;
    if (!BKT.buffers) return 0;
    index = bkt_index(record);
    if (index == 0xFFFFFFFFu) return 0;
    string = *(volatile u32*)(record + 0xCu);
    for (i = 0; i < BKT.count; ++i)   /* menu_text.hpp owner(): the first owner at this index decides */
        if (BKT.owners[i].index == index) {
            if (BKT.owners[i].frame == BK.diagnostics.frames && BKT.owners[i].generation == BKT.generation &&
                BKT.owners[i].string == string && BKT.owners[i].xy == *(volatile u32*)record) o = &BKT.owners[i];
            break;
        }
    if (!o) return 0;
    bytes = bkt_string_bytes(string);
    w = *(volatile s16*)BKT_FB_WIDTH; h = *(volatile s16*)BKT_FB_HEIGHT;
    if (!bytes || w <= 0 || h <= 0 || w > 2048 || h > 2048) return 1;
    cursor = *(volatile u32*)BKT.buffers;
    need = 4u * (BKT_OPEN_WORDS + BKT_CLOSE_WORDS + bytes * BKT_GLYPH_WORDS);
    if (!bkt_room(cursor, need)) return 1;
    bkt_open(cursor, (u32)w, (u32)h);
    *(volatile u32*)BKT.buffers = cursor + 4u * BKT_OPEN_WORDS;
    BKT.spent += need; BKT.left = need - 4u * (BKT_OPEN_WORDS + BKT_CLOSE_WORDS);
    BKT.width = (u32)w; BKT.height = (u32)h;
    BKT.draw_open = 1; BKT.draw_base = o->base; BKT.draw_cut = o->cut;
    return 2;
}
/* B62CC at B7144 (normal emission only): this activation's sp and v1 = triple + 16. */
BK_HELPER void tooie_bk_text_mark(u32 sp, u32 v1) { BKT.mark_sp = sp; BKT.mark_rect = v1 - 16u; }
/* B62CC at B7164 (also every early out): convert this activation's normally emitted rectangle of an admitted
 * record while its charge lasts. `buffers` is B62CC's own gfx buffers argument. Returns 1 when converted. */
BK_HELPER u32 tooie_bk_text_glyph(u32 sp, u32 buffers) {
    u32 rect = BKT.mark_rect, marked = BKT.mark_sp == sp, cursor, w0, w1, w3, w5, k;
    s32 ulx, uly, lrx, lry, s, t, dsdx, dtdy, xs[2], ys[2], ss[2], ts[2];
    volatile u32* r; volatile u32* o;
    BKT.mark_sp = 0; BKT.mark_rect = 0;
    if (!marked || !BKT.draw_open || !BKT.draw_base || BKT.draw_glyph >= BKT_GLYPHS ||
        BKT.left < BKT_GLYPH_WORDS * 4u) return 0;
    if (buffers != BKT.buffers) return 0;
    cursor = *(volatile u32*)BKT.buffers;
    if (cursor != rect + 24u) return 0;                     /* nothing emitted after the triple */
    r = (volatile u32*)rect;
    w0 = r[0]; w1 = r[1]; w3 = r[3]; w5 = r[5];
    /* menu_text.hpp decode(): G_TEXRECT, G_RDPHALF_1, G_RDPHALF_2; tile 0; non-empty. */
    if ((w0 >> 24) != 0xE4u || r[2] != 0xE1000000u || r[4] != 0xF1000000u) return 0;
    lrx = (s32)((w0 >> 12) & 0xFFFu); lry = (s32)(w0 & 0xFFFu);
    ulx = (s32)((w1 >> 12) & 0xFFFu); uly = (s32)(w1 & 0xFFFu);
    if (((w1 >> 24) & 7u) || lrx <= ulx || lry <= uly) return 0;
    s = (s16)(w3 >> 16); t = (s16)(w3 & 0xFFFFu); dsdx = (s16)(w5 >> 16); dtdy = (s16)(w5 & 0xFFFFu);
    /* menu_text.hpp quad(): every term fits s32 (|span*scale| <= 4095*32768 < 2^31); >> is arithmetic in both. */
    xs[0] = ulx - 2 * (s32)BKT.width; xs[1] = lrx - 2 * (s32)BKT.width;
    ys[0] = 2 * (s32)BKT.height - uly; ys[1] = 2 * (s32)BKT.height - lry;
    ss[0] = 2 * s; ss[1] = 2 * (s + (((lrx - ulx) * dsdx) >> 7));
    ts[0] = 2 * t; ts[1] = 2 * (t + (((lry - uly) * dtdy) >> 7));
    for (k = 0; k < 2u; ++k)
        if (!bkt_s16(xs[k]) || !bkt_s16(ys[k]) || !bkt_s16(ss[k]) || !bkt_s16(ts[k])) return 0;
    /* menu_text.hpp glyph(): inline vertices, identity MODELVIEW LOAD|PUSH, group, G_TP_PERSP, quad, G_TP_NONE, pops. */
    o = (volatile u32*)cursor;
    o[0] = 0xDE010000u; o[1] = cursor + 8u + 64u;
    for (k = 0; k < 4u; ++k) {
        u32 ix = k & 1u, iy = k >> 1;
        o[2u + k * 4u] = ((u32)(u16)xs[ix] << 16) | (u16)ys[iy];
        o[3u + k * 4u] = 0;
        o[4u + k * 4u] = ((u32)(u16)ss[ix] << 16) | (u16)ts[iy];
        o[5u + k * 4u] = 0xFFFFFFFFu;
    }
    o[18] = 0xDA380002u; o[19] = (u32)&BKF.identity[0];
    o[20] = 0x6400000Cu; o[21] = BKT.draw_base + BKT.draw_glyph; o[22] = BKT.draw_cut ? BKT_SKIP_ALL : BKT_SIMPLE_VERTS; o[23] = 0;
    o[24] = 0xE3000C00u; o[25] = 0x00080000u;
    o[26] = 0x01004008u; o[27] = cursor + 8u;
    o[28] = 0x07000206u; o[29] = 0x00000604u;
    o[30] = 0xE3000C00u; o[31] = 0;
    o[32] = 0x6400000Du; o[33] = 1u;
    o[34] = 0xD8380002u; o[35] = 64u;
    for (k = 0; k < 6u; ++k) r[k] = 0;                      /* the original triple becomes G_NOOPs */
    *(volatile u32*)BKT.buffers = cursor + 4u * BKT_GLYPH_WORDS;
    BKT.left -= 4u * BKT_GLYPH_WORDS; BKT.draw_glyph += 1u;
    return 1;
}
/* B7448 at B7940 (leave 0: close the last bracket) and its common exit B7950 (leave 1: indices restart). */
BK_HELPER void tooie_bk_text_close(u32 leave) {
    bkt_close_open();
    if (!leave) return;
    BKT.count = 0;
    BKT.draw_base = 0; BKT.draw_cut = 0; BKT.draw_glyph = 0; BKT.draw_open = 0; BKT.mark_sp = 0; BKT.mark_rect = 0;
    BKT.buffers = 0; BKT.left = 0;
}
