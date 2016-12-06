/**
 * @file
 * XmlRulesValidator is a class handling validation of manifests and manifest templates
 * in XML format.
 */

/******************************************************************************
 *  * Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
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
 *     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *     PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include <alljoyn/Status.h>
#include <qcc/StringSource.h>
#include <qcc/StringUtil.h>
#include "KeyInfoHelper.h"
#include "XmlRulesValidator.h"

#define QCC_MODULE "XML_CONVERTER"

using namespace qcc;
using namespace std;

namespace ajn {

XmlRulesValidator* XmlRulesValidator::s_validator = nullptr;
unordered_map<string, PermissionPolicy::Rule::Member::MemberType>* XmlRulesValidator::s_memberTypeMap = nullptr;
unordered_map<string, uint8_t>* XmlRulesValidator::MethodsValidator::s_actionsMap = nullptr;
unordered_map<string, uint8_t>* XmlRulesValidator::PropertiesValidator::s_actionsMap = nullptr;
unordered_map<string, uint8_t>* XmlRulesValidator::SignalsValidator::s_actionsMap = nullptr;

#ifdef REGEX_SUPPORTED
const regex XmlRulesValidator::s_objectPathRegex = regex("(\\*|/(\\*)?|(/[a-zA-Z0-9_]+)+(/?\\*)?)");
const regex XmlRulesValidator::s_interfaceNameRegex = regex("(\\*|[a-zA-Z_][a-zA-Z0-9_]*((\\.[a-zA-Z_][a-zA-Z0-9_]*)*(\\.?\\*)|(\\.[a-zA-Z_][a-zA-Z0-9_]*)+))");
const regex XmlRulesValidator::s_memberNameRegex = regex("(\\*|([a-zA-Z_][a-zA-Z0-9_]*)(\\*)?)");
#endif /* REGEX_SUPPORTED */

void XmlRulesValidator::Init()
{
    QCC_DbgTrace(("%s: Performing validator init.", __FUNCTION__));

    QCC_ASSERT((nullptr == s_validator) && (nullptr == s_memberTypeMap));
    s_validator = new XmlRulesValidator();

    MemberTypeMapInit();
    MethodsValidator::Init();
    SignalsValidator::Init();
    PropertiesValidator::Init();
}

void XmlRulesValidator::Shutdown()
{
    QCC_DbgTrace(("%s: Performing validator cleanup.", __FUNCTION__));

    delete s_validator;
    s_validator = nullptr;

    delete s_memberTypeMap;
    s_memberTypeMap = nullptr;
}

XmlRulesValidator* XmlRulesValidator::GetInstance()
{
    return s_validator;
}

void XmlRulesValidator::MemberTypeMapInit()
{
    QCC_DbgTrace(("%s: Performing s_memberTypeMap init.", __FUNCTION__));

    s_memberTypeMap = new unordered_map<string, PermissionPolicy::Rule::Member::MemberType>();

    (*s_memberTypeMap)[METHOD_MEMBER_TYPE] = PermissionPolicy::Rule::Member::MemberType::METHOD_CALL;
    (*s_memberTypeMap)[PROPERTY_MEMBER_TYPE] = PermissionPolicy::Rule::Member::MemberType::PROPERTY;
    (*s_memberTypeMap)[SIGNAL_MEMBER_TYPE] = PermissionPolicy::Rule::Member::MemberType::SIGNAL;
    (*s_memberTypeMap)[NOT_SPECIFIED_MEMBER_TYPE] = PermissionPolicy::Rule::Member::MemberType::NOT_SPECIFIED;
}

string XmlRulesValidator::GetRootElementName()
{
    return RULES_XML_ELEMENT;
}

void XmlRulesValidator::MethodsValidator::Init()
{
    QCC_DbgTrace(("%s: Performing validator's subclass init.", __FUNCTION__));

    QCC_ASSERT(nullptr == s_actionsMap);

    s_actionsMap = new unordered_map<string, uint8_t>();

    (*s_actionsMap)[DENY_MEMBER_MASK] = 0;
    (*s_actionsMap)[MODIFY_MEMBER_MASK] = PermissionPolicy::Rule::Member::ACTION_MODIFY;
    (*s_actionsMap)[PROVIDE_MEMBER_MASK] = PermissionPolicy::Rule::Member::ACTION_PROVIDE;
}

void XmlRulesValidator::MethodsValidator::Shutdown()
{
    QCC_DbgTrace(("%s: Performing validator's subclass cleanup.", __FUNCTION__));

    delete s_actionsMap;
    s_actionsMap = nullptr;
}

