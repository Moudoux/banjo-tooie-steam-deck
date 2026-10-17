// Production translated FB04 providers versus frozen base-raw originals.
// Derived from the private component harness; no gameplay acceptance claim.
#include "recomp.h"
#include "fixture_facts.h"
#include "fixture_functions.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(FB04_NEGATIVE_GUARD)
#include "candidate_guard_mutant.inc"
#else
#include "candidate_fixture.inc"
#endif
#include "original_fixture.inc"

namespace {
constexpr uint32_t P=0x80220000, Q=0x80224000, MOVED=0x80228000;
constexpr uint32_t STACK=0x80818000, TABLE=0x80135490;
constexpr uint32_t END_CB=0x80700100, INIT_CB=0x80700200;
constexpr uint32_t NONE=0, AWAIT=1, ARMED=2, PENDING=3;
constexpr size_t RAM=16*1024*1024;
std::vector<uint8_t> memory(RAM);
uint8_t* rdram=memory.data();
struct Event { std::string name; uint32_t a,b,c; bool operator==(const Event&) const = default; };
std::vector<Event> trace;
struct View { uint32_t self,current,owner,last,phase; };
std::vector<View> init_views;
struct Key { bool level=false, edge=false, suppressed=false; };
std::map<uint32_t,Key> keys;
struct Action { bool init; uint32_t state; uint32_t nested; uint32_t rewrite; bool consumed=false; };
std::vector<Action> actions;
bool candidate=true, end_callback=false, init_callback=false;
bool moving_loop=false;
uint32_t setting=0, shoes=0;
float delta=.25f;
unsigned invocations=0, abi_checks=0, groups=0, off_cases=0, on_cases=0, getter_calls=0;
unsigned getter_begin=0;
std::map<uint32_t,uint32_t> moves;
std::vector<std::array<uint32_t,3>> move_seen;
std::vector<std::array<uint32_t,2>> move_publication;

void require(bool b,const char* why) { if(!b) throw std::runtime_error(why); }
gpr guest(uint32_t v) { return S32(v); }
uint32_t word(uint32_t p) { return uint32_t(MEM_W(0,guest(p))); }
void word(uint32_t p,uint32_t v) { MEM_W(0,guest(p))=v; }
uint32_t byte(uint32_t p) { return MEM_BU(0,guest(p)); }
uint32_t state(uint32_t p) { return word(p+0x120); }
uint32_t current(uint32_t p) { return word(state(p)+4); }
uint32_t owner() { return word(FB04_SLOT_ADDR); }
uint32_t last() { return word(FB04_SLOT_ADDR+4); }
uint32_t phase() { return byte(FB04_SLOT_ADDR+8); }
void event(const char* name,uint32_t a=0,uint32_t b=0,uint32_t c=0) { trace.push_back({name,a,b,c}); }
bool family(uint32_t s) { return s==0x15 || s==0x17 || s==8 || s==0x7B || s==0x9A || s==0x9C || s==0x95; }
constexpr std::array<gpr recomp_context::*,12> saved{
 &recomp_context::r16,&recomp_context::r17,&recomp_context::r18,&recomp_context::r19,
 &recomp_context::r20,&recomp_context::r21,&recomp_context::r22,&recomp_context::r23,
 &recomp_context::r28,&recomp_context::r29,&recomp_context::r30,&recomp_context::r31};
constexpr std::array<fpr recomp_context::*,12> fsaved{
 &recomp_context::f20,&recomp_context::f21,&recomp_context::f22,&recomp_context::f23,
 &recomp_context::f24,&recomp_context::f25,&recomp_context::f26,&recomp_context::f27,
 &recomp_context::f28,&recomp_context::f29,&recomp_context::f30,&recomp_context::f31};
void clobber(recomp_context* c) {
 c->r1=0x101;c->r2=0x102;c->r3=0x103;c->r4=0x104;c->r5=0x105;c->r6=0x106;c->r7=0x107;
 c->r8=0x108;c->r9=0x109;c->r10=0x10a;c->r11=0x10b;c->r12=0x10c;c->r13=0x10d;
 c->r14=0x10e;c->r15=0x10f;c->r24=0x118;c->r25=0x119;
 c->f0.u64=c->f2.u64=c->f4.u64=c->f6.u64=c->f8.u64=0xabcdef;
 c->f10.u64=c->f12.u64=c->f14.u64=c->f16.u64=c->f18.u64=0xfedcba;
}
void seed(recomp_context& c,uint32_t p,uint32_t arg) {
 c={};unsigned i=0;
 for(auto r:saved)c.*r=guest(0x81230000+4*i++);
 i=0;for(auto r:fsaved)(c.*r).u64=0xcafebabe12340000ull+i++;
 c.r4=guest(p);c.r5=guest(arg);c.r29=guest(STACK);c.r31=guest(0x80045678);
 c.status_reg=0xff01;c.mips3_float_mode=0;c.f_odd=&c.f0.u32h;
}
// Only original declared writable fields, fixed table, slot and bounded stack.
// No full player block is allowed: adjacent player/key/input/state bytes are guards.
void memory_check(const std::vector<uint8_t>& before) {
 std::vector<std::pair<size_t,size_t>> allowed;
 auto span=[&](uint32_t p,size_t n){size_t x=p&0x1fffffff;require(x+n<=RAM,"allowed span in RDRAM");allowed.push_back({x,x+n});};
 span(STACK-4096,4096+16);span(FB04_SLOT_ADDR,FB04_SLOT_SIZE);span(TABLE,32);
 for(auto p:{P,Q,MOVED}) {span(state(p),12);span(word(p+0xa4),4);span(word(p+0xa4)+8,4);}
 std::sort(allowed.begin(),allowed.end());size_t cursor=0;
 for(auto [lo,hi]:allowed) {if(lo>cursor)require(std::memcmp(memory.data()+cursor,before.data()+cursor,lo-cursor)==0,"write outside original fields/stack/slot/table");cursor=std::max(cursor,hi);}
 require(std::memcmp(memory.data()+cursor,before.data()+cursor,RAM-cursor)==0,"write above declared allowed regions");
}
uint32_t checked(recomp_func_t* fn,recomp_context& c) {
 auto before=c;auto bytes=memory;fn(rdram,&c);++invocations;
 for(auto r:saved)require(c.*r==before.*r,"callee-saved GPR/SP/RA restored");
 for(auto r:fsaved)require((c.*r).u64==(before.*r).u64,"callee-saved FPR restored");
 require(c.f_odd==before.f_odd && c.status_reg==before.status_reg && c.mips3_float_mode==before.mips3_float_mode,"float mapping/status preserved");
 memory_check(bytes);++abi_checks;return uint32_t(c.r2);
}
uint32_t call(recomp_func_t* fn,uint32_t p=P,uint32_t arg=0) {recomp_context c;seed(c,p,arg);return checked(fn,c);}
void set(uint32_t p,uint32_t s) {call(candidate?bs_setState:bs_setState_original,p,s);}
void sample(uint32_t p=P) {call(candidate?bainput_update:bainput_update_original,p);}
uint32_t check(uint32_t p=P) {return call(candidate?func_80800710_bsbtrot:func_80800710_bsbtrot_original,p);}
void retire(uint32_t p) {call(tooie_trot_toggle_retire,p);}
void reset(bool cand=true,uint32_t on=0) {
 std::fill(memory.begin(),memory.end(),0xcd);
 for(auto p:{P,Q,MOVED}) {word(p+0x120,p+0x1000);word(p+0xa4,p+0x1100);word(p+0x164,0);
  word(state(p),0);word(state(p)+4,1);word(state(p)+8,0);word(p+0x1100,0xffffffff);word(p+0x1108,0);}
 for(unsigned i=0;i<FB04_SLOT_SIZE;++i) MEM_BU(i,guest(FB04_SLOT_ADDR))=0;
 for(unsigned i=0;i<8;++i)word(TABLE+4*i,0);
 trace.clear();init_views.clear();keys.clear();actions.clear();moves.clear();move_seen.clear();move_publication.clear();moving_loop=false;
 candidate=cand;setting=on;shoes=0;end_callback=false;init_callback=false;delta=.25f;
 getter_begin=getter_calls;
}
void entry(uint32_t p=P) {keys[p]={true,true,false};set(p,0x15);require(owner()==p&&last()==0x15&&phase()==AWAIT,"family entry awaits release");}
void arm(uint32_t p=P) {keys[p]={false,false,false};sample(p);require(phase()==ARMED,"release arms occurrence");}
void pending(uint32_t p=P) {keys[p]={true,true,false};sample(p);require(phase()==PENDING,"eligible press makes pending exit");}
struct Snapshot {std::vector<Event> events;std::array<uint32_t,7> fields;uint32_t result;unsigned getters;};
Snapshot snap(uint32_t result=0) {return {trace,{word(state(P)),current(P),word(state(P)+8),word(P+0x1100),word(P+0x1108),owner(),phase()},result,getter_calls-getter_begin};}
void equal_off(const Snapshot& a,const Snapshot& b,bool returns=false) {
 require(a.events==b.events,"OFF original call order/arguments exact");
 require(a.fields==b.fields,"OFF original state/input data exact");
 auto count=[&](const char* name){return std::count_if(a.events.begin(),a.events.end(),[&](const Event& e){return e.name==name;});};
 auto expected=count("init_lookup")-count("init")+count("released")+count("enable");
 require(a.getters==0&&b.getters==expected,"OFF getter once per commit, input sample or unguarded 710");
 if(returns)require(a.result==b.result,"OFF original return exact");++off_cases;
}
void callback(bool init,uint8_t*,recomp_context* c) {
 uint32_t p=uint32_t(c->r4), s=current(p);
 event(init?"init":"end",p,s,word(state(p)+8));
 if(init)init_views.push_back({p,s,owner(),last(),phase()});
 // Actions are consumed before re-entry; selectors themselves remain double lookups.
 for(size_t i=0;i<actions.size();++i)if(!actions[i].consumed&&actions[i].init==init&&actions[i].state==s) {
  auto action=actions[i];actions[i].consumed=true;
  if(action.nested){c->r4=guest(p);c->r5=guest(action.nested);checked(candidate?bs_setState:bs_setState_original,*c);}
  if(action.rewrite)word(state(p)+8,action.rewrite);
 }
 clobber(c);
}
void end_cb(uint8_t* m,recomp_context* c){callback(false,m,c);}
void init_cb(uint8_t* m,recomp_context* c){callback(true,m,c);}
void configure_scenario(unsigned scenario) {
 end_callback=init_callback=scenario>=2;
 if(scenario==3)actions.push_back({false,1,0x17,0});
 if(scenario==4)actions.push_back({true,0x15,8,0});
 if(scenario==5)actions.push_back({false,1,0,0x17});
}
}

