# Fractal Syntax 30XX

Fractal Syntax 30XX is an external local-use 30XX developer/customization menu. This repository contains the menu source, Visual Studio/CMake build assets, bundled headers/libs, and `Main.txt` offset configuration used by the application at runtime.

## Build Status

This is a Windows project and requires Windows build tooling.

Current limitation: this tree includes `ConsoleIX.vcxproj`, `ConsoleIX.vcxproj.filters`, `ConsoleIX.vcxproj.user`, `CMakeLists.txt`, and `CMakePresets.json`, but no `.sln` or `Makefile` is checked in. The Windows build was not verified in this environment because Windows compiler tooling was not detected.

The CMake path is Windows-only and is configured for Visual Studio 17 2022 x64:

```powershell
cmake --preset windows-msvc-x64
cmake --build --preset windows-msvc-x64-release
```

CI support is provided by `.github/workflows/windows-build.yml`, which validates `Main.txt`, configures CMake, builds the Release target, and uploads the executable plus SHA256 sums.

Known source/dependency inputs currently checked in:

- `ConsoleIX.cpp`
- `ConsoleIX.vcxproj`
- `ConsoleIX.vcxproj.filters`
- `CMakeLists.txt`
- `CMakePresets.json`
- `proc.cpp` / `proc.h`
- `mem.cpp` / `mem.h`
- `Signature.cpp` / `Signature.h`
- `Helpers.h`
- `scripts/ci/validate_offsets.py`
- `include/`
- `lib/`
- `Main.txt`

## Offset Loading

At startup and during reset/reattach, the menu loads offsets in this order:

1. `Main.txt` next to the executable.
2. `Main.txt` in the current working directory.
3. Raw `Main.txt` from this fork:
   `https://raw.githubusercontent.com/Fractal-Echo/Fractal-Syntax-30XX/main/Main.txt`

If offset loading fails, address resolution fails, or a required pointer is unreadable, the attach attempt fails closed. The process handle is closed, runtime addresses are cleared, and the auto-attach loop retries later.

## Safety Behavior

The attach flow is intentionally conservative:

- Process memory reads go through `SafeRead`.
- Runtime writes go through `SafeWrite`.
- Patch writes go through `SafePatch`.
- Writes fail when the process handle is detached or exited.
- Writes fail for address `0`.
- Runtime addresses are resolved through checked lookups.
- Attach is not marked successful until required addresses resolve and `MWeapon` can be safely read.

## Manual Verification Checklist

- Build x64 Debug on Windows.
- Build x64 Release on Windows.
- Launch the app before 30XX and confirm it stays detached/waiting.
- Start 30XX and confirm attach succeeds after the game is ready.
- Close 30XX and confirm detach/retry does not crash.
- Test Reset/Reattach.
- Test missing `Main.txt`.
- Test invalid or stale `Main.txt`.
- Test each UI write path only while attached and resolved.
- Confirm no address `0` writes occur.
- Confirm the old hardcoded upstream offset URL is not used.
