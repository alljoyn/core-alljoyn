#ifndef _ALLJOYN_RULES_XML_CONVERTER_H
#define _ALLJOYN_RULES_XML_CONVERTER_H
/**
 * @file
 * This file defines the converter for a set of PermissionPolicy::Rules in XML format
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

#ifndef __cplusplus
#error Only include XmlRulesConverter.h in C++ code.
#endif

#include <qcc/platform.h>
#include <qcc/XmlElement.h>
#include <alljoyn/Status.h>
#include <alljoyn/PermissionPolicy.h>
#include <map>
#include <unordered_map>
#include <string>
#include <regex>
#include "XmlRulesValidator.h"

namespace ajn {

class XmlRulesConverter {
  public:

    /*
     * Initializes the static members.
     */
    static void Init();

    /**
     * Extract PermissionPolicy::Rules from rules in XML format. The rules
     * XML schema is available under alljoyn_core/docs/manifest_template.xsd.
     *
     * @param[in]    rulesXml   Rules in XML format.
     * @param[out]   rules      Extracted rules.
     *
     * @return   #ER_OK if extracted correctly.
     *           #ER_XML_MALFORMED if the XML does not follow the policy XML schema.
     */
    static QStatus XmlToRules(AJ_PCSTR rulesXml, std::vector<PermissionPolicy::Rule>& rules);

    /**
     * Extract rules XML from an array of PermissionPolicy::Rules.
     *
     * @param[in]    rules      Array containing the rules.
     * @param[in]    rulesCount Number of rules in the array.
     * @param[out]   rulesXml   Rules in XML format.
     * @param[in]    rootName   Name of the root element.
     *
     * @return   #ER_OK if extracted correctly.
     *           #ER_FAIL if the rules do not map to an XML valid according to the rules XML schema.
     */
    static QStatus RulesToXml(const PermissionPolicy::Rule* rules,
                              const size_t rulesCount,
                              std::string& rulesXml,
                              AJ_PCSTR rootName = MANIFEST_XML_ELEMENT);

    /**
     * Extract rules XML from an array of PermissionPolicy::Rules.
     *
     * @param[in]    rules       Array containing the rules.
     * @param[in]    rulesCount  Number of rules in the array.
     * @param[out]   rulesXml    Manifest in XML format.
     *                           Must be freed by calling "delete".
     * @param[in]    rootName    Name of the root element.
     *
     * @return   #ER_OK if extracted correctly.
     *           #ER_FAIL if the rules do not map to an XML valid according to the rules XML schema.
     */
    static QStatus RulesToXml(const PermissionPolicy::Rule* rules,
                              const size_t rulesCount,
                              qcc::XmlElement** rulesXml,
                              AJ_PCSTR rootName = MANIFEST_XML_ELEMENT);

  private:

    /*
     * Map used to extract XML member type values from the PermissionPolicy::Rule::Member::MemberType enum.
     * Cannot use unordered_map with enums as keys for some compilers.
     */
    static std::map<PermissionPolicy::Rule::Member::MemberType, std::string> s_inverseMemberTypeMap;

    /*
     * Map used to extract action masks from XML contents.
     */
    static std::unordered_map<std::string, uint8_t> s_memberMasksMap;

    /*
     * Initializes the "s_inverseMemberTypeMap" member.
     */
    static void InitInverseMemberTypeMap();

    /*
     * Initializes the "s_memberMasksMap" member.
     */
    static void InitMemberMasksMap();

    /**
     * Builds a vector of PermissionPolicy::Rules according to the input XML.
     *
     * @param[in]    root    Root element of the rules XML.
     * @param[out]   rules   Reference to rules vector.
     */
    static void BuildRules(const qcc::XmlElement* root, std::vector<PermissionPolicy::Rule>& rules);

    /**
     * Adds PermissionPolicy::Rules objects to the vector using the provided XML element.
     *
     * @param[in]    node    The "node" element of the rules XML.
     * @param[out]    rules   Vector to add the new rule to.
     */
    static void AddRules(const qcc::XmlElement* node, std::vector<PermissionPolicy::Rule>& rules);

    /**
     * Sets the PermissionPolicy::Rule's interface name according to the input XML.
     *
     * @param[in]    singleInterface The "interface" element of the rules XML.
     * @param[out]   rule            Reference to the new rule.
     */
    static void SetInterfaceName(const qcc::XmlElement* singleInterface, PermissionPolicy::Rule& rule);

    /**
     * Adds a PermissionPolicy::Rule object to the vector using the provided XML element.
     *
     * @param[in]    singleInterface The "interface" element of the rules XML.
     * @param[in]    objectPath      The object path of the rule element.
     * @param[out]    rules           Vector to add the new rule to.
     */
    static void AddRule(const qcc::XmlElement* singleInterface, std::string& objectPath, std::vector<PermissionPolicy::Rule>& rules);

    /**
     * Builds a PermissionPolicy::Rule according to the input XML.
     *
     * @param[in]    singleInterface The "interface" element of the rules XML.
     * @param[in]    objectPath      The object path of the rule element.
     * @param[out]   rule            Reference to the new rule.
     */
    static void BuildRule(const qcc::XmlElement* singleInterface, std::string& objectPath, PermissionPolicy::Rule& rule);

    /**
     * Builds a PermissionPolicy::Rule::Member according to the input XML.
     *
     * @param[in]    xmlMember   The member (method/property/signal) element of the rules XML.
     * @param[out]   member      Reference to the new member.
     */
    static void BuildMember(const qcc::XmlElement* xmlMember, PermissionPolicy::Rule::Member& member);

    /**
     * Adds PermissionPolicy::Rule::Members objects to the rule using the provided XML element.
     *
     * @param[in]    singleInterface The "interface" element of the rules XML.
     * @param[out]   rule            Reference to the new rule.
     */
    static void AddMembers(const qcc::XmlElement* singleInterface, PermissionPolicy::Rule& rule);

    /**
     * Adds a PermissionPolicy::Rule::Member object to the vector using the provided XML element.
     *
     * @param[in]    xmlMember   The member (method/property/signal) element of the rules XML.
     * @param[in]    members     Vector to add the new member to.
     */
    static void AddMember(const qcc::XmlElement* xmlMember, std::vector<PermissionPolicy::Rule::Member>& members);

    /**
     * Sets the PermissionPolicy::Rule::Member's name according to the input XML.
     *
     * @param[in]    xmlMember   The member (method/property/signal) element of the rules XML.
     * @param[out]   member      Reference to the new member.
     */
    static void SetMemberName(const qcc::XmlElement* xmlMember, PermissionPolicy::Rule::Member& member);

    /**
     * Sets the PermissionPolicy::Rule::Member's type according to the input XML.
     *
     * @param[in]    xmlMember   The member (method/property/signal) element of the rules XML.
     * @param[out]   member      Reference to the new member.
     */
    static void SetMemberType(const qcc::XmlElement* xmlMember, PermissionPolicy::Rule::Member& member);

    /**
     * Sets the PermissionPolicy::Rule::Member's action mask according to the input XML.
     *
     * @param[in]    xmlMember   The member (method/property/signal) element of the rules XML.
     * @param[out]   member      Reference to the new member.
     */
    static void SetMemberMask(const qcc::XmlElement* xmlMember, PermissionPolicy::Rule::Member& member);

    /**
     * Builds a PermissionPolicy::Rule::Member action mask according to the input XML.
     *
     * @param[in]    xmlMember   The member (method/property/signal) element of the rules XML.
     * @param[out]   mask        The resulting action mask.
     */
    static void BuildActionMask(const qcc::XmlElement* xmlMember, uint8_t* mask);

    /**
     * Builds a XML rules element according to the input PermissionPolicy::Rules.
     *
     * @param[in]    rules      An array of rules.
     * @param[in]    rulesCount Number of rules in the array.
     * @param[out]   rulesXml   Reference to the built rules XmlElement.
     */
    static void BuildRulesContents(const PermissionPolicy::Rule* rules, size_t rulesCount, qcc::XmlElement* rulesXml);

    /**
     * Adds a XML "node" element to the rules element according to the input PermissionPolicy::Rules.
     *
     * @param[in]    rules       A vector of rules with the same object path.
     * @param[out]   rulesXml   Reference to the built rules XmlElement.
     */
    static void BuildXmlNode(const std::vector<PermissionPolicy::Rule>& rules, qcc::XmlElement* rulesXml);

    /**
     * Adds a XML "interface" element to the "node" element according to the input PermissionPolicy::Rule.
     *
     * @param[in]    rule        A single rule object.
     * @param[out]   nodeElement Reference to the built "node" XmlElement.
     */
    static void BuildXmlInterface(const PermissionPolicy::Rule& rule, qcc::XmlElement* nodeElement);

    /**
     * Adds a XML member (method/property/signal) element to the "interface" element according to the input
     * PermissionPolicy::Rule::Member.
     *
     * @param[in]    member              A single member object.
     * @param[out]   interfaceElement    Reference to the built "interface" XmlElement.
     */
    static void BuildXmlMember(const PermissionPolicy::Rule::Member& member, qcc::XmlElement* interfaceElement);

    /**
     * Adds a XML "annotation" element to the member element according to the input action masks.
     *
     * @param[in]    masks           Member action masks.
     * @param[out]   memberElement   Reference to the built member XmlElement.
     */
    static void BuilXmlAnnotations(uint8_t masks, qcc::XmlElement* memberElement);

    /**
     * Helper method creating a child XmlElement with a preset "name" attribute.
     *
     * @param[in]   parent              The parent XmlElement.
     * @param[in]   childElementName    Name of the created child XmlElement.
     * @param[in]   name                The value of the "name" attribute.
     * @param[out]  child               The new child managed by the parent object.
     *
     * @return    Reference to the newly created child element.
     */
    static void CreateChildWithNameAttribute(qcc::XmlElement* parent,
                                             AJ_PCSTR childElementName,
                                             AJ_PCSTR name,
                                             qcc::XmlElement** child);

    /**
     * Adds an "annotation" XML element to the given parent with a given annotation value.
     *
     * @param[in]    parent          The parent XmlElement.
     * @param[in]    annotationValue Value of the added annotation.
     */
    static void AddChildActionAnnotation(qcc::XmlElement* parent, AJ_PCSTR annotationValue);

    /**
     * Copies rules from a vector to an array.
     *
     * @param[in]    rulesVector Vector containing the copied rules.
     * @param[out]   rules       Array for the extracted rules.
     * @param[out]   rulesCount  Number of rules in the array.
     */
    static void CopyRules(std::vector<PermissionPolicy::Rule>& rulesVector, PermissionPolicy::Rule** rules, size_t* rulesCount);

    /**
     * Checks if given action masks contain a particular action.
     *
     * @param[in]    masks   Input actions masks.
     * @param[in]    action  One particular action.
     *
     * return    True if the masks contain the action.
     *           False otherwise.
     */
    static bool MasksContainAction(uint8_t masks, uint8_t action);
};
} /* namespace ajn */
#endif
