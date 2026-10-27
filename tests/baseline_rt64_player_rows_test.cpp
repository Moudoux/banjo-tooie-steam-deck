// F4 player correspondence collector (rt64_player_rows.hpp) against synthetic RT64 draw data and
// frame maps. No game data, renderer or GPU.
#include "rt64_player_rows.hpp"
#include "rt64_trace_window.hpp"   // L05 F4 run
#include "guest/silo-candidate/render_bridge.h"   // L05 evidence: bridge block domain
#include "guest/silo-candidate/bk_pipeline.h"
#include "menu_text.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>

using namespace RT64;
using Row = TooiePlayerTransform;
using ProjectionRow = TooiePlayerProjection;

namespace {
void require(bool ok, const char *why) {
    if (!ok) throw std::runtime_error(why);
}
uint32_t bits(float value) { uint32_t out; std::memcpy(&out, &value, sizeof(out)); return out; }
hlslpp::float4x4 matrix(float base) {
    return hlslpp::float4x4(base, 0, 0, 0, 0, base, 0, 0, 0, 0, base, 0, base * 10.0f, 1, 2, 1);
}
// One world transform per id, `vertices` vertices each, all using projection `projection`.
void add(DrawData &d, uint32_t id, float base, uint32_t vertices, uint16_t projection = 0) {
    d.worldTransformVertexIndices.push_back(d.vertexCount());
    d.worldTransformGroups.push_back(uint32_t(d.transformGroups.size()));
    TransformGroup group;
    group.matrixId = id;
    group.ordering = G_EX_ORDER_LINEAR;
    d.transformGroups.push_back(group);
    d.worldTransforms.emplace_back(matrix(base));
    d.lerpWorldTransforms.emplace_back(matrix(base + 0.5f));
    d.worldTransformPhysicalAddresses.push_back(0x00100000u + uint32_t(d.worldTransforms.size()) * 64u);
    for (uint32_t v = 0; v < vertices; ++v) {
        d.worldIndices.push_back(uint16_t(d.worldTransforms.size() - 1));
        d.viewProjIndices.push_back(projection);
    }
}
void projections(DrawData &d) {
    for (uint32_t p = 0; p < 2; ++p) {
        d.viewProjTransformGroups.push_back(uint32_t(d.transformGroups.size()));
        TransformGroup group;
        group.matrixId = 0xA0000010u + p;
        d.transformGroups.push_back(group);
        d.viewProjTransforms.emplace_back(matrix(100.0f + p));
        d.modViewProjTransforms.emplace_back(matrix(200.0f + p));
    }
}
} // namespace

