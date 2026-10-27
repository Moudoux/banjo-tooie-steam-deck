# Architecture

Current shared renderer details are in [Rendering](RENDERING.md). The sections below identify its selected responsibilities alongside startup, overlays and the build pipeline.

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

Names record what is known. Do not rename sources to guess at meaning. Numbered route names below are retained identifiers, not the coverage boundary of the current shared renderer. See [Rendering](RENDERING.md).

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
- **ID window**: the shared renderer reserves labels for an owner sample and its traversal. Retained legacy owners (the actor palette, Honey outside the BK selection, and the legacy player boundary only where an engineering configuration enables it) still allocate monotonically increasing `next_block` blocks; that is a different mechanism.
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
| Private renderer policy | Guest code emits extended GBI commands. Five named generic RT64 recipes remain | [bk_pipeline.c](../tools/baseline/guest/silo-candidate/bk_pipeline.c), [Rendering](RENDERING.md), [Baseline.cmake](../cmake/Baseline.cmake) |
| Custom engine start-up | `recomp::register_game`, `recomp::overlays::register_overlays`, `recomp::start` | [main.cpp](../tools/baseline/main.cpp) |
| Per-rate logic | RT64 `UserConfiguration` refresh-rate settings | [minimum_presentation.cpp](../tools/baseline/minimum_presentation.cpp) |
| Large custom frontend | A small custom ImGui menu | [minimum_menu.cpp](../tools/baseline/minimum_menu.cpp) |

One concrete replacement: the earlier composition used two instruction edits in `func_8001A3A0`. The camera composition removes them. `camera_pools.c` defines that function in guest C instead.

### Guest and native responsibilities

The guest side owns game behavior:

- gameplay, actor lifetimes, and descriptor allocation and free;
- draw order and game data;
- render identity: descriptor births, drawn-player roster identity, owner/sample windows, bounded graph history and extended GBI emission. Native submit hooks provide the exact-root acknowledgement needed for source-data reuse.

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
- Apply menu changes during play when the owning system supports it. Queue the
  request to that owner, or publish a small atomic value. Keep related fields
  together. The consumer applies the change at its next safe boundary.
- Do not read mutable menu state from guest or renderer threads. Do not change
  guest memory from menu callbacks. Persist a preference separately from applying it.
- Require restart only for a documented initialization dependency. Show that
  limit beside the option. A new setting must not inherit restart behavior by default.
- Resolution is applied by the graphics owner through RT64 configuration. Quiet
  Talon Trot is read by its guest sound helper through a live host preference.
  Widescreen requests pass to a guest helper before camera refresh. That helper
  calls the original setter and publishes the applied state for RT64. It reads
  the original preference for Use game setting; it never overwrites that preference.
  The old global-settings query override is retired from the selected player.
- Gameplay aspect uses one atomic encoded ratio for guest projection and RT64.
  Each owner applies it at its next boundary; no exact frame delay is promised.
  Cutscene aspect is sampled live by `settings_word()` in `minimum_presentation.cpp`;
  the guest widescreen helper consumes it before camera refresh.
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

These are open cleanup items, not finished architecture. The scoped Alpha 4 ownership migrations are complete; this list records what remains, and not every historical finding has been fixed.

