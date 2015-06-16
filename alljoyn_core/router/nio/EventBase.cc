/*
 * EventBase.cc
 *
 *  Created on: May 18, 2015
 *      Author: erongo
 */
/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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
 ******************************************************************************/
#include "EventBase.h"

namespace nio {

EventBase::EventBase() : enabled(true)
{

}

EventBase::~EventBase()
{
}

void EventBase::Execute()
{
    std::lock_guard<std::recursive_mutex> guard(lock);
    if (enabled) {
        ExecuteInternal();
    }
}

void EventBase::SetEnabled(bool enabled)
{
    std::lock_guard<std::recursive_mutex> guard(lock);
    this->enabled = enabled;
}

} /* namespace nio */
