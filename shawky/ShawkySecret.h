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

// SIP password storage in the ini file, protected with Windows DPAPI for the
// current Windows user (CryptProtectData). No key is stored in the binary.
//
// Stored format: "dpapi1:" + base64(DPAPI blob).
// A value without the prefix is treated as plaintext (for example written by
// the installer from the employee config); MicroSIP's loader then encrypts
// it and writes it back on first start.

#pragma once

#include <afxstr.h>

namespace shawky {

// Same contract as upstream IniDecrypt: returns true when str is now the
// plaintext (or empty); false when str is not an encrypted value (plaintext),
// so the caller re-encrypts it.
bool SecretDecrypt(CString& str);

// Returns the protected form; on failure returns an empty string (never the
// plaintext).
CString SecretEncrypt(const CString& plain);

}  // namespace shawky
