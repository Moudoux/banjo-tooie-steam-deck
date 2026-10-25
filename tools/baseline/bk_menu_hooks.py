"""Option-owned zoombox model9F1 and transition masks. Named overlay identity plus exact non-delay PC."""
import re
HOOKS = [
 ('gcnewoption_entrypoint_5',0x80800A90,'baseline_bk_menu_init'),
 ('gcnewoption_entrypoint_7',0x80800CA8,'baseline_bk_menu_free'),
 ('gcnewoption_entrypoint_1',0x8080078C,'baseline_bk_menu_enter'),
 ('gcnewoption_entrypoint_1',0x808009BC,'baseline_bk_menu_leave'),
 ('gcnewoption_entrypoint_1',0x808007F4,'baseline_bk_menu_primary'),
 ('gcnewoption_entrypoint_1',0x808007FC,'baseline_bk_menu_primary_end'),
 ('gcnewoption_entrypoint_1',0x8080087C,'baseline_bk_menu_upper'),
 ('gcnewoption_entrypoint_1',0x80800884,'baseline_bk_menu_upper_end'),
 ('gcnewoption_entrypoint_1',0x8080094C,'baseline_bk_menu_lower'),
 ('gcnewoption_entrypoint_1',0x80800954,'baseline_bk_menu_lower_end'),
 ('gcnewoption_entrypoint_1',0x808009B0,'baseline_bk_menu_extra'),
 ('gcnewoption_entrypoint_1',0x808009B8,'baseline_bk_menu_extra_end'),
 ('func_80801424_gczoombox',0x8080148C,'baseline_bk_menu_box_model'),
 ('func_80801424_gczoombox',0x80801494,'baseline_bk_menu_box_model_end'),
 ('fxdialog_entrypoint_3',0x80800174,'baseline_bk_menu_model_begin'),
 ('fxdialog_entrypoint_3',0x8080017C,'baseline_bk_menu_model_end'),
 # gctransitionDll entry0 (80800324): entry, the common exit before 80800870 and the three mask DE448
 # calls with their return continuations (TRANSITION_TAG_CONTRACT).
 ('gctransitionDll_entrypoint_0',0x80800324,'baseline_bk_transition_enter'),
 ('gctransitionDll_entrypoint_0',0x808003D8,'baseline_bk_transition_projection_begin'),
 ('gctransitionDll_entrypoint_0',0x808003E0,'baseline_bk_transition_projection_end'),
 ('gctransitionDll_entrypoint_0',0x80800870,'baseline_bk_transition_leave'),
 ('gctransitionDll_entrypoint_0',0x80800484,'baseline_bk_transition_model'),
 ('gctransitionDll_entrypoint_0',0x8080048C,'baseline_bk_transition_model_end'),
 ('gctransitionDll_entrypoint_0',0x8080059C,'baseline_bk_transition_model'),
 ('gctransitionDll_entrypoint_0',0x808005A4,'baseline_bk_transition_model_end'),
 ('gctransitionDll_entrypoint_0',0x808006CC,'baseline_bk_transition_model'),
 ('gctransitionDll_entrypoint_0',0x808006D4,'baseline_bk_transition_model_end'),
 # Transition lifetime: config/state helper (may free its object), core free+compact, registry reset.
 ('func_808000D8_gctransitionDll',0x808000D8,'baseline_bk_transition_config'),
 ('func_800C9214',0x800C9214,'baseline_bk_transition_free'),
 ('func_800C929C',0x800C929C,'baseline_bk_transition_reset'),
]
# The per-PC rule sets below are keyed by PC alone; every hooked PC is distinct across functions.
assert len({pc for _,pc,_ in HOOKS})==len(HOOKS)
# 9B8 is also reached without the optional extra draw: end is activation-matched.
# 870 is entered by every exit of entry0, so it is a label.
LABELS={0x808009B8,0x808009BC,0x80800870}
ENTRIES={0x80800A90,0x80800CA8,0x8080078C,0x80800324,0x808000D8,0x800C9214,0x800C929C}
# Ends whose begin must precede them on every path: each is a pure return continuation of its own
# call, never a branch target that could skip the begin.
RETURN_ONLY={0x808003E0,0x80801494,0x8080017C,0x8080048C,0x808005A4,0x808006D4}
# Before-call seams read these values; none may be produced by the call's delay slot (pc+4).
READS={0x808003D8:('ctx->r29 =','MEM_W(0X74, ctx->r29) =','MEM_W(ctx->r29, 0X74) ='),0x808007F4:('ctx->r4 =','ctx->r19 =','ctx->r21 ='),
       0x8080087C:('ctx->r4 =','ctx->r16 =','ctx->r17 =','ctx->r21 ='),
       0x8080094C:('ctx->r4 =','ctx->r16 =','ctx->r17 =','ctx->r21 ='),
       0x808009B0:('ctx->r4 =','ctx->r21 ='),
       0x8080148C:('ctx->r4 =','ctx->r16 ='),
       0x80800174:('ctx->r4 =','MEM_W(0X18, ctx->r29) =')}
