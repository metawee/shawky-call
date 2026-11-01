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

#include "ShawkyUri.h"

#include <cstring>

namespace shawky {

const char* const kDefaultDialRules = "+20>0;0020>0;20>0:11";

namespace {

const char kScheme[] = "shawkycall:";

char AsciiLower(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c; }

std::string Lower(const std::string& s) {
    std::string r(s);
    for (auto& c : r) c = AsciiLower(c);
    return r;
}

bool IsDigit(char c) { return c >= '0' && c <= '9'; }

bool AllDigits(const std::string& s) {
    for (char c : s)
        if (!IsDigit(c)) return false;
    return true;
}

std::string Trim(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r' || s[b] == '\n')) b++;
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r' || s[e - 1] == '\n')) e--;
    return s.substr(b, e - b);
}

std::string StripQuotes(const std::string& s) {
    std::string t = Trim(s);
    if (t.size() >= 2 && t.front() == '"' && t.back() == '"') t = t.substr(1, t.size() - 2);
    return t;
}

int HexVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool HasControlChars(const std::string& s) {
    for (unsigned char c : s)
        if (c < 0x20 || c == 0x7F) return true;
    // C1 controls U+0080..U+009F are encoded as C2 80..C2 9F.
    for (size_t i = 0; i + 1 < s.size(); i++)
        if ((unsigned char)s[i] == 0xC2 && (unsigned char)s[i + 1] >= 0x80 && (unsigned char)s[i + 1] <= 0x9F)
            return true;
    return false;
}

}  // namespace

const char* UriErrorName(UriError e) {
    switch (e) {
    case UriError::None: return "none";
    case UriError::Empty: return "empty";
    case UriError::TooLong: return "too_long";
    case UriError::BadScheme: return "bad_scheme";
    case UriError::UnknownAction: return "unknown_action";
    case UriError::BadPath: return "bad_path";
    case UriError::BadEncoding: return "bad_encoding";
    case UriError::BadUtf8: return "bad_utf8";
    case UriError::ControlChar: return "control_char";
    case UriError::ParamTooLong: return "param_too_long";
    case UriError::DuplicatePhone: return "duplicate_phone";
    case UriError::MissingPhone: return "missing_phone";
    }
    return "unknown";
}

const char* DialErrorName(DialError e) {
    switch (e) {
    case DialError::None: return "none";
    case DialError::Empty: return "empty";
    case DialError::InvalidChars: return "invalid_chars";
    case DialError::TooShort: return "too_short";
    case DialError::TooLong: return "too_long";
    case DialError::BadPrefix: return "bad_prefix";
    }
    return "unknown";
}

std::string UriResult::Get(const std::string& key) const {
    for (const auto& p : params)
        if (p.first == key) return p.second;
    return std::string();
}

bool IsShawkyUri(const std::string& s) {
    std::string t = StripQuotes(s);
    const size_t n = sizeof(kScheme) - 1;
    return t.size() >= n && Lower(t.substr(0, n)) == kScheme;
}

bool PercentDecode(const std::string& in, std::string& out) {
    out.clear();
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); i++) {
        char c = in[i];
        if (c == '%') {
            if (i + 2 >= in.size()) return false;
            int h = HexVal(in[i + 1]), l = HexVal(in[i + 2]);
            if (h < 0 || l < 0) return false;
            out.push_back((char)(h * 16 + l));
            i += 2;
        } else {
            out.push_back(c);
        }
    }
    return true;
}

