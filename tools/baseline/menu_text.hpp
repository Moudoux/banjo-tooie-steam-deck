#ifndef TOOIE_MENU_TEXT_HPP
#define TOOIE_MENU_TEXT_HPP
#include <stdint.h>
#include <string.h>
#include "menu_scope.hpp"
/* Option-box text owner: pure ownership state and exact command words, no guest memory access. @tooie L14-VR03:
 * the guest owner (bk_text.inc) now makes these decisions and writes; this native model is its reference
 * (bk_text_cases.inc compares the guest words with it). occurrence_base remains the native menu owner's identity.
 * BK dbd52154bc1e00e56867b1fac94a8f907052f41b patches/text_transform_tagging.c: the drawer's text ID is stored per
 * print-buffer slot at enqueue (_printbuffer_push_new 71-93); each letter is drawn as an orthographic quad with its
 * own MODELVIEW load and SimpleVerts group (_printbuffer_draw_letter 337-385) instead of the original texture
 * rectangle (388), with G_TP_PERSP for the quads (printbuffer_draw 418) and 64 texcoord units per texel (354-372).
 * @tooie Only records enqueued by an admitted option-box occurrence are converted. The Tooie queue is a vector
 * (B798C push, B7448 traversal, B62CC G_TEXRECT); records are identified by their vector index within one
 * construction and one vector lifetime (B5E04 frees it; B5E3C defrag keeps contents and indices), and authenticated
 * by their own string/x/y words as B798C finally wrote them. Each rectangle is decoded from its emitted words, so
 * B62CC's clip/scale logic stays authoritative. B7448 establishes RSP texture ON/tile0/level0/scale 0x8000 before its
 * records (D_80123310) and G_TP_NONE (B74A8); B62CC changes neither, so quads keep that texture state and restore
 * G_TP_NONE after each quad. */
