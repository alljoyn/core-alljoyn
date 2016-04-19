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

namespace ajn {

std::unordered_map<std::string, PermissionPolicy::Rule::Member::MemberType> XmlRulesValidator::s_memberTypeMap;
std::unordered_map<std::string, uint8_t> XmlRulesValidator::MethodsValidator::s_actionsMap;
std::unordered_map<std::string, uint8_t> XmlRulesValidator::PropertiesValidator::s_actionsMap;
std::unordered_map<std::string, uint8_t> XmlRulesValidator::SignalsValidator::s_actionsMap;

#ifdef REGEX_SUPPORTED
const std::regex XmlRulesValidator::s_objectPathRegex = std::regex("(\\*|/(\\*)?|(/[a-zA-Z0-9_]+)+(/\\*)?)");
const std::regex XmlRulesValidator::s_interfaceNameRegex = std::regex("(\\*|[a-zA-Z_][a-zA-Z0-9_]*((\\.[a-zA-Z_][a-zA-Z0-9_]*)*(\\.\\*)|(\\.[a-zA-Z_][a-zA-Z0-9_]*)+))");
const std::regex XmlRulesValidator::s_memberNameRegex = std::regex("(\\*|([a-zA-Z_][a-zA-Z0-9_]*)(\\*)?)");
#endif /* REGEX_SUPPORTED */

void XmlRulesValidator::Init()
{
    static bool initialized = false;

    if (!initialized) {
        MemberTypeMapInit();
        MethodsValidator::Init();
        SignalsValidator::Init();
        PropertiesValidator::Init();

        initialized = true;
    }
}

void XmlRulesValidator::MemberTypeMapInit()
{
    s_memberTypeMap[METHOD_MEMBER_TYPE] = PermissionPolicy::Rule::Member::MemberType::METHOD_CALL;
    s_memberTypeMap[PROPERTY_MEMBER_TYPE] = PermissionPolicy::Rule::Member::MemberType::PROPERTY;
    s_memberTypeMap[SIGNAL_MEMBER_TYPE] = PermissionPolicy::Rule::Member::MemberType::SIGNAL;
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

QStatus XmlRulesValidator::Validate(const qcc::XmlElement* rootElement)
{
    QCC_ASSERT(nullptr != rootElement);

    std::unordered_set<std::string> nodeNames;
    QStatus status = ValidateChildrenCountPositive(rootElement);

    for (auto node : rootElement->GetChildren()) {
        status = ValidateNode(node, nodeNames);

        if (ER_OK != status) {
            break;
        }
    }

    return (ER_OK == status) ? ER_OK : ER_XML_MALFORMED;
}

QStatus XmlRulesValidator::ValidateRules(const PermissionPolicy::Rule* rules, const size_t rulesCount)
{
    QCC_ASSERT(nullptr != rules);

    QStatus status = (rulesCount > 0) ? ER_OK : ER_FAIL;

    if (ER_OK == status) {
        std::map<std::string, std::vector<PermissionPolicy::Rule> > objectToRulesMap;
        AssignRulesToObjects(rules, rulesCount, objectToRulesMap);

        status = ValidateObject(objectToRulesMap);
    }

    return (ER_OK == status) ? ER_OK : ER_FAIL;
}

void XmlRulesValidator::AssignRulesToObjects(const PermissionPolicy::Rule* rules, const size_t rulesCount, std::map<std::string, std::vector<PermissionPolicy::Rule> >& objectToRulesMap)
{
    for (size_t index = 0; index < rulesCount; index++) {
        objectToRulesMap[rules[index].GetObjPath().c_str()].push_back(rules[index]);
    }
}

QStatus XmlRulesValidator::ValidateObject(const std::map<std::string, std::vector<PermissionPolicy::Rule> >& objectToRulesMap)
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

QStatus XmlRulesValidator::ValidateRules(const std::vector<PermissionPolicy::Rule>& rules)
{
    QStatus status = ER_OK;
    std::unordered_set<std::string> interfaceNames;

    for (auto rule : rules) {
        status = ValidateRule(rule, interfaceNames);

        if (ER_OK != status) {
            break;
        }
    }

    return status;
}

QStatus XmlRulesValidator::ValidateRule(const PermissionPolicy::Rule& rule, std::unordered_set<std::string>& interfaceNames)
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

    QStatus status = (membersSize > 0) ? ER_OK : ER_XML_MALFORMED;

    if (ER_OK == status) {
        for (size_t index = 0; index < membersSize; index++) {
            const PermissionPolicy::Rule::Member& member = members[index];
            status = memberValidatorFactory.ForType(member.GetMemberType())->Validate(member);

            if (ER_OK != status) {
                break;
            }
        }
    }

    return status;
}

void XmlRulesValidator::ExtractAttributeOrWildcard(const qcc::XmlElement* xmlElement, AJ_PCSTR attributeName, std::string* attribute)
{
    *attribute = xmlElement->GetAttribute(attributeName).c_str();
    if (attribute->empty()) {
        *attribute = WILDCARD_XML_VALUE;
    }
}

QStatus XmlRulesValidator::ValidateNode(const qcc::XmlElement* node, std::unordered_set<std::string>& nodeNames)
{
    QStatus status = ValidateElementName(node, NODE_XML_ELEMENT);

    if (ER_OK == status) {
        status = ValidateAttributeValueUnique(node, nodeNames);
    }

#ifdef REGEX_SUPPORTED
    if (ER_OK == status) {
        status = ValidateNameAttributeValue(node, s_objectPathRegex);
    }
#endif /* REGEX_SUPPORTED */

    if (ER_OK == status) {
        status = ValidateInterfaces(node);
    }

    return status;
}

QStatus XmlRulesValidator::ValidateInterfaces(const qcc::XmlElement* node)
{
    std::unordered_set<std::string> interfaceNames;
    QStatus status = ValidateChildrenCountPositive(node);

    for (auto singleInterface : node->GetChildren()) {
        status = ValidateInterface(singleInterface, interfaceNames);

        if (ER_OK != status) {
            break;
        }
    }

    return status;
}

QStatus XmlRulesValidator::ValidateInterface(const qcc::XmlElement* singleInterface, std::unordered_set<std::string>& interfaceNames)
{
    QStatus status = ValidateElementName(singleInterface, INTERFACE_XML_ELEMENT);

    if (ER_OK == status) {
        status = ValidateAttributeValueUnique(singleInterface, interfaceNames);
    }

#ifdef REGEX_SUPPORTED
    if (ER_OK == status) {
        status = ValidateNameAttributeValue(singleInterface, s_interfaceNameRegex, MAX_INTERFACE_NAME_LENGTH);
    }
#endif /* REGEX_SUPPORTED */

    if (ER_OK == status) {
        status = ValidateMembers(singleInterface);
    }

    return status;
}

QStatus XmlRulesValidator::ValidateMembers(const qcc::XmlElement* singleInterface)
{
    MemberValidatorFactory memberValidatorFactory;
    QStatus status = ValidateChildrenCountPositive(singleInterface);

    for (auto member : singleInterface->GetChildren()) {
        status = ValidateMember(member, memberValidatorFactory);

        if (ER_OK != status) {
            break;
        }
    }

    return status;
}

QStatus XmlRulesValidator::ValidateMember(const qcc::XmlElement* member, MemberValidatorFactory& memberValidatorFactory)
{
    PermissionPolicy::Rule::Member::MemberType type;

    QStatus status = MemberValidator::GetValidMemberType(member, &type);
    if (ER_OK == status) {
        status = memberValidatorFactory.ForType(type)->Validate(member);
    }

    return status;
}

QStatus XmlRulesValidator::MemberValidator::Validate(const qcc::XmlElement* member)
{
    QStatus status = ValidateAttributeValueUnique(member, memberNames);

#ifdef REGEX_SUPPORTED
    if (ER_OK == status) {
        status = ValidateNameAttributeValue(member, s_memberNameRegex, MAX_MEMBER_NAME_LENGTH);
    }
#endif /* REGEX_SUPPORTED */

    if (ER_OK == status) {
        status = ValidateAnnotations(member);
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

QStatus XmlRulesValidator::MemberValidator::ValidateAnnotations(const qcc::XmlElement* member)
{
    std::unordered_set<std::string> presentAnnotations;
    QStatus status = ValidateChildrenCountPositive(member);

    for (auto annotation : member->GetChildren()) {
        status = ValidateAnnotation(annotation, presentAnnotations);

        if (ER_OK != status) {
            break;
        }
    }

    return status;
}

QStatus XmlRulesValidator::MemberValidator::ValidateAnnotation(qcc::XmlElement* annotation, std::unordered_set<std::string>& presentAnnotations)
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

QStatus XmlRulesValidator::MemberValidator::ValidateAnnotationAllowed(qcc::XmlElement* annotation, std::unordered_set<std::string>& presentAnnotations)
{
    QStatus status = ValidateAnnotationAllowedForMember(annotation);

    if (ER_OK == status) {
        status = ValidateDenyAnnotation(presentAnnotations);
    }

    return status;
}

QStatus XmlRulesValidator::MemberValidator::ValidateAnnotationAllowedForMember(qcc::XmlElement* annotation)
{
    String action = annotation->GetAttribute(VALUE_XML_ATTRIBUTE);
    auto foundAction = GetActionsMap().find(action.c_str());

    return (foundAction == GetActionsMap().end()) ? ER_XML_MALFORMED : ER_OK;
}

QStatus XmlRulesValidator::MemberValidator::ValidateDenyAnnotation(std::unordered_set<std::string>& presentAnnotations)
{
    bool denyAbsent = (presentAnnotations.find(DENY_MEMBER_MASK) == presentAnnotations.end());

    return (denyAbsent ||
            presentAnnotations.size() == 1U) ? ER_OK : ER_XML_MALFORMED;
}

QStatus XmlRulesValidator::MemberValidator::GetValidMemberType(const qcc::XmlElement* member, PermissionPolicy::Rule::Member::MemberType* memberType)
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

QStatus ajn::XmlRulesValidator::ValidateAttributeValueUnique(const qcc::XmlElement* xmlElement, std::unordered_set<std::string>& valuesSet, AJ_PCSTR attributeName)
{
    std::string attribute;
    ExtractAttributeOrWildcard(xmlElement, attributeName, &attribute);

    return InsertUniqueOrFail(attribute, valuesSet);
}

#ifdef REGEX_SUPPORTED

QStatus XmlRulesValidator::ValidateNameAttributeValue(const qcc::XmlElement* xmlElement, const std::regex& namePattern, size_t maxNameLength)
{
    std::string nameAttribute;
    ExtractAttributeOrWildcard(xmlElement, NAME_XML_ATTRIBUTE, &nameAttribute);

    return ValidateString(nameAttribute, namePattern, maxNameLength);
}

QStatus XmlRulesValidator::ValidateString(std::string input, const std::regex& pattern, size_t maxLength)
{
    if (input.empty() ||
        (input.size() > maxLength) ||
        !std::regex_match(input, pattern)) {
        return ER_FAIL;
    }

    return ER_OK;
}

#endif // REGEX_SUPPORTED

QStatus XmlRulesValidator::ValidateNameAttributeValue(const qcc::XmlElement* xmlElement, AJ_PCSTR name)
{
    std::string nameAttribute;
    ExtractAttributeOrWildcard(xmlElement, NAME_XML_ATTRIBUTE, &nameAttribute);

    return (nameAttribute == name) ? ER_OK : ER_FAIL;
}

const std::unordered_map<std::string, uint8_t>& XmlRulesValidator::MethodsValidator::GetActionsMap()
{
    return s_actionsMap;
}

uint8_t XmlRulesValidator::MethodsValidator::GetValidActions()
{
    return PermissionPolicy::Rule::Member::ACTION_PROVIDE |
           PermissionPolicy::Rule::Member::ACTION_MODIFY;
}

const std::unordered_map<std::string, uint8_t>& XmlRulesValidator::PropertiesValidator::GetActionsMap()
{
    return s_actionsMap;
}

uint8_t XmlRulesValidator::PropertiesValidator::GetValidActions()
{
    return PermissionPolicy::Rule::Member::ACTION_OBSERVE |
           PermissionPolicy::Rule::Member::ACTION_PROVIDE |
           PermissionPolicy::Rule::Member::ACTION_MODIFY;
}

const std::unordered_map<std::string, uint8_t>& XmlRulesValidator::SignalsValidator::GetActionsMap()
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