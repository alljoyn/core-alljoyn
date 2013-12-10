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

#include "SocketStream.h"

namespace AllJoyn {

/// <summary>
///Handle the received data by a SocketStream object
/// </summary>
public delegate void SocketStreamDataReceivedHandler();

/// <summary>
/// <c>SocketStreamEvent</c> is for notifying a <c>SocketStream</c> object has received incoming data.
/// </summary>
public ref class SocketStreamEvent sealed {
  public:
    /// <summary>
    ///Constructor
    /// </summary>
    /// <param name="sockStream">The <c>SocketStream</c> object</param>
    SocketStreamEvent(SocketStream ^ sockStream);
    /// <summary>
    ///Triggered when SocketStream has received data
    /// </summary>
    event SocketStreamDataReceivedHandler ^ DataReceived;

  private:

    ~SocketStreamEvent();
    void DefaultSocketStreamDataReceivedHandler();
    void SocketEventsChangedHandler(Platform::Object ^ source, int events);
};

}
// SocketStream.h
