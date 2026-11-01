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

// Portable (no Windows/MFC) parsing of shawkycall:// URIs and dial-rule
// normalization. All strings are UTF-8. Unit-tested in tests/.

#pragma once

#include <string>
#include <utility>
#include <vector>

namespace shawky {

const size_t kMaxUriLength = 2048;
const size_t kMaxParamLength = 256;
const size_t kMaxNumberLength = 15;  // E.164 maximum
const size_t kMinNumberLength = 2;

// Default dial rules: +20 / 0020 / 20 (international Egypt) -> 0.
// "20" only applies to numbers with at least 11 digits, so short
// internal extensions such as "201" are never rewritten.
extern const char* const kDefaultDialRules;

enum class UriError {
    None = 0,
    Empty,
    TooLong,
    BadScheme,
    UnknownAction,
    BadPath,
    BadEncoding,     // invalid %XX sequence
    BadUtf8,         // decoded bytes are not valid UTF-8
    ControlChar,     // control characters in a decoded value
    ParamTooLong,
    DuplicatePhone,
    MissingPhone,
};

const char* UriErrorName(UriError e);

struct UriResult {
    UriError error = UriError::None;
    std::string action;                                       // "call"
    std::string phone;                                        // raw decoded phone (not normalized)
    std::vector<std::pair<std::string, std::string>> params;  // all decoded params, in order
    bool ok() const { return error == UriError::None; }
    // First value for key, or empty string.
    std::string Get(const std::string& key) const;
};

// True when s (after trimming spaces and one pair of surrounding quotes)
// starts with "shawkycall:" (case-insensitive).
bool IsShawkyUri(const std::string& s);

UriResult ParseUri(const std::string& uri);

// Percent-decoding (RFC 3986). '+' is kept as a literal '+', because the
// gateway sends phone=+20... unencoded.
bool PercentDecode(const std::string& in, std::string& out);
bool IsValidUtf8(const std::string& s);

// ---- Dial rules -----------------------------------------------------------

struct DialRule {
    std::string from;      // "+20", "0020", "20"
    std::string to;        // "0"
    size_t minDigits = 0;  // rule applies only if the number has >= minDigits digits
};

// Spec: "from>to[:minDigits];from>to..."; e.g. "+20>0;0020>0;20>0:11".
// Empty spec is valid (no rules).
bool ParseDialRules(const std::string& spec, std::vector<DialRule>& rules, std::string* error);

enum class DialError {
    None = 0,
    Empty,
    InvalidChars,  // something other than digits remains after normalization
    TooShort,
    TooLong,
    BadPrefix,
};

const char* DialErrorName(DialError e);

struct DialResult {
    DialError error = DialError::None;
    std::string number;  // digits only
    bool ok() const { return error == DialError::None; }
};

// 1. Strip formatting: spaces, tabs, '-', '.', '(', ')', NBSP.
// 2. Map Arabic-Indic / Extended Arabic-Indic digits to ASCII digits.
// 3. Apply the first matching rule.
// 4. Prepend dialPrefix (digits only, may be empty).
// 5. Result must be digits only, length kMinNumberLength..kMaxNumberLength.
DialResult NormalizeNumber(const std::string& raw, const std::vector<DialRule>& rules,
                           const std::string& dialPrefix);

// Mask for logs: keep the last 3 digits ("********825").
std::string MaskNumber(const std::string& number);

}  // namespace shawky
