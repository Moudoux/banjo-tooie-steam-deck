// Included after baseline_skeletal_cases.inc inside the draw fixture's anonymous namespace.
// N1 feather cut policy: the actual extracted tooie_actor_feather_leaf_id, reached through the
// actual extracted func_800DD024 emitter. Model, node and list words are synthetic 6EA-shaped
// witnesses, never game assets. The original type-three leaf list itself is not executed.
constexpr uint32_t feather_model=0x80640000u,feather_sentinel=0xA55A1234u;
#ifndef TOOIE_BK_DRAW_FIXTURE
constexpr uint32_t feather_route=0x80000002u; // the BK-OFF helper's palette-slot route (actor_palette.c)
#endif
// Public gEXMatrixGroup word2 fields: push bit0, proj bit1, mode bit2, then two-bit components
// pos3 rot5 scale7 skew9 persp11 vert13 tile15, order17, edit19, aspect20, tc22, lookat24.
constexpr uint32_t feather_live=1u|(1u<<3)|(1u<<5)|(1u<<7)|(1u<<9)|(1u<<11)|(2u<<15)|(2u<<24);
constexpr uint32_t feather_cut=1u; // PUSH, MODELVIEW, SIMPLE and every component SKIP.
#ifdef TOOIE_BK_DRAW_FIXTURE
// L06b-8: the BK image builds no feather policy helper; the selected DD024 uses the BK list node alone.
constexpr uint32_t feather_bk_base=0x20000000u;
constexpr uint32_t feather_bk_live=feather_live-(1u<<15); // BK tile INTERPOLATE, not AUTO.
static_assert(feather_bk_live==0x02008AA9u,"BK simple non-map policy from public fields");
static_assert(TOOIE_BK_DD024_FRAME_BYTES>0 && TOOIE_BK_DD024_FRAME_BYTES<guard,
    "receipt-derived DD024/list/group chain fits the fixture stack guard");
#endif
static_assert(feather_live==0x02010AA9u && feather_cut==0x00000001u,
    "feather policies derived from the public group field positions");