void XmlRulesValidator::PropertiesValidator::Init()
{
    QCC_DbgTrace(("%s: Performing validator's subclass init.", __FUNCTION__));

    QCC_ASSERT(nullptr == s_actionsMap);

    s_actionsMap = new unordered_map<string, uint8_t>();

    (*s_actionsMap)[DENY_MEMBER_MASK] = 0;
    (*s_actionsMap)[MODIFY_MEMBER_MASK] = PermissionPolicy::Rule::Member::ACTION_MODIFY;
    (*s_actionsMap)[PROVIDE_MEMBER_MASK] = PermissionPolicy::Rule::Member::ACTION_PROVIDE;
    (*s_actionsMap)[OBSERVE_MEMBER_MASK] = PermissionPolicy::Rule::Member::ACTION_OBSERVE;
}

void XmlRulesValidator::PropertiesValidator::Shutdown()
{
    QCC_DbgTrace(("%s: Performing validator's subclass cleanup.", __FUNCTION__));

    delete s_actionsMap;
    s_actionsMap = nullptr;
}

void XmlRulesValidator::SignalsValidator::Init()
{
    QCC_DbgTrace(("%s: Performing validator's subclass init.", __FUNCTION__));

    QCC_ASSERT(nullptr == s_actionsMap);

    s_actionsMap = new unordered_map<string, uint8_t>();

    (*s_actionsMap)[DENY_MEMBER_MASK] = 0;
    (*s_actionsMap)[PROVIDE_MEMBER_MASK] = PermissionPolicy::Rule::Member::ACTION_PROVIDE;
    (*s_actionsMap)[OBSERVE_MEMBER_MASK] = PermissionPolicy::Rule::Member::ACTION_OBSERVE;
}

void XmlRulesValidator::SignalsValidator::Shutdown()
{
    QCC_DbgTrace(("%s: Performing validator's subclass cleanup.", __FUNCTION__));

    delete s_actionsMap;
    s_actionsMap = nullptr;
}

QStatus XmlRulesValidator::Validate(const XmlElement* rootElement)
{
    QCC_ASSERT(nullptr != rootElement);
    QCC_DbgHLPrintf(("%s: Validating rules XML: %s", __FUNCTION__, rootElement->Generate().c_str()));

    unordered_set<string> nodeNames;
    string rootElementName = GetRootElementName();
    QStatus status = ValidateElementName(rootElement, rootElementName.c_str());
    if (ER_OK != status) {
        return status;
    }

    // For no children we will skip the loop and a valid XML has at least one child.
    status = ER_XML_INVALID_ELEMENT_CHILDREN_COUNT;
    for (auto node : rootElement->GetChildren()) {
        status = ValidateNode(node, nodeNames);
        if (ER_OK != status) {
            return status;
        }
    }

    if (ER_OK != status) {
        QCC_LogError(status, ("%s: There must be at least one 'node' element.", __FUNCTION__));
    }

    return status;
}

QStatus XmlRulesValidator::ValidateRules(const PermissionPolicy::Rule* rules, const size_t rulesCount)
{
    QCC_ASSERT(nullptr != rules);
    QCC_DbgHLPrintf(("%s: Validating %u rule objects.", __FUNCTION__, rulesCount));

    if (0 == rulesCount) {
        QCC_LogError(ER_XML_INVALID_RULES_COUNT, ("%s: There must be at least one rule object.", __FUNCTION__));

        return ER_XML_INVALID_RULES_COUNT;
    }

    map<string, vector<PermissionPolicy::Rule> > objectToRulesMap;
    AssignRulesToObjects(rules, rulesCount, objectToRulesMap);

    return ValidateObject(objectToRulesMap);
}

void XmlRulesValidator::AssignRulesToObjects(const PermissionPolicy::Rule* rules, const size_t rulesCount, map<string, vector<PermissionPolicy::Rule> >& objectToRulesMap)
{
    for (size_t index = 0; index < rulesCount; index++) {
        objectToRulesMap[rules[index].GetObjPath().c_str()].push_back(rules[index]);
    }
}

QStatus XmlRulesValidator::ValidateObject(const map<string, vector<PermissionPolicy::Rule> >& objectToRulesMap)
{
    for (auto rulesUnderObject : objectToRulesMap) {
        QStatus status = ValidateRules(rulesUnderObject.second);
        if (ER_OK != status) {
            return status;
        }
    }

    return ER_OK;
}

