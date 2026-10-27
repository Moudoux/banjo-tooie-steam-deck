// Included inside the existing draw fixture's anonymous namespace after PaletteFixture.
// All model/pose bytes below are synthetic scalar witnesses, never copied game assets.
#ifndef TOOIE_SKELETAL_MAX_FRAME_BYTES
#error Supply the measured maximum nested guest frame extent for these extractions
#endif
static_assert(TOOIE_SKELETAL_MAX_FRAME_BYTES<guard,"skeletal fixture stack guard");
static_assert(sizeof(TooieNpc665Slot)==72 && offsetof(TooieNpc665Slot,skeletal_limits)==60,
    "named skeletal word preserves the resident slot ABI");
constexpr uint32_t skeletal_route=0x80000004u,npc_route=0x80000003u;
constexpr uint32_t npc_idle=0x66500000u,npc_active=2u,npc_failed=4u;
#if TOOIE_NPC665_SECTION_PRESENT
constexpr uint32_t ns=TOOIE_NPC665_STATE;
#else
// L06c-2: NPC665 stays selected while the BK image links no sidecar; every group reading it is B4.
static_assert(TOOIE_NPC665_SELECTED==1u,"L06c-2: selection kept with the BK section absent");
#ifndef TOOIE_BK_B4_FIXTURE
#error NPC665 skeletal groups need the linked NPC665 sidecar
#endif
#endif
struct SkeletalRow { uint32_t resource,graph,lists,vertices,attributes,bytes,poses,bones,hybrids; };
// Rows 4..8 mirror the guest census rows (execution-facts01), rows 9..14 the WP7 shared ordinary
// rows (execution-facts21/25): header tuple, extent, poses and executed DCF48/type24 bounds. Model
// bytes stay synthetic scalars.
constexpr std::array<SkeletalRow,14> skeletal_rows={{{0x707,41408,6304,21024,16392,43280,23,23,18},
    {0x671,16536,760,6832,6600,17656,14,14,13},
    {0x90C,41672,6352,21488,16824,43560,23,23,18},
    {0x616,4616,192,1712,1592,5016,7,7,4},
    {0x617,6032,912,3248,3160,6272,5,5,2},
    {0x627,784,384,616,592,836,1,1,0},
    {0x651,6040,2176,4128,4008,6292,7,7,0},
    {0x656,5152,2176,4296,4208,5340,5,5,0},
    {0x92A,9432,4264,7024,6840,9804,11,11,0},
    {0x676,13528,760,5064,4784,14424,17,17,6},
    {0x88F,22752,4864,10104,9792,23512,19,19,0},
    {0x7F8,24256,4192,9432,9296,24544,8,8,0},
    {0x801,21696,14128,17392,17336,21888,3,3,0},
    {0x915,15064,5032,7936,7880,15336,3,3,2}}};
struct SkeletalFixture {
    PaletteFixture f;
    SkeletalRow row;
    uint32_t effective_model=palette_model;
    explicit SkeletalFixture(SkeletalRow value=skeletal_rows[0]):row(value) {
        auto* rdram=f.rdram; // MEM_H expands against this conventional fixture name.
        player_case=0;skeletal_view=skeletal_phase=0;
#if TOOIE_NPC665_SECTION_PRESENT
        put(f.rdram,ns+12,npc_idle);
#endif
        for(uint32_t index=0;index<row.poses;++index)
            for(uint32_t diagonal=0;diagonal<4;++diagonal)
                put(f.rdram,palette_pose+0x50+index*64+diagonal*20,0x3F800000);
        MEM_H(0x10,guest(palette_descriptor))=1;
        prepare(1);
    }
    ~SkeletalFixture(){player_case=-1;skeletal_view=skeletal_phase=0;skeletal_boundary=-1;}
    uint32_t rec() const {return f.record();}
    uint32_t field(size_t offset) const {return f.field(offset);}
    uint32_t slot(size_t offset) const {return word(f.rdram,rec()+uint32_t(offset));}
    uint32_t packed() const {return row.poses|(row.bones<<8)|(row.hybrids<<16);}
    uint32_t paid() const {return 32+24*row.bones+32*row.hybrids;}
    void camera(uint32_t epoch,uint32_t id=0xA0000001) {
        auto* r=f.rdram;
        constexpr uint32_t cs=TOOIE_CAMERA_STATE_VRAM;
        constexpr uint32_t cc=cs+offsetof(TooieCameraState,control),cp=cs+offsetof(TooieCameraState,saved);
        put(r,cc+offsetof(TooieCameraControl,epoch),epoch);
        put(r,cc+offsetof(TooieCameraControl,main_begin),player_pool);
        put(r,cc+offsetof(TooieCameraControl,main_end),player_pool+TOOIE_CAMERA_M_BYTES);
        put(r,cs+offsetof(TooieCameraRecord,pointer),0x803A0000);
        put(r,cs+offsetof(TooieCameraRecord,birth),1);
        put(r,cs+offsetof(TooieCameraRecord,valid),1);
        put(r,cs+offsetof(TooieCameraRecord,view_id)+skeletal_view*4,id);
        put(r,cs+offsetof(TooieCameraRecord,view_scale)+skeletal_view*4,0x3F800000);
        put(r,cp+offsetof(TooieCameraProduction,matrix),0x803A1000);
        put(r,cp+offsetof(TooieCameraProduction,camera),0x803A0000);
        put(r,cp+offsetof(TooieCameraProduction,birth),1);
        put(r,cp+offsetof(TooieCameraProduction,view),skeletal_view);
        put(r,cp+offsetof(TooieCameraProduction,scale),0x3F800000);
        put(r,cp+offsetof(TooieCameraProduction,id),id);
        put(r,cp+offsetof(TooieCameraProduction,role),TOOIE_CAMERA_WORLD_INITIAL);
        put(r,cp+offsetof(TooieCameraProduction,epoch),epoch);
        put(r,cp+offsetof(TooieCameraProduction,valid),1);
        put(r,cp+offsetof(TooieCameraProduction,policy),0x02008AAA);
        put(r,0x8012D500,0x803A0000);put(r,0x8012D558,0x803A1000);
        put(r,0x80127674,skeletal_view);
    }
    void prepare(uint32_t epoch,uint32_t view=0,uint32_t phase=0,uint32_t camera_id=0xA0000001) {
        skeletal_view=view;skeletal_phase=phase;f.prepare(epoch,0,view,phase);
        auto* r=f.rdram;auto* rdram=r;
        MEM_H(0x14,guest(palette_descriptor))=row.resource;
        put(r,ac+offsetof(TooieActorPaletteContext,pair),(row.resource<<16)|0x1234);
        put(r,effective_model+4,row.graph);MEM_H(8,guest(effective_model))=0x30;
        MEM_H(10,guest(effective_model))=0x10;
        put(r,effective_model+12,row.lists);put(r,effective_model+16,row.vertices);
        put(r,effective_model+0x18,row.attributes);put(r,effective_model+0x28,0);
        if(row.attributes)MEM_H(4,guest(effective_model+row.attributes))=row.poses;
        put(r,palette_pose,row.poses);put(r,palette_pose+4,row.poses);
        put(r,0x8012C95C,effective_model+0x30);put(r,0x8012C960,effective_model+row.lists);
        put(r,0x8012C964,effective_model+row.vertices);
        for(auto entry:std::array<std::array<uint32_t,2>,6>{{{2,0x800DCF48},{3,0x800DD024},
                {10,0x800DD410},{12,0x800DD56C},{17,0x800DD65C},{24,0x800DD998}}})
            put(r,player_dispatch+entry[0]*4,entry[1]);
        camera(epoch,camera_id);
    }
    uint32_t invoke(recomp_func_t* fn,uint32_t a0,uint32_t a1=0,uint32_t a2=0,uint32_t a3=0) {
        auto* r=f.rdram;
        for(uint32_t at=stack-guard;at<stack+32;++at)put_byte(r,at,0xA5);
        auto c=context();c.r4=guest(a0);c.r5=guest(a1);c.r6=guest(a2);c.r7=guest(a3);
        const auto before=c;fn(r,&c);saved(c,before);
        for(uint32_t at=stack-guard;at<stack-TOOIE_SKELETAL_MAX_FRAME_BYTES;++at)
            require(byte(r,at)==0xA5,"skeletal helper stays inside measured nested frames");
        for(uint32_t at=stack+16;at<stack+32;++at)
            require(byte(r,at)==0xA5,"skeletal helper preserves above o32 argument homes");
        return low(c.r2);
    }
    uint32_t begin(){return invoke(candidate::tooie_npc665_admit,player_buffers,palette_cursor_slot,effective_model);}
    void group(uint32_t at,uint32_t id) {
        require(word(f.rdram,at)==0x6400000C && word(f.rdram,at+4)==id &&
            word(f.rdram,at+8)==0x02010AA9 && !word(f.rdram,at+12),"exact shared public skeletal group");
    }
    void admitted(uint32_t expected_limits=0) {
        require(begin()==1,"witness admits through the existing NPC665 export");
        const uint32_t id=word(f.rdram,ac+offsetof(TooieActorPaletteContext,idbase));
        require(word(f.rdram,f.cursor)==0xE0525464 && word(f.rdram,f.cursor+4)==0x10000064,
            "exact public enable precedes root");
        group(f.cursor+8,id+129);
        require(word(f.rdram,palette_cursor_slot)==f.cursor+24,"root reserves exactly 24 bytes");
        require(word(f.rdram,ac+offsetof(TooieActorPaletteContext,admitted))==5 && field(offsetof(TooieRenderBridgeState,active))==1,
            "skeletal owns the shared paid occurrence");
        require(slot(offsetof(TooieNpc665Slot,skeletal_limits))==(expected_limits?expected_limits:packed()),"named per-slot limits");
        put(f.rdram,player_buffers,word(f.rdram,palette_cursor_slot));
    }
    void visit(uint32_t index,bool hybrid) {
        auto* r=f.rdram;uint32_t at=word(r,player_buffers);
        const uint32_t id=word(r,ac+offsetof(TooieActorPaletteContext,idbase));
        uint32_t token=invoke(candidate::tooie_npc665_bone_push,player_buffers,index);
        require(token==1,"top-level bone saves INVALID role");group(at,id+index+1);
        if(hybrid){
            at=word(r,player_buffers);
            require(invoke(candidate::tooie_npc665_leaf_begin,player_buffers)==1,"single-list hybrid admits");
            group(at,id+256+index+1);
            require(word(r,at+16)==0xDA380003 && word(r,at+20)==palette_mtx_pool,
                "hybrid emits original-source LOAD NOPUSH after its group");
            const uint32_t tail=word(r,player_buffers);
            // Explicit original DA reload stand-in. The compiled leaf end must follow it.
            put(r,tail,0xDA380003);put(r,tail+4,palette_mtx_pool+64);put(r,player_buffers,tail+8);
            invoke(candidate::tooie_npc665_leaf_end,player_buffers);
            require(word(r,tail)==0xDA380003 && word(r,tail+8)==0x6400000D && word(r,tail+12)==1,
                "hybrid pop follows and preserves supplied original reload");
        }
        at=word(r,player_buffers);invoke(candidate::tooie_npc665_bone_pop,player_buffers,token);
        require(word(r,at)==0x6400000D && word(r,at+4)==1,"bone pop balances its exact group");
    }
#if TOOIE_NPC665_SECTION_PRESENT
    uint32_t finish(){
        const uint32_t at=word(f.rdram,player_buffers),result=invoke(candidate::tooie_npc665_end,player_buffers);
        require(word(f.rdram,at)==0x6400000D && word(f.rdram,at+4)==1,"end pays the root pop");
        require(!field(offsetof(TooieRenderBridgeState,active)) && !word(f.rdram,ac+48) &&
            word(f.rdram,ns+12)==npc_idle,"end releases and resets shared owner");
        f.cursor=word(f.rdram,player_buffers);return result;
    }
#endif
    void refused(uint32_t reason,bool silent=false) {
        std::array<uint32_t,18> before{};
        for(uint32_t i=0;i<before.size();++i)before[i]=word(f.rdram,rec()+i*4);
        const uint32_t blocks=field(offsetof(TooieRenderBridgeState,next_block));
        const uint32_t calls=field(offsetof(TooieRenderBridgeState,silo_calls));
        const uint32_t refusals=field(offsetof(TooieRenderBridgeState,silo_refusals));
        const uint32_t cursor=word(f.rdram,palette_cursor_slot),gfx=word(f.rdram,player_buffers);
        for(uint32_t at=cursor;at<cursor+32;at+=4)put(f.rdram,at,0xA55A1234);
        require(begin()==0,"unsupported or unpaid skeletal draw refuses");
        require(field(offsetof(TooieRenderBridgeState,next_block))==blocks &&
            word(f.rdram,palette_cursor_slot)==cursor && word(f.rdram,player_buffers)==gfx,"refusal pays no IDs or cursors");
        for(uint32_t i=0;i<before.size();++i)require(word(f.rdram,rec()+i*4)==before[i],"precommit refusal preserves every slot word");
        for(uint32_t at=cursor;at<cursor+32;at+=4)require(word(f.rdram,at)==0xA55A1234,"refusal emits no commands");
        require(field(offsetof(TooieRenderBridgeState,silo_calls))==calls+(silent?0:1) &&
            field(offsetof(TooieRenderBridgeState,silo_refusals))==refusals+(silent?0:1),"silent class exclusion versus counted refusal");
        if(!silent)require(field(offsetof(TooieRenderBridgeState,silo_last_reason))==reason,"exact refusal reason");
    }
};