extern "C" recomp_func_t* get_function(int32_t vram) {
 if(uint32_t(vram)==END_CB)return end_cb;
 if(uint32_t(vram)==INIT_CB)return init_cb;
 throw std::runtime_error("unmapped indirect callback address");
}
extern "C" void tooie_baseline_trot_toggle(uint8_t*,recomp_context* c){++getter_calls;clobber(c);c->r2=setting;}
extern "C" void func_8009CAAC(uint8_t*,recomp_context* c){event("end_lookup",uint32_t(c->r4),uint32_t(c->r5));clobber(c);c->r2=end_callback?guest(END_CB):0;}
extern "C" void func_8009CAF8(uint8_t*,recomp_context* c){event("init_lookup",uint32_t(c->r4),uint32_t(c->r5));clobber(c);c->r2=init_callback?guest(INIT_CB):0;}
extern "C" void func_8009CA70(uint8_t*,recomp_context* c){uint32_t p=uint32_t(c->r4),s=uint32_t(c->r5),mask=uint32_t(c->r6);require(mask==0x40,"family flag exactly 0x40");event("family",p,s,mask);clobber(c);c->r2=family(s);}
extern "C" void func_8008E39C(uint8_t*,recomp_context* c){event("shoes",uint32_t(c->r4));clobber(c);c->r2=shoes;}
extern "C" void bakey_released(uint8_t*,recomp_context* c){uint32_t p=uint32_t(c->r4);require(c->r5==1,"released Z id");event("released",p,1);bool result=!keys[p].level;clobber(c);c->r2=result;}
extern "C" void bakey_pressed(uint8_t*,recomp_context* c){uint32_t p=uint32_t(c->r4);require(c->r5==1,"pressed Z id");event("pressed",p,1);bool result=keys[p].level&&keys[p].edge&&!keys[p].suppressed;clobber(c);c->r2=result;}
extern "C" void bakey_held(uint8_t*,recomp_context* c){uint32_t p=uint32_t(c->r4);require(c->r5==1,"held Z id");event("held",p,1);bool result=keys[p].level&&!keys[p].suppressed;clobber(c);c->r2=result;}
extern "C" void func_800D8FF8(uint8_t*,recomp_context* c){event("delta");clobber(c);c->f0.fl=delta;}
extern "C" void func_800F0E00(uint8_t*,recomp_context* c){float a=c->f12.fl,b=c->f14.fl;event("clamp",std::bit_cast<uint32_t>(a),std::bit_cast<uint32_t>(b));clobber(c);c->f0.fl=std::max(a,b);}
extern "C" void bainput_enable(uint8_t*,recomp_context* c){uint32_t p=uint32_t(c->r4),id=uint32_t(c->r5),v=uint32_t(c->r6);event("enable",p,id,v);require(id==6&&v==0,"original input enable tuple");word(word(p+0xa4),word(word(p+0xa4))&~(1u<<id));clobber(c);}
// Exact extracted getters are delegated, preserving their guest32 load behavior.
extern "C" void bs_getCurrentState(uint8_t* m,recomp_context* c){event("current",uint32_t(c->r4));bs_getCurrentState_original(m,c);}
extern "C" void bs_getPreviousState(uint8_t* m,recomp_context* c){event("previous",uint32_t(c->r4));bs_getPreviousState_original(m,c);}
extern "C" void bs_getNextState(uint8_t* m,recomp_context* c){event("next",uint32_t(c->r4));bs_getNextState_original(m,c);}
extern "C" void func_800F46D8(uint8_t*,recomp_context* c){
 uint32_t old=uint32_t(c->r4),fresh=moves.contains(old)?moves[old]:old;
 event("defrag",old,fresh);move_seen.push_back({old,fresh,owner()});
 // At each following original call all preceding returned addresses are published.
 for(auto [address,returned]:move_publication)require(word(address)==returned,"preceding original move store published before next call");
 clobber(c);c->r2=guest(fresh);
}
extern "C" void tooie_trot_toggle_move(uint8_t* m,recomp_context* c){
 uint32_t old=uint32_t(c->r4),fresh=uint32_t(c->r5),prior_owner=owner(),prior_phase=phase(),prior_last=last();
 if(moving_loop){
  require(!move_seen.empty()&&move_seen.back()[0]==old&&move_seen.back()[1]==fresh,"move receives exact old/returned pointer pair");
  bool found=false;for(unsigned i=0;i<8;++i)if(word(TABLE+4*i)==old){move_publication.push_back({TABLE+4*i,fresh});found=true;break;}
  require(found,"move notification precedes original same-entry publication");
 }
 // The renamed function is the unmodified actual translated guest body.
 tooie_trot_toggle_move_body(m,c);
 require(phase()==prior_phase&&last()==prior_last,"move preserves phase and chain current");
 uint32_t expected=old!=fresh&&prior_owner==old?fresh:prior_owner;
 require(owner()==expected,"owner_move_must_transfer_address_and_preserve_occurrence");
}

