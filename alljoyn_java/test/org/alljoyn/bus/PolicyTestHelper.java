/**
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
*/

package org.alljoyn.bus;

public class PolicyTestHelper {
    public static final String POLICY_BEGIN =
        "<policy>" +
        "<policyVersion>1</policyVersion>" +
        "<serialNumber>2</serialNumber>" +
        "<acls>";

    public static final String ALL_ACL =
        "<acl>" +
        "<peers>" +
        "<peer>" +
        "<type>ALL</type>" +
        "</peer>" +
        "</peers>" +
        "<rules>" +
        "<node name=\"*\">" +
        "<interface name=\"*\">";

    public static final String REPLACE_CA = "caKey";

    public static final String CA_ACL =
        "<acl>" +
        "<peers>" +
        "<peer>" +
        "<type>FROM_CERTIFICATE_AUTHORITY</type>" +
        "<publicKey>" + REPLACE_CA + "</publicKey>" +
        "</peer>" +
        "</peers>" +
        "<rules>" +
        "<node name=\"*\">" +
        "<interface name=\"*\">";

    public static final String REPLACE_MEMBER = "managerKey";
    public static final String REPLACE_MEMBER_GUID = "guid";

    public static final String MEMBER_ACL =
        "<acl>" +
        "<peers>" +
        "<peer>" +
        "<type>WITH_MEMBERSHIP</type>" +
        "<publicKey>" + REPLACE_MEMBER + "</publicKey>" +
        "<sgID>" + REPLACE_MEMBER_GUID + "</sgID>" +
        "</peer>" +
        "</peers>" +
        "<rules>" +
        "<node name=\"*\">" +
        "<interface name=\"*\">";

    public static final String ACL_METHOD =
        "<method name=\"*\">" +
        "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Provide\"/>" +
        "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Modify\"/>" +
        "</method>";

    public static final String ACL_SIGNAL =
		"<signal name=\"*\">" +
        "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Provide\"/>" +
        "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Observe\"/>" +
        "</signal>";

    public static final String ACL_PROPERTY =
        "<property name=\"*\">" +
        "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Provide\"/>" +
        "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Observe\"/>" +
        "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Modify\"/>" +
        "</property>";

    public static final String ACL_ANY =
        "<any name=\"*\">" +
        "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Provide\"/>" +
        "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Observe\"/>" +
        "<annotation name=\"org.alljoyn.Bus.Action\" value=\"Modify\"/>" +
        "</any>";

    public static final String ACL_END =
        "</interface>" +
        "</node>" +
        "</rules>" +
        "</acl>";

    public static final String POLICY_END =
        "</acls>" +
        "</policy>";
}
