/******************************************************************************
 *
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *****************************************************************************/

#include "WeakTypeCoercer.h"

#include <alljoyn/MsgArg.h>
#include <qcc/String.h>
#include <qcc/winrt/utility.h>
#include <alljoyn/Status.h>
#include <MsgArg.h>
#include <AllJoynException.h>
#include <sstream>

using namespace Windows::Foundation;
using namespace ajn;

namespace AllJoyn {

enum TypeConvertTo {
    CONVERT_TO_UINT64 = 1,
    CONVERT_TO_INTEGER = 2,
    CONVERT_TO_DOUBLE = 3
};

bool IsHexNumber(const qcc::String& str)
{
    const char* data = str.c_str();
    bool ret = false;
    uint32 curPos = 0;
    if (data != nullptr && (data[0] == '-' || data[0] == '+')) {
        curPos++;
    }
    if ((str.compare(curPos, 2, "0x") == 0) || (str.compare(curPos, 2, "0X") == 0)) {
        curPos += 2;
        for (; curPos < str.size(); curPos++) {
            if ((data[curPos] >= '0' && data[curPos] <= '9') ||
                (data[curPos] >= 'a' && data[curPos] <= 'f') ||
                (data[curPos] >= 'A' && data[curPos] <= 'F')) {
                continue;
            }
            return false;
        }
        ret = true;
    }
    return ret;
}

bool IsOctNumber(const qcc::String& str)
{
    const char* data = str.c_str();
    bool ret = false;
    uint32 curPos = 0;
    if (data != nullptr && (data[0] == '-' || data[0] == '+')) {
        curPos++;
    }
    if (data[curPos] == '0') {
        curPos++;
        for (; curPos < str.size(); curPos++) {
            if (data[curPos] >= '0' && data[curPos] <= '7') {
                continue;
            }
            return false;
        }
        ret = true;
    }
    return ret;
}

// ([+-])?([0-9]*)(.)?([0-9]*)((e)([+-])?([0-9]+))?

bool IsValidDecimalNumber(const qcc::String& str)
{
    const char* data = str.c_str();
    uint32 curPos = 0;
    uint32 len = str.size();
    if (curPos < len && (data[curPos] == '+' || data[curPos] == '-')) {
        curPos++;
    }

    if ((curPos + 2 <= len)) {
        if (str.compare(curPos, 2, "0x") == 0 || str.compare(curPos, 2, "0X") == 0) {
            return false;
        }
    }

    while (curPos < len) {
        if (data[curPos] >= '0' && data[curPos] <= '9') {
            curPos++;
        } else if (data[curPos] == '.' || tolower(data[curPos]) == 'e') {
            break;
        } else {
            return false;
        }
    }

    if (curPos == len) {
        return true;
    }

    if (data[curPos] == '.') {
        curPos++;
        while (curPos < len) {
            if (data[curPos] >= '0' && data[curPos] <= '9') {
                curPos++;
            } else if (tolower(data[curPos]) == 'e') {
                break;
            } else {
                return false;
            }
        }
        if (curPos == len) {
            return true;
        }
    }

    if (tolower(data[curPos]) == 'e') {
        if (++curPos == len) {
            return false;
        }
        if (data[curPos] == '+' || data[curPos] == '-') {
            if (++curPos == len) {
                return false;
            }
        }
        while (curPos < len) {
            if (data[curPos] >= '0' && data[curPos] <= '9') {
                curPos++;
            } else {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool HasExponent(const qcc::String& str)
{
    const char* data = str.c_str();
    uint32 curPos = 0;
    uint32 len = str.size();
    for (; curPos < len; curPos++) {
        if (tolower(data[curPos]) == 'e') {
            return true;
        }
    }
    return false;
}

template <class T> T GetValue(IPropertyValue ^ prop, bool& success)
{
    T val = 0;
    success = false;
    if (nullptr == prop) {
        return val;
    }

    PropertyType type = prop->Type;
    success = true;
    if (type == Windows::Foundation::PropertyType::Double) {
        val = (T)prop->GetDouble();
    } else if (type == Windows::Foundation::PropertyType::Boolean) {
        val = (T)prop->GetBoolean();
    } else if (type == Windows::Foundation::PropertyType::UInt8) {
        val = (T)prop->GetUInt8();
    } else if (type == Windows::Foundation::PropertyType::UInt16) {
        val = (T)prop->GetUInt16();
    } else if (type == Windows::Foundation::PropertyType::Int16) {
        val = (T)prop->GetInt16();
    } else if (type == Windows::Foundation::PropertyType::UInt32) {
        val = (T)prop->GetUInt32();
    } else if (type == Windows::Foundation::PropertyType::Int32) {
        val = (T)prop->GetInt32();
    } else if (type == Windows::Foundation::PropertyType::UInt64) {
        val = (T)prop->GetUInt64();
    } else if (type == Windows::Foundation::PropertyType::Int64) {
        val = (T)prop->GetInt64();
    } else if (type == Windows::Foundation::PropertyType::String) {
        try {
            qcc::String str = PlatformToMultibyteString(prop->GetString());
            std::stringstream ss(str.c_str());
            if (IsHexNumber(str)) {
                uint64 tmpVal = 0;
                if (!(ss >> std::hex >> tmpVal)) {
                    success = false;
                }
                val = (T)tmpVal;
            } else if (IsOctNumber(str)) {
                uint64 tmpVal = 0;
                if (!(ss >> std::oct >> tmpVal)) {
                    success = false;
                }
                val = (T)tmpVal;
            } else if (IsValidDecimalNumber(str)) {
                if (HasExponent(str)) {
                    float64 tmpVal = 0;
                    if (!(ss >> tmpVal)) {
                        success = false;
                    }
                    val = (T)tmpVal;
                } else {
                    if (!(ss >> val)) {
                        success = false;
                    }
                }
            } else {
                success = false;
            }
        } catch (std::exception&) {
            success = false;
        }
    } else {
        success = false;
    }
    return val;
}

template <class T> T ToNumber(IPropertyValue ^ prop, bool& success, TypeConvertTo type, T min = 0, T max = 0)
{
    int64 tmpVal = 0;
    switch (type) {
    case CONVERT_TO_INTEGER:
        tmpVal = GetValue<int64>(prop, success);
        if (success) {
            if (tmpVal < min || tmpVal > max) {
                success = false;
            }
        }
        return (T)tmpVal;

    case CONVERT_TO_UINT64:
    case CONVERT_TO_DOUBLE:
        return GetValue<T>(prop, success);
    }
    success = false;
    return (T)0;
}

bool ToBoolean(IPropertyValue ^ prop, bool& success)
{
    bool val = false;
    success = false;
    if (nullptr == prop) {
        return val;
    }

    PropertyType type = prop->Type;
    if (type == Windows::Foundation::PropertyType::Boolean) {
        val = prop->GetBoolean();
        success = true;
    } else if (type == Windows::Foundation::PropertyType::String) {
        // "0" and case-insensitive "false" represent false
        qcc::String str = PlatformToMultibyteString(prop->GetString());
        qcc::String newStr;
        size_t pos = 0;
        for (; pos < str.size(); pos++) {
            newStr.push_back(tolower(str[pos]));
        }
        if (newStr.compare("true") == 0 || newStr.compare("1") == 0) {
            val = true;
            success = true;
        } else if (newStr.compare("false") == 0 || newStr.compare("0") == 0) {
            val = false;
            success = true;
        }
    } else {
        // if it is an integer, then 0 represents false and 1 represents true
        bool converted = false;
        uint8 v = GetValue<uint8>(prop, success);
        if (v == 0) {
            val = false;
            success = true;
        } else if (v == 1) {
            val = true;
            success = true;
        }
    }
    return val;
}

Platform::String ^ ToWideCharString(IPropertyValue ^ prop, bool &success)
{
    success = false;
    Platform::String ^ strObj = nullptr;
    if (nullptr == prop) {
        return strObj;
    }

    PropertyType type = prop->Type;
    success = true;
    if (type == Windows::Foundation::PropertyType::String) {
        return prop->GetString();
    }

    wchar_t wcstr[32];
    std::wstringstream wss;
    try {
        if (type == Windows::Foundation::PropertyType::Boolean) {
            wss << prop->GetBoolean();
        } else if (type == Windows::Foundation::PropertyType::Double) {
            wss << prop->GetDouble();
        } else if (type == Windows::Foundation::PropertyType::UInt8) {
            wss << prop->GetUInt8();
        } else if (type == Windows::Foundation::PropertyType::UInt16) {
            wss << prop->GetUInt16();
        } else if (type == Windows::Foundation::PropertyType::Int16) {
            wss << prop->GetInt16();
        } else if (type == Windows::Foundation::PropertyType::UInt32) {
            wss << prop->GetUInt32();
        } else if (type == Windows::Foundation::PropertyType::Int32) {
            wss << prop->GetInt32();
        } else if (type == Windows::Foundation::PropertyType::UInt64) {
            wss << prop->GetUInt64();
        } else if (type == Windows::Foundation::PropertyType::Int64) {
            wss << prop->GetInt64();
        } else {
            success = false;
        }
        if (success) {
            wss >> wcstr;
            strObj = ref new Platform::String(wcstr);
        }
    } catch (std::exception&) {
        success = false;
    }
    return strObj;
}

template <class T> Platform::Array<T> ^ ToArray(IPropertyValue ^ prop, bool &success, TypeConvertTo to, T min = 0, T max = 0)
{
    Platform::Array<T> ^ retObj = nullptr;
    Platform::Array<Platform::Object ^>^ objArray = nullptr;
    success = false;
    if (nullptr == prop) {
        return retObj;
    }
    prop->GetInspectableArray(&objArray);
    if (nullptr != objArray) {
        Platform::Array<T> ^ vals = ref new Platform::Array<T>(objArray->Length);
        uint32 index = 0;
        bool converted = false;
        for (; index < objArray->Length; index++) {
            IPropertyValue ^ element = dynamic_cast<IPropertyValue ^>(objArray[index]);
            converted = false;
            vals[index] = ToNumber<T>(element, converted, to, min, max);
            if (!converted) {
                break;
            }
        }
        if (converted || (objArray->Length == 0)) {
            success = true;
            retObj = vals;
        }
    }
    return retObj;
}

Platform::Array<Platform::Boolean> ^ ToBooleanArray(IPropertyValue ^ prop, bool &success)
{
    Platform::Array<Platform::Boolean> ^ retObj = nullptr;
    Platform::Array<Platform::Object ^> ^ objArray = nullptr;
    success = false;
    if (nullptr == prop) {
        return retObj;
    }

    prop->GetInspectableArray(&objArray);
    if (nullptr != objArray) {
        Platform::Array<Platform::Boolean> ^ vals = ref new Platform::Array<Platform::Boolean>(objArray->Length);
        uint32 index = 0;
        bool converted = false;
        for (; index < objArray->Length; index++) {
            IPropertyValue ^ element = dynamic_cast<IPropertyValue ^>(objArray[index]);
            converted = false;
            vals[index] = ToBoolean(element, converted);
            if (!converted  || (objArray->Length == 0)) {
                break;
            }
        }
        if (converted || (objArray->Length == 0)) {
            success = true;
            retObj = vals;
        }
    }
    return retObj;
}

Platform::Array<Platform::String ^> ^ ToWideCharStringArray(IPropertyValue ^ prop, bool &success)
{
    Platform::Array<Platform::String ^> ^ retObj = nullptr;
    Platform::Array<Platform::Object ^> ^ objArray = nullptr;
    success = false;
    if (nullptr == prop) {
        return retObj;
    }
    prop->GetInspectableArray(&objArray);
    if (nullptr != objArray) {
        Platform::Array<Platform::String ^> ^ vals = ref new Platform::Array<Platform::String ^>(objArray->Length);
        uint32 index = 0;
        bool converted = true;
        for (; index < objArray->Length; index++) {
            IPropertyValue ^ element = dynamic_cast<IPropertyValue ^>(objArray[index]);
            vals[index] = ToWideCharString(element, converted);
            if (!converted) {
                break;
            }
        }

        if (converted || (objArray->Length == 0)) {
            success = true;
            retObj = vals;
        }
    }
    return retObj;
}

Platform::Object ^ WeakTypeCoercer::Coerce(Platform::Object ^ obj, ajn::AllJoynTypeId typeId, bool inParam)
{
    Platform::Object ^ retObj = nullptr;
    if (obj == nullptr) {
        return retObj;
    }

    IPropertyValue ^ prop = dynamic_cast<IPropertyValue ^>(obj);
    PropertyType type = Windows::Foundation::PropertyType::Empty;
    if (nullptr != prop) {
        type = prop->Type;
    }

    switch (typeId) {
    case ALLJOYN_BOOLEAN:
        {
            bool success = false;
            bool val = ToBoolean(prop, success);
            if (success) {
                Platform::Object ^ o = val;
                retObj = o;
            }
        }
        break;

    case ALLJOYN_DOUBLE:
        {
            bool success = false;
            float64 val = ToNumber<float64>(prop, success, CONVERT_TO_DOUBLE);
            if (success) {
                Platform::Object ^ o = val;
                retObj = o;
            }
        }
        break;

    case ALLJOYN_VARIANT:
    case ALLJOYN_STRUCT:
    case ALLJOYN_DICT_ENTRY:
        {
            MsgArg ^ msgArg = dynamic_cast<MsgArg ^>(obj);
            if (msgArg != nullptr) {
                retObj = obj;
            }
        }
        break;

    case ALLJOYN_INT32:
        {
            if (type == Windows::Foundation::PropertyType::Int32) {
                retObj = obj;
            } else {
                bool success = false;
                int32 val = ToNumber<int32>(prop, success, CONVERT_TO_INTEGER, INT32_MIN, INT32_MAX);
                if (success) {
                    Platform::Object ^ o = val;
                    retObj = o;
                }
            }
        }
        break;

    case ALLJOYN_STRING:
        {
            bool success = false;
            Platform::String ^ o = ToWideCharString(prop, success);
            if (success) {
                retObj = o;
            }
        }
        break;

    case ALLJOYN_INT64:
        {
            if (type == Windows::Foundation::PropertyType::Int64) {
                retObj = obj;
            } else {
                bool success = false;
                int64 val = ToNumber<int64>(prop, success, CONVERT_TO_INTEGER, INT64_MIN, INT64_MAX);
                if (success) {
                    Platform::Object ^ o = val;
                    retObj = o;
                }
            }
        }
        break;

    case ALLJOYN_BYTE:
        {
            if (type == Windows::Foundation::PropertyType::UInt8) {
                retObj = obj;
            } else {
                bool success = false;
                uint8 val = ToNumber<uint8>(prop, success, CONVERT_TO_INTEGER, 0, UINT8_MAX);
                if (success) {
                    Platform::Object ^ o = val;
                    retObj = o;
                }
            }
        }
        break;

    case ALLJOYN_UINT32:
        {
            if (type == Windows::Foundation::PropertyType::UInt32) {
                retObj = obj;
            } else {
                bool success = false;
                uint32 val = ToNumber<uint32>(prop, success, CONVERT_TO_INTEGER, 0, UINT32_MAX);
                if (success) {
                    Platform::Object ^ o = val;
                    retObj = o;
                }
            }
        }
        break;

    case ALLJOYN_UINT64:
        {
            if (type == Windows::Foundation::PropertyType::UInt64) {
                retObj = obj;
            } else {
                bool success = false;
                uint64 val = ToNumber<uint64>(prop, success, CONVERT_TO_UINT64);
                if (success) {
                    Platform::Object ^ o = val;
                    retObj = o;
                }
            }
        }
        break;

    case ALLJOYN_OBJECT_PATH:
        {
            if (type == Windows::Foundation::PropertyType::String) {
                retObj = obj;
            }
        }
        break;

    case ALLJOYN_SIGNATURE:
        {
            if (type == Windows::Foundation::PropertyType::String) {
                retObj = obj;
            }
        }
        break;

    case ALLJOYN_HANDLE:
        {
            if (type == Windows::Foundation::PropertyType::UInt64) {
                retObj = obj;
            } else {
                bool success = false;
                uint64 val = ToNumber<uint64>(prop, success, CONVERT_TO_UINT64, 0, UINT64_MAX);
                if (success) {
                    Platform::Object ^ o = val;
                    retObj = o;
                }
            }
        }
        break;

    case ALLJOYN_UINT16:
        {
            if (type == Windows::Foundation::PropertyType::UInt16) {
                retObj = obj;
            } else {
                bool success = false;
                uint16 val = ToNumber<uint16>(prop, success, CONVERT_TO_INTEGER, 0, UINT16_MAX);
                if (success) {
                    Platform::Object ^ o = val;
                    retObj = o;
                }
            }
        }
        break;

    case ALLJOYN_INT16:
        {
            if (type == Windows::Foundation::PropertyType::Int16) {
                retObj = obj;
            } else {
                bool success = false;
                int16 val = ToNumber<int16>(prop, success, CONVERT_TO_INTEGER, INT16_MIN, INT16_MAX);
                if (success) {
                    Platform::Object ^ o = val;
                    retObj = o;
                }
            }
        }
        break;

    case ALLJOYN_ARRAY:
        {
            if (type == Windows::Foundation::PropertyType::InspectableArray) {
                retObj = obj;
            }
        }
        break;

    case ALLJOYN_BOOLEAN_ARRAY:
        {
            if (type == Windows::Foundation::PropertyType::BooleanArray) {
                retObj = obj;
            } else if (type == Windows::Foundation::PropertyType::InspectableArray) {
                bool success = false;
                Platform::Array<Platform::Boolean> ^ boolArr = ToBooleanArray(prop, success);
                if (success) {
                    retObj = boolArr;
                }
            }
        }
        break;

    case ALLJOYN_DOUBLE_ARRAY:
        {
            if (type == Windows::Foundation::PropertyType::DoubleArray) {
                retObj = obj;
            } else if (type == Windows::Foundation::PropertyType::InspectableArray) {
                bool success = false;
                Platform::Array<float64> ^ doubleArr = ToArray<float64>(prop, success, CONVERT_TO_DOUBLE);
                if (success) {
                    retObj = doubleArr;
                }
            }
        }
        break;

    case ALLJOYN_INT32_ARRAY:
        {
            if (type == Windows::Foundation::PropertyType::Int32Array) {
                retObj = obj;
            } else if (type == Windows::Foundation::PropertyType::InspectableArray) {
                bool success = false;
                Platform::Array<int32> ^ int32Arr = ToArray<int32>(prop, success, CONVERT_TO_INTEGER, INT32_MIN, INT32_MAX);
                if (success) {
                    retObj = int32Arr;
                }
            }
        }
        break;

    case ALLJOYN_INT16_ARRAY:
        {
            if (type == Windows::Foundation::PropertyType::Int16Array) {
                retObj = obj;
            } else if (type == Windows::Foundation::PropertyType::InspectableArray) {
                bool success = false;
                Platform::Array<int16> ^ int16Arr = ToArray<int16>(prop, success, CONVERT_TO_INTEGER, INT16_MIN, INT16_MAX);
                if (success) {
                    retObj = int16Arr;
                }
            }
        }
        break;

    case ALLJOYN_UINT16_ARRAY:
        {
            if (type == Windows::Foundation::PropertyType::UInt16Array) {
                retObj = obj;
            } else if (type == Windows::Foundation::PropertyType::InspectableArray) {
                bool success = false;
                Platform::Array<uint16> ^ uint16Arr = ToArray<uint16>(prop, success, CONVERT_TO_INTEGER, 0, UINT16_MAX);
                if (success) {
                    retObj = uint16Arr;
                }
            }
        }
        break;

    case ALLJOYN_UINT64_ARRAY:
        {
            if (type == Windows::Foundation::PropertyType::UInt64Array) {
                retObj = obj;
            } else if (type == Windows::Foundation::PropertyType::InspectableArray) {
                bool success = false;
                Platform::Array<uint64> ^ uint64Arr = ToArray<uint64>(prop, success, CONVERT_TO_UINT64);
                if (success) {
                    retObj = uint64Arr;
                }
            }
        }
        break;

    case ALLJOYN_UINT32_ARRAY:
        {
            if (type == Windows::Foundation::PropertyType::UInt32Array) {
                retObj = obj;
            } else if (type == Windows::Foundation::PropertyType::InspectableArray) {
                bool success = false;
                Platform::Array<uint32> ^ uint32Arr = ToArray<uint32>(prop, success, CONVERT_TO_INTEGER, 0, UINT32_MAX);
                if (success) {
                    retObj = uint32Arr;
                }
            }
        }
        break;

    case ALLJOYN_INT64_ARRAY:
        {
            if (type == Windows::Foundation::PropertyType::Int64Array) {
                retObj = obj;
            } else if (type == Windows::Foundation::PropertyType::InspectableArray) {
                bool success = false;
                Platform::Array<int64> ^ int64Arr = ToArray<int64>(prop, success, CONVERT_TO_INTEGER, INT64_MIN, INT64_MAX);
                if (success) {
                    retObj = int64Arr;
                }
            }
        }
        break;

    case ALLJOYN_BYTE_ARRAY:
        {
            if (type == Windows::Foundation::PropertyType::UInt8Array) {
                retObj = obj;
            } else if (type == Windows::Foundation::PropertyType::InspectableArray) {
                bool success = false;
                Platform::Array<uint8> ^ uint8Arr = ToArray<uint8>(prop, success, CONVERT_TO_INTEGER, 0, UINT8_MAX);
                if (success) {
                    retObj = uint8Arr;
                }
            }
        }
        break;

    case 'sa':
    case 'oa':
    case 'ga':
        {
            if (type == Windows::Foundation::PropertyType::StringArray) {
                retObj = obj;
            } else if (type == Windows::Foundation::PropertyType::InspectableArray) {
                bool success = false;
                Platform::Array<Platform::String ^> ^ strArr = ToWideCharStringArray(prop, success);
                if (success) {
                    retObj = strArr;
                }
            }
        }
        break;

    default:
        break;
    }

    return retObj;
}

}
