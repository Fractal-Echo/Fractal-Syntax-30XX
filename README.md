# Fractal Syntax 30XX

Windows C++ ImGui/OpenGL menu for 30XX.

## Build Locally

Requirements:

- Windows 10/11
- Visual Studio 2022 with the Desktop development with C++ workload
- CMake 3.24 or newer

Build:

```powershell
python scripts/ci/validate_offsets.py Main.txt
cmake --preset windows-msvc-x64
cmake --build --preset windows-msvc-x64-release
```

The executable is written to:

```text
build/windows-msvc-x64/bin/Release/FractalSyntax30XX.exe
```

## GitHub Actions

`.github/workflows/windows-build.yml` runs on `windows-2022` for pushes, pull requests, and manual dispatch.

The workflow:

1. Checks out the repository.
2. Prints runner, CMake, Python, and disk state.
3. Validates `Main.txt` before compiling.
4. Configures and builds the Release x64 target with MSVC.
5. Uploads `FractalSyntax30XX.exe` and `SHA256SUMS` as a workflow artifact.

The workflow does not use secrets, private paths, self-hosted runners, device access, or release publishing.

## Offset Config Validation

`scripts/ci/validate_offsets.py` fails if:

- Required sections or keys are missing.
- Hex offsets are malformed.
- Pointer bases do not exist in `[BASE]`.
- Patch byte lists contain invalid bytes.
- Patch enabled/disabled byte counts differ.
