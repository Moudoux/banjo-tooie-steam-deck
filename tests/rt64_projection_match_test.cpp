// Matching revision2 regression: actual maintained OwnerFixture and production archive.
// Its toy-policy main is renamed and never called by this executable.
#ifndef TOOIE_V10_ARM
#error Compile with TOOIE_V10_ARM=0 and the existing baseline RT64 fixture settings.
#endif
static_assert(TOOIE_V10_ARM == 0, "Selected ownership guards must remain enabled");
#define main session02_retained_owner_main_not_called
#include "tests/rt64_scene_match_policy_test.cpp"
#undef main

namespace {
constexpr uint32_t stableId = 0xA0000101u;
constexpr uint32_t cutId = 0xA0000201u;
uint32_t sessionRows = 0;

void policy(OwnerFixture &f, uint32_t owner, uint32_t index, uint8_t order) {
    auto &g = f.queue.workloads[owner].drawData.transformGroups[index];
    const bool ignore = g.matrixId == G_EX_ID_IGNORE;
    g.decompose = !ignore;
    g.positionInterpolation = g.rotationInterpolation = g.scaleInterpolation =
        g.skewInterpolation = g.perspectiveInterpolation =
        ignore ? G_EX_COMPONENT_SKIP : G_EX_COMPONENT_INTERPOLATE;
    g.vertexInterpolation = g.texcoordInterpolation = G_EX_COMPONENT_SKIP;
    g.tileInterpolation = g.lookAtInterpolation = ignore ? G_EX_COMPONENT_SKIP : G_EX_COMPONENT_AUTO;
    g.ordering = order;
    g.editable = G_EX_EDIT_NONE;
    g.aspectMode = G_EX_ASPECT_AUTO;
}
void add(OwnerFixture &f, uint32_t owner, uint32_t index, float x, uint32_t id, uint8_t order) {
    f.projection(owner,index,x,id);
    policy(f,owner,index,order);
}
void member(const RT64::GameScene &s, size_t position, uint32_t owner, uint32_t projection) {
    assert(position < s.projections.size());
    const auto &p = s.projections[position];
    assert(p.workloadIndex == owner && p.fbPairIndex == 0 && p.projectionIndex == projection);
}
void clean(const OwnerFixture &f) {
    assert(!f.upload && !f.tile && !f.look);
    for (uint32_t owner : {0u,1u,2u,3u}) {
        const auto &d = f.queue.workloads[owner].drawData;
        assert(d.velFloats.empty() && d.tcVelFloats.empty());
    }
}
void output(OwnerFixture &f, const char *name, uint8_t order, unsigned previousCount,
            float x, uint32_t owner, uint32_t index) {
    const auto &m = f.map(owner,index);
    const auto &g = f.queue.workloads[owner].drawData.transformGroups[index];
    std::printf("CAMERA_MATCH row=%u case=%s order=%u previous_count=%u current_count=%u x=%.9g current_scenes=%zu previous_scenes=%zu owner=%u index=%u id=%08x mapped=%u previous_index=%u decompose=%u lerp_translation=%u lerp_rotation=%u lerp_scale=%u lerp_skew=%u lerp_perspective=%u upload=%u tile=%u look=%u\n",
        ++sessionRows,name,unsigned(order),previousCount,f.queue.workloads[owner].fbPairs[0].projectionCount,double(x),
        f.current.perspectiveScenes.size(),f.previous.perspectiveScenes.size(),owner,index,
        g.matrixId,unsigned(m.mapped),m.prevTransformIndex,unsigned(m.rigidBody.lerpDecompose),
        unsigned(m.rigidBody.lerpTranslation),unsigned(m.rigidBody.lerpRotation),
        unsigned(m.rigidBody.lerpScale),unsigned(m.rigidBody.lerpSkew),
        unsigned(m.rigidBody.lerpPerspective),unsigned(f.upload),unsigned(f.tile),unsigned(f.look));
}
void later(uint8_t order, unsigned previousCount, float x, uint32_t laterId, const char *name) {
    OwnerFixture f;
    add(f,0,1,0,stableId,order); add(f,0,2,x,laterId,order);
    add(f,2,1,0,stableId,order);
    if (previousCount == 2) add(f,2,2,0,stableId,order);
    f.set();
    if (x != 0) assert(f.queue.workloads[0].drawData.viewTransforms[2][3][0] !=
        f.queue.workloads[0].drawData.viewTransforms[1][3][0]);
    assert(f.current.perspectiveScenes.size() == 1 && f.previous.perspectiveScenes.size() == 1);
    const auto &cs = f.current.perspectiveScenes[0];
    const auto &ps = f.previous.perspectiveScenes[0];
    assert(cs.projections.size() == 2 && ps.projections.size() == previousCount);
    member(cs,0,0,0); member(cs,1,0,1); member(ps,0,2,0);
    if (previousCount == 2) member(ps,1,2,1);
    assert(f.current.isSceneCompatible(f.queue,cs,cs.projections[1]));
    f.match(); clean(f);
    assert(f.current.frameMap.workloads[0].mapped);
    assert(f.current.frameMap.workloads[0].prevWorkloadIndex == 2);
    assert(f.map(0,1).mapped && f.map(0,1).prevTransformIndex == 1);
    assert(f.map(0,2).mapped == (laterId == stableId));
    if (f.map(0,2).mapped) assert(f.map(0,2).prevTransformIndex == 1);
    output(f,name,order,previousCount,x,0,2);
}
void controls(uint8_t order) {
    {
        OwnerFixture f;
        add(f,0,1,0,stableId,order); add(f,0,2,32,cutId,order);
        add(f,2,1,0,stableId,order); add(f,2,2,32,stableId,order);
        f.set();
        assert(f.current.perspectiveScenes.size()==2 && f.previous.perspectiveScenes.size()==2);
        for (size_t i=0;i<2;++i) {
            assert(f.current.perspectiveScenes[i].projections.size()==1);
            assert(f.previous.perspectiveScenes[i].projections.size()==1);
            member(f.current.perspectiveScenes[i],0,0,uint32_t(i));
            member(f.previous.perspectiveScenes[i],0,2,uint32_t(i));
        }
        assert(!f.current.isSceneCompatible(f.queue,f.current.perspectiveScenes[0],
            f.current.perspectiveScenes[1].projections[0]));
        f.match(); clean(f);
        assert(f.map(0,1).mapped && !f.map(0,2).mapped);
        output(f,"different-scene-cut",order,2,32,0,2);
    }
    {
        OwnerFixture f;
        add(f,0,1,0,stableId,order); add(f,1,2,0,cutId,order);
        add(f,2,1,0,stableId,order); add(f,3,2,0,stableId,order);
        f.set();
        assert(f.current.perspectiveScenes.size()==1 && f.previous.perspectiveScenes.size()==1);
        assert(f.current.perspectiveScenes[0].projections.size()==2);
        assert(f.previous.perspectiveScenes[0].projections.size()==2);
        member(f.current.perspectiveScenes[0],0,0,0); member(f.current.perspectiveScenes[0],1,1,0);
        member(f.previous.perspectiveScenes[0],0,2,0); member(f.previous.perspectiveScenes[0],1,3,0);
        f.match(); clean(f);
        assert(f.map(0,1).mapped && !f.map(1,2).mapped);
        assert(f.current.frameMap.workloads[1].prevWorkloadIndex==3);
        output(f,"foreign-workload-cut",order,2,0,1,2);
    }
    {
        OwnerFixture f;
        add(f,0,1,0,G_EX_ID_IGNORE,order); add(f,0,2,0,G_EX_ID_IGNORE,order);
        add(f,2,1,0,stableId,order); add(f,2,2,0,stableId,order);
        f.set();
        assert(f.current.perspectiveScenes.size()==1 && f.previous.perspectiveScenes.size()==1);
        assert(f.current.perspectiveScenes[0].projections.size()==2);
        assert(f.previous.perspectiveScenes[0].projections.size()==2);
        member(f.current.perspectiveScenes[0],0,0,0); member(f.current.perspectiveScenes[0],1,0,1);
        member(f.previous.perspectiveScenes[0],0,2,0); member(f.previous.perspectiveScenes[0],1,2,1);
        f.match(); clean(f);
        assert(!f.map(0,1).mapped && !f.map(0,2).mapped);
        output(f,"first-ignore",order,2,0,0,2);
    }
}
// Grow-only variant of maintained OwnerFixture::projection. Index 3 must not be
// shrunk away by a later insertion at index 1/2, including repeated indices.
void grow(OwnerFixture &f,uint32_t owner,uint32_t index,uint32_t id) {
    auto &work=f.queue.workloads[owner];
    work.debuggerCamera.enabled=false;work.fbPairCount=1;work.fbPairs.resize(1);
    auto &pair=work.fbPairs[0];
    pair.colorImage.address=4096;pair.colorImage.fmt=0;pair.colorImage.siz=2;pair.colorImage.width=320;
    pair.depthRead=pair.depthWrite=false;pair.scissorRect=RT64::FixedRect(0,0,1280,960);
    auto &d=work.drawData;
    const size_t size=std::max(d.viewTransforms.size(),size_t(index+1));
    d.viewTransforms.resize(size);d.projTransforms.resize(size);d.viewProjTransforms.resize(size);
    d.viewportOrigins.resize(size);d.viewProjTransformGroups.resize(size);d.transformGroups.resize(size);
    d.viewTransforms[index]=RT64::matrixTranslation(hlslpp::float3(0,0,0));
    d.projTransforms[index]=hlslpp::float4x4(2,0,0,0, 0,2,0,0, 0,0,-1,-1, 0,0,-2,0);
    d.viewProjTransforms[index]=hlslpp::mul(d.viewTransforms[index],d.projTransforms[index]);
    d.viewProjTransformGroups[index]=index;d.transformGroups[index].matrixId=id;
    policy(f,owner,index,G_EX_ORDER_LINEAR);
    RT64::Projection projection;projection.type=RT64::Projection::Type::Perspective;
    projection.transformsIndex=index;projection.scissorRect=pair.scissorRect;
    pair.projections.push_back(projection);pair.projectionCount=uint32_t(pair.projections.size());
}
auto &group(OwnerFixture &f,uint32_t owner,uint32_t index) {
    return f.queue.workloads[owner].drawData.transformGroups[index];
}
void skip(OwnerFixture &f,uint32_t owner,uint32_t index) {
    auto &g=group(f,owner,index);
    g.positionInterpolation=g.rotationInterpolation=g.scaleInterpolation=
        g.skewInterpolation=g.perspectiveInterpolation=G_EX_COMPONENT_SKIP;
}
void flags(OwnerFixture &f,uint32_t index,bool decompose,bool interpolate) {
    auto &m=f.map(0,index);
    assert(m.rigidBody.lerpDecompose==decompose);
    assert(m.rigidBody.lerpTranslation==interpolate && m.rigidBody.lerpRotation==interpolate);
    assert(m.rigidBody.lerpScale==interpolate && m.rigidBody.lerpSkew==interpolate);
    // Stock projection matching never calls updatePerspective; preserve that.
    assert(!m.rigidBody.lerpPerspective);
}
void scene(OwnerFixture &f,size_t current,size_t previous) {
    f.set();
    assert(f.current.perspectiveScenes.size()==1 && f.previous.perspectiveScenes.size()==1);
    assert(f.current.perspectiveScenes[0].projections.size()==current);
    assert(f.previous.perspectiveScenes[0].projections.size()==previous);
    for(size_t i=0;i<current;i++)member(f.current.perspectiveScenes[0],i,0,uint32_t(i));
    for(size_t i=0;i<previous;i++)member(f.previous.perspectiveScenes[0],i,2,uint32_t(i));
}
void finish(OwnerFixture &f,const char* name,unsigned previous,uint32_t index) {
    clean(f);output(f,name,G_EX_ORDER_LINEAR,previous,0,0,index);
}
void mandatory() {
    // N1: fallback B and repeated-index shortcut must not replace first anchor A.
    {
        OwnerFixture f;
        grow(f,0,1,stableId);grow(f,0,2,cutId);grow(f,0,2,cutId);grow(f,0,3,stableId);
        grow(f,2,1,stableId);grow(f,2,2,cutId);grow(f,2,2,cutId);
        skip(f,0,2);group(f,0,2).decompose=false;
        scene(f,4,3);f.match();
        assert(f.map(0,2).mapped && f.map(0,2).prevTransformIndex==2);
        flags(f,2,false,false);assert(f.map(0,2).rigidBody.transformIndex==1);
        assert(f.map(0,3).mapped && f.map(0,3).prevTransformIndex==1);flags(f,3,true,true);
        finish(f,"N1-first-anchor-shortcut",3,3);
    }
    {
        OwnerFixture f;
        grow(f,0,1,stableId);grow(f,0,2,cutId);grow(f,0,3,cutId);
        grow(f,2,1,stableId);grow(f,2,2,cutId);scene(f,3,2);f.match();
        assert(f.map(0,2).mapped && f.map(0,2).prevTransformIndex==2);
        assert(!f.map(0,3).mapped);finish(f,"N2-secondary-count-limit",2,3);
    }
    const char* names[]={"N3a-decompose","N3a-position","N3a-rotation","N3a-scale","N3a-skew","N3a-perspective"};
    for(unsigned field=0;field<6;field++) {
        OwnerFixture f;grow(f,0,1,stableId);grow(f,0,2,stableId);grow(f,2,1,stableId);
        auto &g=group(f,0,2);
        switch(field){
        case 0:g.decompose=false;break;
        case 1:g.positionInterpolation=G_EX_COMPONENT_SKIP;break;
        case 2:g.rotationInterpolation=G_EX_COMPONENT_SKIP;break;
        case 3:g.scaleInterpolation=G_EX_COMPONENT_SKIP;break;
        case 4:g.skewInterpolation=G_EX_COMPONENT_SKIP;break;
        case 5:g.perspectiveInterpolation=G_EX_COMPONENT_SKIP;break;
        }
        scene(f,2,1);f.match();assert(f.map(0,1).mapped && !f.map(0,2).mapped);
        finish(f,names[field],1,2);
    }
    for(bool firstSkip:{false,true}) {
        OwnerFixture f;grow(f,0,1,stableId);grow(f,0,2,stableId);
        grow(f,2,1,stableId);grow(f,2,2,stableId);
        skip(f,0,firstSkip?1:2);scene(f,2,2);f.match();
        assert(f.map(0,2).mapped && f.map(0,2).prevTransformIndex==2);
        flags(f,1,true,!firstSkip);flags(f,2,true,firstSkip);
        finish(f,firstSkip?"N3b-skip-to-interpolate":"N3b-interpolate-to-skip",2,2);
    }
    {
        OwnerFixture f;grow(f,0,1,G_EX_ID_AUTO);grow(f,0,2,G_EX_ID_AUTO);grow(f,2,1,G_EX_ID_AUTO);
        skip(f,0,1);group(f,0,1).decompose=false;
        group(f,0,2).positionInterpolation=group(f,0,2).rotationInterpolation=
            group(f,0,2).scaleInterpolation=group(f,0,2).skewInterpolation=
            group(f,0,2).perspectiveInterpolation=G_EX_COMPONENT_AUTO;
        scene(f,2,1);f.match();
        assert(f.map(0,2).mapped && f.map(0,2).prevTransformIndex==1);
        flags(f,1,false,true);flags(f,2,false,true);finish(f,"N4-AUTO-normalization",1,2);
    }
    for(bool autoFirst:{false,true}) {
        OwnerFixture f;uint32_t first=autoFirst?G_EX_ID_AUTO:stableId,second=autoFirst?stableId:G_EX_ID_AUTO;
        grow(f,0,1,first);grow(f,0,2,second);grow(f,2,1,first);scene(f,2,1);f.match();
        assert(f.map(0,1).mapped && !f.map(0,2).mapped);
        finish(f,autoFirst?"N4-AUTO-to-explicit":"N4-explicit-to-AUTO",1,2);
    }
    {
        OwnerFixture f;grow(f,0,1,stableId);grow(f,0,2,stableId);grow(f,2,1,stableId);
        auto &g=group(f,0,2);g.aspectMode=G_EX_ASPECT_ADJUST;g.ordering=G_EX_ORDER_AUTO;
        g.vertexInterpolation=g.texcoordInterpolation=G_EX_COMPONENT_INTERPOLATE;
        g.tileInterpolation=g.lookAtInterpolation=G_EX_COMPONENT_SKIP;g.editable=G_EX_EDIT_ALLOW;
        scene(f,2,1);f.match();assert(f.map(0,2).mapped && f.map(0,2).prevTransformIndex==1);
        flags(f,2,true,true);finish(f,"N5-excluded-fields",1,2);
    }
    {
        OwnerFixture f;grow(f,0,1,stableId);grow(f,0,2,cutId);
        grow(f,2,1,stableId);grow(f,3,2,cutId);f.set();
        assert(f.current.perspectiveScenes.size()==1 && f.previous.perspectiveScenes.size()==1);
        member(f.previous.perspectiveScenes[0],0,2,0);member(f.previous.perspectiveScenes[0],1,3,0);
        f.match();assert(f.map(0,1).mapped && !f.map(0,2).mapped);
        finish(f,"N6-foreign-previous-fallback",2,2);
    }
    {
        OwnerFixture f;grow(f,0,1,stableId);grow(f,0,2,stableId);grow(f,1,2,G_EX_ID_IGNORE);
        grow(f,2,1,stableId);f.set();
        assert(f.current.perspectiveScenes.size()==1 && f.previous.perspectiveScenes.size()==1);
        member(f.current.perspectiveScenes[0],0,0,0);member(f.current.perspectiveScenes[0],1,0,1);
        member(f.current.perspectiveScenes[0],2,1,0);
        f.match();assert(f.map(0,2).mapped && f.map(0,2).prevTransformIndex==1);
        finish(f,"N7-foreign-IGNORE-owner-first",1,2);
    }
}
}
int main() {
    for (uint8_t order : {uint8_t(G_EX_ORDER_LINEAR),uint8_t(G_EX_ORDER_AUTO)}) {
        for (unsigned count : {1u,2u}) for (float x : {0.0f,0.0000001f}) {
            later(order,count,x,stableId,"same-id");
            later(order,count,x,cutId,"later-cut");
            later(order,count,x,G_EX_ID_IGNORE,"later-ignore");
        }
        controls(order);
    }
    mandatory();
    assert(sessionRows == 46);
    std::printf("CAMERA_MATCH complete rows=%u actual_GameFrame_set_match=1 retained_toy_main_called=0\n",sessionRows);
    return 0;
}
