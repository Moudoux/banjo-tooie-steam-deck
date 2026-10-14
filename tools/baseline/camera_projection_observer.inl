// Included inside namespace RT64. Standard headers must be included globally.
// Internal linkage: fixture and queue each own a separate scalar measurement record.
namespace {
struct ShadowMaskSample {uint64_t workload=0;uint32_t pair=0,address=0;bool valid=false,done=false;};
static ShadowMaskSample shadowMaskSample;
static void observeShadowComposite(const RT64::Workload& w) {
 static unsigned rows=0;static uint64_t lastWorkload=UINT64_MAX;
 if(rows>=32 || !shadow_witness::armed.load(std::memory_order_acquire) || w.workloadId==lastWorkload)return;
 lastWorkload=w.workloadId;if(w.fbPairCount>w.fbPairs.size())return;
 uint32_t loadCursor=0;RT64::LoadTexture source{};
 for(uint32_t f=0;f<w.fbPairCount;f++){
  const auto& pair=w.fbPairs[f];if(pair.projectionCount>pair.projections.size())return;
  for(uint32_t p=0;p<pair.projectionCount;p++)if(pair.projections[p].gameCallCount>pair.projections[p].gameCalls.size())return;
  for(uint32_t p=0;p<pair.projectionCount;p++)for(uint32_t c=0;c<pair.projections[p].gameCallCount;c++){
   const auto& call=pair.projections[p].gameCalls[c].callDesc;
   const uint64_t end=uint64_t(call.loadIndex)+call.loadCount;
   if(end>w.drawData.loadOperations.size())return;
   for(;loadCursor<end;loadCursor++){const auto& op=w.drawData.loadOperations[loadCursor];if(op.type!=RT64::LoadOperation::Type::TLUT)source=op.texture;}
   if(call.tileCount==0 || call.tileIndex>=w.drawData.callTiles.size() || source.fmt!=2 || source.siz!=1 || source.width!=304)continue;
   uint32_t maskIndex=UINT32_MAX;
   for(uint32_t m=0;m<f;m++){const auto& image=w.fbPairs[m].colorImage;if(image.address==source.address&&image.fmt==2&&image.siz==1&&image.width==304)maskIndex=m;}
   if(maskIndex==UINT32_MAX || call.rectDsdx!=1024 || call.rectDtdy!=1024)continue;
   const auto& tile=w.drawData.callTiles[call.tileIndex];const auto& mask=w.fbPairs[maskIndex];
   if(mask.projectionCount>mask.projections.size())return;
   for(uint32_t mp=0;mp<mask.projectionCount;mp++)if(mask.projections[mp].gameCallCount>mask.projections[mp].gameCalls.size())return;
   if(!shadowMaskSample.valid)shadowMaskSample={w.workloadId,maskIndex,source.address,true,false};
   // Output is only a candidate. Offline ordered producer/source/strip join is mandatory.
   std::fprintf(stderr,"baseline shadow-composite row=%u workload=%llu pair=%u projection=%u call=%u uid=%u source=%08x mask_pair=%u destination=%08x tlut=%u rawTMEM=%u tileCopy=%u reinterpret=%u,%u,%u rect=%d,%d,%d,%d scissor=%d,%d,%d,%d other=%08x,%08x combine=%08x,%08x alpha=%a,%a\n",++rows,(unsigned long long)w.workloadId,f,p,call.callIndex,call.uid,source.address,maskIndex,pair.colorImage.address,tile.tlut,unsigned(tile.rawTMEM),unsigned(tile.tileCopyUsed),unsigned(tile.reinterpretTile),unsigned(tile.reinterpretFmt),unsigned(tile.reinterpretSiz),call.rect.ulx,call.rect.uly,call.rect.lrx,call.rect.lry,call.scissorRect.ulx,call.scissorRect.uly,call.scissorRect.lrx,call.scissorRect.lry,call.otherMode.H,call.otherMode.L,call.colorCombiner.H,call.colorCombiner.L,double(call.rdpParams.blendColor.w),double(call.rdpParams.primColor.w));
   // Record the actual clear, not a claim based only on target dimensions.
   for(uint32_t mp=0;mp<mask.projectionCount;mp++)for(uint32_t mc=0;mc<mask.projections[mp].gameCallCount;mc++){
    const auto& clear=mask.projections[mp].gameCalls[mc].callDesc;
    if(clear.otherMode.cycleType()!=G_CYC_FILL)continue;
    std::fprintf(stderr,"baseline shadow-clear composite_row=%u mask=%08x fill=%08x rect=%d,%d,%d,%d scissor=%d,%d,%d,%d\n",rows,mask.colorImage.address,clear.fillColor,clear.rect.ulx,clear.rect.uly,clear.rect.lrx,clear.rect.lry,clear.scissorRect.ulx,clear.scissorRect.uly,clear.scissorRect.lrx,clear.scissorRect.lry);
   }
   std::fflush(stderr);if(rows>=32)return;
  }
 }
}

struct CameraCpuObservationRecord {
    uint64_t calls = 0, wrongOwnerCalls = 0, guardCalls = 0, changes = 0;
    uint64_t windows = 0, attempts = 0, rows = 0, skippedSameWindow = 0;
    uint64_t positive = 0, incomplete = 0, invalid = 0, nonfinite = 0;
    std::array<uint64_t, 4> last = {};
    unsigned phase = 0, inWindow = 0;
    bool haveLast = false, selected = false, stopped = false;
    bool ioFailed = false, ioWriteFailed = false, ioFlushFailed = false, ioDiagnosticFailed = false;
    unsigned lastAliases = 0, lastProcessedAliases = 0, lastScalarVisits = 0;
    uint32_t lastTransform = 0;
    const char *lastStatus = "UNARMED"; // Fixture inspection; no exported API.
};
static CameraCpuObservationRecord cameraCpuObservation;

static bool cameraCpuSamePolicy(const TransformGroup &a, const TransformGroup &b) {
    return a.matrixId == b.matrixId && a.decompose == b.decompose &&
        a.positionInterpolation == b.positionInterpolation && a.rotationInterpolation == b.rotationInterpolation &&
        a.scaleInterpolation == b.scaleInterpolation && a.skewInterpolation == b.skewInterpolation &&
        a.perspectiveInterpolation == b.perspectiveInterpolation && a.vertexInterpolation == b.vertexInterpolation &&
        a.texcoordInterpolation == b.texcoordInterpolation && a.tileInterpolation == b.tileInterpolation &&
        a.lookAtInterpolation == b.lookAtInterpolation && a.ordering == b.ordering &&
        a.aspectMode == b.aspectMode && a.editable == b.editable;
}

static void observeShadowMaskMapping(const Workload& cw,const Workload& pw,
 const GameFrameMap::WorkloadMap* wm,uint32_t previousSlot,bool processed,float weight,float previousWeight){
 auto& s=shadowMaskSample;
 if(!s.valid || s.done || cw.workloadId!=s.workload || !processed || !(weight>0.f && weight<1.f))return;
 if(s.pair>=cw.fbPairCount || s.pair>=cw.fbPairs.size())return;
 const auto& mask=cw.fbPairs[s.pair];if(mask.colorImage.address!=s.address || mask.projectionCount>8 || mask.projectionCount>mask.projections.size())return;
 const auto& cd=cw.drawData;const auto& pd=pw.drawData;
 for(uint32_t pi=0;pi<mask.projectionCount;++pi){const uint32_t t=mask.projections[pi].transformsIndex;
  if(t>=cd.viewProjTransformGroups.size() || cd.viewProjTransformGroups[t]>=cd.transformGroups.size())continue;
  const auto& g=cd.transformGroups[cd.viewProjTransformGroups[t]];if((g.matrixId&0xF0000000u)!=0x90000000u || (g.matrixId&511u)!=480u)continue;
  const bool mapped=wm && wm->mapped && wm->prevWorkloadIndex==previousSlot && t<wm->viewProjections.size() && wm->viewProjections[t].mapped;
  const uint32_t pt=mapped?wm->viewProjections[t].prevTransformIndex:UINT32_MAX;
  const bool prior=mapped && pt<pd.viewProjTransformGroups.size() && pd.viewProjTransformGroups[pt]<pd.transformGroups.size() && pt<pd.viewProjTransforms.size();
  const uint32_t priorId=prior?pd.transformGroups[pd.viewProjTransformGroups[pt]].matrixId:0;
  const bool same=prior && cameraCpuSamePolicy(g,pd.transformGroups[pd.viewProjTransformGroups[pt]]),output=t<cd.modViewProjTransforms.size();
  bool finite=output && std::isfinite(previousWeight);double sum=0;for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)if(output){const float v=float(cd.modViewProjTransforms[t][i][j]);finite&=std::isfinite(v);sum+=v;}
  bool priorFinite=prior;for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)if(prior)priorFinite&=std::isfinite(float(pd.viewProjTransforms[pt][i][j]));
  std::fprintf(stderr,"baseline shadow-map sampled=1 workload=%llu previous_workload=%llu mask=%08x pair=%u projection=%u transform=%u id=%08x decompose=%u policy=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u mapped=%u prior_transform=%u prior_id=%08x same_policy=%u prior_present=%u prior_finite16=%u output_present=%u output_finite16=%u weight=%a previous_weight=%a output_sum=%a\n",
   (unsigned long long)cw.workloadId,(unsigned long long)pw.workloadId,s.address,s.pair,pi,t,g.matrixId,unsigned(g.decompose),g.positionInterpolation,g.rotationInterpolation,g.scaleInterpolation,g.skewInterpolation,g.perspectiveInterpolation,g.vertexInterpolation,g.texcoordInterpolation,g.tileInterpolation,g.lookAtInterpolation,g.ordering,g.aspectMode,g.editable,unsigned(mapped),pt,priorId,unsigned(same),unsigned(prior),unsigned(priorFinite),unsigned(output),unsigned(finite),double(weight),double(previousWeight),sum);
  s.done=true;std::fflush(stderr);return; // Unmapped is a sampled result, not a test failure.
 }
}

