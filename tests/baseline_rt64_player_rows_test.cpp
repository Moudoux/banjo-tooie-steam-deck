// F4 player correspondence collector (rt64_player_rows.hpp) against synthetic RT64 draw data and
// frame maps. No game data, renderer or GPU.
#include "rt64_player_rows.hpp"
#include "guest/silo-candidate/bk_pipeline.h"

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
            tooieIsSupplementalRootId(tooieMenuModelIdStart+0x200u), "owned root namespaces");
        require(!tooieIsSupplementalRootId(TOOIE_BK_ACTOR_ID_START+1u) &&
            !tooieIsSupplementalRootId(TOOIE_BK_ACTOR_ID_END) &&
            !tooieIsSupplementalRootId(tooieTransitionIdEnd) &&
            !tooieIsSupplementalRootId(tooieMenuModelIdStart+0x100u) &&
            !tooieIsSupplementalRootId(0u) && !tooieIsSupplementalRootId(0x2001C001u),
            "actor bones, reserved menu sprites, zero and shadow bone offsets excluded");
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
            rows[3].group_id==0x2001E100u && rows[4].group_id==tooieMenuModelIdStart+0x400u,
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
        static_assert(sizeof(TooiePlayerTransform)==328,"representative metadata plus typed transition source summary");
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
        std::printf("baseline_rt64_player_rows: ok\n");
        return 0;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "baseline_rt64_player_rows: %s\n", e.what());
        return 1;
    }
}
