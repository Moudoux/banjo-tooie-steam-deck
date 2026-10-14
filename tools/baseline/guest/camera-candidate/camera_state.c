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
_Static_assert(sizeof(tooie_camera_main)+sizeof(tooie_camera_redirected)==147200,
               "static pool extent");

_Static_assert(offsetof(struct TooieCameraState,owner_frame)==1376,"construction owner offset");
_Static_assert(_Alignof(struct TooieCameraState)==4,"camera state alignment");