bool IsValidUtf8(const std::string& s) {
    const unsigned char* p = (const unsigned char*)s.data();
    size_t n = s.size(), i = 0;
    while (i < n) {
        unsigned char c = p[i];
        size_t len;
        unsigned int cp;
        if (c < 0x80) { i++; continue; }
        else if ((c & 0xE0) == 0xC0) { len = 2; cp = c & 0x1F; }
        else if ((c & 0xF0) == 0xE0) { len = 3; cp = c & 0x0F; }
        else if ((c & 0xF8) == 0xF0) { len = 4; cp = c & 0x07; }
        else return false;
        if (i + len > n) return false;
        for (size_t k = 1; k < len; k++) {
            if ((p[i + k] & 0xC0) != 0x80) return false;
            cp = (cp << 6) | (p[i + k] & 0x3F);
        }
        // Overlong encodings, surrogates, out of range.
        if ((len == 2 && cp < 0x80) || (len == 3 && cp < 0x800) || (len == 4 && cp < 0x10000)) return false;
        if (cp >= 0xD800 && cp <= 0xDFFF) return false;
        if (cp > 0x10FFFF) return false;
        i += len;
    }
    return true;
}

UriResult ParseUri(const std::string& input) {
    UriResult r;
    std::string uri = StripQuotes(input);
    if (uri.empty()) { r.error = UriError::Empty; return r; }
    if (uri.size() > kMaxUriLength) { r.error = UriError::TooLong; return r; }
    if (!IsShawkyUri(uri)) { r.error = UriError::BadScheme; return r; }

    std::string rest = uri.substr(sizeof(kScheme) - 1);
    if (rest.compare(0, 2, "//") == 0) rest = rest.substr(2);

    // Drop fragment.
    size_t hash = rest.find('#');
    if (hash != std::string::npos) rest = rest.substr(0, hash);

    std::string head = rest, query;
    size_t q = rest.find('?');
    if (q != std::string::npos) {
        head = rest.substr(0, q);
        query = rest.substr(q + 1);
    }
    // Allow one trailing slash ("shawkycall://call/?...") that browsers may add.
    if (!head.empty() && head.back() == '/') head.pop_back();
    if (head.find('/') != std::string::npos) { r.error = UriError::BadPath; return r; }

    r.action = Lower(head);
    if (r.action != "call") { r.error = UriError::UnknownAction; return r; }

    size_t start = 0;
    bool havePhone = false;
    while (start <= query.size()) {
        size_t amp = query.find('&', start);
        if (amp == std::string::npos) amp = query.size();
        std::string part = query.substr(start, amp - start);
        start = amp + 1;
        if (part.empty()) {
            if (amp >= query.size()) break;
            continue;
        }
        std::string rawKey = part, rawVal;
        size_t eq = part.find('=');
        if (eq != std::string::npos) {
            rawKey = part.substr(0, eq);
            rawVal = part.substr(eq + 1);
        }
        std::string key, val;
        if (!PercentDecode(rawKey, key) || !PercentDecode(rawVal, val)) { r.error = UriError::BadEncoding; return r; }
        if (!IsValidUtf8(key) || !IsValidUtf8(val)) { r.error = UriError::BadUtf8; return r; }
        if (HasControlChars(key) || HasControlChars(val)) { r.error = UriError::ControlChar; return r; }
        if (key.size() > kMaxParamLength || val.size() > kMaxParamLength) { r.error = UriError::ParamTooLong; return r; }
        key = Lower(key);
        if (key == "phone") {
            if (havePhone) { r.error = UriError::DuplicatePhone; return r; }
            havePhone = true;
            r.phone = Trim(val);
        }
        r.params.emplace_back(key, val);
        if (amp >= query.size()) break;
    }
    if (!havePhone || r.phone.empty()) { r.error = UriError::MissingPhone; return r; }
    return r;
}

