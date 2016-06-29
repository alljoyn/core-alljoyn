/**
 * @file
 * XmlRulesValidator is a class handling validation of manifests and manifest templates
 * in XML format.
 */

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

#include <alljoyn/Status.h>
#include <qcc/StringSource.h>
#include <qcc/StringUtil.h>
#include "KeyInfoHelper.h"
#include "XmlRulesValidator.h"

using namespace qcc;
using namespace std;

namespace ajn {

XmlRulesValidator* XmlRulesValidator::s_validator;
unordered_map<string, PermissionPolicy::Rule::Member::MemberType> XmlRulesValidator::s_memberTypeMap;
unordered_map<string, uint8_t> XmlRulesValidator::MethodsValidator::s_actionsMap;
unordered_map<string, uint8_t> XmlRulesValidator::PropertiesValidator::s_actionsMap;
unordered_map<string, uint8_t> XmlRulesValidator::SignalsValidator::s_actionsMap;

#ifdef REGEX_SUPPORTED
const regex XmlRulesValidator::s_objectPathRegex = regex("(\\*|/(\\*)?|(/[a-zA-Z0-9_]+)+(/?\\*)?)");
const regex XmlRulesValidator::s_interfaceNameRegex = regex("(\\*|[a-zA-Z_][a-zA-Z0-9_]*((\\.[a-zA-Z_][a-zA-Z0-9_]*)*(\\.?\\*)|(\\.[a-zA-Z_][a-zA-Z0-9_]*)+))");
const regex XmlRulesValidator::s_memberNameRegex = regex("(\\*|([a-zA-Z_][a-zA-Z0-9_]*)(\\*)?)");
#endif /* REGEX_SUPPORTED */

void XmlRulesValidator::Init()
{
    s_validator = new XmlRulesValidator();

    MemberTypeMapInit();
    MethodsValidator::Init();
    SignalsValidator::Init();
    PropertiesValidator::Init();
}

void XmlRulesValidator::Shutdown()
{
    delete s_validator;
}

XmlRulesValidator* XmlRulesValidator::GetInstance()
{
    return s_validator;
}

void XmlRulesValidator::MemberTypeMapInit()
{
    s_memberTypeMap[METHOD_MEMBER_TYPE] = PermissionPolicy::Rule::Member::MemberType::METHOD_CALL;
    s_memberTypeMap[PROPERTY_MEMBER_TYPE] = PermissionPolicy::Rule::Member::MemberType::PROPERTY;
    s_memberTypeMap[SIGNAL_MEMBER_TYPE] = PermissionPolicy::Rule::Member::MemberType::SIGNAL;
    s_memberTypeMap[NOT_SPECIFIED_MEMBER_TYPE] = PermissionPolicy::Rule::Member::MemberType::NOT_SPECIFIED;
}

string XmlRulesValidator::GetRootElementName()
{
    return RULES_XML_ELEMENT;
}

void XmlRulesValidator::MethodsValidator::Init()
{
    s_actionsMap[DENY_MEMBER_MASK] = 0;
    s_actionsMap[MODIFY_MEMBER_MASK] = PermissionPolicy::Rule::Member::ACTION_MODIFY;
    s_actionsMap[PROVIDE_MEMBER_MASK] = PermissionPolicy::Rule::Member::ACTION_PROVIDE;
}

void XmlRulesValidator::PropertiesValidator::Init()
{
    s_actionsMap[DENY_MEMBER_MASK] = 0;
    s_actionsMap[MODIFY_MEMBER_MASK] = PermissionPolicy::Rule::Member::ACTION_MODIFY;
    s_actionsMap[PROVIDE_MEMBER_MASK] = PermissionPolicy::Rule::Member::ACTION_PROVIDE;
    s_actionsMap[OBSERVE_MEMBER_MASK] = PermissionPolicy::Rule::Member::ACTION_OBSERVE;
}

void XmlRulesValidator::SignalsValidator::Init()
{
    s_actionsMap[DENY_MEMBER_MASK] = 0;
    s_actionsMap[PROVIDE_MEMBER_MASK] = PermissionPolicy::Rule::Member::ACTION_PROVIDE;
    s_actionsMap[OBSERVE_MEMBER_MASK] = PermissionPolicy::Rule::Member::ACTION_OBSERVE;
}

QStatus XmlRulesValidator::Validate(const XmlElement* rootElement)
{
    QCC_ASSERT(nullptr != rootElement);

    unordered_set<string> nodeNames;
    string rootElementName = GetRootElementName();
    QStatus status = ValidateElementName(rootElement, rootElementName.c_str());

    if (ER_OK == status) {
        // For no children we will skip the loop and a valid XML has at least one child.
        status = ER_XML_MALFORMED;

        for (auto node : rootElement->GetChildren()) {
            status = ValidateNode(node, nodeNames);

            if (ER_OK != status) {
                break;
            }
        }
    }

    return (ER_OK == status) ? ER_OK : ER_XML_MALFORMED;
}

QStatus XmlRulesValidator::ValidateRules(const PermissionPolicy::Rule* rules, const size_t rulesCount)
{
    QCC_ASSERT(nullptr != rules);

    QStatus status = (rulesCount > 0) ? ER_OK : ER_FAIL;

    if (ER_OK == status) {
        map<string, vector<PermissionPolicy::Rule> > objectToRulesMap;
        AssignRulesToObjects(rules, rulesCount, objectToRulesMap);

        status = ValidateObject(objectToRulesMap);
    }

    return (ER_OK == status) ? ER_OK : ER_FAIL;
}

void XmlRulesValidator::AssignRulesToObjects(const PermissionPolicy::Rule* rules, const size_t rulesCount, map<string, vector<PermissionPolicy::Rule> >& objectToRulesMap)
{
    for (size_t index = 0; index < rulesCount; index++) {
        objectToRulesMap[rules[index].GetObjPath().c_str()].push_back(rules[index]);
    }
}

QStatus XmlRulesValidator::ValidateObject(const map<string, vector<PermissionPolicy::Rule> >& objectToRulesMap)
{
    QStatus status = ER_OK;
    for (auto rulesUnderObject : objectToRulesMap) {
        status = ValidateRules(rulesUnderObject.second);

        if (ER_OK != status) {
            break;
        }
    }

    return status;
}

QStatus XmlRulesValidator::ValidateRules(const vector<PermissionPolicy::Rule>& rules)
{
    QStatus status = ER_OK;
    unordered_set<string> interfaceNames;

    for (auto rule : rules) {
        status = ValidateRule(rule, interfaceNames);

        if (ER_OK != status) {
            break;
        }
    }

    return status;
}

QStatus XmlRulesValidator::ValidateRule(const PermissionPolicy::Rule& rule, unordered_set<string>& interfaceNames)
{
    QStatus status = ER_OK;

#ifdef REGEX_SUPPORTED
    status = ValidateString(rule.GetObjPath().c_str(), s_objectPathRegex);

    if (ER_OK == status) {
        status = ValidateString(rule.GetInterfaceName().c_str(), s_interfaceNameRegex, MAX_INTERFACE_NAME_LENGTH);
    }
#endif /* REGEX_SUPPORTED */

    if (ER_OK == status) {
        status = InsertUniqueOrFail(rule.GetInterfaceName().c_str(), interfaceNames);
    }

    if (ER_OK == status) {
        status = ValidateMembers(rule);
    }

    return status;
}

QStatus XmlRulesValidator::ValidateMembers(const PermissionPolicy::Rule& rule)
{
    MemberValidatorFactory memberValidatorFactory;
    size_t membersSize = rule.GetMembersSize();
    const PermissionPolicy::Rule::Member* members = rule.GetMembers();

    // There must be at least one member.
    QStatus status = ER_FAIL;

    for (size_t index = 0; index < membersSize; index++) {
        const PermissionPolicy::Rule::Member& member = members[index];
        status = memberValidatorFactory.ForType(member.GetMemberType())->Validate(member);

        if (ER_OK != status) {
            break;
        }
    }

    return status;
}

QStatus XmlRulesValidator::ValidateNode(const XmlElement* node, unordered_set<string>& nodeNames)
{
    vector<XmlElement*> annotations;
    vector<XmlElement*> interfaces;
    QStatus status = ValidateNodeCommon(node, nodeNames);

    SeparateAnnotations(node, annotations, interfaces);

    if (ER_OK == status) {
        status = ValidateNodeAnnotations(annotations);
    }

    if (ER_OK == status) {
        status = ValidateInterfaces(interfaces);
    }

    return status;
}

QStatus XmlRulesValidator::ValidateNodeCommon(const XmlElement* node, unordered_set<string>& nodeNames)
{
    QStatus status = ValidateElementName(node, NODE_XML_ELEMENT);

    if (ER_OK == status) {
        status = ValidateAttributeValueUnique(node, nodeNames, NAME_XML_ATTRIBUTE);
    }

    if (ER_OK == status) {
        status = ValidateChildrenCountPositive(node);
    }

#ifdef REGEX_SUPPORTED
    if (ER_OK == status) {
        status = XmlValidator::ValidateNameAttributeValue(node, s_objectPathRegex);
    }
#endif /* REGEX_SUPPORTED */

    return status;
}

QStatus XmlRulesValidator::ValidateNodeAnnotations(const vector<XmlElement*>& annotations)
{
    return (annotations.size() == 0) ? ER_OK : ER_XML_MALFORMED;
}

QStatus XmlRulesValidator::ValidateInterfaces(const vector<XmlElement*>& interfaces)
{
    unordered_set<string> interfaceNames;

    // There must be at least one interface.
    QStatus status = ER_XML_MALFORMED;

    for (auto singleInterface : interfaces) {
        status = ValidateInterface(singleInterface, interfaceNames);

        if (ER_OK != status) {
            break;
        }
    }

    return status;
}

QStatus XmlRulesValidator::ValidateInterface(const XmlElement* singleInterface, unordered_set<string>& interfaceNames)
{
    vector<XmlElement*> annotations;
    vector<XmlElement*> members;
    QStatus status = ValidateInterfaceCommon(singleInterface, interfaceNames);

    SeparateAnnotations(singleInterface, annotations, members);

    if (ER_OK == status) {
        status = ValidateInterfaceAnnotations(annotations);
    }

    if (ER_OK == status) {
        status = ValidateMembers(members);
    }

    return status;
}

QStatus XmlRulesValidator::ValidateInterfaceCommon(const XmlElement* singleInterface, unordered_set<string>& interfaceNames)
{
    QStatus status = ValidateElementName(singleInterface, INTERFACE_XML_ELEMENT);

    if (ER_OK == status) {
        status = ValidateAttributeValueUnique(singleInterface, interfaceNames, NAME_XML_ATTRIBUTE);
    }

#ifdef REGEX_SUPPORTED
    if (ER_OK == status) {
        status = XmlValidator::ValidateNameAttributeValue(singleInterface, s_interfaceNameRegex, MAX_INTERFACE_NAME_LENGTH);
    }
#endif /* REGEX_SUPPORTED */

    if (ER_OK == status) {
        status = ValidateChildrenCountPositive(singleInterface);
    }

    return status;
}

QStatus XmlRulesValidator::ValidateInterfaceAnnotations(const vector<XmlElement*>& annotations)
{
    return (annotations.size() == 0) ? ER_OK : ER_XML_MALFORMED;
}

QStatus XmlRulesValidator::ValidateMembers(const vector<XmlElement*>& members)
{
    MemberValidatorFactory memberValidatorFactory;

    // There must be at least one member.
    QStatus status = ER_XML_MALFORMED;

    for (auto member : members) {
        status = ValidateMember(member, memberValidatorFactory);

        if (ER_OK != status) {
            break;
        }
    }

    return status;
}

QStatus XmlRulesValidator::ValidateMember(const XmlElement* member, MemberValidatorFactory& memberValidatorFactory)
{
    PermissionPolicy::Rule::Member::MemberType type;

    QStatus status = MemberValidator::GetValidMemberType(member, &type);
    if (ER_OK == status) {
        status = memberValidatorFactory.ForType(type)->Validate(member);
    }

    return status;
}

QStatus XmlRulesValidator::MemberValidator::Validate(const XmlElement* member)
{
    QStatus status = ValidateAttributeValueUnique(member, memberNames, NAME_XML_ATTRIBUTE);

#ifdef REGEX_SUPPORTED
    if (ER_OK == status) {
        status = XmlValidator::ValidateNameAttributeValue(member, s_memberNameRegex, MAX_MEMBER_NAME_LENGTH);
    }
#endif /* REGEX_SUPPORTED */

    if (ER_OK == status) {
        status = ValidateMemberAnnotations(member);
    }

    return status;
}

QStatus XmlRulesValidator::MemberValidator::Validate(const PermissionPolicy::Rule::Member& member)
{
    QStatus status = ValidateMemberName(member.GetMemberName().c_str());

    if (ER_OK == status) {
        status = ValidateActionMask(member.GetActionMask());
    }

    return status;
}

QStatus XmlRulesValidator::MemberValidator::ValidateMemberName(AJ_PCSTR name)
{
    QStatus status = ER_OK;

#ifdef REGEX_SUPPORTED
    status = ValidateString(name, s_memberNameRegex, MAX_MEMBER_NAME_LENGTH);
#endif /* REGEX_SUPPORTED */

    if (ER_OK == status) {
        status = InsertUniqueOrFail(name, memberNames);
    }

    return status;
}

QStatus XmlRulesValidator::MemberValidator::ValidateActionMask(uint8_t actionMask)
{
    uint8_t validActions = GetValidActions();
    return ((validActions | actionMask) == validActions) ? ER_OK : ER_FAIL;
}

QStatus XmlRulesValidator::MemberValidator::ValidateMemberAnnotations(const XmlElement* member)
{
    unordered_set<string> presentAnnotations;
    QStatus status = ValidateChildrenCountPositive(member);

    for (auto annotation : member->GetChildren()) {
        status = ValidateAnnotation(annotation, presentAnnotations);

        if (ER_OK != status) {
            break;
        }
    }

    return status;
}

QStatus XmlRulesValidator::MemberValidator::ValidateAnnotation(XmlElement* annotation, unordered_set<string>& presentAnnotations)
{
    QStatus status = ValidateElementName(annotation, ANNOTATION_XML_ELEMENT);

    if (ER_OK == status) {
        status = ValidateNameAttributeValue(annotation, ACTION_ANNOTATION_NAME);
    }

    if (ER_OK == status) {
        status = ValidateAttributeValueUnique(annotation, presentAnnotations, VALUE_XML_ATTRIBUTE);
    }

    if (ER_OK == status) {
        status = ValidateAnnotationAllowed(annotation, presentAnnotations);
    }

    return status;
}

QStatus XmlRulesValidator::MemberValidator::ValidateAnnotationAllowed(XmlElement* annotation, unordered_set<string>& presentAnnotations)
{
    QStatus status = ValidateAnnotationAllowedForMember(annotation);

    if (ER_OK == status) {
        status = ValidateDenyAnnotation(presentAnnotations);
    }

    return status;
}

QStatus XmlRulesValidator::MemberValidator::ValidateAnnotationAllowedForMember(XmlElement* annotation)
{
    String action = annotation->GetAttribute(VALUE_XML_ATTRIBUTE);
    auto foundAction = GetActionsMap().find(action.c_str());

    return (foundAction == GetActionsMap().end()) ? ER_XML_MALFORMED : ER_OK;
}

QStatus XmlRulesValidator::MemberValidator::ValidateDenyAnnotation(unordered_set<string>& presentAnnotations)
{
    bool denyAbsent = (presentAnnotations.find(DENY_MEMBER_MASK) == presentAnnotations.end());

    return (denyAbsent ||
            presentAnnotations.size() == 1U) ? ER_OK : ER_XML_MALFORMED;
}

QStatus XmlRulesValidator::MemberValidator::GetValidMemberType(const XmlElement* member, PermissionPolicy::Rule::Member::MemberType* memberType)
{
    QStatus status = ER_OK;
    auto foundType = s_memberTypeMap.find(member->GetName().c_str());
    if (foundType != s_memberTypeMap.end()) {
        *memberType = foundType->second;
    } else {
        status = ER_XML_MALFORMED;
    }

    return status;
}

const unordered_map<string, uint8_t>& XmlRulesValidator::MethodsValidator::GetActionsMap()
{
    return s_actionsMap;
}

uint8_t XmlRulesValidator::MethodsValidator::GetValidActions()
{
    return PermissionPolicy::Rule::Member::ACTION_PROVIDE |
           PermissionPolicy::Rule::Member::ACTION_MODIFY;
}

const unordered_map<string, uint8_t>& XmlRulesValidator::PropertiesValidator::GetActionsMap()
{
    return s_actionsMap;
}

uint8_t XmlRulesValidator::PropertiesValidator::GetValidActions()
{
    return PermissionPolicy::Rule::Member::ACTION_OBSERVE |
           PermissionPolicy::Rule::Member::ACTION_PROVIDE |
           PermissionPolicy::Rule::Member::ACTION_MODIFY;
}

const unordered_map<string, uint8_t>& XmlRulesValidator::SignalsValidator::GetActionsMap()
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