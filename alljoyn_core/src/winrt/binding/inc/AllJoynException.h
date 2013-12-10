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

#pragma once

#include <qcc/platform.h>
#include <Status_CPP0x.h>
#include <qcc/winrt/utility.h>
#include <qcc/winrt/exception.h>

namespace AllJoyn {

/// <summary>
/// A helper class to interpret a customized COMException thrown by AllJoyn component. When an AllJoyn
/// method/signal call fails, AllJoyn throws a customized COMException which contains the QStatus code.
/// This class provides static methods to retrieve the QStatus/Error code and the corresponding error message.
/// </summary>
public ref class AllJoynException sealed {
  public:
    /// <summary>
    /// Map a COMException HResult property to the corresponding AllJoyn QStatus code.
    /// </summary>
    /// <param name="hresult">The HResult property of a COMException </param>
    /// <returns>The corresponding AllJoyn QStatus code </returns>
    static QStatus GetErrorCode(int hresult)
    {
        return (QStatus)(hresult & 0x3FFFFFFF);
    }

    /// <summary>
    /// Get a text string that gives more information about a COMException thrown by AllJoyn component.
    /// </summary>
    /// <param name="hresult">The HResult property of a COMException </param>
    /// <returns>A text string that sheds light on the cause of the COMException </returns>
    static Platform::String ^ GetErrorMessage(int hresult)
    {
        return MultibyteToPlatformString(qcc::QCC_StatusMessage(hresult & 0x3FFFFFFF));
    }
};

}
// AllJoynException.h