QStatus XmlRulesValidator::ValidateRules(const vector<PermissionPolicy::Rule>& rules)
{
    unordered_set<string> interfaceNames;

    for (auto rule : rules) {
        QStatus status = ValidateRule(rule, interfaceNames);
        if (ER_OK != status) {
            return status;
        }
    }

    return ER_OK;
}

QStatus XmlRulesValidator::ValidateRule(const PermissionPolicy::Rule& rule, unordered_set<string>& interfaceNames)
{
    QStatus status;

#ifdef REGEX_SUPPORTED
    status = ValidateString(rule.GetObjPath().c_str(), s_objectPathRegex);
    if (ER_OK != status) {
        QCC_LogError(ER_XML_INVALID_OBJECT_PATH,
                     ("%s: Invalid object path: %s.",
                      __FUNCTION__, rule.GetObjPath().c_str()));

        return ER_XML_INVALID_OBJECT_PATH;
    }

    status = ValidateString(rule.GetInterfaceName().c_str(), s_interfaceNameRegex, MAX_INTERFACE_NAME_LENGTH);
    if (ER_OK != status) {
        QCC_LogError(ER_XML_INVALID_INTERFACE_NAME,
                     ("%s: Invalid interface name: %s.",
                      __FUNCTION__, rule.GetInterfaceName().c_str()));

        return ER_XML_INVALID_INTERFACE_NAME;
    }
#endif /* REGEX_SUPPORTED */

    status = InsertUniqueOrFail(rule.GetInterfaceName().c_str(), interfaceNames);
    if (ER_OK != status) {
        QCC_LogError(ER_XML_INTERFACE_NAME_NOT_UNIQUE,
                     ("%s: An interface with the same name already exists: %s.",
                      __FUNCTION__, rule.GetInterfaceName().c_str()));

        return ER_XML_INTERFACE_NAME_NOT_UNIQUE;
    }

    return ValidateMembers(rule);
}

QStatus XmlRulesValidator::ValidateMembers(const PermissionPolicy::Rule& rule)
{
    MemberValidatorFactory memberValidatorFactory;
    size_t membersSize = rule.GetMembersSize();
    const PermissionPolicy::Rule::Member* members = rule.GetMembers();

    if (0 == membersSize) {
        QCC_LogError(ER_XML_INTERFACE_MEMBERS_MISSING, ("%s: There must be at least one member object.", __FUNCTION__));

        return ER_XML_INTERFACE_MEMBERS_MISSING;
    }

    for (size_t index = 0; index < membersSize; index++) {
        const PermissionPolicy::Rule::Member& member = members[index];
        QStatus status = memberValidatorFactory.ForType(member.GetMemberType())->Validate(member);
        if (ER_OK != status) {
            return status;
        }
    }

    return ER_OK;
}

QStatus XmlRulesValidator::ValidateNode(const XmlElement* node, unordered_set<string>& nodeNames)
{
    vector<XmlElement*> annotations;
    vector<XmlElement*> interfaces;
    QStatus status = ValidateNodeCommon(node, nodeNames);
    if (ER_OK != status) {
        return status;
    }

    SeparateAnnotations(node, annotations, interfaces);
    status = ValidateNodeAnnotations(annotations);
    if (ER_OK != status) {
        return status;
    }

    return ValidateInterfaces(interfaces);
}

QStatus XmlRulesValidator::ValidateNodeCommon(const XmlElement* node, unordered_set<string>& nodeNames)
{
    QStatus status = ValidateElementName(node, NODE_XML_ELEMENT);
    if (ER_OK != status) {
        return status;
    }

    status = ValidateNodeNameUnique(node, nodeNames);
    if (ER_OK != status) {
        return status;
    }

    status = ValidateChildrenCountPositive(node);
    if (ER_OK != status) {
        return status;
    }

#ifdef REGEX_SUPPORTED
    status = ValidateNameAttributeValue(node, s_objectPathRegex);
    if (ER_OK != status) {
        return ER_XML_INVALID_OBJECT_PATH;
    }
#endif /* REGEX_SUPPORTED */

    return ER_OK;
}

QStatus XmlRulesValidator::ValidateNodeNameUnique(const XmlElement* node, unordered_set<string>& nodeNames)
{
    QStatus status = ValidateAttributeValueUnique(node, nodeNames, NAME_XML_ATTRIBUTE);

    if (ER_OK != status) {
        QCC_LogError(ER_XML_OBJECT_PATH_NOT_UNIQUE, ("%s: An object with the same path already exists.", __FUNCTION__));

        return ER_XML_OBJECT_PATH_NOT_UNIQUE;
    }

    return ER_OK;
}

