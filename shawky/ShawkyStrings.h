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

// Strings added by Shawky Call. Arabic first, English second line, so the
// message is clear whatever UI language is active.

#pragma once

#include <wchar.h>

namespace shawky {

enum class StringId {
    None = 0,
    AppName,
    NotRegistered,
    InvalidNumber,
    BadLink,
};

const wchar_t* GetString(StringId id);

}  // namespace shawky
