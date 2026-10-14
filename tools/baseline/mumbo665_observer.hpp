// Notes only. Include after feather_sc/feather_bridge/feather_ready in observer block.
namespace {
void mumbo665_model(uint8_t* rdram,recomp_context* ctx) {
 static unsigned rows=0;if(rows>=4 || !feather_ready())return;
 const uint32_t sp=uint32_t(ctx->r29),m=uint32_t(ctx->r25);if((sp&3u) || !shadow_readable(sp,0x110) || (m&3u) || !shadow_readable(m,44))return;
 const uint32_t a=feather_sc(rdram,offsetof(TooieSiloContext,actor)),d=feather_sc(rdram,offsetof(TooieSiloContext,descriptor));
 const uint32_t slot=feather_sc(rdram,offsetof(TooieSiloContext,slot)),birth=feather_sc(rdram,offsetof(TooieSiloContext,birth)),pair=feather_sc(rdram,offsetof(TooieSiloContext,pair));
 if(slot>=200u || d!=0x80132ED0u+slot*0x30u || !birth || birth>TOOIE_BRIDGE_MAX_BIRTH || (a&3u) || !shadow_readable(a,0x9C))return;
 if(feather_bridge(rdram,offsetof(TooieRenderBridgeState,silo_depth))!=1u || feather_bridge(rdram,offsetof(TooieRenderBridgeState,births)+4*slot)!=birth ||
    feather_sc(rdram,offsetof(TooieSiloContext,epoch))!=shadow_word(rdram,1032) || feather_sc(rdram,offsetof(TooieSiloContext,epoch))!=feather_bridge(rdram,offsetof(TooieRenderBridgeState,epoch)) || feather_sc(rdram,offsetof(TooieSiloContext,buffers))!=uint32_t(MEM_W(0xF8,ctx->r29)))return;
 if(uint32_t(MEM_W(0,(gpr)(int32_t)a))!=d || MEM_HU(0x14,(gpr)(int32_t)d)!=0x665u ||
    pair!=((uint32_t(MEM_HU(0x14,(gpr)(int32_t)d))<<16)|MEM_HU(0x16,(gpr)(int32_t)d)) ||
    MEM_W(0x60,(gpr)(int32_t)0x8012C948u)!=0x801017D0u || uint32_t(MEM_W(0x64,(gpr)(int32_t)0x8012C948u))!=a || uint32_t(MEM_W(0x10C,ctx->r29))!=m)return;
 const uint32_t pose=uint32_t(MEM_W(0,(gpr)(int32_t)0x8012C968u)),table=uint32_t(MEM_W(0,(gpr)(int32_t)0x8012CF8Cu));
 const bool pose_ok=!(pose&3u)&&shadow_readable(pose,8);if((table&3u) || !shadow_readable(table,100))return;
 std::fprintf(stderr,"baseline mumbo665 row=%u epoch=%u actor=%08x descriptor=%08x slot=%u birth=%u pair=%08x view=%u phase=%u model=%08x headerA=%04x bone_section=%u deform_section=%u pose=%08x pose_readable=%u pose_count=%u pose_capacity=%u render_mode=%u render_submode=%u handler2=%08x handler17=%08x handler24=%08x main_id=%08x source_pc=800DEFA0\n",
 ++rows,shadow_word(rdram,1032),a,d,slot,birth,pair,feather_sc(rdram,offsetof(TooieSiloContext,view)),feather_sc(rdram,offsetof(TooieSiloContext,phase)),m,unsigned(MEM_HU(0xA,(gpr)(int32_t)m)),uint32_t(MEM_W(0x18,(gpr)(int32_t)m)),uint32_t(MEM_W(0x28,(gpr)(int32_t)m)),pose,unsigned(pose_ok),pose_ok?uint32_t(MEM_W(0,(gpr)(int32_t)pose)):0,pose_ok?uint32_t(MEM_W(4,(gpr)(int32_t)pose)):0,uint32_t(MEM_W(0,(gpr)(int32_t)0x8012C838u)),unsigned(MEM_BU(0,(gpr)(int32_t)0x8012CA11u)),uint32_t(MEM_W(8,(gpr)(int32_t)table)),uint32_t(MEM_W(68,(gpr)(int32_t)table)),uint32_t(MEM_W(96,(gpr)(int32_t)table)),shadow_word(rdram,1112));
 std::fflush(stderr);
}
}
// Same existing proposed DEFA0 dispatcher: call mumbo665_model(rdram,ctx) BEFORE feather-specific gates.
