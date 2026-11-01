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
#include "ShawkyApp.h"
#include "ShawkyStrings.h"
#include "ShawkyUri.h"

#include "mainDlg.h"
#include "settings.h"
#include "global.h"

#include <string>
#include <vector>

namespace shawky {

namespace {

const UINT kPollMs = 300;
const DWORD kWaitColdStartMs = 15000;  // app just started: wait for the first REGISTER
const DWORD kWaitRunningMs = 3000;     // app already running: short grace only
const DWORD kColdStartWindowMs = 30000;

DWORD g_startTick = 0;
UINT_PTR g_timer = 0;
CString g_pendingNumber;
DWORD g_pendingSince = 0;
DWORD g_pendingWait = 0;
StringId g_pendingError = StringId::None;

std::string ToUtf8(const CString& s) {
    if (s.IsEmpty()) return std::string();
    int n = WideCharToMultiByte(CP_UTF8, 0, s, s.GetLength(), NULL, 0, NULL, NULL);
    std::string out(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s, s.GetLength(), &out[0], n, NULL, NULL);
    return out;
}

CString FromUtf8(const std::string& s) {
    if (s.empty()) return CString();
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), NULL, 0);
    CString out;
    LPWSTR buf = out.GetBuffer(n);
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), buf, n);
    out.ReleaseBuffer(n);
    return out;
}

void Log(const char* fmt, const char* a = "", const char* b = "") {
    if (is_pjsua_running()) {
        PJ_LOG(3, ("shawky", fmt, a, b));
    }
}

CString ReadShawkySetting(LPCTSTR key, LPCTSTR def) {
    const TCHAR kMissing[] = _T("\x01");
    CString value;
    LPTSTR ptr = value.GetBuffer(1024);
    GetPrivateProfileString(_T("Shawky"), key, kMissing, ptr, 1024, accountSettings.iniFile);
    value.ReleaseBuffer();
    if (value == kMissing) return CString(def);
    return value;
}

void LoadDialConfig(std::vector<DialRule>& rules, std::string& prefix) {
    CString defRules(kDefaultDialRules);
    std::string spec = ToUtf8(ReadShawkySetting(_T("DialRules"), defRules));
    std::string err;
    if (!ParseDialRules(spec, rules, &err)) {
        Log("invalid DialRules in ini (%s), using defaults", err.c_str());
        ParseDialRules(kDefaultDialRules, rules, NULL);
    }
    prefix = ToUtf8(ReadShawkySetting(_T("DialPrefix"), _T("")));
    prefix.erase(0, prefix.find_first_not_of(" \t"));
    prefix.erase(prefix.find_last_not_of(" \t") + 1);
}

void StopTimer() {
    if (g_timer) {
        KillTimer(NULL, g_timer);
        g_timer = 0;
    }
}

void ShowError(StringId id) {
    HWND owner = (mainDlg && ::IsWindow(mainDlg->m_hWnd)) ? mainDlg->m_hWnd : NULL;
    ::MessageBoxW(owner, GetString(id), GetString(StringId::AppName),
                  MB_OK | MB_ICONWARNING | MB_SETFOREGROUND | MB_TOPMOST | MB_RTLREADING | MB_RIGHT);
}

void Dial(const CString& number) {
    if (!mainDlg) return;
    if (mainDlg->pageDialer) {
        CString n = number;
        mainDlg->pageDialer->DialedAdd(n);
    }
    // Same path as typing the number and pressing Call.
    mainDlg->MakeCall(number, false, true);
}

VOID CALLBACK PollTimerProc(HWND, UINT, UINT_PTR, DWORD) {
    // Deferred error (never show a modal box inside WM_COPYDATA: the second
    // instance would block in SendMessage until the user clicks OK).
    if (g_pendingError != StringId::None) {
        StringId id = g_pendingError;
        g_pendingError = StringId::None;
        if (g_pendingNumber.IsEmpty()) StopTimer();
        ShowError(id);
        return;
    }
    if (g_pendingNumber.IsEmpty()) {
        StopTimer();
        return;
    }
    if (IsRegisteredToPbx()) {
        CString number = g_pendingNumber;
        g_pendingNumber.Empty();
        StopTimer();
        Log("dialing %s", MaskNumber(ToUtf8(number)).c_str());
        Dial(number);
        return;
    }
    if (GetTickCount() - g_pendingSince >= g_pendingWait) {
        Log("not registered, call to %s dropped", MaskNumber(ToUtf8(g_pendingNumber)).c_str());
        g_pendingNumber.Empty();
        StopTimer();
        ShowError(StringId::NotRegistered);
    }
}

void StartTimer() {
    if (!g_timer) g_timer = SetTimer(NULL, 0, kPollMs, PollTimerProc);
}