struct FeatherFixture {
    SkeletalFixture t;
    uint32_t cursor=player_cursor;
    FeatherFixture(){prepare(1);}
    uint32_t node(uint32_t role) const {return feather_model+17552u+(role-1u)*56u;}
    uint32_t list(uint32_t role) const {return (feather_model+12976u+(63u+(role-1u)*18u)*8u+8u)&0x1FFFFFFFu;}
    uint32_t field(size_t offset) const {return t.field(offset);}
    uint32_t slot(size_t offset) const {return word(t.f.rdram,t.rec()+uint32_t(offset));}
    uint32_t id(uint32_t view,uint32_t phase,uint32_t role) const {
        return TOOIE_BRIDGE_ID_BASE+(slot(offsetof(TooieActorPaletteSlot,block_and_valid))&0x7FFFFu)*512u+
            view*128u+phase*16u+role;
    }
    // The existing skeletal camera/epoch witness, plus the feather route's original premises.
    void prepare(uint32_t epoch,uint32_t view=0,uint32_t phase=0,uint32_t camera=0xA0000001u) {
        t.prepare(epoch,view,phase,camera);
        auto* r=t.f.rdram;auto* rdram=r;
        constexpr uint32_t cs=TOOIE_CAMERA_STATE_VRAM;
        put(r,0x8012CA18,0x6EA);put(r,0x8012C960,feather_model+12976u);
        put(r,feather_model+4,17120u);MEM_H(10,guest(feather_model))=0;
        put(r,feather_model+12,12976u);put(r,feather_model+0x18,15792u);put(r,feather_model+0x28,0);
        for(uint32_t role=1;role<=3;++role){
            put(r,node(role),3);put(r,node(role)+4,0);MEM_H(8,guest(node(role)))=int16_t(63u+(role-1u)*18u);
        }
        for(const auto& entry:std::array<std::array<uint32_t,2>,5>{{{0,0x800DC634},{1,0x800DC958},
                {2,0x800DCF48},{3,0x800DD024},{12,0x800DD56C}}})
            put(r,player_dispatch+entry[0]*4,entry[1]);
        for(uint32_t i=1;i<=8;++i)put(r,0x8012C850+i*4,i==2);
        for(uint32_t a:{0x8012C820u,0x8012C838u,0x8012CEC4u,0x8012C980u,0x8012C9B0u,0x8012C9B8u,0x8012CA28u})put(r,a,0);
        // DD024's own gate: an open camera construction.
        put(r,cs+offsetof(TooieCameraState,owner_frame),0x80F10000u);
        put(r,cs+offsetof(TooieCameraState,control)+offsetof(TooieCameraControl,active),1);
    }
    // One actual DD024 call at the live cursor. Returns the emitted group ID, or 0 when only the
    // original leaf DE was written. Checks every owned word, the cursor and last_end either way.
    uint32_t emit(uint32_t role,uint32_t policy) {
#ifdef TOOIE_BK_DRAW_FIXTURE
        auto* r=t.f.rdram;
        const uint32_t at=cursor,end=field(offsetof(TooieRenderBridgeState,last_end));
        put(r,player_buffers,at);
        // L06b-8: BK's DD024 calls no feather helper, and the BK image builds none.
        const uint32_t extra=field(offsetof(TooieRenderBridgeState,extra_bytes));
        const uint32_t admissions=field(offsetof(TooieRenderBridgeState,silo_admissions));
        const uint32_t refusals=field(offsetof(TooieRenderBridgeState,silo_refusals));
        // Explicit owner/cut inputs to the real list helpers; this is not a camera-adapter run.
        constexpr uint32_t bs=TOOIE_BK_STATE;
        put(r,bs+offsetof(TooieBkState,cur_drawn_model_transform_id),feather_bk_base);
        put(r,bs+offsetof(TooieBkState,cur_model_transform_id_offset),role);
        put(r,bs+offsetof(TooieBkState,cur_model_uses_bones),1);
        put(r,bs+offsetof(TooieBkState,skip_all_interpolation),policy==feather_cut);
        put(r,bs+offsetof(TooieBkState,cur_drawn_model_skip_interpolation),0);
        put(r,bs+offsetof(TooieBkState,root_depth),0);
        constexpr uint32_t groups=bs+offsetof(TooieBkState,diagnostics)+offsetof(TooieBkDiagnostics,groups);
        const uint32_t old_groups=word(r,groups);
        for(uint32_t a=at;a<at+64;a+=4)put(r,a,feather_sentinel);
        for(uint32_t a=stack-guard;a<stack+32;++a)put_byte(r,a,0xA5);
        auto c=context();c.r4=guest(player_buffers);c.r5=guest(node(role));
        const auto original=c;candidate::func_800DD024(r,&c);saved(c,original);
        for(uint32_t a=stack-guard;a<stack-TOOIE_BK_DD024_FRAME_BYTES;++a)
            require(byte(r,a)==0xA5,"BK DD024/list/group chain stays within measured frames");
        for(uint32_t a=stack+16;a<stack+32;++a)require(byte(r,a)==0xA5,"BK DD024 preserves above its argument homes");
        require(word(r,at)==0x6400000Cu && word(r,at+4)==feather_bk_base+role &&
            word(r,at+8)==(policy==feather_cut?feather_cut:feather_bk_live) && !word(r,at+12),
            "actual BK explicit-ID group and live/SkipAll policy");
        require(word(r,at+16)==0xDA380000u && word(r,at+20)==TOOIE_BK_FLOAT,
            "actual BK identity MUL/PUSH uses the linked immutable identity");
        require(word(r,at+24)==0xDE000000u && word(r,at+28)==list(role),
            "BK wrapper preserves original leaf DE and physical list");
        require(word(r,at+32)==0xD8380002u && word(r,at+36)==64u &&
            word(r,at+40)==0x6400000Du && word(r,at+44)==1u,
            "BK matrix pop then group pop are balanced");
        require(word(r,player_buffers)==at+48 && word(r,groups)==old_groups+1 &&
            word(r,bs+offsetof(TooieBkState,cur_model_transform_id_offset))==role,
            "BK list consumes48 bytes and one group without advancing traversal offset");
        for(uint32_t a=at+48;a<at+64;a+=4)require(word(r,a)==feather_sentinel,"BK DD024 writes exactly its wrapper");
        require(field(offsetof(TooieRenderBridgeState,last_end))==end &&
            field(offsetof(TooieRenderBridgeState,extra_bytes))==extra &&
            field(offsetof(TooieRenderBridgeState,silo_admissions))==admissions &&
            field(offsetof(TooieRenderBridgeState,silo_refusals))==refusals,
            "BK DD024 invokes and debits no row helper");
        cursor=at+48;return word(r,at+4);
#else
        auto* r=t.f.rdram;
        const uint32_t at=cursor,end=field(offsetof(TooieRenderBridgeState,last_end));
        for(uint32_t a=at;a<at+48;a+=4)put(r,a,feather_sentinel);
        put(r,player_buffers,at);
        for(uint32_t a=stack-guard;a<stack+32;++a)put_byte(r,a,0xA5);
        auto c=context();c.r4=guest(player_buffers);c.r5=guest(node(role));
        const auto before=c;candidate::func_800DD024(r,&c);saved(c,before);
        for(uint32_t a=stack-guard;a<stack-guard+64;++a)
            require(byte(r,a)==0xA5,"DD024 and its nested feather helper stay inside the fixture guard");
        for(uint32_t a=stack+16;a<stack+32;++a)require(byte(r,a)==0xA5,"DD024 preserves above its o32 argument homes");
        if(!policy){
            require(word(r,at)==0xDE000000u && word(r,at+4)==list(role) && word(r,player_buffers)==at+8,
                "refused feather keeps exactly the original leaf DE");
            for(uint32_t a=at+8;a<at+48;a+=4)require(word(r,a)==feather_sentinel,"refused feather emits no group or pop");
            require(field(offsetof(TooieRenderBridgeState,last_end))==end,"refused feather leaves last_end");
            cursor=at+8;return 0;
        }
        require(word(r,at)==0xE0525464u && word(r,at+4)==0x10000064u && word(r,at+8)==0x6400000Cu &&
            word(r,at+16)==policy && !word(r,at+20),"exact Enable and explicit-ID MODELVIEW group policy");
        require(word(r,at+24)==0xDE000000u && word(r,at+28)==list(role),"original leaf DE follows the 24-byte prefix");
        require(word(r,at+32)==0x6400000Du && word(r,at+36)==1u && word(r,at+40)==feather_sentinel,
            "exactly one paid MODELVIEW pop");
        require(word(r,player_buffers)==at+40 && field(offsetof(TooieRenderBridgeState,last_end))==at+40,
            "cursor and last_end follow the paid pop");
        cursor=at+40;return word(r,at+12);
#endif
    }
};

