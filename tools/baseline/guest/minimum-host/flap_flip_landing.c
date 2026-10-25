/* bsbflip_update, deps/banjo-tooie/src/overlays/bs/bflip.c:
 * inherited body except the @recomp declaration, snapshot and argument below. */
#include "common.h"

#include "core2/anctrl.h"
#include "overlays/ba/anim.h"
#include "overlays/ba/playerstate.h"
#include "overlays/ba/physics.h"
#include "overlays/ba/stick.h"
#include "core2/1E72EA0.h"
#include "overlays/bs/state.h"
#include "core2/1E66990.h"
#include "core2/1E76880.h"
#include "core2/1E76CC0.h"
#include "core2/1E78BF0.h"
#include "core2/1E79FD0.h"
#include "core2/1EA0690.h"
#include "core2/1EC8070.h"
#include "core2/1ECA640.h"
#include "bs.h"
#include "player.h"
#include "overlays/ba/key.h"


extern void func_80800000_bsbflip(PlayerState *self);
extern void func_80800064_bsbflip(PlayerState *self, s32 substate);
extern s32 anctrl_getPlaybackType(AnimCtrl *anctrl);
extern s32 bainput_should_beak_bust(PlayerState *self);
extern s32 func_8008DA24(PlayerState *self);

__attribute__((section(".recomp_patch"), noinline))
void bsbflip_update(PlayerState *self) {
    BanjoStateId next_state = BS_STATE_0_INVALID;
    BanjoStateId landing_state; /* @recomp Port-added landing snapshot. */
    AnimCtrl *anctrl = baanim_getAnimCtrlPtr(self);
    if ( anctrl_isAt(anctrl, 0.2394f)
        && (anctrl_getPlaybackType(anctrl) != 2)
        && (anctrl_getIndex(anctrl) == ASSET_4B_ANIM_BSBFLIP_ENTER)
    ){
        func_8009DF94(self, 0x3FF, 0.558f, 22000);
    }
    /* This overlay's substate: func_80800064_bsbflip writes unk15C.word;
     * init selects 1, end selects 0; update consumes 1-4. The shared union name stays unknown. */
    switch(self->unk15C.word) {
        case 1:
            if (anctrl_isAt(anctrl, 0.1837f)){
                func_80800064_bsbflip(self, 2);
            }
            break;

        case 2:
            func_80800000_bsbflip(self);
            if (anctrl_isStopped(anctrl)) {
               func_80800064_bsbflip(self, 3); 
            }
            if (bainput_should_beak_bust(self)) {
                next_state = BS_STATE_F_BBUSTER;
            }
            if (func_8008DA24(self)) {
                next_state = BS_STATE_A8;
            }
            break;

        case 3:
            if (func_8008E260(self)) {
                next_state = BS_STATE_3D_FALL_TUMBLING;
            }
            /* @recomp Keep the pre-A-release state for the grounded landing choice. */
            landing_state = next_state;
            if (bakey_released(self, BUTTON_A)) {
                next_state = BS_STATE_2F_FALL;
            }
            if (player_isStable(self)) {
                /* @recomp Use that snapshot here; transition to 4 and break remain inherited.
                 * Releasing A selects Fall only while airborne; grounded flips keep their landing. */
                next_state = func_800A01F8(self, landing_state);
                func_80800064_bsbflip(self, 4);
                break;
            } 

            if (bainput_should_beak_bust(self)) {
                next_state = BS_STATE_F_BBUSTER;
            }
            if (func_8008DA24(self)) {
                next_state = BS_STATE_A8;
            }
            
            break;

        case 4:
            if (anctrl_isStopped(anctrl)) {
                next_state = BS_STATE_1_SLOW_WALK;
            }
            next_state = func_800A01F8(self, next_state);
            break;

    }
    if (player_inWater(self)) {
        next_state = BS_STATE_4C_LANDING_IN_WATER;
    }
    bs_setState(self, next_state);
}

