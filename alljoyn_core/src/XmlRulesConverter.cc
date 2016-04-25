/**
 * @file
 * XmlRulesConverter is a class handling conversion of rules and rules templates
 * between XML format and an array of PermissionPolicy::Rules
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
#include "XmlRulesConverter.h"

using namespace qcc;
using namespace std;

namespace ajn {

map<PermissionPolicy::Rule::Member::MemberType, string> XmlRulesConverter::s_inverseMemberTypeMap;

unordered_map<string, uint8_t> XmlRulesConverter::s_memberMasksMap;

void XmlRulesConverter::Init()
{
    static bool initialized = false;

    if (!initialized) {
        InitInverseMemberTypeMap();
        InitMemberMasksMap();

        initialized = true;
    }
}

void XmlRulesConverter::InitInverseMemberTypeMap()
{
    s_inverseMemberTypeMap[PermissionPolicy::Rule::Member::MemberType::METHOD_CALL] = METHOD_MEMBER_TYPE;
    s_inverseMemberTypeMap[PermissionPolicy::Rule::Member::MemberType::PROPERTY] = PROPERTY_MEMBER_TYPE;
    s_inverseMemberTypeMap[PermissionPolicy::Rule::Member::MemberType::SIGNAL] = SIGNAL_MEMBER_TYPE;
}

void XmlRulesConverter::InitMemberMasksMap()
{
    s_memberMasksMap[DENY_MEMBER_MASK] = 0;
    s_memberMasksMap[MODIFY_MEMBER_MASK] = PermissionPolicy::Rule::Member::ACTION_MODIFY;
    s_memberMasksMap[PROVIDE_MEMBER_MASK] = PermissionPolicy::Rule::Member::ACTION_PROVIDE;
    s_memberMasksMap[OBSERVE_MEMBER_MASK] = PermissionPolicy::Rule::Member::ACTION_OBSERVE;
}

QStatus XmlRulesConverter::XmlToRules(AJ_PCSTR rulesXml, vector<PermissionPolicy::Rule>& rules)
{
    QCC_ASSERT(nullptr != rulesXml);

    XmlElement* root = nullptr;
    QStatus status = qcc::XmlElement::GetRoot(rulesXml, &root);

    if (ER_OK == status) {
        status = XmlRulesValidator::Validate(root);
    }

    if (ER_OK == status) {
        BuildRules(root, rules);
    }

    delete root;
    return status;
}

QStatus XmlRulesConverter::RulesToXml(const PermissionPolicy::Rule* rules,
                                      const size_t rulesCount,
                                      string& rulesXml,
                                      AJ_PCSTR rootElement)
{
    QCC_ASSERT(nullptr != rules);

    qcc::XmlElement* rulesXmlElement = nullptr;
    QStatus status = RulesToXml(rules,
                                rulesCount,
                                &rulesXmlElement,
                                rootElement);

    if (ER_OK == status) {
        rulesXml = rulesXmlElement->Generate();
    }

    delete rulesXmlElement;

    return status;
}

QStatus XmlRulesConverter::RulesToXml(const PermissionPolicy::Rule* rules,
                                      const size_t rulesCount,
                                      qcc::XmlElement** rulesXml,
                                      AJ_PCSTR rootElement)
{
    QCC_ASSERT(nullptr != rulesXml);
    QCC_ASSERT(nullptr != rules);

    QStatus status = XmlRulesValidator::ValidateRules(rules, rulesCount);

    if (ER_OK == status) {
        *rulesXml = new qcc::XmlElement(rootElement);

        BuildRulesContents(rules, rulesCount, *rulesXml);
    }

    return status;
}

void XmlRulesConverter::BuildRules(const qcc::XmlElement* root, vector<ajn::PermissionPolicy::Rule>& rules)
{
    for (auto node : root->GetChildren()) {
        AddRules(node, rules);
    }
}

void XmlRulesConverter::AddRules(const qcc::XmlElement* node, vector<ajn::PermissionPolicy::Rule>& rules)
{
    string objectPath;
    XmlRulesValidator::ExtractAttributeOrWildcard(node, NAME_XML_ATTRIBUTE, &objectPath);

    for (auto singleInterface : node->GetChildren()) {
        AddRule(singleInterface, objectPath, rules);
    }
}

void XmlRulesConverter::AddRule(const qcc::XmlElement* singleInterface, string& objectPath, vector<ajn::PermissionPolicy::Rule>& rules)
{
    PermissionPolicy::Rule rule;
    BuildRule(singleInterface, objectPath, rule);
    rules.push_back(rule);
}

void XmlRulesConverter::BuildRule(const qcc::XmlElement* singleInterface, string& objectPath, ajn::PermissionPolicy::Rule& rule)
{
    rule.SetObjPath(objectPath.c_str());
    SetInterfaceName(singleInterface, rule);
    AddMembers(singleInterface, rule);
}

void XmlRulesConverter::SetInterfaceName(const qcc::XmlElement* singleInterface, PermissionPolicy::Rule& rule)
{
    string interfaceName;
    XmlRulesValidator::ExtractAttributeOrWildcard(singleInterface, NAME_XML_ATTRIBUTE, &interfaceName);
    rule.SetInterfaceName(interfaceName.c_str());
}

void XmlRulesConverter::AddMembers(const qcc::XmlElement* singleInterface, ajn::PermissionPolicy::Rule& rule)
{
    vector<PermissionPolicy::Rule::Member> members;

    for (auto xmlMember : singleInterface->GetChildren()) {
        AddMember(xmlMember, members);
    }

    rule.SetMembers(members.size(), members.data());
}

void XmlRulesConverter::AddMember(const qcc::XmlElement* xmlMember, vector<PermissionPolicy::Rule::Member>& members)
{
    PermissionPolicy::Rule::Member member;
    BuildMember(xmlMember, member);
    members.push_back(member);
}

void XmlRulesConverter::BuildMember(const qcc::XmlElement* xmlMember, PermissionPolicy::Rule::Member& member)
{
    SetMemberName(xmlMember, member);
    SetMemberType(xmlMember, member);
    SetMemberMask(xmlMember, member);
}

void XmlRulesConverter::SetMemberName(const qcc::XmlElement* xmlMember, PermissionPolicy::Rule::Member& member)
{
    string name;
    XmlRulesValidator::ExtractAttributeOrWildcard(xmlMember, NAME_XML_ATTRIBUTE, &name);
    member.SetMemberName(name.c_str());
}

void XmlRulesConverter::SetMemberType(const qcc::XmlElement* xmlMember, PermissionPolicy::Rule::Member& member)
{
    AJ_PCSTR type = xmlMember->GetName().c_str();
    member.SetMemberType(XmlRulesValidator::s_memberTypeMap.find(type)->second);
}

void XmlRulesConverter::SetMemberMask(const qcc::XmlElement* xmlMember, PermissionPolicy::Rule::Member& member)
{
    uint8_t mask = 0;
    BuildActionMask(xmlMember, &mask);
    member.SetActionMask(mask);
}

void XmlRulesConverter::BuildActionMask(const qcc::XmlElement* xmlMember, uint8_t* mask)
{
    for (auto annotation : xmlMember->GetChildren()) {
        String maskString = annotation->GetAttribute(VALUE_XML_ATTRIBUTE);
        *mask |= s_memberMasksMap.find(maskString.c_str())->second;
    }
}

void XmlRulesConverter::BuildRulesContents(const PermissionPolicy::Rule* rules, const size_t rulesCount, qcc::XmlElement* rulesXml)
{
    map<string, vector<PermissionPolicy::Rule> > objectToRulesMap;
    XmlRulesValidator::AssignRulesToObjects(rules, rulesCount, objectToRulesMap);

    for (auto objectAndRulesPair : objectToRulesMap) {
        BuildXmlNode(objectAndRulesPair.second, rulesXml);
    }
}

void XmlRulesConverter::BuildXmlNode(const vector<PermissionPolicy::Rule>& rules, qcc::XmlElement* rulesElement)
{
    qcc::XmlElement* nodeElement = nullptr;
    CreateChildWithNameAttribute(rulesElement, NODE_XML_ELEMENT, rules[0].GetObjPath().c_str(), &nodeElement);

    for (auto rule : rules) {
        BuildXmlInterface(rule, nodeElement);
    }
}

void XmlRulesConverter::BuildXmlInterface(const PermissionPolicy::Rule& rule, qcc::XmlElement* nodeElement)
{
    qcc::XmlElement* interfaceElement = nullptr;
    CreateChildWithNameAttribute(nodeElement, INTERFACE_XML_ELEMENT, rule.GetInterfaceName().c_str(), &interfaceElement);

    for (size_t index = 0; index < rule.GetMembersSize(); index++) {
        BuildXmlMember(rule.GetMembers()[index], interfaceElement);
    }
}

void XmlRulesConverter::BuildXmlMember(const PermissionPolicy::Rule::Member& member, qcc::XmlElement* interfaceElement)
{
    qcc::XmlElement* memberElement = nullptr;
    string xmlType = s_inverseMemberTypeMap.find(member.GetMemberType())->second;
    CreateChildWithNameAttribute(interfaceElement, xmlType.c_str(), member.GetMemberName().c_str(), &memberElement);

    BuilXmlAnnotations(member.GetActionMask(), memberElement);
}

void XmlRulesConverter::BuilXmlAnnotations(const uint8_t masks, qcc::XmlElement* memberElement)
{
    if (MasksContainAction(masks, PermissionPolicy::Rule::Member::ACTION_PROVIDE)) {
        AddChildActionAnnotation(memberElement, PROVIDE_MEMBER_MASK);
    }

    if (MasksContainAction(masks, PermissionPolicy::Rule::Member::ACTION_OBSERVE)) {
        AddChildActionAnnotation(memberElement, OBSERVE_MEMBER_MASK);
    }

    if (MasksContainAction(masks, PermissionPolicy::Rule::Member::ACTION_MODIFY)) {
        AddChildActionAnnotation(memberElement, MODIFY_MEMBER_MASK);
    }
}

void XmlRulesConverter::CreateChildWithNameAttribute(qcc::XmlElement* parent,
                                                     AJ_PCSTR childElementName,
                                                     AJ_PCSTR name,
                                                     qcc::XmlElement** child)
{
    *child = parent->CreateChild(childElementName);
    (*child)->AddAttribute(NAME_XML_ATTRIBUTE, name);
}

void XmlRulesConverter::AddChildActionAnnotation(qcc::XmlElement* parent, AJ_PCSTR annotationValue)
{
    qcc::XmlElement* annotation = nullptr;
    CreateChildWithNameAttribute(parent, ANNOTATION_XML_ELEMENT, ACTION_ANNOTATION_NAME, &annotation);
    annotation->AddAttribute(VALUE_XML_ATTRIBUTE, annotationValue);
}

void XmlRulesConverter::CopyRules(vector<ajn::PermissionPolicy::Rule>& rulesVector, PermissionPolicy::Rule** rules, size_t* rulesCount)
{
    *rulesCount = rulesVector.size();
    *rules = new PermissionPolicy::Rule[*rulesCount];

    for (size_t index = 0; index < rulesVector.size(); index++) {
        (*rules)[index] = rulesVector[index];
    }
}

bool XmlRulesConverter::MasksContainAction(uint8_t masks, uint8_t action)
{
    return (masks & action) == action;
}
}