#ifndef TOOIE_BK_B4_FIXTURE
// L06b-3: the NPC665 admit/bone/leaf bodies these groups drive exist only in the BK-OFF image, where they
// run (B4). SkeletalFixture above stays for the feather and title53 cases; its NPC members are inline.
// DE498 and original getters remain doubles; this observes the real DE448 capture/end scopes.
void skeletal_boundary_draw(uint8_t* rdram,recomp_context* c) {
    ++skeletal_boundary_calls;
    const uint32_t captured=word(rdram,ac),depth=word(rdram,state+offsetof(TooieRenderBridgeState,silo_depth));
    if(depth==1)skeletal_boundary_actor=captured;
    if(skeletal_boundary==2 && depth==1){
        auto nested=*c;nested.r4=guest(palette_actor);nested.r5=guest(0x801017D0);
        candidate::func_80101808(rdram,&nested);
        require(word(rdram,ac)==captured && word(rdram,state+offsetof(TooieRenderBridgeState,silo_depth))==1,
            "nested ordinary wrapper cannot steal or clear the direct-world context");
    }
    if(skeletal_boundary==1 && captured && depth==1){
        auto admit=*c;admit.r4=guest(player_buffers);admit.r5=guest(palette_cursor_slot);admit.r6=guest(palette_model);
        candidate::tooie_npc665_admit(rdram,&admit);require(low(admit.r2)==1,"title scoped owner reaches skeletal admission");
        put(rdram,player_buffers,word(rdram,palette_cursor_slot));
    }
    put(rdram,0x8012C9A8,0); // Explicit original DE318 cleanup stand-in.
    clobber(c);c->r2=guest(returned);
}

