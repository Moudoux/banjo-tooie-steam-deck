#pragma once
/* Recorded replay divisor. The demo record stores pending; the frame's first
 * wait query moves it to active, which that frame's wait loop then uses. */
struct DemoTimingState { unsigned int pending, active; };
extern struct DemoTimingState tooie_demo_timing_state;
