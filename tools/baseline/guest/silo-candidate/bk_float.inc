/* BK float position/velocity producer for both Tooie dbanim entries (B3).
 * Included by bk_pipeline.c after tooie_bk_state; every function here is always-inline.
 * Adapted from BanjoRecomp dbd52154 patches/transform_tagging.c recomp_reset_skinning_stack,
 * recomp_apply_cpu_skinning (449-588) and modelRender_draw 844-867 / 936-940. The record walk is
 * the Tooie grammar from execution-facts47 bk-float-bindings (BT_ENTRY0/1, BT_POSE, BT_POSITION);
 * BK's BKModelUnk28 stepping is not copied. The original integer producer and its normals and
 * attribute writes stay untouched; this reruns XYZ only, in single-precision float.
 *
 * Segment base (facts47/49): the actual integer vertex-0 address the draw loads, which in the BK
 * selection is the original header+24. The P04b redirect is not selected with BK, so nothing
 * rewrites segment 1 after construction; root begin still re-reads the draw's own segment-1 word
 * and publishes no floats if it names any other stream. */
#define TOOIE_BK_FLOATS 65536u                 /* BK SKINNING_POSITIONS_MAX */
#define TOOIE_BK_RECORDS 1024u
#define TOOIE_BK_RECORD_PROBE TOOIE_BK_RECORDS /* whole bounded table per owner ID */
#define TOOIE_BK_MAP_LIMIT 65536u              /* bounded terminator search per group */
struct TooieBkCapture {
    u32 key, command, deform, header, pose, entry, valid, model;
};
struct TooieBkSegment { u32 pos, vel, base, enabled; };   /* enabled: 0 none, 1 own, 2 suspended outer */
struct TooieBkRecord { u32 id, frame, model, start; };    /* BK ModelSkinningData + owner key */
struct TooieBkFloat {
    u32 identity[16];                                     /* BK identity_fixed_mtx */
    u32 frame, count, skinned, velocities, exhausted, rejected, suspended, redirected;
    struct TooieBkCapture capture;
    struct TooieBkSegment segs[TOOIE_BK_ROOT_DEPTH];
    struct TooieBkRecord records[TOOIE_BK_RECORDS];
    f32 pos[2][TOOIE_BK_FLOATS];                          /* BK sSkinningPosFloats */
    f32 vel[TOOIE_BK_FLOATS];                             /* BK sSkinningVelFloats */
};
_Static_assert(__builtin_offsetof(struct TooieBkFloat, frame) == 64 &&
    __builtin_offsetof(struct TooieBkFloat, suspended) == 88 &&
    __builtin_offsetof(struct TooieBkFloat, capture) == 96 &&
    __builtin_offsetof(struct TooieBkFloat, segs) == 128 &&
    __builtin_offsetof(struct TooieBkFloat, records) == 256 &&
    __builtin_offsetof(struct TooieBkFloat, pos) == 16640 &&
    sizeof(struct TooieBkFloat) == 803072u, "BK float producer layout1");
volatile struct TooieBkFloat tooie_bk_float __attribute__((section(".tooie_bk_float"), aligned(8))) = {
    .identity = { 0x00010000u, 0u, 0x00000001u, 0u, 0u, 0x00010000u, 0u, 0x00000001u,
                  0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u },
};
#define BKF tooie_bk_float
extern volatile u32 D_8012C964, D_8012C978;

/* BK recomp_reset_skinning_stack, at the A7888 frame begin. */
SI void bk_skin_frame_begin(void) {
    BKF.count = 0;
    BKF.frame += 1u;
    BKF.capture.valid = 0;
}

