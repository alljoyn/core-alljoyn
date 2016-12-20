#ifndef _STUNATTRIBUTE_H
#define _STUNATTRIBUTE_H
/**
 * @file
 *
 * This file is a convenient wrapper for including all the STUN Message
 * Attribute header files..
 */

/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#ifndef __cplusplus
#error Only include StunAttribute.h in C++ code.
#endif

#include <StunAttributeAlternateServer.h>
#include <StunAttributeBase.h>
#include <StunAttributeChannelNumber.h>
#include <StunAttributeData.h>
#include <StunAttributeDontFragment.h>
#include <StunAttributeErrorCode.h>
#include <StunAttributeEvenPort.h>
#include <StunAttributeFingerprint.h>
#include <StunAttributeIceControlled.h>
#include <StunAttributeIceControlling.h>
#include <StunAttributeLifetime.h>
#include <StunAttributeMappedAddress.h>
#include <StunAttributeMessageIntegrity.h>
#include <StunAttributePriority.h>
#include <StunAttributeRequestedTransport.h>
#include <StunAttributeReservationToken.h>
#include <StunAttributeSoftware.h>
#include <StunAttributeUnknownAttributes.h>
#include <StunAttributeUseCandidate.h>
#include <StunAttributeUsername.h>
#include <StunAttributeXorMappedAddress.h>
#include <StunAttributeAllocatedXorServerReflexiveAddress.h>
#include <StunAttributeXorPeerAddress.h>
#include <StunAttributeXorRelayedAddress.h>
#include <StunAttributeIceCheckFlag.h>

#endif