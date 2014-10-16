/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#include "gtest/gtest.h"
#include "AuthorizationData.h"
#include <iostream>

using namespace ajn;

TEST(AuthorizationDataTest, BasicTest) {
    AuthorizationData ad;
    std::cout << "Constructed AuthorizationData.\n";
    qcc::String ifn = "org.allseen.control.TV";
    qcc::String mbr = "*";
    Type t = Type::SIGNAL;
    Action a = Action::PROVIDE;
    ad.AddRule(ifn, mbr, t, a);
    std::cout << "Added rule to AuthorizationData.\n";
    ASSERT_TRUE(ad.IsAllowed(ifn, mbr, t, a));
    ajn::MsgArg ma;
    ad.Marshal(ma);
    std::cout << "Marshalled the AuthorizationData.\n";
    AuthorizationData ad2;
    ad2.Unmarshal(ma);
    std::cout << "Unmarshalled the AuthorizationData.\n";
    ASSERT_TRUE(ad2.IsAllowed(ifn, mbr, t, a));
    qcc::String str = ad2.ToString();
    std::cout << "Serialized the authorization data:" << str << "\n";
    qcc::String expectedStr = "{\"version\":1,\"rules\":[\"org.allseen.control.TV\":{\"*\":P}]}";
    ASSERT_TRUE(str == expectedStr);
    AuthorizationData ad3;
    ad3.FromString(str);
    qcc::String str3 = ad3.ToString();
    std::cout << "Deserialized the authorization data:" << str3 << "\n";
    ASSERT_TRUE(str3 == expectedStr);

    AuthorizationData emptyAd;
    qcc::String emptyAdExpectedStr = "{\"version\":1,\"rules\":[]}";
    qcc::String emptyAdStr = emptyAd.ToString();
    std::cout << "Serialized empty authorization data:" << emptyAdStr << "\n";
    ASSERT_TRUE(emptyAdStr == emptyAdExpectedStr);
    AuthorizationData emptyAd2;
    emptyAd2.FromString(emptyAdStr);
    qcc::String emptyAdStr2 = emptyAd2.ToString();
    std::cout << "Deserialized empty authorization data:" << emptyAdStr2 << "\n";
    ASSERT_TRUE(emptyAdStr2 == emptyAdExpectedStr);
}
