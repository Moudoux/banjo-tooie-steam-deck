# Architecture

This page explains how the selected Windows player source is built and who owns each behavior. Read it before you change code.

This project is a work in progress. The design is not final, and improvements are continually evaluated.

- Building: [Windows build guide](WINDOWS.md).
- Playing: [player guide](PLAYER_GUIDE.md).
- Contribution rules: [CONTRIBUTING.md](../CONTRIBUTING.md).

## Terms

Each term has one meaning on this page.

- **MIPS**: the N64 CPU instruction set.
- **Display list**: an ordered list of drawing commands. The game writes display lists into memory, and the renderer reads them.
- **ABI** (application binary interface): the calling convention, register use and memory layout that two pieces of code agree on.
- **Guest**: the original N64 game code and its memory (RDRAM). Code written for the game's ABI is guest code.
- **Native side**: PC platform and runtime code outside the guest ABI. It includes RT64, the player shell and adapters.
- **Raw game code**: C that N64Recomp generates from the original game functions.
- **Guest patch**: a function written in C for the guest ABI. It is compiled to MIPS, then recompiled in N64Recomp patch mode.
- **Patch image**: `boot.bin`. It holds patch code bytes and initialized patch state.
- **Hook**: a call that N64Recomp's `hook` configuration inserts into raw game code before one instruction address.
- **Instruction edit**: a replacement instruction word that N64Recomp's `instruction` configuration applies before translation.
- **Thunk**: the native function that a hook calls. It forwards to a guest helper.
- **Adapter**: native code that provides a declared runtime service, such as start-up, overlay publication or SI access.
- **Extended GBI**: RT64's public display-list commands in `rt64_extended_gbi.h`, such as `gEXMatrixGroup`.
- **Owner**: the code that creates, changes and ends a piece of state.
- **Refusal**: an owner declines to add a label. The path preserves the original draw or emits an explicit ignore policy.

Guest ABI and actual execution are different things:

- Guest patches are written and compiled for the N64 ABI.
- They never run as MIPS on the PC.
- N64Recomp translates them to C. The PC runs that C as compiled x64 code.
- Raw game code runs the same way.

## Naming in the source

Names record what is known. Do not rename sources to guess at meaning.

- **`func_<address>`** is the original function address. It preserves the link to the ROM and the decompilation.
- **Overlay suffixes**, such as `_chhoneycarrier`, name the overlay. Together with the address, they identify one function.
- **`D_<address>`** preserves a data location; it does not establish what the data means.
- **`unk<offset>` and raw `+0x…` offsets** preserve layout where a useful field name has not been established.
- **`tooie_*`** marks guest helpers and state that the port added.
- **`baseline_*`** marks native thunks and adapters, and build flags of the selected engine.
- **`625`, `665` and `621`** are values that guard checks compare. They are route identifiers, not semantic names.

What each route value is compared against:

- **625**: `cpu625.c` compares the first `u32` at the backpack pointer with `0x625` (lines 115 and 139).
- **621**: `backpack621_owner.c` compares the first `u32` at the part pointer with `0x621` (line 68).
- **665**: `npc665_owner.c` compares `0x665` with the selector's high half, a `u16` at actor data `+4`, `D_8012BE64`, `D_8012CA18` and a table entry. The precise meaning of those fields is not established.

## State concepts

These concepts recur below.

- **Slot**: a descriptor's index in the original pool `D_80132ED0[200]`.
- **Birth**: a number that distinguishes allocation occurrences of one slot. A reused slot gets a new birth.
- **Epoch**: a number that distinguishes frame construction passes.
- **ID block**: a reserved range of renderer labels for one slot and birth.
- **Poisoned**: a flag that records an invariant failure. Admission paths that check it refuse while it is set. `silo_epoch` in [silo_bridge.h](../tools/baseline/guest/silo-candidate/silo_bridge.h) clears it when the epoch changes.

## Why the design changed in Alpha 4

### What Alpha 1 to 3 did

This describes the earlier approach in general. It is not an audit of each earlier release.

- The project edited generated C after N64Recomp emitted it.
- Native C++ held game policy. It read and wrote guest memory and registers at raw offsets.
- Game-specific policy lived inside RT64, through private source changes.
- A large custom platform frontend replaced standard runtime start-up.

### Why that was poor practice

- Edits to generated output are hard to trace. They live in disposable text, not in patch source.
- Those edits could be lost whenever N64Recomp's output changed.
- Native code took implicit ownership of game state. The game and the port could disagree about lifetime and order without an error.
- Raw offsets in C++ hid game structures. Readers could not easily map them to the decompilation.
- Private renderer changes coupled the port to one RT64 version. Each update meant checking them again.
- Fixes inside private changes did not reach the shared renderer.
- A code mod that replaced an edited function could drop the edit.

Looking back, much of that code was AI slop. It was produced quickly and in volume, and it worked in tested scenes. It was also hard to trace and hard to maintain, and it relied on state it did not own.

### What replaced it