- **Native hooks.** The maintained raw condition declares start-up, overlay, audio and scheduler hooks into native code.
- **Render hooks.** The selected recipes include shared frame/root/submit, geometry, selector, camera, shadow, menu and transition boundaries. Read [bk_pipeline_hooks.py](../tools/baseline/bk_pipeline_hooks.py), [prepare_camera_bridge.py](../tools/baseline/prepare_camera_bridge.py) and their selected recipe imports; the older numbered-route list alone does not describe this composition.
- **Register writes from hooks.** One hook in `func_80082088` assigns `r21` from the original ROM-key word through `baseline_overlay_hardware_word`.
- **Instruction edits.** `baseline_raw_condition.toml` declares nine instruction replacements, including two PI-status reads replaced by `0x340A0000` (`ori $t2,$zero,0`). Other replacements include `0x03E00008` (`jr $ra`) and `0`.
- **Native player checks.** Engineering configurations that select `BASELINE_PLAYER_BOUNDARY` keep a native enable write and tripwire check in `baseline_render_construction_begin` ([adapters.cpp](../tools/baseline/adapters.cpp)): the write turns on the retained guest player boundary once, and a nonzero tripwire count stops the player through `rejected_body`. The player presets do not select it, so neither is built there. The selected camera composition advances its frame counter in guest `tooie_camera_construction_begin`. Bridge-only engineering compositions keep the native increment.
- **Retained legacy render owners.** In the player, one owner from before the shared pipeline, the actor palette, still emits labels in the selected BK composition, while Honey uses its original raw routine through normal overlay dispatch. The palette's own modelview group is conditional; that change is statically qualified and runtime acceptance is pending. The player does not enable the legacy player boundary, so it emits no group there; its guest code and shared storage are kept intentionally for engineering configurations.
  - The legacy player boundary ([player_boundary.c](../tools/baseline/guest/silo-candidate/player_boundary.c)): the BK `func_800DE448` in [draw_boundaries.c](../tools/baseline/guest/silo-candidate/draw_boundaries.c) still calls `tooie_player_begin` and `tooie_player_end`, but the player presets do not select `TOOIE_BASELINE_PLAYER_BOUNDARY`: with no native enable, `tooie_player_begin` returns before any group, block, prepay or count, and the native tripwire check is not built. Compositions that select it keep the outer matrix group (block ID + 129, every component skipped) and its fatal tripwire.
  - The actor palette ([actor_palette.c](../tools/baseline/guest/silo-candidate/actor_palette.c)): gives every admitted header-bit40 draw its WORLD projection ID. It pushes its own modelview group only for draws whose BK actor owner is not current. See Player lifecycle and retained routes below.
  - Honey (`func_80800380_chhoneycarrier` in [render_bridge.c](../tools/baseline/guest/render_bridge.c)): under BK the selected raw corpus retains its original body; no patch replacement or restoration row is registered. The selected raw `func_80101870` calls `func_80101808` directly, so the carrier is labelled by the BK actor owner like every other actor. Its original overlay function-table entry remains. Non-BK compositions keep the earlier block-ID wrapper and its fatal close.
- **Native reads of guest memory.** `adapters.cpp` also holds start-up observers, thread admission checks, rejected-function providers, an SI adapter and core1/core2 publication.
- **Raw offsets.** Guest patches still use offsets such as `player_body(self)+0x11` and fixed addresses such as `0x80122DC8`.
- **Original-address registry restoration.** In the selected camera build, `write_registry_header` in [prepare_camera_bridge.py](../tools/baseline/prepare_camera_bridge.py) generates every restoration row from `selected.syms.toml`:
  - the core2 rows;
  - the base rows: `boot_osInitialize` and `tooie_core1_osInitialize`, plus `rom_read_word` and `heap_realloc` under their native selections;
  - the overlay rows: the selected `bsbtrot` bodies (plus Honey only outside BK), each with its overlay ID and its section's function count.
  `camera_restore_base` and `camera_restore_overlays` in [camera_registry.hpp](../tools/baseline/camera_registry.hpp) restore each row only after checking its section identity and that the original was omitted. Overlay sections are reached through the overlay ID table and checked against index, ROM, RAM, size and function count. The same generated header owns the selected provider list; its length is the provider count that registration checks. Restored tables are sorted by offset. Camera builds whose header lacks the generated rows, and the non-camera engineering fallback, keep the literal rows in `adapters.cpp`. These are configure- and registration-time checks, not gameplay acceptance.
