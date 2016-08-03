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
#include "XmlRulesConverterTest.h"

TwoStringsParams::TwoStringsParams(AJ_PCSTR _rulesXml, AJ_PCSTR first, AJ_PCSTR second) :
    m_rulesXml(_rulesXml)
{
    m_strings = std::vector<std::string>(2U);
    m_strings[0] = first;
    m_strings[1] = second;
}

AJ_PCSTR s_validNeedAllRulesXml =
    "<rules>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</method>"
    "<property>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "</property>"
    "<signal>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "</signal>"
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validNodeWithName =
    "<rules>"
    "<node name = \"/Node\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validNodeWithUnderscore =
    "<rules>"
    "<node name = \"/_Node\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validNodeWildcardOnly =
    "<rules>"
    "<node name = \"*\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validNodeWithWildcard =
    "<rules>"
    "<node name = \"/Node/*\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validNodeWithDigit =
    "<rules>"
    "<node name = \"/Node1\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validNodeNameWithWildcardNotAfterSlash =
    "<rules>"
    "<node name = \"/Node*\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validInterfaceWithName =
    "<rules>"
    "<node>"
    "<interface name = \"org.Interface\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validInterfaceWithWildcard =
    "<rules>"
    "<node>"
    "<interface name = \"org.interface.*\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validInterfaceWithDigit =
    "<rules>"
    "<node>"
    "<interface name = \"org.interface1\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validInterfaceWithUnderscore =
    "<rules>"
    "<node>"
    "<interface name = \"_org.interface\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validInterfaceNameWithWildcardNotAfterDot =
    "<rules>"
    "<node>"
    "<interface name = \"org.interface*\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validMemberWithName =
    "<rules>"
    "<node>"
    "<interface>"
    "<method name = \"methodName\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validMemberWithDigit =
    "<rules>"
    "<node>"
    "<interface>"
    "<method name = \"methodName5\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validMemberWithUnderscore =
    "<rules>"
    "<node>"
    "<interface>"
    "<method name = \"_methodName\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validMemberWithWildcard =
    "<rules>"
    "<node>"
    "<interface>"
    "<method name = \"methodName*\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validMethodWithDeny =
    "<rules>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Deny\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validSameNameInterfacesInSeparateNodes =
    "<rules>"
    "<node name = \"/Node0\">"
    "<interface name = \"org.interface\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "<node name = \"/Node1\">"
    "<interface name = \"org.interface\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validNamelessInterfacesInSeparateNodes =
    "<rules>"
    "<node name = \"/Node0\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "<node name = \"/Node1\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validDifferentNameInterfacesInOneNode =
    "<rules>"
    "<node>"
    "<interface name = \"org.interface1\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "<interface name = \"org.interface2\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validSameNameMethodsInSeparateInterfaces =
    "<rules>"
    "<node>"
    "<interface name = \"org.interface0\">"
    "<method name = \"Method\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "<interface name = \"org.interface1\">"
    "<method name = \"Method\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validNamelessMethodsInSeparateInterfaces =
    "<rules>"
    "<node>"
    "<interface name = \"org.interface0\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "<interface name = \"org.interface1\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validDifferentNameMethodsInOneInterface =
    "<rules>"
    "<node>"
    "<interface>"
    "<method name = \"Method0\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "<method name = \"Method1\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validSameNamePropertiesInSeparateInterfaces =
    "<rules>"
    "<node>"
    "<interface name = \"org.interface0\">"
    "<property name = \"Property\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</property>"
    "</interface>"
    "<interface name = \"org.interface1\">"
    "<property name = \"Property\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</property>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validNamelessPropertiesInSeparateInterfaces =
    "<rules>"
    "<node>"
    "<interface name = \"org.interface0\">"
    "<property>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</property>"
    "</interface>"
    "<interface name = \"org.interface1\">"
    "<property>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</property>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validDifferentNamePropertiesInOneInterface =
    "<rules>"
    "<node>"
    "<interface>"
    "<property name = \"Property0\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</property>"
    "<property name = \"Property1\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</property>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validSameNameSignalsInSeparateInterfaces =
    "<rules>"
    "<node>"
    "<interface name = \"org.interface0\">"
    "<signal name = \"Signal\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</signal>"
    "</interface>"
    "<interface name = \"org.interface1\">"
    "<signal name = \"Signal\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</signal>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validNamelessSignalsInSeparateInterfaces =
    "<rules>"
    "<node>"
    "<interface name = \"org.interface0\">"
    "<signal>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</signal>"
    "</interface>"
    "<interface name = \"org.interface1\">"
    "<signal>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</signal>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validDifferentNameSignalsInOneInterface =
    "<rules>"
    "<node>"
    "<interface>"
    "<signal name = \"Signal0\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</signal>"
    "<signal name = \"Signal1\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</signal>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validSameNameAnyMembersInSeparateInterfaces =
    "<rules>"
    "<node>"
    "<interface name = \"org.interface0\">"
    "<any name = \"Any\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</any>"
    "</interface>"
    "<interface name = \"org.interface1\">"
    "<any name = \"Any\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validNamelessAnyMembersInSeparateInterfaces =
    "<rules>"
    "<node>"
    "<interface name = \"org.interface0\">"
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</any>"
    "</interface>"
    "<interface name = \"org.interface1\">"
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_validDifferentNameAnyMembersInOneInterface =
    "<rules>"
    "<node>"
    "<interface>"
    "<any name = \"Any0\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</any>"
    "<any name = \"Any1\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</rules>";
AJ_PCSTR s_needAllManifestTemplateWithNodeSecurityLevelAnnotation =
    "<manifest>"
    "<node>"
    SECURITY_LEVEL_ANNOTATION(PRIVILEGED_SECURITY_LEVEL)
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</method>"
    "<property>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "</property>"
    "<signal>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "</signal>"
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</manifest>";
AJ_PCSTR s_needAllManifestTemplateWithInterfaceSecurityLevelAnnotation =
    "<manifest>"
    "<node>"
    "<interface>"
    SECURITY_LEVEL_ANNOTATION(PRIVILEGED_SECURITY_LEVEL)
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "</method>"
    "<property>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "</property>"
    "<signal>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "</signal>"
    "<any>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Observe\"/>"
    "</any>"
    "</interface>"
    "</node>"
    "</manifest>";