/* RT64 gEXSetVertexSegment; base must be the actual vertex-0 address the draw loads. */
SI GfxCommand* bk_segment(GfxCommand* cursor, u32 element, u32 enabled, u32 data, u32 base) {
    gEXSetVertexSegment(cursor, element, enabled, data, base);
    return cursor + 2;
}
SI GfxCommand* bk_segments_off(GfxCommand* cursor) {
    cursor = bk_segment(cursor, G_EX_VERTEX_POSITION, G_EX_DISABLED, 0, 0);
    return bk_segment(cursor, G_EX_VERTEX_VELOCITY, G_EX_DISABLED, 0, 0);
}
SI GfxCommand* bk_segments_on(GfxCommand* cursor, volatile struct TooieBkSegment* s) {
    cursor = bk_segment(cursor, G_EX_VERTEX_POSITION, G_EX_ENABLED, s->pos, s->base);
    return s->vel ? bk_segment(cursor, G_EX_VERTEX_VELOCITY, G_EX_ENABLED, s->vel, s->base) :
        bk_segment(cursor, G_EX_VERTEX_VELOCITY, G_EX_DISABLED, 0, 0);
}
/* Effective segment state at root depth d: own enable, explicit suspension, or the parent's. */
SI volatile struct TooieBkSegment* bk_segments_effective(s32 d) {
    for (; d >= 0; d--) {
        if (BKF.segs[d].enabled == 1u) return &BKF.segs[d];
        if (BKF.segs[d].enabled == 2u) return 0;
    }
    return 0;
}

/* BT_POSITION 80116588-80116658: result[j] = (((x*m[0][j]) + m[3][j]) + y*m[1][j]) + z*m[2][j],
 * single precision in exactly this order; the unrounded value is published. */
SI void bk_transform(u32 point, u32 matrix, f32* out) {
    f32 x = (f32)*(volatile s16*)(point + 0u);
    f32 y = (f32)*(volatile s16*)(point + 2u);
    f32 z = (f32)*(volatile s16*)(point + 4u);
    const volatile f32* m = (const volatile f32*)matrix;
    u32 j;
    for (j = 0; j < 3u; j++) {
        f32 r = x * m[j];
        r = r + m[12u + j];
        r = r + y * m[4u + j];
        r = r + z * m[8u + j];
        out[j] = r;
    }
}

/* BT_ENTRY0 80800040-80800120 / BT_ENTRY1 808001B0-80800320 XYZ grammar. Returns 0 when the
 * supplied extents cannot cover the walk; the original integer result is then used unchanged. */
SI u32 bk_dbanim_rerun(volatile struct TooieBkCapture* c, f32* dst, u32 count) {
    u32 deform = c->deform, group, map, matrix, words;
    s32 groups, g, pose_index, points, normals, limit, point;
    s32 header0;
    f32 t[3];
    u32 cached;
    if (!silo_range(deform, 4u, 1u)) return 0;
    header0 = *(volatile s16*)(deform + 0u);
    groups = *(volatile s16*)(deform + 2u);
    group = deform + 4u;
    for (g = 0; g < groups; g++) {
        if (!silo_range(group, 6u, 1u)) return 0;
        pose_index = *(volatile s16*)(group + 0u);
        points = *(volatile s16*)(group + 2u);
        normals = *(volatile s16*)(group + 4u);
        /* BT_POSE ADE2C: -1 is pose+0x10; nonnegative is pose+0x50+64*index. The original has
         * no guard for other negative indices; the maintained walker refuses them. */
        if (pose_index == -1) matrix = c->pose + 0x10u;
        else if (pose_index >= 0) matrix = c->pose + 0x50u + 64u * (u32)pose_index;
        else return 0;
        if (!silo_range(matrix, 64u, 3u)) return 0;
        /* The position kernel is a do-while: point 0 is read even for nonpositive P. */
        limit = points > 0 ? points : 1;
        if (!silo_range(group + 6u, 6u * (u32)limit, 1u)) return 0;
        /* Entry0: group+6+6P+(header.s16(0)!=0 ? 4N : 0). Entry1: group+6+6P+4N always. */
        map = (u32)((s32)group + 6 + 6 * points + ((c->entry || header0 != 0) ? 4 * normals : 0));
        point = 0; cached = 0;
        for (words = 0;; words++) {
            s32 code;
            if (words >= TOOIE_BK_MAP_LIMIT || !silo_range(map, 2u, 1u)) return 0;
            code = *(volatile s16*)map;
            map += 2u;
            if (code >= 0) {
                /* Scatter the current transformed point; repeated targets keep encounter order,
                 * so the last write wins. The source point does not advance. */
                if ((u32)code >= count || point >= limit) return 0;
                if (!cached) { bk_transform(group + 6u + 6u * (u32)point, matrix, t); cached = 1; }
                dst[(u32)code * 3u + 0u] = t[0];
                dst[(u32)code * 3u + 1u] = t[1];
                dst[(u32)code * 3u + 2u] = t[2];
            }
            else if (code == -1) { point += 1; cached = 0; }
            else break;   /* < -1 terminates; the next group starts right after it */
        }
        group = map;
    }
    return 1;
}

