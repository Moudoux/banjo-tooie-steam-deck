#if BASELINE_PLAYER_BOUNDARY
extern "C" uint32_t baseline_player_observed_idbase();
extern "C" bool baseline_palette_observation_mode();
extern "C" uint64_t baseline_palette_observed_blocks();
#include "render_bridge.h"
#endif
#if !BASELINE_SILO_FAMILY_OBSERVER
// Included inside RT64; render-thread scalars only, no GPU/presentation/lifetime claim.
static void observeHoneyFrameMatch(WorkloadQueue &queue, const GameFrame &curFrame, const GameFrame &prevFrame,
    bool processed, bool uploaded, float curWeight, float prevWeight, uint32_t targetRate, bool targetEmpty) {
    static uint64_t calls = 0, rows = 0, candidates = 0, unmapped = 0, multi = 0, changing = 0, interior = 0, failures = 0;
    static uint64_t currentMatched = 0, previousMatched = 0, processCalls = 0, uploadCalls = 0, adjacentCandidates = 0;
    static std::vector<std::array<uint64_t, 4>> pairs;
    static bool stopped = false;
    ++calls;
    currentMatched += curFrame.matched;
    previousMatched += prevFrame.matched;
    processCalls += processed;
    uploadCalls += uploaded;
    auto counters = [&]() {
        std::fprintf(stderr, "FM_COUNTER calls=%llu rows=%llu pairs=%zu candidates=%llu unmapped=%llu multi=%llu changing=%llu interior=%llu failures=%llu currentMatched=%llu previousMatched=%llu processCalls=%llu uploadCalls=%llu adjacentCandidates=%llu stopped=%u\n",
            calls, rows, pairs.size(), candidates, unmapped, multi, changing, interior, failures, currentMatched, previousMatched, processCalls, uploadCalls, adjacentCandidates, unsigned(stopped));
        std::fflush(stderr);
    };
    if (calls == 1 || calls % 1000 == 0) counters();
    if (stopped) return;
    auto fail = [&](const char *reason) {
        ++failures;
        if (rows < 512) { ++rows; std::fprintf(stderr, "FM_FAILURE call=%llu reason=%s\n", calls, reason); }
        std::fflush(stderr);
        if (rows == 512) { stopped = true; counters(); }
    };
    auto honey = [](const TransformGroup &g) {
        return (g.matrixId & 0xF0000000u) == 0x90000000u && (g.matrixId & 0x1FFu) == 0x001u && g.decompose &&
            g.positionInterpolation == 1 && g.rotationInterpolation == 1 && g.scaleInterpolation == 1 &&
            g.skewInterpolation == 0 && g.perspectiveInterpolation == 0 && g.vertexInterpolation == 0 &&
            g.texcoordInterpolation == 0 && g.tileInterpolation == 0 && g.lookAtInterpolation == 0 &&
            g.ordering == 0 && g.aspectMode == 0 && g.editable == 0;
    };
    if (curFrame.workloads.size() != 1 || prevFrame.workloads.size() > 1) { fail("OWNER_COUNT"); return; }
    const uint32_t c = curFrame.workloads[0];
    if (c >= queue.workloads.size()) { fail("CURRENT_OWNER_INDEX"); return; }
    const auto &cw = queue.workloads[c];
    const auto &cd = cw.drawData;
    const auto *wm = c < curFrame.frameMap.workloads.size() ? &curFrame.frameMap.workloads[c] : nullptr;
    const bool ownerMapped = wm && wm->mapped;
    const uint32_t p = ownerMapped ? wm->prevWorkloadIndex : (prevFrame.workloads.empty() ? UINT32_MAX : prevFrame.workloads[0]);
    if (p != UINT32_MAX && (p >= queue.workloads.size() || std::find(prevFrame.workloads.begin(), prevFrame.workloads.end(), p) == prevFrame.workloads.end())) { fail("PREVIOUS_OWNER_INDEX_MEMBERSHIP"); return; }
    if (ownerMapped && p == UINT32_MAX) { fail("PREVIOUS_OWNER_INDEX_MEMBERSHIP"); return; }
    const Workload *pw = p != UINT32_MAX ? &queue.workloads[p] : nullptr;
    const uint64_t adj = pw ? cw.workloadId - pw->workloadId : UINT64_MAX;
    const bool adjacentWorkloads = pw && pw->workloadId != UINT64_MAX && adj == 1;
    if (pw && (p == c || pw->workloadId == cw.workloadId)) { fail("SELF_PAIR"); return; }
    struct Uses { std::vector<uint32_t> vertices, faces, ordinal; std::unordered_map<uint32_t, uint32_t> count; };
    auto scan = [&](const DrawData &d, Uses &u) {
        const size_t n = d.worldTransforms.size();
        if (n > 65536 || d.worldTransformGroups.size() != n || d.transformGroups.size() > 65536 || d.worldIndices.size() > 65536 || d.faceIndices.size() > 65536) return false;
        u.vertices.assign(n, 0);
        u.faces.assign(n, 0);
        u.ordinal.assign(n, 0);
        for (size_t t = 0; t < n; ++t) {
            if (d.worldTransformGroups[t] >= d.transformGroups.size()) return false;
            const auto &g = d.transformGroups[d.worldTransformGroups[t]];
            if (g.ordering == G_EX_ORDER_LINEAR && g.matrixId != G_EX_ID_AUTO && g.matrixId != G_EX_ID_IGNORE) u.ordinal[t] = u.count[g.matrixId]++;
        }
        for (uint32_t t : d.worldIndices) { if (t >= n) return false; ++u.vertices[t]; }
        for (uint32_t v : d.faceIndices) { if (v >= d.worldIndices.size()) return false; ++u.faces[d.worldIndices[v]]; }
        return true;
    };
    Uses cu, pu;
    if (!scan(cd, cu) || (pw && !scan(pw->drawData, pu))) { fail("INPUT_BOUND_OR_INDEX"); return; }
    bool bearing = false;
    for (size_t t = 0; t < cd.worldTransforms.size(); ++t) bearing |= honey(cd.transformGroups[cd.worldTransformGroups[t]]) && cu.vertices[t] != 0;
    if (!bearing) return;
    const std::array<uint64_t, 4> pair = { c, cw.workloadId, p, pw ? pw->workloadId : UINT64_MAX };
    if (std::find(pairs.begin(), pairs.end(), pair) == pairs.end()) {
        if (pairs.size() == 120) { stopped = true; counters(); return; }
        pairs.push_back(pair);
    }
    for (uint32_t t = 0; t < cd.worldTransforms.size() && rows < 512; ++t) {
        const auto &g = cd.transformGroups[cd.worldTransformGroups[t]];
        if (!honey(g) || !cu.vertices[t]) continue;
        ++candidates;
        adjacentCandidates += adjacentWorkloads;
        const uint32_t cc = cu.count[g.matrixId];
        const char *status = wm ? "UNMAPPED_OWNER" : "NO_OWNER_MAP";
        uint32_t pt = UINT32_MAX, pc = pw ? pu.count[g.matrixId] : 0, po = UINT32_MAX, pv = 0, pf = 0;
        float endpoint = 0, outputPrev = 0, outputCur = 0;
        bool finite = std::isfinite(curWeight) && std::isfinite(prevWeight), qualified = false, output = false;
        const hlslpp::float4x4 current = cd.worldTransforms[t];
        for (int r = 0; r < 4; ++r) for (int col = 0; col < 4; ++col) finite &= std::isfinite(float(current[r][col]));
        if (ownerMapped) {
            if (t >= wm->transforms.size()) { fail("TRANSFORM_MAP_INDEX"); continue; }
            const auto &tm = wm->transforms[t];
            status = "UNMAPPED_WORLD";
            if (tm.mapped) {
                pt = tm.prevTransformIndex;
                const auto &pd = pw->drawData;
                if (pt >= pd.worldTransforms.size()) { fail("PREVIOUS_WORLD_INDEX"); continue; }
                const auto &pg = pd.transformGroups[pd.worldTransformGroups[pt]];
                pc = pu.count[g.matrixId]; po = pu.ordinal[pt]; pv = pu.vertices[pt]; pf = pu.faces[pt];
                qualified = honey(pg) && pg.matrixId == g.matrixId && pv;
                status = qualified ? "MAPPED_STATIC" : "PREVIOUS_POLICY_ID_VERTEX_MISMATCH";
                output = qualified && processed && uploaded;
                if (output && t >= cd.lerpWorldTransforms.size()) { fail("OUTPUT_WORLD_INDEX"); continue; }
                const hlslpp::float4x4 a = pd.worldTransforms[pt], b = cd.worldTransforms[t];
                const hlslpp::float4x4 o = output ? hlslpp::float4x4(cd.lerpWorldTransforms[t]) : b;
                for (int r = 0; r < 4; ++r) for (int col = 0; col < 4; ++col) {
                    const float av = float(a[r][col]), bv = float(b[r][col]), ov = float(o[r][col]);
                    finite &= std::isfinite(av) && std::isfinite(bv) && (!output || std::isfinite(ov));
                    endpoint = std::max(endpoint, std::fabs(av - bv));
                    if (output) {
                        outputPrev = std::max(outputPrev, std::fabs(ov - av)); outputCur = std::max(outputCur, std::fabs(ov - bv));
                    }
                }
                finite &= std::isfinite(endpoint) && std::isfinite(outputPrev) && std::isfinite(outputCur);
                if (finite && qualified && endpoint > 1.0e-6f) { ++changing; status = "MAPPED_CHANGING"; }
                if (finite && output && adjacentWorkloads && cc == 1 && pc == 1 && !cw.paused && endpoint > 1.0e-6f && curWeight > 0 && curWeight < 1 && outputPrev > 1.0e-6f && outputCur > 1.0e-6f) { ++interior; status = "CPU_INTERMEDIATE_OUTPUT"; }
            }
        }
        if (!finite) { ++failures; status = "NONFINITE"; }
        if (!qualified) ++unmapped;
        if (cc > 1 || pc > 1) ++multi;
        ++rows;
        std::fprintf(stderr, "FM_ROW call=%llu pair=%zu c=%u p=%u cid=%llu pid=%llu adj=%llu cPresent=%llu pPresent=%llu cSubmit=%llu pSubmit=%llu cPaused=%u pPaused=%u cMatched=%u pMatched=%u processed=%u uploaded=%u wc=%.9g wp=%.9g target=%u original=%u targetEmpty=%u id=%08x t=%u pt=%u cCount=%u pCount=%u cOrdinal=%u pOrdinal=%u cv=%u pv=%u cf=%u pf=%u finite=%u output=%u delta=%.9g outPrev=%.9g outCur=%.9g status=%s multiId=%u scene=UNPROVED\n",
            calls, pairs.size(), c, p, cw.workloadId, pw ? pw->workloadId : 0, adj, cw.presentId, pw ? pw->presentId : 0, cw.submissionFrame, pw ? pw->submissionFrame : 0, unsigned(cw.paused), unsigned(pw && pw->paused), unsigned(curFrame.matched), unsigned(prevFrame.matched), unsigned(processed), unsigned(uploaded), curWeight, prevWeight, targetRate, cw.viOriginalRate, unsigned(targetEmpty), g.matrixId, t, pt, cc, pc, cu.ordinal[t], po, cu.vertices[t], pv, cu.faces[t], pf, unsigned(finite), unsigned(output), endpoint, outputPrev, outputCur, status, unsigned(cc > 1 || pc > 1));
    }
    std::fflush(stderr);
    if (rows == 512 || pairs.size() == 120) { stopped = true; counters(); }
}

