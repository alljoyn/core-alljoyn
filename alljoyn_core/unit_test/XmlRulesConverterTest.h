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

#include <string>
#include <vector>
#include <qcc/platform.h>
#include "XmlConverterTest.h"
#include "XmlManifestTemplateValidator.h"

#define SECURITY_LEVEL_ANNOTATION(level) "<annotation name=\"" SECURITY_LEVEL_ANNOTATION_NAME "\" value=\"" level "\"/>"

struct TwoStringsParams {
  public:
    AJ_PCSTR m_rulesXml;
    std::vector<std::string> m_strings;

    TwoStringsParams(AJ_PCSTR _rulesXml, AJ_PCSTR first, AJ_PCSTR second);
};

extern AJ_PCSTR s_validNeedAllRulesXml;
extern AJ_PCSTR s_validNodeWithName;
extern AJ_PCSTR s_validNodeWithUnderscore;
extern AJ_PCSTR s_validNodeWildcardOnly;
extern AJ_PCSTR s_validNodeWithWildcard;
extern AJ_PCSTR s_validNodeWithDigit;
extern AJ_PCSTR s_validNodeNameWithWildcardNotAfterSlash;
extern AJ_PCSTR s_validInterfaceWithName;
extern AJ_PCSTR s_validInterfaceWithWildcard;
extern AJ_PCSTR s_validInterfaceWithDigit;
extern AJ_PCSTR s_validInterfaceWithUnderscore;
extern AJ_PCSTR s_validInterfaceNameWithWildcardNotAfterDot;
extern AJ_PCSTR s_validMemberWithName;
extern AJ_PCSTR s_validMemberWithDigit;
extern AJ_PCSTR s_validMemberWithUnderscore;
extern AJ_PCSTR s_validMemberWithWildcard;
extern AJ_PCSTR s_validMethodWithDeny;
extern AJ_PCSTR s_validSameNameInterfacesInSeparateNodes;
extern AJ_PCSTR s_validNamelessInterfacesInSeparateNodes;
extern AJ_PCSTR s_validDifferentNameInterfacesInOneNode;
extern AJ_PCSTR s_validSameNameMethodsInSeparateInterfaces;
extern AJ_PCSTR s_validNamelessMethodsInSeparateInterfaces;
extern AJ_PCSTR s_validDifferentNameMethodsInOneInterface;
extern AJ_PCSTR s_validSameNamePropertiesInSeparateInterfaces;
extern AJ_PCSTR s_validNamelessPropertiesInSeparateInterfaces;
extern AJ_PCSTR s_validDifferentNamePropertiesInOneInterface;
extern AJ_PCSTR s_validSameNameSignalsInSeparateInterfaces;
extern AJ_PCSTR s_validNamelessSignalsInSeparateInterfaces;
extern AJ_PCSTR s_validDifferentNameSignalsInOneInterface;
extern AJ_PCSTR s_validSameNameAnyMembersInSeparateInterfaces;
extern AJ_PCSTR s_validNamelessAnyMembersInSeparateInterfaces;
extern AJ_PCSTR s_validDifferentNameAnyMembersInOneInterface;
extern AJ_PCSTR s_needAllManifestTemplateWithNodeSecurityLevelAnnotation;
extern AJ_PCSTR s_needAllManifestTemplateWithInterfaceSecurityLevelAnnotation;