int main() {
    try {
        // Selected ID ranges: exactly bk_pipeline.h's player domains; shadow, actor and others excluded.
        require(tooieIsPlayerTransformId(TOOIE_BK_PLAYER_ID_START) &&
            tooieIsPlayerTransformId(TOOIE_BK_PLAYER_ID_END - 1u) &&
            tooieIsPlayerTransformId(TOOIE_BK_PLAYER_SLOT_ID_START) &&
            tooieIsPlayerTransformId(TOOIE_BK_PLAYER_SLOT_ID_END - 1u), "player domains selected");
        require(!tooieIsPlayerTransformId(TOOIE_BK_PLAYER_ID_START - 1u) &&
            !tooieIsPlayerTransformId(TOOIE_BK_PLAYER_ID_END) &&
            !tooieIsPlayerTransformId(TOOIE_BK_PLAYER_SLOT_ID_START - 1u) &&
            !tooieIsPlayerTransformId(TOOIE_BK_PLAYER_SLOT_ID_END) &&
            !tooieIsPlayerTransformId(TOOIE_BK_ACTOR_ID_START) && !tooieIsPlayerTransformId(0u),
            "shadow, actor and other IDs excluded");

        require(tooieIsSupplementalRootId(TOOIE_BK_ACTOR_ID_START) &&
            tooieIsSupplementalRootId(TOOIE_BK_ACTOR_ID_START+0x100u) &&
            tooieIsSupplementalRootId(tooieMenuModelIdStart) &&
            tooieIsSupplementalRootId(tooieMenuModelIdStart+0x200u) &&
            tooieIsSupplementalRootId(tooieMenuModelIdStart+0x100u), "owned root namespaces, menu sprite components included");
        static_assert(tooieMenuTextIdStart == tooie_menu_text::first_id && tooieMenuTextIdEnd == tooie_menu_text::end_id &&
            tooie_menu_text::projection_id == tooieMenuTextIdEnd, "F4 text range is the text owner's glyph range");
        require(tooieIsSupplementalRootId(tooieMenuTextIdStart) &&
            tooieIsSupplementalRootId(tooieMenuTextIdStart+tooie_menu_text::glyphs) &&
            tooieIsSupplementalRootId(tooieMenuTextIdEnd-tooie_menu_text::glyphs) &&
            !tooieIsSupplementalRootId(tooieMenuTextIdStart+1u) &&
            !tooieIsSupplementalRootId(tooieMenuTextIdStart+tooie_menu_text::glyphs+1u) &&
            !tooieIsSupplementalRootId(tooieMenuTextIdEnd), "text line first glyphs only; other letters and the text projection excluded");
        require(!tooieIsSupplementalRootId(TOOIE_BK_ACTOR_ID_START+1u) &&
            !tooieIsSupplementalRootId(TOOIE_BK_ACTOR_ID_END) &&
            !tooieIsSupplementalRootId(tooieTransitionIdEnd) &&
            !tooieIsSupplementalRootId(tooieMenuModelIdStart+0x101u) &&
            !tooieIsSupplementalRootId(0u) && !tooieIsSupplementalRootId(0x2001C001u),
            "actor bones, menu sprite bones, zero and shadow bone offsets excluded");
        for(uint32_t view=0;view<5u;++view){
            const uint32_t id=tooieTransitionIdStart+view*0x100u;
            require(tooieIsSupplementalRootId(id)&&!tooieIsSupplementalRootId(id+1u),
                "each transition view root included, its bone excluded");
        }
        require(!tooieIsSupplementalRootId(tooieTransitionIdEnd),"sixth transition view excluded");
        // Prior slot rule.
        GameFrameMap::WorkloadMap map;
        map.mapped = true;
        map.prevWorkloadIndex = 1;
        const std::vector<uint32_t> previousFrame = {1};
        require(tooiePlayerPriorSlotValid(&map, 2, 50, previousFrame, 4, 49), "valid prior slot");
        require(!tooiePlayerPriorSlotValid(&map, 2, 50, std::vector<uint32_t>{3}, 4, 49), "prior not in prevFrame");
        require(!tooiePlayerPriorSlotValid(&map, 1, 50, previousFrame, 4, 49), "self slot pair");
        require(!tooiePlayerPriorSlotValid(&map, 2, 49, previousFrame, 4, 49), "self workload ID pair");
        require(!tooiePlayerPriorSlotValid(&map, 2, 50, previousFrame, 1, 49), "prior outside queue");
        require(!tooiePlayerPriorSlotValid(nullptr, 2, 50, previousFrame, 4, 49), "no owner map");
        GameFrameMap::WorkloadMap unmapped = map;
        unmapped.mapped = false;
        require(!tooiePlayerPriorSlotValid(&unmapped, 2, 50, previousFrame, 4, 49), "unmapped owner");

        DrawData current, previous;
        projections(current);
        projections(previous);
        add(current, 0x20000500u, 1.0f, 3);                    // t0 body root, slot 0
        add(current, 0x20000501u, std::numeric_limits<float>::quiet_NaN(), 2);  // t1 nonfinite raw
        add(current, TOOIE_BK_PLAYER_ID_END + 0x500u, 3.0f, 2);  // t2 shadow domain: excluded
        add(current, 0x20000502u, 4.0f, 2);                    // t3 mapped, prior index out of range
        add(current, TOOIE_BK_PLAYER_SLOT_ID_START + 0x501u, 5.0f, 0); // t4 slot 1, no vertices
        add(current, 0x50000000u, 6.0f, 1);                    // t5 actor: excluded
        current.viewProjIndices[current.worldTransformVertexIndices[1] + 1] = 1;  // t1 mixed projection
        add(previous, 0x20000400u, 9.0f, 1);
        add(previous, 0x20000500u, 0.5f, 3);
        add(previous, 0x20000501u, 1.5f, 2);
        map.transforms.resize(current.worldTransforms.size());
        map.transforms[0].mapped = true; map.transforms[0].prevTransformIndex = 1;   // different index
        map.transforms[3].mapped = true; map.transforms[3].prevTransformIndex = 99;  // prior invalid
        map.transforms[4].mapped = true; map.transforms[4].prevTransformIndex = 0;   // other prior ID
        map.viewProjections.resize(current.viewProjTransforms.size());
        map.viewProjections[0].mapped = true; map.viewProjections[0].prevTransformIndex = 1;

        std::vector<Row> rows;
        std::vector<ProjectionRow> projected;
        rows.reserve(tooiePlayerRowsPerPresentation);
        projected.reserve(tooiePlayerProjectionsPerPresentation);
        tooieCollectPlayerRows(7, current, 2, 50, &map, &previous, 1, 49, 0.25f, 0.75f, Row::CurrentMatched,
            true, true, rows, projected);
        require(rows.size() == 4, "every player-labelled transform, including vertexless; shadow/actor excluded");
        const Row &root = rows[0], &nan = rows[1], &invalid = rows[2], &other = rows[3];
        require(root.index == 0 && nan.index == 1 && invalid.index == 3 && other.index == 4, "emission order");
        require(root.presentation == 7 && root.workload_slot == 2 && root.workload_id == 50 &&
            root.previous_slot == 1 && root.previous_workload_id == 49 && root.weight == 0.25f &&
            root.previous_weight == 0.75f, "presentation, slots, IDs and weights");
        // Actual mapping to a different previous index, with that index's own ID and matrix.
        require((root.flags & (Row::OwnerMapped | Row::PriorSlotValid | Row::Mapped | Row::PriorIndexValid |
            Row::Interpolated | Row::CurrentMatched | Row::ProjectionKnown)) ==
            (Row::OwnerMapped | Row::PriorSlotValid | Row::Mapped | Row::PriorIndexValid | Row::Interpolated |
            Row::CurrentMatched | Row::ProjectionKnown), "mapped root flags");
        require(root.previous_index == 1 && root.previous_group_id == 0x20000500u && root.previous_vertices == 3,
            "actual previous index, ID and vertex count");
        require(root.raw[0] == bits(1.0f) && root.prior[0] == bits(0.5f) && root.interpolated[0] == bits(1.5f) &&
            root.raw[12] == bits(10.0f), "raw, prior and interpolated matrices are the actual words");
        // Unmatched and nonfinite: raw NaN bits preserved, no prior evidence.
        require(!(nan.flags & (Row::Mapped | Row::PriorIndexValid)) && nan.previous_index == UINT32_MAX,
            "unmatched transform has no prior");
        require(nan.raw[0] == bits(std::numeric_limits<float>::quiet_NaN()) && (nan.flags & Row::Interpolated),
            "NaN preserved bit-exactly, never filtered");
        require((nan.flags & Row::ProjectionMixed) && nan.projection == 0, "mixed projection flagged");
        // Mapped but previous index outside the prior workload: mapped, prior invalid.
        require((invalid.flags & Row::Mapped) && !(invalid.flags & Row::PriorIndexValid) &&
            invalid.previous_index == 99 && invalid.previous_group_id == 0, "prior index invalid is explicit");
        // A mapping to a different previous ID is recorded as such, never repaired.
        require((other.flags & Row::PriorIndexValid) && other.previous_group_id == 0x20000400u &&
            other.group_id == TOOIE_BK_PLAYER_SLOT_ID_START + 0x501u && other.vertices == 0 &&
            !(other.flags & Row::ProjectionKnown), "different prior ID kept; vertexless has no projection");
        // Projections: referenced once per presentation and slot, with mapped prior and final.
        require(projected.size() == 2 && projected[0].index == 0 && projected[0].group_id == 0xA0000010u &&
            (projected[0].flags & (ProjectionRow::Mapped | ProjectionRow::PriorIndexValid | ProjectionRow::Final |
            ProjectionRow::GroupKnown)) == (ProjectionRow::Mapped | ProjectionRow::PriorIndexValid |
            ProjectionRow::Final | ProjectionRow::GroupKnown) && projected[0].previous_index == 1 &&
            projected[0].raw[0] == bits(100.0f) && projected[0].prior[0] == bits(101.0f) &&
            projected[0].final[0] == bits(200.0f), "projection association and matrices");
        require(projected[0].weight == 0.25f && projected[0].previous_group_id == 0xA0000011u &&
            !(projected[0].flags & ProjectionRow::OrthoScene) && projected[0].paired_scene == UINT32_MAX,
            "player projections carry weight and the actual prior group; no scene claim");

        // Unprocessed transforms/projections and no valid prior slot: absent, never substituted.
        rows.clear(); projected.clear();
        tooieCollectPlayerRows(8, current, 2, 50, &map, nullptr, 3, 0, 1.0f, 0.0f, 0, false, false, rows, projected);
        require(rows.size() == 4 && !(rows[0].flags & (Row::Interpolated | Row::PriorSlotValid | Row::PriorIndexValid)) &&
            (rows[0].flags & Row::Mapped) && rows[0].previous_slot == 3 && rows[0].previous_workload_id == 0 &&
            rows[0].interpolated[0] == 0 && rows[0].prior[0] == 0, "raw only: no interpolated or prior words");
        require(projected.size() == 2 && !(projected[0].flags & (ProjectionRow::Final | ProjectionRow::PriorIndexValid |
            ProjectionRow::ProjectionsProcessed)) && !(rows[0].flags & Row::TransformsProcessed),
            "projection without final or prior");

        // Bounded presentation: the row limit stops collection and flags the last kept row.
        DrawData many;
        projections(many);
        for (uint32_t i = 0; i < tooiePlayerRowsPerPresentation + 6; ++i) add(many, 0x20000500u + (i % 200u), 1.0f, 1);
        rows.clear(); projected.clear();
        const size_t capacity = rows.capacity();
        tooieCollectPlayerRows(9, many, 0, 60, nullptr, nullptr, UINT32_MAX, 0, 1.0f, 0.0f, 0, true, true, rows, projected);
        require(rows.size() == tooiePlayerRowsPerPresentation && (rows.back().flags & Row::Truncated) &&
            !(rows.front().flags & Row::Truncated) && rows.capacity() == capacity,
            "row limit, explicit truncation, no reallocation");
        require(!(rows.front().flags & (Row::OwnerMapped | Row::Mapped)) && rows.front().previous_slot == UINT32_MAX,
            "no owner map: unmapped and no prior slot");
        // Existing saturated player rows must remain intact when supplement is requested.
        DrawData actors; projections(actors);
        for(uint32_t i=0;i<70u;++i) add(actors,TOOIE_BK_ACTOR_ID_START+i*0x100u,3.0f,1);
        tooieCollectPlayerRows(9,actors,1,61,nullptr,nullptr,UINT32_MAX,0,1,0,0,true,true,rows,projected,true);
        require(rows.size()==tooiePlayerRowsPerPresentation && tooieIsPlayerTransformId(rows.back().group_id),
            "supplement never replaces saturated player rows");
        rows.clear(); projected.clear();
        tooieCollectPlayerRows(10,actors,1,61,nullptr,nullptr,UINT32_MAX,0,1,0,0,true,true,rows,projected);
        require(rows.empty(),"default pass preserves player-only selection");
        tooieCollectPlayerRows(10,actors,1,61,nullptr,nullptr,UINT32_MAX,0,1,0,0,true,true,rows,projected,true);
        require(rows.size()==tooieOwnedRootsPerPresentation && (rows.back().flags&Row::Truncated),
            "root supplement cap64 is explicit");
        tooieCollectPlayerRows(10,actors,2,62,nullptr,nullptr,UINT32_MAX,0,1,0,0,true,true,rows,projected,true);
        require(rows.size()==tooieOwnedRootsPerPresentation && rows.capacity()==capacity,
            "supplement cap is across workloads without reallocating");
        // Players precede the supplement even when a transition occurs first in draw order.
        DrawData mixed; projections(mixed);
        add(mixed,tooieTransitionIdStart+0x100u,4.0f,1);
        add(mixed,TOOIE_BK_PLAYER_ID_START,5.0f,1);
        add(mixed,tooieTransitionIdStart+0x400u,6.0f,1);
        add(mixed,tooieTransitionIdStart+1u,7.0f,1);
        add(mixed,tooieTransitionIdEnd,8.0f,1);
        rows.clear(); projected.clear();
        tooieCollectPlayerRows(11,mixed,0,70,nullptr,nullptr,UINT32_MAX,0,1,0,0,true,true,rows,projected);
        require(rows.size()==1&&rows[0].group_id==TOOIE_BK_PLAYER_ID_START,"player-only first pass");
        tooieCollectPlayerRows(11,mixed,0,70,nullptr,nullptr,UINT32_MAX,0,1,0,0,true,true,rows,projected,true);
        require(rows.size()==3&&rows[0].group_id==TOOIE_BK_PLAYER_ID_START&&
            rows[1].group_id==tooieTransitionIdStart+0x100u&&rows[2].group_id==tooieTransitionIdStart+0x400u,
            "transition roots append after players in existing world order");
        tooieCollectPlayerRows(11,actors,1,71,nullptr,nullptr,UINT32_MAX,0,1,0,0,true,true,rows,projected,true);
        require(rows.size()==1+tooieOwnedRootsPerPresentation&&(rows.back().flags&Row::Truncated),
            "two transitions consume two slots of the shared actor/menu64 budget");
        tooieCollectPlayerRows(11,mixed,2,72,nullptr,nullptr,UINT32_MAX,0,1,0,0,true,true,rows,projected,true);
        require(rows.size()==1+tooieOwnedRootsPerPresentation&&rows.capacity()==capacity,
            "later transition workloads cannot exceed the shared budget or reallocate");
        // Shadow roots expose the consumed projection mapping; a missing map is never fabricated.
        DrawData shadows; projections(shadows);
        add(shadows,TOOIE_BK_PLAYER_ID_END+0x500u,1,1);
        add(shadows,TOOIE_BK_PLAYER_ID_END+0x501u,1,1); // non-root excluded
        rows.clear(); projected.clear();
        tooieCollectPlayerRows(12,shadows,0,80,nullptr,nullptr,UINT32_MAX,0,1,0,0,true,true,rows,projected);
        require(rows.empty(),"shadows remain outside player-first pass");
        tooieCollectPlayerRows(12,shadows,0,80,nullptr,nullptr,UINT32_MAX,0,1,0,0,true,true,rows,projected,true);
        require(rows.size()==1&&projected.size()==1&&!(projected[0].flags&ProjectionRow::Mapped),
            "shadow root and its unmapped consumed projection retained");
        GameFrameMap::WorkloadMap shadowMap; shadowMap.mapped=true; shadowMap.prevWorkloadIndex=1;
        shadowMap.viewProjections.resize(2);shadowMap.viewProjections[0].mapped=true;
        shadowMap.viewProjections[0].prevTransformIndex=0;
        rows.clear(); projected.clear();
        tooieCollectPlayerRows(13,shadows,0,81,&shadowMap,&shadows,1,80,1,0,0,true,true,rows,projected,true);
        require(rows.size()==1&&projected.size()==1&&(projected[0].flags&ProjectionRow::Mapped)&&
            (projected[0].flags&ProjectionRow::PriorIndexValid)&&projected[0].previous_workload_id==80,
            "shadow projection recovery mapping comes from RT64 map");
        tooieCollectPlayerRows(13,actors,2,82,nullptr,nullptr,UINT32_MAX,0,1,0,0,true,true,rows,projected,true);
        require(rows.size()==tooieOwnedRootsPerPresentation&&rows.capacity()==capacity,
            "shadow shares existing supplemental budget without growing storage");
        // Reproduce report1/4 starvation: repeated early actors/body must not hide late roots.
        DrawData crowded, late; projections(crowded); projections(late);
        add(crowded,TOOIE_BK_PLAYER_ID_START,1,1);
        for(uint32_t i=0;i<70;++i) add(crowded,TOOIE_BK_ACTOR_ID_START,2,1);
        for(uint32_t i=0;i<30;++i) add(crowded,0x2001C500u,3,1);
        add(late,0x2001E100u,4,1); add(late,tooieMenuModelIdStart+0x400u,5,1);
        rows.clear(); projected.clear();
        tooieCollectPlayerRows(20,crowded,0,80,nullptr,nullptr,UINT32_MAX,0,1,0,0,true,true,rows,projected);
        tooieCollectPlayerRows(20,crowded,0,80,nullptr,nullptr,UINT32_MAX,0,1,0,0,true,true,rows,projected,true,true);
        tooieCollectPlayerRows(20,late,1,81,nullptr,nullptr,UINT32_MAX,0,1,0,0,true,true,rows,projected,true,true);
        require(rows.size()==5 && rows[0].group_id==TOOIE_BK_PLAYER_ID_START &&
            rows[4].group_id==0x2001E100u && rows[3].group_id==tooieMenuModelIdStart+0x400u,
            "representatives across workloads retain late backpack/menu after repeated actor/body");
        require(!(rows[0].flags&Row::Representative) && rows[0].group_matrix_count==0 &&
            (rows[1].flags&Row::Representative) && rows[1].group_matrix_count==70 &&
            (rows[2].flags&Row::Representative) && rows[2].group_matrix_count==30 &&
            rows[3].group_matrix_count==1 && rows[4].group_matrix_count==1,
            "representative metadata counts every matrix in its exact workload/group");
        require(!(rows[3].flags&Row::Mapped) && rows[3].previous_slot==UINT32_MAX,
            "representative selection does not manufacture prior mapping");
        tooieCollectPlayerRows(20,crowded,0,80,nullptr,nullptr,UINT32_MAX,0,1,0,0,true,true,rows,projected,true,false);
        tooieCollectPlayerRows(20,late,1,81,nullptr,nullptr,UINT32_MAX,0,1,0,0,true,true,rows,projected,true,false);
        require(rows.size()==1+tooieOwnedRootsPerPresentation && rows.capacity()==capacity &&
            (rows.back().flags&Row::Truncated) && projected.size()<=tooiePlayerProjectionsPerPresentation,
            "representatives plus duplicates retain shared caps and explicit truncation");
        require(!(rows[5].flags&Row::Representative) && rows[5].group_matrix_count==0,
            "duplicate fill rows do not claim representative counts");
        static_assert(sizeof(TooiePlayerTransform)==336,"representative metadata, transition source and vertex-change witness");
        for(size_t i=0;i<rows.size();++i) for(size_t j=i+1;j<rows.size();++j)
            require(rows[i].workload_id!=rows[j].workload_id || rows[i].index!=rows[j].index,
                "fill pass never emits a selected matrix twice");
        // A ninth distinct projection must not silently lose its matrix detail.
        DrawData cameras;
        for (uint16_t p=0; p<9; ++p) {
            cameras.viewProjTransforms.emplace_back(matrix(float(p+1)));
            add(cameras, 0x20000500u+p, 1.0f, 1, p);
        }
        rows.clear(); projected.clear();
        tooieCollectPlayerRows(10,cameras,0,61,nullptr,nullptr,UINT32_MAX,0,1,0,0,true,true,rows,projected);
        require(projected.size()==8 && (rows.back().flags & Row::ProjectionTruncated), "projection detail cap explicit");
        require((rows[0].flags & Row::TransformsProcessed) && (projected[0].flags & ProjectionRow::ProjectionsProcessed) &&
            !(projected[0].flags & ProjectionRow::Final), "processing requested but missing output is distinct");
        // Modified-vertex copies can retain an earlier world index in a later allocation range.
        DrawData copied;
        projections(copied);
        add(copied,0x20000500u,1,1,0);
        add(copied,0x20000501u,1,2,1);
        copied.worldIndices[1]=0;
        copied.viewProjIndices[1]=0;
        rows.clear(); projected.clear();
        tooieCollectPlayerRows(11,copied,0,62,nullptr,nullptr,UINT32_MAX,0,1,0,0,true,true,rows,projected);
        require(rows[1].projection==1 && !(rows[1].flags & Row::ProjectionMixed) &&
            (rows[1].flags & Row::ProjectionIncomplete),
            "copied earlier transform cannot supply this transform's camera");

        // ---- Warp-UI witness --------------------------------------------------------------------
        // W1: RT64 matchScenes pairing. 3 current x 2 previous; sorted: (2,0)=0,(2,1)=0,(0,1)=.1,(1,0)=.1,...
        {
            const float d[6] = {0.5f, 0.1f, 0.1f, 0.9f, 0.0f, 0.0f};
            uint32_t pairedScenes[3] = {7, 7, 7};
            require(tooiePairScenes(d, 3, 2, pairedScenes) && pairedScenes[0] == 1 &&
                pairedScenes[1] == UINT32_MAX && pairedScenes[2] == 0,
                "greedy nearest pairing, ties in draw order, surplus current scene unpaired");
            const float n[1] = {std::numeric_limits<float>::quiet_NaN()};
            uint32_t one[1] = {5};
            require(!tooiePairScenes(n, 1, 1, one), "NaN difference: RT64 order undefined, pairing not claimed");
            require(!tooiePairScenes(d, tooieOrthoPairingScenes + 1u, 1, one) && one[0] == 5,
                "beyond the 32-scene limit: pairing not claimed or written");
        }
        // W2: the scene interpolation predicate and one OrthoScene row with the prior RT64 actually uses.
        {
            DrawData now, before; projections(now); projections(before);
            GameFrameMap::WorkloadMap sceneMap; sceneMap.mapped = true; sceneMap.prevWorkloadIndex = 1;
            sceneMap.viewProjections.resize(2);
            sceneMap.viewProjections[0].mapped = true; sceneMap.viewProjections[0].prevTransformIndex = 0;
            sceneMap.viewProjections[1].mapped = true; sceneMap.viewProjections[1].prevTransformIndex = 0;
            require(!tooieOrthoSceneInterpolating(now, &sceneMap, &before, 0), "identical prior: not interpolating");
            require(tooieOrthoSceneInterpolating(now, &sceneMap, &before, 1), "different prior: interpolating");
            require(!tooieOrthoSceneInterpolating(now, &sceneMap, nullptr, 1) &&
                !tooieOrthoSceneInterpolating(now, nullptr, &before, 1), "no prior slot or map: never claimed");
            sceneMap.viewProjections[1].prevTransformIndex = 9;
            require(!tooieOrthoSceneInterpolating(now, &sceneMap, &before, 1), "prior outside previous: not claimed");
            sceneMap.viewProjections[1].prevTransformIndex = 0;
            std::vector<ProjectionRow> scenesOut; scenesOut.reserve(1);
            TooieOrthoScene info{};
            info.ordinal = 3; info.position = 2; info.projections = 5; info.scenes = 6; info.previousScenes = 4;
            info.pairedScene = 1; info.pairedGroupId = 0xA0000010u;
            info.flags = ProjectionRow::SceneCensus | ProjectionRow::PairKnown;
            require(tooieCollectOrthoScene(30, now, 2, 90, &sceneMap, &before, 1, 89, 0.25f, true, 1, info, scenesOut) &&
                scenesOut.size() == 1, "one scene row");
            const ProjectionRow &s = scenesOut[0];
            const uint32_t expected = ProjectionRow::OrthoScene | ProjectionRow::SceneCensus | ProjectionRow::PairKnown |
                ProjectionRow::Mapped | ProjectionRow::PriorIndexValid | ProjectionRow::Final | ProjectionRow::GroupKnown |
                ProjectionRow::OwnerMapped | ProjectionRow::PriorSlotValid | ProjectionRow::ProjectionsProcessed;
            require((s.flags & expected) == expected &&
                !(s.flags & (ProjectionRow::SceneTruncated | ProjectionRow::SceneCrossWorkload)), "scene row flags");
            require(s.index == 1 && s.group_id == 0xA0000011u && s.previous_index == 0 &&
                s.previous_group_id == 0xA0000010u && s.raw[0] == bits(101.0f) && s.prior[0] == bits(100.0f) &&
                s.final[0] == bits(201.0f) && s.weight == 0.25f,
                "actual raw, the prior RT64 uses, final, weight and that prior's group");
            require(s.scene == (3u | (2u << 16)) && s.scene_projections == 5 && s.scene_counts == (6u | (4u << 16)) &&
                s.paired_scene == 1 && s.paired_group_id == 0xA0000010u && s.workload_slot == 2 &&
                s.workload_id == 90 && s.previous_slot == 1 && s.previous_workload_id == 89,
                "scene identity, counts, pairing and workload identities");
            require(!tooieCollectOrthoScene(31, now, 2, 90, &sceneMap, &before, 1, 89, 0.5f, true, 0, info, scenesOut) &&
                scenesOut.size() == 1 && scenesOut.capacity() == 1, "no capacity: refused, never reallocated");
            scenesOut.clear(); info.flags = 0;
            tooieCollectOrthoScene(32, now, 2, 90, nullptr, nullptr, UINT32_MAX, 0, 1.0f, false, 0, info, scenesOut);
            require(scenesOut.size() == 1 && (scenesOut[0].flags & ProjectionRow::OrthoScene) &&
                !(scenesOut[0].flags & (ProjectionRow::Mapped | ProjectionRow::PriorIndexValid | ProjectionRow::Final |
                ProjectionRow::PairKnown)) && scenesOut[0].paired_scene == UINT32_MAX,
                "unmapped unprocessed scene: raw only, no pairing claim");
        }
        // W3: AUTO-ortho selection. Only AUTO groups on listed orthographic indices qualify.
        {
            DrawData now, before; projections(now); projections(before);
            add(now, 0x30040000u, 1.0f, 1, 1);   // t0 explicit menu ID on the orthographic index: excluded
            add(now, G_EX_ID_AUTO, 2.0f, 1, 0);  // t1 AUTO on an unlisted index: excluded below
            for (uint32_t i = 0; i < 10; ++i) add(now, G_EX_ID_AUTO, 3.0f + float(i), 2, 1);  // t2..t11
            for (uint32_t i = 0; i < 10; ++i) add(before, G_EX_ID_AUTO, 3.0f, 2, 1);         // priors at x 30
            GameFrameMap::WorkloadMap autoMap; autoMap.mapped = true; autoMap.prevWorkloadIndex = 1;
            autoMap.transforms.resize(now.worldTransforms.size());
            for (uint32_t t = 2; t < 11; ++t) {   // t11 stays unmapped
                autoMap.transforms[t].mapped = true;
                autoMap.transforms[t].prevTransformIndex = t - 2;
            }
            const uint32_t ortho[1] = {1};
            TooieAutoOrthoTop top{};
            tooieCollectAutoOrtho(40, now, 0, 95, &autoMap, &before, 1, 94, 0.5f, 0.5f, Row::CurrentMatched, true,
                ortho, 1, top);
            require(top.eligible == 10 && top.count == tooieAutoOrthoRowsPerPresentation,
                "ten eligible AUTO-ortho transforms, eight retained");
            require(top.rows[0].index == 10 && top.rows[7].index == 3,
                "ranked by mapped translation displacement |raw - prior|, largest first");
            const Row &a = top.rows[0];
            const uint32_t autoFlags = Row::AutoOrtho | Row::Mapped | Row::PriorIndexValid | Row::Interpolated |
                Row::ProjectionKnown | Row::CurrentMatched | Row::OwnerMapped | Row::PriorSlotValid;
            require((a.flags & autoFlags) == autoFlags && a.group_id == G_EX_ID_AUTO && a.projection == 1 &&
                a.group_matrix_count == 10 && a.previous_index == 8 && a.previous_group_id == G_EX_ID_AUTO &&
                a.raw[12] == bits(110.0f) && a.prior[12] == bits(30.0f) && a.weight == 0.5f &&
                a.workload_id == 95 && a.previous_workload_id == 94, "AUTO-ortho row identities and matrices");
            std::vector<Row> out; out.reserve(tooieAutoOrthoRowsPerPresentation);
            tooieEmitAutoOrtho(top, out);
            require(out.size() == tooieAutoOrthoRowsPerPresentation && (out.back().flags & Row::Truncated) &&
                !(out.front().flags & Row::Truncated), "eligible beyond the cap: explicit truncation");
            // Without a map nothing is ranked: draw order, no prior or interpolation claimed.
            TooieAutoOrthoTop plain{};
            tooieCollectAutoOrtho(41, now, 0, 95, nullptr, nullptr, UINT32_MAX, 0, 1.0f, 0.0f, 0, false, ortho, 1, plain);
            require(plain.eligible == 10 && plain.count == 8 && plain.rows[0].index == 2 && plain.rows[7].index == 9 &&
                !(plain.rows[0].flags & (Row::Mapped | Row::Interpolated | Row::PriorSlotValid)),
                "unmapped candidates keep draw order without prior claims");
            // A different orthographic index list selects only its own AUTO transform; no truncation then.
            const uint32_t other[1] = {0};
            TooieAutoOrthoTop single{};
            tooieCollectAutoOrtho(42, now, 0, 95, &autoMap, &before, 1, 94, 0.5f, 0.5f, 0, true, other, 1, single);
            std::vector<Row> one; one.reserve(tooieAutoOrthoRowsPerPresentation);
            tooieEmitAutoOrtho(single, one);
            require(single.eligible == 1 && one.size() == 1 && one[0].index == 1 && one[0].group_matrix_count == 1 &&
                !(one[0].flags & (Row::Truncated | Row::Mapped)), "only listed orthographic indices qualify");
        }
#include "rt64_sprite_onset_cases.inc"
        // W4: unreported parts under an orthographic projection. AUTO, IGNORE and explicit non-root IDs qualify;
        // supplemental roots and player IDs keep their own rows. vertex_delta is RT64's vertex velocity input and ranks
        // a part whose geometry moves under a static matrix above a smaller matrix step.
        {
            DrawData now, before; projections(now); projections(before);
            auto part = [](DrawData &d, uint32_t id, float base, float x) {
                add(d, id, base, 2, 1);
                for (uint32_t v = 0; v < 2; ++v) d.posFloats.insert(d.posFloats.end(), {x, 0.0f, 0.0f});
            };
            part(now, 0x30040000u, 1.0f, 0.0f);                    // t0 menu root: supplemental row, excluded
            part(now, 0x30040001u, 1.0f, 24.0f);                   // t1 menu part: static matrix, vertices +24
            now.transformGroups.back().vertexInterpolation = G_EX_COMPONENT_SKIP;
            part(now, G_EX_ID_IGNORE, 2.0f, 0.0f);                 // t2 IGNORE: never mapped by RT64
            part(now, TOOIE_BK_PLAYER_ID_START + 1u, 3.0f, 0.0f);  // t3 player bone: player rows, excluded
            part(now, G_EX_ID_AUTO, 4.0f, 0.0f);                   // t4 AUTO: matrix step 10, vertices static
            for (float base : {1.0f, 1.0f, 2.0f, 3.0f, 3.0f}) part(before, G_EX_ID_AUTO, base, 0.0f);
            GameFrameMap::WorkloadMap partMap; partMap.mapped = true; partMap.prevWorkloadIndex = 1;
            partMap.transforms.resize(now.worldTransforms.size());
            for (uint32_t t : {1u, 4u}) { partMap.transforms[t].mapped = true; partMap.transforms[t].prevTransformIndex = t; }
            const uint32_t ortho[1] = {1};
            TooieAutoOrthoTop top{};
            tooieCollectAutoOrtho(50, now, 0, 96, &partMap, &before, 1, 95, 0.5f, 0.5f, 0, true, ortho, 1, top);
            require(top.eligible == 3 && top.count == 3 && top.rows[0].index == 1 && top.rows[1].index == 4 &&
                top.rows[2].index == 2, "menu part, AUTO and IGNORE qualify; root and player excluded; vertex change ranks first");
            const Row &p = top.rows[0], &a = top.rows[1], &g = top.rows[2];
            require(p.group_id == 0x30040001u && (p.flags & Row::UnrootedOrtho) && (p.flags & Row::VertexSkip) &&
                (p.flags & Row::Mapped) && p.vertex_delta == 24.0f && p.raw[12] == p.prior[12],
                "static-matrix part: vertex_delta carries the move RT64 will not blend");
            require(a.group_id == G_EX_ID_AUTO && !(a.flags & Row::UnrootedOrtho) && a.vertex_delta == 0.0f &&
                a.raw[12] == bits(40.0f) && a.prior[12] == bits(30.0f), "AUTO row unchanged apart from vertex_delta");
            require(g.group_id == G_EX_ID_IGNORE && (g.flags & Row::UnrootedOrtho) && !(g.flags & Row::Mapped) &&
                g.vertex_delta == -1.0f, "IGNORE part: reported, unmapped, no delta claimed");
        }
        // W5: an option box's root group owns two matrices. Both are observed in the representative pass, with the
        // vertex change RT64 will not blend under a SKIP group; a range-count change is topology, not motion.
        {
            DrawData now, before, shrunk; projections(now); projections(before); projections(shrunk);
            auto box = [](DrawData &d, uint32_t id, float base, uint32_t n, float x) {
                add(d, id, base, n, 1);
                d.transformGroups.back().vertexInterpolation = G_EX_COMPONENT_SKIP;
                for (uint32_t v = 0; v < n; ++v) d.posFloats.insert(d.posFloats.end(), {x, float(v), 0.0f});
            };
            box(now, tooieMenuModelIdStart + 0x600u, 1.0f, 3, 0.0f);    // t0 root group, first matrix
            box(now, tooieMenuModelIdStart + 0x600u, 2.0f, 3, 24.0f);   // t1 same group, vertices +24
            box(now, tooieMenuModelIdStart + 0x700u, 3.0f, 4, 5.0f);    // t2 sprite component
            box(before, tooieMenuModelIdStart + 0x600u, 1.0f, 3, 0.0f);
            box(before, tooieMenuModelIdStart + 0x600u, 2.0f, 3, 0.0f);
            box(before, tooieMenuModelIdStart + 0x700u, 3.0f, 4, 5.0f);
            box(shrunk, tooieMenuModelIdStart + 0x600u, 1.0f, 3, 0.0f);
            box(shrunk, tooieMenuModelIdStart + 0x600u, 2.0f, 2, 0.0f);  // prior range one vertex shorter
            box(shrunk, tooieMenuModelIdStart + 0x700u, 3.0f, 4, 5.0f);
            GameFrameMap::WorkloadMap boxMap; boxMap.mapped = true; boxMap.prevWorkloadIndex = 1;
            boxMap.transforms.resize(now.worldTransforms.size());
            for (uint32_t t = 0; t < 3; ++t) { boxMap.transforms[t].mapped = true; boxMap.transforms[t].prevTransformIndex = t; }
            rows.clear(); projected.clear();
            tooieCollectPlayerRows(30, now, 0, 90, &boxMap, &before, 1, 89, 0.5f, 0.5f, 0, true, true, rows, projected, true, true);
            require(rows.size() == 3 && rows[0].index == 0 && rows[1].index == 1 && rows[2].index == 2,
                "representative pass retains every matrix of a menu group");
            require((rows[0].flags & Row::Representative) && rows[0].group_matrix_count == 2 &&
                !(rows[1].flags & Row::Representative) && rows[1].group_matrix_count == 0 &&
                (rows[2].flags & Row::Representative) && rows[2].group_matrix_count == 1,
                "one representative per group, counting both box matrices");
            require((rows[1].flags & (Row::Mapped | Row::VertexSkip)) == (Row::Mapped | Row::VertexSkip) &&
                (rows[1].flags & Row::VertexMeasured) && rows[1].vertex_delta == 24.0f && rows[0].vertex_delta == 0.0f && rows[2].vertex_delta == 0.0f,
                "vertex motion under a matched menu root is measured");
            tooieCollectPlayerRows(31, now, 0, 90, &boxMap, &before, 1, 89, 0.5f, 0.5f, 0, true, true, rows, projected, true, false);
            require(rows.size() == 3, "the fill pass adds no duplicate of an already retained menu matrix");
            rows.clear(); projected.clear();
            tooieCollectPlayerRows(32, now, 0, 90, &boxMap, &shrunk, 1, 89, 0.5f, 0.5f, 0, true, true, rows, projected, true, true);
            require(rows.size() == 3 && rows[1].vertices == 3 && rows[1].previous_vertices == 2 && rows[1].vertex_delta == -1.0f,
                "range-count change is topology: counts differ and no motion is claimed");
            rows.clear(); projected.clear();
            tooieCollectPlayerRows(33, now, 0, 90, &boxMap, &before, 1, 89, 0.5f, 0.5f, 0, true, true, rows, projected);
            require(rows.empty(), "player pass unchanged: menu IDs are not player rows");
        }
        { // Saturated supplemental budget: menu matrices precede unrelated actor representatives.
            DrawData busy; projections(busy);
            for (uint32_t i=0;i<tooieOwnedRootsPerPresentation;++i) add(busy,TOOIE_BK_ACTOR_ID_START+i*256,1.0f,2,1);
            add(busy,tooieMenuModelIdStart,1.0f,2,1);
            add(busy,tooieMenuModelIdStart,2.0f,2,1);
            add(busy,tooieMenuModelIdStart+256,3.0f,2,1);
            rows.clear();projected.clear();
            tooieCollectPlayerRows(34,busy,0,91,nullptr,nullptr,UINT32_MAX,0,0.5f,0.5f,0,true,true,rows,projected,true,true);
            require(rows.size()==tooieOwnedRootsPerPresentation && rows[0].group_id==tooieMenuModelIdStart &&
                rows[1].group_id==tooieMenuModelIdStart && rows[2].group_id==tooieMenuModelIdStart+256,
                "menu matrices survive a full earlier actor census without increasing the cap");
        }
        {   // L05 evidence: the legacy player boundary's bridge-domain +129 outer group (player_boundary.c:240,264).
            require(TOOIE_BRIDGE_ID_BASE==0x90000000u && TOOIE_BRIDGE_ID_BASE+TOOIE_BRIDGE_BLOCK_COUNT*512u==0xA0000000u,
                "bridge block domain [0x90000000,0xA0000000)");
            const uint32_t last=TOOIE_BRIDGE_ID_BASE+(TOOIE_BRIDGE_BLOCK_COUNT-1u)*512u;
            require(tooieIsBridgePlayerOuterId(TOOIE_BRIDGE_ID_BASE+129u) &&
                tooieIsBridgePlayerOuterId(TOOIE_BRIDGE_ID_BASE+512u+129u) && tooieIsBridgePlayerOuterId(last+129u),
                "every bridge block's +129, first to last block");
            require(!tooieIsBridgePlayerOuterId(TOOIE_BRIDGE_ID_BASE+128u) && !tooieIsBridgePlayerOuterId(TOOIE_BRIDGE_ID_BASE+130u) &&
                !tooieIsBridgePlayerOuterId(TOOIE_BRIDGE_ID_BASE+1u) && !tooieIsBridgePlayerOuterId(TOOIE_BRIDGE_ID_BASE-512u+129u) &&
                !tooieIsBridgePlayerOuterId(0xA0000000u+129u) && !tooieIsBridgePlayerOuterId(TOOIE_BK_PLAYER_ID_START+129u) &&
                !tooieIsBridgePlayerOuterId(0u) && !tooieIsBridgePlayerOuterId(0xFFFFFFFFu),
                "other offsets, the camera domain and BK player IDs excluded");
            DrawData d;
            add(d,TOOIE_BK_PLAYER_ID_START+129u,1.0f,1);
            add(d,TOOIE_BRIDGE_ID_BASE+3u*512u+129u,2.0f,1);
            add(d,TOOIE_BRIDGE_ID_BASE+1u,3.0f,0);
            add(d,last+129u,4.0f,0);
            d.worldTransforms.emplace_back(matrix(5.0f));   // no group entry: unresolved, never a silent miss
            TooieBridgeOuterCount count;
            tooieCountBridgePlayerOuter(d,count);
            require(count.worlds==5 && count.players==1 && count.unresolved==1 && count.hits==2 && count.firstIndex==1 &&
                count.firstId==TOOIE_BRIDGE_ID_BASE+3u*512u+129u,
                "every world transform counted; +129 hits counted; unresolved reported separately");
            DrawData clean;
            add(clean,TOOIE_BK_PLAYER_ID_START,1.0f,1);
            TooieBridgeOuterCount none;
            tooieCountBridgePlayerOuter(clean,none);
            require(none.worlds==1 && none.players==1 && !none.hits && !none.unresolved && none.firstIndex==UINT32_MAX,
                "negative: BK player ID only, no hit");
            DrawData empty;
            TooieBridgeOuterCount zero;
            tooieCountBridgePlayerOuter(empty,zero);
            require(!zero.worlds && !zero.hits && !zero.unresolved, "empty draw data counts nothing; coverage is reported separately");
        }
        {   // L05 F4 run: one bounded run per session (rt64_trace_window.hpp OneShot).
            using tooie::rt64_trace::OneShot;
            OneShot run;
            require(run.next(false)==OneShot::idle && run.next(false)==OneShot::idle && !run.completed(),
                "idle until the first F4");
            require(run.next(true)==0u, "the first arm starts the run");
            require(run.next(true)==1u && run.next(false)==2u, "a press during the run neither restarts nor queues");
            for (uint32_t i=3;i<OneShot::duration;++i) require(run.next(i%2u==0u)==i, "every run presentation indexed");
            require(run.completed() && run.next(true)==OneShot::idle && run.next(false)==OneShot::idle && run.completed(),
                "completed: later F4s start nothing");
        }
        std::printf("baseline_rt64_player_rows: ok\n");
        return 0;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "baseline_rt64_player_rows: %s\n", e.what());
        return 1;
    }
}