namespace tooie_menu_text {
// IDs: one block per menu box occurrence (BK: drawer ID x 256), two lines of 128 glyphs.
constexpr uint32_t lines = 2, glyphs = 128, block = lines * glyphs;
constexpr uint32_t occurrences = tooie_menu::slots * tooie_menu::invocations * tooie_menu::boxes * tooie_menu::roles;
constexpr uint32_t first_id = 0x30200000u;
constexpr uint32_t end_id = first_id + occurrences * block;
constexpr uint32_t projection_id = end_id;   // the single text projection group (static; SkipAll, ADJUST)
static_assert(occurrences == 3u * 2u * 32u * 4u && end_id == 0x30230000u && projection_id < 0x30240000u,
    "every slot x invocation x box x role x line x glyph fits 0x30200000..0x30230000, then one projection ID");
inline uint32_t occurrence_base(const tooie_menu::State& s, uint32_t gfx) {
    const auto& c = s.active;
    if (!c.valid || c.gfx != gfx || c.slot >= tooie_menu::slots || c.invocation >= tooie_menu::invocations ||
        c.box >= tooie_menu::boxes || c.role >= tooie_menu::roles) return 0;
    return first_id + (((c.slot * tooie_menu::invocations + c.invocation) * tooie_menu::boxes + c.box) *
        tooie_menu::roles + c.role) * block;
}

// ---- Enqueue ownership (BK print_sPrintBufferIDs) ------------------------------------------------------
constexpr uint32_t owned_records = 64;   // bounded; a record beyond it stays an original rectangle
struct Owner { uint32_t index, frame, generation, string, xy, base, cut; };
struct Queue {
    Owner owners[owned_records]{};
    uint32_t count = 0, frame = 0, generation = 0;   // generation: one per vector lifetime (B5E04 free)
    uint32_t pending_base = 0, pending_cut = 0, pending_lines = 0;   // the open zoombox text scope; 0 = none
};
inline void scope_begin(Queue& q, uint32_t base, uint32_t cut) { q.pending_base = base; q.pending_cut = cut; q.pending_lines = 0; }
inline void scope_end(Queue& q) { q.pending_base = 0; q.pending_cut = 0; q.pending_lines = 0; }
// A new construction: no ownership survives into it (enqueue and traversal share one construction).
inline void construction(Queue& q, uint32_t frame) { if (frame != q.frame) { q.frame = frame; q.count = 0; } }
// B5E04 frees the vector: no ownership survives into its successor, even at the same index with the same words.
// (B5E3C defrag relocates the vector with its contents and indices, so it changes nothing here.)
inline void freed(Queue& q) { q.count = 0; ++q.generation; }
// One completed push inside the scope (B798C at B7A84, its record's x/y and string already stored): the record's
// vector index and its own string pointer and x/y words.
inline void pushed(Queue& q, uint32_t index, uint32_t string, uint32_t xy) {
    if (!q.pending_base || q.pending_lines >= lines) return;
    for (uint32_t i = 0; i < q.count; ++i)   // an index reused within the construction replaces its stale owner
        if (q.owners[i].index == index) { q.owners[i] = q.owners[--q.count]; break; }
    if (q.count >= owned_records) return;
    q.owners[q.count++] = Owner{index, q.frame, q.generation, string, xy,
        q.pending_base + q.pending_lines * glyphs, q.pending_cut};
    ++q.pending_lines;
}
// The owner of the record traversed at `index`, only in its own construction and vector lifetime and with its own
// words.
inline const Owner* owner(const Queue& q, uint32_t index, uint32_t frame, uint32_t string, uint32_t xy) {
    for (uint32_t i = 0; i < q.count; ++i) {
        const Owner& o = q.owners[i];
        if (o.index == index)
            return o.frame == frame && o.generation == q.generation && o.string == string && o.xy == xy ? &o : nullptr;
    }
    return nullptr;
}
// The traversal consumed and cleared the vector (B7448 vector_clear): indices restart.
inline void cleared(Queue& q) { q.count = 0; }
// Bytes of the NUL-terminated string at `string` (one glyph per byte), or 0 when it is outside RDRAM, crosses
// 0x81000000 before its terminator, is empty, or is longer than one line. `byte(a)` reads guest byte a.
template <class Read> inline uint32_t string_bytes(uint32_t string, Read byte) {
    if (string < 0x80000000u || string >= 0x81000000u) return 0;
    uint32_t n = 0;
    while (n <= glyphs) {
        if (string + n >= 0x81000000u) return 0;   // never read across the end of RDRAM
        if (!byte(string + n)) return n;
        ++n;
    }
    return 0;
}
// Draw state of one B7448 traversal: the admitted record's owner, its converted-glyph ordinal, the open bracket and
// the B62CC activation whose rectangle triple was normally emitted.
struct Draw { uint32_t base = 0, cut = 0, glyph = 0, open = 0, mark_sp = 0, mark_rect = 0; };

// ---- Exact words --------------------------------------------------------------------------------------
// The original F3DEX2 gSPTextureRectangle triple: G_TEXRECT (E4: lrx,lry | tile,ulx,uly in 10.2), G_RDPHALF_1
// (E1: s,t in s10.5), G_RDPHALF_2 (F1: dsdx,dtdy in s5.10).
struct Rect { int32_t ulx, uly, lrx, lry; uint32_t tile; int32_t s, t, dsdx, dtdy; };
inline bool decode(const uint32_t w[6], Rect& r) {
    if ((w[0] >> 24) != 0xE4u || w[2] != 0xE1000000u || w[4] != 0xF1000000u) return false;
    r.lrx = int32_t((w[0] >> 12) & 0xFFFu); r.lry = int32_t(w[0] & 0xFFFu);
    r.tile = (w[1] >> 24) & 7u; r.ulx = int32_t((w[1] >> 12) & 0xFFFu); r.uly = int32_t(w[1] & 0xFFFu);
    r.s = int16_t(w[3] >> 16); r.t = int16_t(w[3] & 0xFFFFu);
    r.dsdx = int16_t(w[5] >> 16); r.dtdy = int16_t(w[5] & 0xFFFFu);
    // Shape refusal only (no mode inference): the established texture state is tile 0; an empty rectangle draws nothing.
    return r.tile == 0u && r.lrx > r.ulx && r.lry > r.uly;
}
// Vertices in BK's centred quarter-pixel units for an ortho of +-2W x +-2H (y up), corners in BK order (ul, ur, ll,
// lr). Texcoords reproduce the rectangle's own mapping under texture scale 0x8000 (BK's 64 units per texel): twice
// the s10.5 value, s at ulx plus dx(1/4 px) * dsdx / 128 (dsdx is 1/1024 texel per pixel). False if any leaves s16.
inline bool quad(const Rect& r, uint32_t width, uint32_t height, uint32_t v[16]) {
    const int64_t xs[2] = {int64_t(r.ulx) - 2 * int64_t(width), int64_t(r.lrx) - 2 * int64_t(width)};
    const int64_t ys[2] = {2 * int64_t(height) - r.uly, 2 * int64_t(height) - r.lry};
    const int64_t ss[2] = {2 * int64_t(r.s), 2 * (int64_t(r.s) + ((int64_t(r.lrx - r.ulx) * r.dsdx) >> 7))};
    const int64_t ts[2] = {2 * int64_t(r.t), 2 * (int64_t(r.t) + ((int64_t(r.lry - r.uly) * r.dtdy) >> 7))};
    for (int i = 0; i < 2; ++i)
        if (xs[i] < -32768 || xs[i] > 32767 || ys[i] < -32768 || ys[i] > 32767 ||
            ss[i] < -32768 || ss[i] > 32767 || ts[i] < -32768 || ts[i] > 32767) return false;
    for (int iy = 0; iy < 2; ++iy) for (int ix = 0; ix < 2; ++ix) {
        uint32_t* o = v + (iy * 2 + ix) * 4;
        o[0] = (uint32_t(uint16_t(int16_t(xs[ix]))) << 16) | uint16_t(int16_t(ys[iy]));
        o[1] = 0u;                                                     // z 0, flag 0
        o[2] = (uint32_t(uint16_t(int16_t(ss[ix]))) << 16) | uint16_t(int16_t(ts[iy]));
        o[3] = 0xFFFFFFFFu;                                            // white, opaque shade
    }
    return true;
}
inline uint32_t f32_bits(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }
// gEXMatrixGroup word2 fields (rt64_extended_gbi.h 321).
constexpr uint32_t simple_verts = 1u | (1u << 3) | (1u << 5) | (1u << 7) | (1u << 9) | (1u << 11) | (1u << 13) |
    (1u << 15) | (1u << 22) | (2u << 24);              // BK gEXMatrixGroupSimpleVerts, PUSH, MODELVIEW
constexpr uint32_t skip_all = 1u;                       // BK gEXMatrixGroupSkipAll, PUSH, MODELVIEW
constexpr uint32_t projection_adjust = 1u | (1u << 1) | (2u << 20);   // SkipAll, PUSH, PROJECTION, ASPECT_ADJUST
static_assert(simple_verts == 0x0240AAA9u && projection_adjust == 0x00200003u, "public field positions");

// Record bracket: inline data (16-byte viewport, float ortho matrix) lives in the display list behind a branch, so
// it has exactly the list's lifetime. Viewport, projection (and its group) and geometry mode are pushed and popped
// with RT64's own state stacks (rt64_extended_gbi.h 404-534); RSP texture and other modes are not touched.
constexpr uint32_t open_words = 2 + 4 + 16 + 2 + 2 + 2 + 4 + 4 + 2 + 2;   // 40 words, 160 bytes
inline uint32_t open(uint32_t* w, uint32_t at, uint32_t width, uint32_t height) {
    uint32_t n = 0;
    const uint32_t vp = at + 8, mtx = vp + 16, after = mtx + 64;
    w[n++] = 0xDE010000u; w[n++] = after;                                      // gSPBranchList over the data
    const uint32_t sx = 2 * width, sy = 2 * height;                            // Vp: scale = translate = 2W, 2H
    w[n++] = (sx << 16) | sy; w[n++] = 0x01FF0000u; w[n++] = (sx << 16) | sy; w[n++] = 0x01FF0000u;
    const float m[16] = {1.0f / float(2 * width), 0, 0, 0,  0, 1.0f / float(2 * height), 0, 0,
                         0, 0, 1.0f, 0,  0, 0, 0, 1.0f};
    for (float f : m) w[n++] = f32_bits(f);
    w[n++] = 0x64000015u; w[n++] = 0;                                          // gEXPushViewport
    w[n++] = 0xDC080008u; w[n++] = vp;                                         // gSPViewport
    w[n++] = 0x6400001Du; w[n++] = 0;                                          // gEXPushProjectionMatrix
    w[n++] = 0x6400000Cu; w[n++] = projection_id; w[n++] = projection_adjust; w[n++] = 0;   // projection group
    w[n++] = 0x64000030u; w[n++] = 7u; w[n++] = 0; w[n++] = mtx;              // gEXMatrixFloat PROJECTION|LOAD
    w[n++] = 0x64000029u; w[n++] = 0;                                          // gEXPushGeometryMode
    w[n++] = 0xD9F0F9FEu; w[n++] = 0x00200004u;   // clear Z, cull, fog, lighting, texgen; set shade, smooth
    return n;
}
// One converted glyph: inline vertices behind a branch; the fixed identity MODELVIEW LOAD|PUSH (a distinct world
// transform per glyph, BK 339); its group (BK 342/345); G_TP_PERSP for the quad (BK 418); the quad (BK 380); then
// the established G_TP_NONE, group pop and MODELVIEW pop.
constexpr uint32_t glyph_words = 2 + 16 + 2 + 4 + 2 + 2 + 2 + 2 + 2 + 2;   // 36 words, 144 bytes
inline uint32_t glyph(uint32_t* w, uint32_t at, const uint32_t v[16], uint32_t identity, uint32_t id, uint32_t cut) {
    uint32_t n = 0;
    const uint32_t vtx = at + 8, after = vtx + 64;
    w[n++] = 0xDE010000u; w[n++] = after;
    for (int i = 0; i < 16; ++i) w[n++] = v[i];
    w[n++] = 0xDA380002u; w[n++] = identity;                                  // MODELVIEW LOAD|PUSH fixed identity
    w[n++] = 0x6400000Cu; w[n++] = id; w[n++] = cut ? skip_all : simple_verts; w[n++] = 0;
    w[n++] = 0xE3000C00u; w[n++] = 0x00080000u;                               // G_TP_PERSP
    w[n++] = 0x01004008u; w[n++] = vtx;                                       // gSPVertex(4, 0)
    w[n++] = 0x07000206u; w[n++] = 0x00000604u;                               // gSP1Quadrangle(0,1,3,2,0)
    w[n++] = 0xE3000C00u; w[n++] = 0x00000000u;                               // G_TP_NONE (B74A8's state)
    w[n++] = 0x6400000Du; w[n++] = 1u;                                        // group pop (MODELVIEW)
    w[n++] = 0xD8380002u; w[n++] = 64u;                                       // MODELVIEW pop
    return n;
}
constexpr uint32_t close_words = 2 + 2 + 2 + 2;   // 8 words, 32 bytes
inline uint32_t close(uint32_t* w) {
    uint32_t n = 0;
    w[n++] = 0x6400002Au; w[n++] = 0;                                          // gEXPopGeometryMode
    w[n++] = 0x6400000Du; w[n++] = 1u | (1u << 8);                             // projection group pop
    w[n++] = 0x6400001Eu; w[n++] = 0;                                          // gEXPopProjectionMatrix
    w[n++] = 0x64000016u; w[n++] = 0;                                          // gEXPopViewport
    return n;
}
// Whole-record bound charged before the first mutation: the bracket plus one glyph per string byte (B7448 has one
// normal B62CC callsite, B78E8, and each normal B62CC emits one triple).
inline uint32_t record_bytes(uint32_t string_bytes) { return 4u * (open_words + close_words + string_bytes * glyph_words); }
// The original rectangle triple becomes G_NOOPs in place: the list keeps its length and cursor.
inline void silence(uint32_t w[6]) { for (int i = 0; i < 6; ++i) w[i] = 0; }
}
#endif