/* @tooie BK keeps one ModelSkinningData per marker (transform_tagging.c 483-511); Tooie keys this
 * fixed table by owner sample ID. Placement in the bounded table from the multiplicative bucket:
 * the exact ID; else the first stale slot (never used, or not drawn in this or the previous
 * construction); else 0, because every slot holds a record that is current or that the next
 * construction's adjacency needs: no live owner is evicted and this draw is POSITION-only. The
 * whole table is read before a stale slot is taken, so an ID is never held twice and a record
 * survives the staleness of slots ahead of it. */
SI volatile struct TooieBkRecord* bk_record(u32 id) {
    const u32 bucket = ((id >> 8) * 2654435761u) >> 22;
    volatile struct TooieBkRecord* stale = 0;
    u32 i;
    for (i = 0; i < TOOIE_BK_RECORD_PROBE; i++) {
        volatile struct TooieBkRecord* r = &BKF.records[(bucket + i) & (TOOIE_BK_RECORDS - 1u)];
        if (r->id == id) return r;
        if (!stale && (!r->id || (r->frame != BKF.frame && r->frame != BKF.frame - 1u)))
            stale = r;
    }
    return stale;
}

/* BK recomp_apply_cpu_skinning: per-owner record, positions from the current integer XYZ,
 * the float rerun scattered over them, velocity only against the exact previous frame and
 * the same model, and never across this view's camera cut (the cut that skips the groups also
 * withholds explicit velocity; the record keeps this frame's positions so history restarts).
 * Records are keyed by the owner sample ID (a creation generation, so a new creation never
 * inherits history). Nested actor attachments get no record, and an owner whose record table
 * is full of live records gets none (counted as exhausted): positions without velocity. */
SI u32 bk_apply_skinning(volatile struct TooieBkCapture* c, u32 model, f32** pos, f32** vel) {
    volatile struct TooieBkRecord* rec = 0;
    struct TooieBkRecord prev = {0, 0, 0, 0};
    u32 id = BK.cur_drawn_model_transform_id, header = c->header, count, i, j, start;
    f32 *dst_pos, *dst_vel;
    *pos = 0; *vel = 0;
    if (id && !(BK.owner.kind == TOOIE_BK_OWNER_ACTOR && id != BK.owner.id)) {
        rec = bk_record(id);
        if (!rec) bk_count(&BKF.exhausted);
        else {
            if (rec->id != id) { rec->id = id; rec->frame = 0; rec->model = 0; rec->start = 0; }
            prev.frame = rec->frame; prev.model = rec->model; prev.start = rec->start;
            rec->frame = BKF.frame; rec->model = model; rec->start = BKF.count;
        }
    }
    if (!silo_range(header, 24u, 3u)) { if (rec) rec->frame = 0; return 0; }
    count = (u32)(*(volatile u16*)(header + 0x16u)) >> 1;     /* BT_COUNT; bit 0 is not count */
    if (!count || !silo_range(header + 24u, count * 16u, 1u) ||
        BKF.count + count * 3u > TOOIE_BK_FLOATS) {
        if (BKF.count + count * 3u > TOOIE_BK_FLOATS) bk_count(&BKF.exhausted);
        if (rec) rec->frame = 0;
        return 0;
    }
    start = BKF.count;
    dst_pos = (f32*)&BKF.pos[BKF.frame & 1u][start];
    dst_vel = (f32*)&BKF.vel[start];
    /* Copy unmodified positions: untouched targets keep their current integer XYZ. */
    for (i = 0, j = 0; i < count; i++) {
        u32 v = header + 24u + i * 16u;
        dst_pos[j++] = (f32)*(volatile s16*)(v + 0u);
        dst_pos[j++] = (f32)*(volatile s16*)(v + 2u);
        dst_pos[j++] = (f32)*(volatile s16*)(v + 4u);
    }
    BKF.count += count * 3u;
    if (BKF.count & 1u) BKF.count += 1u;
    if (!bk_dbanim_rerun(c, dst_pos, count)) {
        /* @tooie Unlike BK's exhaustion path, a refused walk also clears the history it set. */
        bk_count(&BKF.rejected);
        if (rec) rec->frame = 0;
        return 0;
    }
    if (rec && !BK.skip_all_interpolation && !BK.cur_drawn_model_skip_interpolation &&
        prev.frame == BKF.frame - 1u && prev.model == model) {
        const f32* prev_pos = (const f32*)&BKF.pos[(BKF.frame & 1u) ^ 1u][prev.start];
        for (j = 0; j < count * 3u; j++) dst_vel[j] = dst_pos[j] - prev_pos[j];
        *vel = dst_vel;
        bk_count(&BKF.velocities);
    }
    *pos = dst_pos;
    bk_count(&BKF.skinned);
    return 1;
}