QStatus XmlRulesValidator::ValidateNodeAnnotations(const vector<XmlElement*>& annotations)
{
    if (!annotations.empty()) {
        QCC_LogError(ER_XML_INVALID_ANNOTATIONS_COUNT,
                     ("%s: Policy or signed manifest rules' nodes should not contain any annotations.",
                      __FUNCTION__));

        return ER_XML_INVALID_ANNOTATIONS_COUNT;
    }

    return ER_OK;
}

QStatus XmlRulesValidator::ValidateInterfaces(const vector<XmlElement*>& interfaces)
{
    unordered_set<string> interfaceNames;

    // There must be at least one interface.
    QStatus status = ER_XML_INVALID_ELEMENT_CHILDREN_COUNT;
    for (auto singleInterface : interfaces) {
        status = ValidateInterface(singleInterface, interfaceNames);
        if (ER_OK != status) {
            return status;
        }
    }

    if (ER_OK != status) {
        QCC_LogError(status, ("%s: Node must have at least one 'interface' element.", __FUNCTION__));
    }

    return status;
}

QStatus XmlRulesValidator::ValidateInterface(const XmlElement* singleInterface, unordered_set<string>& interfaceNames)
{
    vector<XmlElement*> annotations;
    vector<XmlElement*> members;
    QStatus status = ValidateInterfaceCommon(singleInterface, interfaceNames);
    if (ER_OK != status) {
        return status;
    }

    SeparateAnnotations(singleInterface, annotations, members);
    status = ValidateInterfaceAnnotations(annotations);
    if (ER_OK != status) {
        return status;
    }

    return ValidateMembers(members);
}

QStatus XmlRulesValidator::ValidateInterfaceCommon(const XmlElement* singleInterface, unordered_set<string>& interfaceNames)
{
    QStatus status = ValidateElementName(singleInterface, INTERFACE_XML_ELEMENT);
    if (ER_OK != status) {
        return status;
    }

    status = ValidateInterfaceNameUnique(singleInterface, interfaceNames);
    if (ER_OK != status) {
        return status;
    }

#ifdef REGEX_SUPPORTED
    status = XmlValidator::ValidateNameAttributeValue(singleInterface, s_interfaceNameRegex, MAX_INTERFACE_NAME_LENGTH);
    if (ER_OK != status) {
        return ER_XML_INVALID_INTERFACE_NAME;
    }
#endif /* REGEX_SUPPORTED */

    return ValidateChildrenCountPositive(singleInterface);
}

QStatus XmlRulesValidator::ValidateInterfaceNameUnique(const XmlElement* singleInterface, unordered_set<string>& interfaceNames)
{
    QStatus status = ValidateAttributeValueUnique(singleInterface, interfaceNames, NAME_XML_ATTRIBUTE);

    if (ER_OK != status) {
        QCC_LogError(ER_XML_INTERFACE_NAME_NOT_UNIQUE, ("%s: An interface with the same name already exists in this object.", __FUNCTION__));

        return ER_XML_INTERFACE_NAME_NOT_UNIQUE;
    }

    return ER_OK;
}

QStatus XmlRulesValidator::ValidateInterfaceAnnotations(const vector<XmlElement*>& annotations)
{
    if (!annotations.empty()) {
        QCC_LogError(ER_XML_INVALID_ANNOTATIONS_COUNT,
                     ("%s: Policy or signed manifest rules' interfaces should not contain any annotations.",
                      __FUNCTION__));

        return ER_XML_INVALID_ANNOTATIONS_COUNT;
    }

    return ER_OK;
}

QStatus XmlRulesValidator::ValidateMembers(const vector<XmlElement*>& members)
{
    MemberValidatorFactory memberValidatorFactory;

    // There must be at least one member.
    QStatus status = ER_XML_INVALID_ELEMENT_CHILDREN_COUNT;
    for (auto member : members) {
        status = ValidateMember(member, memberValidatorFactory);
        if (ER_OK != status) {
            return status;
        }
    }

    if (ER_OK != status) {
        QCC_LogError(status, ("%s: Interface must have at least one member.", __FUNCTION__));
    }

    return status;
}