| Earlier approach | Current approach | Source |
|---|---|---|
| Text edits to generated C | Declared N64Recomp configuration (`ignored`, `hook`, `instruction`) plus guest patches | [baseline_raw_condition.toml](../tools/baseline/baseline_raw_condition.toml), `prepare_raw` in [prepare_camera_bridge.py](../tools/baseline/prepare_camera_bridge.py) |
| Game policy in native code | Selected game and render owners moved to guest C; native exceptions remain below | [tools/baseline/guest](../tools/baseline/guest/) |
| Private renderer policy | Guest code emits extended GBI commands. Four named generic RT64 recipes remain | [actor_palette.c](../tools/baseline/guest/silo-candidate/actor_palette.c), [Baseline.cmake](../cmake/Baseline.cmake) |
| Custom engine start-up | `recomp::register_game`, `recomp::overlays::register_overlays`, `recomp::start` | [main.cpp](../tools/baseline/main.cpp) |
| Per-rate logic | RT64 `UserConfiguration` refresh-rate settings | [minimum_presentation.cpp](../tools/baseline/minimum_presentation.cpp) |
| Large custom frontend | A small ImGui menu. It is still custom and Windows-only | [minimum_menu.cpp](../tools/baseline/minimum_menu.cpp) |

One concrete replacement: the earlier composition used two instruction edits in `func_8001A3A0`. The camera composition removes them. `camera_pools.c` defines that function in guest C instead.

### Guest and native responsibilities

The guest side owns game behavior:

- gameplay, actor lifetimes, and descriptor allocation and free;
- draw order and game data;
- render identity: side tables, births, ID blocks and extended GBI emission.

The native side owns the platform and runtime:

- window, input devices, settings, menu, files and the save folder;
- threads, scheduling, overlay loading and RSP audio, through N64ModernRuntime;
- rendering, through RT64.

The boundary is a **declared native ABI**. It is not an absolute ban on native access to guest memory. These native accesses are legitimate when declared:

- A thunk receives fixed registers from a hook and forwards them to a guest helper.
- An adapter marshals arguments for a runtime API, such as SI, PI or overlay loading.
- An adapter checks guest bytes before it publishes them, such as the core1 and core2 hashes.
- Native code reads patch state at fixed offsets that `static_assert` checks against the guest layout.

Native code must not decide game semantics. It must not silently own guest lifetimes.

### Rules for changes

**DO:**

- Write game behavior changes as guest C in `tools/baseline/guest/`.
- Preserve the original semantics and operation order.
- Change them only for an explicitly justified fix, and say why in the patch. The guest `heap_realloc` replacement is one deliberate change of this kind.
- Keep new state in side tables in the patch image. Leave original objects at their original size.
- Check each premise before you emit a tag. Refuse if a check fails.
- Emit renderer intent as extended GBI commands in the guest display list.
- Pair each push with a pop in the same scope.
- Use runtime and RT64 settings for frame rate, resolution and aspect ratio.
- Name overlay functions with their overlay suffix.
- Declare every native access to guest memory. Name the function, the offsets and the reason.
- Regenerate after each guest edit. Update image pins only after you review the change.

**DO NOT:**

- Edit `funcs_*.c`, `boot.c` or any other generated output.
- Put game policy in native code.
- Read or write guest memory from native code outside a declared thunk, adapter or check.
- Change RT64 or runtime source for one game's behavior.
- Add separate game patches per frame rate or per aspect ratio.
- Identify an overlay function by address alone.
- Treat a refusal as proof of coverage.
- Replace a failing pin to make a build pass.

### What is still not in that shape

These are open cleanup items, not finished architecture.

