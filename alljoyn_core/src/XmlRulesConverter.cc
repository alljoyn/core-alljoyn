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
#include "XmlManifestTemplateValidator.h"

using namespace qcc;
using namespace std;

namespace ajn {

XmlRulesConverter* XmlRulesConverter::s_converter = nullptr;
map<PermissionPolicy::Rule::Member::MemberType, string> XmlRulesConverter::s_inverseMemberTypeMap;
unordered_map<string, uint8_t> XmlRulesConverter::s_memberMasksMap;

void XmlRulesConverter::Init()
{
    s_converter = new XmlRulesConverter();

    InitInverseMemberTypeMap();
    InitMemberMasksMap();
}

void XmlRulesConverter::Shutdown()
{
    delete s_converter;
    s_converter = nullptr;
}

XmlRulesConverter* XmlRulesConverter::GetInstance()
{
    return s_converter;
}

void XmlRulesConverter::InitInverseMemberTypeMap()
{
    s_inverseMemberTypeMap[PermissionPolicy::Rule::Member::MemberType::METHOD_CALL] = METHOD_MEMBER_TYPE;
    s_inverseMemberTypeMap[PermissionPolicy::Rule::Member::MemberType::PROPERTY] = PROPERTY_MEMBER_TYPE;
    s_inverseMemberTypeMap[PermissionPolicy::Rule::Member::MemberType::SIGNAL] = SIGNAL_MEMBER_TYPE;
    s_inverseMemberTypeMap[PermissionPolicy::Rule::Member::MemberType::NOT_SPECIFIED] = NOT_SPECIFIED_MEMBER_TYPE;
}

void XmlRulesConverter::InitMemberMasksMap()
{
    s_memberMasksMap[DENY_MEMBER_MASK] = 0;
    s_memberMasksMap[MODIFY_MEMBER_MASK] = PermissionPolicy::Rule::Member::ACTION_MODIFY;
    s_memberMasksMap[PROVIDE_MEMBER_MASK] = PermissionPolicy::Rule::Member::ACTION_PROVIDE;
    s_memberMasksMap[OBSERVE_MEMBER_MASK] = PermissionPolicy::Rule::Member::ACTION_OBSERVE;
}

XmlRulesConverter::XmlRulesConverter()
{
}

XmlRulesConverter::XmlRulesConverter(const XmlRulesConverter& other)
{
    QCC_UNUSED(other);
}

XmlRulesConverter& XmlRulesConverter::operator=(const XmlRulesConverter& other)
{
    QCC_UNUSED(other);
    return *this;
}

string XmlRulesConverter::GetRootElementName()
{
    return RULES_XML_ELEMENT;
}

XmlRulesValidator* XmlRulesConverter::GetValidator()
{
    return XmlRulesValidator::GetInstance();
}

QStatus XmlRulesConverter::XmlToRules(AJ_PCSTR rulesXml, vector<PermissionPolicy::Rule>& rules)
{
    QCC_ASSERT(nullptr != rulesXml);

    XmlElement* root = nullptr;
    QStatus status = XmlElement::GetRoot(rulesXml, &root);

    if (ER_OK == status) {
        status = GetValidator()->Validate(root);
    }

    if (ER_OK == status) {
        BuildRules(root, rules);
    }

    delete root;
    return status;
}

QStatus XmlRulesConverter::RulesToXml(const PermissionPolicy::Rule* rules,
                                      const size_t rulesCount,
                                      string& rulesXml)
{
    QCC_ASSERT(nullptr != rules);

    XmlElement* rulesXmlElement = nullptr;
    QStatus status = RulesToXml(rules,
                                rulesCount,
                                &rulesXmlElement);

    if (ER_OK == status) {
        rulesXml = rulesXmlElement->Generate();
    }

    delete rulesXmlElement;

    return status;
}

QStatus XmlRulesConverter::RulesToXml(const PermissionPolicy::Rule* rules,
                                      const size_t rulesCount,
                                      XmlElement** rulesXml)
{
    QCC_ASSERT(nullptr != rulesXml);
    QCC_ASSERT(nullptr != rules);

    QStatus status = GetValidator()->ValidateRules(rules, rulesCount);

    if (ER_OK == status) {
        *rulesXml = new XmlElement(GetRootElementName());

        BuildRulesContents(rules, rulesCount, *rulesXml);
    }

    return status;
}

void XmlRulesConverter::BuildRules(const XmlElement* root, vector<PermissionPolicy::Rule>& rules)
{
    for (auto node : root->GetChildren()) {
        AddRules(node, rules);
    }
}

void XmlRulesConverter::AddRules(const XmlElement* node, vector<PermissionPolicy::Rule>& rules)
{
    string objectPath;
    vector<XmlElement*> objectAnnotations;
    vector<XmlElement*> interfaces;

    XmlRulesValidator::ExtractAttributeOrWildcard(node, NAME_XML_ATTRIBUTE, &objectPath);
    XmlValidator::SeparateAnnotations(node, objectAnnotations, interfaces);

    for (auto singleInterface : interfaces) {
        AddRule(singleInterface, objectPath, objectAnnotations, rules);
    }

}

void XmlRulesConverter::AddRule(const XmlElement* singleInterface,
                                const string& objectPath,
                                const vector<XmlElement*>& objectAnnotations,
                                vector<PermissionPolicy::Rule>& rules)
{
    PermissionPolicy::Rule rule;
    BuildRule(singleInterface, objectPath, objectAnnotations, rule);
    rules.push_back(rule);
}

void XmlRulesConverter::BuildRule(const XmlElement* singleInterface,
                                  const string& objectPath,
                                  const vector<XmlElement*>& objectAnnotations,
                                  PermissionPolicy::Rule& rule)
{
    vector<XmlElement*> interfaceAnnotations;
    vector<XmlElement*> xmlMembers;

    XmlValidator::SeparateAnnotations(singleInterface, interfaceAnnotations, xmlMembers);

    rule.SetObjPath(objectPath.c_str());
    SetInterfaceName(singleInterface, rule);
    AddMembers(xmlMembers, rule);
    UpdateRuleAnnotations(objectAnnotations, interfaceAnnotations, rule);
}

void XmlRulesConverter::SetInterfaceName(const XmlElement* singleInterface, PermissionPolicy::Rule& rule)
{
    string interfaceName;
    XmlRulesValidator::ExtractAttributeOrWildcard(singleInterface, NAME_XML_ATTRIBUTE, &interfaceName);
    rule.SetInterfaceName(interfaceName.c_str());
}

void XmlRulesConverter::AddMembers(const vector<XmlElement*>& xmlMembers, PermissionPolicy::Rule& rule)
{
    vector<PermissionPolicy::Rule::Member> members;

    for (auto xmlMember : xmlMembers) {
        AddMember(xmlMember, members);
    }

    rule.SetMembers(members.size(), members.data());
}

void XmlRulesConverter::UpdateRuleAnnotations(const vector<XmlElement*>& objectAnnotations,
                                              const vector<XmlElement*>& interfaceAnnotations,
                                              PermissionPolicy::Rule& rule)
{
    map<string, string> annotationsMap;

    // Interface annotations override the object-level ones.
    UpdateAnnotationsMap(objectAnnotations, annotationsMap);
    UpdateAnnotationsMap(interfaceAnnotations, annotationsMap);

    UpdateRuleSecurityLevel(annotationsMap, rule);
}

void XmlRulesConverter::UpdateRuleSecurityLevel(const map<string, string>& annotationsMap, PermissionPolicy::Rule& rule)
{
    auto nameToValuePair = annotationsMap.find(SECURITY_LEVEL_ANNOTATION_NAME);
    if (nameToValuePair != annotationsMap.end()) {
        rule.SetSecurityLevel(XmlManifestTemplateValidator::s_securityLevelMap[nameToValuePair->second]);
    }
}

void XmlRulesConverter::AddMember(const XmlElement* xmlMember, vector<PermissionPolicy::Rule::Member>& members)
{
    PermissionPolicy::Rule::Member member;
    BuildMember(xmlMember, member);
    members.push_back(member);
}

void XmlRulesConverter::BuildMember(const XmlElement* xmlMember, PermissionPolicy::Rule::Member& member)
{
    SetMemberName(xmlMember, member);
    SetMemberType(xmlMember, member);
    SetMemberMask(xmlMember, member);
}

void XmlRulesConverter::SetMemberName(const XmlElement* xmlMember, PermissionPolicy::Rule::Member& member)
{
    string name;
    XmlRulesValidator::ExtractAttributeOrWildcard(xmlMember, NAME_XML_ATTRIBUTE, &name);
    member.SetMemberName(name.c_str());
}

void XmlRulesConverter::SetMemberType(const XmlElement* xmlMember, PermissionPolicy::Rule::Member& member)
{
    AJ_PCSTR type = xmlMember->GetName().c_str();
    member.SetMemberType(XmlRulesValidator::s_memberTypeMap.find(type)->second);
}

void XmlRulesConverter::SetMemberMask(const XmlElement* xmlMember, PermissionPolicy::Rule::Member& member)
{
    uint8_t mask = 0;
    BuildActionMask(xmlMember, &mask);
    member.SetActionMask(mask);
}

void XmlRulesConverter::BuildActionMask(const XmlElement* xmlMember, uint8_t* mask)
{
    for (auto annotation : xmlMember->GetChildren()) {
        String maskString = annotation->GetAttribute(VALUE_XML_ATTRIBUTE);
        *mask |= s_memberMasksMap.find(maskString.c_str())->second;
    }
}

void XmlRulesConverter::BuildRulesContents(const PermissionPolicy::Rule* rules, const size_t rulesCount, XmlElement* rulesXml)
{
    map<string, vector<PermissionPolicy::Rule> > objectToRulesMap;
    XmlRulesValidator::AssignRulesToObjects(rules, rulesCount, objectToRulesMap);

    for (auto objectAndRulesPair : objectToRulesMap) {
        BuildXmlNode(objectAndRulesPair.second, rulesXml);
    }
}

void XmlRulesConverter::BuildXmlNode(const vector<PermissionPolicy::Rule>& rules, XmlElement* rulesElement)
{
    XmlElement* nodeElement = nullptr;
    CreateChildWithNameAttribute(rulesElement, NODE_XML_ELEMENT, rules[0].GetObjPath().c_str(), &nodeElement);

    for (auto rule : rules) {
        BuildXmlInterface(rule, nodeElement);
    }
}

void XmlRulesConverter::BuildXmlInterface(const PermissionPolicy::Rule& rule, XmlElement* nodeElement)
{
    XmlElement* interfaceElement = nullptr;
    CreateChildWithNameAttribute(nodeElement, INTERFACE_XML_ELEMENT, rule.GetInterfaceName().c_str(), &interfaceElement);

    BuildXmlInterfaceAnnotations(rule, interfaceElement);

    for (size_t index = 0; index < rule.GetMembersSize(); index++) {
        BuildXmlMember(rule.GetMembers()[index], interfaceElement);
    }
}

void XmlRulesConverter::BuildXmlInterfaceAnnotations(const PermissionPolicy::Rule& rule, XmlElement* interfaceElement)
{
    // Basic rule sets currently do not have annotations in XML format.
    QCC_UNUSED(rule);
    QCC_UNUSED(interfaceElement);
}

void XmlRulesConverter::BuildXmlMember(const PermissionPolicy::Rule::Member& member, XmlElement* interfaceElement)
{
    XmlElement* memberElement = nullptr;
    string xmlType = s_inverseMemberTypeMap.find(member.GetMemberType())->second;
    CreateChildWithNameAttribute(interfaceElement, xmlType.c_str(), member.GetMemberName().c_str(), &memberElement);

    BuilXmlAnnotations(member.GetActionMask(), memberElement);
}

void XmlRulesConverter::BuilXmlAnnotations(const uint8_t masks, XmlElement* memberElement)
{
    if (MasksContainAction(masks, PermissionPolicy::Rule::Member::ACTION_PROVIDE)) {
        AddChildAnnotation(memberElement, ACTION_ANNOTATION_NAME, PROVIDE_MEMBER_MASK);
    }

    if (MasksContainAction(masks, PermissionPolicy::Rule::Member::ACTION_OBSERVE)) {
        AddChildAnnotation(memberElement, ACTION_ANNOTATION_NAME, OBSERVE_MEMBER_MASK);
    }

    if (MasksContainAction(masks, PermissionPolicy::Rule::Member::ACTION_MODIFY)) {
        AddChildAnnotation(memberElement, ACTION_ANNOTATION_NAME, MODIFY_MEMBER_MASK);
    }
}

void XmlRulesConverter::CreateChildWithNameAttribute(XmlElement* parent,
                                                     AJ_PCSTR childElementName,
                                                     AJ_PCSTR name,
                                                     XmlElement** child)
{
    *child = parent->CreateChild(childElementName);
    (*child)->AddAttribute(NAME_XML_ATTRIBUTE, name);
}

void XmlRulesConverter::AddChildAnnotation(XmlElement* parent, AJ_PCSTR annotationName, AJ_PCSTR annotationValue)
{
    XmlElement* annotation = nullptr;
    CreateChildWithNameAttribute(parent, ANNOTATION_XML_ELEMENT, annotationName, &annotation);
    annotation->AddAttribute(VALUE_XML_ATTRIBUTE, annotationValue);
}

void XmlRulesConverter::CopyRules(vector<PermissionPolicy::Rule>& rulesVector, PermissionPolicy::Rule** rules, size_t* rulesCount)
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
void XmlRulesConverter::UpdateAnnotationsMap(const vector<XmlElement*>& annotations, map<string, string>& annotationsMap)
{
    for (auto annotation : annotations) {
        const String annotationName = annotation->GetAttribute(NAME_XML_ATTRIBUTE);
        const String annotationValue = annotation->GetAttribute(VALUE_XML_ATTRIBUTE);

        if (!annotationName.empty() && !annotationValue.empty()) {
            annotationsMap[annotationName.c_str()] = annotationValue.c_str();
        }
    }
}
}