QStatus XmlRulesValidator::ValidateMember(const XmlElement* member, MemberValidatorFactory& memberValidatorFactory)
{
    PermissionPolicy::Rule::Member::MemberType type;

    QStatus status = MemberValidator::GetValidMemberType(member, &type);
    if (ER_OK != status) {
        return status;
    }

    return memberValidatorFactory.ForType(type)->Validate(member);
}

QStatus XmlRulesValidator::MemberValidator::Validate(const XmlElement* member)
{
    QStatus status = ValidateMemberNameUnique(member, memberNames);
    if (ER_OK != status) {
        return status;
    }

#ifdef REGEX_SUPPORTED
    status = XmlValidator::ValidateNameAttributeValue(member, s_memberNameRegex, MAX_MEMBER_NAME_LENGTH);
    if (ER_OK != status) {
        return ER_XML_INVALID_MEMBER_NAME;
    }
#endif /* REGEX_SUPPORTED */

    return ValidateMemberAnnotations(member);
}

QStatus XmlRulesValidator::MemberValidator::Validate(const PermissionPolicy::Rule::Member& member)
{
    QStatus status = ValidateMemberName(member.GetMemberName().c_str());
    if (ER_OK != status) {
        return status;
    }

    return ValidateActionMask(member.GetActionMask());
}

QStatus XmlRulesValidator::MemberValidator::ValidateMemberName(AJ_PCSTR name)
{
    QStatus status;
#ifdef REGEX_SUPPORTED
    status = ValidateString(name, s_memberNameRegex, MAX_MEMBER_NAME_LENGTH);
    if (ER_OK != status) {
        QCC_LogError(ER_XML_INVALID_MEMBER_NAME,
                     ("%s: Invalid member name: %s.",
                      __FUNCTION__, name));

        return ER_XML_INVALID_MEMBER_NAME;
    }
#endif /* REGEX_SUPPORTED */

    status = InsertUniqueOrFail(name, memberNames);
    if (ER_OK != status) {
        QCC_LogError(ER_XML_MEMBER_NAME_NOT_UNIQUE,
                     ("%s: A member with the same name already exists: %s.",
                      __FUNCTION__, name));

        return ER_XML_MEMBER_NAME_NOT_UNIQUE;
    }

    return ER_OK;
}

QStatus XmlRulesValidator::MemberValidator::ValidateActionMask(uint8_t actionMask)
{
    uint8_t validActions = GetValidActions();
    if ((validActions | actionMask) != validActions) {
        QCC_LogError(ER_XML_INVALID_MEMBER_ACTION,
                     ("%s: Action mask %hhx not allowed for this member. Valid actions are: %hhx",
                      __FUNCTION__, actionMask, validActions));

        return ER_XML_INVALID_MEMBER_ACTION;
    }

    return ER_OK;
}

QStatus XmlRulesValidator::MemberValidator::ValidateMemberNameUnique(const XmlElement* member, unordered_set<string>& memberNames)
{
    QStatus status = ValidateAttributeValueUnique(member, memberNames, NAME_XML_ATTRIBUTE);

    if (ER_OK != status) {
        QCC_LogError(ER_XML_MEMBER_NAME_NOT_UNIQUE, ("%s: A member with the same name already exists in this interface.", __FUNCTION__));

        return ER_XML_MEMBER_NAME_NOT_UNIQUE;
    }

    return ER_OK;
}

QStatus XmlRulesValidator::MemberValidator::ValidateMemberAnnotations(const XmlElement* member)
{
    unordered_set<string> presentAnnotations;

    QStatus status = ER_XML_INVALID_ELEMENT_CHILDREN_COUNT;
    for (auto annotation : member->GetChildren()) {
        status = ValidateAnnotation(annotation, presentAnnotations);
        if (ER_OK != status) {
            return status;
        }
    }

    if (ER_OK != status) {
        QCC_LogError(status, ("%s: Member must have at least one annotation.", __FUNCTION__));
    }

    return status;
}

QStatus XmlRulesValidator::MemberValidator::ValidateAnnotationUnique(const XmlElement* annotation, unordered_set<string>& presentAnnotations)
{
    QStatus status = ValidateAttributeValueUnique(annotation, presentAnnotations, VALUE_XML_ATTRIBUTE);

    if (ER_OK != status) {
        QCC_LogError(ER_XML_ANNOTATION_NOT_UNIQUE, ("%s: The same annotation already exists in this XML element.", __FUNCTION__));

        return ER_XML_ANNOTATION_NOT_UNIQUE;
    }

    return ER_OK;
}