- **Native hooks.** The maintained raw condition declares start-up, overlay, audio and scheduler hooks into native code.
- **Render hooks.** The selected render hooks include `func_800A7840` at `0x800A7888` and `0x800A7900`, and `func_800DE498` at `0x800DEFA0` and `0x800DEFB0`. Shadow, background, demo-timing, CPU625, NPC665 and minimum-host hooks are added by flag.
- **Register writes from hooks.** Three hooks in `func_80082088` assign guest registers from `baseline_overlay_hardware_word`.
- **Instruction edits.** `baseline_raw_condition.toml` declares nine instruction replacements. Some write `0x03E00008` (`jr $ra`) and some write `0`.
- **Native writes to patch state.** `baseline_render_construction_begin` in [adapters.cpp](../tools/baseline/adapters.cpp) advances the epoch in `tooie_render_bridge_state`.
- **Native player checks.** The same function enables the player boundary and stops the game on a player tripwire.
- **Native reads of guest memory.** `adapters.cpp` also holds start-up observers, thread admission checks, rejected-function providers, an SI adapter and core1/core2 publication.
- **Raw offsets.** Guest patches still use offsets such as `player_body(self)+0x11` and fixed addresses such as `0x80122DC8`.
- **Carried upstream patches.** See [Pinned upstream and carried patches](#pinned-upstream-and-carried-patches).
- **Windows-only shell.** The player uses a small ImGui menu, not a shared cross-platform frontend.
- **No mod support.** The player loads no mods and exports no mod API. It claims no compatibility with code mods.
- **Old source.** The old engine and unselected experiments remain in the tree.

## What is built

The supported player is the `windows-player` preset in [CMakePresets.json](../CMakePresets.json). It sets `TOOIE_BASELINE=ON`.

[CMakeLists.txt](../CMakeLists.txt) then includes [cmake/Baseline.cmake](../cmake/Baseline.cmake) and calls `return()`. Everything after that `return()` belongs to the old engine (`TooieRecompiled`). The player does not build it.

| Layer | Where | Status |
|---|---|---|
| Player shell and host | `tools/baseline/main.cpp`, `player_startup.cpp`, `devices.cpp`, `minimum_*.cpp`, `adapters.cpp` | Selected |
| Shared native helpers | Only the `src/` files that `target_sources(TooieBaseline …)` names in `Baseline.cmake` | Selected. The rest of `src/` belongs to the old engine |
| Guest patches | `tools/baseline/guest/**`, plus objects from the flag-selected preparation scripts | Only what the selected flags compile |
| Generators | `tools/prepare_codegen.py`, `tools/baseline/prepare_*.py`, `tools/prepare_runtime_baseline_selected.py` | Selected |
| Generated output | `build/windows-dev/baseline/**`, `generated/**` | Produced by maintained recipes; never edited by hand or committed |
| Pinned upstream | `deps/N64ModernRuntime`, `deps/rt64`, `deps/N64Recomp-codegen`, LLVM 19.1.3 | Used as-is, apart from the named recipes below |
| Old engine and experiments | `src/frontend_app.cpp` and other unlinked `src/` files, `cmake/NativeHost.cmake`, `cmake/PersistentRuntime.cmake` | Not in the player |

### How to find what is selected

Directory names do not select anything. `silo-candidate`, `camera-candidate` and `cpu625-candidate` are historical names. Use this procedure.

1. **Preset.** Read the `windows-player` `cacheVariables` in `CMakePresets.json`. Note each `TOOIE_BASELINE_*` value.
2. **Native sources.** In `cmake/Baseline.cmake`, find each `target_sources(TooieBaseline …)`. Check the `if(...)` around it against the preset values.
3. **Guest flags.** Read `SELECTION` in [prepare_player_guest.py](../tools/baseline/prepare_player_guest.py). It is `--actor-palette --demo-timing --shadow-fix --pre-background --cpu625 --minimum-host --npc665 --backpack621 --shadow-coefficients`.
4. **Flags to files.** Read `prepare_image` in [prepare_camera_bridge.py](../tools/baseline/prepare_camera_bridge.py). Step 5 lists what it compiles.
5. **What `prepare_image` compiles:**
   - Always: `camera_lifecycle`, `camera_ncpod`, `camera_pools`, `camera_emitters`, `camera_replay`, `camera_hook_leaves` and `camera_state` from `camera-candidate/`.
   - `--demo-timing` adds `demo_timing.c`.
   - `--shadow-fix` adds `shadow_projection.c`.
   - `--pre-background` adds `camera_pre_background_state.c` and `camera_pre_background_slots.c`.
   - The shared objects `player`, `actor`, `bone`, `pair`, `silo`, `leaf` and `lifecycle` come from `compile_objects` in `prepare_silo_bridge.py`. That function names each source file.
   - `--cpu625`, `--npc665`, `--backpack621` and `--minimum-host` add objects from `prepare_cpu625.py`, `prepare_npc665.py`, `prepare_backpack621.py` and `prepare_minimum_host.py`.
   - `prepare_honey_containment` derives `camera_honey_bridge.c` from [guest/render_bridge.c](../tools/baseline/guest/render_bridge.c) by exact, pinned insertions. The derivative is build output.
   - The link input list is the `inputs=` line before the `ld.lld` call.
6. **Confirm locally.** After preparation, `composition-v2/receipt.json` lists every input file and hash. `composition-v2/boot.c` lists every `RECOMP_FUNC` definition. Both are local build output. Do not commit them.

## Build pipeline

The executable holds two kinds of recompiled code and one data image. Guest patches go through these steps:

1. Guest C in `tools/baseline/guest/` marks patch functions with section `.recomp_patch`. Helpers use their own sections, such as `.tooie_camera_helpers`.
2. LLVM compiles each file to a big-endian MIPS object in ELF, an executable and object file format: `clang -target mips -EB -mips2 -mabi=32 -O2 -G0`.
3. `ld.lld -m elf32btsmip -T render_bridge.ld --emit-relocs` links the objects into `boot.elf`.
4. N64Recomp reads `boot.elf` with `strict_patch_mode`. It writes `boot.c`, `boot.bin` and `recomp_overlays.inl` (the function registry).
5. The pinned `clang-cl` compiles `boot.c` to x64 machine code as `BaselineBootPatch`. `file_to_c` embeds `boot.bin` as data.
6. At run time, `main.cpp` registers the game and overlays. `register_boot_patch` passes the image to `recomp::overlays::register_patches`. Then `recomp::start` runs the game.
7. Raw game code and patch code both run as x64 code. They write display-list commands into guest memory. Extended GBI commands are ordinary words in those lists.
8. RT64 reads the submitted display lists and renders them.

Raw game code follows a separate path:

- `tools/prepare_codegen.py` and `prepare_baseline_raw.py` produce ROM-derived inputs.
- N64Recomp applies the declared hooks and instruction edits from its configuration. It writes `raw/funcs_*.c`.
- `clang-cl` compiles that output as emitted, as `BaselineRecompiledFuncs`. No later text or regex step rewrites it.
- Functions in the `ignored` list are left out, so that guest patches provide them.
- `prepare_raw` removes each inserted hook line from a changed function and compares the result with the parent output. That check covers its own stage against the declared parent condition. It does not prove that ROM semantics are untouched.

`authenticate_function_table` in [prepare_render_bridge.py](../tools/baseline/prepare_render_bridge.py) orders declared registry entries against the linked ELF. This recipe changes generated registry metadata, not raw function bodies.

Keep three outputs apart:

- **Raw recompiled code** (`funcs_*.c`): one translation per original game function, after declared hooks and instruction edits.
- **Recompiled patch code** (`boot.c`): PC-executable code built from guest C. Calls into patch functions run this code.
- **Patch data image** (`boot.bin`): bytes mapped into guest memory from `0x80801000`, outside the original 8 MiB RDRAM.

The selected image is 294,132 bytes, SHA-256 `7ec531fa…`. It contains:

- the 2,760-byte boot and heap image at its start;
- `tooie_render_bridge_state` (18,844 bytes);
- `tooie_camera_state` (1,380 bytes);
- static camera pools (147,200 bytes).

Scripts that produce each stage:

```text
tools/prepare_codegen.py          ROM + decomp → config/selected.syms.toml, overlay-ids, heap patch object
tools/baseline/prepare_player_guest.py --rom … (WSL)
  ├─ prepare_baseline_raw.py      baseline_raw_condition.toml → build/…/baseline/raw
  ├─ prepare_boot_heap.py         boot-initializer/*.c + heap.o → heap-patch/boot.bin (2,760 B)
  ├─ prepare_silo_bridge.py       silo-candidate/*.c → Silo raw + image
  └─ prepare_render_bridge.py --camera --raw --image <SELECTION>
                                  → composition-v2/{raw/, boot.c, boot.bin, recomp_overlays.inl, *_layout.h}
cmake --preset windows-player     checks guest image SHA-256 7ec531fa…; composes the runtime sources
  BaselineRecompiledFuncs  ← composition-v2/raw/funcs_*.c   (raw recompiled game code)
  BaselineBootPatch        ← composition-v2/boot.c          (recompiled guest patches)
  TooieBaseline            ← boot.bin via file_to_c → register_boot_patch → recomp::overlays::register_patches
```

## Who owns what

| Area | Game owner (original function) | Port code | Upstream mechanism |
|---|---|---|---|
| Start-up and runtime | Boot `0x80000400`; initializers `boot_osInitialize` (`0x800016F0`), `tooie_core1_osInitialize` (core1 `osInitialize`, `0x8002D7B0`) | [main.cpp](../tools/baseline/main.cpp) `main`; guest [boot-initializer](../tools/baseline/guest/boot-initializer/) | `recomp::register_game`, `recomp::start` |
| Overlays and guest memory | core1 `0x80012030`, ROM `0x01E29B60`, size `0x31350`; core2 `0x800815C0`, ROM `0x01E5AEB0`, size `0xA5170` | [adapters.cpp](../tools/baseline/adapters.cpp) `register_boot_patch`, `baseline_observe_and_publish_core1/2` | `recomp::overlays::register_overlays`, `register_patches`, `load_overlays` |
| Actor and descriptor lifetime | Descriptor allocation `func_800EBED4`; pool `D_80132ED0[200]` of `0x30`-byte entries; player init and free `func_80092898`, `func_800F3880` | [render_bridge.c](../tools/baseline/guest/render_bridge.c), [player_lifecycle.c](../tools/baseline/guest/silo-candidate/player_lifecycle.c), [camera_lifecycle.c](../tools/baseline/guest/camera-candidate/camera_lifecycle.c) | Guest patch mode |
| Pose and animation | Pose `D_8012C968`; bone, leaf and pair draw `func_800DCF48`, `func_800DD998`, `func_80101808`/`func_800DE448` | [actor_palette.c](../tools/baseline/guest/silo-candidate/actor_palette.c), [npc665_owner.c](../tools/baseline/guest/silo-candidate/npc665_owner.c), [cpu625.c](../tools/baseline/guest/cpu625-candidate/cpu625.c), backpack621 owner | Guest patch mode |
| Renderer labels, camera, shadow | Draw `func_800DE498`; frame construction `func_800A7840`; projection emitters `func_800E42F0`, `func_800E44FC`, `func_800E4640`; shadow path `func_800A6250`, `func_800A6B24`, `func_800A5EC4` | [draw_boundaries.c](../tools/baseline/guest/silo-candidate/draw_boundaries.c), [camera_emitters.c](../tools/baseline/guest/camera-candidate/camera_emitters.c), [camera_hook_leaves.c](../tools/baseline/guest/camera-candidate/camera_hook_leaves.c), [shadow_projection.c](../tools/baseline/guest/camera-candidate/shadow_projection.c), [camera_state.c](../tools/baseline/guest/camera-candidate/camera_state.c) | RT64 extended GBI |
| Input and settings | Game reads controller port 0 | [minimum_host.cpp](../tools/baseline/minimum_host.cpp), [minimum_menu.cpp](../tools/baseline/minimum_menu.cpp), [minimum_presentation.cpp](../tools/baseline/minimum_presentation.cpp), [devices.cpp](../tools/baseline/devices.cpp); `src/frontend_config.cpp`, `src/frontend_input_preference.cpp` | `ultramodern::input::callbacks_t`, RT64 `UserConfiguration` |
| Audio | Core1 audio microcode task | [src/tooie_audio_rsp.cpp](../src/tooie_audio_rsp.cpp) plus generated RSP code | `recomp::rsp::callbacks_t` |
| Saves | 16 Kbit EEPROM | `GameEntry{.save_type = Eep16k, .game_id = u8"bt.n64.us.1.0"}` in `main.cpp`; [src/runtime_save_root.cpp](../src/runtime_save_root.cpp) (`<profile>/saves`); import in [player_startup.cpp](../tools/baseline/player_startup.cpp) | Runtime PI and save code, with the save-writing patch below |

### State, lifetime and order

**Bridge state** ([render_bridge.c](../tools/baseline/guest/render_bridge.c))

- `tooie_render_bridge_state` lives in the patch image.
- It starts with a magic value, a layout number and `birth_cursor = 1`.

**Births**

- The patched `func_800EBED4` keeps the original allocator body.
- It stamps `births[slot]` after the original field writes.
- The stamp comes before the call to `func_800EBE74`.
- Only descriptors inside the pool get a stamp.
- Birth `0` means "no valid birth".
- After `TOOIE_BRIDGE_MAX_BIRTH`, new births are `0` and `birth_saturated` counts them.

**Slot**

- The slot is `(descriptor − D_80132ED0) / 0x30`.
- It is valid only inside the pool on a `0x30` boundary.
- Otherwise the owner refuses.

**ID blocks** ([actor_palette.c](../tools/baseline/guest/silo-candidate/actor_palette.c))

- The Honey carrier's `bind_block` keeps its block while `birth` and the selector pair match.
- The actor palette also requires matching route, model, controller, pose, effective selector and leaf offsets.
- Otherwise the owner takes `next_block`.
- In `actor_palette.c` and `render_bridge.c`, `next_block` only increases.
- When it reaches `TOOIE_BRIDGE_BLOCK_COUNT`, those owners refuse new blocks.

**Per-draw context** ([draw_boundaries.c](../tools/baseline/guest/silo-candidate/draw_boundaries.c))

- `func_80101808` fills `SC` only when no outer actor is active and nesting depth is 0.
- `SC` records slot, birth, selector pair, epoch, view and phase.
- `func_80101808` clears `SC` after `func_800DE448` returns.
- `func_800DE448` increments `SS.silo_depth`. Owners act only at depth 1.

**Poisoning and pops**

- Admission paths in `actor_palette.c` check `SS.poisoned` and refuse while it is set.
- `tooie_actor_palette_end` writes the pop only if buffer, pool, epoch and range checks pass.
- If those checks fail, it counts a tripwire, sets `poisoned` and writes no pop.
- `silo_epoch` clears `poisoned` at the next epoch.
- This page does not claim that every failure path leaves balanced output.

**Camera construction** ([camera_hook_leaves.c](../tools/baseline/guest/camera-candidate/camera_hook_leaves.c))

- `tooie_camera_construction_begin` sets `owner_frame` from the hook at `0x800A7888`.
- `tooie_camera_construction_end` clears it from the hook at `0x800A7900`.
- Re-entry latches and is counted.
- The camera pools are static arrays in the patch image ([camera_state.c](../tools/baseline/guest/camera-candidate/camera_state.c)).

**Player lifecycle** ([player_lifecycle.c](../tools/baseline/guest/silo-candidate/player_lifecycle.c))

- `func_80092898` first retires CPU625 and backpack621 IDs.
- It then clears the player boundary, runs the original body and marks the boundary complete.
- `func_800F3880` retires the same IDs and clears the boundary.
- It then runs the original `func_80098E64` and `heap_free`.
- If a group is still open, the boundary is not cleared and the state is poisoned.

**Threads**

- The native observers in `adapters.cpp` use atomics.
- Guest patch state has no locks.
- Some source comments call fields "guest-thread-only".
- This guide and its source pass do not establish which guest threads run the allocator, draw and lifecycle patches. Nor do they show whether those patches can run concurrently.

### Overlay identity

A raw address is **not** unique. Many overlays are linked at the same VRAM range from `0x80800000`, so `0x80800380` names a different function in each overlay.

Overlay functions therefore carry their overlay in the symbol name. Examples are `func_80800380_chhoneycarrier`, `func_80800D8C_chnests` and `func_80800034_glglobalsettings`.

The generated registry identifies functions by their N64Recomp section entry (`recomp_overlays.inl`, ordered by `config/overlay-ids.txt`), not by address alone.

## Traces

### Actor draw

**Step 1: entry**

- The game calls `func_80101808(actor, callback)`, the original actor-draw boundary.
- The raw code leaves this function out.
- The selected body is the guest patch in `draw_boundaries.c`.

**Step 2: identity binding**

- The patch runs the original calls in order.
- It checks that `actor->unk0` (the descriptor) lies inside `D_80132ED0` on a `0x30` boundary.
- If so, it records `slot`, `birth = SS.births[slot]` and the selector pair (`unk14`/`unk16`) in `SC`.

**Step 3: inner draw**

- `func_800DE448` (also patched) tracks nesting depth.
- When `D_8012C948[24] == 0x80092234` (the player check), it calls `tooie_player_begin`.
- It then calls the original `func_800DE498`.

**Step 4: hook in the original body**

- `func_800DE498` is raw game code.
- At `0x800DEFB0`, an N64Recomp hook calls a native thunk.
- The thunk forwards to the selected guest helper.
- With the shadow fix compiled, that helper is `tooie_shadow_de498` in `shadow_projection.c`.
- Without it, the helper is `tooie_camera_de498_ignore`.

**Step 5: admission**

- The helper asks `tooie_actor_palette_begin` (`actor_palette.c`) to admit the draw.
- The pose, model and pool checks must all pass.
- On success, `tooie_actor_palette_begin` emits the actor's modelview group.
- The helper then emits a projection group with `cam_projection_policy(…, CAM_POLICY_WORLD)`.
- On failure, the helper emits `CAM_POLICY_IGNORE`.

**Step 6: unwinding**

- On the way out, `func_800DE448` calls `tooie_actor_palette_end`.
- It then calls `tooie_player_end`.
- `func_80101808` then clears `SC`.

### A setting: output frame rate

1. The menu's "Output rate" control in `minimum_menu.cpp` queues a preference request. `host::tick` applies it, and `minimum_host.cpp` persists the accepted value and publishes one packed rate request.
2. At startup, `minimum_presentation.cpp` maps the saved value to RT64 `UserConfiguration::RefreshRate::Original`, `Display` or `Manual` with its manual target. Default also selects Original.
3. During play, the graphics owner in `devices.cpp` consumes the latest request and queues a rate-only runtime configuration update. `minimum_presentation.cpp` changes only RT64's refresh-rate fields under the shared configuration lock. Aspect and pending framebuffer changes remain owned by their existing paths.
4. RT64 interpolation produces the higher output rate. No separate 60 FPS or 120 FPS game patch exists. Present Early remains selected for the player at every output rate.

The **Fullscreen** checkbox in `minimum_menu.cpp` requests borderless desktop
fullscreen through SDL in `minimum_host.cpp`. The menu reflects actual window
flags; a failed change shows an error and is not saved. VSync also applies
during play. Resolution and aspect choices remain latched until the next launch.

At player startup, `minimum_presentation.cpp` enables RT64's existing
`textureLOD.scale` setting so mip choice follows rendered resolution. This is
source behavior; texture appearance and missing-mipmap shimmer have not been
visually accepted.

### Renderer label owner

To find who emitted a matrix-group ID:

- **Actor IDs** are `TOOIE_BRIDGE_ID_BASE + block*512 + view*128 + phase*16 + role`.
- Actor IDs come from `tooie_actor_palette_begin`, `tooie_actor_cpu_root_begin` and `tooie_actor_feather_leaf_id` in [actor_palette.c](../tools/baseline/guest/silo-candidate/actor_palette.c).
- Each block is bound to a slot's `birth` and selector pair.
- **Honey exception:** the Honey carrier alone uses `bind_block` (`… + 1`) in [render_bridge.c](../tools/baseline/guest/render_bridge.c).
- **Camera and projection IDs** come from `tooie_camera_state` (`camera_state.c`, `.control.next_id`). `camera_emitters.c` emits them.
- **Shadow** uses `PC.idbase + SHADOW_ROLE_FIRST` (480) in `shadow_projection.c`.
- The shadow policy differs from `CAM_POLICY_WORLD` only in the decompose bit.
- `TOOIE_SHADOW_COEFFICIENTS=1` selects the shipped shadow route policy.
- **Feather leaves** (route `0x6EA`) use `tooie_actor_feather_leaf_id`. The patched `func_800DD024` in `camera_hook_leaves.c` emits them.

```sh
rg -n "TOOIE_BRIDGE_ID_BASE|TOOIE_CAMERA_ID_FIRST|SHADOW_ROLE_FIRST" tools/baseline/guest
rg -n "gEXMatrixGroup|gEXPopMatrixGroup|cam_projection_policy" tools/baseline/guest
rg -n "PATCH .*func_80101808" tools/baseline/guest
rg -n "0x800DEFB0|before_vram" tools/baseline/prepare_camera_bridge.py
rg -n "_chhoneycarrier|_glglobalsettings" tools/baseline
rg -n "target_sources\(TooieBaseline" cmake/Baseline.cmake
```

### Adding a guest patch

1. Write the C function in the right `tools/baseline/guest/` file, in `.recomp_patch`. Follow [Rules for changes](#rules-for-changes).
2. Add the function to the composition lists in `prepare_silo_bridge.py` and `prepare_camera_bridge.py`.
3. For camera providers, also update `camera-candidate/camera_hook_contract.json`.
4. Regenerate. The object, import, relocation and image checks fail until all lists agree.
5. Review the change. Then update the image pins (see [Windows build guide](WINDOWS.md#reusing-or-regenerating-a-build), "Guest C edits").

## Pinned upstream and carried patches

The project maintains these source changes against pinned upstream versions.

**Runtime**

- N64ModernRuntime is pinned at `ca568b6a…`.
- At configure time, `tools/prepare_runtime_baseline_selected.py` derives child sources for `events`, `threads`, `threadqueue` and `mesgqueue` (ultramodern).
- It also derives `dp`, `recomp` and `pi` (librecomp).
- The patches in `patches/runtime/` are `events-next-framebuffer`, `events-dp-freeze`, `threads-suspended-stop`, `producer-quiescent-cleanup`, `threadqueue-removal`, `dp-freeze` and `recomp-terminal-fence`.
- Each input and output is checked against a pinned hash.

**RT64**

- RT64 is pinned at `6f1c2d99…`.
- `TOOIE_BASELINE_RT64_GENERIC_FIXES` applies four recipes from `patches/rt64/`: `owner_index_guard`, `transform_upload_guard`, `rotation_interpolation` and `pop_matrix_group_projection`.
- The minimum host replaces RT64's SDL backend and Inspector sources from `minimum_host_*` recipes.

A new upstream pin requires a fresh check of each recipe.

## Comparison with Banjo-Kazooie

This section separates three sources:

- **BK original**: the Banjo-Kazooie decompilation, pinned at [`351ca158`](https://gitlab.com/banjo.decomp/banjo-kazooie/-/tree/351ca1580c10e550160ac11c77824fa9a498015e).
- **BK port**: BanjoRecomp, pinned at [`dbd52154`](https://github.com/BanjoRecomp/BanjoRecomp/tree/dbd52154bc1e00e56867b1fac94a8f907052f41b).
- **Tooie**: this repository. The Tooie decompilation is pinned at `25033be6` in `dependencies.lock.json`.

**Size note.** This documentation has not established a comparable count of original-game functions or lines for the two games. Their symbol corpora use different library and overlay inclusion rules. A larger game does not justify more native code or more coupling.

### Object references and handles

**BK original**

- Markers live in a pool at `D_8036E7C8`.
- An occupancy bitmap tracks used markers.
- The allocator `func_80332A60` writes `unk5C` from a static counter that starts at `0x387FB` ([code_A5BC0.c L2240–2257](https://gitlab.com/banjo.decomp/banjo-kazooie/-/blob/351ca1580c10e550160ac11c77824fa9a498015e/src/core2/code_A5BC0.c#L2240-2257)).
- Release clears `unk5C` and the bitmap bit ([L2260–2266](https://gitlab.com/banjo.decomp/banjo-kazooie/-/blob/351ca1580c10e550160ac11c77824fa9a498015e/src/core2/code_A5BC0.c#L2260-2266)).

**BK port**

- The `marker_init` patch keeps the original initialization order ([marker_extension_patches.c L9–L77](https://github.com/BanjoRecomp/BanjoRecomp/blob/dbd52154bc1e00e56867b1fac94a8f907052f41b/patches/marker_extension_patches.c#L9-L77)).
- After that, it stores a native provider handle in `marker_handles[0x1000]`.
- The release patch runs the original release first.
- It then destroys the provider data ([L79–L90](https://github.com/BanjoRecomp/BanjoRecomp/blob/dbd52154bc1e00e56867b1fac94a8f907052f41b/patches/marker_extension_patches.c#L79-L90)).
- The draw path uses the provider's spawn index, not `unk5C`.

**Tooie original**

- Actors reference a `0x30`-byte descriptor through `actor->unk0`.
- The descriptors live in `D_80132ED0[200]`.
- The allocator body that the patch keeps also writes a serial from `func_800EC854` at offset `+0x12` ([render_bridge.c](../tools/baseline/guest/render_bridge.c)).

**Tooie choice**

- A guest side table, `births[200]`, lives in patch state.
- The patched allocator stamps it.
- There is no native provider.
- The descriptor stays the same size.
- This page does not judge whether the original serial could serve instead. BK made the same kind of choice with `unk5C`.

### Allocation, reuse and reset

**BK port**

- The provider increments `spawn_count` for each creation ([recomp_extension_api.cpp L129–L168](https://github.com/BanjoRecomp/BanjoRecomp/blob/dbd52154bc1e00e56867b1fac94a8f907052f41b/src/game/recomp_extension_api.cpp#L129-L168)).
- A mutex guards the provider.
- The marker transform base is `MARKER_TRANSFORM_ID_START + spawn_index × 256`.
- The map-load patch runs the original pool initialization first ([load_patches.c L105–L130](https://github.com/BanjoRecomp/BanjoRecomp/blob/dbd52154bc1e00e56867b1fac94a8f907052f41b/patches/load_patches.c#L105-L130)).
- It then clears provider data, which resets `spawn_count` ([L61–L70](https://github.com/BanjoRecomp/BanjoRecomp/blob/dbd52154bc1e00e56867b1fac94a8f907052f41b/src/game/recomp_extension_api.cpp#L61-L70)).
- It also sets a global interpolation skip.
- `game_draw` clears the skip at its end ([graphics_patches.c L123–L169](https://github.com/BanjoRecomp/BanjoRecomp/blob/dbd52154bc1e00e56867b1fac94a8f907052f41b/patches/graphics_patches.c#L123-L169)).
- IDs can repeat after a map reset. The skip frame is the policy meant to bridge that.

**Tooie choice**

- Each in-pool allocation takes the next birth.
- A reused slot gets a new birth while the birth counter has capacity.
- A later admitted draw can acquire a new block; allocation alone does not assign one.
- In the cited files, the block counter only increases.
- When blocks run out, those owners refuse.
- The cited files contain no map-load block reset and no skip frame.

### Poses and traversal

**BK original**

- `modelRender` traverses geometry commands.
- Display-list, bone and billboard commands push and pop matrices ([modelRender.c L706–819](https://gitlab.com/banjo.decomp/banjo-kazooie/-/blob/351ca1580c10e550160ac11c77824fa9a498015e/src/core2/modelRender.c#L706-819)).

**BK port**

- A shared traversal places tags inside `modelRender_draw`.
- It resets the traversal offset and bones flag before root geometry.
- Bone and billboard commands advance the offset.
- The group ID is the base plus the traversal offset. Map models use the command offset ([`set_model_matrix_group`, L198–L240](https://github.com/BanjoRecomp/BanjoRecomp/blob/dbd52154bc1e00e56867b1fac94a8f907052f41b/patches/transform_tagging.c#L198-L240)).
- Tagging is conditional. It depends on the ID base, skip flags, admission predicates and geometry commands.
- With a zero base and no global skip, the helper emits nothing.

**Tooie choice**

- The selected routes do not provide BK-style general traversal tagging.
- Recognized producer routes emit tags: the actor palette, the player, NPC665, CPU625, backpack621 and feather leaves.
- Each route checks pose, model data and capacity.
- Other draws are refused and left to RT64's default handling.

**Trade-off**

- BK's shared traversal offers broader, reusable tag placement.
- Tooie's route coverage remains limited to the routes written and checked so far.

### Overlay identity

**BK port**

- Decompilation names identify overlays (`core2`, `CC`, `MMM` and others).
- Guest patches load them with `recomp_load_overlays_by_rom` ([overlay_loading.c L56–L128](https://github.com/BanjoRecomp/BanjoRecomp/blob/dbd52154bc1e00e56867b1fac94a8f907052f41b/patches/overlay_loading.c#L56-L128)).
- `boot___osInitialize_common` loads core1. `overlay_load` loads the rest.

**Tooie choice**

- Most functions are address-named. Overlay functions carry an overlay suffix.
- Native adapters publish core1 and core2 only after their bytes match pinned hashes ([adapters.cpp](../tools/baseline/adapters.cpp), [prepare_player_guest.py](../tools/baseline/prepare_player_guest.py)).
- The core1/core2 publication decisions remain in native adapters. BK guest patches call the runtime overlay API.

### Tag scope and order

**BK port**

- `__marker_draw` saves the enclosing transform base ([marker_transform_tagging.c L40–L84](https://github.com/BanjoRecomp/BanjoRecomp/blob/dbd52154bc1e00e56867b1fac94a8f907052f41b/patches/marker_transform_tagging.c#L40-L84)).
- It binds the marker base around each `drawFunc` call.
- Afterwards it restores the saved base, not zero.
- Each caller pops a group only when its push returned true ([transform_tagging.c L243–L298](https://github.com/BanjoRecomp/BanjoRecomp/blob/dbd52154bc1e00e56867b1fac94a8f907052f41b/patches/transform_tagging.c#L243-L298)).

**Tooie choice**

- The actor modelview push starts from a hook inside raw `func_800DE498`.
- A projection group with `WORLD` or `IGNORE` policy follows.
- The pop comes after `func_800DE498` returns, at depth 1 in `func_800DE448`.
- The pop is checked. A failed check poisons the state and writes no pop.

**Difference**

- BK emits from guest patch bodies of named functions.
- Tooie still enters part of its scope through a native thunk.

### Mod consequences

**BK port**

- Mods can attach data to markers and read spawn indices through exported functions ([marker_extension_patches.c L92–L110](https://github.com/BanjoRecomp/BanjoRecomp/blob/dbd52154bc1e00e56867b1fac94a8f907052f41b/patches/marker_extension_patches.c#L92-L110)).
- Mods can set transform IDs ([transform_tagging.c L959–L970](https://github.com/BanjoRecomp/BanjoRecomp/blob/dbd52154bc1e00e56867b1fac94a8f907052f41b/patches/transform_tagging.c#L959-L970)).
- The handle array is larger than the game's normal marker count, so that mods can add markers.

**Tooie, current state**

- The player loads no mods and exports no API.
- Side tables match the original 200-entry pool. Descriptors outside it are refused, not tracked.

**Tooie, if mods were added later**

- A live recompiler that rebuilds a ROM-derived function without this port's hook configuration can omit the inserted hook calls.
- A mod that deliberately overrides a registered guest C replacement removes that replacement's behavior, unless the two are composed or chained.
- Overriding `func_80101808`, `func_800DE448` or `func_800EBED4` that way would remove the identity work.

### What transfers and what does not

These principles transfer:

- Keep side tables instead of enlarging original objects.
- Place lifecycle notifications at a verified boundary relative to the original operations.
- Pair push and pop in the caller's scope.
- Restore the saved enclosing state, not zero.
- Reset or skip at known boundaries.
- Derive IDs from a stable occurrence, not from an address.

These assumptions do not transfer:

- BK's marker pool, bitmap and `unk5C` counter.
- BK's geometry-command layout and traversal offsets.
- BK's transform ID ranges, such as 256 IDs per marker.
- BK's native provider and its reset policy.
- BK's overlay table and names.
