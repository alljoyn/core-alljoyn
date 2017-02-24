/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
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

#include <string>
#include <vector>

struct SizeParams {
  public:
    AJ_PCSTR rulesXml;
    size_t integer;

    SizeParams(AJ_PCSTR _rulesXml, size_t _integer) :
        rulesXml(_rulesXml),
        integer(_integer)
    { }
};

struct TwoStringsParams {
  public:
    AJ_PCSTR rulesXml;
    std::vector<std::string> strings;

    TwoStringsParams(AJ_PCSTR _rulesXml, AJ_PCSTR first, AJ_PCSTR second) :
        rulesXml(_rulesXml)
    {
        strings = std::vector<std::string>(2U);
        strings[0] = first;
        strings[1] = second;
    }
};

static AJ_PCSTR VALID_NEED_ALL_MANIFEST_TEMPLATE =
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
static AJ_PCSTR VALID_NODE_WITH_NAME =
    "<manifest>"
    "<node name = \"/Node\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR VALID_NODE_WITH_UNDERSCORE =
    "<manifest>"
    "<node name = \"/_Node\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR VALID_NODE_WILDCARD_ONLY =
    "<manifest>"
    "<node name = \"*\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR VALID_NODE_WITH_WILDCARD =
    "<manifest>"
    "<node name = \"/Node/*\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR VALID_NODE_WITH_DIGIT =
    "<manifest>"
    "<node name = \"/Node1\">"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR VALID_INTERFACE_WITH_NAME =
    "<manifest>"
    "<node>"
    "<interface name = \"org.Interface\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR VALID_INTERFACE_WITH_WILDCARD =
    "<manifest>"
    "<node>"
    "<interface name = \"org.interface.*\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR VALID_INTERFACE_WITH_DIGIT =
    "<manifest>"
    "<node>"
    "<interface name = \"org.interface1\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR VALID_INTERFACE_WITH_UNDERSCORE =
    "<manifest>"
    "<node>"
    "<interface name = \"_org.interface\">"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR VALID_MEMBER_WITH_NAME =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method name = \"methodName\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR VALID_MEMBER_WITH_DIGIT =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method name = \"methodName5\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR VALID_MEMBER_WITH_UNDERSCORE =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method name = \"_methodName\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR VALID_MEMBER_WITH_WILDCARD =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method name = \"methodName*\">"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Modify\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR VALID_METHOD_WITH_DENY =
    "<manifest>"
    "<node>"
    "<interface>"
    "<method>"
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Deny\"/>"
    "</method>"
    "</interface>"
    "</node>"
    "</manifest>";
static AJ_PCSTR VALID_SAME_NAME_INTERFACES_IN_SEPARATE_NODES =
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
static AJ_PCSTR VALID_NAMELESS_INTERFACES_IN_SEPARATE_NODES =
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
static AJ_PCSTR VALID_DIFFERENT_NAME_INTERFACES_IN_ONE_NODE =
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
static AJ_PCSTR VALID_SAME_NAME_METHODS_IN_SEPARATE_INTERFACES =
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
static AJ_PCSTR VALID_NAMELESS_METHODS_IN_SEPARATE_INTERFACES =
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
static AJ_PCSTR VALID_DIFFERENT_NAME_METHODS_IN_ONE_INTERFACE =
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
static AJ_PCSTR VALID_SAME_NAME_PROPERTIES_IN_SEPARATE_INTERFACES =
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
static AJ_PCSTR VALID_NAMELESS_PROPERTIES_IN_SEPARATE_INTERFACES =
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
static AJ_PCSTR VALID_DIFFERENT_NAME_PROPERTIES_IN_ONE_INTERFACE =
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
static AJ_PCSTR VALID_SAME_NAME_SIGNALS_IN_SEPARATE_INTERFACES =
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
static AJ_PCSTR VALID_NAMELESS_SIGNALS_IN_SEPARATE_INTERFACES =
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
static AJ_PCSTR VALID_DIFFERENT_NAME_SIGNALS_IN_ONE_INTERFACE =
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