int main() try {
 // 1: extracted-original OFF differential: all guards, cooldowns, callback shapes.
 for(unsigned forced=0;forced<2;++forced)for(unsigned shoe=0;shoe<2;++shoe)for(unsigned release=0;release<2;++release){
  reset(false);word(P+0x164,forced);shoes=shoe;keys[P].level=!release;auto a=snap(check());
  reset(true);word(P+0x164,forced);shoes=shoe;keys[P].level=!release;auto b=snap(check());equal_off(a,b,true);
 }
 for(float cooldown:{0.f,1.f,.125f}) {
  reset(false);word(P+0x1108,std::bit_cast<uint32_t>(cooldown));sample();auto a=snap();
  reset(true);word(P+0x1108,std::bit_cast<uint32_t>(cooldown));sample();equal_off(a,snap());
 }
 for(unsigned scenario=0;scenario<6;++scenario) {
  uint32_t request=scenario==0?0:scenario==2?1:0x15;
  reset(false);configure_scenario(scenario);set(P,request);auto a=snap();
  reset(true);configure_scenario(scenario);set(P,request);equal_off(a,snap());
 }
 ++groups;
 // 2: entry-level press is not an exit, then release arms.
 reset(true,1);entry();sample();require(phase()==AWAIT&&check()==0,"entry press and held Z do not toggle");arm();++on_cases;++groups;
 // Sensitivity: an original eligible held level without an edge must not toggle.
 keys[P]={true,false,false};sample();require(phase()==ARMED,"held_without_press_edge must not exit");++on_cases;
 // 3: press in non-polled state8, followed by repeated checks; reads never consume.
 set(P,8);pending();for(unsigned i=0;i<4;++i)require(check()==1&&phase()==PENDING,"repeated pending check stable");++on_cases;++groups;
 // Every cited non-polled family state receives exactly the same input seam.
 for(uint32_t s:{0x7bu,0x9au,0x9cu,0x95u}){reset(true,1);entry();set(P,s);arm();pending();require(check()==1&&last()==s,"eligible edge retained in each nonpolled family state");++on_cases;}
 reset(true,1);entry();arm();pending();
 // 4: pending survives family and retires outside; current chain exact.
 set(P,0x15);set(P,0x17);set(P,8);require(phase()==PENDING&&last()==8,"pending persists in family");set(P,1);require(owner()==0&&phase()==NONE,"outside transition retires");++on_cases;++groups;
 // 5: guards take original priority in every phase, shoes do not consume pending.
 for(unsigned ph=NONE;ph<=PENDING;++ph){reset(true,1);if(ph){entry();if(ph>=ARMED)arm();if(ph==PENDING)pending();}
  word(P+0x164,1);shoes=1;require(check()==1&&phase()==ph,"forced exit before shoes and toggle");
  word(P+0x164,0);require(check()==0&&phase()==ph,"shoes preserve phase");++on_cases;}
 ++groups;
 // 6: callback nesting commits before init and respects mutable next.
 reset(true,1);end_callback=init_callback=true;actions={{false,1,0x17,0},{true,0x17,8,0}};set(P,0x15);
 require(!init_views.empty(),"nested init observed");for(auto v:init_views)if(family(v.current))require(v.owner==P&&v.last==v.current,"commit observed before each family init");
 require(current(P)==0&&owner()==0,"nested end consumes mutable next before outer commit");
 set(P,0x15);require(owner()==P&&phase()==AWAIT,"reentry is fresh occurrence");
 reset(true,1);end_callback=init_callback=true;actions={{false,1,0,0x17}};set(P,0x15);require(last()==0x17&&current(P)==0x17&&phase()==AWAIT,"mutated next controls actual committed family state");++on_cases;++groups;
 // 7: direct E7AC-style state zeroing breaks the observed chain at sampler.
 reset(true,1);entry();arm();word(state(P),0);word(state(P)+4,0);word(state(P)+8,0);sample();require(owner()==0&&phase()==NONE&&check()==1,"reset chain break falls back to original release");
 // 710 itself can be the first observer after a direct reset; a pending exit
 // must not be mistaken for a request in the replacement occurrence.
 reset(true,1);entry();arm();pending();word(state(P),0);word(state(P)+4,0);word(state(P)+8,0);
 require(check()==0&&owner()==0&&phase()==NONE,"direct_reset_710_before_sample_must_retire_and_use_original_released");
 // Setter-side previous mismatch, rather than only sampler, must retire too.
 reset(true,1);entry();arm();word(state(P)+4,0x17);set(P,8);require(owner()==0,"setter chain break retires");++on_cases;++groups;
 // 8: two players and non-owner fallback; non-owner sampling must not touch slot.
 reset(true,1);entry();arm();pending();set(Q,0x15);auto saved_owner=owner(),saved_last=last(),saved_phase=phase();
 keys[Q]={true,true,false};sample(Q);require(check(Q)==0,"nonowner original held behavior");keys[Q].level=false;require(check(Q)==1,"nonowner original release behavior");
 require(owner()==saved_owner&&last()==saved_last&&phase()==saved_phase,"nonowner never overwrites slot");++on_cases;++groups;
 // 9: lifecycle no-op for nonowner; owner retire and reused pointer start fresh.
 retire(Q);require(owner()==P&&phase()==PENDING,"nonowner retire inert");retire(P);require(owner()==0&&phase()==NONE,"owner retire frees slot");
 word(state(P)+4,1);entry();require(phase()==AWAIT,"reused pointer clean occurrence");++on_cases;++groups;
 // 10: original suppression is authoritative both before and after arming.
 reset(true,1);entry();keys[P]={true,true,true};sample();require(phase()==AWAIT,"suppressed held press cannot arm");keys[P]={false,false,true};sample();require(phase()==ARMED,"released is original raw level even under suppression");keys[P]={true,true,true};sample();require(phase()==ARMED&&check()==0,"suppressed press cannot exit");++on_cases;++groups;
 // Live bool: disabled observations retire each phase at every policy seam.
 for(unsigned old_phase=AWAIT;old_phase<=PENDING;++old_phase)for(unsigned seam=0;seam<3;++seam) {
  reset(true,1);entry();if(old_phase>=ARMED)arm();if(old_phase==PENDING)pending();
  setting=0;keys[P]={true,false,false};
  if(seam==0)sample();else if(seam==1)set(P,0x17);else require(check()==0,"OFF exit seam uses original held policy");
  require(owner()==0&&last()==0&&phase()==NONE,"OFF at each seam retires old occurrence");
  keys[P]={false,false,false};require(check()==1,"OFF release uses original exit policy");
 }
 // Enabling mid-family never adopts it, whether Z is held or released.
 for(bool held:{false,true}) {
  reset(true,0);set(P,0x15);require(owner()==0&&phase()==NONE,"disabled entry never latches");
  setting=1;keys[P]={held,false,false};sample();
  require(owner()==0&&phase()==NONE&&check()==uint32_t(!held),"ON mid-family retains original hold policy");
  set(P,1);entry();require(owner()==P&&phase()==AWAIT,"ON takes effect on fresh family entry");
 }
 // A nonowner can observe OFF first; the existing owner alone resets its slot.
 reset(true,1);entry();arm();pending();setting=0;sample(Q);
 require(owner()==0&&last()==0&&phase()==NONE,"nonowner OFF observation retires global occurrence");
 // Idle sampling and explicit retire are idempotent; no state/input is injected.
 auto idle_owner=owner(),idle_last=last(),idle_phase=phase(),idle_current=current(P);
 for(unsigned i=0;i<3;++i){sample();retire(P);retire(0);}
 require(owner()==idle_owner&&last()==idle_last&&phase()==idle_phase&&current(P)==idle_current,"idle retirement leaves slot and original state unchanged");
 // Coalesced OFF then ON without a guest observation remains ordinary ON.
 reset(true,1);entry();arm();pending();setting=0;setting=1;sample();
 require(owner()==P&&phase()==PENDING&&check()==1,"unobserved OFF then ON retains enabled occurrence");
 ++groups;
 // 11 is checked for every invocation including callbacks and lifecycle.
 require(abi_checks==invocations&&abi_checks>0,"ABI checked for every invocation");++groups;
 // Production negative compiles the actual translated 710 guard mutation.
 // The three earlier component mutants remain separate historical evidence.
 // Move correction cases: OFF 8-entry differential, transfer/nonowner/same/reuse/retire.
#if FB04_HAS_MOVE
 auto table_setup=[](){word(TABLE,P);word(TABLE+4,0);word(TABLE+8,Q);word(TABLE+12,0);word(TABLE+16,P+0x200);word(TABLE+20,0);word(TABLE+24,0);word(TABLE+28,P+0x300);moves={{P,MOVED},{Q,Q},{P+0x200,P+0x400},{P+0x300,P+0x300}};};
 reset(false);table_setup();call(func_800F86D8_original);auto original_trace=trace;std::array<uint32_t,8> original_table{};for(unsigned i=0;i<8;++i)original_table[i]=word(TABLE+4*i);
 reset(true);table_setup();moving_loop=true;call(func_800F86D8);moving_loop=false;require(trace==original_trace,"move OFF original call trace exact");for(unsigned i=0;i<8;++i)require(word(TABLE+4*i)==original_table[i],"move OFF same-entry table publication exact");++off_cases;
 reset(true,1);entry();arm();pending();word(state(MOVED),word(state(P)));word(state(MOVED)+4,current(P));word(state(MOVED)+8,word(state(P)+8));keys[MOVED]=keys[P];
 word(TABLE,P);word(TABLE+4,Q);moves={{P,MOVED},{Q,Q+0x400}};moving_loop=true;call(func_800F86D8);moving_loop=false;
 require(owner()==MOVED&&last()==0x15&&phase()==PENDING,"owner_move_must_transfer_address_and_preserve_occurrence");
 require(word(TABLE)==MOVED&&word(TABLE+4)==Q+0x400,"move store goes to original same slot");require(check(MOVED)==1,"moved owner pending exit retained");
 require(move_seen.size()==2&&move_seen[1][2]==MOVED,"owner transfer precedes next original move call");
 call(tooie_trot_toggle_move,Q,Q+0x800);require(owner()==MOVED&&phase()==PENDING,"nonowner move inert");
 call(tooie_trot_toggle_move,MOVED,MOVED);require(owner()==MOVED&&phase()==PENDING,"same address move inert");
 word(state(P)+4,1);set(P,0x15);keys[P]={true,true,false};require(check(P)==0&&owner()==MOVED,"old address reused by different player is nonowner");
 retire(P);require(owner()==MOVED,"old address retire cannot retire moved owner");retire(MOVED);require(owner()==0&&phase()==NONE,"retire new address frees moved owner");
 ++groups;
#else
 require(false,"fourth original move function required by final design");
#endif
 std::cout<<"{\"status\":\"PASS\",\"contract_groups\":"<<groups<<",\"off_cases\":"<<off_cases<<",\"on_cases\":"<<on_cases<<",\"invocations\":"<<invocations<<",\"abi_checks\":"<<abi_checks<<",\"getter_calls\":"<<getter_calls<<",\"move_cases\":6,\"production_negative_guard\":1,\"scope\":\"production-extracted compiled providers only; no gameplay acceptance\"}\n";
 return 0;
}catch(const std::exception& e){
 std::cerr<<"FAIL: "<<e.what()<<'\n';
#if defined(FB04_NEGATIVE_GUARD)
 // WILL_FAIL must accept only the intended candidate-guard failure. An
 // unrelated failure returns zero so CTest rejects this negative control.
 return std::string(e.what()).find("direct_reset_710_before_sample_must_retire_and_use_original_released")!=std::string::npos?1:0;
#else
 return 1;
#endif
}