#ifdef TOOIE_BK_DRAW_FIXTURE
// Cross-list modelview stack (post44 save-stack finding). A header-bit40 root issues no modelview
// matrix of its own, and its type3 lists own the stack across siblings. Shape of the three
// save-select frame graphs: list0 LOAD/PUSHes a matrix, a selector-branch sibling list draws under
// it, a third list pops it. The actual DD024 -> list helpers emit every node; only the original
// list bodies are synthetic symbolic words (no asset bytes). The emitted stream and the unwrapped
// original stream then run on a model of RT64's modelview and matrix-group stacks (rt64_rsp.cpp
// matrixCommon modelview branch, popMatrix, matrixId/popMatrixId, setVertexCommon): the two are
// separate stacks, and a vertex after a matrix change takes the top value and the current group.
namespace cross_list {
constexpr uint32_t parent=0x0E000000u,bone=0x05000000u; // symbolic: enclosing top; segment5 offset0 LOAD
constexpr uint32_t vtx=0x01004008u;                       // F3DEX2 G_VTX word0
constexpr uint32_t owner=TOOIE_BK_ACTOR_ID_START+0x7000u; // synthetic actor-domain draw ID, offset 0
constexpr uint32_t bit40=2u,grouped=1u;                   // TooieBkRootFrame.pushed_group (bk_pipeline.c)
struct Cmd{uint32_t w0,w1;};
using Body=std::vector<Cmd>;
using Lists=std::vector<std::pair<uint32_t,const Body*>>;
struct Vertex{uint32_t matrix,group;};
struct Machine{
    const Lists& lists;
    std::vector<uint32_t> stack{parent},groups{0u};
    std::vector<Vertex> vertices;
    bool changed=true;Vertex current{};
    explicit Machine(const Lists& l):lists(l){}
    void run(const Body& cmds){
        for(size_t i=0;i<cmds.size();++i){
            const Cmd c=cmds[i];
            if(c.w0==0x6400000Cu){
                require(i+1<cmds.size(),"group carries its policy word");
                const uint32_t policy=cmds[++i].w0;
                require(!(policy&2u),"modelview group only");
                if(policy&1u)groups.push_back(c.w1);else groups.back()=c.w1;
            }else if(c.w0==0x6400000Du){
                require(c.w1==1u,"one modelview group pop");
                if(groups.size()>1)groups.pop_back();
            }else if((c.w0&0xFFFFFF00u)==0xDA380000u){
                const uint32_t params=(c.w0&0xFFu)^1u; // F3DEX2 stores params ^ G_MTX_PUSH
                require(!(params&4u),"modelview matrix only");
                uint32_t value=stack.back();
                if(params&2u)value=c.w1;else require(c.w1==TOOIE_BK_FLOAT,"MUL only by the linked identity");
                if(params&1u)stack.push_back(value);else stack.back()=value;
                changed=true;
            }else if(c.w0==0xD8380002u){
                for(uint32_t n=c.w1/64u;n--;)if(stack.size()>1)stack.pop_back();
                changed=true;
            }else if(c.w0==0xDE000000u){
                const Body* body=nullptr;
                for(const auto& entry:lists)if(entry.first==c.w1)body=entry.second;
                require(body!=nullptr,"DE names a modelled original list");
                run(*body);
            }else if(c.w0==vtx){
                if(changed){current={stack.back(),groups.back()};changed=false;}
                vertices.push_back(current);
            }else throw std::runtime_error("unmodelled command in cross-list stream");
        }
    }
};
Machine execute(const Body& stream,const Lists& lists){Machine m(lists);m.run(stream);return m;}
// Pose equivalence: the same matrix for every vertex in order, and the same final stack.
bool same_pose(const Machine& a,const Machine& b){
    if(a.vertices.size()!=b.vertices.size() || a.stack!=b.stack)return false;
    for(size_t i=0;i<a.vertices.size();++i)if(a.vertices[i].matrix!=b.vertices[i].matrix)return false;
    return true;
}
enum class Shape{Kept,Bracket,Plain};

unsigned fixtures(FeatherFixture& f){
    unsigned cases=0;
    auto* r=f.t.f.rdram;
    constexpr uint32_t bs=TOOIE_BK_STATE;
    constexpr uint32_t groups_diag=bs+offsetof(TooieBkState,diagnostics)+offsetof(TooieBkDiagnostics,groups);
    const auto frame=[&](uint32_t i){
        return bs+uint32_t(offsetof(TooieBkState,roots))+i*uint32_t(sizeof(TooieBkRootFrame))+
            uint32_t(offsetof(TooieBkRootFrame,pushed_group));
    };
    // One actual DD024 call for node(role) under the given recorded root frames; returns its commands.
    const auto dd024=[&](uint32_t role,uint32_t depth,const std::vector<uint32_t>& frames,uint32_t id){
        for(uint32_t i=0;i<TOOIE_BK_ROOT_DEPTH;++i)put(r,frame(i),i<frames.size()?frames[i]:0u);
        put(r,bs+offsetof(TooieBkState,root_depth),depth);
        put(r,bs+offsetof(TooieBkState,cur_drawn_model_transform_id),id);
        put(r,bs+offsetof(TooieBkState,cur_model_transform_id_offset),0);   // every root resets it
        put(r,bs+offsetof(TooieBkState,cur_model_uses_bones),0);
        put(r,bs+offsetof(TooieBkState,skip_all_interpolation),0);
        put(r,bs+offsetof(TooieBkState,cur_drawn_model_skip_interpolation),0);
        const uint32_t at=f.cursor,groups=word(r,groups_diag);
        put(r,player_buffers,at);
        for(uint32_t a=at;a<at+64;a+=4)put(r,a,feather_sentinel);
        for(uint32_t a=stack-guard;a<stack+32;++a)put_byte(r,a,0xA5);
        auto c=context();c.r4=guest(player_buffers);c.r5=guest(f.node(role));
        const auto original=c;candidate::func_800DD024(r,&c);saved(c,original);
        for(uint32_t a=stack-guard;a<stack-TOOIE_BK_DD024_FRAME_BYTES;++a)
            require(byte(r,a)==0xA5,"cross-list DD024/list/group chain stays within measured frames");
        for(uint32_t a=stack+16;a<stack+32;++a)require(byte(r,a)==0xA5,"cross-list DD024 preserves above its argument homes");
        const uint32_t end=word(r,player_buffers);
        require(end>at && end<=at+48 && !((end-at)&7u),"one bounded DD024 wrapper");
        for(uint32_t a=end;a<at+64;a+=4)require(word(r,a)==feather_sentinel,"cross-list DD024 writes exactly its wrapper");
        require(word(r,groups_diag)==groups+(id?1u:0u),"one group per tagged list");
        require(word(r,bs+offsetof(TooieBkState,root_depth))==depth &&
            word(r,bs+offsetof(TooieBkState,cur_model_transform_id_offset))==0,"list helpers keep root depth and traversal offset");
        for(uint32_t i=0;i<TOOIE_BK_ROOT_DEPTH;++i)
            require(word(r,frame(i))==(i<frames.size()?frames[i]:0u),"list helpers never write root frames");
        Body out;for(uint32_t a=at;a<end;a+=8)out.push_back({word(r,a),word(r,a+4)});
        f.cursor=end;return out;
    };
    // Exact emitted words per shape: Kept = group, identity MUL/NOPUSH, DE, group pop (40 bytes);
    // Bracket = BK func_80338904 group, identity MUL/PUSH, DE, matrix pop, group pop (48 bytes);
    // Plain = no ID and no skip, so only the original DE.
    const auto shape=[&](const Body& w,uint32_t role,uint32_t id,Shape s){
        if(s==Shape::Plain){
            require(w.size()==1 && w[0].w0==0xDE000000u && w[0].w1==f.list(role),"untagged list keeps exactly its original DE");
            return;
        }
        const bool nopush=s==Shape::Kept;
        require(w.size()==(nopush?5u:6u) && w[0].w0==0x6400000Cu && w[0].w1==id && (w[1].w0&1u) && !w[1].w1,
            "explicit-ID MODELVIEW group push");
        require(w[2].w0==(nopush?0xDA380001u:0xDA380000u) && w[2].w1==TOOIE_BK_FLOAT,
            nopush?"bit40 root: identity MUL/NOPUSH":"BK identity MUL/PUSH");
        require(w[3].w0==0xDE000000u && w[3].w1==f.list(role),"original DE and physical list");
        if(!nopush)require(w[4].w0==0xD8380002u && w[4].w1==64u,"BK one-matrix pop");
        require(w.back().w0==0x6400000Du && w.back().w1==1u,"group pop closes the list");
    };
    // Emits every node through the actual helpers, checks pose equivalence against `expect` first
    // (so the pre-fix bracket fails there), then the exact words of each wrapper.
    const auto graph=[&](uint32_t depth,const std::vector<uint32_t>& frames,Shape s,uint32_t id,const Lists& lists,
            uint32_t roles,const Machine* expect=nullptr,const char* why=""){
        f.cursor=player_cursor;Body stream;std::vector<Body> emitted;
        for(uint32_t role=1;role<=roles;++role){
            emitted.push_back(dd024(role,depth,frames,id));
            stream.insert(stream.end(),emitted.back().begin(),emitted.back().end());
        }
        Machine m=execute(stream,lists);
        if(expect)require(same_pose(m,*expect),why);
        for(uint32_t role=1;role<=roles;++role)shape(emitted[role-1u],role,id,s);
        return m;
    };
    const Body push_bone{{0xDA380002u,bone},{vtx,0x06000000u}};                      // list0: LOAD/PUSH, own vertices
    const Body selected{{vtx,0x06000100u}};                                          // selector branch: vertices only
    const Body pop_bone{{vtx,0x06000200u},{0xD8380002u,64u}};                        // list69: one-matrix pop
    const Body balanced{{0xDA380002u,bone},{vtx,0x06000300u},{0xD8380002u,64u}};     // stack-neutral list
    const Lists cross{{f.list(1),&push_bone},{f.list(2),&selected},{f.list(3),&pop_bone}};
    const Lists neutral{{f.list(1),&balanced}};
    const Machine original=execute({{0xDE000000u,f.list(1)},{0xDE000000u,f.list(2)},{0xDE000000u,f.list(3)}},cross);
    require(original.vertices.size()==3 && original.vertices[0].matrix==bone && original.vertices[1].matrix==bone &&
        original.vertices[2].matrix==bone && original.stack==std::vector<uint32_t>{parent},
        "original: sibling and selector-branch vertices draw under list0's pushed matrix; list69 restores the parent");
    // X1: innermost recorded root is bit40: every vertex keeps the original matrix, and every list is tagged.
    const Machine kept=graph(1,{bit40},Shape::Kept,owner,cross,3,&original,
        "bit40 root: cross-list matrix reaches the selector branch and the stack ends as the original");
    for(const Vertex& v:kept.vertices)require(v.group==owner,"bit40 root: every list vertex carries its BK group");
    ++cases;
    // X2: the pre-fix stream. With no open bit40 frame (the closed one left in roots[0] is never read) the
    // BK bracket is emitted; it restores depth but drops list0's matrix before the selector branch.
    const Machine bracket=graph(0,{bit40},Shape::Bracket,owner,cross,3);
    require(!same_pose(bracket,original) && bracket.vertices[0].matrix==bone && bracket.vertices[1].matrix==parent &&
        bracket.stack==original.stack,"BK per-list PUSH/pop reproduces the cross-list loss: depth restored, pose not");
    ++cases;
    // X3: a nested ordinary root inside a bit40 root is its own innermost frame: BK bracket unchanged.
    graph(2,{bit40,grouped},Shape::Bracket,owner,cross,3);++cases;
    // X4: a bit40 root nested inside an ordinary root keeps the original stack.
    graph(2,{grouped,bit40},Shape::Kept,owner,cross,3,&original,"nested bit40 root keeps the cross-list matrix");
    ++cases;
    // X5: an overflow root records no frame; the existing BK bracket is unchanged.
    graph(TOOIE_BK_ROOT_DEPTH+1u,{0,0,0,0,0,0,0,bit40},Shape::Bracket,owner,cross,3);++cases;
    // X6: a stack-neutral list renders the same matrix under either close, with the same group.
    const Machine neutral_original=execute({{0xDE000000u,f.list(1)}},neutral);
    for(const auto& form:std::array<std::pair<uint32_t,Shape>,2>{{{bit40,Shape::Kept},{grouped,Shape::Bracket}}}){
        const Machine m=graph(1,{form.first},form.second,owner,neutral,1,&neutral_original,
            "stack-neutral list: same pose under the bit40 and ordinary closes");
        require(m.vertices.size()==1 && m.vertices[0].matrix==bone && m.vertices[0].group==owner,
            "stack-neutral list: same matrix and group under either close");
    }
    ++cases;
    // X7: an untagged bit40 list (no ID, no skip) keeps exactly its original DE and pose.
    graph(1,{bit40},Shape::Plain,0u,cross,3,&original,"untagged bit40 lists are the original stream");
    ++cases;
    put(r,bs+offsetof(TooieBkState,root_depth),0);
    for(uint32_t i=0;i<TOOIE_BK_ROOT_DEPTH;++i)put(r,frame(i),0);
    f.cursor=player_cursor;
    return cases;
}
}
#endif