[[maybe_unused]] unsigned skeletal_fixtures() {
    unsigned cases=0;
    // E1: exact original-665 admission branch and emitter behaviour, including poisoned slot limits.
    {
        SkeletalFixture t;auto* r=t.f.rdram;auto* rdram=r;
        t.row={0x665,44976,12144,26248,0,48516,41,41,38};t.prepare(1);
        MEM_H(10,guest(palette_model))=0;put(r,palette_actor+0x10,0x80086980);
        MEM_H(0x8C,guest(palette_actor))=1;MEM_H(0,guest(palette_controller_pool))=0x3C;
        MEM_H(4,guest(palette_data))=0x665;
        put(r,palette_data+0x40,palette_data-0x1B14+0x4E8);
        put(r,palette_data+0x14,palette_data-0x1B14+0x7FC);
        MEM_H(0,guest(0x8012BE62))=1;MEM_H(0,guest(0x8012BE64))=0x665;
        put(r,0x8012CA18,0x665);put(r,0x8012B804,palette_model);put(r,0x8012B808,0x665u<<18);
        put(r,ns,palette_data);put(r,ns+4,1);put(r,ns+8,0x1F);put(r,ns+12,npc_idle|1);
        put(r,t.rec()+offsetof(TooieNpc665Slot,skeletal_limits),1);
        require(t.begin()==1,"original pending NPC665 still admits");
        require(t.slot(offsetof(TooieNpc665Slot,flags))==npc_route && !t.slot(offsetof(TooieNpc665Slot,skeletal_limits)),
            "665 renewal clears the formerly skeletal word and uses its own constants");
        require(t.field(offsetof(TooieRenderBridgeState,extra_bytes))==2232,"665 original payment");
        const uint32_t id=word(r,ac+44);t.group(t.f.cursor+8,id+129);
        put(r,player_buffers,word(r,palette_cursor_slot));
        for(uint32_t i=0;i<41;++i)t.visit(i,i<38);
        require(word(r,ns)==41 && word(r,ns+8)==38,"665 retains all 41 bones and 38 hybrids");
        require(t.finish()==0,"665 exact complete occurrence balances");++cases;
    }
    // E2: each row emits every allowed bone/hybrid and uses a distinct full view/phase page.
    for(const auto row:skeletal_rows){
        SkeletalFixture t(row);t.prepare(1,3,6);t.admitted();
        const uint32_t id=word(t.f.rdram,ac+44);
        require(id==TOOIE_BRIDGE_ID_BASE+27*512,"view3 phase6 uses page27, never compact CPU-root strides");
        require(t.field(offsetof(TooieRenderBridgeState,extra_bytes))==t.paid(),"row pays its complete envelope");
        for(uint32_t i=0;i<row.bones;++i)t.visit(i,i<row.hybrids);
        require(t.finish()==0,"every witnessed maximum closes cleanly");++cases;
    }
    // E3: matrix room is one aligned matrix short; ledger/list probes cross their exact boundary.
    for(unsigned which=0;which<3;++which){
        SkeletalFixture t;auto* r=t.f.rdram;
        if(which==0)put(r,player_buffers+4,palette_mtx_pool+0x12C00-64*(1+t.row.bones+t.row.hybrids)+64);
        if(which==1){t.f.cursor=player_pool+TOOIE_CAMERA_M_BYTES-(1640+t.paid())+1;put(r,palette_cursor_slot,t.f.cursor);}
        if(which==2){put(r,state+offsetof(TooieRenderBridgeState,seen_epoch),1);
            put(r,state+offsetof(TooieRenderBridgeState,extra_bytes),6400-t.paid()+1);}
        t.refused(which==1?TOOIE_BRIDGE_RANGE:TOOIE_BRIDGE_CAPACITY);++cases;
    }
    // The aligned physical boundary just below the complete payment also refuses CAPACITY.
    {SkeletalFixture t;t.f.cursor=player_pool+TOOIE_CAMERA_M_BYTES-(1640+t.paid())+8;
        put(t.f.rdram,palette_cursor_slot,t.f.cursor);t.refused(TOOIE_BRIDGE_CAPACITY);++cases;}
    // E4: class exclusions are silent; a refused pending draw never falls through.
    for(unsigned which=0;which<7;++which){
        SkeletalFixture t;auto* r=t.f.rdram;auto* rdram=r;
        if(which<2)put(r,ac+12,((which?0x917u:0x665u)<<16)|0x1234);
        if(which==2)MEM_H(10,guest(palette_model))=0x40;
        if(which==3)put(r,palette_model+0x28,3024); // CPU stream family remains separate.
        if(which==4)put(r,palette_model+4,17552); // Non-witness graph, e.g. billboard family.
        if(which==5)put(r,palette_model+16,0);
        if(which==6)put(r,ns+12,npc_idle|1);
        t.refused(TOOIE_BRIDGE_BIRTH,which!=6);++cases;
    }
    // E5: shared emitter tripwires, plus an out-of-range slot before any limits read.
    for(unsigned which=0;which<5;++which){
        SkeletalFixture t;t.admitted();auto* r=t.f.rdram;
        if(which==0)t.invoke(candidate::tooie_npc665_bone_push,player_buffers,t.row.poses);
        if(which==1){put(r,ns,t.row.bones);t.invoke(candidate::tooie_npc665_bone_push,player_buffers,0);}
        if(which==2){put(r,ns+4,1);put(r,ns+8,t.row.hybrids);t.invoke(candidate::tooie_npc665_leaf_begin,player_buffers);}
        if(which==3){put(r,ns+4,0);t.invoke(candidate::tooie_npc665_leaf_begin,player_buffers);}
        if(which==4){put(r,ac+60,200);t.invoke(candidate::tooie_npc665_bone_push,player_buffers,0);}
        require(word(r,ns+12)&npc_failed,"invalid emitter state trips synchronously");
        require(t.finish()!=0,"tripwire reaches the checked end contract");++cases;
    }
    // E6: no gap-driven renewal; pose class/model/birth change does renew; no block recycling.
    {
        SkeletalFixture t;auto* rdram=t.f.rdram;
        t.admitted();const uint32_t first=word(t.f.rdram,ac+44);require(!t.finish(),"initial lifetime closes");
        for(uint32_t epoch:{2u,5u}){t.prepare(epoch);t.admitted();require(word(t.f.rdram,ac+44)==first,"same identity survives next and missed epochs");require(!t.finish(),"stable lifetime closes");++cases;}
        MEM_H(0x10,guest(palette_descriptor))=0;put(t.f.rdram,0x8012C840,palette_pose);
        t.prepare(6);t.admitted();require(word(t.f.rdram,ac+44)==first+28*512,"direct-to-scratch pose class renews 28 pages");require(!t.finish(),"scratch closes");
        put(t.f.rdram,0x8012C824,1);t.prepare(7);t.admitted();require(word(t.f.rdram,ac+44)==first+56*512,"scratch input class change renews");require(!t.finish(),"animated scratch closes");
        put(t.f.rdram,state+offsetof(TooieRenderBridgeState,births),2);t.prepare(8);t.admitted();require(word(t.f.rdram,ac+44)==first+84*512,"birth change renews");require(!t.finish(),"new birth closes");
        t.effective_model+=0x10000;t.prepare(9);t.admitted();require(word(t.f.rdram,ac+44)==first+112*512,"effective model pointer change renews without a pair change");require(!t.finish(),"new model closes");
        cases+=4;
    }
    {SkeletalFixture t;t.admitted();require(!t.finish(),"duplicate setup closes");t.prepare(1);t.refused(TOOIE_BRIDGE_DUPLICATE);++cases;}
    {SkeletalFixture t;t.admitted();require(!t.finish(),"cut setup closes");t.prepare(2,0,0,0xA0000002);
        require(t.begin()==0 && t.field(offsetof(TooieRenderBridgeState,silo_last_reason))==TOOIE_BRIDGE_VIEW,"camera cut refuses");
        require(t.slot(offsetof(TooieNpc665Slot,camera_cut_views))==1 && t.field(offsetof(TooieRenderBridgeState,next_block))==28,"camera cut holds lifetime and latches view");++cases;}
    {SkeletalFixture t;put(t.f.rdram,state+offsetof(TooieRenderBridgeState,next_block),TOOIE_BRIDGE_BLOCK_COUNT-27);
        t.refused(TOOIE_BRIDGE_BLOCKS);++cases;}
    {SkeletalFixture t;put(t.f.rdram,state+offsetof(TooieRenderBridgeState,next_block),TOOIE_BRIDGE_BLOCK_COUNT-28);
        t.admitted();require(word(t.f.rdram,ac+44)+27*512+256+t.row.poses<0xA0000000,"last reservation stays below camera namespace");
        require(t.field(offsetof(TooieRenderBridgeState,next_block))==TOOIE_BRIDGE_BLOCK_COUNT,"final reservation does not wrap");require(!t.finish(),"last block closes");++cases;}
    {SkeletalFixture t;t.admitted();auto* r=t.f.rdram;put(r,state+offsetof(TooieRenderBridgeState,births),2);
        t.invoke(candidate::func_8008B10C,palette_controller,0);
        require(!t.slot(offsetof(TooieNpc665Slot,block_and_valid)) && !t.field(offsetof(TooieRenderBridgeState,silo_valid_count)) &&
            (word(r,ns+12)&npc_failed),"retiring an active skeletal slot marks its paid owner failed");
        require(t.finish()!=0,"retirement preserves the paid pop and reaches checked failure");
        t.prepare(2);t.admitted();require(word(r,ac+44)==TOOIE_BRIDGE_ID_BASE+28*512 &&
            t.field(offsetof(TooieRenderBridgeState,next_block))==56,"retirement never recycles a reserved block");
        require(!t.finish(),"new lifetime after retirement closes");++cases;}
    // E7: later existing helpers cannot change the already paid owner/counters/stream.
    {SkeletalFixture t;t.admitted();auto* r=t.f.rdram;
        const uint32_t calls=t.field(offsetof(TooieRenderBridgeState,silo_calls)),at=word(r,player_buffers),blocks=t.field(offsetof(TooieRenderBridgeState,next_block));
        require(!t.f.begin() && !t.invoke(candidate::tooie_actor_cpu_root_begin,player_buffers,palette_cursor_slot,palette_model,0xA0000001) &&
            !t.invoke(candidate::tooie_actor_feather_leaf_id,player_buffers,palette_model,0xA0000001),"later routes return silently");
        require(t.field(offsetof(TooieRenderBridgeState,silo_calls))==calls && t.field(offsetof(TooieRenderBridgeState,next_block))==blocks &&
            word(r,player_buffers)==at && word(r,ac+48)==5,"later routes preserve paid skeletal state");require(!t.finish(),"shared occurrence closes after exclusions");++cases;}
    // E8: real DE448 capture scope, ordinary nested wrapper, checked close, and title exit.
    for(unsigned which=0;which<8;++which){
        SkeletalFixture t;auto* r=t.f.rdram;auto* rdram=r;
        skeletal_boundary=int(which==1?1:which==2?2:0);
        skeletal_boundary_calls=0;skeletal_boundary_actor=0;
        put(r,ac,0);put(r,state+offsetof(TooieRenderBridgeState,silo_depth),0);
        uint32_t position=palette_actor+4;
        if(which==3)position=palette_actor+0x200;
        if(which==4)put(r,0x8012C9A8,0);
        if(which==5)MEM_H(0x14,guest(palette_descriptor))=0x65E;
        if(which==6)MEM_H(0x14,guest(palette_descriptor))=0x671;
        if(which==7)MEM_H(0x14,guest(palette_descriptor))=0x90C;
        auto c=context();c.r4=guest(position);c.r5=guest(rotation);c.r6=guest(0x3F800000);c.r7=guest(extra);
        put(r,stack+0x10,palette_model);const auto before=c;candidate::func_800DE448(r,&c);saved(c,before);
        const bool captures=which<3||which>=6;
        require(skeletal_boundary_actor==(captures?palette_actor:0),"title capture requires current staged callback, world position and resource scope");
        require(!word(r,ac) && !word(r,ac+48) && !t.field(offsetof(TooieRenderBridgeState,silo_depth)),"direct owner clears after end and depth decrement");
        require(skeletal_boundary_calls==(which==2?2u:1u),"nested wrapper reaches the original draw double once more");
        if(which==1)require(!t.field(offsetof(TooieRenderBridgeState,active)) && word(r,ns+12)==npc_idle,"actual checked end ran before title context clear");
        // A subsequent title-exit call has no staging and must not recover a stale actor.
        skeletal_boundary_actor=0;c=context();c.r4=guest(position);c.r5=guest(rotation);c.r6=guest(0x3F800000);c.r7=guest(extra);
        put(r,stack+0x10,palette_model);candidate::func_800DE448(r,&c);
        require(!skeletal_boundary_actor && !word(r,ac),"cleared staging prevents title-exit recapture");++cases;
    }
    // E9 is the existing select_prop_fixtures() call in main, run before this extension.
    // E10: each model-data predicate names itself above the MODEL_DATA low byte, with the
    // witness row in bits16..17. Simultaneous section failures are reported together.
    for(unsigned which=0;which<8;++which){
        SkeletalFixture t(which==7?skeletal_rows[1]:skeletal_rows[0]);auto* r=t.f.rdram;auto* rdram=r;
        const uint32_t m=t.effective_model;uint32_t bits=0;
        if(which==0){put(r,0x8012C95C,m+0x38);bits=0x100;}
        if(which==1){put(r,0x8012C960,m+t.row.lists+8);bits=0x200;}
        if(which==2||which==7){put(r,0x8012C964,m+t.row.vertices+16);bits=0x400;}
        if(which==3){MEM_H(4,guest(m+t.row.attributes))=t.row.poses+1;bits=0x800;}
        if(which==4){MEM_H(8,guest(m))=int16_t(-8);bits=0x1000;}
        if(which==5){put(r,player_dispatch+24*4,0);bits=0x2000;}
        if(which==6){put(r,0x8012C95C,m+0x38);put(r,0x8012C964,m+t.row.vertices+16);bits=0x500;}
        // A foreign list refuses under either qualified callback; E11 covers both.
        if(bits&0x400u)put(r,0x8012C9A8,0x801015D0);
        t.refused(TOOIE_BRIDGE_MODEL_DATA|bits|((which==7?2u:1u)<<16));++cases;
    }
    // E12: census rows. Type3 rows also require the default type3 handler entry, which is
    // not consulted for other rows; tuple near-misses stay silent class exclusions; mode4
    // segment3 still refuses as OVERRIDE bit6; the row number names refusals in bits16..23.
    {SkeletalFixture t(skeletal_rows[5]);put(t.f.rdram,player_dispatch+3*4,0);
        t.refused(TOOIE_BRIDGE_MODEL_DATA|0x2000u|(6u<<16));++cases;}
    {SkeletalFixture t(skeletal_rows[3]);put(t.f.rdram,player_dispatch+3*4,0);t.admitted();
        for(uint32_t i=0;i<t.row.bones;++i)t.visit(i,i<t.row.hybrids);
        require(t.finish()==0,"type3 table entry is not consulted for rows without type3");++cases;}
    {SkeletalFixture t(skeletal_rows[3]);put(t.f.rdram,t.effective_model+12,t.row.lists+8);
        t.refused(TOOIE_BRIDGE_BIRTH,true);++cases;}
    {SkeletalFixture t(skeletal_rows[4]);put(t.f.rdram,0x8012CA28,4);
        t.refused(TOOIE_BRIDGE_OVERRIDE|(0x40u<<8));++cases;}
    {SkeletalFixture t(skeletal_rows[7]);auto* rdram=t.f.rdram;
        MEM_H(4,guest(t.effective_model+t.row.attributes))=t.row.poses+1;
        t.refused(TOOIE_BRIDGE_MODEL_DATA|0x800u|(8u<<16));++cases;}
    // A census row inherits the accepted single-list hybrid emitter: every bone and every
    // type24 call of 0x616 emits and closes at its exact executed prepay.
    {SkeletalFixture t(skeletal_rows[3]);t.admitted();
        require(t.field(offsetof(TooieRenderBridgeState,extra_bytes))==328,"0x616 prepays 32+24*7+32*4");
        for(uint32_t i=0;i<7;++i)t.visit(i,i<4);
        require(t.finish()==0,"0x616 exact executed bounds close");++cases;}
    // E13: row 9 (0x92A, WP7 selector-prefix ordinary row; E2 above already admits it and
    // closes every executed bone). Its type3 leaves need the default type3 entry; the ledger
    // boundary is its own 296-byte prepay; a near-miss header tuple stays a silent exclusion.
    {SkeletalFixture t(skeletal_rows[8]);put(t.f.rdram,player_dispatch+3*4,0);
        t.refused(TOOIE_BRIDGE_MODEL_DATA|0x2000u|(9u<<16));++cases;}
    {SkeletalFixture t(skeletal_rows[8]);require(t.paid()==296,"0x92A prepays 32+24*11");
        put(t.f.rdram,state+offsetof(TooieRenderBridgeState,seen_epoch),1);
        put(t.f.rdram,state+offsetof(TooieRenderBridgeState,extra_bytes),6400-t.paid()+1);
        t.refused(TOOIE_BRIDGE_CAPACITY);++cases;}
    {SkeletalFixture t(skeletal_rows[8]);put(t.f.rdram,t.effective_model+16,t.row.vertices+8);
        t.refused(TOOIE_BRIDGE_BIRTH,true);++cases;}
    // E14: rows 10..14 (E2 above admits each and closes every executed bone/hybrid). Row 14's one
    // type24 node has two list members, both current bone 2/parent 0: one label role, each member
    // counted against the 2-call bound; a third call trips. Row 10 keeps the conservative type3 check.
    {SkeletalFixture t(skeletal_rows[13]);t.admitted();auto* r=t.f.rdram;
        require(t.paid()==168,"0x915 prepays 32+24*3+32*2");
        t.visit(0,false);t.visit(1,false);
        const uint32_t id=word(r,ac+44);
        uint32_t at=word(r,player_buffers);
        const uint32_t token=t.invoke(candidate::tooie_npc665_bone_push,player_buffers,2);
        require(token==1,"top-level bone2 push");t.group(at,id+3);
        for(uint32_t member=0;member<2;++member){
            at=word(r,player_buffers);
            require(t.invoke(candidate::tooie_npc665_leaf_begin,player_buffers)==1,"each list member of the node admits");
            t.group(at,id+256+3);
            const uint32_t tail=word(r,player_buffers);
            put(r,tail,0xDA380003);put(r,tail+4,palette_mtx_pool+64);put(r,player_buffers,tail+8);
            t.invoke(candidate::tooie_npc665_leaf_end,player_buffers);
            require(word(r,tail+8)==0x6400000D && word(r,tail+12)==1,"each member's pop follows its original reload");
        }
        at=word(r,player_buffers);t.invoke(candidate::tooie_npc665_bone_pop,player_buffers,token);
        require(word(r,at)==0x6400000D,"bone2 pop balances");
        require(t.finish()==0,"0x915 two-member node closes at its exact bound");++cases;}
    {SkeletalFixture t(skeletal_rows[13]);t.admitted();auto* r=t.f.rdram;
        const uint32_t token=t.invoke(candidate::tooie_npc665_bone_push,player_buffers,2);
        for(uint32_t member=0;member<2;++member){
            require(t.invoke(candidate::tooie_npc665_leaf_begin,player_buffers)==1,"two members admit");
            t.invoke(candidate::tooie_npc665_leaf_end,player_buffers);
        }
        t.invoke(candidate::tooie_npc665_leaf_begin,player_buffers);
        require(word(r,ns+12)&npc_failed,"a third list call exceeds the finite bound and trips");
        t.invoke(candidate::tooie_npc665_bone_pop,player_buffers,token);
        require(t.finish()!=0,"the tripwire reaches the checked end contract");++cases;}
    {SkeletalFixture t(skeletal_rows[9]);put(t.f.rdram,player_dispatch+3*4,0);
        t.refused(TOOIE_BRIDGE_MODEL_DATA|0x2000u|(10u<<16));++cases;}
    // E15: row 15 (07C2 bd0db0f9, header exactly 0x0100). DE498 installed C820=80122F70 and completed
    // the returning setup call DE000000/00122F70 at the local cursor before DEFA0, so it sits at
    // current-8; its 13 static commands are not active-stream bytes and are not charged again.
    {
        constexpr SkeletalRow row07C2{0x7C2,12280,2984,7264,7096,12724,10,10,0};
        const auto header0100=[](SkeletalFixture& t,uint32_t c820,uint32_t word0,uint32_t word1) {
            auto* rdram=t.f.rdram;MEM_H(10,guest(t.effective_model))=0x100;
            put(rdram,0x8012C820,c820);put(rdram,t.f.cursor-8,word0);put(rdram,t.f.cursor-4,word1);
        };
        {SkeletalFixture t(row07C2);header0100(t,0x80122F70,0xDE000000,0x00122F70);
            require(t.paid()==272,"0x7C2 prepays 32+24*10");t.admitted();
            for(uint32_t bone=0;bone<10;++bone)t.visit(bone,false);
            require(t.finish()==0,"0x7C2 closes all ten bones at its exact bound");++cases;}
        // Bit0 refuses anything but the exact original setup call.
        for(int which=0;which<5;++which) {
            SkeletalFixture t(row07C2);
            if(which==0)header0100(t,0,0xDE000000,0x00122F70);            // setup pointer not installed
            if(which==1)header0100(t,0x80122DB0,0xDE000000,0x00122F70);   // the reset list, not the setup
            if(which==2)header0100(t,0x80122F70,0xDE010000,0x00122F70);   // not the returning push form
            if(which==3)header0100(t,0x80122F70,0xDE000000,0x00122DB0);   // another target
            if(which==4){header0100(t,0x80122F70,0xDE000000,0x00122F70);  // no 8 MAIN bytes below
                t.f.cursor=player_pool;put(t.f.rdram,palette_cursor_slot,t.f.cursor);}
            t.refused(TOOIE_BRIDGE_OVERRIDE|(1u<<8));++cases;
        }
        // The header filter is row-scoped: rows 1..14 stay excluded with 0x0100, and row 15 with
        // anything but exactly 0x0100 (0x10, 0, 0x108, 0x102) stays excluded, all silently.
        {SkeletalFixture t(skeletal_rows[0]);header0100(t,0x80122F70,0xDE000000,0x00122F70);
            t.refused(0,true);++cases;}
        for(uint32_t flags:{0x10u,0u,0x108u,0x102u}) {
            SkeletalFixture t(row07C2);header0100(t,0x80122F70,0xDE000000,0x00122F70);
            auto* rdram=t.f.rdram;MEM_H(10,guest(t.effective_model))=int16_t(flags);t.refused(0,true);++cases;
        }
        // Every other row keeps C820==0 even when the exact setup pointer is present.
        {SkeletalFixture t(skeletal_rows[0]);put(t.f.rdram,0x8012C820,0x80122F70);
            t.refused(TOOIE_BRIDGE_OVERRIDE|(1u<<8));++cases;}
        // The default type3 table check is retained for row 15.
        {SkeletalFixture t(row07C2);header0100(t,0x80122F70,0xDE000000,0x00122F70);
            put(t.f.rdram,player_dispatch+3*4,0);t.refused(TOOIE_BRIDGE_MODEL_DATA|0x2000u|(15u<<16));++cases;}
    }
    // E16: row 16, the shared static root-only family (0658 daf18dee, execution-facts37). Header flags
    // exactly 0; attributes 0, so no pose section exists and DE498 cleared C968 before DEFA0. The
    // envelope is enable + root group + pop (32 bytes); DE498's root LOAD/PUSH and pop are the only
    // matrix work (one Mtx slot); any DCF48 bone or type24 list call fails closed. Identity is the
    // descriptor slot + birth with a fixed key. prepare() restages flags 0x10 and C968=palette_pose
    // for every row, so root() restores this row's DE498 state after every prepare().
    {
        constexpr SkeletalRow row0658{0x658,8040,4264,6160,0,8100,0,0,0};
        const auto root=[](SkeletalFixture& t){auto* rdram=t.f.rdram;
            MEM_H(10,guest(t.effective_model))=0;put(rdram,0x8012C968,0);};
        {SkeletalFixture t(row0658);t.prepare(1,3,6);root(t);
            require(t.paid()==32,"0x658 prepays the enable, root group and root pop only");t.admitted();
            require(word(t.f.rdram,ac+44)==TOOIE_BRIDGE_ID_BASE+27*512,"view3 phase6 uses page27");
            require(t.field(offsetof(TooieRenderBridgeState,extra_bytes))==32,"0x658 pays exactly its 32-byte envelope");
            require(t.slot(offsetof(TooieNpc665Slot,pose_handle))==0x01000000u &&
                t.slot(offsetof(TooieNpc665Slot,birth))==word(t.f.rdram,state+offsetof(TooieRenderBridgeState,births)),
                "descriptor birth identity with the fixed root-only key");
            require(t.finish()==0,"0x658 closes with the paid root pop and no bone or list call");++cases;}
        // Attributes 0: a staged C968 is unqualified (DE498 would have cleared it); no pose block is read.
        {SkeletalFixture t(row0658);root(t);put(t.f.rdram,0x8012C968,palette_pose);t.refused(TOOIE_BRIDGE_ANIMATION);++cases;}
        // No bone and no list-call bound: a DCF48 push or a type24 list call trips the checked end.
        for(unsigned which=0;which<2;++which){
            SkeletalFixture t(row0658);root(t);t.admitted();auto* r=t.f.rdram;
            if(which==0)require(!t.invoke(candidate::tooie_npc665_bone_push,player_buffers,0),"0x658 has no bone role");
            else require(!t.invoke(candidate::tooie_npc665_leaf_begin,player_buffers),"0x658 has no hybrid list call");
            require(word(r,ns+12)&npc_failed,"the invalid emitter call trips synchronously");
            require(t.finish()!=0,"the tripwire reaches the checked end contract");++cases;
        }
        // The root Mtx slot is the exact matrix bound: 64 bytes left admit and close, 0 refuse CAPACITY.
        for(uint32_t left:{64u,0u}){
            SkeletalFixture t(row0658);root(t);put(t.f.rdram,player_buffers+4,palette_mtx_pool+0x12C00-left);
            if(left){t.admitted();require(t.finish()==0,"one root Mtx slot suffices");}
            else t.refused(TOOIE_BRIDGE_CAPACITY);
            ++cases;
        }
        // Row-scoped header and tuple: flags exactly 0 (the harness 0x10, and 0x02/0x40, are silent);
        // a deform word or a nonzero attributes word misses the row silently.
        for(uint32_t flags:{0x10u,0x02u,0x40u}){
            SkeletalFixture t(row0658);root(t);auto* rdram=t.f.rdram;
            MEM_H(10,guest(t.effective_model))=int16_t(flags);t.refused(0,true);++cases;
        }
        {SkeletalFixture t(row0658);root(t);put(t.f.rdram,t.effective_model+0x28,8);t.refused(0,true);++cases;}
        {SkeletalFixture t(row0658);root(t);put(t.f.rdram,t.effective_model+0x18,16);t.refused(0,true);++cases;}
        // The default handler table stays exact for type3 (DD024) and type12 (DD56C).
        for(uint32_t type:{3u,12u}){
            SkeletalFixture t(row0658);root(t);put(t.f.rdram,player_dispatch+type*4,0);
            t.refused(TOOIE_BRIDGE_MODEL_DATA|0x2000u|(16u<<16));++cases;
        }
        // Root guards are unchanged: segment3 mode4 (C948[56]), a staged root mode (C948[28]) and a
        // post-draw callback (C948[26]) each refuse with their own OVERRIDE bit.
        {SkeletalFixture t(row0658);root(t);put(t.f.rdram,0x8012CA28,4);t.refused(TOOIE_BRIDGE_OVERRIDE|(0x40u<<8));++cases;}
        {SkeletalFixture t(row0658);root(t);put(t.f.rdram,0x8012C9B8,1);t.refused(TOOIE_BRIDGE_OVERRIDE|(0x20u<<8));++cases;}
        {SkeletalFixture t(row0658);root(t);put(t.f.rdram,0x8012C9B0,0x80224000);t.refused(TOOIE_BRIDGE_OVERRIDE|(0x10u<<8));++cases;}
        // Lifetime: a dead descriptor (active bitmap clear) or a rebirth (new SS.births value) refuses
        // BIRTH before any identity is reused.
        {SkeletalFixture t(row0658);root(t);for(uint32_t i=0;i<25u;++i)put_byte(t.f.rdram,0x80135450u+i,0);
            t.refused(TOOIE_BRIDGE_BIRTH);++cases;}
        {SkeletalFixture t(row0658);root(t);const uint32_t births=state+offsetof(TooieRenderBridgeState,births);
            put(t.f.rdram,births,word(t.f.rdram,births)+1);t.refused(TOOIE_BRIDGE_BIRTH);++cases;}
        // Identity: one ID across the next and a missed epoch, and across a descriptor pose-class change
        // (the key is fixed; no pose source is read); a birth change renews 28 pages; a same-epoch repeat
        // of one view/phase is DUPLICATE.
        {
            SkeletalFixture t(row0658);root(t);auto* rdram=t.f.rdram;
            t.admitted();const uint32_t first=word(rdram,ac+44);require(!t.finish(),"initial root lifetime closes");
            for(uint32_t epoch:{2u,5u}){t.prepare(epoch);root(t);t.admitted();
                require(word(rdram,ac+44)==first,"same root identity survives next and missed epochs");
                require(!t.finish(),"stable root lifetime closes");++cases;}
            MEM_H(0x10,guest(palette_descriptor))=0;put(rdram,0x8012C840,palette_pose);
            t.prepare(6);root(t);t.admitted();
            require(word(rdram,ac+44)==first && t.slot(offsetof(TooieNpc665Slot,pose_handle))==0x01000000u,
                "a descriptor pose-class change does not renew the fixed root key");
            require(!t.finish(),"pose-class probe closes");++cases;
            put(rdram,state+offsetof(TooieRenderBridgeState,births),2);t.prepare(8);root(t);t.admitted();
            require(word(rdram,ac+44)==first+28*512,"birth change renews 28 pages");require(!t.finish(),"new birth closes");++cases;
        }
        {SkeletalFixture t(row0658);root(t);t.admitted();require(!t.finish(),"duplicate setup closes");
            t.prepare(1);root(t);t.refused(TOOIE_BRIDGE_DUPLICATE);++cases;}
    }
    // E11: staged vertex lists. Under either qualified callback, C948[7] is the model's own
    // list or one of the copies original 80100D24 made for the actor's handle (80100A74
    // record+0/+4). The actor trigger/selector word only chooses between them, so it varies
    // independently. FreeList, record and copy headers are synthetic scalar witnesses.
    constexpr uint32_t copy_pool=0x802C0000u,copy_handle=2u,copy_count=8u;
    constexpr uint32_t copy_record=copy_pool+4u+12u*copy_handle;
    constexpr uint32_t other_record=copy_pool+4u+12u*1u;
    constexpr std::array<uint32_t,2> copies={0x802C1000u,0x802C2000u};
    constexpr uint32_t foreign_list=0x802C3000u,other_copy=0x802C4000u,packed_record=0x802C5000u;
    constexpr std::array<uint32_t,2> callbacks={0x801015D0u,0x801017D0u};
    // Actor word 0x70: no trigger, word bit21 (copy0), byte70 bit0 with byte71 bit7 (copy1).
    constexpr std::array<uint32_t,3> triggers={0u,0x00200000u,0x01800000u};
    const auto copy_witness=[&](SkeletalFixture& t) {
        auto* rdram=t.f.rdram;
        put(rdram,0x80135A60,copy_pool);MEM_H(0,guest(copy_pool))=12;MEM_H(2,guest(copy_pool))=4;
        put(rdram,copy_record,copies[0]);put(rdram,copy_record+4,copies[1]);put_byte(rdram,copy_record+9,1);
        put(rdram,other_record,other_copy);put_byte(rdram,other_record+9,1);
        MEM_H(0x16,guest(t.effective_model+t.row.vertices))=copy_count<<1;
        for(const uint32_t list:{copies[0],copies[1],foreign_list,other_copy})
            MEM_H(0x16,guest(list))=(copy_count<<1)|1;
        MEM_H(0x60,guest(palette_actor))=copy_handle;
    };
    // Per-draw inputs: staged callback and C948[7].
    const auto stage=[&](SkeletalFixture& t,uint32_t callback,uint32_t list) {
        put(t.f.rdram,0x8012C9A8,callback);put(t.f.rdram,0x8012C964,list);
    };
    const auto own_list=[](const SkeletalFixture& t) {return t.effective_model+t.row.vertices;};
    // Own list and both owned copies admit under both callbacks and every trigger word.
    // DF720's packed record stays staged throughout and is never a predicate.
    for(const uint32_t callback:callbacks)
        for(unsigned which=0;which<3;++which)
            for(const uint32_t trigger:triggers){
                SkeletalFixture t;copy_witness(t);
                put(t.f.rdram,palette_actor+0x70,trigger);
                stage(t,callback,which==0?own_list(t):copies[which-1]);
                put(t.f.rdram,0x8012C974,packed_record);
                t.admitted();
                require(word(t.f.rdram,ac+44)==TOOIE_BRIDGE_ID_BASE,"own list and owned copies take the first identity");
                require(!t.finish(),"owned list closes independent of callback and trigger word");++cases;
            }
    // List, copy, callback and trigger changes across epochs never renew the identity.
    {SkeletalFixture t;copy_witness(t);uint32_t first=0;
        const std::array<uint32_t,4> lists={own_list(t),copies[0],copies[1],own_list(t)};
        for(uint32_t epoch=1;epoch<=4;++epoch){
            if(epoch>1)t.prepare(epoch);
            stage(t,callbacks[epoch&1u],lists[epoch-1]);
            put(t.f.rdram,palette_actor+0x70,triggers[epoch%3u]);
            t.admitted();
            const uint32_t id=word(t.f.rdram,ac+44);
            if(epoch==1)first=id;
            require(id==first && t.field(offsetof(TooieRenderBridgeState,next_block))==28,
                "alternating lists keep one identity and one block");
            require(!t.finish(),"alternating list closes");
        }
        ++cases;}
    // Count-halfword bit0 is a transient dirty flag: 80100D24 sets it and F68C's RGB
    // restore clears it. A drawn copy admits with one identity whether it is set or clear.
    for(const uint32_t callback:callbacks){
        SkeletalFixture t;copy_witness(t);auto* rdram=t.f.rdram;uint32_t first=0;
        for(uint32_t epoch=1;epoch<=4;++epoch){
            if(epoch>1)t.prepare(epoch);
            const uint32_t dirty=epoch&1u;
            MEM_H(0x16,guest(copies[0]))=(copy_count<<1)|dirty;
            stage(t,callback,copies[0]);
            t.admitted();
            const uint32_t id=word(rdram,ac+44);
            if(epoch==1)first=id;
            require(id==first && t.field(offsetof(TooieRenderBridgeState,next_block))==28,
                "dirty flag transitions keep one identity and one block");
            require(!t.finish(),"clean or dirty copy closes");
        }
        ++cases;
    }
    // Foreign and malformed lists refuse as MODEL_DATA vertex under both callbacks.
    for(const uint32_t callback:callbacks)
        for(unsigned which=0;which<8;++which){
            SkeletalFixture t;copy_witness(t);auto* rdram=t.f.rdram;
            uint32_t list=copies[0];
            if(which==0)list=foreign_list;                                   // Well formed, but unowned.
            if(which==1)list=other_copy;                                     // Another handle's copy.
            if(which==2)MEM_H(0x16,guest(copies[0]))=((copy_count+1)<<1)|1;  // Count differs.
            if(which==3)put_byte(rdram,copy_record+9,0);                     // Record not filled.
            if(which==4)MEM_H(0x60,guest(palette_actor))=0;                  // No handle.
            if(which==5)MEM_H(0x60,guest(palette_actor))=4;                  // Handle at capacity.
            if(which==6)MEM_H(0,guest(copy_pool))=16;                        // Not the stride-12 FreeList.
            if(which==7)list=copies[0]+16;                                   // Inside a copy, not its header.
            put(rdram,palette_actor+0x70,triggers[2]);
            stage(t,callback,list);
            t.refused(TOOIE_BRIDGE_MODEL_DATA|0x400u|(1u<<16));++cases;
        }
    // An owned copy does not qualify a callback outside the two traced callbacks.
    for(const uint32_t callback:{0x80092234u,0x801017D4u,0u}){
        SkeletalFixture t;copy_witness(t);stage(t,callback,copies[1]);
        t.refused(TOOIE_BRIDGE_OVERRIDE|(0x100u<<8));++cases;
    }

    // E12: C838/CA11 appearance, staged before DEFA0 and consumed by DE498 at DF150.
    // The rows below are scalar stand-ins for the original D_80123030 table (mode4 uses a
    // synthetic word). No original initializer or worker runs in this fixture. That modes
    // 1..3 write only vertex RGB, normals and packed scratch is original-source evidence.
    constexpr uint32_t appearance_table=0x80123030u;
    const auto appearance_rows=[&](SkeletalFixture& t) {
        const std::array<std::array<uint32_t,3>,5> rows={{{0x8010EF70u,0x8010F070u,0x8010F270u},
            {0x8010E2C4u,0x8010E4C4u,0x8010E7F4u},{0x8010EECCu,0u,0u},{0x8010F950u,0u,0u},
            {0x8010F000u,0u,0u}}};
        for(uint32_t mode=0;mode<rows.size();++mode)
            for(uint32_t i=0;i<3;++i)put(t.f.rdram,appearance_table+mode*12+i*4,rows[mode][i]);
    };
    // DF660/DF720 staging: enable, mode byte at C948+0xC9, packed record at C948[11].
    const auto appearance=[&](SkeletalFixture& t,uint32_t enable,uint32_t mode) {
        put(t.f.rdram,0x8012C838,enable);put_byte(t.f.rdram,0x8012CA11,uint8_t(mode));
        put(t.f.rdram,0x8012C974,enable?packed_record:0u);
    };
    // Retired pre-callback inputs: copy triggers, word74 bits24..27 (incl. bit25),
    // byte78 bit1 and byte7B mode bits.
    const auto actor_inputs=[&](SkeletalFixture& t) {
        auto* rdram=t.f.rdram;
        put(rdram,palette_actor+0x70,triggers[2]);put(rdram,palette_actor+0x74,0x0F000000u);
        put_byte(rdram,palette_actor+0x78,2);put_byte(rdram,palette_actor+0x7B,0xC0);
    };
    // Both asset shapes, every qualified mode: all bones and hybrids emit and close.
    // Row0 (0x707, Klungo shape) uses 801017D0; row1 (0x671, minion shape) uses 801015D0.
    for(unsigned shape=0;shape<2;++shape)
        for(uint32_t mode=1;mode<=3;++mode){
            SkeletalFixture t(skeletal_rows[shape]);copy_witness(t);appearance_rows(t);actor_inputs(t);
            stage(t,callbacks[1u-shape],copies[shape]);appearance(t,1,mode);
            t.admitted();
            require(word(t.f.rdram,ac+44)==TOOIE_BRIDGE_ID_BASE,"qualified appearance keeps the first identity");
            require(t.field(offsetof(TooieRenderBridgeState,extra_bytes))==t.paid(),"appearance adds no payment");
            for(uint32_t i=0;i<t.row.bones;++i)t.visit(i,i<t.row.hybrids);
            require(t.finish()==0,"every bone and hybrid closes under a qualified appearance row");++cases;
        }
    // Appearance on, mode changes and off again keep one identity and one block.
    {SkeletalFixture t(skeletal_rows[1]);copy_witness(t);appearance_rows(t);uint32_t first=0;
        const std::array<uint32_t,5> modes={0u,1u,2u,3u,0u};
        for(uint32_t epoch=1;epoch<=5;++epoch){
            if(epoch>1)t.prepare(epoch);
            const uint32_t mode=modes[epoch-1];
            stage(t,callbacks[0],mode?copies[epoch&1u]:own_list(t));
            appearance(t,mode?1u:0u,mode);
            t.admitted();
            const uint32_t id=word(t.f.rdram,ac+44);
            if(epoch==1)first=id;
            require(id==first && t.field(offsetof(TooieRenderBridgeState,next_block))==28,
                "appearance transitions keep one identity and one block");
            require(!t.finish(),"appearance transition closes");
        }
        ++cases;}
    // Retired actor inputs alone no longer refuse.
    {SkeletalFixture t;actor_inputs(t);t.admitted();
        require(!t.finish(),"pre-callback actor inputs are not admission predicates");++cases;}
    // Unqualified appearance refuses as OVERRIDE bit1.
    for(unsigned which=0;which<8;++which){
        SkeletalFixture t(skeletal_rows[1]);copy_witness(t);appearance_rows(t);auto* rdram=t.f.rdram;
        uint32_t enable=1,mode=1;
        if(which==0)mode=0;                                                // Type0 ST row, not this route.
        if(which==1)mode=4;                                                // Mode4: no conclusion.
        if(which==2)mode=0xFF;                                             // Unknown mode byte.
        if(which==3)enable=2;                                              // DF150 tests C838==1 only.
        if(which==4)put(rdram,appearance_table+12+4,0x8010E4C8u);          // Mode1 worker changed.
        if(which==5){mode=2;put(rdram,appearance_table+24+4,0x8010E4C4u);} // Mode2 gains a worker.
        if(which==6){mode=3;put(rdram,appearance_table+36,0x8010F954u);}   // Mode3 initializer changed.
        if(which==7)put(rdram,appearance_table+12+8,0u);                   // Mode1 cleanup missing.
        stage(t,callbacks[0],copies[0]);appearance(t,enable,mode);
        t.refused(TOOIE_BRIDGE_OVERRIDE|(0x2u<<8));++cases;
    }
    // Enable with mode byte 0 refuses only bit1; actor byte74 is no predicate.
    {SkeletalFixture t;put(t.f.rdram,0x8012C838,1);put_byte(t.f.rdram,palette_actor+0x74,1);
        t.refused(TOOIE_BRIDGE_OVERRIDE|(0x2u<<8));++cases;}
    // A worker already present at DEFA0 was not installed by this draw: bit2.
    {SkeletalFixture t(skeletal_rows[1]);copy_witness(t);appearance_rows(t);
        stage(t,callbacks[0],copies[0]);appearance(t,1,1);put(t.f.rdram,0x8012CEC4,0x8010E4C4u);
        t.refused(TOOIE_BRIDGE_OVERRIDE|(0x4u<<8));++cases;}
    // Remaining structural overrides are still named together: C820 bit0, post-callback bit4.
    {SkeletalFixture t(skeletal_rows[1]);copy_witness(t);appearance_rows(t);
        stage(t,callbacks[0],copies[0]);appearance(t,1,3);
        put(t.f.rdram,0x8012C820,1);put(t.f.rdram,0x8012C9B0,0x80101900u);
        t.refused(TOOIE_BRIDGE_OVERRIDE|((0x1u|0x10u)<<8));++cases;}
    // The hard 6400-byte budget is unchanged: exact boundary admits, one byte more refuses.
    for(uint32_t over=0;over<2;++over){
        SkeletalFixture t(skeletal_rows[1]);copy_witness(t);appearance_rows(t);
        stage(t,callbacks[0],copies[1]);appearance(t,1,2);
        put(t.f.rdram,state+offsetof(TooieRenderBridgeState,seen_epoch),1);
        put(t.f.rdram,state+offsetof(TooieRenderBridgeState,extra_bytes),6400-t.paid()+over);
        if(over){t.refused(TOOIE_BRIDGE_CAPACITY);++cases;continue;}
        t.admitted();
        require(t.field(offsetof(TooieRenderBridgeState,extra_bytes))==6400,"appearance admission fills the hard budget exactly");
        require(!t.finish(),"boundary admission closes");++cases;
    }
    return cases;
}


