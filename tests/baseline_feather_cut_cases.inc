// Included after baseline_skeletal_cases.inc inside the draw fixture's anonymous namespace.
// N1 feather cut policy: the actual extracted tooie_actor_feather_leaf_id, reached through the
// actual extracted func_800DD024 emitter. Model, node and list words are synthetic 6EA-shaped
// witnesses, never game assets. The original type-three leaf list itself is not executed.
constexpr uint32_t feather_model=0x80640000u,feather_route=0x80000002u,feather_sentinel=0xA55A1234u;
// Public gEXMatrixGroup word2 fields: push bit0, proj bit1, mode bit2, then two-bit components
// pos3 rot5 scale7 skew9 persp11 vert13 tile15, order17, edit19, aspect20, tc22, lookat24.
constexpr uint32_t feather_live=1u|(1u<<3)|(1u<<5)|(1u<<7)|(1u<<9)|(1u<<11)|(2u<<15)|(2u<<24);
constexpr uint32_t feather_cut=1u; // PUSH, MODELVIEW, SIMPLE and every component SKIP.
#ifdef TOOIE_BK_DRAW_FIXTURE
// The old policy helper remains under direct control; the selected DD024 uses BK instead.
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
#ifdef TOOIE_BK_DRAW_FIXTURE
    uint32_t current_camera=0;
#endif
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
#ifdef TOOIE_BK_DRAW_FIXTURE
        current_camera=camera;
#endif
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
        // Preserve all F1-F10 helper controls as actual calls. BK's DD024 no longer calls it.
        const uint32_t retained_id=t.invoke(candidate::tooie_actor_feather_leaf_id,
            player_buffers,node(role),current_camera);
        require(bool(retained_id)==bool(policy),"retained feather helper admission/refusal control");
        require(word(r,player_buffers)==at && field(offsetof(TooieRenderBridgeState,last_end))==end,
            "retained policy helper emits no commands or paid close");
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
            "BK DD024 does not invoke or debit the retained row helper a second time");
        cursor=at+48;return retained_id;
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

unsigned feather_cut_fixtures() {
    unsigned cases=0;
    FeatherFixture f;auto* r=f.t.f.rdram;
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
#ifdef TOOIE_BK_DRAW_FIXTURE
    require(word(r,earlier+4)==feather_bk_base+1 && word(r,earlier+8)==feather_bk_live,
        "earlier BK group stays live after later explicit cuts");++cases;
#else
    require(word(r,earlier+12)==first && word(r,earlier+16)==feather_live,"earlier group stays live");++cases;
#endif
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
    return cases;
}
