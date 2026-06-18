# Review Notes

## What Changed

- Added guarded `SafeRead`, `SafeWrite`, and `SafePatch` helpers in `ConsoleIX.cpp`.
- Replaced unchecked pointer-chain resolution during offset parsing with checked safe reads.
- Added checked runtime address resolution before marking attach successful.
- Centralized reset and auto-attach behavior through `TryAttachAndLoadOffsets`.
- Changed failed attach behavior to detach, clear runtime state, and retry later.
- Changed offset loading to prefer local `Main.txt`, then fall back to this fork's raw `Main.txt`.
- Added `.gitignore` entries for common Visual Studio and Windows build outputs.
- Added remote build/review assets from `origin/codex/windows-actions-build-env`: `.gitattributes`, CMake config, CMake presets, Windows GitHub Actions workflow, and offset validation script.
- Added README build/review documentation.

## Why The Change Was Needed

During the game's loading phase, the process can exist before required runtime pointers are valid. The previous auto-attach flow parsed offsets, read unchecked map entries, and immediately used resolved addresses. If a pointer chain was unreadable or unresolved during loading, the app could keep an invalid address and attempt reads or writes too early.

The new flow fails closed instead: required addresses must resolve, `MWeapon` must pass a safe probe read, and every runtime write path must have a live process handle and nonzero address.

## Verified

- `git diff --check` passed before documentation changes.
- No direct `ReadProcessMemory` calls remain outside `SafeRead` in `ConsoleIX.cpp`.
- No direct `WriteProcessMemory` calls remain outside `SafeWrite` / `SafePatch` in `ConsoleIX.cpp`.
- No `mem::PatchEx` calls remain in `ConsoleIX.cpp`.
- No unchecked runtime `BaseMap["key"]` or `AddressMap["key"]` reads remain; bracket use is limited to parser assignment.
- The old hardcoded `Jay-Lexington404/UwU-30XX` offset URL has no matches.
- Failed attach paths call `DetachProcess()`.
- Process loss and shutdown call `DetachProcess()`.
- `ResolveRuntimeAddresses()` requires address resolution and a successful `SafeRead` probe before attach success.
- `python3 scripts/ci/validate_offsets.py Main.txt` passed in this environment.
- `git diff --check` passed after adding build assets.

## Not Verified In This Environment

- Windows x64 Debug build.
- Windows x64 Release build.
- Runtime behavior against a live `30XX.exe` process.
- Manual UI toggle behavior in-game.
- CMake configure/build, because `cmake` is not installed in this environment.

Build verification was not possible because Windows compiler tooling was not detected in this environment. After adding the missing remote build assets, the tree includes `ConsoleIX.vcxproj`, `ConsoleIX.vcxproj.filters`, `ConsoleIX.vcxproj.user`, `CMakeLists.txt`, and `CMakePresets.json`, but no `.sln` or `Makefile` is checked in.

## Windows Manual Tests Still Required

- Build x64 Debug on Windows.
- Build x64 Release on Windows.
- Launch the app before 30XX and confirm waiting/detached state.
- Start 30XX and confirm attach succeeds only after required addresses are readable.
- Close 30XX and confirm detach/retry does not crash.
- Test Reset/Reattach.
- Test missing `Main.txt` failure path.
- Test invalid or stale `Main.txt` failure path.
- Test each UI toggle/write path only writes while attached and resolved.
- Confirm no writes happen to address `0`.