- **CMake renames.** `BaselineBootPatch` is compiled with `osVirtualToPhysical` and `osWritebackDCache` defined to their `*_recomp` names.
- **Thunk stack offsets.** Native thunks read qualified guest frames. Exact generated hook inverses, linker/layout checks and actual-helper fixtures verify specific bindings; they do not establish arbitrary ABI changes or gameplay acceptance. Review each helper against its selected original callsite rather than treating a synthetic fixture as provenance.
- **Generated patch C after preparation.** The selected player pins the image, layout and the exact `boot.c`, `recomp_overlays.inl` and `funcs.h` bytes. Configure checks the source directory used by `BaselineBootPatch` and those hashes. Changes trigger configure again before compilation. Repeated pinned-recompiler runs established these byte identities; a receipt cannot approve its own output. Older engineering selections do not gain this check automatically.
- **Python-authored runtime text.** Some selected runtime children still come from Python text recipes rather than tracked patches. See [patches/runtime/PROVENANCE.md](../patches/runtime/PROVENANCE.md).
- **Optimized Python.** Several preparers check identity with `assert`. Their entry points refuse to run under `python -O` or with `PYTHONOPTIMIZE` set.
- **Carried upstream patches.** See [Pinned upstream and carried patches](#pinned-upstream-and-carried-patches).
- **Custom shell.** The player uses a small ImGui menu, not a shared frontend.
- **Texture packs (experimental, untested in play).** Source support: the player accepts RT64 `.rtz` packs through the runtime's mod manifest/order service; in-game replacement has not been tested. Code mods and ROM patches remain unsupported; no guest mod API is exported. The engineering baseline keeps an empty mod game ID.
- **Old source.** The old engine and unselected experiments remain in the tree.

## What is built

The supported player is the `windows-player` preset in [CMakePresets.json](../CMakePresets.json). It sets `TOOIE_BASELINE=ON`. A `linux-player` preset exists but is not qualified and is not a current focus.

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
3. **Guest flags.** Read `SELECTION` in [prepare_player_guest.py](../tools/baseline/prepare_player_guest.py). That source owns the complete selected flag list.
4. **Flags to files.** Read `prepare_image` in [prepare_camera_bridge.py](../tools/baseline/prepare_camera_bridge.py). Step 5 lists what it compiles.
5. **Main composition owners** (use `prepare_image` for the complete selected input list):
   - Always: `camera_lifecycle`, `camera_ncpod`, `camera_pools`, `camera_emitters`, `camera_replay`, `camera_hook_leaves` and `camera_state` from `camera-candidate/`.
   - `--demo-timing` adds `demo_timing.c`.
   - `--shadow-fix` adds `shadow_projection.c`.
   - `--pre-background` adds `camera_pre_background_state.c` and `camera_pre_background_slots.c`.
   - The shared objects `player`, `actor`, `bone`, `pair`, `silo`, `leaf` and `lifecycle` come from `compile_objects` in `prepare_silo_bridge.py`. That function names each source file.
   - `--cpu625`, `--npc665`, `--backpack621` and `--minimum-host` add objects from `prepare_cpu625.py`, `prepare_npc665.py`, `prepare_backpack621.py` and `prepare_minimum_host.py`.
   - `prepare_honey_containment` derives `camera_honey_bridge.c` from [guest/render_bridge.c](../tools/baseline/guest/render_bridge.c) by exact, pinned insertions. The derivative is build output.
   - The link input list is the `inputs=` line before the `ld.lld` call.
   - `--bk-pipeline` adds the shared pipeline and selects `TOOIE_BK_PIPELINE` branches inside retained objects. An object name such as `actor`, `leaf` or `npc665` does not establish that its legacy label path executes.
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

- `tools/prepare_codegen.py` and `prepare_baseline_raw.py` produce ROM-derived inputs. The player needs only four `prepare_codegen.py` products: `config/selected.syms.toml`, `config/overlay-ids.txt`, `generated/core2-expected.bin` and `generated/heap-patch-products/heap.o`. `--player-inputs` prepares them in a fresh `.codegen/player-inputs-*` scratch root with the existing heap pipeline and validators. It installs each product only if absent, refuses if an existing file differs, and writes no corpus, lifter output or codegen receipt. Downstream preparers still pin `selected.syms.toml`, `core2-expected.bin` and `heap.o`. `overlay-ids.txt` has no individual pin; `raw-receipt.json` only records its hash.
- N64Recomp applies the declared hooks and instruction edits from its configuration. It writes `raw/funcs_*.c`.
- `clang-cl` compiles that output as emitted, as `BaselineRecompiledFuncs`. No later text or regex step rewrites it.
- Functions in the `ignored` list are excluded from raw translation. Selected guest replacements or declared native providers supply them.
- `prepare_raw` removes each inserted hook line from a changed function and compares the result with the parent output. That check covers its own stage against the declared parent condition. It does not prove that ROM semantics are untouched.

`authenticate_function_table` in [prepare_render_bridge.py](../tools/baseline/prepare_render_bridge.py) checks each declared registry table against the linked ELF: every member, offset and extent must match, and no extents overlap. It does not rewrite the table. `recomp_overlays.inl` keeps N64Recomp's emitted order, because the runtime lookups and the native registry checks do not depend on order.

Keep three outputs apart:

- **Raw recompiled code** (`funcs_*.c`): one translation per original game function, after declared hooks and instruction edits.
- **Recompiled patch code** (`boot.c`): PC-executable code built from guest C. Calls into patch functions run this code.
- **Patch data image** (`boot.bin`): bytes mapped into guest memory from `0x80801000`, outside the original 8 MiB RDRAM.

The selected image contains boot and heap code, composed guest-owner state and static camera pools.

- [prepare_player_guest.py](../tools/baseline/prepare_player_guest.py) owns the selected image and layout identities.
- [Baseline.cmake](../cmake/Baseline.cmake) independently checks the image before embedding it.
- The preparation receipt and generated `*_layout.h` files record the composed inputs, image size and state locations.

Scripts that produce each stage:

```text
tools/prepare_codegen.py [--player-inputs]
                                  ROM + decomp → config/selected.syms.toml, overlay-ids, core2-expected.bin, heap.o
tools/baseline/prepare_player_guest.py --rom … (WSL)
  ├─ prepare_baseline_raw.py      baseline_raw_condition.toml → build/…/baseline/raw
  ├─ prepare_boot_heap.py         boot-initializer/*.c + heap.o → heap-patch/boot.bin (2,760 B)
  ├─ prepare_silo_bridge.py       silo-candidate/*.c → Silo raw + image
  └─ prepare_render_bridge.py --camera --raw --image <SELECTION>
                                  → composition-v2/{raw/, boot.c, boot.bin, recomp_overlays.inl, *_layout.h}
cmake --preset windows-player     checks the source-owned guest image pin; composes the runtime sources
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
| Pose and animation | Original pose and deformation producers; shared bone/list traversal | [bk_float.inc](../tools/baseline/guest/silo-candidate/bk_float.inc), [bk_graph.inc](../tools/baseline/guest/silo-candidate/bk_graph.inc), [bk_select.inc](../tools/baseline/guest/silo-candidate/bk_select.inc), [leaf_handler.c](../tools/baseline/guest/silo-candidate/leaf_handler.c) | Guest patch mode; original attribute production retained |
| Renderer labels, camera, shadow | Draw owner, root/graph, view and shadow producer | [bk_pipeline.c](../tools/baseline/guest/silo-candidate/bk_pipeline.c), [bk_attach.inc](../tools/baseline/guest/silo-candidate/bk_attach.inc), [bk_shadow_attach.inc](../tools/baseline/guest/silo-candidate/bk_shadow_attach.inc), [camera_emitters.c](../tools/baseline/guest/camera-candidate/camera_emitters.c) | Public RT64 commands; see [Rendering](RENDERING.md) |
| Input and settings | Game reads controller port 0 | [minimum_host.cpp](../tools/baseline/minimum_host.cpp), [minimum_menu.cpp](../tools/baseline/minimum_menu.cpp), [minimum_presentation.cpp](../tools/baseline/minimum_presentation.cpp), [devices.cpp](../tools/baseline/devices.cpp); `src/frontend_config.cpp`, `src/frontend_input_preference.cpp` | `ultramodern::input::callbacks_t`, RT64 `UserConfiguration` |
| Audio | Core1 audio microcode task | [src/tooie_audio_rsp.cpp](../src/tooie_audio_rsp.cpp) plus generated RSP code | `recomp::rsp::callbacks_t` |
| Saves | 16 Kbit EEPROM | `GameEntry{.save_type = Eep16k, .game_id = u8"bt.n64.us.1.0"}` and `recomp::register_config_path(<profile>)` in `main.cpp` (runtime `config/saves`, i.e. `<profile>/saves`); atomic promotion in [src/runtime_save_root.cpp](../src/runtime_save_root.cpp); import in [player_startup.cpp](../tools/baseline/player_startup.cpp) | Runtime PI and save code, with the save-writing patch below |

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

**Shared owner/sample identity**

[bk_pipeline.h](../tools/baseline/guest/silo-candidate/bk_pipeline.h) defines finite actor, player, attachment, view and phase domains. Actor IDs use validated creation generations; player IDs use the drawn roster slot, including separate split-player histories. This is arithmetic sample identity, not the old `next_block` palette allocator. Its generation reduction and roster reuse still have finite lifetime obligations.

**Per-draw and root state**

The selected branches in [draw_boundaries.c](../tools/baseline/guest/silo-candidate/draw_boundaries.c) call shared owner begin/end helpers. Scope tokens restore the enclosing owner and transform base. Root helpers in [bk_pipeline.c](../tools/baseline/guest/silo-candidate/bk_pipeline.c) save and restore traversal offset, bone state and cut policy. The root closes at the qualified normal return or bit40 join; late closure is diagnostic failure, not a substitute accepted route.

[bk_float.inc](../tools/baseline/guest/silo-candidate/bk_float.inc) restores nested float streams and suspends foreign inherited streams. Its 1024-record owner lookup protects current/adjacent-frame records, with stale-only reuse and explicit capacity refusal. [bk_select.inc](../tools/baseline/guest/silo-candidate/bk_select.inc) reserves supported branch spans for player and projected-shadow roots (actor roots keep the original traversal numbering, as in BK) and separately applies the conservative player branch-discontinuity guard. Those mechanisms are distinct from historical palette poisoning.

**Camera construction and storage**

Construction boundaries remain qualified hooks. Camera identity state is stored in the patch image, while qualified float projection/view matrices and restore commands use the original matrix allocation route expanded by [camera_pools.c](../tools/baseline/guest/camera-candidate/camera_pools.c). [camera_emitters.c](../tools/baseline/guest/camera-candidate/camera_emitters.c) handles emission and restoration. Do not describe all camera storage as the old static identity arrays.

The selected shared-renderer build omits the older DEFB0 ignore helper because the shadow owner supplies that path. It also omits the native NPC paid-close wrapper whose guest caller was retired. Non-shared compositions retain those implementations; generated helper tables and exact import checks follow the selected composition. The selected BK composition omits the backpack621 helper section and its 612-byte legacy state. The rate body, Hand862 and Trot still follow the backpack selection; section presence is tracked separately. Shared-section metadata is measured from the current image and checked against its linked ELF. Non-BK compositions retain all eight helpers and their state. The BK composition omits the unused NPC665 section, end routine and 16-byte sidecar. NPC665 selection remains explicit, with section presence represented separately; link checks reject residual helper, sidecar or relocation references. Non-BK NPC665 retains all eight helpers and its state. Older receipts require regeneration to supply the section-presence contract. The BK composition also omits the CPU625 helper section and its 464-byte state. CPU625 selection remains explicit for composition prerequisites, while section presence controls native registration, transports and diagnostic reads. F4 reports the absent CPU625 cursor as unavailable through its existing validity bit, rather than a valid zero. Link checks reject remaining helper, state or relocation references; non-BK retains its helpers and state. The BK player lifecycle no longer calls CPU625 or backpack621 retirement: their owner tables have no remaining population path for valid player objects. Trot retirement remains live. Non-BK lifecycle calls are unchanged. The legacy PC[8] secondary-route owner is also absent from the BK composition. BK still labels the split player directly from its staged callback and roster slot; shared layout-5 state remains compatible, and non-BK builds retain the legacy owner.

The selected BK composition also omits the actor CPU-root helper and its camera leaf/native thunk. Their DEFA0 transport was already disabled under BK. The actor palette remains. The BK composition also omits the legacy feather-ID helper: its DD024 path uses the BK list owner directly. Non-BK compositions retain both legacy helpers. Generated registry rows and section counts follow that selection.

**Player lifecycle and retained routes**

Option-box sprite groups use their component ID without adding the graph traversal offset. The guest handler owns that temporary offset choice and restores it after emitting the group; the native sprite thunk passes arguments without writing the BK offset. Billboard handlers retain their graph ordinal.

[player_lifecycle.c](../tools/baseline/guest/silo-candidate/player_lifecycle.c) retains original lifecycle work and compatibility paths for numbered producers. Retirement of CPU625/backpack621 labels alone does not describe all shared histories. Current ownership follows the actual drawn roster binding; unusual same-slot/model reuse requires separate evidence. [actor_palette.c](../tools/baseline/guest/silo-candidate/actor_palette.c) is still selected under BK. For each header-bit40 draw that neither the shadow scope nor the menu scope owns, the BK DEFB0 helper in [shadow_projection.c](../tools/baseline/guest/camera-candidate/shadow_projection.c) asks `tooie_actor_palette_begin` for a projection ID. The palette admits only an actor captured by `func_80101808`, under a qualified WORLD camera, whose model matches one of its finite recognizers: the palette leaf grammar, the save-select hand (Hand862) or the save-select props (4A8). An admitted draw always gets a WORLD projection ID from a `next_block` block (allocated when its binding is new or changed and otherwise reused). Under BK, when the same draw's BK actor owner is current (same birth, view and phase, nonzero ID, not an attachment scope), the palette pushes no enable, modelview group or prepaid pop, and its close skips the pop: the owner's type-3 list brackets bind every vertex load in its scope. This is statically qualified; runtime acceptance is pending. Any admission whose BK owner is not current (for example after BK scope overflow) keeps the palette's own modelview group and the paid pop/poison close. A failed close stops the player where `TOOIE_BRIDGE_REQUIRED_STOP` is compiled in. Every admission keeps the binding tripwire. Hand862 uses the same original-program object-memory and immutable-loader premise as the other admitted models; its CPU vector-cache writes remain original. Other header-bit40 draws that are neither shadow- nor menu-owned get projection ID 0 with IGNORE. See [Rendering](RENDERING.md) for current limits.

**Threads**

- The native observers in `adapters.cpp` use atomics.
- Guest patch state has no locks.
- Some source comments call fields "guest-thread-only".
- This guide and its source pass do not establish which guest threads run the allocator, draw and lifecycle patches. Nor do they show whether those patches can run concurrently.

### Overlay identity

A raw address is **not** unique. Many overlays are linked at the same VRAM range from `0x80800000`, so `0x80800380` names a different function in each overlay.

Overlay functions therefore carry their overlay in the symbol name. Examples are `func_80800380_chhoneycarrier`, `func_80800D8C_chnests` and `func_80800034_glglobalsettings`.

The generated registry identifies functions by their N64Recomp section entry (`recomp_overlays.inl`, ordered by `config/overlay-ids.txt`), not by address alone. In the selected BK camera build, Honey stays in its original raw overlay table; the selected `bsbtrot` bodies are restored into their original overlay function tables from generated rows that carry the overlay ID. Boot, core1 and core2 replacements use generated base and core2 rows (see [What is still not in that shape](#what-is-still-not-in-that-shape)).

## Traces

### Actor draw

1. The selected [draw_boundaries.c](../tools/baseline/guest/silo-candidate/draw_boundaries.c) preserves the actor's original argument resolution and draw order. Under `TOOIE_BK_PIPELINE`, it opens an owner scope with `tooie_bk_actor_draw_begin` and restores that scope afterward.
2. `tooie_bk_de448_begin` handles direct-world/player routes and nested attachments. Player identity is resolved from the staged draw callback and its actual roster-bound owner, not the controlled-player index.
3. Qualified hooks within `func_800DE498` invoke the shared root helpers. Normal and projection-only roots keep their original matrix roles. Root selection, cut policy and float production are established before graph emission.
4. Supported graph helpers emit matrix groups and preserve stable node correspondence. CPU-written leaf vertices temporarily suspend the earlier float stream. In the selected leaf handler, deformation callbacks run before that suspension bracket; inside it are only command writes and address conversion.
5. Normal root return or the bit40 join closes the matching root and restores enclosing traversal/float state. `tooie_bk_de448_end` then restores the owner scope. A missing expected close is counted as a defect.
6. At submission, the exact display-list root is registered for runtime acknowledgement; its data is not reused until the acknowledgement arrives. Suppressed constructions do not wait. See [Rendering](RENDERING.md) for the distinct construction and submitted-history rules.

### A setting: output frame rate

1. The menu's "Output rate" control in `minimum_menu.cpp` queues a preference request. `host::tick` applies it, and `minimum_host.cpp` persists the accepted value and publishes one packed rate request.
2. At startup, `minimum_presentation.cpp` maps the saved value to RT64 `UserConfiguration::RefreshRate::Original`, `Display` or `Manual` with its manual target. Default also selects Original.
3. During play, the graphics owner in `devices.cpp` consumes the latest request and queues a rate-only runtime configuration update. `minimum_presentation.cpp` changes only RT64's refresh-rate fields under the shared configuration lock. Aspect and pending framebuffer changes remain owned by their existing paths.
4. RT64 interpolation produces the higher output rate. No separate 60 FPS or 120 FPS game patch exists. Present Early remains selected for the player at every output rate.

The **Fullscreen** checkbox in `minimum_menu.cpp` requests borderless desktop
fullscreen through SDL in `minimum_host.cpp`. The menu reflects actual window
flags; a failed change shows an error and is not saved. VSync also applies
during play. Resolution requests pass through `devices.cpp::update_screen` to
`minimum_presentation.cpp::update_resolution` under the RT64 configuration lock.
Gameplay aspect and cutscene aspect use the live guest/renderer boundaries described above.

At player startup, `minimum_presentation.cpp` enables RT64's existing
`textureLOD.scale` setting so mip choice follows rendered resolution. This is
source behavior; texture appearance and missing-mipmap shimmer have not been
visually accepted.

### Renderer label owner

Use the selected definitions rather than historical palette formulas:

- Actor and player sample windows: `bk_actor_id`, `bk_player_id` and owner scopes in [bk_pipeline.c](../tools/baseline/guest/silo-candidate/bk_pipeline.c), with domains and bounds in [bk_pipeline.h](../tools/baseline/guest/silo-candidate/bk_pipeline.h).
- Graph offsets and selector spans: [bk_graph.inc](../tools/baseline/guest/silo-candidate/bk_graph.inc), [bk_sort.inc](../tools/baseline/guest/silo-candidate/bk_sort.inc) and [bk_select.inc](../tools/baseline/guest/silo-candidate/bk_select.inc).
- Sequential player attachment: [bk_attach.inc](../tools/baseline/guest/silo-candidate/bk_attach.inc).
- Shadow identity and lifetime: [bk_shadow_attach.inc](../tools/baseline/guest/silo-candidate/bk_shadow_attach.inc) and [shadow_projection.c](../tools/baseline/guest/camera-candidate/shadow_projection.c). Cuts keep identity while changing interpolation policy.
- Camera/view/projection identity: [camera_state.c](../tools/baseline/guest/camera-candidate/camera_state.c) and [camera_emitters.c](../tools/baseline/guest/camera-candidate/camera_emitters.c).
- Menu occurrence windows: [menu_scope.hpp](../tools/baseline/menu_scope.hpp) and the selected menu hook/thunk recipe. An unregistered drawn option can claim a free or inactive slot; current and previous construction users are protected. New admissions and gaps cut history. Address relocation can still cost a cut; this is not stable semantic identity across relocation. Transition projection roles are separately scoped; neither proves pixel-level edge correctness.
- Unowned UI models: the core `DF944` draw wrapper supplies a scoped model and projection ID through [ui_scope.hpp](../tools/baseline/ui_scope.hpp) and the existing menu helpers. Already-owned draws keep their IDs. The fallback identity is the model plus its draw ordinal within the construction; first sightings and gaps cut history. This is not semantic element identity: removing an earlier copy of the same model can shift the remaining ordinals. Overflow leaves the original draw unchanged.
- F4 unowned-root witness: [issue_evidence_guest.hpp](../tools/baseline/issue_evidence_guest.hpp) joins the DE498 entry position to its exact stack frame and construction. An actor/resource association requires a live descriptor and pool match at position minus four. It does not infer an owner from a stale callback or translated return register, and it never changes rendering admission.

NPC665 and feather ID formulas are not reached under BK, and no legacy formula is a map of the current shared domains. The palette formula still supplies the projection ID for every header-bit40 actor draw it admits, and its modelview group ID only for owner-fallback admissions. Where an engineering configuration enables it (the player does not), the legacy player boundary still emits block IDs (see [What is still not in that shape](#what-is-still-not-in-that-shape)). Determine which compile-time branch executes before using any of them to interpret a trace. Root-ID ordinal zero can occur more than once; a label alone is not a unique matrix or exact guest-frame join.

### Adding a guest patch

1. Write the C function in the right `tools/baseline/guest/` file, in `.recomp_patch`. Follow [Rules for changes](#rules-for-changes).
2. Update the owning preparation script and the composition provider/import lists. Follow the selected owner from `prepare_camera_bridge.py`; for example, minimum-host object checks live in `prepare_minimum_host.py`.
3. For camera providers, also update `camera-candidate/camera_hook_contract.json`.
4. Regenerate. The object, import, relocation and image checks fail until all lists agree.
5. Review the change. Then update the image pins (see [Windows build guide](WINDOWS.md#reusing-or-regenerating-a-build), "Guest C edits").

## Pinned upstream and carried patches

The project maintains these source changes against pinned upstream versions.

**Runtime**

- N64ModernRuntime is pinned at `ca568b6a…`.
- At configure time, [prepare_runtime_baseline_selected.py](../tools/prepare_runtime_baseline_selected.py) composes child sources for `events`, `timer`, `threads`, `threadqueue` and `mesgqueue` (ultramodern), plus `dp`, `recomp` and `pi` (librecomp).
- Its `PATCHES` table names the maintained patches in `patches/runtime/`, including `events-q-vi-lock`, `timer-guest-clock` and `mesgqueue-required-admission`.
- Its `RECIPES` and `EXPECTED` tables pin the supporting producers and selected child outputs.
- Each input and output is checked against a pinned hash.
- [patches/runtime/PROVENANCE.md](../patches/runtime/PROVENANCE.md) lists each child's derivation, including the edits still made by Python recipes.

**RT64**

- RT64 is pinned at `6f1c2d99…`.
- `TOOIE_BASELINE_RT64_GENERIC_FIXES` applies five recipes from `patches/rt64/`: `owner_index_guard`, `transform_upload_guard`, `rotation_interpolation`, `pop_matrix_group_projection` and `projection_residual`. The last selects only projection-preservation operations 4/5: source combined-matrix preservation and mapped decomposition residuals, without product camera heuristics. CPU matrix/depth qualification does not establish visual acceptance.
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

Creation stamping is retained, but the selected shared renderer computes owner/sample windows directly instead of allocating a palette block for every admitted binding. Player identity is roster-based. Float history uses bounded stale-only storage; cuts and model/adjacency checks govern reuse. Neither an unlimited identity guarantee nor identical map-reset policy is claimed. Under the selected BK pipeline, the monotonically increasing `next_block` allocator still serves the legacy player boundary (only where an engineering configuration enables it; the player does not) and the actor palette's header-bit40 admissions (their projection IDs, and the palette group where it is still pushed); BK owner and sample IDs do not use it.

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

The selected renderer now has shared root, bone, list and billboard tagging, stable sorted traversal and type13 bounding-box traversal. Supported type12 alternatives reserve fixed spans. The actual Tooie grammar, handler identities and finite ID windows are checked rather than copied from another game's layout. Unsupported cases keep their original drawing path, with explicit correspondence limits. Float deformation, split-player scopes and branch-discontinuity policy are described in [Rendering](RENDERING.md).

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

Qualified native hooks enter guest root helpers, and selected guest draw boundaries carry lexical owner tokens. Normal root return and the projection-only join restore enclosing traversal, cut and float state. A cut preserves a stable object's ID while withholding interpolation. The depth-one palette pop/poison sequence is not the shared root contract. It still runs for palette admissions whose BK actor owner is not current. Owner-bound admissions push no group and skip the pop, but keep the binding tripwire. Native hook ABI and exact original instruction ordering remain qualification obligations.

### Mod consequences

**BK port**

- Mods can attach data to markers and read spawn indices through exported functions ([marker_extension_patches.c L92–L110](https://github.com/BanjoRecomp/BanjoRecomp/blob/dbd52154bc1e00e56867b1fac94a8f907052f41b/patches/marker_extension_patches.c#L92-L110)).
- Mods can set transform IDs ([transform_tagging.c L959–L970](https://github.com/BanjoRecomp/BanjoRecomp/blob/dbd52154bc1e00e56867b1fac94a8f907052f41b/patches/transform_tagging.c#L959-L970)).
- The handle array is larger than the game's normal marker count, so that mods can add markers.

**Tooie, selected local player**

- Runtime discovers texture packs under the owned profile's `mods` folder and supplies their manifest and order. When Runtime loads mods at game start, the native enable callback stages each pack's path; the graphics owner applies the staged set once, through RT64's public texture cache after upload completion. The disable callback is registered but has no caller: there is no pack menu, so packs are never toggled or reordered while the game runs. Moving a file out of `mods`, or editing `mods.json`, takes effect on the next start. No custom archive loader or code-mod hook is added.
- Where packs replace the same texture, the first entry in Runtime's mod order wins, as in RecompFrontend's renderer. Packs Runtime has not seen before are ordered ahead of known ones. Runtime accepts the same texture-only content from mod folders and `.nrm` files; the guide documents only `.rtz`. A pack whose config declares RecompFrontend's HD option (`_recomp_texture_pack_enabled`, a two-value enum or a bool) is applied only while its stored or default value is nonzero, as in that renderer; the option cannot be changed while the game runs.
- Failure stays closed. A mod with code or ROM-patch content refuses start, naming its file and the `mods` folder. A pack that fails Runtime's load checks (for example a higher minimum version or a missing dependency) stops the game from starting; Runtime's report gains the same folder before it is logged. If RT64 cannot open a staged pack, every pack is cleared and the original textures are used. `player.log` records both start messages with the folder; the dialog is shown only after terminal closure.
- Side tables match the original 200-entry pool. Descriptors outside it are refused, not tracked.

**Tooie, if code mods were added later**

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
- BK's complete ID-domain allocation: Tooie adopts 256-ID sample windows but adds its own actor/player/view/phase/attachment domains and finite bounds.
- BK's native provider and its reset policy.
- BK's overlay table and names.

Option-text conversion state is owned by the maintained guest text helper: queue admission, construction limits, glyph conversion, projection open/close and cursor writes. Native text thunks forward arguments and retain diagnostic witness counters. Menu occurrence identity remains with its existing owner. This migration has translated-fixture and native binding validation; it does not claim new gameplay or warp-menu jitter acceptance.
