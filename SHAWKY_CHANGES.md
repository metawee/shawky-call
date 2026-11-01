# Shawky Call: changes to upstream MicroSIP

Upstream: MicroSIP 3.22.16 (tag `upstream-3.22.16`, see `UPSTREAM.md`).
Every change inside `microsip/` is marked `// SHAWKY:` (`<!-- SHAWKY: -->` in XML).
To list them: `git diff upstream-3.22.16 -- microsip/`.

## Changed upstream files

| File | Change | Phase |
|---|---|---|
| `microsip/mainDlg.cpp` | Include `../shawky/ShawkyApp.h`; call `shawky::OnStartup()` before the command line is processed (repairs the HKCU URL scheme); `CommandLine()` routes `shawkycall:` URIs to `shawky::TryHandleCommandLine()` before upstream handling. | 1 |
| `microsip/mainDlg.h` | Link the x64 pjproject library name (`libpjproject-x86_64-x64-vc14-*`) on `_WIN64`. | 1 |
| `microsip/settings.cpp` | `IniEncrypt`/`IniDecrypt` use per-user DPAPI (`shawky/ShawkySecret.cpp`) instead of upstream's unpublished static `_GLOBAL_KEY`. | 1 |
| `microsip/const.h` | `_GLOBAL_VIDEO` disabled (audio-only build; no SDL/FFmpeg). | 1 |
| `microsip/microsip.vcxproj` | Add `shawky/*.cpp`; `ShawkyStrings.cpp` compiled with `/utf-8`; remove `chkstk_ms.lib` (MinGW helper not needed with MSVC-built deps). | 1 |

## New files (not upstream)

| Path | Purpose |
|---|---|
| `shawky/ShawkyUri.{h,cpp}` | Portable `shawkycall://` parser, percent/UTF-8 validation, dial rules and number normalization. |
| `shawky/ShawkyApp.{h,cpp}` | Windows glue: URL scheme registration (HKCU), dial after registration, Arabic errors. |
| `shawky/ShawkySecret.{h,cpp}` | DPAPI password protection. |
| `shawky/ShawkyStrings.{h,cpp}` | Arabic and English strings added by Shawky Call. |
| `tests/` | Unit tests (CMake, runs on Linux and Windows). |
| `build/` | `config_site.h` for pjproject, `build-deps.ps1`, `build.ps1`. |
| `lang/langpack_arabic.txt` | Upstream MicroSIP Arabic translation (exported from microsip.org/translation). |
| `.github/workflows/build.yml` | CI: unit tests (gcc + MSVC) and the x64 build. |
| `docs/` | Arabic test checklist and manual-test links. |