void QueueError(StringId id) {
    g_pendingError = id;
    StartTimer();
}

CString ExpectedCommand(const CString& exePath) {
    CString cmd;
    cmd.Format(_T("\"%s\" \"%%1\""), (LPCTSTR)exePath);
    return cmd;
}

bool SetRegString(HKEY root, LPCTSTR subkey, LPCTSTR name, const CString& value) {
    HKEY key;
    if (RegCreateKeyEx(root, subkey, 0, NULL, 0, KEY_SET_VALUE, NULL, &key, NULL) != ERROR_SUCCESS) return false;
    LONG rc = RegSetValueEx(key, name, 0, REG_SZ, (const BYTE*)(LPCTSTR)value,
                            (DWORD)((value.GetLength() + 1) * sizeof(TCHAR)));
    RegCloseKey(key);
    return rc == ERROR_SUCCESS;
}

CString GetRegString(HKEY root, LPCTSTR subkey, LPCTSTR name) {
    TCHAR buf[2048] = {0};
    DWORD size = sizeof(buf) - sizeof(TCHAR);
    if (RegGetValue(root, subkey, name, RRF_RT_REG_SZ, NULL, buf, &size) != ERROR_SUCCESS) return CString();
    return CString(buf);
}

CString ExePath() {
    TCHAR path[MAX_PATH * 2] = {0};
    GetModuleFileName(NULL, path, _countof(path));
    return CString(path);
}

}  // namespace

bool RegisterUrlScheme(const CString& exePath) {
    const HKEY root = HKEY_CURRENT_USER;
    bool ok = true;
    ok &= SetRegString(root, _T("Software\\Classes\\shawkycall"), NULL, CString(_T("URL:Shawky Call")));
    ok &= SetRegString(root, _T("Software\\Classes\\shawkycall"), _T("URL Protocol"), CString());
    CString icon;
    icon.Format(_T("\"%s\",0"), (LPCTSTR)exePath);
    ok &= SetRegString(root, _T("Software\\Classes\\shawkycall\\DefaultIcon"), NULL, icon);
    ok &= SetRegString(root, _T("Software\\Classes\\shawkycall\\shell\\open\\command"), NULL, ExpectedCommand(exePath));
    return ok;
}

void OnStartup() {
    g_startTick = GetTickCount();
    CString exe = ExePath();
    CString current = GetRegString(HKEY_CURRENT_USER, _T("Software\\Classes\\shawkycall\\shell\\open\\command"), NULL);
    bool hasProtocol = RegGetValue(HKEY_CURRENT_USER, _T("Software\\Classes\\shawkycall"), _T("URL Protocol"),
                                   RRF_RT_REG_SZ, NULL, NULL, NULL) == ERROR_SUCCESS;
    if (current.CompareNoCase(ExpectedCommand(exe)) != 0 || !hasProtocol) {
        RegisterUrlScheme(exe);
    }
}

bool IsRegisteredToPbx() {
    if (!is_pjsua_running()) return false;
    if (!pjsua_acc_is_valid(account)) return false;
    pjsua_acc_info info;
    if (pjsua_acc_get_info(account, &info) != PJ_SUCCESS) return false;
    // Same condition MicroSIP's status bar shows as "Online": last REGISTER
    // answered 2xx and a registration session exists. (expires is the time to
    // the next refresh and may briefly be 0, so it is not used.)
    return info.has_registration && info.status / 100 == 2 &&
           info.expires != PJSIP_EXPIRES_NOT_SPECIFIED;
}

bool TryHandleCommandLine(const CString& params) {
    std::string utf8 = ToUtf8(params);
    if (!IsShawkyUri(utf8)) return false;

    UriResult uri = ParseUri(utf8);
    if (!uri.ok()) {
        Log("rejected shawkycall URI: %s", UriErrorName(uri.error));
        QueueError(StringId::BadLink);
        return true;
    }
    std::vector<DialRule> rules;
    std::string prefix;
    LoadDialConfig(rules, prefix);
    DialResult dial = NormalizeNumber(uri.phone, rules, prefix);
    if (!dial.ok()) {
        Log("rejected phone number: %s", DialErrorName(dial.error));
        QueueError(StringId::InvalidNumber);
        return true;
    }
    // Other params (name, salesman, project, ...) are intentionally ignored
    // in this phase.
    g_pendingNumber = FromUtf8(dial.number);
    g_pendingSince = GetTickCount();
    g_pendingWait = (GetTickCount() - g_startTick < kColdStartWindowMs) ? kWaitColdStartMs : kWaitRunningMs;
    StartTimer();
    return true;
}

}  // namespace shawky