# Before-call seams whose call must be exactly this original target (pc..pc+8 of the generated body).
TARGETS={0x808003D8:'func_800E44FC(rdram, ctx);',0x80800484:'func_800DE448(rdram, ctx);',0x8080059C:'func_800DE448(rdram, ctx);',
         0x808006CC:'func_800DE448(rdram, ctx);'}
# Thunks match activations against these exact prologue frames: entry1 leave at sp+0x38, and the
# transition model seams against entry0's entry sp at sp+0x70.
FRAMES={'gcnewoption_entrypoint_1':0x38,'gctransitionDll_entrypoint_0':0x70}
def add_hooks(hooks,selected):
 assert type(selected) is bool
 if not selected:return
 for func,pc,name in HOOKS:
  assert not any(h['func']==func and h.get('before_vram')==pc for h in hooks),(func,hex(pc))
  hooks.append(dict(func=func,before_vram=pc,text=name+'(rdram, ctx);'))
def strip_and_verify(after,selected):
 assert type(selected) is bool
 if selected:
  for func,size in FRAMES.items():
   frames=re.findall(r'ctx->r29 = ADD32\(ctx->r29, -0X([0-9A-F]+)\);',after[func])
   assert frames and int(frames[0],16)==size,(func,'prologue frame',frames)
 for func,pc,name in HOOKS:
  call='    '+name+'(rdram, ctx);\n'
  # The three transition model seams share one thunk name: count exact call+marker pairs per PC.
  marker='    // 0x%08X:'%pc
  if selected:
   assert after[func].count(call+marker)==1,(func,hex(pc),'exact PC')
  else:
   assert call not in after[func],(func,name)
  if not selected:continue
  if pc in LABELS:
   assert re.search(r'^[ \t]*L_%08X:\n'%pc+re.escape(call+marker),after[func],re.M),(func,'label must precede hook')
  if pc in ENTRIES:
   assert after[func].index(marker)==after[func].index('    // 0x'),(func,'entry')
  if pc in RETURN_ONLY:
   assert not re.search(r'^[ \t]*L_%08X:'%pc,after[func],re.M),(func,hex(pc),'end must not be a branch target')
  if pc in READS:
   delay=after[func].index('    // 0x%08X:'%(pc+4))
   following=after[func].find('    // 0x',delay+1)
   segment=after[func][delay:following if following>=0 else len(after[func])]
   assert not any(w in segment for w in READS[pc]),(func,hex(pc),'value read by the hook is set in the delay slot')
  if pc in TARGETS:
   start=after[func].index(marker)
   end=after[func].index('    // 0x%08X:'%(pc+8),start)
   assert TARGETS[pc] in after[func][start:end],(func,hex(pc),'original call target')
  after[func]=after[func].replace(call+marker,marker,1)
 if selected:
  for func in {f for f,_,_ in HOOKS}:
   assert not any(name+'(rdram, ctx);' in after[func] for _,_,name in HOOKS),(func,'every hook stripped')
 return after