/* The draw's own segment-1 word, re-read at root begin after every DEFA0 transport, must name
 * the original header+24 stream (RT64 masks both to physical). */
SI u32 bk_segment_is_original(volatile struct TooieBkCapture* c) {
    u32 word;
    if (!silo_range(c->command, 8u, 7u)) return 0;
    word = *(volatile u32*)(c->command + 4u);
    return ((word ^ (c->header + 24u)) & 0x1FFFFFFFu) == 0u;
}

/* Close-free suspension: a root that publishes no floats of its own inside one that does keeps
 * foreign vertices out of the enclosing arena. */
SI GfxCommand* bk_skin_suspend(u32 depth, GfxCommand* cursor) {
    volatile struct TooieBkSegment* s = &BKF.segs[depth];
    s->enabled = 0;
    if (bk_segments_effective((s32)depth - 1)) {
        s->enabled = 2;
        cursor = bk_segments_off(cursor);
    }
    return cursor;
}

/* modelRender_draw 844-867 at DF0B0 (after the original deform at DEE8C-DEF00 has run):
 * consume this DE498's capture, rerun, enable POSITION and VELOCITY with the original base.
 * Returns the advanced local cursor. */
SI GfxCommand* bk_skin_root_begin(u32 depth, u32 cursor_slot, u32 model, GfxCommand* cursor) {
    volatile struct TooieBkSegment* s = &BKF.segs[depth];
    volatile struct TooieBkCapture* c = &BKF.capture;
    f32 *pos, *vel;
    u32 own = c->valid && c->key == cursor_slot && c->model == model;
    c->valid = 0;
    if (own && !bk_segment_is_original(c)) { bk_count(&BKF.redirected); own = 0; }
    if (own && bk_apply_skinning(c, model, &pos, &vel)) {
        s->pos = (u32)pos; s->vel = (u32)vel; s->base = c->header + 24u; s->enabled = 1;
        return bk_segments_on(cursor, s);
    }
    return bk_skin_suspend(depth, cursor);
}

/* modelRender_draw 936-940 at DF234 (normal) or DF238 (bit40): clear this root's float use,
 * restoring the enclosing root's segments when this root sits inside one. */
SI GfxCommand* bk_skin_root_end(u32 depth, GfxCommand* cursor) {
    volatile struct TooieBkSegment* s = &BKF.segs[depth];
    volatile struct TooieBkSegment* outer;
    if (!s->enabled) return cursor;
    s->enabled = 0;
    outer = bk_segments_effective((s32)depth - 1);
    return outer ? bk_segments_on(cursor, outer) : bk_segments_off(cursor);
}