static void observePreSlots(const Workload& cw,const Workload& pw,const GameFrameMap::WorkloadMap* wm,uint32_t previousSlot,bool processed,float weight) {
 static unsigned rows=0;static uint64_t last=UINT64_MAX;if(rows>=4 || cw.workloadId==last || !processed || !(weight>0.f&&weight<1.f) || !shadow_witness::armed.load(std::memory_order_acquire))return;
 const auto& cd=cw.drawData;const auto& pd=pw.drawData;if(cw.fbPairCount>64 || cw.fbPairCount>cw.fbPairs.size() || pw.fbPairCount>64 || pw.fbPairCount>pw.fbPairs.size())return;
 auto vertices=[](const DrawData& d,uint32_t n){if(n>=d.worldTransformVertexIndices.size())return 0u;const uint32_t a=d.worldTransformVertexIndices[n],b=n+1<d.worldTransformVertexIndices.size()?d.worldTransformVertexIndices[n+1]:d.vertexCount();return a<=b&&b<=d.vertexCount()?b-a:0u;};
 for(uint32_t f=0;f<cw.fbPairCount;f++){const auto& fb=cw.fbPairs[f];if(fb.projectionCount>256 || fb.projectionCount>fb.projections.size())return;
 for(uint32_t q=0;q<fb.projectionCount;q++){const auto& projection=fb.projections[q];const uint32_t t=projection.transformsIndex;
  if(!projection.gameCallCount || projection.gameCallCount>projection.gameCalls.size() || t>=cd.viewProjTransformGroups.size() || cd.viewProjTransformGroups[t]>=cd.transformGroups.size())continue;
  const auto& g=cd.transformGroups[cd.viewProjTransformGroups[t]];if(g.matrixId<0xA0000000u || g.matrixId>=0xB0000000u || g.decompose || g.positionInterpolation!=1 || g.rotationInterpolation!=1)continue;
  const bool owner=wm&&wm->mapped&&wm->prevWorkloadIndex==previousSlot,pm=owner&&t<wm->viewProjections.size()&&wm->viewProjections[t].mapped;const uint32_t pt=pm?wm->viewProjections[t].prevTransformIndex:UINT32_MAX;
  const bool prior=pm&&pt<pd.viewProjTransformGroups.size()&&pd.viewProjTransformGroups[pt]<pd.transformGroups.size();const uint32_t priorId=prior?pd.transformGroups[pd.viewProjTransformGroups[pt]].matrixId:0;
  bool priorDraw=false;for(uint32_t pf=0;pf<pw.fbPairCount;pf++){const auto& b=pw.fbPairs[pf];if(b.projectionCount>256 || b.projectionCount>b.projections.size())return;for(uint32_t pq=0;pq<b.projectionCount;pq++)if(b.projections[pq].transformsIndex==pt && b.projections[pq].gameCallCount && b.projections[pq].gameCallCount<=b.projections[pq].gameCalls.size())priorDraw=true;}
  uint32_t reached[2]{},mapped[2]{},same[2]{},identity[2]{},oldGeometry[2]{},finite[2]{},priorModelId[2]{};const uint32_t limit=uint32_t(std::min<size_t>(4096,cd.worldTransformGroups.size()));
  for(uint32_t w=0;w<limit;w++){const uint32_t gi=cd.worldTransformGroups[w];if(gi>=cd.transformGroups.size() || w>=cd.worldTransforms.size() || !vertices(cd,w))continue;
   const auto& model=cd.transformGroups[gi];const uint32_t slot=model.matrixId-g.matrixId-1u;if(slot>=2)continue;reached[slot]++;
   const bool m=owner&&w<wm->transforms.size()&&wm->transforms[w].mapped;const uint32_t old=m?wm->transforms[w].prevTransformIndex:UINT32_MAX;
   if(m && old<pd.worldTransformGroups.size() && old<pd.worldTransforms.size() && pd.worldTransformGroups[old]<pd.transformGroups.size()){mapped[slot]++;const auto& og=pd.transformGroups[pd.worldTransformGroups[old]];priorModelId[slot]=og.matrixId;identity[slot]+=og.matrixId==model.matrixId;same[slot]+=cameraCpuSamePolicy(model,og);oldGeometry[slot]+=vertices(pd,old)>0;}
   bool ok=w<cd.lerpWorldTransforms.size();for(unsigned a=0;a<4;a++)for(unsigned b=0;b<4;b++)if(ok)ok&=std::isfinite(float(cd.lerpWorldTransforms[w][a][b]));finite[slot]+=ok;
  }
  std::fprintf(stderr,"baseline pre-map row=%u workload=%llu previous_workload=%llu pair=%u projection=%u transform=%u id=%08x prior_transform=%u prior_id=%08x projection_mapped=%u projection_same_policy=%u current_draw_calls=%u prior_draw=%u weight=%a model_ids=%08x,%08x prior_model_ids=%08x,%08x reached=%u,%u mapped=%u,%u same_policy=%u,%u identity_equal=%u,%u prior_geometry=%u,%u finite_output16=%u,%u scanned=%u truncated=%u\n",++rows,(unsigned long long)cw.workloadId,(unsigned long long)pw.workloadId,f,q,t,g.matrixId,pt,priorId,unsigned(pm),unsigned(prior&&cameraCpuSamePolicy(g,pd.transformGroups[pd.viewProjTransformGroups[pt]])),projection.gameCallCount,unsigned(priorDraw),double(weight),g.matrixId+1,g.matrixId+2,priorModelId[0],priorModelId[1],reached[0],reached[1],mapped[0],mapped[1],same[0],same[1],identity[0],identity[1],oldGeometry[0],oldGeometry[1],finite[0],finite[1],limit,unsigned(limit<cd.worldTransformGroups.size()));last=cw.workloadId;std::fflush(stderr);return;
 }}
}

