# Shawky Call — Plan (Phase 0)

## Upstream facts (verified 2026-10-31)
- Latest **published source**: MicroSIP **3.22.16** (`https://www.microsip.org/download/MicroSIP-3.22.16-src.7z`,
  sha256 `9c1ff942ae1ae56bdfcdd04c3d6e242a056d7f0dd50bb19f6e13226e7c819b83`). Latest binary is 3.22.20, but its source is not published yet.
- PJSIP: **2.15.1** (per MicroSIP changelog "pjsip update 2.15.1"). Not bundled in the archive.
- License: GPL v2 or later, copyright headers "2011-2026 MicroSIP".
- Project: `microsip.vcxproj` (MFC, Unicode), configs Win32/x64/ARM64. It expects to sit **inside a pjproject tree**
  (`..\pjlib`, `..\build\vs\pjproject-vs14-common-config.props`, `..\third_party\...`).
- Third-party deps: pjproject 2.15.1, opus, bcg729, OpenSSL, SDL, SQLiteCpp, `chkstk_ms.lib`; jsoncpp is bundled.
- `_GLOBAL_KEY` (INI password encryption key) is **redacted** (`*********`) in the public source.
- Single instance plus WM_COPYDATA command-line forwarding already exist (`microsip.cpp:~690`, `CmainDlg::CommandLine`).
- Translation: `langpack_*.txt` next to the exe, with RTL support. **Arabic is 100% translated** upstream.
- Upstream update check: `http://update.microsip.org/softphone-update.txt` (`mainDlg.cpp:5866`).

## Build environment
- Visual Studio 2022 (MSVC v143), Windows SDK 10.0.22621, x64 Release, MFC static.
- The dev container is Linux, so it **cannot build or run the Windows exe**. CI uses a GitHub Actions `windows-2022` runner.
- Linux can run: unit tests for the pure C++ parser and dial rules (g++), PowerShell scripts (pwsh), and an Inno Setup syntax check.

## Layout
```
microsip/            MicroSIP 3.22.16 source (tag upstream-3.22.16 = unmodified import)
shawky/              new code: ShawkyUri.{h,cpp}, DialRule, UrlScheme (HKCU), Autostart, Updater, Branding
tests/               unit tests (portable C++, no MFC)
installer/           ShawkyCall.iss, config handling
tools/               New-ShawkyIni.ps1 (CSV -> ini), publish-update.ps1, sign.ps1
build/               build-deps.ps1 (pjproject 2.15.1 + deps), build.ps1
SHAWKY_CHANGES.md    every touched upstream file
```

## Upstream files expected to change (marked `// SHAWKY:`)
- `microsip.cpp`: forward `shawkycall://` to the running instance, plus a first-run repair of the scheme and Run key.
- `mainDlg.cpp`: in `CommandLine()`, route `shawkycall:` to `shawky::HandleUri`, check registration, show an Arabic error; replace `CheckUpdates()`; tray tooltip.
- `const.h`, `define.h`: name "Shawky Call", version, update URL, key from a build define.
- `main.rc`, `res/*.ico`, `res/main.rc2`: icon, VERSIONINFO, About box, removed links.
- `settings.cpp`: default language Arabic, autostart default ON.
- `microsip.vcxproj`: add `shawky/*.cpp`, optional signing post-build.

## Risks
1. **Dependency build (biggest risk).** Upstream does not publish its exact pjproject `config_site.h` or any pjproject patches.
   Fallback: disable video/SDL first and get audio-only working.
2. `_GLOBAL_KEY` is redacted, so we must choose how the SIP password is stored (decision needed).
3. No real Windows test inside this container. GUI behavior must be verified on an office PC with the Arabic checklist.
4. 3.22.20 fixes are not available as source. Rebase when upstream publishes it.