#else
// Family mode is diagnostic only. The caller owns sharedResources->workloadMutex.
static void observeHoneyFrameMatch(WorkloadQueue &queue, const GameFrame &curFrame, const GameFrame &prevFrame,
    bool processed, bool uploaded, float curWeight, float prevWeight, uint32_t targetRate, bool targetEmpty) {
    struct Receipt {
        uint64_t calls=0, rows=0, honeyRows=0, simpleRows=0, honeyPairs=0, simplePairs=0, guardFailures=0;
        std::vector<std::array<uint64_t,4>> pairs;
        bool simpleCap=false, failed=false;
#if BASELINE_PLAYER_BOUNDARY
        bool playerArmed=false;
        uint64_t prearmPairs=0;
        bool playerFailureSeen=false;
        std::array<uint64_t,4> lastPrearm{};
#endif
    };
    static Receipt s;
#if BASELINE_PLAYER_BOUNDARY
    static const bool paletteMode=baseline_palette_observation_mode();
    const uint64_t selected=paletteMode?baseline_palette_observed_blocks():0;
    const uint32_t selectedA=uint32_t(selected),selectedB=uint32_t(selected>>32);
    static uint64_t droppedRows=0;
#endif
    ++s.calls;
    auto ioFailure=[&]() {
        if (!s.failed) {
            s.failed=true;
            #if BASELINE_PLAYER_BOUNDARY
            std::fprintf(stdout,"FM_PLAYER_FAILURE call=%llu reason=IO_FAILURE status=INVALID\n",s.calls);
#else
            std::fprintf(stdout,"FM_SIMPLE_FAILURE call=%llu reason=IO_FAILURE status=INVALID\n",s.calls);
#endif
            std::fflush(stdout);
        }
    };
    auto diagnostic=[&](bool cap) {
        const int written=std::fprintf(stderr,
            cap ? "FM_SIMPLE_CAP call=%llu rows=%llu pairs=%llu sharedRows=%llu sharedPairs=%llu guardFailures=%llu status=INCOMPLETE\n" :
                  "FM_SIMPLE_COUNTER call=%llu rows=%llu pairs=%llu sharedRows=%llu sharedPairs=%llu guardFailures=%llu status=INCOMPLETE\n",
            s.calls,s.simpleRows,s.simplePairs,s.rows,static_cast<unsigned long long>(s.pairs.size()),s.guardFailures);
        if (std::fflush(stderr)!=0 || written<0 || std::ferror(stderr)) ioFailure();
    };
    if (s.failed) return;
    if (s.calls==1 || s.calls%1000==0) diagnostic(false);
    if (s.failed) return;

    // No SIMPLE workload/map/draw-data access precedes this short cursor snapshot.
    uint32_t guardedCurrent=UINT32_MAX,guardedPrevious=UINT32_MAX;
    bool simpleGuard=curFrame.workloads.size()==1 && prevFrame.workloads.size()==1 && queue.workloads.size()==4;
    if (simpleGuard) {
        guardedCurrent=curFrame.workloads[0];guardedPrevious=prevFrame.workloads[0];
        const std::lock_guard<std::mutex> cursorLock(queue.cursorMutex);
        simpleGuard=guardedPrevious<4 && guardedCurrent<4 && guardedPrevious==queue.barrierCursor &&
            guardedCurrent==(queue.barrierCursor+1)%4 && queue.writeCursor!=guardedCurrent && queue.writeCursor!=guardedPrevious;
    }
    if (!simpleGuard) ++s.guardFailures;

    auto honey=[](const TransformGroup &g) {
        return (g.matrixId&0xF0000000u)==0x90000000u && (g.matrixId&0x1FFu)==1 && g.decompose &&
            g.positionInterpolation==1 && g.rotationInterpolation==1 && g.scaleInterpolation==1 &&
            g.skewInterpolation==0 && g.perspectiveInterpolation==0 && g.vertexInterpolation==0 &&
            g.texcoordInterpolation==0 && g.tileInterpolation==0 && g.lookAtInterpolation==0 &&
            g.ordering==0 && g.aspectMode==0 && g.editable==0;
    };
    auto simple=[](const TransformGroup &g) {
#if BASELINE_PLAYER_BOUNDARY
        const uint32_t role=g.matrixId&511u;
        const bool fallback=role==129u;
        const bool leaf=role>=257u && role<=324u;
        return (g.matrixId&0xF0000000u)==0x90000000u && (role<=68u || fallback || leaf) && !g.decompose &&
            g.positionInterpolation==unsigned(!fallback) && g.rotationInterpolation==unsigned(!fallback) &&
            g.scaleInterpolation==unsigned(!fallback) && g.skewInterpolation==unsigned(!fallback) &&
            g.perspectiveInterpolation==unsigned(!fallback) && g.vertexInterpolation==G_EX_COMPONENT_SKIP &&
            g.texcoordInterpolation==G_EX_COMPONENT_SKIP && g.tileInterpolation==(fallback?0:2) &&
            g.lookAtInterpolation==(fallback?0:2) && g.ordering==0 && g.aspectMode==0 && g.editable==0;
#else
        const uint32_t role=g.matrixId&15u,phase=(g.matrixId>>4)&7u;
        return (g.matrixId&0xF0000000u)==0x90000000u && role>=1 && role<=9 &&
            (phase==0 || phase==1 || phase==2 || phase==5 || phase==6) && !g.decompose &&
            g.positionInterpolation==1 && g.rotationInterpolation==1 && g.scaleInterpolation==1 &&
            g.skewInterpolation==1 && g.perspectiveInterpolation==1 && g.vertexInterpolation==G_EX_COMPONENT_SKIP &&
            g.texcoordInterpolation==0 && g.tileInterpolation==2 && g.lookAtInterpolation==2 &&
            g.ordering==0 && g.aspectMode==0 && g.editable==0;
#endif
    };
#if BASELINE_PLAYER_BOUNDARY
    auto paletteRoot=[](const TransformGroup &root) {
        static_assert(TOOIE_BRIDGE_PALETTE_ROOT_POLICY==0x0201154Eu);
        return (root.matrixId&0xF000000Fu)==0x90000000u && root.decompose && root.positionInterpolation==1 &&
            root.rotationInterpolation==2 && root.scaleInterpolation==2 && root.skewInterpolation==2 && root.perspectiveInterpolation==2 &&
            root.vertexInterpolation==0 && root.texcoordInterpolation==0 && root.tileInterpolation==2 && root.lookAtInterpolation==2 &&
            root.ordering==0 && root.aspectMode==0 && root.editable==0;
    };
    auto palette=[](const TransformGroup &g) {
        const uint32_t role=g.matrixId&15u,phase=(g.matrixId>>4)&7u;
        static_assert(TOOIE_BRIDGE_PALETTE_LEAF_POLICY==0x02010AA9u);
        return (g.matrixId&0xF0000000u)==0x90000000u && (role==1 || role==2) &&
            (phase==0 || phase==1 || phase==2 || phase==5 || phase==6) && !g.decompose &&
            g.positionInterpolation==1 && g.rotationInterpolation==1 && g.scaleInterpolation==1 &&
            g.skewInterpolation==1 && g.perspectiveInterpolation==1 && g.vertexInterpolation==0 &&
            g.texcoordInterpolation==0 && g.tileInterpolation==2 && g.lookAtInterpolation==2 &&
            g.ordering==0 && g.aspectMode==0 && g.editable==0;
    };
#endif
    const bool interior=curWeight>0.f && curWeight<1.f;
    // A mixed pair makes one membership decision before either family can insert it.
    bool decisionReady=false,newPair=false,insertPair=false;
    std::array<uint64_t,4> decision{};
    auto observeFamily=[&](bool simpleMode) {
        uint64_t &familyRows=simpleMode?s.simpleRows:s.honeyRows;
        uint64_t &familyPairs=simpleMode?s.simplePairs:s.honeyPairs;
        const uint64_t rowLimit=simpleMode?128:384,pairLimit=simpleMode?16:104;
        if (s.failed) return;
#if BASELINE_PLAYER_BOUNDARY
        if (!simpleMode && (familyRows>=rowLimit || familyPairs>=pairLimit || s.rows>=512)) return;
#else
        if (familyRows>=rowLimit || familyPairs>=pairLimit || s.rows>=512) return;
#endif
        if (simpleMode && (!simpleGuard || !interior)) return;
        if (curFrame.workloads.size()!=1 || prevFrame.workloads.size()>1) return;
        const uint32_t c=curFrame.workloads[0];
        if (c>=queue.workloads.size()) return;
        const auto &cw=queue.workloads[c];const auto &cd=cw.drawData;
        const auto *wm=c<curFrame.frameMap.workloads.size()?&curFrame.frameMap.workloads[c]:nullptr;
        const bool ownerMapped=wm && wm->mapped;
        const uint32_t p=ownerMapped?wm->prevWorkloadIndex:(prevFrame.workloads.empty()?UINT32_MAX:prevFrame.workloads[0]);
        if (simpleMode && (p!=guardedPrevious || c!=guardedCurrent || cw.paused || cw.debuggerCamera.enabled)) {++s.guardFailures;return;}
        if (p!=UINT32_MAX && (p>=queue.workloads.size() || std::find(prevFrame.workloads.begin(),prevFrame.workloads.end(),p)==prevFrame.workloads.end())) return;
        if (ownerMapped && p==UINT32_MAX) return;
        const Workload *pw=p!=UINT32_MAX?&queue.workloads[p]:nullptr;
        if (simpleMode && (!pw || pw->paused || pw->debuggerCamera.enabled)) {++s.guardFailures;return;}
        if (pw && (p==c || pw->workloadId==cw.workloadId)) return;
        const uint64_t adj=pw?cw.workloadId-pw->workloadId:UINT64_MAX;
        const bool adjacent=pw && pw->workloadId!=UINT64_MAX && adj==1;
#if BASELINE_PLAYER_BOUNDARY
        auto playerFailure=[&](const char *reason,const char *status="INVALID") {
            if(s.playerFailureSeen) return;
            const int written=std::fprintf(stderr,"FM_PLAYER_FAILURE call=%llu reason=%s status=%s\n",s.calls,reason,status);
            if(written<0||std::fflush(stderr)!=0||std::ferror(stderr)) ioFailure();
            else s.playerFailureSeen=true;
        };
#endif
        struct Uses {std::vector<uint32_t> vertices,faces,ordinal;std::unordered_map<uint32_t,uint32_t> count;};
        auto scan=[](const DrawData &d,Uses &u) {
            const size_t n=d.worldTransforms.size();
            if (n>65536 || d.worldTransformGroups.size()!=n || d.transformGroups.size()>65536 || d.worldIndices.size()>65536 || d.faceIndices.size()>65536) return 0;
            u.vertices.assign(n,0);u.faces.assign(n,0);u.ordinal.assign(n,0);
            for (size_t t=0;t<n;++t) {
                if (d.worldTransformGroups[t]>=d.transformGroups.size()) return 0;
                const auto &g=d.transformGroups[d.worldTransformGroups[t]];
                if (g.ordering==G_EX_ORDER_LINEAR && g.matrixId!=G_EX_ID_AUTO && g.matrixId!=G_EX_ID_IGNORE) u.ordinal[t]=u.count[g.matrixId]++;
            }
            for (uint32_t t:d.worldIndices) {if(t>=n)return 0;++u.vertices[t];}
            for (uint32_t v:d.faceIndices) {if(v>=d.worldIndices.size())return 1;++u.faces[d.worldIndices[v]];}
            return 2;
        };
        Uses cu,pu;
        const int currentScan=scan(cd,cu),previousScan=pw?scan(pw->drawData,pu):2;
        if (!currentScan || !previousScan) {
#if BASELINE_PLAYER_BOUNDARY
            playerFailure("INPUT_BOUND_OR_INDEX");
#endif
            return;
        }
#if BASELINE_PLAYER_BOUNDARY
        const uint32_t playerBlock=baseline_player_observed_idbase();
        auto paletteWithRoot=[&](const TransformGroup &g) {
            if(!palette(g) || cd.viewProjTransforms.size()>65536 || cd.viewProjTransformGroups.size()!=cd.viewProjTransforms.size()) return false;
            for(uint32_t index:cd.viewProjTransformGroups) if(index<cd.transformGroups.size()) {
                const auto& root=cd.transformGroups[index];
                if(root.matrixId==(g.matrixId&~15u) && paletteRoot(root)) return true;
            }
            return false;
        };
#endif
        auto policy=[&](const TransformGroup &g) {
#if BASELINE_PLAYER_BOUNDARY
            if(simpleMode) {
                if(playerBlock && (g.matrixId&0xFFFFFE00u)==playerBlock) return simple(g);
                return paletteMode && paletteWithRoot(g);
            }
#endif
            return simpleMode?simple(g):honey(g);
        };
#if BASELINE_PLAYER_BOUNDARY
        auto leafRole=[](uint32_t id) {const uint32_t role=id&511u;return role>=257u&&role<=324u;};
#endif
        bool bearing=false;
#if BASELINE_PLAYER_BOUNDARY
        bool paletteBearing=false;
        auto playerPolicy=[&](const TransformGroup &g){return playerBlock && (g.matrixId&0xFFFFFE00u)==playerBlock && simple(g);};
        if(paletteMode && simpleMode) for(size_t t=0;t<cd.worldTransforms.size();++t)
            paletteBearing|=policy(cd.transformGroups[cd.worldTransformGroups[t]]) &&
                !playerPolicy(cd.transformGroups[cd.worldTransformGroups[t]]) && cu.vertices[t]!=0;
        if(paletteMode && simpleMode && s.prearmPairs<4) {
            uint32_t rootless=0,orphanRoots=0,example=0;
            for(size_t t=0;t<cd.worldTransforms.size();++t) {
                const auto& g=cd.transformGroups[cd.worldTransformGroups[t]];
                if(cu.vertices[t] && palette(g) && !playerPolicy(g) && !paletteWithRoot(g)) {++rootless;if(!example)example=g.matrixId;}
            }
            if(cd.viewProjTransforms.size()<=65536 && cd.viewProjTransformGroups.size()==cd.viewProjTransforms.size())
                for(uint32_t index:cd.viewProjTransformGroups) if(index<cd.transformGroups.size()) {
                    const auto& root=cd.transformGroups[index];if(!paletteRoot(root)) continue;
                    bool world=false;
                    for(size_t t=0;t<cd.worldTransforms.size();++t) {
                        const auto& g=cd.transformGroups[cd.worldTransformGroups[t]];
                        world|=cu.vertices[t] && palette(g) && !playerPolicy(g) && root.matrixId==(g.matrixId&~15u);
                    }
                    if(!world) {++orphanRoots;if(!example)example=root.matrixId;}
                }
            const std::array<uint64_t,4> observed={c,cw.workloadId,p,pw?pw->workloadId:UINT64_MAX};
            if((rootless || orphanRoots) && observed!=s.lastPrearm) {
                const int written=std::fprintf(stderr,"FM_PALETTE_OTHER seq=%llu cid=%llu pid=%llu rootlessWorlds=%u rootsWithoutWorlds=%u example=%08x sampled=1 pairLimit=4\n",
                    s.prearmPairs+1,cw.workloadId,pw?pw->workloadId:0,rootless,orphanRoots,example);
                if(written<0 || std::fflush(stderr)!=0 || std::ferror(stderr)){ioFailure();return;}
                ++s.prearmPairs;s.lastPrearm=observed;
            }
        }
        if(paletteMode && simpleMode && (!playerBlock || !paletteBearing)) return;
#endif
        for (size_t t=0;t<cd.worldTransforms.size();++t) bearing|=policy(cd.transformGroups[cd.worldTransformGroups[t]]) && cu.vertices[t]!=0;
#if !BASELINE_PLAYER_BOUNDARY
        if (!bearing) return;
#endif
#if BASELINE_PLAYER_BOUNDARY
        struct VertexWitness { uint32_t count=0,changed=0; float delta=0,maximum=0,error=0; bool velEqual=false,tcEqual=false; };
        if(simpleMode) {
            if(s.playerFailureSeen) return;
            if(currentScan!=2 || previousScan!=2) {playerFailure("INPUT_BOUND_OR_INDEX");return;}
            auto endpoints=[&](const DrawData &d,const Uses &uses)->const char* {
                std::unordered_map<uint64_t,size_t> first;
                for(size_t t=0;t<d.worldTransforms.size();++t) {
                    const auto &g=d.transformGroups[d.worldTransformGroups[t]];
                    if(playerBlock&&(g.matrixId&0xFFFFFE00u)==playerBlock) {
                        const uint32_t role=g.matrixId&511u;
                        if(!(role<=68u||role==129u||leafRole(g.matrixId))) return "BONE_ROLE_RANGE";
                    }
                    if(!playerPolicy(g)) continue;
                    const bool leaf=leafRole(g.matrixId);
                    if(leaf && (uses.count.at(g.matrixId)&1u)) return "LEAF_ODD_WORLD_COUNT";
                    const uint64_t key=(uint64_t(g.matrixId)<<1)|(leaf?(uses.ordinal[t]&1u):0u);
                    const auto [entry,inserted]=first.emplace(key,t);
                    for(int r=0;r<4;++r)for(int col=0;col<4;++col) {
                        const float value=float(d.worldTransforms[t][r][col]);
                        if(!std::isfinite(value)) return "NONFINITE_ENDPOINT";
                        if((g.matrixId&511u)!=129u && !inserted &&
                           value!=float(d.worldTransforms[entry->second][r][col])) return leaf?"LEAF_DUPLICATE_ENDPOINT":"BONE_DUPLICATE_ENDPOINT";
                    }
                }
                return nullptr;
            };
            if(const char *reason=endpoints(cd,cu)) {playerFailure(reason);return;}
            if(const char *reason=endpoints(pw->drawData,pu)) {playerFailure(reason);return;}
            for(size_t t=0;t<cd.worldTransforms.size();++t) {
                const auto &g=cd.transformGroups[cd.worldTransformGroups[t]];
                if(!playerPolicy(g)) continue;
                if(processed&&uploaded) {
                    if(t>=cd.lerpWorldTransforms.size()) {playerFailure("OUTPUT_WORLD_INDEX");return;}
                    for(int r=0;r<4;++r)for(int col=0;col<4;++col) {
                        const float output=float(cd.lerpWorldTransforms[t][r][col]);
                        if(!std::isfinite(output)) {playerFailure("NONFINITE_OUTPUT");return;}
                        if((g.matrixId&511u)==129u && output!=float(cd.worldTransforms[t][r][col])) {
                            playerFailure("FALLBACK_OUTPUT_CHANGED");return;
                        }
                    }
                }
                if(!ownerMapped) continue;
                if(t>=wm->transforms.size()) {playerFailure("TRANSFORM_MAP_INDEX");return;}
                const auto &tm=wm->transforms[t];
                if(!tm.mapped) continue;
                const uint32_t pt=tm.prevTransformIndex;
                const auto &pd=pw->drawData;
                if(pt>=pd.worldTransforms.size()) {playerFailure("PREVIOUS_WORLD_INDEX");return;}
                for(int r=0;r<4;++r)for(int col=0;col<4;++col)
                    if(!std::isfinite(float(pd.worldTransforms[pt][r][col]))) {playerFailure("NONFINITE_ENDPOINT");return;}
                const auto &pg=pd.transformGroups[pd.worldTransformGroups[pt]];
                if(!playerPolicy(pg)||pg.matrixId!=g.matrixId) {playerFailure("BONE_MAPPED_ID_ROLE");return;}
                if(leafRole(g.matrixId) && ((cu.ordinal[t]^pu.ordinal[pt])&1u)) {
                    playerFailure("LEAF_MAPPED_PARITY");return;
                }

            }
        }
        if(paletteMode && simpleMode) {
            bool hasA=false,hasB=false,hasPlayer=false;
            for(size_t t=0;t<cd.worldTransforms.size();++t) {
                const auto& g=cd.transformGroups[cd.worldTransformGroups[t]];
                if(!cu.vertices[t] || !policy(g)) continue;
                if(playerPolicy(g)) {hasPlayer=true;continue;}
                const uint32_t block=g.matrixId&0xFFFFFE00u;
                hasA|=selectedA && block==selectedA;hasB|=selectedB && block==selectedB;
            }
            if(!interior || !hasPlayer || !hasA || (familyPairs && !hasB)) return;
        }
        if(simpleMode && (familyRows>=rowLimit || familyPairs>=pairLimit || s.rows>=512)) return;
#endif
        if (!bearing) return;
        const std::array<uint64_t,4> pair={c,cw.workloadId,p,pw?pw->workloadId:UINT64_MAX};
        if (!decisionReady) {decision=pair;newPair=std::find(s.pairs.begin(),s.pairs.end(),pair)==s.pairs.end();decisionReady=true;}
        if (pair!=decision) return;
        if (simpleMode && !newPair) return;
#if BASELINE_PLAYER_BOUNDARY
        if(simpleMode && !paletteMode && !s.playerArmed) {
            if(pair!=s.lastPrearm) { ++s.prearmPairs; s.lastPrearm=pair; }
            bool changed=false;
            if(ownerMapped && adjacent && currentScan==2 && previousScan==2) {
                for(size_t t=0;t<cd.worldTransforms.size();++t) {
                    const auto &g=cd.transformGroups[cd.worldTransformGroups[t]];
                    if(!policy(g) || !cu.vertices[t] || t>=wm->transforms.size()) continue;
                    const auto &tm=wm->transforms[t];
                    if(!tm.mapped || tm.prevTransformIndex>=pw->drawData.worldTransforms.size()) continue;
                    const uint32_t pt=tm.prevTransformIndex;
                    const auto &pg=pw->drawData.transformGroups[pw->drawData.worldTransformGroups[pt]];
                    if(!policy(pg) || pg.matrixId!=g.matrixId || !pu.vertices[pt] ||
                        ((g.matrixId&511u)==129u||(g.matrixId&511u)==0u)) continue;
                    bool finite=true;float delta=0;
                    for(int r=0;r<4;++r)for(int col=0;col<4;++col) {
                        const float a=float(pw->drawData.worldTransforms[pt][r][col]);
                        const float b=float(cd.worldTransforms[t][r][col]);
                        finite&=std::isfinite(a)&&std::isfinite(b);delta=std::max(delta,std::fabs(a-b));
                    }
                    changed|=finite&&delta>1.e-6f;
                }
            }
            if(!changed) return;
            s.playerArmed=true;
            const int written=std::fprintf(stderr,"FM_PLAYER_ARM call=%llu prearmPairs=%llu cid=%llu pid=%llu idbase=%08x\n",
                s.calls,s.prearmPairs,cw.workloadId,pw->workloadId,playerBlock);
            if(written<0||std::fflush(stderr)!=0||std::ferror(stderr)){ioFailure();return;}
        }
#endif
        if (interior && newPair) {
            if (s.pairs.size()>=120) return;
            ++familyPairs;insertPair=true;
        }
        auto fail=[&](const char *reason) {
#if BASELINE_PLAYER_BOUNDARY
            if(simpleMode) {playerFailure(reason);return;}
#endif
            if (familyRows>=rowLimit || s.rows>=512) return;
            const int written=std::fprintf(stderr,simpleMode?
                "FM_SIMPLE_FAILURE call=%llu reason=%s status=INVALID\n":"FM_FAILURE call=%llu reason=%s\n",s.calls,reason);
            if (std::fflush(stderr)!=0 || written<0 || std::ferror(stderr)) {ioFailure();return;}
            ++familyRows;++s.rows;
        };
        if(currentScan==1 || previousScan==1){fail("INPUT_BOUND_OR_INDEX");return;}
#if BASELINE_PLAYER_BOUNDARY
        struct Sample {std::array<uint32_t,5> seen{},kept{};};
        std::unordered_map<uint32_t,Sample> samples;
#endif
        const bool collectCategories=
#if BASELINE_PLAYER_BOUNDARY
            paletteMode && simpleMode;
#else
            false;
#endif
        for (uint32_t t=0;t<cd.worldTransforms.size() && !s.failed && (collectCategories || (familyRows<rowLimit && s.rows<512));++t) {
            const auto &g=cd.transformGroups[cd.worldTransformGroups[t]];
            if (!policy(g) || !cu.vertices[t]) continue;
#if BASELINE_PLAYER_BOUNDARY
            const bool paletteRow=simpleMode && paletteMode && !playerPolicy(g);
            if(paletteRow && (g.matrixId&0xFFFFFE00u)!=selectedA && (g.matrixId&0xFFFFFE00u)!=selectedB) continue;
#endif
            const uint32_t cc=cu.count[g.matrixId];
            const char *status=wm?"UNMAPPED_OWNER":"NO_OWNER_MAP";
            uint32_t pt=UINT32_MAX,pc=pw?pu.count[g.matrixId]:0,po=UINT32_MAX,pv=0,pf=0;
            float endpoint=0,outputPrev=0,outputCur=0;
            bool finite=std::isfinite(curWeight)&&std::isfinite(prevWeight),qualified=false,output=false;
#if BASELINE_PLAYER_BOUNDARY
            float rawMatrices[48]{};bool rawAvailable=false;
            VertexWitness vertex;const char *topologyStatus="UNAVAILABLE";
#endif
            const hlslpp::float4x4 current=cd.worldTransforms[t];
            for(int r=0;r<4;++r)for(int col=0;col<4;++col)finite&=std::isfinite(float(current[r][col]));
            if (ownerMapped) {
                if(t>=wm->transforms.size()){fail("TRANSFORM_MAP_INDEX");continue;}
                const auto &tm=wm->transforms[t];status="UNMAPPED_WORLD";
                if(tm.mapped){
                    pt=tm.prevTransformIndex;const auto &pd=pw->drawData;
                    if(pt>=pd.worldTransforms.size()){fail("PREVIOUS_WORLD_INDEX");continue;}
                    const auto &pg=pd.transformGroups[pd.worldTransformGroups[pt]];
                    pc=pu.count[g.matrixId];po=pu.ordinal[pt];pv=pu.vertices[pt];pf=pu.faces[pt];
                    qualified=policy(pg)&&pg.matrixId==g.matrixId&&pv;
#if BASELINE_PLAYER_BOUNDARY
                    const bool fallback=simpleMode&&!paletteRow&&(g.matrixId&511u)==129u;
                    const bool diagnosticRole=simpleMode&&!paletteRow&&(g.matrixId&511u)==0u;
                    if(fallback||diagnosticRole) qualified=false;
#endif
                    status=qualified?"MAPPED_STATIC":"PREVIOUS_POLICY_ID_VERTEX_MISMATCH";
                    output=qualified&&processed&&uploaded;
#if BASELINE_PLAYER_BOUNDARY
                    if((fallback||diagnosticRole)&&policy(pg)&&pg.matrixId==g.matrixId) {status="MAPPED_STATIC";output=processed&&uploaded;}
#endif
                    if(output&&t>=cd.lerpWorldTransforms.size()){fail("OUTPUT_WORLD_INDEX");continue;}
                    const hlslpp::float4x4 a=pd.worldTransforms[pt],b=cd.worldTransforms[t];
                    const hlslpp::float4x4 result=output?hlslpp::float4x4(cd.lerpWorldTransforms[t]):b;
                    for(int r=0;r<4;++r)for(int col=0;col<4;++col){
                        const float av=float(a[r][col]),bv=float(b[r][col]),ov=float(result[r][col]);
                        finite&=std::isfinite(av)&&std::isfinite(bv)&&(!output||std::isfinite(ov));endpoint=std::max(endpoint,std::fabs(av-bv));
#if BASELINE_PLAYER_BOUNDARY
                        rawMatrices[r*4+col]=av;rawMatrices[16+r*4+col]=bv;rawMatrices[32+r*4+col]=ov;
#endif
                        if(output){outputPrev=std::max(outputPrev,std::fabs(ov-av));outputCur=std::max(outputCur,std::fabs(ov-bv));}
                    }
                    finite&=std::isfinite(endpoint)&&std::isfinite(outputPrev)&&std::isfinite(outputCur);
                    if(finite&&qualified&&endpoint>1.e-6f)status="MAPPED_CHANGING";
#if BASELINE_PLAYER_BOUNDARY
                    rawAvailable=finite&&output;
                    const bool stream=simpleMode ? cc>0&&pc>0&&curFrame.matched&&prevFrame.matched&&!fallback&&!diagnosticRole : cc==1&&pc==1;
#else
                    const bool stream=cc==1&&pc==1;
#endif
                    if(finite&&output&&adjacent&&stream&&!cw.paused&&endpoint>1.e-6f&&interior&&outputPrev>1.e-6f&&outputCur>1.e-6f)status="CPU_INTERMEDIATE_OUTPUT";
                }
            }
            if(!finite)status="NONFINITE";
#if BASELINE_PLAYER_BOUNDARY
            if(paletteMode && simpleMode) {
                const bool mappingMiss=std::strcmp(status,"UNMAPPED_OWNER")==0 || std::strcmp(status,"NO_OWNER_MAP")==0 || std::strcmp(status,"UNMAPPED_WORLD")==0;
                const unsigned category=!finite?4:qualified?(endpoint>1.e-6f?0:1):mappingMiss?2:3;
                auto& sample=samples[g.matrixId&0xFFFFFE00u];++sample.seen[category];
                if(sample.kept[category]>=1 || familyRows>=rowLimit || s.rows>=512) {++droppedRows;continue;}
                ++sample.kept[category];
            } else
#endif
            if(familyRows>=rowLimit || s.rows>=512) break;
#if BASELINE_PLAYER_BOUNDARY
            const char *playerFormat=rawAvailable ?
                "FM_PLAYER_ROW call=%llu pair=%zu c=%u p=%u cid=%llu pid=%llu adj=%llu cPresent=%llu pPresent=%llu cSubmit=%llu pSubmit=%llu cPaused=%u pPaused=%u cMatched=%u pMatched=%u processed=%u uploaded=%u wc=%.9g wp=%.9g target=%u original=%u targetEmpty=%u id=%08x t=%u pt=%u cCount=%u pCount=%u cOrdinal=%u pOrdinal=%u cv=%u pv=%u cf=%u pf=%u finite=%u output=%u delta=%.9g outPrev=%.9g outCur=%.9g status=%s multiId=%u scene=UNPROVED role=%u view=%u phase=%u family=%s prev=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g cur=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g out=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g vCount=%u vChanged=%u vDelta=%.9g vMax=%.9g vErr=%.9g velEqual=%u tcEqual=%u topology=%s\n" :
                "FM_PLAYER_ROW call=%llu pair=%zu c=%u p=%u cid=%llu pid=%llu adj=%llu cPresent=%llu pPresent=%llu cSubmit=%llu pSubmit=%llu cPaused=%u pPaused=%u cMatched=%u pMatched=%u processed=%u uploaded=%u wc=%.9g wp=%.9g target=%u original=%u targetEmpty=%u id=%08x t=%u pt=%u cCount=%u pCount=%u cOrdinal=%u pOrdinal=%u cv=%u pv=%u cf=%u pf=%u finite=%u output=%u delta=%.9g outPrev=%.9g outCur=%.9g status=%s multiId=%u scene=UNPROVED role=%u view=%u phase=%u family=%s prev=NA cur=NA out=NA vCount=%u vChanged=%u vDelta=%.9g vMax=%.9g vErr=%.9g velEqual=%u tcEqual=%u topology=%s\n";
#endif
            const char *format=simpleMode?
#if BASELINE_PLAYER_BOUNDARY
                playerFormat:
#else
                "FM_SIMPLE_ROW call=%llu pair=%zu c=%u p=%u cid=%llu pid=%llu adj=%llu cPresent=%llu pPresent=%llu cSubmit=%llu pSubmit=%llu cPaused=%u pPaused=%u cMatched=%u pMatched=%u processed=%u uploaded=%u wc=%.9g wp=%.9g target=%u original=%u targetEmpty=%u id=%08x t=%u pt=%u cCount=%u pCount=%u cOrdinal=%u pOrdinal=%u cv=%u pv=%u cf=%u pf=%u finite=%u output=%u delta=%.9g outPrev=%.9g outCur=%.9g status=%s multiId=%u scene=UNPROVED role=%u view=%u phase=%u family=POLICY_ONLY\n":
#endif
                "FM_ROW call=%llu pair=%zu c=%u p=%u cid=%llu pid=%llu adj=%llu cPresent=%llu pPresent=%llu cSubmit=%llu pSubmit=%llu cPaused=%u pPaused=%u cMatched=%u pMatched=%u processed=%u uploaded=%u wc=%.9g wp=%.9g target=%u original=%u targetEmpty=%u id=%08x t=%u pt=%u cCount=%u pCount=%u cOrdinal=%u pOrdinal=%u cv=%u pv=%u cf=%u pf=%u finite=%u output=%u delta=%.9g outPrev=%.9g outCur=%.9g status=%s multiId=%u scene=UNPROVED\n";
            const size_t pairNumber=s.pairs.size()+(insertPair?1:0);
#if BASELINE_PLAYER_BOUNDARY
            auto emit=[&](auto... witness) {
                return std::fprintf(stderr,format,s.calls,pairNumber
#else
            const int written=std::fprintf(stderr,format,s.calls,pairNumber
#endif
                ,c,p,cw.workloadId,pw?pw->workloadId:0,adj,cw.presentId,pw?pw->presentId:0,cw.submissionFrame,pw?pw->submissionFrame:0,
                unsigned(cw.paused),unsigned(pw&&pw->paused),unsigned(curFrame.matched),unsigned(prevFrame.matched),unsigned(processed),unsigned(uploaded),curWeight,prevWeight,targetRate,cw.viOriginalRate,unsigned(targetEmpty),g.matrixId,t,pt,cc,pc,cu.ordinal[t],po,cu.vertices[t],pv,cu.faces[t],pf,unsigned(finite),unsigned(output),endpoint,outputPrev,outputCur,status,unsigned(cc>1||pc>1)
#if BASELINE_PLAYER_BOUNDARY
                ,simpleMode&&!paletteRow?(g.matrixId&511u):(g.matrixId&15u),simpleMode&&!paletteRow?UINT32_MAX:((g.matrixId>>7)&3u),simpleMode&&!paletteRow?UINT32_MAX:((g.matrixId>>4)&7u)
                ,paletteRow?"ACTOR_PALETTE":leafRole(g.matrixId)?"PLAYER_LEAF_BODY":"PLAYER_BONE_BODY"
#else
                ,g.matrixId&15u,(g.matrixId>>7)&3u,(g.matrixId>>4)&7u
#endif
#if BASELINE_PLAYER_BOUNDARY
                ,witness...);
            };
            const int written=rawAvailable ? emit(rawMatrices[0],rawMatrices[1],rawMatrices[2],rawMatrices[3],rawMatrices[4],rawMatrices[5],rawMatrices[6],rawMatrices[7],rawMatrices[8],rawMatrices[9],rawMatrices[10],rawMatrices[11],rawMatrices[12],rawMatrices[13],rawMatrices[14],rawMatrices[15],rawMatrices[16],rawMatrices[17],rawMatrices[18],rawMatrices[19],rawMatrices[20],rawMatrices[21],rawMatrices[22],rawMatrices[23],rawMatrices[24],rawMatrices[25],rawMatrices[26],rawMatrices[27],rawMatrices[28],rawMatrices[29],rawMatrices[30],rawMatrices[31],rawMatrices[32],rawMatrices[33],rawMatrices[34],rawMatrices[35],rawMatrices[36],rawMatrices[37],rawMatrices[38],rawMatrices[39],rawMatrices[40],rawMatrices[41],rawMatrices[42],rawMatrices[43],rawMatrices[44],rawMatrices[45],rawMatrices[46],rawMatrices[47],vertex.count,vertex.changed,vertex.delta,vertex.maximum,vertex.error,unsigned(vertex.velEqual),unsigned(vertex.tcEqual),topologyStatus) :
                emit(vertex.count,vertex.changed,vertex.delta,vertex.maximum,vertex.error,unsigned(vertex.velEqual),unsigned(vertex.tcEqual),topologyStatus);
#else
                );
#endif
            if(std::fflush(stderr)!=0||written<0||std::ferror(stderr)){ioFailure();return;}
            ++familyRows;++s.rows;
        }
#if BASELINE_PLAYER_BOUNDARY
        if(paletteMode && simpleMode) for(const auto& entry:samples) {
            const uint32_t block=entry.first;unsigned roots=0,mappedRoots=0;
            if(block!=playerBlock && cd.viewProjTransforms.size()<=65536 && cd.viewProjTransformGroups.size()==cd.viewProjTransforms.size())
                for(size_t i=0;i<cd.viewProjTransforms.size();++i) {
                    if(cd.viewProjTransformGroups[i]>=cd.transformGroups.size()) continue;
                    const auto& root=cd.transformGroups[cd.viewProjTransformGroups[i]];
                    if((root.matrixId&0xFFFFFE00u)!=block || !paletteRoot(root)) continue;
                    ++roots;mappedRoots+=ownerMapped && i<wm->viewProjections.size() && wm->viewProjections[i].mapped;
                }
            const auto& count=entry.second;
            const int written=std::fprintf(stderr,"FM_PALETTE_BLOCK cid=%llu pid=%llu block=%08x player=%u finiteChanging=%u finiteStatic=%u unmapped=%u ineligible=%u nonfinite=%u kept=%u rootSeen=%u rootMapped=%u droppedRows=%llu guardFailures=%llu sampled=1 uploaded=%u\n",
                cw.workloadId,pw?pw->workloadId:0,block,unsigned(block==playerBlock),count.seen[0],count.seen[1],count.seen[2],count.seen[3],count.seen[4],count.kept[0]+count.kept[1]+count.kept[2]+count.kept[3]+count.kept[4],roots,mappedRoots,droppedRows,s.guardFailures,unsigned(uploaded));
            if(written<0 || std::fflush(stderr)!=0 || std::ferror(stderr)){ioFailure();return;}
        }
#endif
    };
    observeFamily(false);
    observeFamily(true);
    if(insertPair)s.pairs.push_back(decision);
    if(!s.failed&&!s.simpleCap&&(s.simpleRows>=128||s.simplePairs>=16)){s.simpleCap=true;diagnostic(true);}
}

#endif
