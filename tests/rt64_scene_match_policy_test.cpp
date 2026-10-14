#include "rt64_scene_match_policy.hpp"
#include <cassert>
#include <algorithm>
#include <array>
#include <vector>

struct Projection { int type; };
struct Pair { uint32_t projectionCount; std::vector<Projection> projections; };
struct Workload { uint32_t fbPairCount; std::vector<Pair> fbPairs; };


#ifdef TOOIE_V10_ARM
#include "hle/rt64_workload_queue.h"
#include "common/rt64_math.h"
#include <cstdio>
#include <stdexcept>
namespace {
constexpr bool innerRemoved = TOOIE_V10_ARM != 0;
constexpr bool outerRemoved = TOOIE_V10_ARM == 2;
struct OwnerFixture {
    RT64::WorkloadQueue queue;
    RT64::GameFrame current, previous;
    bool upload = false, tile = false, look = false;
    void projection(uint32_t owner, uint32_t index, float x, uint32_t id) {
        auto &work = queue.workloads[owner];
        work.debuggerCamera.enabled = false;
        work.fbPairCount = 1;
        work.fbPairs.resize(1);
        auto &pair = work.fbPairs[0];
        pair.colorImage.address = 4096;
        pair.colorImage.fmt = 0;
        pair.colorImage.siz = 2;
        pair.colorImage.width = 320;
        pair.depthRead = pair.depthWrite = false;
        pair.scissorRect = RT64::FixedRect(0, 0, 1280, 960);
        auto &d = work.drawData;
        d.viewTransforms.resize(3);
        d.projTransforms.resize(3);
        d.viewProjTransforms.resize(3);
        d.viewportOrigins.resize(3);
        d.viewProjTransformGroups.resize(3);
        d.transformGroups.resize(3);
        d.viewTransforms[index] = RT64::matrixTranslation(hlslpp::float3(x, 0.0f, 0.0f));
        d.projTransforms[index] = hlslpp::float4x4(2,0,0,0, 0,2,0,0, 0,0,-1,-1, 0,0,-2,0);
        d.viewProjTransforms[index] = hlslpp::mul(d.viewTransforms[index], d.projTransforms[index]);
        d.viewProjTransformGroups[index] = index;
        auto &group = d.transformGroups[index];
        group.matrixId = id;
        group.aspectMode = G_EX_ASPECT_STRETCH;
        group.decompose = false;
        RT64::Projection projection;
        projection.type = RT64::Projection::Type::Perspective;
        projection.transformsIndex = index;
        projection.scissorRect = pair.scissorRect;
        pair.projections.push_back(projection);
        pair.projectionCount = uint32_t(pair.projections.size());
    }
    void set(bool unmatched = false) {
        const uint32_t c[] = {0,1}, p[] = {2,3};
        current.set(queue, c, 2);
        previous.set(queue, p, unmatched ? 1 : 2);
    }
    void match() {
        if (innerRemoved) assert(previous.workloads.size() >= current.workloads.size());
        current.match(nullptr, queue, previous, nullptr, upload, tile, look);
        assert(!upload);
    }
    auto &map(uint32_t owner, uint32_t index) {
        assert(owner < current.frameMap.workloads.size());
        auto &parent = current.frameMap.workloads[owner];
        assert(index < parent.viewProjections.size());
        return parent.viewProjections[index];
    }
    void report(const char *name, uint32_t owner, uint32_t index) {
        auto &value = map(owner,index);
        std::printf("V10 arm=%u case=%s owner=%u index=%u mapped=%u previous=%u upload=%u\n",
            unsigned(TOOIE_V10_ARM), name, owner, index, unsigned(value.mapped),
            value.prevTransformIndex, unsigned(upload));
    }
};
void cross_parent(bool positive, bool direct = false) {
    OwnerFixture f;
    f.projection(0,1,0,17); f.projection(1,1,32,19);
    f.projection(2,1,positive ? 0 : 32,positive ? 17 : 19);
    f.projection(3,1,positive ? 32 : 0,positive ? 19 : 17);
    f.set();
    assert(f.current.perspectiveScenes.size()==2 && f.previous.perspectiveScenes.size()==2);
    f.match();
    assert(f.current.frameMap.workloads[0].prevWorkloadIndex==2);
    assert(f.current.frameMap.workloads[1].prevWorkloadIndex==3);
    if (direct) {
        assert(!outerRemoved && !positive && !f.map(0,1).mapped);
        std::unordered_map<uint32_t,RT64::ModifiedBuffers> modified;
        f.current.matchScene(f.queue,f.previous,f.current.perspectiveScenes[0],
            f.previous.perspectiveScenes[1],modified,f.tile,f.look);
        assert(modified.empty() && !f.tile && !f.look);
        assert(f.map(0,1).mapped==innerRemoved);
    } else {
        assert(f.map(0,1).mapped==(positive || outerRemoved));
        assert(f.map(1,1).mapped==(positive || outerRemoved));
    }
    f.report(direct ? "direct-parent-defense" : positive ? "same-parent" : "cross-parent",0,1);
}
void projection_owner(bool positive) {
    OwnerFixture f;
    for (uint32_t owner : {0u,2u}) {
        f.projection(owner,1,0,17); f.projection(owner,2,32,19);
    }
    if (!positive) { f.projection(1,2,0,17); f.projection(3,2,0,17); }
    f.set();
    assert(f.current.perspectiveScenes.size()==2);
    assert(f.current.perspectiveScenes[0].projections.size()==(positive ? 1u : 2u));
    assert(f.current.perspectiveScenes[0].projections[0].workloadIndex==0);
    if (!positive) assert(f.current.perspectiveScenes[0].projections[1].workloadIndex==1);
    assert(f.current.perspectiveScenes[1].projections[0].workloadIndex==0);
    f.match();
    assert(f.map(0,1).mapped && f.map(0,1).prevTransformIndex==1);
    assert(f.map(0,2).mapped && f.map(0,2).prevTransformIndex==((innerRemoved&&!positive)?1u:2u));
    f.report(positive ? "same-projection-owner" : "foreign-current-projection",0,2);
}
void previous_projection_owner(bool positive) {
    OwnerFixture f;
    f.projection(0,1,0,G_EX_ID_IGNORE);
    f.projection(0,2,0,17);
    f.projection(2,1,0,G_EX_ID_IGNORE);
    f.projection(2,2,positive ? 0 : 32,positive ? 17 : 19);
    if (!positive) {
        auto &pair = f.queue.workloads[2].fbPairs[0];
        pair.projections.pop_back();
        pair.projectionCount = 1;
        f.projection(3,2,0,17);
    }
    f.set();
    assert(f.current.perspectiveScenes.size()==1 && f.previous.perspectiveScenes.size()==1);
    f.match();
    assert(!f.map(0,1).mapped);
    assert(f.map(0,2).mapped==(positive || innerRemoved));
    if (f.map(0,2).mapped) assert(f.map(0,2).prevTransformIndex==2);
    f.report(positive ? "same-previous-projection" : "foreign-previous-projection",0,2);
}
void unmatched_parent() {
    assert(!innerRemoved);
    OwnerFixture f;
    f.projection(0,1,0,17); f.projection(1,1,32,19); f.projection(2,1,0,17);
    f.set(true);
    f.match();
    assert(!f.current.frameMap.workloads[1].mapped);
    assert(f.current.frameMap.workloads[1].viewProjections.empty());
    std::unordered_map<uint32_t,RT64::ModifiedBuffers> modified;
    f.current.matchScene(f.queue,f.previous,f.current.perspectiveScenes[1],
        f.previous.perspectiveScenes[0],modified,f.tile,f.look);
    assert(modified.empty() && !f.tile && !f.look && !f.upload);
    assert(f.current.frameMap.workloads[1].viewProjections.empty());
    std::printf("V10 arm=0 case=unmatched-parent refused=1 upload=0\n");
}
enum class Domain { World, Tile, LookAt };
void call_data(OwnerFixture &f, uint32_t owner, Domain domain, bool callPresent) {
    auto &d = f.queue.workloads[owner].drawData;
    d.worldTransforms.resize(2);
    d.worldTransforms[1] = RT64::matrixTranslation(hlslpp::float3(owner%2 ? 100.0f : 12.0f,0,0));
    if (owner<2) d.worldTransforms[1][3][0] += 1.0f;
    d.worldTransformGroups.assign(2,0);
    d.worldTransformPhysicalAddresses.assign(2,0);
    d.worldTransformVertexIndices.assign(2,0);
    d.worldIndices.assign(3,1);
    d.posFloats = {0,0,0, 1,0,0, 0,1,0};
    d.faceIndices = {0,1,2};
    auto &group = d.transformGroups[0];
    group.matrixId = domain==Domain::World ? G_EX_ID_AUTO : G_EX_ID_IGNORE;
    group.ordering = G_EX_ORDER_AUTO;
    group.vertexInterpolation = group.texcoordInterpolation = G_EX_COMPONENT_SKIP;
    group.tileInterpolation = domain==Domain::Tile ? G_EX_COMPONENT_INTERPOLATE : G_EX_COMPONENT_SKIP;
    d.callTiles.resize(2);
    d.rdpTiles.resize(2);
    auto &tile = d.rdpTiles[1];
    tile.fmt=0; tile.siz=2; tile.stride=8;
    tile.masks=tile.maskt=8; tile.shifts=tile.shiftt=0;
    tile.cms=tile.cmt=G_TX_WRAP;
    tile.uls=owner<2 ? 1.0f : 0.0f; tile.ult=0;
    tile.lrs=owner<2 ? 5.0f : 4.0f; tile.lrt=4;
    d.lookAtIndices.assign(3,(1<<RSP_LOOKAT_INDEX_SHIFT)|RSP_LOOKAT_INDEX_ENABLED);
    d.rspLookAt.resize(2);
    if (owner%2==0) {
        d.rspLookAt[1].x = owner<2 ? hlslpp::float3(1,0,0) : hlslpp::float3(0,1,0);
        d.rspLookAt[1].y = hlslpp::float3(0,0,1);
    } else {
        d.rspLookAt[1].x = owner<2 ? hlslpp::float3(0,1,0) : hlslpp::float3(0,0,1);
        d.rspLookAt[1].y = hlslpp::float3(1,0,0);
    }
    assert(d.worldTransformVertexCount(1)==3 && d.posFloats.size()==9);
    if (!callPresent) return;
    RT64::GameCall call{};
    call.callDesc.minWorldMatrix = call.callDesc.maxWorldMatrix = 1;
    call.callDesc.triangleCount = 1;
    call.callDesc.geometryMode = domain==Domain::LookAt ? G_LIGHTING|G_TEXTURE_GEN : 0;
    call.callDesc.colorCombiner.L = call.callDesc.colorCombiner.H = 0;
    call.callDesc.otherMode.L = call.callDesc.otherMode.H = 0;
    call.callDesc.tileIndex=1; call.callDesc.tileCount=1;
    call.meshDesc.faceIndicesStart=0;
    auto &projection=f.queue.workloads[owner].fbPairs[0].projections[0];
    projection.gameCalls.push_back(call);
    projection.gameCallCount=1;
}
void call_owner(Domain domain, bool currentForeign, bool previousForeign) {
    assert(!outerRemoved || (!currentForeign && !previousForeign));
    OwnerFixture f;
    for (uint32_t owner=0;owner<4;++owner) {
        f.projection(owner,1,0,17);
        bool present=owner==(currentForeign ? 1u : 0u) || owner==(previousForeign ? 3u : 2u);
        call_data(f,owner,domain,present);
    }
    const uint32_t c=currentForeign ? 1 : 0, p=previousForeign ? 3 : 2;
    const auto &cc=f.queue.workloads[c].fbPairs[0].projections[0].gameCalls[0];
    const auto &pc=f.queue.workloads[p].fbPairs[0].projections[0].gameCalls[0];
    const uint32_t id=domain==Domain::World ? G_EX_ID_AUTO : G_EX_ID_IGNORE;
    assert(f.current.hashFromCall(cc,id)==f.previous.hashFromCall(pc,id));
    f.set();
    assert(f.current.perspectiveScenes.size()==1 && f.previous.perspectiveScenes.size()==1);
    assert(f.current.perspectiveScenes[0].projections.size()==2);
    f.match();
    auto &map=f.current.frameMap.workloads[0];
    const bool expected=innerRemoved || (!currentForeign && !previousForeign);
    bool mapped=false, used=false;
    if (domain==Domain::World) {
        assert(map.transforms.size()>1 && map.prevTransformsMapped.size()>1);
        mapped=map.transforms[1].mapped; used=map.prevTransformsMapped[1];
        if (mapped) assert(map.transforms[1].prevTransformIndex==1);
        assert(!f.tile && !f.look);
    } else if (domain==Domain::Tile) {
        assert(map.tiles.size()>1 && map.prevTilesMapped.size()>1);
        mapped=map.tiles[1].mapped; used=map.prevTilesMapped[1];
        if (mapped) assert(map.tiles[1].deltaUls==1 && map.tiles[1].deltaLrs==1);
        assert(f.tile==expected && !f.look);
    } else {
        assert(map.lookAt.size()>1 && map.prevLookAtMapped.size()>1);
        mapped=map.lookAt[1].mapped; used=map.prevLookAtMapped[1];
        if (mapped) {
            assert(float(map.lookAt[1].deltaX[0])==1.0f);
            assert(float(map.lookAt[1].deltaX[1])==-1.0f);
            assert(float(map.lookAt[1].deltaX[2])==0.0f);
        }
        assert(f.look==expected && !f.tile);
    }
    assert(mapped==expected && used==expected && !f.upload);
    for (uint32_t owner=0;owner<4;++owner) {
        assert(f.queue.workloads[owner].drawData.velFloats.empty());
        assert(f.queue.workloads[owner].drawData.tcVelFloats.empty());
    }
    std::printf("V10 arm=%u case=call-owner domain=%u current_foreign=%u previous_foreign=%u mapped=%u previous_used=%u upload=0\n",
        unsigned(TOOIE_V10_ARM),unsigned(domain),unsigned(currentForeign),unsigned(previousForeign),unsigned(mapped),unsigned(used));
}
void run_owner_cases() {
    cross_parent(false); cross_parent(true);
    if (!outerRemoved) cross_parent(false,true);
    projection_owner(true);
    if (!outerRemoved) projection_owner(false);
    if (!outerRemoved) {
        previous_projection_owner(false); previous_projection_owner(true);
        for (Domain domain : {Domain::World,Domain::Tile,Domain::LookAt}) {
            call_owner(domain,true,false); call_owner(domain,false,true);
            call_owner(domain,true,true); call_owner(domain,false,false);
        }
    }
    if (outerRemoved) {
        previous_projection_owner(true);
        for (Domain domain : {Domain::World,Domain::Tile,Domain::LookAt})
            call_owner(domain,false,false);
    }
    if (!innerRemoved) unmatched_parent();
}
}
#endif