#endif
// Title53 uses the actual CPU-root and DE448 bodies. All original tail bytes and
// getter results remain explicit doubles; no original callback or RT64 is executed.
constexpr uint32_t title_private=0x802B1000u;
void title53_suffix(uint8_t* rdram,uint32_t at) {
    for(uint32_t offset=0;offset<184;offset+=4)put(rdram,at+offset,0x13572468);
    put(rdram,at,0xDA380002);put(rdram,at+4,palette_mtx_pool); // Original header0 root load.
    for(uint32_t i=0;i<15;++i) {
        put(rdram,at+8+i*8,0xDE000000);put(rdram,at+12+i*8,palette_model+424+i*8);
    }
    put(rdram,at+128,0xD8380002);put(rdram,at+132,64); // Original modelview pop.
    // The remaining48 bytes stand for the conservative scalar remainder allowance.
    put(rdram,player_buffers,at+184);
    put(rdram,player_buffers+4,word(rdram,player_buffers+4)+64);
    for(uint32_t address:{0x8012C9A8u,0x8012C968u,0x8012C960u,0x8012CA18u})put(rdram,address,0);
}
struct Title53Fixture {
    SkeletalFixture t{SkeletalRow{0x65E,14712,424,4096,6936,14952,19,0,0}};
    Title53Fixture(){title_private_result=title_private;title_private_getters=0;prepare(1);}
    ~Title53Fixture(){title_boundary=false;}
    void prepare(uint32_t epoch,uint32_t view=0,uint32_t phase=0) {
        t.prepare(epoch,view,phase);
        auto* rdram=t.f.rdram;
        MEM_H(0x16,guest(palette_descriptor))=0;
        put(rdram,ac+12,0x065E0000);put(rdram,0x8012CA18,0x65E);
        put(rdram,palette_actor+0x74,0x10000000);MEM_H(0x76,guest(palette_actor))=0x53u<<7;
        MEM_H(0x86,guest(palette_actor))=1;
        put(rdram,palette_data,0x00040317);put(rdram,palette_data+0x40,0x80803D0C);
        MEM_H(0xC6,guest(title_private))=0;
        MEM_H(10,guest(t.effective_model))=0;put(rdram,t.effective_model+0x28,3024);
        put(rdram,player_dispatch+17*4,0x800DD65C);
    }
    uint32_t begin(uint32_t camera=0xA0000001) {
        return t.invoke(candidate::tooie_actor_cpu_root_begin,player_buffers,palette_cursor_slot,t.effective_model,camera);
    }
    uint32_t admitted(uint32_t admitted=TOOIE_BRIDGE_TITLE53_ADMITTED) {
        const uint32_t root=begin();auto* r=t.f.rdram;
        require(root && word(r,palette_cursor_slot)==t.f.cursor+24,"title CPU root reserves one public push");
        const std::array<uint32_t,6> expected={0xE0525464,0x10000064,0x6400000C,root+1,0x02014AA9,0};
        for(uint32_t i=0;i<expected.size();++i)require(word(r,t.f.cursor+i*4)==expected[i],"exact CPU vertex-AUTO public group");
        require(word(r,ac+48)==admitted && t.field(offsetof(TooieRenderBridgeState,extra_bytes))==32,
            "title and ordinary CPU routes retain exact32-byte debit");
        return root;
    }
    void finish(bool tripwire=false) {
        auto* r=t.f.rdram;
        const uint32_t at=word(r,palette_cursor_slot);
        title53_suffix(r,at);
        if(tripwire)put(r,state+offsetof(TooieRenderBridgeState,births),2);
        t.invoke(candidate::tooie_actor_palette_end,player_buffers);
        require(word(r,at)==0xDA380002 && word(r,at+128)==0xD8380002,
            "close preserves supplied original load and pop");
        require(word(r,at+184)==0x6400000D && word(r,at+188)==1 &&
            word(r,player_buffers)==at+192,"title pays its one pop after original cleanup");
        require(!t.field(offsetof(TooieRenderBridgeState,active)) && !word(r,ac+56),"title close clears paid active state");
        require(t.field(offsetof(TooieRenderBridgeState,tripwire_count))==unsigned(tripwire),"owner tripwire is retained at paid close");
        t.f.cursor=at+192;
    }
    void refused(uint32_t reason) {
        auto* r=t.f.rdram;
        std::array<uint32_t,18> before{};
        for(uint32_t i=0;i<before.size();++i)before[i]=word(r,t.rec()+i*4);
        const uint32_t blocks=t.field(offsetof(TooieRenderBridgeState,next_block));
        const uint32_t extra=t.field(offsetof(TooieRenderBridgeState,seen_epoch))==t.field(offsetof(TooieRenderBridgeState,epoch))?
            t.field(offsetof(TooieRenderBridgeState,extra_bytes)):0;
        const uint32_t at=word(r,palette_cursor_slot),gfx=word(r,player_buffers),mtx=word(r,player_buffers+4);
        for(uint32_t offset=0;offset<32;offset+=4)put(r,at+offset,0xA55A1234);
        require(!begin(),"unsupported title input refuses before group");
        require(!word(r,ac+56) && !t.field(offsetof(TooieRenderBridgeState,active)),"title refusal leaves no open group");
        require(t.field(offsetof(TooieRenderBridgeState,silo_last_reason))==reason,"title refusal identifies exact boundary");
        require(t.field(offsetof(TooieRenderBridgeState,next_block))==blocks &&
            t.field(offsetof(TooieRenderBridgeState,extra_bytes))==extra && word(r,palette_cursor_slot)==at &&
            word(r,player_buffers)==gfx && word(r,player_buffers+4)==mtx,"title refusal spends no IDs/cursors/matrices");
        for(uint32_t i=0;i<before.size();++i)require(word(r,t.rec()+i*4)==before[i],"title precommit refusal preserves all history");
        for(uint32_t offset=0;offset<32;offset+=4)require(word(r,at+offset)==0xA55A1234,"title refusal emits zero commands");
    }
};
#ifndef TOOIE_BK_B4_FIXTURE
// L06b-7: the CPU-root body these title53 groups drive exists only in the BK-OFF image, where they run (B4).
void title_boundary_draw(uint8_t* rdram,recomp_context* c) {
    require(word(rdram,ac)==title_boundary_expected,"real DE448 captures only exact title53 owner");
    if(!title_boundary_expected) {
        put(rdram,0x8012C9A8,0);clobber(c);c->r2=guest(returned);return;
    }
    auto admit=*c;admit.r4=guest(player_buffers);admit.r5=guest(palette_cursor_slot);
    admit.r6=guest(palette_model);admit.r7=guest(0xA0000001);
    candidate::tooie_actor_cpu_root_begin(rdram,&admit);
    require(low(admit.r2)!=0 && word(rdram,ac+48)==TOOIE_BRIDGE_TITLE53_ADMITTED,
        "actual direct-world envelope reaches title CPU admission");
    title53_suffix(rdram,word(rdram,palette_cursor_slot));
    clobber(c);c->r2=guest(returned);
}
[[maybe_unused]] unsigned title53_fixtures() {
    unsigned cases=0;
    {Title53Fixture f;f.admitted();f.finish();++cases;}
    // Exact metadata prefix, producer dispatch and subtype: all are independent of model header.
    for(unsigned which=0;which<6;++which) {
        Title53Fixture f;auto* rdram=f.t.f.rdram;
        if(which==0)MEM_H(2,guest(palette_data))=0x318;
        if(which==1)put(rdram,palette_data+0x38,0);
        if(which==2)put(rdram,palette_data+0x40,0);
        if(which==3)MEM_H(0x76,guest(palette_actor))=0x54u<<7;
        if(which==4)put(rdram,palette_actor+0x74,word(rdram,palette_actor+0x74)&~0x10000000u);
        if(which==5){MEM_H(0x16,guest(palette_descriptor))=1;f.refused(TOOIE_BRIDGE_BIRTH);++cases;continue;}
        f.refused(TOOIE_BRIDGE_BIRTH);++cases;
    }
    for(int16_t handle:{int16_t(0),int16_t(-1),int16_t(250)}) {
        Title53Fixture f;auto* rdram=f.t.f.rdram;MEM_H(0x86,guest(palette_actor))=handle;
        f.refused(TOOIE_BRIDGE_ANIMATION);require(!title_private_getters,"invalid private handle never enters unchecked getter");++cases;
    }
    for(unsigned which=0;which<3;++which) {
        Title53Fixture f;auto* rdram=f.t.f.rdram;
        if(which==0)title_private_result=0x80FFFFFC;
        if(which==1)MEM_H(0xC6,guest(title_private))=1;
        if(which==2)put(rdram,0x8012C964,palette_model+4096+16);
        f.refused(which==2?TOOIE_BRIDGE_MODEL_DATA:TOOIE_BRIDGE_OVERRIDE);++cases;
    }
    for(unsigned which=0;which<6;++which) {
        Title53Fixture f;auto* rdram=f.t.f.rdram;
        if(which==0)put(rdram,0x8012C968,0x80FFFFC0);
        if(which==1)put(rdram,palette_pose,18);
        if(which==2)put(rdram,palette_pose+4,18);
        if(which==3)put(rdram,palette_pose+0x50,0x7F800000);
        if(which==4)MEM_H(0x10,guest(palette_descriptor))=0;
        if(which==5)put(rdram,0x8012CE58,0x7FC00000);
        f.refused(TOOIE_BRIDGE_ANIMATION);++cases;
    }
    {Title53Fixture f;auto* r=f.t.f.rdram;put(r,0x8012C9A8,0);
        require(!f.begin() && !f.t.field(offsetof(TooieRenderBridgeState,silo_calls)) &&
            !f.t.field(offsetof(TooieRenderBridgeState,next_block)) && !title_private_getters,
            "unstaged callback remains a silent nonclass draw");++cases;}
    for(int16_t handle:{int16_t(-1),int16_t(250)}) {
        Title53Fixture f;auto* rdram=f.t.f.rdram;MEM_H(0x10,guest(palette_descriptor))=handle;
        f.refused(TOOIE_BRIDGE_ANIMATION);require(!palette_pose_getters,"invalid pose handle never enters unchecked AE080");++cases;
    }
    for(unsigned animated=0;animated<2;++animated) {
        Title53Fixture f;auto* rdram=f.t.f.rdram;
        MEM_H(0x10,guest(palette_descriptor))=0;put(rdram,0x8012C840,palette_pose);put(rdram,0x8012C824,animated);
        f.admitted();require(word(rdram,f.t.rec()+offsetof(TooieActorPaletteSlot,pose))==animated+1,
            "title scratch renewal key uses input class, never scratch address or values");f.finish();++cases;
    }
    {Title53Fixture f;const uint32_t first=f.admitted();f.finish();f.prepare(2);
        put(f.t.f.rdram,palette_pose+0x50,0x40000000);
        require(f.admitted()==first,"changed finite numerical pose preserves AUTO correspondence ID");f.finish();++cases;}
    for(unsigned which=0;which<3;++which) {
        Title53Fixture f;f.admitted();f.finish();f.prepare(1);auto* rdram=f.t.f.rdram;
        if(which==0){MEM_H(0x10,guest(palette_descriptor))=0;put(rdram,0x8012C840,palette_pose);}
        if(which==1){f.t.effective_model+=0x10000;f.prepare(1);}
        if(which==2)put(rdram,f.t.rec()+offsetof(TooieActorPaletteSlot,flags),TOOIE_BRIDGE_CPU_ROOT_ROUTE);
        f.refused(TOOIE_BRIDGE_DUPLICATE);++cases;
    }
    {Title53Fixture f;const uint32_t first=f.admitted();f.finish();f.prepare(2);auto* rdram=f.t.f.rdram;
        MEM_H(0x10,guest(palette_descriptor))=0;put(rdram,0x8012C840,palette_pose);
        require(f.admitted()==first+512,"new-epoch input-class change reserves one existing block");f.finish();++cases;}
    {Title53Fixture f;f.admitted();f.finish();f.prepare(1);f.refused(TOOIE_BRIDGE_DUPLICATE);++cases;}
    {Title53Fixture f;f.admitted();f.finish();f.prepare(1);put(f.t.f.rdram,0x8012C9A8,0x801015D0);
        f.refused(TOOIE_BRIDGE_DUPLICATE);++cases;}
    {Title53Fixture f;f.admitted();f.finish();f.prepare(2);auto* r=f.t.f.rdram;
        require(!f.begin(0xA0000002) && f.t.field(offsetof(TooieRenderBridgeState,silo_last_reason))==TOOIE_BRIDGE_VIEW,
            "title camera cut refuses");
        require(word(r,f.t.rec()+offsetof(TooieActorPaletteSlot,camera_cut_views))==1,"title cut history latched");
        const uint32_t blocks=f.t.field(offsetof(TooieRenderBridgeState,next_block));
        put(r,f.t.rec()+offsetof(TooieActorPaletteSlot,pose),0);f.refused(TOOIE_BRIDGE_DUPLICATE);
        require(word(r,f.t.rec()+offsetof(TooieActorPaletteSlot,camera_cut_views))==1 &&
            f.t.field(offsetof(TooieRenderBridgeState,next_block))==blocks,"class change cannot erase same-epoch cut");++cases;}
    for(unsigned which=0;which<2;++which) {
        Title53Fixture f;if(which==0)skeletal_view=1;else skeletal_phase=1;
        f.refused(which==0?TOOIE_BRIDGE_VIEW:TOOIE_BRIDGE_PHASE);++cases;
    }
    for(unsigned room:{216u,208u}) {
        Title53Fixture f;f.t.f.cursor=player_pool+TOOIE_BRIDGE_LOWER_BYTES-room;put(f.t.f.rdram,palette_cursor_slot,f.t.f.cursor);
        if(room==216){f.admitted();f.finish();require(word(f.t.f.rdram,player_buffers)==player_pool+TOOIE_BRIDGE_LOWER_BYTES,"216-byte complete title envelope exact boundary");}
        else f.refused(TOOIE_BRIDGE_CAPACITY);++cases;
    }
    for(unsigned room:{64u,0u}) {
        Title53Fixture f;put(f.t.f.rdram,player_buffers+4,palette_mtx_pool+0x12C00-room);
        if(room){f.admitted();f.finish();}else f.refused(TOOIE_BRIDGE_CAPACITY);++cases;
    }
    {Title53Fixture f;put(f.t.f.rdram,state+offsetof(TooieRenderBridgeState,next_block),TOOIE_BRIDGE_BLOCK_COUNT);
        f.refused(TOOIE_BRIDGE_BLOCKS);++cases;}
    {Title53Fixture f;f.admitted();f.finish(true);++cases;}
    // A foreign producer/private state still does not change the accepted ordinary65E route.
    {Title53Fixture f;auto* rdram=f.t.f.rdram;put(rdram,0x8012C9A8,0x801015D0);
        MEM_H(2,guest(palette_data))=0x999;MEM_H(0x86,guest(palette_actor))=0;
        f.admitted(TOOIE_BRIDGE_CPU_ROOT_ADMITTED);require(!title_private_getters,"ordinary65E adds no private getter");f.finish();++cases;}
    for(unsigned which=0;which<8;++which) {
        Title53Fixture f;auto* r=f.t.f.rdram;auto* rdram=r;title_boundary=true;skeletal_boundary=0;
        title_boundary_expected=which?0:palette_actor;
        if(which==1)MEM_H(2,guest(palette_data))=0x318;
        if(which==2)put(r,palette_data+0x38,0);
        if(which==3)put(r,palette_data+0x40,0);
        if(which==4)MEM_H(0x76,guest(palette_actor))=0x54u<<7;
        if(which==5)put(r,palette_actor+0x74,word(r,palette_actor+0x74)&~0x10000000u);
        if(which==6)MEM_H(0x16,guest(palette_descriptor))=1;
        put(r,ac,0);put(r,state+offsetof(TooieRenderBridgeState,silo_depth),0);
        auto c=context();c.r4=guest(palette_actor+(which==7?8u:4u));c.r5=guest(rotation);c.r6=guest(0x3F800000);c.r7=0;
        put(r,stack+0x10,palette_model);const auto before=c;candidate::func_800DE448(r,&c);saved(c,before);
        require(!word(r,ac) && !word(r,ac+48) && !word(r,ac+56) && !f.t.field(offsetof(TooieRenderBridgeState,silo_depth)),
            "real DE448 pairs title close then clears the captured owner");
        if(!which)require(word(r,f.t.f.cursor+208)==0x6400000D && word(r,player_buffers)==f.t.f.cursor+216,
            "real title boundary executes the new admitted6 close");
        else require(!f.t.field(offsetof(TooieRenderBridgeState,next_block)) && !title_private_getters,
            "excluded title capture never reaches private getter or label allocation");
        ++cases;
    }
    // CPU-root renewal reuses the shared union slot. A previous palette/NPC/Silo
    // occupant's controller must not survive it, or that controller's setters retire this owner.
    {Title53Fixture f;auto* r=f.t.f.rdram;
        put(r,f.t.rec()+offsetof(TooieActorPaletteSlot,controller),palette_controller);
        f.admitted();f.finish();
        require(word(r,f.t.rec()+offsetof(TooieActorPaletteSlot,controller))==0,
            "CPU-root renewal clears an inherited union controller word");
        f.t.invoke(candidate::func_8008B10C,palette_controller,0x3E800000);
        require((word(r,f.t.rec()+8)&TOOIE_BRIDGE_BLOCK_VALID)!=0,
            "a previous occupant's controller setter cannot retire the CPU-root owner");
        ++cases;}
    return cases;
}
#endif
