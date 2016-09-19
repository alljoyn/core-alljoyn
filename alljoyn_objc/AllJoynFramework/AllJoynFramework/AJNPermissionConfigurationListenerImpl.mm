////////////////////////////////////////////////////////////////////////////////
// Copyright AllSeen Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for any
//    purpose with or without fee is hereby granted, provided that the above
//    copyright notice and this permission notice appear in all copies.
//
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import "AJNPermissionConfigurationListenerImpl.h"

AJNPermissionConfigurationListenerImpl::AJNPermissionConfigurationListenerImpl(id<AJNPermissionConfigurationListener> delegate) :
    m_delegate(delegate)
{
}

AJNPermissionConfigurationListenerImpl::~AJNPermissionConfigurationListenerImpl()
{
    m_delegate = nil;
}

QStatus AJNPermissionConfigurationListenerImpl::FactoryReset()
{
    if ([m_delegate respondsToSelector:@selector(factoryReset)]) {
        return [m_delegate factoryReset];
    }

    return ER_FAIL;
}

void AJNPermissionConfigurationListenerImpl::StartManagement()
{
    if ([m_delegate respondsToSelector:@selector(startManagement)]) {
        [m_delegate startManagement];
    }
}

void AJNPermissionConfigurationListenerImpl::EndManagement()
{
    if ([m_delegate respondsToSelector:@selector(endManagement)]) {
        [m_delegate endManagement];
    }

}

void AJNPermissionConfigurationListenerImpl::PolicyChanged()
{
    if ([m_delegate respondsToSelector:@selector(didPolicyChange)]) {
        [m_delegate didPolicyChange];
    }
}
