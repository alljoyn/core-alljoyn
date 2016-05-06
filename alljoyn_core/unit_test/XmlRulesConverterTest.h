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

struct SizeParams {
  public:
    AJ_PCSTR m_rulesXml;
    size_t m_integer;

    SizeParams(AJ_PCSTR _rulesXml, size_t _integer) :
        m_rulesXml(_rulesXml),
        m_integer(_integer)
    { }
};

struct TwoStringsParams {
  public:
    AJ_PCSTR m_rulesXml;
    std::vector<std::string> m_strings;

    TwoStringsParams(AJ_PCSTR _rulesXml, AJ_PCSTR first, AJ_PCSTR second) :
        m_rulesXml(_rulesXml)
    {
        m_strings = std::vector<std::string>(2U);
        m_strings[0] = first;
        m_strings[1] = second;
    }
};

static AJ_PCSTR s_validNeedAllManifestTemplate =
    "<manifest>"
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
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_validNodeWithName =
    "<manifest>"
    "<node name = \"/Node\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_validNodeWithUnderscore =
    "<manifest>"
    "<node name = \"/_Node\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_validNodeWildcardOnly =
    "<manifest>"
    "<node name = \"*\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_validNodeWithWildcard =
    "<manifest>"
    "<node name = \"/Node/*\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_validNodeWithDigit =
    "<manifest>"
    "<node name = \"/Node1\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_validInterfaceWithName =
    "<manifest>"
    "<node>"
    "<interface name = \"org.Interface\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_validInterfaceWithWildcard =
    "<manifest>"
    "<node>"
    "<interface name = \"org.interface.*\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_validInterfaceWithDigit =
    "<manifest>"
    "<node>"
    "<interface name = \"org.interface1\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_validInterfaceWithUnderscore =
    "<manifest>"
    "<node>"
    "<interface name = \"_org.interface\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_validMemberWithName =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method name = \"methodName\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_validMemberWithDigit =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method name = \"methodName5\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_validMemberWithUnderscore =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method name = \"_methodName\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_validMemberWithWildcard =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method name = \"methodName*\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_validMethodWithDeny =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Deny\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR s_validSameNameInterfacesInSeparateNodes =
    "<manifest>"
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
    "</manifest>";
static AJ_PCSTR s_validNamelessInterfacesInSeparateNodes =
    "<manifest>"
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
    "</manifest>";
static AJ_PCSTR s_validDifferentNameInterfacesInOneNode =
    "<manifest>"
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
    "</manifest>";
static AJ_PCSTR s_validSameNameMethodsInSeparateInterfaces =
    "<manifest>"
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
    "</manifest>";
static AJ_PCSTR s_validNamelessMethodsInSeparateInterfaces =
    "<manifest>"
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
    "</manifest>";
static AJ_PCSTR s_validDifferentNameMethodsInOneInterface =
    "<manifest>"
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
    "</manifest>";
static AJ_PCSTR s_validSameNamePropertiesInSeparateInterfaces =
    "<manifest>"
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
    "</manifest>";
static AJ_PCSTR s_validNamelessPropertiesInSeparateInterfaces =
    "<manifest>"
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
    "</manifest>";
static AJ_PCSTR s_validDifferentNamePropertiesInOneInterface =
    "<manifest>"
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
    "</manifest>";
static AJ_PCSTR s_validSameNameSignalsInSeparateInterfaces =
    "<manifest>"
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
    "</manifest>";
static AJ_PCSTR s_validNamelessSignalsInSeparateInterfaces =
    "<manifest>"
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
    "</manifest>";
static AJ_PCSTR s_validDifferentNameSignalsInOneInterface =
    "<manifest>"
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
    "</manifest>";
