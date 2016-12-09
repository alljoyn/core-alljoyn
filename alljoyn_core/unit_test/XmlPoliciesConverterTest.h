/******************************************************************************
 * Copyright (c) 2016 Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright 2016 Open Connectivity Foundation and Contributors to
 *    AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *     PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include <string>
#include <vector>
#include "XmlConverterTest.h"

#define BASIC_VALID_RULES \
    "<rules>" \
    "<node>" \
    "<interface>" \
    "<method>" \
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>" \
    "</method>" \
    "</interface>" \
    "</node>" \
    "</rules>"

#define FIRST_VALID_PUBLIC_KEY \
    "-----BEGIN PUBLIC KEY-----\n" \
    "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEaGDzY4qFMpBqZcex+c66GdvMo/Q9\n" \
    "H5DHr/+//RCDkuUorPCEHoJS5+UTkvn7XJNqp0gzGj+v55hrz0p/V9NHQw==\n" \
    "-----END PUBLIC KEY-----" \

#define SECOND_VALID_PUBLIC_KEY \
    "-----BEGIN PUBLIC KEY-----\n" \
    "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAETHDIEthNo/28eKnN4cBtapQ6n3qA\n" \
    "IJkXmOuHapWDvx6MUqLesiqrWe2zwY+tLPdMB+ud16kWp34Na/ZMHfxi3A==\n" \
    "-----END PUBLIC KEY-----"

#define FIRST_VALID_GUID "16fc57377fced83516fc57377fced835"
#define SECOND_VALID_GUID "f5175474e932917fa3d9b5fd665f65de"

extern AJ_PCSTR s_validAllTypePeer;
extern AJ_PCSTR s_validTwoAcls;
extern AJ_PCSTR s_validAnyTrustedPeer;
extern AJ_PCSTR s_validFromCa;
extern AJ_PCSTR s_validWithMembership;
extern AJ_PCSTR s_validWithPublicKey;
extern AJ_PCSTR s_validAnyTrustedPeerWithOther;
extern AJ_PCSTR s_validTwoDifferentCa;
extern AJ_PCSTR s_validTwoWithMembershipDifferentKeys;
extern AJ_PCSTR s_validTwoWithMembershipDifferentSgids;
extern AJ_PCSTR s_validTwoDifferentWithPublicKey;
extern AJ_PCSTR s_validSameKeyCaAndWithPublicKey;
// Valid only until ASACORE-2985 is resolved.
extern AJ_PCSTR s_validNoRulesElement;