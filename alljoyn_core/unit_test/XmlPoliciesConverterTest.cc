/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/

#include <string>
#include <vector>
#include "XmlPoliciesConverterTest.h"

AJ_PCSTR s_validAllTypePeer =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>ALL</type>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
AJ_PCSTR s_validTwoAcls =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>ALL</type>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>ANY_TRUSTED</type>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
AJ_PCSTR s_validAnyTrustedPeer =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>ANY_TRUSTED</type>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
AJ_PCSTR s_validFromCa =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>FROM_CERTIFICATE_AUTHORITY</type>"
    "<publicKey>"
    FIRST_VALID_PUBLIC_KEY
    "</publicKey>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
AJ_PCSTR s_validWithMembership =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>WITH_MEMBERSHIP</type>"
    "<publicKey>"
    FIRST_VALID_PUBLIC_KEY
    "</publicKey>"
    "<sgID>" FIRST_VALID_GUID "</sgID>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
AJ_PCSTR s_validWithPublicKey =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>WITH_PUBLIC_KEY</type>"
    "<publicKey>"
    FIRST_VALID_PUBLIC_KEY
    "</publicKey>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
AJ_PCSTR s_validAnyTrustedPeerWithOther =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>ANY_TRUSTED</type>"
    "</peer>"
    "<peer>"
    "<type>WITH_PUBLIC_KEY</type>"
    "<publicKey>"
    FIRST_VALID_PUBLIC_KEY
    "</publicKey>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
AJ_PCSTR s_validTwoDifferentCa =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>FROM_CERTIFICATE_AUTHORITY</type>"
    "<publicKey>"
    FIRST_VALID_PUBLIC_KEY
    "</publicKey>"
    "</peer>"
    "<peer>"
    "<type>FROM_CERTIFICATE_AUTHORITY</type>"
    "<publicKey>"
    SECOND_VALID_PUBLIC_KEY
    "</publicKey>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
AJ_PCSTR s_validTwoWithMembershipDifferentKeys =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>WITH_MEMBERSHIP</type>"
    "<publicKey>"
    FIRST_VALID_PUBLIC_KEY
    "</publicKey>"
    "<sgID>" FIRST_VALID_GUID "</sgID>"
    "</peer>"
    "<peer>"
    "<type>WITH_MEMBERSHIP</type>"
    "<publicKey>"
    SECOND_VALID_PUBLIC_KEY
    "</publicKey>"
    "<sgID>" FIRST_VALID_GUID "</sgID>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
AJ_PCSTR s_validTwoWithMembershipDifferentSgids =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>WITH_MEMBERSHIP</type>"
    "<publicKey>"
    FIRST_VALID_PUBLIC_KEY
    "</publicKey>"
    "<sgID>" FIRST_VALID_GUID "</sgID>"
    "</peer>"
    "<peer>"
    "<type>WITH_MEMBERSHIP</type>"
    "<publicKey>"
    FIRST_VALID_PUBLIC_KEY
    "</publicKey>"
    "<sgID>" SECOND_VALID_GUID "</sgID>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
AJ_PCSTR s_validTwoDifferentWithPublicKey =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>WITH_PUBLIC_KEY</type>"
    "<publicKey>"
    FIRST_VALID_PUBLIC_KEY
    "</publicKey>"
    "</peer>"
    "<peer>"
    "<type>WITH_PUBLIC_KEY</type>"
    "<publicKey>"
    SECOND_VALID_PUBLIC_KEY
    "</publicKey>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";
AJ_PCSTR s_validSameKeyCaAndWithPublicKey =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>FROM_CERTIFICATE_AUTHORITY</type>"
    "<publicKey>"
    FIRST_VALID_PUBLIC_KEY
    "</publicKey>"
    "</peer>"
    "<peer>"
    "<type>WITH_PUBLIC_KEY</type>"
    "<publicKey>"
    FIRST_VALID_PUBLIC_KEY
    "</publicKey>"
    "</peer>"
    "</peers>"
    BASIC_VALID_RULES
    "</acl>"
    "</acls>"
    "</policy>";

// Valid only until ASACORE-2985 is resolved.
AJ_PCSTR s_validNoRulesElement =
    "<policy>"
    "<policyVersion>1</policyVersion>"
    "<serialNumber>10</serialNumber>"
    "<acls>"
    "<acl>"
    "<peers>"
    "<peer>"
    "<type>ALL</type>"
    "</peer>"
    "</peers>"
    "</acl>"
    "</acls>"
    "</policy>";