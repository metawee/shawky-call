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

// UTF-8 with BOM; compiled with /utf-8.

#include "ShawkyStrings.h"

namespace shawky {

const wchar_t* GetString(StringId id) {
    switch (id) {
    case StringId::AppName:
        return L"Shawky Call";
    case StringId::NotRegistered:
        return L"لا يمكن إجراء المكالمة: البرنامج غير متصل بسنترال المكتب.\n"
               L"تأكد من توصيل الشبكة ثم حاول مرة أخرى، أو كلّم الدعم الفني.\n\n"
               L"Cannot place the call: not registered to the office PBX.";
    case StringId::InvalidNumber:
        return L"رقم التليفون غير صحيح، ولم يتم الاتصال.\n\n"
               L"Invalid phone number. The call was not placed.";
    case StringId::BadLink:
        return L"رابط الاتصال غير صحيح، ولم يتم الاتصال.\n\n"
               L"Invalid call link. The call was not placed.";
    case StringId::None:
        break;
    }
    return L"";
}

}  // namespace shawky
