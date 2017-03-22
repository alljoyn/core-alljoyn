#ifndef _ALLJOYN_RULES_XML_CONVERTER_H
#define _ALLJOYN_RULES_XML_CONVERTER_H
/**
 * @file
 * This file defines the converter for a set of PermissionPolicy::Rules in XML format
 */

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

    /**
     * Initializes the static members.
     */
    static void Init();

    /**
     * Performs the static members cleanup.
     */
    static void Shutdown();

    /**
     * Return the singleton instance of the converter.
     */
    static XmlRulesConverter* GetInstance();

    /**
     * Default virtual destructor.
     */
    virtual ~XmlRulesConverter();

    /**
     * Extract PermissionPolicy::Rules from rules in XML format. The rules
     * XML schema is available under alljoyn_core/docs/manifest_template.xsd.
     *
     * @param[in]    rulesXml   Rules in XML format.
     * @param[out]   rules      Extracted rules.
     *
     * @return   #ER_OK if extracted correctly.
     *           One of the #ER_XML_CONVERTER_ERROR group if the XML does not follow the policy XML schema.
     */
    QStatus XmlToRules(AJ_PCSTR rulesXml, std::vector<PermissionPolicy::Rule>& rules);

    /**
     * Extract rules XML from an array of PermissionPolicy::Rules.
     *
     * @param[in]    rules      Array containing the rules.
     * @param[in]    rulesCount Number of rules in the array.
     * @param[out]   rulesXml   Rules in XML format.
     * @param[in]    rootName   Name of the root element.
     *
     * @return   #ER_OK if extracted correctly.
     *           One of the #ER_XML_CONVERTER_ERROR group if the rules do not map to an XML valid according to the rules XML schema.
     */
    QStatus RulesToXml(const PermissionPolicy::Rule* rules,
                       const size_t rulesCount,
                       std::string& rulesXml);

    /**
     * Extract rules XML from an array of PermissionPolicy::Rules.
     *
     * @param[in]    rules       Array containing the rules.
     * @param[in]    rulesCount  Number of rules in the array.
     * @param[out]   rulesXml    Manifest in XML format.
     *                           Must be freed by calling "delete".
     *
     * @return   #ER_OK if extracted correctly.
     *           One of the #ER_XML_CONVERTER_ERROR group if the rules do not map to an XML valid according to the rules XML schema.
     */
    QStatus RulesToXml(const PermissionPolicy::Rule* rules,
                       const size_t rulesCount,
                       qcc::XmlElement** rulesXml);

  protected:

    /**
     * User shouldn't be able to create their own instance of the converter.
     */
    XmlRulesConverter();
    XmlRulesConverter(const XmlRulesConverter& other);
    XmlRulesConverter& operator=(const XmlRulesConverter& other);

    /**
     * Retrieves the root element name valid for the converted XML.
     *
     * @return   Root element name.
     */
    virtual std::string GetRootElementName();

    /**
     * Retrieves a validator for the current converter.
     *
     * @return   Validator associated with this converter.
     */
    virtual XmlRulesValidator* GetValidator();

    /**
     * Adds "annotation" elements to the "interface" element if required.
     *
     * @param[in]    rule                A single rule object.
     * @param[out]  interfaceElement    Reference to the built "interface" XmlElement.
     */
    virtual void BuildXmlInterfaceAnnotations(const PermissionPolicy::Rule& rule, qcc::XmlElement* interfaceElement);

    /**
     * Adds an "annotation" XML element to the given parent with a given annotation value and name.
     *
     * @param[in]    parent          The parent XmlElement.
     * @param[in]    annotationName  Name of the added annotation.
     * @param[in]    annotationValue Value of the added annotation.
     */
    void AddChildAnnotation(qcc::XmlElement* parent, AJ_PCSTR annotationName, AJ_PCSTR annotationValue);

    /**
     * Returns the current converter's rule type.
     *
     * @return   Converter rule type.
     */
    virtual PermissionPolicy::Rule::RuleType GetRuleType();

  private:

    /**
     * Static converter instance. Required to enable method overwriting.
     */
    static XmlRulesConverter* s_converter;

    /*
     * Map used to extract XML member type values from the PermissionPolicy::Rule::Member::MemberType enum.
     * Cannot use unordered_map with enums as keys for some compilers.
     */
    static std::map<PermissionPolicy::Rule::Member::MemberType, std::string>* s_inverseMemberTypeMap;

    /*
     * Map used to extract action masks from XML contents.
     */
    static std::unordered_map<std::string, uint8_t>* s_memberMasksMap;

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
    void BuildRules(const qcc::XmlElement* root, std::vector<PermissionPolicy::Rule>& rules);

    /**
     * Adds PermissionPolicy::Rules objects to the vector using the provided XML element.
     *
     * @param[in]    node    The "node" element of the rules XML.
     * @param[out]    rules   Vector to add the new rule to.
     */
    void AddRules(const qcc::XmlElement* node, std::vector<PermissionPolicy::Rule>& rules);

    /**
     * Sets the PermissionPolicy::Rule's interface name according to the input XML.
     *
     * @param[in]    singleInterface The "interface" element of the rules XML.
     * @param[out]   rule            Reference to the new rule.
     */
    void SetInterfaceName(const qcc::XmlElement* singleInterface, PermissionPolicy::Rule& rule);

    /**
     * Adds a PermissionPolicy::Rule object to the vector using the provided XML element.
     *
     * @param[in]    singleInterface    The "interface" element of the rules XML.
     * @param[in]    objectPath         The object path of the rule element.
     * @param[in]    objectAnnotations  The annotations defined for the processed object.
     * @param[out]    rules             Vector to add the new rule to.
     */
    void AddRule(const qcc::XmlElement* singleInterface,
                 const std::string& objectPath,
                 const std::vector<qcc::XmlElement*>& objectAnnotations,
                 std::vector<PermissionPolicy::Rule>& rules);

    /**
     * Builds a PermissionPolicy::Rule according to the input XML.
     *
     * @param[in]    singleInterface    The "interface" element of the rules XML.
     * @param[in]    objectPath         The object path of the rule element.
     * @param[in]    objectAnnotations  The annotations defined for the processed object.
     * @param[out]   rule               Reference to the new rule.
     */
    void BuildRule(const qcc::XmlElement* singleInterface,
                   const std::string& objectPath,
                   const std::vector<qcc::XmlElement*>& objectAnnotations,
                   PermissionPolicy::Rule& rule);

    /**
     * Updates a PermissionPolicy::Rule with the values of its annotations.
     *
     * @param[in]    objectAnnotations       The object annotations defined for that rule.
     * @param[in]    interfaceAnnotations    The interface annotations defined for that rule.
     * @param[out]   rule                    Reference to the new rule.
     */
    void UpdateRuleAnnotations(const std::vector<qcc::XmlElement*>& objectAnnotations,
                               const std::vector<qcc::XmlElement*>& interfaceAnnotations,
                               PermissionPolicy::Rule& rule);

    /**
     * Updates a PermissionPolicy::Rule security level.
     *
     * @param[in]    annotationsMap  All annotations name->value map.
     * @param[out]   rule            Reference to the new rule.
     */
    void UpdateRuleSecurityLevel(const std::map<std::string, std::string>& annotationsMap, PermissionPolicy::Rule& rule);

    /**
     * Builds a PermissionPolicy::Rule::Member according to the input XML.
     *
     * @param[in]    xmlMember   The member (method/property/signal) element of the rules XML.
     * @param[out]   member      Reference to the new member.
     */
    void BuildMember(const qcc::XmlElement* xmlMember, PermissionPolicy::Rule::Member& member);

    /**
     * Adds PermissionPolicy::Rule::Members objects to the rule using the provided XML element.
     *
     * @param[in]    xmlMembers A collection of XML member elements.
     * @param[out]   rule       Reference to the new rule.
     */
    void AddMembers(const std::vector<qcc::XmlElement*>& xmlMembers, PermissionPolicy::Rule& rule);

    /**
     * Adds a PermissionPolicy::Rule::Member object to the vector using the provided XML element.
     *
     * @param[in]    xmlMember   The member (method/property/signal) element of the rules XML.
     * @param[in]    members     Vector to add the new member to.
     */
    void AddMember(const qcc::XmlElement* xmlMember, std::vector<PermissionPolicy::Rule::Member>& members);

    /**
     * Sets the PermissionPolicy::Rule::Member's name according to the input XML.
     *
     * @param[in]    xmlMember   The member (method/property/signal) element of the rules XML.
     * @param[out]   member      Reference to the new member.
     */
    void SetMemberName(const qcc::XmlElement* xmlMember, PermissionPolicy::Rule::Member& member);

    /**
     * Sets the PermissionPolicy::Rule::Member's type according to the input XML.
     *
     * @param[in]    xmlMember   The member (method/property/signal) element of the rules XML.
     * @param[out]   member      Reference to the new member.
     */
    void SetMemberType(const qcc::XmlElement* xmlMember, PermissionPolicy::Rule::Member& member);

    /**
     * Sets the PermissionPolicy::Rule::Member's action mask according to the input XML.
     *
     * @param[in]    xmlMember   The member (method/property/signal) element of the rules XML.
     * @param[out]   member      Reference to the new member.
     */
    void SetMemberMask(const qcc::XmlElement* xmlMember, PermissionPolicy::Rule::Member& member);

    /**
     * Builds a PermissionPolicy::Rule::Member action mask according to the input XML.
     *
     * @param[in]    xmlMember   The member (method/property/signal) element of the rules XML.
     * @param[out]   mask        The resulting action mask.
     */
    void BuildActionMask(const qcc::XmlElement* xmlMember, uint8_t* mask);

    /**
     * Builds a XML rules element according to the input PermissionPolicy::Rules.
     *
     * @param[in]    rules      An array of rules.
     * @param[in]    rulesCount Number of rules in the array.
     * @param[out]   rulesXml   Reference to the built rules XmlElement.
     */
    void BuildRulesContents(const PermissionPolicy::Rule* rules, size_t rulesCount, qcc::XmlElement* rulesXml);

    /**
     * Adds a XML "node" element to the rules element according to the input PermissionPolicy::Rules.
     *
     * @param[in]    rules       A vector of rules with the same object path.
     * @param[out]   rulesXml   Reference to the built rules XmlElement.
     */
    void BuildXmlNode(const std::vector<PermissionPolicy::Rule>& rules, qcc::XmlElement* rulesXml);

    /**
     * Adds a XML "interface" element to the "node" element according to the input PermissionPolicy::Rule.
     *
     * @param[in]    rule        A single rule object.
     * @param[out]   nodeElement Reference to the built "node" XmlElement.
     */
    void BuildXmlInterface(const PermissionPolicy::Rule& rule, qcc::XmlElement* nodeElement);

    /**
     * Adds a XML member (method/property/signal) element to the "interface" element according to the input
     * PermissionPolicy::Rule::Member.
     *
     * @param[in]    member              A single member object.
     * @param[out]   interfaceElement    Reference to the built "interface" XmlElement.
     */
    void BuildXmlMember(const PermissionPolicy::Rule::Member& member, qcc::XmlElement* interfaceElement);

    /**
     * Adds a XML "annotation" element to the member element according to the input action masks.
     *
     * @param[in]    masks           Member action masks.
     * @param[out]   memberElement   Reference to the built member XmlElement.
     */
    void BuilXmlAnnotations(uint8_t masks, qcc::XmlElement* memberElement);

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
    void CreateChildWithNameAttribute(qcc::XmlElement* parent,
                                      AJ_PCSTR childElementName,
                                      AJ_PCSTR name,
                                      qcc::XmlElement** child);

    /**
     * Copies rules from a vector to an array.
     *
     * @param[in]    rulesVector Vector containing the copied rules.
     * @param[out]   rules       Array for the extracted rules.
     * @param[out]   rulesCount  Number of rules in the array.
     */
    void CopyRules(std::vector<PermissionPolicy::Rule>& rulesVector, PermissionPolicy::Rule** rules, size_t* rulesCount);

    /**
     * Checks if given action masks contain a particular action.
     *
     * @param[in]    masks   Input actions masks.
     * @param[in]    action  One particular action.
     *
     * return    True if the masks contain the action.
     *           False otherwise.
     */
    bool MasksContainAction(uint8_t masks, uint8_t action);

    /**
     * A helper method to update a name->value map containing the D-Bus annotations.
     *
     * @param[in]    annotations     XML elements containing the annotations.
     * @param[out]   annotationsMap  Map to be updated.
     */
    void UpdateAnnotationsMap(const std::vector<qcc::XmlElement*>& annotations, std::map<std::string, std::string>& annotationsMap);
};
} /* namespace ajn */
#endif
