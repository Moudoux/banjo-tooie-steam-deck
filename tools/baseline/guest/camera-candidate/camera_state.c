#define TOOIE_CAMERA_GUEST 1
#include "camera_identity.h"
#define STATE_SECTION __attribute__((section(".tooie_camera_state"),aligned(4),used))
#define POOL_SECTION __attribute__((section(".tooie_camera_pools"),aligned(16),used))
struct TooieCameraState tooie_camera_state STATE_SECTION = {
 .control = {.magic=TOOIE_CAMERA_MAGIC,.layout=TOOIE_CAMERA_LAYOUT,
             .next_id=TOOIE_CAMERA_ID_FIRST,.next_birth=1,.next_session=1,.next_serial=1}
};
unsigned char tooie_camera_main[2][TOOIE_CAMERA_M_BYTES] POOL_SECTION = {{0}};
unsigned char tooie_camera_redirected[2][TOOIE_CAMERA_R_BYTES] POOL_SECTION = {{0}};
_Static_assert(sizeof(struct TooieCameraState)==1380,"camera ABI extent");
_Static_assert(sizeof(tooie_camera_main)+sizeof(tooie_camera_redirected)==813824,
               "static pool extent");
/* M1: MAIN = 0x76C0 + 6400 + 121344 + 211968 + 6400. These values move together with
 * render_bridge.ld, prepare_camera_bridge.py and prepare_silo_bridge.py. */
_Static_assert(TOOIE_CAMERA_M_BYTES==0x5BEC0u && TOOIE_BRIDGE_LOWER_BYTES==0x5A5C0u &&
    TOOIE_CAMERA_M_BYTES%TOOIE_CAMERA_POOL_ALIGN==0u,"partitioned MAIN extent");

_Static_assert(offsetof(struct TooieCameraState,owner_frame)==1376,"construction owner offset");
_Static_assert(_Alignof(struct TooieCameraState)==4,"camera state alignment");