static void observeCameraProjectionCpu(WorkloadQueue &queue, const GameFrame &current, const GameFrame &previous,
    bool processed, bool uploaded, float currentWeight, float previousWeight, float aspectScale,
    uint32_t targetRate, bool targetEmpty) {
    auto &r = cameraCpuObservation;
    ++r.calls;
    if (r.calls == 1) {
        std::fprintf(stderr, "CAMERA_CPU event=armed K=30 windows=120 attempts=480 rows=512 scenes=64 projections=256 calls=64 triangles=256\n");
        std::fflush(stderr);
    }
    if (current.workloads.size() != 1 || previous.workloads.size() != 1) {
        ++r.wrongOwnerCalls; r.lastStatus = "INCOMPLETE_OWNER_COUNT"; return;
    }
    const uint32_t c = current.workloads[0], p = previous.workloads[0];
    // The production caller holds workloadMutex. The short cursor snapshot is
    // released before ANY workload/previous framebuffer/call/face traversal.
    bool stable = false;
    {
        const std::lock_guard<std::mutex> cursorLock(queue.cursorMutex);
        stable = queue.workloads.size() == 4 && c < 4 && p < 4 &&
            p == queue.barrierCursor && c == (queue.barrierCursor + 1) % 4 &&
            queue.writeCursor != p && queue.writeCursor != c;
    }
    if (!stable) { ++r.guardCalls; r.lastStatus = "INCOMPLETE_SLOT_GUARD"; return; }
    const auto &cw = queue.workloads[c]; const auto &pw = queue.workloads[p];
    observeShadowComposite(cw);
    if (cw.paused || pw.paused || cw.debuggerCamera.enabled || pw.debuggerCamera.enabled) {
        ++r.guardCalls; r.lastStatus = "INCOMPLETE_PAUSED_DEBUGGER"; return;
    }
    if (c == p || cw.workloadId == pw.workloadId) {
        ++r.wrongOwnerCalls; r.lastStatus = "INCOMPLETE_SELF_OWNER"; return;
    }
    const auto* receiptMap=c<current.frameMap.workloads.size()?&current.frameMap.workloads[c]:nullptr;
    observeShadowMaskMapping(cw,pw,receiptMap,p,processed,currentWeight,previousWeight);
    observePreSlots(cw,pw,receiptMap,p,processed,currentWeight);
    if(r.stopped)return;
    const std::array<uint64_t, 4> tuple = {c, cw.workloadId, p, pw.workloadId};
    if (!r.haveLast || tuple != r.last) {
        r.haveLast = true; r.last = tuple; ++r.changes; r.inWindow = 0;
        r.selected = (++r.phase == 30);
        if (r.selected) { r.phase = 0; if (r.windows == 120) r.stopped = true; else ++r.windows; }
    }
    auto summary = [&]() {
        std::fprintf(stderr, "CAMERA_CPU event=stop calls=%llu changes=%llu windows=%llu attempts=%llu rows=%llu positive=%llu incomplete=%llu invalid=%llu nonfinite=%llu wrongOwner=%llu guard=%llu skipped=%llu\n",
            r.calls,r.changes,r.windows,r.attempts,r.rows,r.positive,r.incomplete,r.invalid,r.nonfinite,r.wrongOwnerCalls,r.guardCalls,r.skippedSameWindow);
        std::fflush(stderr);
    };
    if (r.stopped) { summary(); return; }
    if (!r.selected) return;
    if (r.inWindow == 4) { ++r.skippedSameWindow; return; }
    ++r.inWindow; ++r.attempts;
    struct Budget { unsigned scenes=0, projections=0, calls=0, triangles=0; bool truncated=false, invalid=false; } b;
    auto take = [&](unsigned &n, unsigned limit) { if (n == limit) { b.truncated=true; return false; } ++n; return true; };
    struct Choice {
        const Projection *proj = nullptr;
        const TransformGroup *group = nullptr;
        uint32_t fb=0, projection=0, scene=0, call=0, triangle=0, t=0, pt=0;
        Projection::Type type = Projection::Type::None;
        unsigned rank=0, aliases=0, processedAliases=0, previousAliases=0;
        bool draw=false, previousDraw=false, policy=false;
    } choice;
    const auto &cd = cw.drawData; const auto &pd = pw.drawData;
    const auto *wm = c < current.frameMap.workloads.size() ? &current.frameMap.workloads[c] : nullptr;
    const bool ownerMapped = wm && wm->mapped && wm->prevWorkloadIndex == p;
    auto inputs = [&](const DrawData &d, uint32_t t) {
        if (t == 0 || t >= d.viewTransforms.size() || t >= d.projTransforms.size() ||
            t >= d.viewProjTransforms.size() || t >= d.viewProjTransformGroups.size()) { b.invalid=true; return false; }
        if (d.viewProjTransformGroups[t] >= d.transformGroups.size()) { b.invalid=true; return false; }
        return true;
    };
    auto witness = [&](const DrawData &d, const Projection &proj, uint32_t &ci, uint32_t &ti) {
        if (proj.scissorRect.isNull() || proj.scissorRect.isEmpty() || !proj.gameCallCount) return false;
        if (proj.gameCallCount > proj.gameCalls.size()) { b.invalid=true; return false; }
        for (uint32_t i=0; i<proj.gameCallCount; ++i) {
            if (!take(b.calls,64)) return false;
            const auto &call=proj.gameCalls[i];
            if (call.callDesc.scissorRect.isNull()) continue;
            const uint64_t start=call.meshDesc.faceIndicesStart, n=uint64_t(call.callDesc.triangleCount)*3;
            if (start > d.faceIndices.size() || n > d.faceIndices.size()-start) { b.invalid=true; return false; }
            for (uint32_t j=0; j<call.callDesc.triangleCount; ++j) {
                if (!take(b.triangles,256)) return false;
                bool same=true;
                for (unsigned k=0;k<3;++k) {
                    const uint32_t v=d.faceIndices[size_t(start)+size_t(j)*3+k];
                    if (v >= d.viewProjIndices.size()) { b.invalid=true; return false; }
                    same &= d.viewProjIndices[v] == proj.transformsIndex;
                }
                if (same) { ci=i; ti=j; return true; }
            }
        }
        return false;
    };
    // All current, predecessor and alias passes share one budget. No vector/map allocation or recursion.
    auto scan = [&](const GameFrame &frame, const Workload &w, uint32_t owner, auto visit) {
        auto scenes = [&](const std::vector<GameScene> &list, Projection::Type type) {
            for (uint32_t s=0;s<list.size();++s) {
                if (!take(b.scenes,64)) return;
                for (const auto &entry:list[s].projections) {
                    if (!take(b.projections,256)) return;
                    if (entry.workloadIndex != owner || entry.fbPairIndex >= w.fbPairCount || entry.fbPairIndex >= w.fbPairs.size()) { b.invalid=true; return; }
                    const auto &fb=w.fbPairs[entry.fbPairIndex];
                    if (entry.projectionIndex >= fb.projectionCount || entry.projectionIndex >= fb.projections.size()) { b.invalid=true; return; }
                    const auto &proj=fb.projections[entry.projectionIndex];
                    if (proj.type != type || !inputs(w.drawData,proj.transformsIndex)) { b.invalid=true; return; }
                    visit(proj,entry,s,type);
                    if (b.invalid || b.truncated) return;
                }
            }
        };
        scenes(frame.perspectiveScenes,Projection::Type::Perspective);
        if (!b.invalid && !b.truncated) scenes(frame.orthographicScenes,Projection::Type::Orthographic);
    };
    scan(current,cw,c,[&](const Projection &proj,const GameIndices::Projection &entry,uint32_t s,Projection::Type type) {
        const uint32_t t=proj.transformsIndex;
        const auto &g=cd.transformGroups[cd.viewProjTransformGroups[t]];
        if (g.matrixId < 0xA0000000u || g.matrixId >= 0xB0000000u) return;
        uint32_t ci=0,ti=0; const bool draw=witness(cd,proj,ci,ti);
        bool mapped=false;
        if (ownerMapped) {
            if (t >= wm->viewProjections.size()) { b.invalid=true; return; }
            mapped=wm->viewProjections[t].mapped;
        }
        uint32_t pt=0; unsigned previousAliases=0;
        bool policy=false, previousDraw=false;
        if (mapped && !b.invalid && !b.truncated) {
            pt=wm->viewProjections[t].prevTransformIndex;
            if (inputs(pd,pt)) {
                policy=cameraCpuSamePolicy(g,pd.transformGroups[pd.viewProjTransformGroups[pt]]);
                scan(previous,pw,p,[&](const Projection &prior,const GameIndices::Projection &,uint32_t,Projection::Type priorType) {
                    if (prior.transformsIndex != pt || priorType != type) return;
                    ++previousAliases;
                    uint32_t pci=0,pti=0;
                    if (!previousDraw) previousDraw=witness(pd,prior,pci,pti);
                });
            }
        }
        // Rank only after predecessor association, with the SAME budget. A later
        // complete candidate must outrank an earlier mapped-but-unqualified A.
        const bool qualified=mapped&&draw&&previousDraw&&policy;
        const unsigned rank=qualified?4u:(mapped?3u:(draw?2u:1u));
        if (rank > choice.rank) {
            choice.proj=&proj; choice.group=&g; choice.t=t; choice.fb=entry.fbPairIndex;
            choice.projection=entry.projectionIndex; choice.scene=s; choice.type=type;
            choice.call=ci; choice.triangle=ti; choice.draw=draw; choice.rank=rank;
            choice.pt=pt; choice.policy=policy; choice.previousDraw=previousDraw;
            choice.previousAliases=previousAliases;
        }
    });
    const GameFrameMap::ViewProjectionMap *vm=nullptr;
    if (choice.proj && !b.invalid && !b.truncated) {
        scan(current,cw,c,[&](const Projection &proj,const GameIndices::Projection &,uint32_t,Projection::Type) {
            if (proj.transformsIndex == choice.t) { ++choice.aliases; choice.processedAliases += !proj.scissorRect.isNull(); }
        });
        if (ownerMapped) vm=&wm->viewProjections[choice.t];
    }
    const char *status="NO_CANDIDATE";
    bool finite=std::isfinite(currentWeight)&&std::isfinite(previousWeight)&&std::isfinite(aspectScale);
    bool output=false;
    std::array<double,12> sums={};
    std::array<uint32_t,12> sampleBits={};
    unsigned scalarVisits=0;
    if (choice.proj) {
        status=(!vm || !vm->mapped) ? "INCOMPLETE_UNMAPPED_A" : "INCOMPLETE_MAPPED_A";
        output=!b.invalid && !b.truncated && vm && vm->mapped && choice.draw && choice.previousDraw && choice.policy && processed && aspectScale==1.f;
        if (output) {
            const uint32_t t=choice.t;
            if (t>=cd.modViewTransforms.size()||t>=cd.modProjTransforms.size()||t>=cd.modViewProjTransforms.size()||
                t>=cd.prevViewTransforms.size()||t>=cd.prevProjTransforms.size()||t>=cd.prevViewProjTransforms.size()) b.invalid=true;
            else {
                auto matrix=[&](const auto &m,unsigned n) {
                    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j) {
                        const float v=float(m[i][j]); ++scalarVisits; finite &= std::isfinite(v); sums[n]+=v;
                        if(i==3&&j==0)std::memcpy(&sampleBits[n],&v,4);
                    }
                };
                matrix(cd.viewTransforms[t],0);matrix(cd.projTransforms[t],1);matrix(cd.viewProjTransforms[t],2);
                matrix(pd.viewTransforms[choice.pt],3);matrix(pd.projTransforms[choice.pt],4);matrix(pd.viewProjTransforms[choice.pt],5);
                matrix(cd.modViewTransforms[t],6);matrix(cd.modProjTransforms[t],7);matrix(cd.modViewProjTransforms[t],8);
                matrix(cd.prevViewTransforms[t],9);matrix(cd.prevProjTransforms[t],10);matrix(cd.prevViewProjTransforms[t],11);
                status="MAPPED_DRAW_PROJECTION_CPU_OUTPUT";
            }
        }
    }
    if (b.truncated) status="TRUNCATED";
    if (!finite) status="NONFINITE";
    if (b.invalid) status="INVALID";
    const bool positive=std::strcmp(status,"MAPPED_DRAW_PROJECTION_CPU_OUTPUT")==0;
    r.lastStatus=status;
    const uint64_t nextPositive=r.positive+positive, nextInvalid=r.invalid+b.invalid, nextNonfinite=r.nonfinite+!finite;
    const uint64_t nextIncomplete=r.incomplete+(!positive&&!b.invalid&&finite), nextRows=r.rows+1;
    r.lastAliases=choice.aliases; r.lastProcessedAliases=choice.processedAliases; r.lastScalarVisits=scalarVisits;
    r.lastTransform=choice.t;
    // Counters commit only after the complete row write AND flush succeed.
    const TransformGroup empty; const auto &g=choice.group?*choice.group:empty;
    const RigidBody emptyBody; const auto &rb=vm?vm->rigidBody:emptyBody;
    const int rowWrite=std::fprintf(stderr,"CAMERA_CPU event=attempt status=%s call=%llu changes=%llu phase=%u K=30 window=%llu attempt=%llu inWindow=%u rows=%llu c=%u p=%u cid=%llu pid=%llu cPresent=%llu pPresent=%llu cSubmit=%llu pSubmit=%llu processed=%u uploaded=%u wc=%.9g wp=%.9g interior=%u aspect=%.9g target=%u targetEmpty=%u type=%u scene=%u fb=%u projection=%u t=%u pt=%u witnessCall=%u witnessTriangle=%u aliases=%u processedAliases=%u previousAliases=%u aliasesComplete=%u id=%08x policy=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u flags=%u,%u,%u,%u,%u,%u transformIndex=%u valid=%u,%u visits=%u,%u,%u,%u scalars=%u positive=%llu incomplete=%llu invalid=%llu nonfinite=%llu wrongOwner=%llu guard=%llu skipped=%llu matrixSums=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g sampleBits=%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x\n",
        status,r.calls,r.changes,r.phase,r.windows,r.attempts,r.inWindow,nextRows,c,p,cw.workloadId,pw.workloadId,cw.presentId,pw.presentId,cw.submissionFrame,pw.submissionFrame,
        unsigned(processed),unsigned(uploaded),currentWeight,previousWeight,unsigned(currentWeight>0&&currentWeight<1),aspectScale,targetRate,unsigned(targetEmpty),unsigned(choice.type),choice.scene,choice.fb,choice.projection,choice.t,choice.pt,choice.call,choice.triangle,choice.aliases,choice.processedAliases,choice.previousAliases,unsigned(!b.truncated&&!b.invalid),g.matrixId,
        unsigned(g.decompose),g.positionInterpolation,g.rotationInterpolation,g.scaleInterpolation,g.skewInterpolation,g.perspectiveInterpolation,g.vertexInterpolation,g.texcoordInterpolation,g.tileInterpolation,g.lookAtInterpolation,g.ordering,g.aspectMode,g.editable,unsigned(choice.policy),
        unsigned(rb.lerpTranslation),unsigned(rb.lerpRotation),unsigned(rb.lerpScale),unsigned(rb.lerpSkew),unsigned(rb.lerpPerspective),unsigned(rb.lerpDecompose),unsigned(rb.transformIndex),unsigned(rb.transforms[0].valid),unsigned(rb.transforms[1].valid),b.scenes,b.projections,b.calls,b.triangles,scalarVisits,nextPositive,nextIncomplete,nextInvalid,nextNonfinite,r.wrongOwnerCalls,r.guardCalls,r.skippedSameWindow,
        sums[0],sums[1],sums[2],sums[3],sums[4],sums[5],sums[6],sums[7],sums[8],sums[9],sums[10],sums[11],
        sampleBits[0],sampleBits[1],sampleBits[2],sampleBits[3],sampleBits[4],sampleBits[5],sampleBits[6],sampleBits[7],sampleBits[8],sampleBits[9],sampleBits[10],sampleBits[11]);
    const int rowFlush=std::fflush(stderr);
    if (rowWrite<0 || rowFlush!=0) {
        r.ioFailed=true; r.ioWriteFailed=rowWrite<0; r.ioFlushFailed=rowFlush!=0;
        r.stopped=true; r.lastStatus="IO_FAILURE";
        // Existing monitor consumes stdout too; an unknown event is fatal instrumentation.
        // No write retry. If this notification also fails, the sticky state remains.
        const int diagnosticWrite=std::fprintf(stdout,"CAMERA_CPU event=io-failure call=%llu attempt=%llu writeFailed=%u flushFailed=%u rows=%llu\n",
            r.calls,r.attempts,unsigned(r.ioWriteFailed),unsigned(r.ioFlushFailed),r.rows);
        const int diagnosticFlush=std::fflush(stdout);
        r.ioDiagnosticFailed=diagnosticWrite<0 || diagnosticFlush!=0;
        return;
    }
    r.rows=nextRows; r.positive=nextPositive; r.incomplete=nextIncomplete;
    r.invalid=nextInvalid; r.nonfinite=nextNonfinite;
    if (r.attempts==480 || r.rows==512) { r.stopped=true; summary(); }
}
} // anonymous namespace: no public or mixed archive/fixture inline Record type.
