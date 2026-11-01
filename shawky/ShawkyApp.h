/*
 * Shawky Call - customized MicroSIP for Shawky Group
 * Copyright (C) 2026 CWS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

// Windows glue between MicroSIP (MFC) and the portable shawky code.
// Hooks called from upstream files are marked "// SHAWKY:" there.

#pragma once

#include <afxstr.h>

namespace shawky {

// Called once from CmainDlg::OnInitDialog before the command line is processed.
// Repairs the HKCU shawkycall:// registration if it is missing or points to
// another exe.
void OnStartup();

// Called from CmainDlg::CommandLine (both for the first instance and for
// URIs forwarded from a second instance with WM_COPYDATA).
// Returns true if params was a shawkycall: URI (handled here, upstream must
// not process it), false otherwise.
bool TryHandleCommandLine(const CString& params);

// Writes HKCU\Software\Classes\shawkycall for the given exe. Returns true on success.
bool RegisterUrlScheme(const CString& exePath);

// True when the active SIP account is registered (last REGISTER got 2xx).
bool IsRegisteredToPbx();

}  // namespace shawky
