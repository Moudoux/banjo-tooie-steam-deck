#pragma once
#include "rt64_matrix_trace.hpp"
#include "hle/rt64_workload.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace RT64 {
// Source draw observation only: current object pose, final CPU projection, no vertex velocity,
// raster screenScale/offset, clipping, or opaque-coverage claim. No guest reads or allocation.
inline TooieTransitionSourceDraw tooieTransitionSourceDraw(const DrawData& data,
    const std::vector<FramebufferPair>& pairs, uint32_t pairCount, uint32_t root,
    bool projectionsProcessed) noexcept {
    using O=TooieTransitionSourceDraw;
    O out{}; out.flags=O::Observed | O::CurrentPose;
    constexpr uint32_t maxCalls=4096, maxIndices=32766, maxModifies=256;
    uint32_t scannedCalls=0, scannedIndices=0, scannedProjections=0, firstProjection=UINT32_MAX;
    bool haveSource=false, haveBounds=false;
    float minX=INFINITY,minY=INFINITY,maxX=-INFINITY,maxY=-INFINITY;
    auto bad=[&](uint32_t bit){out.flags|=bit;};
    if(root>=data.worldTransforms.size()){bad(O::Invalid);return out;}
    if(pairCount>pairs.size())bad(O::Invalid);
    if(data.modifyPosUints.size()%2)bad(O::Invalid);
    if(data.modifyPosUints.size()/2>maxModifies)bad(O::Modified);
    for(uint32_t f=0;f<std::min<size_t>(pairCount,pairs.size());++f){
        if(f>=256){bad(O::Truncated);goto done;}
        const auto& pair=pairs[f];
        if(pair.projectionCount>pair.projections.size())bad(O::Invalid);
        for(uint32_t p=0;p<std::min<size_t>(pair.projectionCount,pair.projections.size());++p){
            if(scannedProjections++>=4096){bad(O::Truncated);goto done;}
            const auto& projection=pair.projections[p];
            if(projection.gameCallCount>projection.gameCalls.size())bad(O::Invalid);
            for(uint32_t c=0;c<std::min<size_t>(projection.gameCallCount,projection.gameCalls.size());++c){
                if(scannedCalls++>=maxCalls){bad(O::Truncated);goto done;}
                const auto& call=projection.gameCalls[c];
                if(projection.type!=Projection::Type::Perspective && projection.type!=Projection::Type::Orthographic)continue;
                // Maintained call range is a broad-phase exclusion only; inclusion still requires exact index ownership.
                if(call.callDesc.minWorldMatrix>call.callDesc.maxWorldMatrix){bad(O::Invalid);continue;}
                if(root<call.callDesc.minWorldMatrix || root>call.callDesc.maxWorldMatrix)continue;
                const uint64_t start=call.meshDesc.faceIndicesStart,n=uint64_t(call.callDesc.triangleCount)*3;
                if(start>data.faceIndices.size() || n>data.faceIndices.size()-start){bad(O::Invalid);continue;}
                bool countedCall=false;
                for(uint64_t i=0;i<n;i+=3){
                    if(scannedIndices>maxIndices-3){bad(O::Truncated);goto done;} scannedIndices+=3;
                    uint32_t v[3];unsigned owned=0;bool valid=true;
                    for(unsigned j=0;j<3;++j){v[j]=data.faceIndices[size_t(start+i+j)];
                        if(v[j]>=data.worldIndices.size()){bad(O::Invalid);valid=false;}
                        else if(data.worldIndices[v[j]]==root)++owned;
                    }
                    if(!owned)continue;
                    if(!countedCall){++out.calls;countedCall=true;
                        const auto& s=call.callDesc.scissorRect;
                        const uint32_t pi=projection.transformsIndex;
                        if(pi>=data.rspViewports.size() || uint64_t(pi)*4+4>data.viewportClipRatios.size() || pi>=data.viewportOrigins.size())bad(O::Invalid);
                        else {
                            const auto& sv=data.rspViewports[pi]; bool finiteViewport=true;
                            for(unsigned k=0;k<4;++k){
                                const unsigned axis=k&1u;
                                const double edge=(double(sv.translate[axis])-double(sv.scale[axis])*data.viewportClipRatios[size_t(pi)*4+k])*4.0;
                                if(!std::isfinite(edge) || std::abs(edge)>double(INT32_MAX)-4096.0)finiteViewport=false;
                            }
                            if(!finiteViewport){bad(O::Invalid);continue;}
                            auto vp=sv.rect(&data.viewportClipRatios[size_t(pi)*4]);
                            const int32_t sc[4]={s.ulx,s.uly,s.lrx,s.lry};
                            const int32_t vr[4]={vp.ulx,vp.uly,vp.lrx,vp.lry};
                            if(haveSource){for(unsigned k=0;k<4;++k)if(out.scissor[k]!=sc[k] || out.viewport[k]!=vr[k])bad(O::MixedSource);}
                            else {for(unsigned k=0;k<4;++k){out.scissor[k]=sc[k];out.viewport[k]=vr[k];}haveSource=true;}
                            if(call.callDesc.scissorLeftOrigin!=G_EX_ORIGIN_NONE || call.callDesc.scissorRightOrigin!=G_EX_ORIGIN_NONE || data.viewportOrigins[pi]!=G_EX_ORIGIN_NONE)bad(O::CustomOrigin);
                        }
                    }
                    if(!valid || owned!=3){bad(O::MixedOwnership);continue;}
                    ++out.triangles;
                    for(unsigned j=0;j<3;++j){
                        const uint32_t vertex=v[j];const uint64_t pos=uint64_t(vertex)*3;
                        if(pos+3>data.posFloats.size() || vertex>=data.viewProjIndices.size()){bad(O::Invalid);continue;}
                        const uint32_t pi=data.viewProjIndices[vertex];
                        if(pi!=projection.transformsIndex){bad(O::MixedProjection);continue;}
                        if(firstProjection==UINT32_MAX)firstProjection=pi;else if(firstProjection!=pi)bad(O::MixedProjection);
                        if(pi>=data.viewProjTransforms.size() || (projectionsProcessed && pi>=data.modViewProjTransforms.size())){bad(O::Invalid);continue;}
                        bool modified=false;
                        for(size_t m=0;m+1<data.modifyPosUints.size() && m<maxModifies*2;m+=2)
                            if((data.modifyPosUints[m]>>1)==vertex)modified=true;
                        if(modified){bad(O::Modified);continue;}
                        const auto& matrix=projectionsProcessed?data.modViewProjTransforms[pi]:data.viewProjTransforms[pi];
                        auto clip=hlslpp::mul(hlslpp::mul(hlslpp::float4(data.posFloats[size_t(pos)],data.posFloats[size_t(pos)+1],data.posFloats[size_t(pos)+2],1.0f),data.worldTransforms[root]),matrix);
                        const float x=float(clip.x),y=float(clip.y),z=float(clip.z),w=float(clip.w);
                        if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z)||!std::isfinite(w)||w<=0){bad(O::InvalidW);continue;}
                        const float nx=x/w,ny=-y/w;
                        if(!std::isfinite(nx)||!std::isfinite(ny)){bad(O::InvalidW);continue;}
                        minX=std::min(minX,nx);minY=std::min(minY,ny);maxX=std::max(maxX,nx);maxY=std::max(maxY,ny);haveBounds=true;
                    }
                }
            }
        }
    }
 done:
    if(haveSource)out.flags|=O::Source;
    const uint32_t disqualify=O::Invalid|O::Truncated|O::MixedOwnership|O::MixedProjection|O::Modified|O::InvalidW;
    if(haveBounds && !(out.flags&disqualify)){out.flags|=O::Bounds;out.ndc[0]=minX;out.ndc[1]=minY;out.ndc[2]=maxX;out.ndc[3]=maxY;}
    if(projectionsProcessed)out.flags|=O::ProcessedProjection;
    return out;
}
}
