/******************************************************************************
 *
 * Copyright (c) 2011-2013, AllSeen Alliance. All rights reserved.
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

#pragma once

#include <alljoyn/MsgArg.h>
#include <qcc/String.h>
#include <map>
#include <list>
#include <qcc/ManagedObj.h>
#include <qcc/Mutex.h>

namespace AllJoyn {

/// <summary>
/// Enumeration of the various message arg types.
/// </summary>
/// <remarks>
/// Most of these map directly to the values used in the
/// DBus wire protocol but some are specific to the AllJoyn implementation.
/// </remarks>
public enum class AllJoynTypeId {
    /// <summary> AllJoyn INVALID typeId</summary>
    ALLJOYN_INVALID          = ajn::ALLJOYN_INVALID,
    /// <summary> AllJoyn array container type</summary>
    ALLJOYN_ARRAY            = ajn::ALLJOYN_ARRAY,
    /// <summary> AllJoyn boolean basic type, @c 0 is @c FALSE and @c 1 is @c TRUE - Everything else is invalid</summary>
    ALLJOYN_BOOLEAN          = ajn::ALLJOYN_BOOLEAN,
    /// <summary> AllJoyn IEEE 754 double basic type</summary>
    ALLJOYN_DOUBLE           = ajn::ALLJOYN_DOUBLE,
    /// <summary> AllJoyn dictionary or map container type - an array of key-value pairs</summary>
    ALLJOYN_DICT_ENTRY       = ajn::ALLJOYN_DICT_ENTRY,
    /// <summary> AllJoyn signature basic type</summary>
    ALLJOYN_SIGNATURE        = ajn::ALLJOYN_SIGNATURE,
    /// <summary> AllJoyn socket handle basic type</summary>
    ALLJOYN_HANDLE           = ajn::ALLJOYN_HANDLE,
    /// <summary> AllJoyn 32-bit signed integer basic type</summary>
    ALLJOYN_INT32            = ajn::ALLJOYN_INT32,
    /// <summary> AllJoyn 16-bit signed integer basic type</summary>
    ALLJOYN_INT16            = ajn::ALLJOYN_INT16,
    /// <summary> AllJoyn Name of an AllJoyn object instance basic type</summary>
    ALLJOYN_OBJECT_PATH      = ajn::ALLJOYN_OBJECT_PATH,
    /// <summary> AllJoyn 16-bit unsigned integer basic type</summary>
    ALLJOYN_UINT16           = ajn::ALLJOYN_UINT16,
    /// <summary> AllJoyn struct container type</summary>
    ALLJOYN_STRUCT           = ajn::ALLJOYN_STRUCT,
    /// <summary> AllJoyn UTF-8 NULL terminated string basic type</summary>
    ALLJOYN_STRING           = ajn::ALLJOYN_STRING,
    /// <summary> AllJoyn 64-bit unsigned integer basic type</summary>
    ALLJOYN_UINT64           = ajn::ALLJOYN_UINT64,
    /// <summary> AllJoyn 32-bit unsigned integer basic type</summary>
    ALLJOYN_UINT32           = ajn::ALLJOYN_UINT32,
    /// <summary> AllJoyn variant container type</summary>
    ALLJOYN_VARIANT          = ajn::ALLJOYN_VARIANT,
    /// <summary> AllJoyn 64-bit signed integer basic type</summary>
    ALLJOYN_INT64            = ajn::ALLJOYN_INT64,
    /// <summary> AllJoyn 8-bit unsigned integer basic type</summary>
    ALLJOYN_BYTE             = ajn::ALLJOYN_BYTE,
    /// <summary> Never actually used as a typeId: specified as ALLJOYN_STRUCT</summary>
    ALLJOYN_STRUCT_OPEN      = ajn::ALLJOYN_STRUCT_OPEN,
    /// <summary> Never actually used as a typeId: specified as ALLJOYN_STRUCT</summary>
    ALLJOYN_STRUCT_CLOSE     = ajn::ALLJOYN_STRUCT_CLOSE,
    /// <summary> Never actually used as a typeId: specified as ALLJOYN_DICT_ENTRY</summary>
    ALLJOYN_DICT_ENTRY_OPEN  = ajn::ALLJOYN_DICT_ENTRY_OPEN,
    /// <summary> Never actually used as a typeId: specified as ALLJOYN_DICT_ENTRY</summary>
    ALLJOYN_DICT_ENTRY_CLOSE = ajn::ALLJOYN_DICT_ENTRY_CLOSE,
    /// <summary> AllJoyn array of booleans</summary>
    ALLJOYN_BOOLEAN_ARRAY    = ajn::ALLJOYN_BOOLEAN_ARRAY,
    /// <summary> AllJoyn array of IEEE 754 doubles</summary>
    ALLJOYN_DOUBLE_ARRAY     = ajn::ALLJOYN_DOUBLE_ARRAY,
    /// <summary> AllJoyn array of 32-bit signed integers</summary>
    ALLJOYN_INT32_ARRAY      = ajn::ALLJOYN_INT32_ARRAY,
    /// <summary> AllJoyn array of 16-bit signed integers</summary>
    ALLJOYN_INT16_ARRAY      = ajn::ALLJOYN_INT16_ARRAY,
    /// <summary> AllJoyn array of 16-bit unsigned integers</summary>
    ALLJOYN_UINT16_ARRAY     = ajn::ALLJOYN_UINT16_ARRAY,
    /// <summary> AllJoyn array of 64-bit unsigned integers</summary>
    ALLJOYN_UINT64_ARRAY     = ajn::ALLJOYN_UINT64_ARRAY,
    /// <summary> AllJoyn array of 32-bit unsigned integers</summary>
    ALLJOYN_UINT32_ARRAY     = ajn::ALLJOYN_UINT32_ARRAY,
    /// <summary> AllJoyn array of 64-bit signed integers</summary>
    ALLJOYN_INT64_ARRAY      = ajn::ALLJOYN_INT64_ARRAY,
    /// <summary> AllJoyn array of 8-bit unsigned integers</summary>
    ALLJOYN_BYTE_ARRAY       = ajn::ALLJOYN_BYTE_ARRAY,
    /// <summary> This never appears in a signature but is used for matching arbitrary message args</summary>
    ALLJOYN_WILDCARD         = ajn::ALLJOYN_WILDCARD
};

ref class __MsgArg {
  private:
    friend ref class MsgArg;
    friend class _MsgArg;
    __MsgArg();
    ~__MsgArg();

    property Object ^ Value;
    property Object ^ Key;
};

class _MsgArg : protected ajn::MsgArg {
  protected:
    friend class qcc::ManagedObj<_MsgArg>;
    friend ref class MsgArg;
    friend class _BusObject;
    friend ref class BusObject;
    friend ref class ProxyBusObject;
    _MsgArg();
    ~_MsgArg();

    ::QStatus BuildArray(ajn::MsgArg* arry, const qcc::String elemSig, const Platform::Array<Platform::Object ^> ^ args, int32_t& argIndex);
    ::QStatus VBuildArgs(const char*& signature, uint32_t sigLen, ajn::MsgArg* arg,  int32_t maxCompleteTypes, const Platform::Array<Platform::Object ^> ^ args, int32_t& argIndex, int32_t recursionLevel);
    void SetObject(AllJoyn::MsgArg ^ msgArg, bool isKey);

    __MsgArg ^ _eventsAndProperties;
    std::map<void*, void*> _refMap;
    std::list<qcc::String> _strRef;
    std::list<ajn::MsgArg*> _msgScratch;
};

/// <summary>
/// This class deals with bus message types and the operations on them
/// </summary>
public ref class MsgArg sealed {
  public:
    MsgArg();

    /// <summary>
    /// Constructor to build a message arg. If the constructor fails for any reason, it throws a COMException.
    /// </summary>
    /// <param name="signature">The signature for MsgArg value.</param>
    /// <param name="args">An array contains one or more values that correspond to the signature to initialize the MsgArg.</param>
    /// <remarks>
    /// <para> - <c>'a'</c> : The array length followed by:</para>
    /// <para>       - If the element type is a basic or string type, then an array of values of that type.</para>
    /// <para>       - If the element type is an ARRAY, STRUCT, DICT_ENTRY, or VARIANT,
    ///                then the element in args is an array of MsgArgs where each MsgArg has the signature specified by the element type.</para>
    /// <para>       - If the element type is specified using the wildcard character '*', the element in args is an
    ///                array of MsgArgs. The array element type is determined from the type of the
    ///                first MsgArg in the array, all the elements must have the same type.</para>
    /// <para> - <c>'b'</c> : A bool value </para>
    /// <para> - <c>'d'</c> : A double (64 bits)</para>
    /// <para> - <c>'g'</c> : A pointer to a NUL terminated string (pointer must remain valid for lifetime of the MsgArg)</para>
    /// <para> - <c>'h'</c> : A qcc::SocketFd</para>
    /// <para> - <c>'i'</c> : An int (32 bits)</para>
    /// <para> - <c>'n'</c> : An int (16 bits)</para>
    /// <para> - <c>'o'</c> : A pointer to a NUL terminated string (pointer must remain valid for lifetime of the MsgArg)</para>
    /// <para> - <c>'q'</c> : A uint (16 bits)</para>
    /// <para> - <c>'s'</c> : A pointer to a NUL terminated string (pointer must remain valid for lifetime of the MsgArg)</para>
    /// <para> - <c>'t'</c> : A uint (64 bits)</para>
    /// <para> - <c>'u'</c> : A uint (32 bits)</para>
    /// <para> - <c>'v'</c> : Not allowed, the actual type must be provided.</para>
    /// <para> - <c>'x'</c> : An int (64 bits)</para>
    /// <para> - <c>'y'</c> : A byte (8 bits)</para>
    /// <para> - <c>'('</c> and <c>')'</c> : The list of values that appear between the parentheses using the notation above</para>
    /// <para> - <c>'{'</c> and <c>'}'</c> : A pair values using the notation above.</para>
    /// <para> - <c>'*'</c> : A MsgArg object.</para>
    /// </remarks>
    /// <exception cref="Platform::COMException">
    /// HRESULT will contain the AllJoyn error status code for the error.
    /// <para>   - #ER_OK if the MsgArg was successfully set</para>
    /// <para>   - An error status otherwise</para>
    /// </exception>
    MsgArg(Platform::String ^ signature, const Platform::Array<Platform::Object ^> ^ args);

    /// <summary>
    /// Get the value in the MsgArg object
    /// </summary>
    /// <remarks>
    ///
    /// </remarks>
    property Object ^ Value
    {
        Platform::Object ^ get();
    }

    /// <summary>
    /// Get the key in the MsgArg object
    /// </summary>
    /// <remarks>
    /// Used for ALLJOYN_DICT_ENTRY type.
    /// </remarks>
    property Object ^ Key
    {
        Platform::Object ^ get();
    }

    /// <summary>
    /// Set data type coercion mode when creating MsgArg objects.
    /// <param name="mode"> The coercion mode. If the value is "strict", then AllJoyn will do strict data type checking;
    /// if it is "weak", AllJoyn will map the weak data type to strict AllJoyn data type specified in the signature.
    /// The "weak" coercion is required for weakly typed language such as JavaScript. The "strict" is suggested for
    /// strong type languages such as C# and C++.</param>
    /// </summary>
    static void SetTypeCoercionMode(Platform::String ^ mode);

  private:
    friend class _MsgArg;
    friend ref class BusObject;
    friend class _BusObject;
    friend ref class ProxyBusObject;
    friend ref class Message;
    friend ref class MessageHeaderFields;
    friend class _ProxyBusObjectListener;
    MsgArg(const ajn::MsgArg * msgArg);
    MsgArg(const qcc::ManagedObj<_MsgArg>* msgArg);
    ~MsgArg();

    qcc::ManagedObj<_MsgArg>* _mMsgArg;
    _MsgArg* _msgArg;
};

}
// MsgArg.h
