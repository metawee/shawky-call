# Shawky Call

Windows softphone for the Shawky Group sales team. It is a fork of
[MicroSIP](https://www.microsip.org) 3.22.16 (GPL v2 or later) on
[PJSIP](https://www.pjsip.org) 2.15.1.

- `microsip/`: upstream source. Our changes are marked `// SHAWKY:` and listed in `SHAWKY_CHANGES.md`.
- `shawky/`: our code.
- License: GPL v2 or later (`COPYING`). Upstream copyright notices are kept.

## Features (phase 1)

- `shawkycall://call?phone=+201001234567&name=...` dials through the registered SIP account.
  - The phone number is URL-decoded (UTF-8), validated and normalized with the dial rules.
  - Other parameters are accepted and ignored for now.
  - Malformed input is rejected with an Arabic message. URI text is never passed to a shell.
- Single instance: a second launch forwards the URI to the running app (`WM_COPYDATA`) and exits.
- If the app is not registered to the PBX, it waits for registration: up to 15 s right after start, 3 s when already running. Then it shows an Arabic message and does not dial.
- The URL scheme is registered per user under `HKCU\Software\Classes\shawkycall` and repaired on every start.
- The SIP password in the ini is protected with Windows DPAPI for the current user. No key is in the binary.
- Arabic UI: `langpack_arabic.txt` next to the exe. It is the upstream translation, with RTL support.

## Configuration (`[Shawky]` section of the MicroSIP ini)

| Key | Default | Meaning |
|---|---|---|
| `DialRules` | `+20>0;0020>0;20>0:11` | `from>to[:minDigits]` separated by `;`. The first match wins. `:11` means the rule applies only to numbers with at least 11 digits, so extensions are never rewritten. |
| `DialPrefix` | *(empty)* | Digits prepended after the rules, e.g. an outbound route prefix. |

Normalization:
1. Strip spaces, `-`, `.`, `(`, `)`, NBSP and bidi marks.
2. Map Arabic-Indic digits to ASCII digits.
3. Apply the first matching rule.
4. Add the prefix.
5. The result must be 2 to 15 ASCII digits.

## Build

Reproducible build, the same one CI runs (`.github/workflows/build.yml` on `windows-2022`).

Requirements:
- Windows 10 or 11 x64.
- **Visual Studio 2022** (MSVC toolset **v143**) with the "Desktop development with C++" workload and the "C++ MFC for latest v143 build tools (x86 & x64)" component.
- Windows 10/11 SDK.
- Git and CMake 3.16 or newer (both come with VS 2022).

From a *Developer PowerShell for VS 2022* in the repository root:

```powershell
./build/build.ps1            # first run builds the deps (about 30-60 min), later runs are faster
./build/build.ps1 -SkipDeps  # rebuild only Shawky Call
```

Output is in `out\ShawkyCall\` (exe, `langpack_arabic.txt`, `COPYING`).

Pinned dependencies (see `build/build-deps.ps1`):

| Dependency | Version | Source |
|---|---|---|
| pjproject | 2.15.1 (`7de6e686`) | github.com/pjsip/pjproject |
| vcpkg | 2026.07.29 (`9e593bb1`) | opus, openssl, sqlitecpp, sqlite3 (`x64-windows-static`) |
| bcg729 | 1.1.1 (`faaa8958`) | github.com/BelledonneCommunications/bcg729 |

pjproject options are in `build/config_site.h`: TLS/SRTP/DTLS via OpenSSL, Opus, G.729, no video.

## Tests

```bash
cmake -S tests -B build-tests && cmake --build build-tests && ./build-tests/test_uri
```

The manual office checklist (Arabic) is in `docs/TESTING-ar.md`.