unsigned feather_cut_fixtures() {
    unsigned cases=0;
    FeatherFixture f;auto* r=f.t.f.rdram;
#ifdef TOOIE_BK_DRAW_FIXTURE
    // L06b-8: BK's DD024 reads only the 6EA node, the list base and the BK owner/cut words emit() writes
    // (bk_graph.inc, bk_pipeline.c), so B1-B3 run the actual DD024 -> BK list helpers on every distinct
    // input the former BK F1-F10 calls used. emit() checks every emitted word, frame and bridge counter.
    // B1: each role, live.
    for(uint32_t role=1;role<=3;++role)f.emit(role,feather_live);
    ++cases;
    // B2: each role, SkipAll under the same explicit ID.
    for(uint32_t role=1;role<=3;++role)f.emit(role,feather_cut);
    ++cases;
    // B3 (former BK F7): later SkipAll groups never rewrite an earlier live group.
    const uint32_t earlier=f.cursor;
    f.emit(1,feather_live);f.emit(2,feather_cut);f.emit(3,feather_cut);
    require(word(r,earlier+4)==feather_bk_base+1 && word(r,earlier+8)==feather_bk_live,
        "earlier BK group stays live after later explicit cuts");++cases;
#else
    const auto extra=[&](){return f.field(offsetof(TooieRenderBridgeState,extra_bytes));};
    const auto reason=[&](){return f.field(offsetof(TooieRenderBridgeState,silo_last_reason));};
    const auto cut=[&](){return f.slot(offsetof(TooieActorPaletteSlot,camera_cut_views));};
    const auto camera=[&](uint32_t view){return f.slot(offsetof(TooieActorPaletteSlot,camera_id)+view*4u);};
    uint32_t got=0;
    // F1: stable camera; every role admits live, one lifetime block, 32 bytes per role.
    const uint32_t blocks=f.field(offsetof(TooieRenderBridgeState,next_block));
    for(uint32_t role=1;role<=3;++role){
        got=f.emit(role,feather_live);require(got==f.id(0,0,role),"stable feather roles admit live under their logical IDs");
    }
    require(f.field(offsetof(TooieRenderBridgeState,next_block))==blocks+1 &&
        f.slot(offsetof(TooieActorPaletteSlot,flags))==feather_route && extra()==96 && !cut() && camera(0)==0xA0000001u,
        "one feather lifetime, 32-byte debit per role, no cut");
    const uint32_t first=f.id(0,0,1);++cases;
    // F2: a view0 camera-ID change keeps the same ID, skips every component, pays 32 and pops once.
    f.prepare(2,0,0,0xA0000002u);
    const uint32_t admissions=f.field(offsetof(TooieRenderBridgeState,silo_admissions));
    const uint32_t refusals=f.field(offsetof(TooieRenderBridgeState,silo_refusals));
    got=f.emit(1,feather_cut);
    require(got==first,"cut occurrence keeps its actual logical ID, not ID_IGNORE");
    require(cut()==1u && camera(0)==0xA0000002u && extra()==32 && f.field(offsetof(TooieRenderBridgeState,next_block))==blocks+1 &&
        f.field(offsetof(TooieRenderBridgeState,silo_admissions))==admissions+1 &&
        f.field(offsetof(TooieRenderBridgeState,silo_refusals))==refusals,
        "cut is an admission: sticky view bit, camera update, ordinary debit, no renewal or refusal");++cases;
    // F3: another admissible role in the same view and epoch also skips.
    got=f.emit(2,feather_cut);require(got==first+1 && extra()==64,"second role in the cut view/epoch skips under its ID");++cases;
    // F4: a duplicate role still refuses and falls back to the original leaf DE alone.
    got=f.emit(1,0);require(!got && reason()==TOOIE_BRIDGE_DUPLICATE && extra()==64,"duplicate stays refused without debit");++cases;
    // F5: another view in the cut epoch stays live.
    f.prepare(2,1,0,0xA0000003u);
    got=f.emit(3,feather_live);
    require(got==f.id(1,0,3) && cut()==1u && camera(1)==0xA0000003u && extra()==96,
        "first view1 occurrence is live while view0 is cut");++cases;
    // F6: the next epoch with a stable view0 camera returns live under the same ID.
    f.prepare(3,0,0,0xA0000002u);
    got=f.emit(1,feather_live);require(got==first && !cut() && extra()==32,"stable next epoch interpolates under the same ID");++cases;
    // F7: a mid-epoch same-view change is sticky for later roles and never rewrites earlier groups.
    f.prepare(4,0,0,0xA0000002u);
    const uint32_t earlier=f.cursor;
    got=f.emit(1,feather_live);require(got==first,"pre-change occurrence is live");
    f.prepare(4,0,0,0xA0000004u);
    got=f.emit(2,feather_cut);require(got==first+1,"post-change role skips");
    got=f.emit(3,feather_cut);require(got==first+2 && cut()==1u,"later role in that epoch stays skipped");
    require(word(r,earlier+12)==first && word(r,earlier+16)==feather_live,"earlier group stays live");++cases;
    // F8: the following stable epoch is live again.
    f.prepare(5,0,0,0xA0000004u);
    got=f.emit(2,feather_live);require(got==first+1 && !cut(),"post-cut stable epoch interpolates");++cases;
    // F9: a cut admission uses the ordinary ledger; an exhausted ledger still refuses and falls back.
    f.prepare(6,0,0,0xA0000005u);
    put(r,state+offsetof(TooieRenderBridgeState,seen_epoch),6);put(r,state+offsetof(TooieRenderBridgeState,extra_bytes),6368);
    got=f.emit(1,feather_cut);require(got==first && extra()==6400,"the last 32 ledger bytes pay a cut occurrence");
    got=f.emit(2,0);require(!got && reason()==TOOIE_BRIDGE_CAPACITY && extra()==6400,"exhausted ledger still refuses the next role");
    ++cases;
    // F10: a premise refusal precedes the cut latch; once the premise holds, the same epoch skips.
    f.prepare(7,1,0,0xA0000003u);
    got=f.emit(1,feather_live);require(got==f.id(1,0,1),"view1 stays live and opens epoch7");
    f.prepare(7,0,0,0xA0000006u);put(r,0x8012C850+4,1);
    got=f.emit(1,0);require(!got && reason()==TOOIE_BRIDGE_SELECTOR && camera(0)==0xA0000005u && !cut(),
        "selector refusal leaves the view0 camera unlatched");
    put(r,0x8012C850+4,0);
    got=f.emit(1,feather_cut);require(got==first && cut()==1u && camera(0)==0xA0000006u,"same-epoch admissible occurrence latches and skips");
    ++cases;
#endif
#ifdef TOOIE_BK_DRAW_FIXTURE
    // The bit40 cross-list stack group runs on the same fixture after B1-B3; its count is separate.
    const unsigned stack_cases=cross_list::fixtures(f);
    require(stack_cases==7,"all bit40 cross-list stack cases ran");
    std::cout<<"BK cross-list stack compiled cases="<<stack_cases
        <<"; DD024+list_helpers=ACTUAL; list_bodies=SYMBOLIC; RT64_stacks=MODELED\n";
#endif
    return cases;
}
