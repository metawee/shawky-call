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

#include "stdafx.h"
#include "ShawkySecret.h"

#include <wincrypt.h>
#pragma comment(lib, "Crypt32.lib")

namespace shawky {

namespace {

const TCHAR kPrefix[] = _T("dpapi1:");
const size_t kPrefixLen = _countof(kPrefix) - 1;

// Not a secret: only binds blobs to this application's purpose.
const char kEntropy[] = "ShawkyCall/SIP-password/v1";

}  // namespace

bool SecretDecrypt(CString& str) {
    if (str.IsEmpty()) return true;
    if (str.Left((int)kPrefixLen) != kPrefix) return false;  // plaintext

    CString b64 = str.Mid((int)kPrefixLen);
    DWORD size = 0;
    if (!CryptStringToBinary(b64, 0, CRYPT_STRING_BASE64, NULL, &size, NULL, NULL) || size == 0) {
        str.Empty();
        return true;
    }
    CByteArray blob;
    blob.SetSize(size);
    if (!CryptStringToBinary(b64, 0, CRYPT_STRING_BASE64, blob.GetData(), &size, NULL, NULL)) {
        str.Empty();
        return true;
    }
    DATA_BLOB in = {size, blob.GetData()};
    DATA_BLOB entropy = {(DWORD)(sizeof(kEntropy) - 1), (BYTE*)kEntropy};
    DATA_BLOB out = {0, NULL};
    if (!CryptUnprotectData(&in, NULL, &entropy, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &out)) {
        // Blob from another Windows user or machine: drop it, never use it as a password.
        str.Empty();
        return true;
    }
    CString plain((LPCWSTR)out.pbData, (int)(out.cbData / sizeof(WCHAR)));
    SecureZeroMemory(out.pbData, out.cbData);
    LocalFree(out.pbData);
    str = plain;
    return true;
}

CString SecretEncrypt(const CString& plain) {
    if (plain.IsEmpty()) return CString();
    DATA_BLOB in = {(DWORD)(plain.GetLength() * sizeof(WCHAR)), (BYTE*)(LPCWSTR)plain};
    DATA_BLOB entropy = {(DWORD)(sizeof(kEntropy) - 1), (BYTE*)kEntropy};
    DATA_BLOB out = {0, NULL};
    if (!CryptProtectData(&in, L"Shawky Call SIP password", &entropy, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &out)) {
        return CString();
    }
    DWORD chars = 0;
    CString result;
    if (CryptBinaryToString(out.pbData, out.cbData, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &chars)) {
        CString b64;
        LPTSTR buf = b64.GetBuffer(chars);
        if (CryptBinaryToString(out.pbData, out.cbData, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, buf, &chars)) {
            b64.ReleaseBuffer(chars);
            result = CString(kPrefix) + b64;
        } else {
            b64.ReleaseBuffer(0);
        }
    }
    LocalFree(out.pbData);
    return result;
}

}  // namespace shawky