int main() {
    using namespace tooie::rt64_match;
    // Gloomy Caverns: main pass pair1/slot1 and secondary pair3/slot2
    // were swapped for thirteen guest frames despite stable draw-pass order.
    Workload current{4, {{1, {{0}}}, {2, {{0}, {1}}},
        {2, {{0}, {1}}}, {5, {{0}, {1}, {1}, {1}, {1}}}}};
    auto previous = current;
    assert(same_projection_prefix(current, previous, 3, 4));

    // Pause entry capture 56814 -> 56815: all-orthographic slots grow from
    // 24 to 26 as a counter is inserted. Current slot 7 is previous slot 5,
    // despite an identical type prefix. Treating the ordinal as an identity
    // pulled its screen centroid 172 pixels toward a different counter.
    Workload oldMenu{1, {{24, std::vector<Projection>(24, {2})}}};
    Workload newMenu{1, {{26, std::vector<Projection>(26, {2})}}};
    const bool menuOrdinal = same_projection_prefix(newMenu, oldMenu, 0, 7) ||
        same_projection_prefix(newMenu, oldMenu, 0, 5);
    assert(!menuOrdinal);
    assert(allow_scene_pass_pair(menuOrdinal, 0, 7, 0, 5));
    assert(allow_scene_pass_pair(true, 1, 1, 1, 1));
    assert(allow_scene_pass_pair(true, 3, 2, 3, 2));
    assert(!allow_scene_pass_pair(true, 1, 1, 3, 2));
    assert(!allow_scene_pass_pair(true, 3, 2, 1, 1));
    assert(!allow_scene_pass_pair(true, 3, 2, 3, 3));
    // Replay the observed ranking crossover: nearest-matrix candidates would
    // greedily choose 1->4 and 4->1. Pass provenance must preserve 1->1/4->4.
    struct Candidate { float score; uint32_t current, previous; };
    std::array<Candidate, 4> candidates{{{0.1f, 0, 1}, {0.2f, 1, 0},
        {1.0f, 0, 0}, {1.1f, 1, 1}}};
    std::stable_sort(candidates.begin(), candidates.end(),
        [](const auto& a, const auto& b) { return a.score < b.score; });
    constexpr uint32_t pairs[]{1, 3}, slots[]{1, 2}, transforms[]{1, 4};
    const auto select = [&](bool stable) {
        std::array<uint32_t, 2> mapped{0, 0};
        std::array<bool, 2> used{};
        for (const auto& c : candidates) {
            if (mapped[c.current] || used[c.previous] ||
                !allow_scene_pass_pair(stable, pairs[c.current], slots[c.current],
                    pairs[c.previous], slots[c.previous])) continue;
            mapped[c.current] = transforms[c.previous];
            used[c.previous] = true;
        }
        return mapped;
    };
    assert((select(true) == std::array<uint32_t, 2>{1, 4}));
    assert((select(false) == std::array<uint32_t, 2>{4, 1}));
    // Captured workload3721 keeps main pair1/slot1 but drops later pair3
    // projections. That tail change must not allow main -> old secondary.
    previous = current;
    previous.fbPairs[3].projectionCount = 2;
    assert(!allow_scene_pass_pair(same_projection_prefix(current, previous, 1, 1), 1, 1, 3, 2));
    // The reverse candidate must also reserve the proven main-view history.
    assert(same_projection_prefix(current, previous, 1, 1));
    assert(!same_projection_prefix(current, previous, 3, 2));
    assert(!allow_scene_pass_pair(
        same_projection_prefix(current, previous, 3, 2) ||
        same_projection_prefix(current, previous, 1, 1), 3, 2, 1, 1));
    // A missing target cannot be treated as an established pass.
    assert(!same_projection_prefix(current, previous, 3, 4));
    // Changes after the target within its own pair are harmless too.
    previous = current;
    previous.fbPairs[1].projections.push_back({1});
    ++previous.fbPairs[1].projectionCount;
    assert(same_projection_prefix(current, previous, 1, 1));
    previous = current;
    // Changed pass layout retains the existing distance-based fallback.
    previous.fbPairs[1].projectionCount = 1;
    assert(!same_projection_prefix(current, previous, 3, 4));
    assert(allow_scene_pass_pair(false, 1, 1, 3, 2));
    previous = current;
    previous.fbPairs[1].projections[1].type = 2;
    assert(!same_projection_prefix(current, previous, 3, 4));
    previous = current;
    previous.fbPairCount = 3;
    assert(!same_projection_prefix(current, previous, 3, 4));
    previous = current;
    // RT64 retains spare vector capacity: inactive slots aren't topology.
    previous.fbPairs.push_back({0, {}});
    previous.fbPairs[1].projections.push_back({2});
    assert(same_projection_prefix(current, previous, 3, 4));
#ifdef TOOIE_V10_ARM
    run_owner_cases();
#endif
}