QStatus XmlRulesValidator::MemberValidator::ValidateAnnotation(XmlElement* annotation, unordered_set<string>& presentAnnotations)
{
    QStatus status = ValidateElementName(annotation, ANNOTATION_XML_ELEMENT);
    if (ER_OK != status) {
        return status;
    }

    status = ValidateNameAttributeValue(annotation, ACTION_ANNOTATION_NAME);
    if (ER_OK != status) {
        return status;
    }

    status = ValidateAnnotationUnique(annotation, presentAnnotations);
    if (ER_OK != status) {
        return status;
    }

    return ValidateAnnotationAllowed(annotation, presentAnnotations);
}

QStatus XmlRulesValidator::MemberValidator::ValidateAnnotationAllowed(XmlElement* annotation, unordered_set<string>& presentAnnotations)
{
    QStatus status = ValidateAnnotationAllowedForMember(annotation);
    if (ER_OK != status) {
        return status;
    }

    return ValidateDenyAnnotation(presentAnnotations);
}

QStatus XmlRulesValidator::MemberValidator::ValidateAnnotationAllowedForMember(XmlElement* annotation)
{
    String action = annotation->GetAttribute(VALUE_XML_ATTRIBUTE);
    auto foundAction = GetActionsMap()->find(action.c_str());

    if (foundAction == GetActionsMap()->end()) {
        QCC_LogError(ER_XML_INVALID_MEMBER_ACTION,
                     ("%s: Invalid action for this member: '%s'.",
                      __FUNCTION__, action.c_str()));

        return ER_XML_INVALID_MEMBER_ACTION;
    }

    return ER_OK;
}

QStatus XmlRulesValidator::MemberValidator::ValidateDenyAnnotation(unordered_set<string>& presentAnnotations)
{
    bool denyPresent = (presentAnnotations.find(DENY_MEMBER_MASK) != presentAnnotations.end());

    if (denyPresent && (presentAnnotations.size() > 1U)) {
        QCC_LogError(ER_XML_MEMBER_DENY_ACTION_WITH_OTHER,
                     ("%s: Member cannot have the 'Deny' action along with other ones.",
                      __FUNCTION__));

        return ER_XML_MEMBER_DENY_ACTION_WITH_OTHER;
    }

    return ER_OK;
}

QStatus XmlRulesValidator::MemberValidator::GetValidMemberType(const XmlElement* member, PermissionPolicy::Rule::Member::MemberType* memberType)
{
    auto foundType = s_memberTypeMap->find(member->GetName().c_str());
    if (foundType != s_memberTypeMap->end()) {
        *memberType = foundType->second;
    } else {
        QCC_LogError(ER_XML_INVALID_MEMBER_TYPE,
                     ("%s: Invalid member type '%s'.",
                      __FUNCTION__, member->GetName().c_str()));

        return ER_XML_INVALID_MEMBER_TYPE;
    }

    return ER_OK;
}

const unordered_map<string, uint8_t>* XmlRulesValidator::MethodsValidator::GetActionsMap()
{
    return s_actionsMap;
}

uint8_t XmlRulesValidator::MethodsValidator::GetValidActions()
{
    return PermissionPolicy::Rule::Member::ACTION_PROVIDE |
           PermissionPolicy::Rule::Member::ACTION_MODIFY;
}

const unordered_map<string, uint8_t>* XmlRulesValidator::PropertiesValidator::GetActionsMap()
{
    return s_actionsMap;
}

uint8_t XmlRulesValidator::PropertiesValidator::GetValidActions()
{
    return PermissionPolicy::Rule::Member::ACTION_OBSERVE |
           PermissionPolicy::Rule::Member::ACTION_PROVIDE |
           PermissionPolicy::Rule::Member::ACTION_MODIFY;
}

const unordered_map<string, uint8_t>* XmlRulesValidator::SignalsValidator::GetActionsMap()
{
    return s_actionsMap;
}

uint8_t XmlRulesValidator::SignalsValidator::GetValidActions()
{
    return PermissionPolicy::Rule::Member::ACTION_OBSERVE |
           PermissionPolicy::Rule::Member::ACTION_PROVIDE;
}

XmlRulesValidator::MemberValidator* XmlRulesValidator::MemberValidatorFactory::ForType(PermissionPolicy::Rule::Member::MemberType type)
{
    QCC_ASSERT(m_validators.find(type) != m_validators.end());
    return m_validators[type];
}
}