bool ParseDialRules(const std::string& spec, std::vector<DialRule>& rules, std::string* error) {
    rules.clear();
    size_t start = 0;
    std::string s = spec;
    while (start <= s.size()) {
        size_t semi = s.find(';', start);
        if (semi == std::string::npos) semi = s.size();
        std::string item = Trim(s.substr(start, semi - start));
        start = semi + 1;
        if (!item.empty()) {
            DialRule rule;
            size_t gt = item.find('>');
            if (gt == std::string::npos) {
                if (error) *error = "missing '>' in rule: " + item;
                return false;
            }
            rule.from = Trim(item.substr(0, gt));
            std::string rhs = Trim(item.substr(gt + 1));
            size_t colon = rhs.find(':');
            if (colon != std::string::npos) {
                std::string min = Trim(rhs.substr(colon + 1));
                rhs = Trim(rhs.substr(0, colon));
                if (min.empty() || min.size() > 2 || !AllDigits(min)) {
                    if (error) *error = "bad minimum length in rule: " + item;
                    return false;
                }
                rule.minDigits = (size_t)std::stoi(min);
            }
            rule.to = rhs;
            std::string fromDigits = rule.from;
            if (!fromDigits.empty() && fromDigits[0] == '+') fromDigits = fromDigits.substr(1);
            if (fromDigits.empty() || !AllDigits(fromDigits) || !AllDigits(rule.to)) {
                if (error) *error = "rule must be [+]digits>digits: " + item;
                return false;
            }
            rules.push_back(rule);
        }
        if (semi >= s.size()) break;
    }
    return true;
}

DialResult NormalizeNumber(const std::string& raw, const std::vector<DialRule>& rules,
                           const std::string& dialPrefix) {
    DialResult r;
    if (!AllDigits(dialPrefix)) { r.error = DialError::BadPrefix; return r; }

    // 1+2: strip formatting characters and map Arabic digits.
    std::string n;
    const unsigned char* p = (const unsigned char*)raw.data();
    size_t len = raw.size();
    for (size_t i = 0; i < len;) {
        unsigned char c = p[i];
        if (c == ' ' || c == '\t' || c == '-' || c == '.' || c == '(' || c == ')') { i++; continue; }
        if (c == 0xC2 && i + 1 < len && p[i + 1] == 0xA0) { i += 2; continue; }  // NBSP
        // U+0660..U+0669 Arabic-Indic digits: D9 A0..D9 A9
        if (c == 0xD9 && i + 1 < len && p[i + 1] >= 0xA0 && p[i + 1] <= 0xA9) {
            n.push_back((char)('0' + (p[i + 1] - 0xA0)));
            i += 2;
            continue;
        }
        // U+06F0..U+06F9 Extended Arabic-Indic digits: DB B0..DB B9
        if (c == 0xDB && i + 1 < len && p[i + 1] >= 0xB0 && p[i + 1] <= 0xB9) {
            n.push_back((char)('0' + (p[i + 1] - 0xB0)));
            i += 2;
            continue;
        }
        // U+200E/U+200F/U+202A..U+202E/U+2066..U+2069 bidi marks (E2 80 8E, E2 80 8F, E2 80 AA..AE, E2 81 A6..A9)
        if (c == 0xE2 && i + 2 < len &&
            ((p[i + 1] == 0x80 && (p[i + 2] == 0x8E || p[i + 2] == 0x8F || (p[i + 2] >= 0xAA && p[i + 2] <= 0xAE))) ||
             (p[i + 1] == 0x81 && p[i + 2] >= 0xA6 && p[i + 2] <= 0xA9))) {
            i += 3;
            continue;
        }
        n.push_back((char)c);
        i++;
    }
    if (n.empty()) { r.error = DialError::Empty; return r; }

    // 3: first matching rule.
    size_t digitCount = 0;
    for (char c : n)
        if (IsDigit(c)) digitCount++;
    for (const auto& rule : rules) {
        if (n.compare(0, rule.from.size(), rule.from) == 0 && digitCount >= rule.minDigits) {
            n = rule.to + n.substr(rule.from.size());
            break;
        }
    }

    // 4+5
    n = dialPrefix + n;
    if (!AllDigits(n)) { r.error = DialError::InvalidChars; return r; }
    if (n.size() > kMaxNumberLength) { r.error = DialError::TooLong; return r; }
    if (n.size() < kMinNumberLength) { r.error = DialError::TooShort; return r; }
    r.number = n;
    return r;
}

std::string MaskNumber(const std::string& number) {
    if (number.size() <= 3) return std::string(number.size(), '*');
    return std::string(number.size() - 3, '*') + number.substr(number.size() - 3);
}

}  // namespace shawky
