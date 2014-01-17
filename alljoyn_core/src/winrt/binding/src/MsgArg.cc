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

#include "MsgArg.h"

#include <TypeCoercerFactory.h>
#include <StrictTypeCoercer.h>
#include <qcc/winrt/utility.h>
#include <qcc/Debug.h>
#include <SignatureUtils.h>
#include <ObjectReference.h>
#include <AllJoynException.h>

using namespace Windows::Foundation;
using namespace ajn;

#define QCC_MODULE "ALLJOYN"

#define ADD_SCRATCH(s) \
    { _msgScratch.push_back(s); \
    }

#define CLEAR_SCRATCH() \
    { std::list<ajn::MsgArg*>::const_iterator itr2; \
      for (itr2 = _msgScratch.begin(); itr2 != _msgScratch.end(); ++itr2) { \
          delete [] *itr2; \
      } \
      _msgScratch.clear(); \
    }

#define ADD_STRING_REF(s) \
    { _strRef.push_back(s); \
    }

#define CLEAR_STRING_REFS() \
    { _strRef.clear(); \
    }

namespace AllJoyn {

// Default type coercion is "strict"
ITypeCoercer* typeCoercer = TypeCoercerFactory::GetTypeCoercer("strict");

MsgArg::MsgArg()
{
    ::QStatus status = ER_OK;

    while (true) {

        // Create _MsgArg managed object
        _mMsgArg = new qcc::ManagedObj<_MsgArg>();
        // Check for allocation error
        if (NULL == _mMsgArg) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store pointer to _MsgArg for convenience
        _msgArg = &(**_mMsgArg);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

MsgArg::MsgArg(Platform::String ^ signature, const Platform::Array<Platform::Object ^> ^ args)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check args for invalid values
        if (nullptr == args) {
            status = ER_BAD_ARG_2;
            break;
        }
        // Create the managed version of MsgArg
        _mMsgArg = new qcc::ManagedObj<_MsgArg>();
        // Check for allocation error
        if (NULL == _mMsgArg) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store pointer to _MsgArg for convenience
        _msgArg = &(**_mMsgArg);
        // Convert signature to qcc::String
        qcc::String strSignature = PlatformToMultibyteString(signature);
        // Get the length of the signature
        size_t sigLen = (strSignature.c_str() ? strlen(strSignature.c_str()) : 0);
        // Check for invalid signature length
        if ((sigLen < 1) || (sigLen > 255)) {
            status = ER_BUS_BAD_SIGNATURE;
            break;
        } else {
            const char* signature = strSignature.c_str();
            int argIndex = 0;
            // Build up the MsgArg
            status = _msgArg->VBuildArgs(signature, sigLen, _msgArg, 1, args, argIndex, 0);
            break;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

MsgArg::MsgArg(const ajn::MsgArg* msgArg)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check msgArg for invalid values
        if (NULL == msgArg) {
            status = ER_BAD_ARG_1;
            break;
        }

        _mMsgArg = new qcc::ManagedObj<_MsgArg>();
        // Check for allocation error
        if (NULL == _mMsgArg) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        _msgArg = &(**_mMsgArg);
        ajn::MsgArg* destMsg = _msgArg;
        *destMsg = *msgArg;
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

MsgArg::MsgArg(const qcc::ManagedObj<_MsgArg>* msgArg)
{
    ::QStatus status = ER_OK;

    while (true) {
        // Check msgArg for invalid values
        if (NULL == msgArg) {
            status = ER_BAD_ARG_1;
            break;
        }
        // Attach msgArg to _MsgArg managed object
        _mMsgArg = new qcc::ManagedObj<_MsgArg>(*msgArg);
        // Check for allocation error
        if (NULL == _mMsgArg) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        // Store pointer to _MsgArg for convenience
        _msgArg = &(**_mMsgArg);
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

MsgArg::~MsgArg()
{
    // Delete _MsgArg managed object to adjust reference count
    if (NULL != _mMsgArg) {
        delete _mMsgArg;
        _mMsgArg = NULL;
        _msgArg = NULL;
    }
}

Platform::Object ^ MsgArg::Value::get()
{
    // Get Value
    Platform::Object ^ val = _msgArg->_eventsAndProperties->Value;
    // Check if value needs conversion
    if (nullptr == val) {
        // Convert wrapped value
        _msgArg->SetObject(this, false);
        // Store the result
        val = _msgArg->_eventsAndProperties->Value;
    }
    // Return Value
    return val;
}

Platform::Object ^ MsgArg::Key::get()
{
    // Get Key
    Platform::Object ^ key = _msgArg->_eventsAndProperties->Key;
    // Check if key needs conversion
    if (nullptr == key && _msgArg->typeId == ajn::AllJoynTypeId::ALLJOYN_DICT_ENTRY) {
        // Convert wrapped key
        _msgArg->SetObject(this, true);
        // Store the result
        key = _msgArg->_eventsAndProperties->Key;
    }
    // Return Key
    return key;
}

void MsgArg::SetTypeCoercionMode(Platform::String ^ mode)
{
    // Change the MsgArg type coercion factory
    typeCoercer = TypeCoercerFactory::GetTypeCoercer(mode);
}

_MsgArg::_MsgArg()
    : ajn::MsgArg()
{
    ::QStatus status = ER_OK;

    while (true) {
        // Create the private ref type
        _eventsAndProperties = ref new __MsgArg();
        // Check for allocation error
        if (nullptr == _eventsAndProperties) {
            status = ER_OUT_OF_MEMORY;
            break;
        }
        break;
    }

    // Bubble up any QStatus error as an exception
    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

_MsgArg::~_MsgArg()
{
    _eventsAndProperties = nullptr;
    // Clear the ref map
    ClearObjectMap(NULL, &(this->_refMap));
    // Clear the msgarg scratch
    CLEAR_SCRATCH();
    // Clear the string ref list
    CLEAR_STRING_REFS();
}

::QStatus _MsgArg::BuildArray(ajn::MsgArg* arry, const qcc::String elemSig, const Platform::Array<Platform::Object ^> ^ args, int32_t& argIndex)
{
    ::QStatus status = ER_OK;
    ajn::MsgArg* elements = NULL;

    switch (elemSig[0]) {
    case '*':
    case 'a':
    case 'v':
    case '(':
    case '{':
        {
            // Grab next object to convert
            Platform::Object ^ rawObj = args[argIndex++];
            // Check for valid object
            if (nullptr != rawObj) {
                // Convert object to alljoyn array
                Platform::Object ^ objVariantArray = typeCoercer->Coerce(rawObj, ajn::ALLJOYN_ARRAY, true);
                // Check for failed conversion
                if (nullptr == objVariantArray) {
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
                // Get box array from object
                Platform::IBoxArray<Platform::Object ^> ^ boxArray = dynamic_cast<Platform::IBoxArray<Platform::Object ^> ^>(objVariantArray);
                // Check for failed conversion
                if (nullptr == boxArray) {
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
                // Get array
                Platform::Array<Platform::Object ^> ^ objArray = boxArray->Value;
                // Check for failed get
                if (nullptr == objArray) {
                    status = ER_FAIL;
                    break;
                }
                // Get number of elements in the array
                size_t numElements = objArray->Length;
                if (numElements > 0) {
                    // Allocate an array for the msgargs
                    ajn::MsgArg* nativeArgs = new ajn::MsgArg[numElements];
                    // Check for failed allocation
                    if (NULL == nativeArgs) {
                        status = ER_OUT_OF_MEMORY;
                        break;
                    }
                    // Hold on to msgarg scratch to release later
                    ADD_SCRATCH(nativeArgs);
                    for (int i = 0; i < numElements; i++) {
                        // Convert each object in array to alljoyn variant
                        Platform::Object ^ obj = typeCoercer->Coerce(objArray[i], ajn::ALLJOYN_VARIANT, true);
                        // Check for failed conversion
                        if (nullptr == obj) {
                            status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                            break;
                        }
                        // Get the wrapped MsgArg from object
                        AllJoyn::MsgArg ^ msgarg = dynamic_cast<AllJoyn::MsgArg ^>(obj);
                        // Check for failed conversion
                        if (nullptr == msgarg) {
                            status = ER_FAIL;
                            break;
                        }
                        // Add a reference to the msgarg
                        AddObjectReference(NULL, msgarg, &(this->_refMap));
                        ajn::MsgArg* temp = msgarg->_msgArg;
                        // Deep copy the msgarg
                        nativeArgs[i] = *temp;
                    }
                    // Add a reference to this array
                    AddObjectReference(NULL, objArray, &(this->_refMap));
                    elements = nativeArgs;
                }
                qcc::String sig = qcc::String::Empty;
                if (numElements > 0) {
                    sig = elements[0].Signature();
                } else {
                    switch (elemSig[0]) {
                    case '(':
                        for (int i = 0; i < elemSig.size(); i++) {
                            sig.append(elemSig[i]);
                            if (elemSig[i] == ')') {
                                break;
                            }
                        }
                        break;

                    case '{':
                        for (int i = 0; i < elemSig.size(); i++) {
                            sig.append(elemSig[i]);
                            if (elemSig[i] == '}') {
                                break;
                            }
                        }
                        break;

                    default:
                        sig = elemSig[0];
                    }
                }
                // Check elements all have same type as the first element.
                for (size_t i = 1; i < numElements; i++) {
                    if (!elements[i].HasSignature(sig.c_str())) {
                        status = ER_BUS_BAD_VALUE;
                        QCC_LogError(status, ("Array element[%d] does not have expected signature \"%s\"", i, sig.c_str()));
                        break;
                    }
                }
                if (status == ER_OK) {
                    // Set the elements of the array
                    status = arry->v_array.SetElements(sig.c_str(), numElements, elements);
                }
            }
        }
        break;

    case 'h':
        {
            // Grab the next object to convert
            Platform::Object ^ rawObj = args[argIndex++];
            // Check for valid object
            if (nullptr != rawObj) {
                // Convert object to alljoyn uint64 array
                Platform::Object ^ obj = typeCoercer->Coerce(rawObj, ajn::ALLJOYN_UINT64_ARRAY, true);
                // Check for conversion failure
                if (nullptr == obj) {
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
                // Get box array from object
                Platform::IBoxArray<uint64> ^ boxArray = dynamic_cast<Platform::IBoxArray<uint64> ^>(obj);
                // Check for failed conversion
                if (nullptr == boxArray) {
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
                // Get array from boxed array
                Platform::Array<uint64> ^ sArray = boxArray->Value;
                // Check for unexpected value
                if (nullptr == sArray) {
                    status = ER_FAIL;
                    break;
                }
                // Get number of elements in array
                size_t numElements = sArray->Length;
                if (numElements >  0) {
                    // Allocate an array for the msgargs
                    ajn::MsgArg* nativeArgs = new ajn::MsgArg[numElements];
                    // Check for allocation error
                    if (NULL == nativeArgs) {
                        status = ER_OUT_OF_MEMORY;
                        break;
                    }
                    // Hold on to msgarg scratch to release later
                    ADD_SCRATCH(nativeArgs);
                    for (int i = 0; i < numElements; i++) {
                        // Get object from uint64 value
                        Platform::Object ^ o = PropertyValue::CreateUInt64(sArray[i]);
                        // Check for allocation error
                        if (nullptr == o) {
                            status = ER_OUT_OF_MEMORY;
                            break;
                        }
                        // Allocate object array
                        Platform::Array<Platform::Object ^> ^ objArr = ref new Platform::Array<Platform::Object ^>(1);
                        // Check for failed allocation
                        if (nullptr == objArr) {
                            status = ER_OUT_OF_MEMORY;
                            break;
                        }
                        // Pack object
                        objArr[0] = o;
                        // Create handle type MsgArg
                        AllJoyn::MsgArg ^ msgarg = ref new AllJoyn::MsgArg("h", objArr);
                        // Check for failed allocation
                        if (nullptr == msgarg) {
                            status = ER_OUT_OF_MEMORY;
                            break;
                        }
                        // Add reference to new msgarg
                        AddObjectReference(NULL, msgarg, &(this->_refMap));
                        ajn::MsgArg* temp = msgarg->_msgArg;
                        // Deep copy the msgarg
                        nativeArgs[i] = *temp;
                    }
                    elements = nativeArgs;
                }
                const qcc::String sig = elemSig[0];
                if (status == ER_OK) {
                    // Set the elements of the array
                    status = arry->v_array.SetElements(sig.c_str(), numElements, elements);
                }
            }
        }
        break;

    case 'o':
    case 's':
    case 'g':
        {
            // Grab the next object to convert
            Platform::Object ^ rawObj = args[argIndex++];
            // Check for valid object
            if (nullptr != rawObj) {
                // Convert object to alljoyn array type specified in signature
                Platform::Object ^ obj = typeCoercer->Coerce(rawObj, (ajn::AllJoynTypeId)((elemSig[0] << 8) | 'a'), true);
                // Check for failed conversion
                if (nullptr == obj) {
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
                // Get string boxed array
                Platform::IBoxArray<Platform::String ^> ^ boxArray = dynamic_cast<Platform::IBoxArray<Platform::String ^> ^>(obj);
                // Check for conversion failure
                if (nullptr == boxArray) {
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
                // Get array from boxed array
                Platform::Array<Platform::String ^> ^ sArray = boxArray->Value;
                // Check for unexpected value
                if (nullptr == sArray) {
                    status = ER_FAIL;
                    break;
                }
                // Get number of elements in array
                size_t numElements = sArray->Length;
                if (numElements > 0) {
                    // Allocate an array for the msgargs
                    ajn::MsgArg* nativeArgs = new ajn::MsgArg[numElements];
                    // Check for allocation error
                    if (NULL == nativeArgs) {
                        status = ER_OUT_OF_MEMORY;
                        break;
                    }
                    // Hold on to msgarg scratch to release later
                    ADD_SCRATCH(nativeArgs);
                    for (int i = 0; i < numElements; i++) {
                        // Allocate an object array
                        Platform::Array<Platform::Object ^> ^ objArr = ref new Platform::Array<Platform::Object ^>(1);
                        // Check for allocation error
                        if (nullptr == objArr) {
                            status = ER_OUT_OF_MEMORY;
                            break;
                        }
                        // Pack the string
                        objArr[0] = sArray[i];
                        // Create message from signature type
                        AllJoyn::MsgArg ^ msgarg = ref new AllJoyn::MsgArg((elemSig[0] == 's') ? "s" : (elemSig[0] == 'o') ? "o" : "g", objArr);
                        // Check for allocation error
                        if (nullptr == msgarg) {
                            status = ER_OUT_OF_MEMORY;
                            break;
                        }
                        // Add object reference to msgarg
                        AddObjectReference(NULL, msgarg, &(this->_refMap));
                        ajn::MsgArg* temp = msgarg->_msgArg;
                        // Deep copy the msgarg
                        nativeArgs[i] = *temp;
                    }
                    elements = nativeArgs;
                }
                const qcc::String sig = elemSig[0];
                if (status == ER_OK) {
                    // Set the elements of the array
                    status = arry->v_array.SetElements(sig.c_str(), numElements, elements);
                }
            }
        }
        break;

    case 'b':
        {
            // Grab the next object to convert
            Platform::Object ^ rawObj = args[argIndex++];
            // Check for valid object
            if (nullptr != rawObj) {
                // Convert object to alljoyn boolean array
                Platform::Object ^ obj = typeCoercer->Coerce(rawObj, ajn::ALLJOYN_BOOLEAN_ARRAY, true);
                // Check for failed conversion
                if (nullptr == obj) {
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
                // Get box array from object
                Platform::IBoxArray<Platform::Boolean> ^ boxArray = dynamic_cast<Platform::IBoxArray<Platform::Boolean> ^>(obj);
                // Check for conversion failure
                if (nullptr != boxArray) {
                    // Get the boolean array
                    Platform::Array<Platform::Boolean> ^ objArray = boxArray->Value;
                    // Check for invalid array length
                    if (nullptr == objArray) {
                        status = ER_FAIL;
                        break;
                    }
                    // Add a reference to the object array
                    AddObjectReference(NULL, objArray, &(this->_refMap));
                    // Fill in the rest of the array specifiers
                    arry->typeId = ALLJOYN_BOOLEAN_ARRAY;
                    arry->v_scalarArray.v_bool = objArray->Data;
                    arry->v_scalarArray.numElements = objArray->Length;
                } else {
                    // Invalid conversion
                    status = ER_FAIL;
                    break;
                }
            }
        }
        break;

    case 'd':
        {
            // Grab the next object to convert
            Platform::Object ^ rawObj = args[argIndex++];
            // Check for valid object
            if (nullptr != rawObj) {
                // Convert object to alljoyn double array
                Platform::Object ^ obj = typeCoercer->Coerce(rawObj, ajn::ALLJOYN_DOUBLE_ARRAY, true);
                // Check for failed conversion
                if (nullptr == obj) {
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
                // Get box array from object
                Platform::IBoxArray<float64> ^ boxArray = dynamic_cast<Platform::IBoxArray<float64> ^>(obj);
                // Check for conversion failure
                if (nullptr != boxArray) {
                    // Get the double array
                    Platform::Array<float64> ^ objArray = boxArray->Value;
                    // Check for invalid array length
                    if (nullptr == objArray) {
                        status = ER_FAIL;
                        break;
                    }
                    // Add a reference to the object array
                    AddObjectReference(NULL, objArray, &(this->_refMap));
                    // Fill in the rest of the array specifiers
                    arry->typeId = ALLJOYN_DOUBLE_ARRAY;
                    arry->v_scalarArray.v_double = objArray->Data;
                    arry->v_scalarArray.numElements = objArray->Length;
                } else {
                    // Invalid conversion
                    status = ER_FAIL;
                    break;
                }
            }
        }
        break;

    case 'i':
        {
            // Grab the next object to convert
            Platform::Object ^ rawObj = args[argIndex++];
            // Check for valid object
            if (nullptr != rawObj) {
                // Convert object to alljoyn int32 array
                Platform::Object ^ obj = typeCoercer->Coerce(rawObj, ajn::ALLJOYN_INT32_ARRAY, true);
                // Check for failed conversion
                if (nullptr == obj) {
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
                // Get box array from object
                Platform::IBoxArray<int32> ^ boxArray = dynamic_cast<Platform::IBoxArray<int32> ^>(obj);
                // Check for conversion failure
                if (nullptr != boxArray) {
                    // Get the int32 array
                    Platform::Array<int32> ^ objArray = boxArray->Value;
                    // Check for invalid array length
                    if (nullptr == objArray) {
                        status = ER_FAIL;
                        break;
                    }
                    // Add a reference to the object array
                    AddObjectReference(NULL, objArray, &(this->_refMap));
                    // Fill in the rest of the array specifiers
                    arry->typeId = ALLJOYN_INT32_ARRAY;
                    arry->v_scalarArray.v_int32 = objArray->Data;
                    arry->v_scalarArray.numElements = objArray->Length;
                } else {
                    // Invalid conversion
                    status = ER_FAIL;
                    break;
                }
            }
        }
        break;

    case 'n':
        {
            // Grab the next object to convert
            Platform::Object ^ rawObj = args[argIndex++];
            // Check for valid object
            if (nullptr != rawObj) {
                // Convert object to alljoyn int16 array
                Platform::Object ^ obj = typeCoercer->Coerce(rawObj, ajn::ALLJOYN_INT16_ARRAY, true);
                // Check for failed conversion
                if (nullptr == obj) {
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
                // Get box array from object
                Platform::IBoxArray<int16> ^ boxArray = dynamic_cast<Platform::IBoxArray<int16> ^>(obj);
                // Check for conversion failure
                if (nullptr != boxArray) {
                    // Get the int16 array
                    Platform::Array<int16> ^ objArray = boxArray->Value;
                    // Check for invalid array length
                    if (nullptr == objArray) {
                        status = ER_FAIL;
                        break;
                    }
                    // Add a reference to the object array
                    AddObjectReference(NULL, objArray, &(this->_refMap));
                    // Fill in the rest of the array specifiers
                    arry->typeId = ALLJOYN_INT16_ARRAY;
                    arry->v_scalarArray.v_int16 = objArray->Data;
                    arry->v_scalarArray.numElements = objArray->Length;
                } else {
                    // Invalid conversion
                    status = ER_FAIL;
                    break;
                }
            }
        }
        break;

    case 'q':
        {
            // Grab the next object to convert
            Platform::Object ^ rawObj = args[argIndex++];
            // Check for valid object
            if (nullptr != rawObj) {
                // Convert object to alljoyn uint16 array
                Platform::Object ^ obj = typeCoercer->Coerce(rawObj, ajn::ALLJOYN_UINT16_ARRAY, true);
                // Check for failed conversion
                if (nullptr == obj) {
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
                // Get box array from object
                Platform::IBoxArray<uint16> ^ boxArray = dynamic_cast<Platform::IBoxArray<uint16> ^>(obj);
                // Check for conversion failure
                if (nullptr != boxArray) {
                    // Get the uint16 array
                    Platform::Array<uint16> ^ objArray = boxArray->Value;
                    // Check for invalid array length
                    if (nullptr == objArray) {
                        status = ER_FAIL;
                        break;
                    }
                    // Add a reference to the object array
                    AddObjectReference(NULL, objArray, &(this->_refMap));
                    // Fill in the rest of the array specifiers
                    arry->typeId = ALLJOYN_UINT16_ARRAY;
                    arry->v_scalarArray.v_uint16 = objArray->Data;
                    arry->v_scalarArray.numElements = objArray->Length;
                } else {
                    // Invalid conversion
                    status = ER_FAIL;
                    break;
                }
            }
        }
        break;

    case 't':
        {
            // Grab the next object to convert
            Platform::Object ^ rawObj = args[argIndex++];
            // Check for valid object
            if (nullptr != rawObj) {
                // Convert object to alljoyn uint64 array
                Platform::Object ^ obj = typeCoercer->Coerce(rawObj, ajn::ALLJOYN_UINT64_ARRAY, true);
                // Check for failed conversion
                if (nullptr == obj) {
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
                // Get box array from object
                Platform::IBoxArray<uint64> ^ boxArray = dynamic_cast<Platform::IBoxArray<uint64> ^>(obj);
                // Check for conversion failure
                if (nullptr != boxArray) {
                    // Get uint64 array
                    Platform::Array<uint64> ^ objArray = boxArray->Value;
                    // Check for invalid array length
                    if (nullptr == objArray) {
                        status = ER_FAIL;
                        break;
                    }
                    // Add a reference to the object array
                    AddObjectReference(NULL, objArray, &(this->_refMap));
                    // Fill in the rest of the array specifiers
                    arry->typeId = ALLJOYN_UINT64_ARRAY;
                    arry->v_scalarArray.v_uint64 = objArray->Data;
                    arry->v_scalarArray.numElements = objArray->Length;
                } else {
                    // Invalid conversion
                    status = ER_FAIL;
                    break;
                }
            }
        }
        break;

    case 'u':
        {
            // Grab the next object to convert
            Platform::Object ^ rawObj = args[argIndex++];
            // Check for valid object
            if (nullptr != rawObj) {
                // Convert object to alljoyn uint32 array
                Platform::Object ^ obj = typeCoercer->Coerce(rawObj, ajn::ALLJOYN_UINT32_ARRAY, true);
                // Check for failed conversion
                if (nullptr == obj) {
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
                // Get box array from object
                Platform::IBoxArray<uint32> ^ boxArray = dynamic_cast<Platform::IBoxArray<uint32> ^>(obj);
                // Check for conversion failure
                if (nullptr != boxArray) {
                    // Get uint32 array
                    Platform::Array<uint32> ^ objArray = boxArray->Value;
                    // Check for invalid array length
                    if (nullptr == objArray) {
                        status = ER_FAIL;
                        break;
                    }
                    // Add a reference to the object array
                    AddObjectReference(NULL, objArray, &(this->_refMap));
                    // Fill in the rest of the array specifiers
                    arry->typeId = ALLJOYN_UINT32_ARRAY;
                    arry->v_scalarArray.v_uint32 = objArray->Data;
                    arry->v_scalarArray.numElements = objArray->Length;
                } else {
                    // Invalid conversion
                    status = ER_FAIL;
                    break;
                }
            }
        }
        break;

    case 'x':
        {
            // Grab the next object to convert
            Platform::Object ^ rawObj = args[argIndex++];
            // Check for valid object
            if (nullptr != rawObj) {
                // Convert object to int64 array
                Platform::Object ^ obj = typeCoercer->Coerce(rawObj, ajn::ALLJOYN_INT64_ARRAY, true);
                // Check for failed conversion
                if (nullptr == obj) {
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
                // Get box array from object
                Platform::IBoxArray<int64> ^ boxArray = dynamic_cast<Platform::IBoxArray<int64> ^>(obj);
                // Check for conversion failure
                if (nullptr != boxArray) {
                    // Get int64 array
                    Platform::Array<int64> ^ objArray = boxArray->Value;
                    // Check for invalid array length
                    if (nullptr == objArray) {
                        status = ER_FAIL;
                        break;
                    }
                    // Add a reference to the object array
                    AddObjectReference(NULL, objArray, &(this->_refMap));
                    // Fill in the rest of the array specifiers
                    arry->typeId = ALLJOYN_INT64_ARRAY;
                    arry->v_scalarArray.v_int64 = objArray->Data;
                    arry->v_scalarArray.numElements = objArray->Length;
                } else {
                    // Invalid conversion
                    status = ER_FAIL;
                    break;
                }
            }
        }
        break;

    case 'y':
        {
            // Grab the next object to convert
            Platform::Object ^ rawObj = args[argIndex++];
            // Check for valid object
            if (nullptr != rawObj) {
                // Convert object to alljoyn byte array
                Platform::Object ^ obj = typeCoercer->Coerce(rawObj, ajn::ALLJOYN_BYTE_ARRAY, true);
                // Check for failed conversion
                if (nullptr == obj) {
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
                // Get box array from object
                Platform::IBoxArray<uint8> ^ boxArray = dynamic_cast<Platform::IBoxArray<uint8> ^>(obj);
                // Check for conversion failure
                if (nullptr != boxArray) {
                    // Get the byte array
                    Platform::Array<uint8> ^ objArray = boxArray->Value;
                    // Check for invalid array length
                    if (nullptr == objArray) {
                        status = ER_FAIL;
                        break;
                    }
                    // Add a reference to the object array
                    AddObjectReference(NULL, objArray, &(this->_refMap));
                    // Fill in the rest of the array specifiers
                    arry->typeId = ALLJOYN_BYTE_ARRAY;
                    arry->v_scalarArray.v_byte = objArray->Data;
                    arry->v_scalarArray.numElements = objArray->Length;
                } else {
                    // Invalid conversion
                    status = ER_FAIL;
                    break;
                }
            }
        }
        break;

    default:
        status = ER_BUS_BAD_SIGNATURE;
        QCC_LogError(status, ("Invalid char '\\%d' in array element signature", elemSig[0]));
        break;
    }
    if (status != ER_OK) {
        arry->typeId = ALLJOYN_INVALID;
    }
    return status;
}

::QStatus _MsgArg::VBuildArgs(const char*& signature, uint32_t sigLen, ajn::MsgArg* arg, int32_t maxCompleteTypes, const Platform::Array<Platform::Object ^> ^ args, int32_t& argIndex, int32_t recursionLevel)
{
    ::QStatus status = ER_OK;
    size_t numArgs = 0;

    // Check for a balanced complete type
    while (sigLen-- && (argIndex < args->Length) && maxCompleteTypes--) {
        switch (*signature++) {
        case '*':
            {
                // Convert object to alljoyn variant
                Platform::Object ^ obj = typeCoercer->Coerce(args[argIndex++], ajn::ALLJOYN_VARIANT, true);
                // Check for conversion failure
                if (nullptr != obj) {
                    // Get MsgArg from object
                    AllJoyn::MsgArg ^ val = dynamic_cast<AllJoyn::MsgArg ^>(obj);
                    ajn::MsgArg* v = NULL;
                    // Check for conversion failure
                    if (nullptr != val && NULL != val->_msgArg) {
                        v = val->_msgArg;
                        // Add an object reference to object
                        AddObjectReference(NULL, obj, &(this->_refMap));
                    } else {
                        status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                        break;
                    }
                    if (v->typeId == ALLJOYN_ARRAY) {
                        // Set the elements of the array
                        status = arg->v_array.SetElements(v->v_array.GetElemSig(), v->v_array.GetNumElements(), (ajn::MsgArg*)v->v_array.GetElements());
                    } else {
                        // Deep copy the msgarg
                        *arg = *v;
                    }
                } else {
                    // Convert type to alljoyn array
                    Platform::Object ^ objVariantArray = typeCoercer->Coerce(args[argIndex - 1], ajn::ALLJOYN_ARRAY, true);
                    // Check for failed conversion
                    if (nullptr == objVariantArray) {
                        status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                        break;
                    }
                    --argIndex;
                    const char* elemSig = signature - 1;
                    arg->typeId = ALLJOYN_ARRAY;
                    size_t elemSigLen = signature - elemSig;
                    // Convert object to array
                    status = BuildArray(arg, qcc::String(elemSig, elemSigLen), args, argIndex);
                    sigLen -= (elemSigLen - 1);
                }
            }
            break;

        case 'a':
            {
                const char* elemSig = signature;
                // Set the MsgArg type
                arg->typeId = ALLJOYN_ARRAY;
                if (*elemSig == '*') {
                    ++signature;
                } else {
                    // Get signature for array
                    status = SignatureUtils::ParseContainerSignature(*arg, signature);
                }
                if (status == ER_OK) {
                    size_t elemSigLen = signature - elemSig;
                    // Build array with signature
                    status = BuildArray(arg, qcc::String(elemSig, elemSigLen), args, argIndex);
                    sigLen -= elemSigLen;
                } else {
                    status = ER_BUS_NOT_A_COMPLETE_TYPE;
                    QCC_LogError(status, ("Signature for array was not a complete type"));
                    arg->typeId = ALLJOYN_INVALID;
                }
            }
            break;

        case 'b':
            {
                // Convert current argument to alljoyn boolean
                Platform::Object ^ obj = typeCoercer->Coerce(args[argIndex++], ajn::ALLJOYN_BOOLEAN, true);
                // Get the box object
                Platform::IBox<Platform::Boolean> ^ t = dynamic_cast<Platform::IBox<Platform::Boolean> ^>(obj);
                // Check for conversion failure
                if (nullptr != t) {
                    // Get the boolean value
                    Platform::Boolean param = t->Value;
                    // Fill in the rest of the type specifiers
                    arg->typeId = ALLJOYN_BOOLEAN;
                    arg->v_bool = param;
                    // Add a reference to the object
                    AddObjectReference(NULL, obj, &(this->_refMap));
                } else {
                    // Invalid conversion
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
            }
            break;

        case 'd':
            {
                // Convert current argument to alljoyn double
                Platform::Object ^ obj = typeCoercer->Coerce(args[argIndex++], ajn::ALLJOYN_DOUBLE, true);
                // Get the box object
                Platform::IBox<float64> ^ t = dynamic_cast<Platform::IBox<float64> ^>(obj);
                // Check for conversion failure
                if (nullptr != t) {
                    // Get the double value
                    float64 param = t->Value;
                    // Fill in the rest of the type specifiers
                    arg->typeId = ALLJOYN_DOUBLE;
                    arg->v_double = param;
                    // Add a reference to the object
                    AddObjectReference(NULL, obj, &(this->_refMap));
                } else {
                    // Invalid conversion
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
            }
            break;

        case 'e':
            {
                // Check if a valid number of arguments exist for the dictionary entry
                if ((argIndex + 1) < args->Length) {
                    // Convert current arg to alljoyn variant
                    Platform::Object ^ objKey = typeCoercer->Coerce(args[argIndex++], ajn::ALLJOYN_VARIANT, true);
                    AllJoyn::MsgArg ^ key = nullptr;
                    if (nullptr != objKey) {
                        key = dynamic_cast<AllJoyn::MsgArg ^>(objKey);
                    }
                    ajn::MsgArg* k = NULL;
                    // Check for failed conversion
                    if (nullptr != key || NULL == key->_msgArg) {
                        // Store the unmanaged MsgArg
                        k = key->_msgArg;
                        // Add an object reference
                        AddObjectReference(NULL, objKey, &(this->_refMap));
                    } else {
                        // Invalid conversion
                        status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                        break;
                    }
                    // Convert current arg to alljoyn variant
                    Platform::Object ^ objVal = typeCoercer->Coerce(args[argIndex++], ajn::ALLJOYN_VARIANT, true);
                    AllJoyn::MsgArg ^ val = nullptr;
                    if (nullptr != objVal) {
                        val = dynamic_cast<AllJoyn::MsgArg ^>(objVal);
                    }
                    ajn::MsgArg* v = NULL;
                    // Check for failed conversion
                    if (nullptr != val && NULL != val->_msgArg) {
                        // Store the unmanated MsgArg
                        v = val->_msgArg;
                        // Add an object reference
                        AddObjectReference(NULL, objVal, &(this->_refMap));
                    } else {
                        // Invalid conversion
                        status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                        break;
                    }
                    // Fill in the rest of the type specifiers
                    arg->typeId = ALLJOYN_DICT_ENTRY;
                    arg->v_dictEntry.key = k;
                    arg->v_dictEntry.val = v;
                } else {
                    status = ER_BAD_ARG_COUNT;
                    break;
                }
            }
            break;

        case 'g':
            {
                // Get the current arg
                Platform::Object ^ rawObj = args[argIndex++];
                // Convert object to alljoyn signature
                Platform::Object ^ obj = typeCoercer->Coerce(rawObj, ajn::ALLJOYN_SIGNATURE, true);
                // Convert object to string
                Platform::String ^ t = dynamic_cast<Platform::String ^>(obj);
                // Check for failed conversion
                if (nullptr != obj || nullptr == rawObj) {
                    Platform::String ^ param = t;
                    // Convert arg to qcc::String
                    qcc::String strParam = PlatformToMultibyteString(param);
                    // Check for failed allocation
                    if (nullptr != param && strParam.empty()) {
                        status = ER_OUT_OF_MEMORY;
                        break;
                    }
                    // Add a string ref
                    ADD_STRING_REF(strParam);
                    // Check for invalid signature
                    if (!SignatureUtils::IsValidSignature(strParam.c_str())) {
                        status = ER_BUS_BAD_SIGNATURE;
                        QCC_LogError(status, ("String \"%s\" is not a legal signature", strParam.c_str()));
                        break;
                    }
                    // Fill in the rest of the type specifiers
                    arg->typeId = ALLJOYN_SIGNATURE;
                    arg->v_signature.sig = strParam.c_str();
                    arg->v_signature.len = strParam.length();
                    if (nullptr != obj) {
                        // Add an object reference
                        AddObjectReference(NULL, obj, &(this->_refMap));
                    }
                } else {
                    // Invalid conversion
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
            }
            break;

        case 'h':
            {
                // Convert current arg to alljoyn handle
                Platform::Object ^ obj = typeCoercer->Coerce(args[argIndex++], ajn::ALLJOYN_HANDLE, true);
                // Get the box object
                Platform::IBox<uint64> ^ t = dynamic_cast<Platform::IBox<uint64> ^>(obj);
                // Check for failed conversion
                if (nullptr != t) {
                    // Get the uint64
                    uint64 param = t->Value;
                    // Fill in the rest of the type specifiers
                    arg->typeId = ALLJOYN_HANDLE;
                    arg->v_handle.fd = (qcc::SocketFd)(void*)param;
                    // Add an object reference
                    AddObjectReference(NULL, obj, &(this->_refMap));
                } else {
                    // Invalid conversion
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
            }
            break;

        case 'i':
            {
                // Convert current arg to alljoyn int32
                Platform::Object ^ obj = typeCoercer->Coerce(args[argIndex++], ajn::ALLJOYN_INT32, true);
                // Get the box object
                Platform::IBox<int32> ^ t = dynamic_cast<Platform::IBox<int32> ^>(obj);
                // Check for failed conversion
                if (nullptr != t) {
                    // Get the int32
                    int32 param = t->Value;
                    // Fill in the rest of the type specifiers
                    arg->typeId = ALLJOYN_INT32;
                    arg->v_int32 = param;
                    // Add an object reference
                    AddObjectReference(NULL, obj, &(this->_refMap));
                } else {
                    // Invalid conversion
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
            }
            break;

        case 'n':
            {
                // Convert current arg to alljoyn int16
                Platform::Object ^ obj = typeCoercer->Coerce(args[argIndex++], ajn::ALLJOYN_INT16, true);
                // Get the box object
                Platform::IBox<int16> ^ t = dynamic_cast<Platform::IBox<int16> ^>(obj);
                // Check for failed conversion
                if (nullptr != t) {
                    // Get the int16
                    int16 param = t->Value;
                    // Fill in the rest of the type specifiers
                    arg->typeId = ALLJOYN_INT16;
                    arg->v_int16 = param;
                    // Add an object reference
                    AddObjectReference(NULL, obj, &(this->_refMap));
                } else {
                    // Invalid conversion
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
            }
            break;

        case 'o':
            {
                // Get the current arg object
                Platform::Object ^ rawObj = args[argIndex++];
                // Convert object to alljoyn object path
                Platform::Object ^ obj = typeCoercer->Coerce(rawObj, ajn::ALLJOYN_OBJECT_PATH, true);
                // Convert object to string
                Platform::String ^ t = dynamic_cast<Platform::String ^>(obj);
                // Check for failed conversion
                if (nullptr != obj || nullptr == rawObj) {
                    Platform::String ^ param = t;
                    // Convert arg to qcc::String
                    qcc::String strParam = PlatformToMultibyteString(param);
                    // Check for failed conversion
                    if (nullptr != param && strParam.empty()) {
                        status = ER_OUT_OF_MEMORY;
                        break;
                    }
                    // Add a string reference
                    ADD_STRING_REF(strParam);
                    // Fill in the rest of the type specifiers
                    arg->typeId = ALLJOYN_OBJECT_PATH;
                    arg->v_objPath.str = strParam.c_str();
                    arg->v_objPath.len = strParam.length();
                    if (nullptr != obj) {
                        // Add an object reference
                        AddObjectReference(NULL, obj, &(this->_refMap));
                    }
                } else {
                    // Invalid conversion
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
            }
            break;

        case 'q':
            {
                // Convert current arg to alljoyn uint16
                Platform::Object ^ obj = typeCoercer->Coerce(args[argIndex++], ajn::ALLJOYN_UINT16, true);
                // Get the box object
                Platform::IBox<uint16> ^ t = dynamic_cast<Platform::IBox<uint16> ^>(obj);
                // Check for failed conversion
                if (nullptr != t) {
                    // Get the uint16 value
                    uint16 param = t->Value;
                    // Fill in the rest of the type specifiers
                    arg->typeId = ALLJOYN_UINT16;
                    arg->v_uint16 = param;
                    // Add object reference
                    AddObjectReference(NULL, obj, &(this->_refMap));
                } else {
                    // Invalid conversion
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
            }
            break;

        case 'r':
            {
                // Convert the current arg to alljoyn array
                Platform::Object ^ objVariantArray = typeCoercer->Coerce(args[argIndex++], ajn::ALLJOYN_ARRAY, true);
                // Check for failed conversion
                if (nullptr == objVariantArray) {
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
                // Convert object to box array
                Platform::IBoxArray<Platform::Object ^> ^ boxArray = dynamic_cast<Platform::IBoxArray<Platform::Object ^> ^>(objVariantArray);
                // Check for failed conversion
                if (nullptr == boxArray) {
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
                // Get object array value from box array
                Platform::Array<Platform::Object ^> ^ objArray = boxArray->Value;
                // Check for invalid array length
                if (nullptr == objArray || objArray->Length < 1) {
                    status = ER_FAIL;
                    break;
                }
                // Allocate an array for the msgargs
                ajn::MsgArg* nativeArgs = new ajn::MsgArg[objArray->Length];
                // Check for allocation error
                if (NULL == nativeArgs) {
                    status = ER_OUT_OF_MEMORY;
                    break;
                }
                // Hold on to msgarg scratch to release later
                ADD_SCRATCH(nativeArgs);
                for (int i = 0; i < objArray->Length; i++) {
                    // Convert object to alljoyn variant
                    Platform::Object ^ obj = typeCoercer->Coerce(objArray[i], ajn::ALLJOYN_VARIANT, true);
                    // Check for conversion failure
                    if (nullptr == obj) {
                        status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                        break;
                    }
                    // Get msgarg from object
                    AllJoyn::MsgArg ^ msgarg = dynamic_cast<AllJoyn::MsgArg ^>(obj);
                    // Check for failed conversion
                    if (nullptr == msgarg) {
                        status = ER_FAIL;
                        break;
                    }
                    // Add an object reference to msgarg
                    AddObjectReference(NULL, msgarg, &(this->_refMap));
                    ajn::MsgArg* temp = msgarg->_msgArg;
                    // Deep copy the msgarg
                    nativeArgs[i] = *temp;
                }
                // Add an object reference to the array
                AddObjectReference(NULL, objArray, &(this->_refMap));
                // Fill in the rest of the type specifiers
                arg->typeId = ALLJOYN_STRUCT;
                arg->v_struct.numMembers = objArray->Length;
                arg->v_struct.members = nativeArgs;
            }
            break;

        case 's':
            {
                // Get the current arg
                Platform::Object ^ rawObj = args[argIndex++];
                // Convert the current object to alljoyn string
                Platform::Object ^ obj = typeCoercer->Coerce(rawObj, ajn::ALLJOYN_STRING, true);
                // Convert object to Platform::String
                Platform::String ^ t = dynamic_cast<Platform::String ^>(obj);
                // Check for failed conversion
                // In WinRT for an empty String "", obj returned by Coerce() is 0x0000000000000000 L
                if (nullptr != obj || nullptr == rawObj || ((nullptr == obj) && (nullptr != rawObj))) {
                    Platform::String ^ param = t;
                    // Convert arg to qcc::String
                    qcc::String strParam = PlatformToMultibyteString(param);
                    // Check for failed conversion
                    if (nullptr != param && strParam.empty()) {
                        status = ER_OUT_OF_MEMORY;
                        break;
                    }
                    // Add a string reference
                    ADD_STRING_REF(strParam);
                    // Fill in the rest of the type specifiers
                    arg->typeId = ALLJOYN_STRING;
                    arg->v_string.str = strParam.c_str();
                    arg->v_string.len = strParam.length();
                    if (nullptr != obj) {
                        // Add an object reference
                        AddObjectReference(NULL, obj, &(this->_refMap));
                    }
                } else {
                    // Invalid conversion
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
            }
            break;

        case 't':
            {
                // Convert current arg to alljoyn uint64
                Platform::Object ^ obj = typeCoercer->Coerce(args[argIndex++], ajn::ALLJOYN_UINT64, true);
                // Get the box object
                Platform::IBox<uint64> ^ t = dynamic_cast<Platform::IBox<uint64> ^>(obj);
                // Check for failed conversion
                if (nullptr != t) {
                    // Get the uint64 value
                    uint64 param = t->Value;
                    // Fill in the rest of the type specifiers
                    arg->typeId = ALLJOYN_UINT64;
                    arg->v_uint64 = param;
                    // Add an object reference
                    AddObjectReference(NULL, obj, &(this->_refMap));
                } else {
                    // Invalid conversion
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
            }
            break;

        case 'u':
            {
                // Convert current arg to alljoyn uint32
                Platform::Object ^ obj = typeCoercer->Coerce(args[argIndex++], ajn::ALLJOYN_UINT32, true);
                // Get the box object
                Platform::IBox<uint32> ^ t = dynamic_cast<Platform::IBox<uint32> ^>(obj);
                // Check for failed conversion
                if (nullptr != t) {
                    // Get the uint32 value
                    uint32 param = t->Value;
                    // Fill in the rest of the type specifiers
                    arg->typeId = ALLJOYN_UINT32;
                    arg->v_uint32 = param;
                    // Add an object reference
                    AddObjectReference(NULL, obj, &(this->_refMap));
                } else {
                    // Invalid conversion
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
            }
            break;

        case 'v':
            {
                // Convert current arg to alljoyn variant
                Platform::Object ^ obj = typeCoercer->Coerce(args[argIndex++], ajn::ALLJOYN_VARIANT, true);
                AllJoyn::MsgArg ^ val = nullptr;
                if (nullptr != obj) {
                    val = dynamic_cast<AllJoyn::MsgArg ^>(obj);
                }
                ajn::MsgArg* v = NULL;
                // Check for failed conversion
                if (nullptr != val && NULL != val->_msgArg) {
                    // Get unmanaged MsgArg
                    v = val->_msgArg;
                    // Add an object reference
                    AddObjectReference(NULL, obj, &(this->_refMap));
                } else {
                    // Invalid conversion
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
                // Fill in the rest of the type specifiers
                arg->typeId = ALLJOYN_VARIANT;
                arg->v_variant.val = v;
            }
            break;

        case 'x':
            {
                // Convert current arg to alljoyn int64
                Platform::Object ^ obj = typeCoercer->Coerce(args[argIndex++], ajn::ALLJOYN_INT64, true);
                // Get the box object
                Platform::IBox<int64> ^ t = dynamic_cast<Platform::IBox<int64> ^>(obj);
                // Check for failed conversion
                if (nullptr != t) {
                    // Get the in64 value
                    int64 param = t->Value;
                    // Fill in the rest of the type specifiers
                    arg->typeId = ALLJOYN_INT64;
                    arg->v_int64 = param;
                    // Add an object reference
                    AddObjectReference(NULL, obj, &(this->_refMap));
                } else {
                    // Invalid conversion
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
            }
            break;

        case 'y':
            {
                // Convert current arg to alljoyn byte
                Platform::Object ^ obj = typeCoercer->Coerce(args[argIndex++], ajn::ALLJOYN_BYTE, true);
                // Get the box object
                Platform::IBox<uint8> ^ t = dynamic_cast<Platform::IBox<uint8> ^>(obj);
                // Check for failed conversion
                if (nullptr != t) {
                    // Get the byte value
                    uint8 param = t->Value;
                    // Fill in the rest of the type specifiers
                    arg->typeId = ALLJOYN_BYTE;
                    arg->v_byte = param;
                    // Add an object reference
                    AddObjectReference(NULL, obj, &(this->_refMap));
                } else {
                    // Invalid conversion
                    status = (::QStatus)((int)ER_BAD_ARG_1 + argIndex - 1);
                    break;
                }
            }
            break;

        case '(':
            {
                const char* memberSig = signature;
                // Fill in the type specifier
                arg->typeId = ALLJOYN_STRUCT;
                // Parse container signature
                status = SignatureUtils::ParseContainerSignature(*arg, signature);
                if (status == ER_OK) {
                    // -1 to exclude the closing ')'
                    size_t memSigLen = signature - memberSig - 1;
                    arg->v_struct.members = new ajn::MsgArg[arg->v_struct.numMembers];
                    // Parse the container type
                    status = VBuildArgs(memberSig, memSigLen, arg->v_struct.members, arg->v_struct.numMembers, args, argIndex, recursionLevel + 1);
                    sigLen -= (memSigLen + 1);
                } else {
                    QCC_LogError(status, ("Signature for STRUCT was not a complete type"));
                    arg->typeId = ALLJOYN_INVALID;
                    status = ER_BUS_BAD_SIGNATURE;
                    break;
                }
            }
            break;

        case '{':
            {
                const char* memberSig = signature;
                // Fill in the type specifier
                arg->typeId = ALLJOYN_DICT_ENTRY;
                status = SignatureUtils::ParseContainerSignature(*arg, signature);
                if (status == ER_OK) {
                    // -1 to exclude the closing '}'
                    size_t memSigLen = signature - memberSig - 1;
                    arg->v_dictEntry.key = new ajn::MsgArg;
                    arg->v_dictEntry.val = new ajn::MsgArg;
                    // Parse the key type
                    status = VBuildArgs(memberSig, memSigLen, arg->v_dictEntry.key, 1, args, argIndex, recursionLevel + 1);
                    if (status != ER_OK) {
                        break;
                    }
                    // Parse the value type
                    status = VBuildArgs(memberSig, memSigLen, arg->v_dictEntry.val, 1, args, argIndex, recursionLevel + 1);
                    if (status != ER_OK) {
                        break;
                    }
                    sigLen -= (memSigLen + 1);
                } else {
                    QCC_LogError(status, ("Signature for DICT_ENTRY was not a complete type"));
                    arg->typeId = ALLJOYN_INVALID;
                    status = ER_BUS_BAD_SIGNATURE;
                    break;
                }
            }
            break;

        default:
            QCC_LogError(ER_BUS_BAD_SIGNATURE, ("Invalid char '\\%d' in signature", *(signature - 1)));
            // Fill in the type specifier
            arg->typeId = ALLJOYN_INVALID;
            status = ER_BUS_BAD_SIGNATURE;
            break;
        }

        if (status != ER_OK) {
            arg->Clear();
            break;
        }

        arg++;
    }

    // Check signature match at the top level of recursion when parsing is successful
    if (ER_OK == status && 0 == recursionLevel) {
        if (*signature != '\0') {
            // Signature didn't match what was parsed
            arg->Clear();
            status = ER_BUS_BAD_SIGNATURE;
        } else if (argIndex != args->Length) {
            // Number of arguments supplied didn't match signature
            arg->Clear();
            status = ER_BAD_ARG_COUNT;
        }
    }

    return status;
}

void _MsgArg::SetObject(AllJoyn::MsgArg ^ arg, bool isKey)
{
    ::QStatus status = ER_OK;

    switch (arg->_msgArg->typeId) {
    case ALLJOYN_BOOLEAN:
        {
            // Convert to alljoyn boolean
            Platform::Object ^ obj = PropertyValue::CreateBoolean(arg->_msgArg->v_bool);
            if (isKey) {
                // Store result as key
                arg->_msgArg->_eventsAndProperties->Key = typeCoercer->Coerce(obj, ajn::ALLJOYN_BOOLEAN, false);
            } else {
                // Store result as value
                arg->_msgArg->_eventsAndProperties->Value = typeCoercer->Coerce(obj, ajn::ALLJOYN_BOOLEAN, false);
            }
        }
        break;

    case ALLJOYN_DOUBLE:
        {
            // Convert to alljoyn double
            Platform::Object ^ obj = PropertyValue::CreateDouble(arg->_msgArg->v_double);
            if (isKey) {
                // Store result as key
                arg->_msgArg->_eventsAndProperties->Key = typeCoercer->Coerce(obj, ajn::ALLJOYN_DOUBLE, false);
            } else {
                // Store result as value
                arg->_msgArg->_eventsAndProperties->Value = typeCoercer->Coerce(obj, ajn::ALLJOYN_DOUBLE, false);
            }
        }
        break;

    case ALLJOYN_DICT_ENTRY:
        {
            // Convert to alljoyn dictionary entry
            AllJoyn::MsgArg ^ newKey = ref new AllJoyn::MsgArg(arg->_msgArg->v_dictEntry.key);
            AllJoyn::MsgArg ^ newValue = ref new AllJoyn::MsgArg(arg->_msgArg->v_dictEntry.val);
            if (isKey) {
                // Store result as key
                arg->_msgArg->_eventsAndProperties->Key = typeCoercer->Coerce(newKey->Value, arg->_msgArg->v_dictEntry.key->typeId, false);
            } else {
                // Store result as value
                arg->_msgArg->_eventsAndProperties->Value = typeCoercer->Coerce(newValue->Value, arg->_msgArg->v_dictEntry.val->typeId, false);
            }
        }
        break;

    case ALLJOYN_INT32:
        {
            // Convert to alljoyn int32
            Platform::Object ^ obj = PropertyValue::CreateInt32(arg->_msgArg->v_int32);
            if (isKey) {
                // Store result as key
                arg->_msgArg->_eventsAndProperties->Key = typeCoercer->Coerce(obj, ajn::ALLJOYN_INT32, false);
            } else {
                // Store result as value
                arg->_msgArg->_eventsAndProperties->Value = typeCoercer->Coerce(obj, ajn::ALLJOYN_INT32, false);
            }
        }
        break;

    case ALLJOYN_STRUCT:
        {
            // Convert to alljoyn struct
            ajn::MsgArg* elements = arg->_msgArg->v_struct.members;
            size_t elementCount = arg->_msgArg->v_struct.numMembers;
            // Allocate object array with count matching the number of struct elements
            Platform::Array<Platform::Object ^> ^ arr = ref new Platform::Array<Platform::Object ^>(elementCount);
            for (int i = 0; i < elementCount; i++) {
                // Convert element to wrapped MsgArg
                AllJoyn::MsgArg ^ newArg = ref new AllJoyn::MsgArg(&(elements[i]));
                // Add object reference
                AddObjectReference(NULL, newArg, &(this->_refMap));
                // Call recursive set to expand struct type
                SetObject(newArg, isKey);
                if (isKey) {
                    // Store result as key
                    arr[i] = newArg->_msgArg->_eventsAndProperties->Key;
                } else {
                    // Store result as value
                    arr[i] = newArg->_msgArg->_eventsAndProperties->Value;
                }
            }
            // Make array inspectable
            Platform::Object ^ obj = PropertyValue::CreateInspectableArray(arr);
            if (isKey) {
                // Store result as key
                arg->_msgArg->_eventsAndProperties->Key = typeCoercer->Coerce(obj, ajn::ALLJOYN_ARRAY, false);
            } else {
                // Store result as value
                arg->_msgArg->_eventsAndProperties->Value = typeCoercer->Coerce(obj, ajn::ALLJOYN_ARRAY, false);
            }
        }
        break;

    case ALLJOYN_STRING:
        {
            // Convert to alljoyn string
            qcc::String val = arg->_msgArg->v_string.str;
            if (isKey) {
                // Store result as key
                arg->_msgArg->_eventsAndProperties->Key = typeCoercer->Coerce(MultibyteToPlatformString(val.c_str()), ajn::ALLJOYN_STRING, false);
            } else {
                // Store result as value
                arg->_msgArg->_eventsAndProperties->Value = typeCoercer->Coerce(MultibyteToPlatformString(val.c_str()), ajn::ALLJOYN_STRING, false);
            }
        }
        break;

    case ALLJOYN_VARIANT:
        {
            // Convert to alljoyn variant
            AllJoyn::MsgArg ^ newArg = ref new AllJoyn::MsgArg(arg->_msgArg->v_variant.val);
            if (isKey) {
                // Store result as key
                arg->_msgArg->_eventsAndProperties->Key = typeCoercer->Coerce(newArg, ajn::ALLJOYN_VARIANT, false);
            } else {
                // Store result as value
                arg->_msgArg->_eventsAndProperties->Value = typeCoercer->Coerce(newArg, ajn::ALLJOYN_VARIANT, false);
            }
        }
        break;

    case ALLJOYN_INT64:
        {
            // Convert to alljoyn int64
            Platform::Object ^ obj = PropertyValue::CreateInt64(arg->_msgArg->v_int64);
            if (isKey) {
                // Store result as key
                arg->_msgArg->_eventsAndProperties->Key = typeCoercer->Coerce(obj, ajn::ALLJOYN_INT64, false);
            } else {
                // Store result as value
                arg->_msgArg->_eventsAndProperties->Value = typeCoercer->Coerce(obj, ajn::ALLJOYN_INT64, false);
            }
        }
        break;

    case ALLJOYN_BYTE:
        {
            // Convert to alljoyn byte
            Platform::Object ^ obj = PropertyValue::CreateUInt8(arg->_msgArg->v_byte);
            if (isKey) {
                // Store result as key
                arg->_msgArg->_eventsAndProperties->Key = typeCoercer->Coerce(obj, ajn::ALLJOYN_BYTE, false);
            } else {
                // Store result as value
                arg->_msgArg->_eventsAndProperties->Value = typeCoercer->Coerce(obj, ajn::ALLJOYN_BYTE, false);
            }
        }
        break;

    case ALLJOYN_UINT32:
        {
            // Convert to alljoyn uint32
            Platform::Object ^ obj = PropertyValue::CreateUInt32(arg->_msgArg->v_uint32);
            if (isKey) {
                // Store result as key
                arg->_msgArg->_eventsAndProperties->Key = typeCoercer->Coerce(obj, ajn::ALLJOYN_UINT32, false);
            } else {
                // Store result as value
                arg->_msgArg->_eventsAndProperties->Value = typeCoercer->Coerce(obj, ajn::ALLJOYN_UINT32, false);
            }
        }
        break;

    case ALLJOYN_UINT64:
        {
            // Convert to alljoyn uint64
            Platform::Object ^ obj = PropertyValue::CreateUInt64(arg->_msgArg->v_uint64);
            if (isKey) {
                // Store result as key
                arg->_msgArg->_eventsAndProperties->Key = typeCoercer->Coerce(obj, ajn::ALLJOYN_UINT64, false);
            } else {
                // Store result as value
                arg->_msgArg->_eventsAndProperties->Value = typeCoercer->Coerce(obj, ajn::ALLJOYN_UINT64, false);
            }
        }
        break;

    case ALLJOYN_OBJECT_PATH:
        {
            // Convert to alljoyn object path
            qcc::String val = arg->_msgArg->v_objPath.str;
            if (isKey) {
                // Store result as key
                arg->_msgArg->_eventsAndProperties->Key = typeCoercer->Coerce(MultibyteToPlatformString(val.c_str()), ajn::ALLJOYN_OBJECT_PATH, false);
            } else {
                // Store result as value
                arg->_msgArg->_eventsAndProperties->Value = typeCoercer->Coerce(MultibyteToPlatformString(val.c_str()), ajn::ALLJOYN_OBJECT_PATH, false);
            }
        }
        break;

    case ALLJOYN_SIGNATURE:
        {
            // Convert to alljoyn signature
            qcc::String val = arg->_msgArg->v_signature.sig;
            if (isKey) {
                // Store result as key
                arg->_msgArg->_eventsAndProperties->Key = typeCoercer->Coerce(MultibyteToPlatformString(val.c_str()), ajn::ALLJOYN_SIGNATURE, false);
            } else {
                // Store result as value
                arg->_msgArg->_eventsAndProperties->Value = typeCoercer->Coerce(MultibyteToPlatformString(val.c_str()), ajn::ALLJOYN_SIGNATURE, false);
            }
        }
        break;

    case ALLJOYN_HANDLE:
        {
            // Convert result to alljoyn handle
            Platform::Object ^ obj = PropertyValue::CreateUInt64(arg->_msgArg->v_handle.fd);
            if (isKey) {
                // Store result as key
                arg->_msgArg->_eventsAndProperties->Key = typeCoercer->Coerce(obj, ajn::ALLJOYN_HANDLE, false);
            } else {
                // Store result as value
                arg->_msgArg->_eventsAndProperties->Value = typeCoercer->Coerce(obj, ajn::ALLJOYN_HANDLE, false);
            }
        }
        break;

    case ALLJOYN_UINT16:
        {
            // Convert result to uint16
            Platform::Object ^ obj = PropertyValue::CreateUInt16(arg->_msgArg->v_uint16);
            if (isKey) {
                // Store result as key
                arg->_msgArg->_eventsAndProperties->Key = typeCoercer->Coerce(obj, ajn::ALLJOYN_UINT16, false);
            } else {
                // Store result as value
                arg->_msgArg->_eventsAndProperties->Value = typeCoercer->Coerce(obj, ajn::ALLJOYN_UINT16, false);
            }
        }
        break;

    case ALLJOYN_INT16:
        {
            // Convert result to int16
            Platform::Object ^ obj = PropertyValue::CreateInt16(arg->_msgArg->v_int16);
            if (isKey) {
                // Store result as key
                arg->_msgArg->_eventsAndProperties->Key = typeCoercer->Coerce(obj, ajn::ALLJOYN_INT16, false);
            } else {
                // Store result as value
                arg->_msgArg->_eventsAndProperties->Value = typeCoercer->Coerce(obj, ajn::ALLJOYN_INT16, false);
            }
        }
        break;

    case ALLJOYN_ARRAY:
        {
            // Convert to alljoyn array
            ajn::MsgArg* elements = (ajn::MsgArg*)arg->_msgArg->v_array.GetElements();
            // Check elements for invalid values
            if (NULL != elements) {
                // Get the array length
                size_t elementCount = arg->_msgArg->v_array.GetNumElements();
                // Allocate array sized for the number of elements
                Platform::Array<Platform::Object ^> ^ arr = ref new Platform::Array<Platform::Object ^>(elementCount);
                for (int i = 0; i < elementCount; i++) {
                    // Create wrapped MsgArg
                    AllJoyn::MsgArg ^ newArg = ref new AllJoyn::MsgArg(&(elements[i]));
                    // Add object reference
                    AddObjectReference(NULL, newArg, &(this->_refMap));
                    // Store the result
                    arr[i] = newArg;
                }
                // Create inspectable array
                Platform::Object ^ obj = PropertyValue::CreateInspectableArray(arr);
                if (isKey) {
                    // Store result as key
                    arg->_msgArg->_eventsAndProperties->Key = typeCoercer->Coerce(obj, ajn::ALLJOYN_ARRAY, false);
                } else {
                    // Store result as value
                    arg->_msgArg->_eventsAndProperties->Value = typeCoercer->Coerce(obj, ajn::ALLJOYN_ARRAY, false);
                }
            }
        }
        break;

    case ALLJOYN_BOOLEAN_ARRAY:
        {
            // Convert to alljoyn boolean array
            Platform::ArrayReference<Platform::Boolean> arrRef((bool*)arg->_msgArg->v_scalarArray.v_bool,
                                                               arg->_msgArg->v_scalarArray.numElements);
            Platform::Object ^ obj = PropertyValue::CreateBooleanArray(arrRef);
            if (isKey) {
                // Store result as key
                arg->_msgArg->_eventsAndProperties->Key = typeCoercer->Coerce(obj, ajn::ALLJOYN_BOOLEAN_ARRAY, false);
            } else {
                // Store result as value
                arg->_msgArg->_eventsAndProperties->Value = typeCoercer->Coerce(obj, ajn::ALLJOYN_BOOLEAN_ARRAY, false);
            }
        }
        break;

    case ALLJOYN_DOUBLE_ARRAY:
        {
            // Convert to alljoyn double array
            Platform::ArrayReference<float64> arrRef((float64*)arg->_msgArg->v_scalarArray.v_double,
                                                     arg->_msgArg->v_scalarArray.numElements);
            Platform::Object ^ obj = PropertyValue::CreateDoubleArray(arrRef);
            if (isKey) {
                // Store result as key
                arg->_msgArg->_eventsAndProperties->Key = typeCoercer->Coerce(obj, ajn::ALLJOYN_DOUBLE_ARRAY, false);
            } else {
                // Store result as value
                arg->_msgArg->_eventsAndProperties->Value = typeCoercer->Coerce(obj, ajn::ALLJOYN_DOUBLE_ARRAY, false);
            }
        }
        break;

    case ALLJOYN_INT32_ARRAY:
        {
            // Convert to alljoyn int32 array
            Platform::ArrayReference<int32> arrRef((int32*)arg->_msgArg->v_scalarArray.v_int32,
                                                   arg->_msgArg->v_scalarArray.numElements);
            Platform::Object ^ obj = PropertyValue::CreateInt32Array(arrRef);
            if (isKey) {
                // Store result as key
                arg->_msgArg->_eventsAndProperties->Key = typeCoercer->Coerce(obj, ajn::ALLJOYN_INT32_ARRAY, false);
            } else {
                // Store result as value
                arg->_msgArg->_eventsAndProperties->Value = typeCoercer->Coerce(obj, ajn::ALLJOYN_INT32_ARRAY, false);
            }
        }
        break;

    case ALLJOYN_INT16_ARRAY:
        {
            // Convert to alljoyn int16 array
            Platform::ArrayReference<int16> arrRef((int16*)arg->_msgArg->v_scalarArray.v_int16,
                                                   arg->_msgArg->v_scalarArray.numElements);
            Platform::Object ^ obj = PropertyValue::CreateInt16Array(arrRef);
            if (isKey) {
                // Store result as key
                arg->_msgArg->_eventsAndProperties->Key = typeCoercer->Coerce(obj, ajn::ALLJOYN_INT16_ARRAY, false);
            } else {
                // Store result as value
                arg->_msgArg->_eventsAndProperties->Value = typeCoercer->Coerce(obj, ajn::ALLJOYN_INT16_ARRAY, false);
            }
        }
        break;

    case ALLJOYN_UINT16_ARRAY:
        {
            // Convert to alljoyn uint16 array
            Platform::ArrayReference<uint16> arrRef((uint16*)arg->_msgArg->v_scalarArray.v_uint16,
                                                    arg->_msgArg->v_scalarArray.numElements);
            Platform::Object ^ obj = PropertyValue::CreateUInt16Array(arrRef);
            if (isKey) {
                // Store result as key
                arg->_msgArg->_eventsAndProperties->Key = typeCoercer->Coerce(obj, ajn::ALLJOYN_UINT16_ARRAY, false);
            } else {
                // Store result as value
                arg->_msgArg->_eventsAndProperties->Value = typeCoercer->Coerce(obj, ajn::ALLJOYN_UINT16_ARRAY, false);
            }
        }
        break;

    case ALLJOYN_UINT64_ARRAY:
        {
            // Convert to alljoyn uint64 array
            Platform::ArrayReference<uint64> arrRef((uint64*)arg->_msgArg->v_scalarArray.v_uint64,
                                                    arg->_msgArg->v_scalarArray.numElements);
            Platform::Object ^ obj = PropertyValue::CreateUInt64Array(arrRef);
            if (isKey) {
                // Store result as key
                arg->_msgArg->_eventsAndProperties->Key = typeCoercer->Coerce(obj, ajn::ALLJOYN_UINT64_ARRAY, false);
            } else {
                // Store result as value
                arg->_msgArg->_eventsAndProperties->Value = typeCoercer->Coerce(obj, ajn::ALLJOYN_UINT64_ARRAY, false);
            }
        }
        break;

    case ALLJOYN_UINT32_ARRAY:
        {
            // Convert to alljoyn uint32 array
            Platform::ArrayReference<uint32> arrRef((uint32*)arg->_msgArg->v_scalarArray.v_uint32,
                                                    arg->_msgArg->v_scalarArray.numElements);
            Platform::Object ^ obj = PropertyValue::CreateUInt32Array(arrRef);
            if (isKey) {
                // Store result as key
                arg->_msgArg->_eventsAndProperties->Key = typeCoercer->Coerce(obj, ajn::ALLJOYN_UINT32_ARRAY, false);
            } else {
                // Store result as value
                arg->_msgArg->_eventsAndProperties->Value = typeCoercer->Coerce(obj, ajn::ALLJOYN_UINT32_ARRAY, false);
            }
        }
        break;

    case ALLJOYN_INT64_ARRAY:
        {
            // Convert to alljoyn int64 array
            Platform::ArrayReference<int64> arrRef((int64*)arg->_msgArg->v_scalarArray.v_int64,
                                                   arg->_msgArg->v_scalarArray.numElements);
            Platform::Object ^ obj = PropertyValue::CreateInt64Array(arrRef);
            if (isKey) {
                // Store result as key
                arg->_msgArg->_eventsAndProperties->Key = typeCoercer->Coerce(obj, ajn::ALLJOYN_INT64_ARRAY, false);
            } else {
                // Store result as value
                arg->_msgArg->_eventsAndProperties->Value = typeCoercer->Coerce(obj, ajn::ALLJOYN_INT64_ARRAY, false);
            }
        }
        break;

    case ALLJOYN_BYTE_ARRAY:
        {
            // Convert to alljoyn byte array
            Platform::ArrayReference<uint8> arrRef((uint8*)arg->_msgArg->v_scalarArray.v_byte,
                                                   arg->_msgArg->v_scalarArray.numElements);
            Platform::Object ^ obj = PropertyValue::CreateUInt8Array(arrRef);
            if (isKey) {
                // Store result as key
                arg->_msgArg->_eventsAndProperties->Key = typeCoercer->Coerce(obj, ajn::ALLJOYN_BYTE_ARRAY, false);
            } else {
                // Store result as value
                arg->_msgArg->_eventsAndProperties->Value = typeCoercer->Coerce(obj, ajn::ALLJOYN_BYTE_ARRAY, false);
            }
        }
        break;

    default:
        break;
    }

    if (ER_OK != status) {
        QCC_THROW_EXCEPTION(status);
    }
}

__MsgArg::__MsgArg()
{
    Value = nullptr;
    Key = nullptr;
}

__MsgArg::~__MsgArg()
{
    Value = nullptr;
    Key = nullptr;
}

}

