#ifndef _ALLJOYN_XML_RULES_VALIDATOR_H
#define _ALLJOYN_XML_RULES_VALIDATOR_H

/*
 * Workaround for ASACORE-2712
 */
#if !defined(__GNUC__) || \
    ((__GNUC__ >= 4) && \
    (__GNUC_MINOR__ >= 9))
#define REGEX_SUPPORTED
#endif

/**
 * @file
 * This file defines the validator for manifests and manifest templates in XML format
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
#error Only include XmlRulesValidator.h in C++ code.
#endif

#include <unordered_map>
#include <alljoyn/Status.h>
#include <alljoyn/PermissionPolicy.h>
#include <qcc/XmlElement.h>
#include "XmlValidator.h"

#ifdef REGEX_SUPPORTED
#include <regex>
#endif /* REGEX_SUPPORTED */

namespace ajn {

#define MANIFEST_XML_ELEMENT "manifest"
#define NODE_XML_ELEMENT "node"
#define INTERFACE_XML_ELEMENT "interface"
#define ANNOTATION_XML_ELEMENT "annotation"
#define NAME_XML_ATTRIBUTE "name"
#define VALUE_XML_ATTRIBUTE "value"
#define METHOD_MEMBER_TYPE "method"
#define PROPERTY_MEMBER_TYPE "property"
#define SIGNAL_MEMBER_TYPE "signal"
#define DENY_MEMBER_MASK "Deny"
#define OBSERVE_MEMBER_MASK "Observe"
#define PROVIDE_MEMBER_MASK "Provide"
#define MODIFY_MEMBER_MASK "Modify"
#define WILDCARD_XML_VALUE "*"
#define ACTION_ANNOTATION_NAME "org.alljoyn.Bus.Action"
#define MAX_INTERFACE_NAME_LENGTH 255
#define MAX_MEMBER_NAME_LENGTH 255

class XmlRulesValidator : public XmlValidator {
  public:

    /**
     * Mapping between the member types in string format and PermissionPolicy::Rule::Member::MemberType enum.
     */
    static std::unordered_map<std::string, PermissionPolicy::Rule::Member::MemberType> s_memberTypeMap;

    /*
     * Initializes the static members.
     */
    static void Init();

    /**
     * Validates the set of rules map to an XML that is valid according
     * to the schema for manifest/manifest template/policy rules XMLs.
     * The schema is available under alljoyn_core/docs/manifest_template.xsd.
     *
     * @param[in]   rules       Array of rules representing the manifest.
     * @param[in]   rulesCount  Number of rules in the array.
     *
     * @return
     *            #ER_OK if the input is correct.
     *            #ER_FAIL if the rules are not following the schema.
     */
    static QStatus ValidateRules(const PermissionPolicy::Rule* rules, const size_t rulesCount);

    /**
     * Validates the root containts a collection of qcc::XmlElements
     * representing valid "node" elements according to the manifest template schema.
     *
     * @param[in]   rootElement  Root element containing "node" elements.
     *
     * @return
     *            #ER_OK if the input is correct.
     *            #ER_XML_MALFORMED if the XML is not following the schema.
     */
    static QStatus Validate(const qcc::XmlElement* rootElement);

    /**
     * Default destructor.
     */
    virtual ~XmlRulesValidator()
    { }

    /**
     * Helper method to extract an attribute from qcc::XmlElement or
     * a wildcard "*" if the argument is not present.
     *
     * @param[in]   xmlElement       XML element to extract the attribute from.
     * @param[in]   attributeName    Name of the extracted attribute.
     * @param[out]  attribute        The extracted attribute value.
     */
    static void ExtractAttributeOrWildcard(const qcc::XmlElement* xmlElement, AJ_PCSTR attributeName, std::string* attribute);

    /**
     * Helper method to group rules according to their object path.
     *
     * @param[in]   rules            Array of rules.
     * @param[in]   rulesCount       Number of rules in the array.
     * @param[out]  objectToRulesMap Output map: object path -> collection of rules.
     */
    static void AssignRulesToObjects(const PermissionPolicy::Rule* rules,
                                     const size_t rulesCount,
                                     std::map<std::string, std::vector<PermissionPolicy::Rule> >& objectToRulesMap);

  private:

    class MemberValidatorFactory;

#ifdef REGEX_SUPPORTED

    /**
     * Regex describing the allowed object path format.
     */
    static const std::regex s_objectPathRegex;

    /**
     * Regex describing the allowed interface name format.
     */
    static const std::regex s_interfaceNameRegex;

    /**
     * Regex describing the allowed member name format.
     */
    static const std::regex s_memberNameRegex;

#endif /* REGEX_SUPPORTED */

    /*
     * Initializes "s_memberTypeMap".
     */
    static void MemberTypeMapInit();

    /**
     * Validates the qcc::XmlElement represents a valid "node"
     * elements according to the manifest template schema.
     *
     * @param[in]   node         A XML "node" element.
     * @param[in]   nodeNames    A collection of names for already verified nodes.
     *
     * @return
     *            #ER_OK if the input is correct.
     *            #ER_XML_MALFORMED if the XML is not following the schema.
     */
    static QStatus ValidateNode(const qcc::XmlElement* node, std::unordered_set<std::string>& nodeNames);

    /**
     * Validates the qcc::XmlElement contains valid "interface"
     * elements according to the manifest template schema.
     *
     * @param[in]   node  A "node" XML element.
     *
     * @return
     *            #ER_OK if the input is correct.
     *            #ER_XML_MALFORMED if the XML is not following the schema.
     */
    static QStatus ValidateInterfaces(const qcc::XmlElement* node);

    /**
     * Validates the qcc::XmlElement represent a valid "interface"
     * elements according to the manifest template schema.
     *
     * @param[in]   singleInterface  A XML "interface" element.
     * @param[in]   interfaceNames   A collection of names for already verified interfaces.
     *
     * @return
     *            #ER_OK if the input is correct.
     *            #ER_XML_MALFORMED if the XML is not following the schema.
     */
    static QStatus ValidateInterface(const qcc::XmlElement* singleInterface, std::unordered_set<std::string>& interfaceNames);

    /**
     * Validates the qcc::XmlElement contains valid members (method/property/signal)
     * elements according to the manifest template schema.
     *
     * @param[in]   singleInterface  A XML "interface" element
     *
     * @return
     *            #ER_OK if the input is correct.
     *            #ER_XML_MALFORMED if the XML is not following the schema.
     */
    static QStatus ValidateMembers(const qcc::XmlElement* singleInterface);

    /**
     * Validates the qcc::XmlElement represents a valid member.
     * elements according to the manifest template schema.
     *
     * @param[in]   member                   A XML "member" element.
     * @param[in]   memberValidatorFactory   A member validator factory.
     *
     * @return
     *            #ER_OK if the input is correct.
     *            #ER_XML_MALFORMED if the XML is not following the schema.
     */
    static QStatus ValidateMember(const qcc::XmlElement* member, MemberValidatorFactory& memberValidatorFactory);

    /**
     * Validates the collection of PermissionPolicy::Rules for a specific object
     * path represent a valid "node" element according to the manifest template schema.
     *
     * @param[in]   objectToRulesMap A mapping between object paths and a collection of rules.
     *
     * @return
     *            #ER_OK if the input is correct.
     *            #ER_FAIL otherwise.
     */
    static QStatus ValidateObject(const std::map<std::string, std::vector<PermissionPolicy::Rule> >& objectToRulesMap);

    /**
     * Validates the collection of PermissionPolicy::Rule represent a valid
     * "node" element according to the manifest template schema.
     *
     * @param[in]   rules       Collection of rules mapping to one "node" element.
     *
     * @return
     *            #ER_OK if the input is correct.
     *            #ER_FAIL otherwise.
     */
    static QStatus ValidateRules(const std::vector<PermissionPolicy::Rule>& rules);

    /**
     * Validates a single PermissionPolicy::Rule represents a valid
     * "interface" element according to the manifest template schema.
     *
     * @param[in]   rule             Single PermissionPolicy::Rule representing an "interface" element.
     * @param[in]   interfaceNames   Collection of interface names for already verified rules.
     *
     * @return
     *            #ER_OK if the input is correct.
     *            #ER_FAIL otherwise.
     */
    static QStatus ValidateRule(const PermissionPolicy::Rule& rule, std::unordered_set<std::string>& interfaceNames);

    /**
     * Validates the collection of PermissionPolicy::Rule::Members represent
     * valid member (method/property/signal) elements according to the manifest
     * template schema.
     *
     * @param[in]   rule Rule containing the member elements.
     *
     * @return
     *            #ER_OK if the input is correct.
     *            #ER_FAIL otherwise.
     */
    static QStatus ValidateMembers(const PermissionPolicy::Rule& rule);

    /**
     * Validates the qcc::XmlElement's attribute is unique in the given set.
     *
     * @param[in]    xmlElement      Xml element being verified.
     * @param[in]    valuesSet       A set to validated against.
     * @param[in]    attributeName   The attribute, which uniqueness will be checked.
     *
     * @return
     *            #ER_OK if the input is correct.
     *            #ER_XML_MALFORMED otherwise.
     */
    static QStatus ValidateAttributeValueUnique(const qcc::XmlElement* xmlElement,
                                                std::unordered_set<std::string>& valuesSet,
                                                AJ_PCSTR attributeName = NAME_XML_ATTRIBUTE);

#ifdef REGEX_SUPPORTED

    /**
     * Validates the qcc::XmlElement's name attribute exists,
     * follows the correct pattern and is of correct size.
     *
     * @param[in]    xmlElement      Xml element being verified.
     * @param[in]    namePattern     Pattern the attribute must follow.
     * @param[in]    maxNameLength   Maximum lenght of the attribute.
     *
     * @return
     *            #ER_OK if the input is correct.
     *            #ER_XML_MALFORMED otherwise.
     */
    static QStatus ValidateNameAttributeValue(const qcc::XmlElement* xmlElement, const std::regex& namePattern, size_t maxNameLength = SIZE_MAX);

    /**
     * Validates the string exists, follows the correct pattern and is of correct size.
     *
     * @param[in]    input       Validated string.
     * @param[in]    pattern     Pattern the string must follow.
     * @param[in]    maxLength   Maximum lenght of the string.
     *
     * @return
     *            #ER_OK if the input is correct.
     *            #ER_FAIL otherwise.
     */
    static QStatus ValidateString(std::string input, const std::regex& pattern, size_t maxLength = SIZE_MAX);

#endif // REGEX_SUPPORTED

    /**
     * Validates the qcc::XmlElement's name attribute exists
     * and is equal to the expected value.
     *
     * @param[in]    xmlElement  Xml element being verified.
     * @param[in]    name        Expected name attribute value.
     *
     * @return
     *            #ER_OK if the input is correct.
     *            #ER_XML_MALFORMED otherwise.
     */
    static QStatus ValidateNameAttributeValue(const qcc::XmlElement* xmlElement, AJ_PCSTR name);

    /*
     * Class for validating interface members (method/property/signal) both in qcc::XmlElement
     * form and PermissionPolicy::Rule::Member form.
     */
    class MemberValidator {
      public:

        /**
         * Default destructor.
         */
        virtual ~MemberValidator()
        { }

        /**
         * Validates the member given as qcc::XmlElement object.
         *
         * @param[in]    member  Validated member.
         *
         * @return
         *            #ER_OK if the input is correct.
         *            #ER_XML_MALFORMED otherwise.
         */
        QStatus Validate(const qcc::XmlElement* member);

        /**
         * Validates the member given as PermissionPolicy::Rule::Member object.
         *
         * @param[in]    member  Validated member.
         *
         * @return
         *            #ER_OK if the input is correct.
         *            #ER_FAIL otherwise.
         */
        QStatus Validate(const PermissionPolicy::Rule::Member& member);

        /**
         * Returns a member type from the qcc::XmlElement or an error if
         * the XML element didn't contain the correct information.
         *
         * @param[in]    member      A XML "member" element.
         * @param[out]   memberType  Retrieved member type.
         *
         * @return
         *            #ER_OK if the input is correct.
         *            #ER_XML_MALFORMED if the XML is not a valid member.
         */
        static QStatus GetValidMemberType(const qcc::XmlElement* member, PermissionPolicy::Rule::Member::MemberType* memberType);

      private:

        /**
         * Collection of names for already validated memebers.
         */
        std::unordered_set<std::string> memberNames;

        /**
         * @return A string -> action mask map of actions valid for the current member type.
         */
        virtual const std::unordered_map<std::string, uint8_t>& GetActionsMap() = 0;

        /**
         * @return A mask of actions valid for the current member type.
         */
        virtual uint8_t GetValidActions() = 0;

        /**
         * Validates a collection of "annotation" XML elements inside the member element.
         *
         * @param[in]    member  Member XML element.
         *
         * @return
         *            #ER_OK if the input is correct.
         *            #ER_XML_MALFORMED otherwise.
         */
        QStatus ValidateAnnotations(const qcc::XmlElement* member);

        /**
         * Validates a single "annotation" XML element inside the member element.
         *
         * @param[in]    annotation          Validated annotation.
         * @param[in]    presentAnnotations  Values of previous annotations in the given memeber.
         *
         * @return
         *            #ER_OK if the input is correct.
         *            #ER_XML_MALFORMED otherwise.
         */
        QStatus ValidateAnnotation(qcc::XmlElement* annotation, std::unordered_set<std::string>& presentAnnotations);

        /**
         * Validates the given annotation is allowed in a member element at all.
         *
         * @param[in]    annotation          Validated annotation.
         * @param[in]    presentAnnotations  Values of previous annotations in the given memeber.
         *
         * @return
         *            #ER_OK if the input is correct.
         *            #ER_XML_MALFORMED otherwise.
         */
        QStatus ValidateAnnotationAllowed(qcc::XmlElement* annotation, std::unordered_set<std::string>& presentAnnotations);

        /**
         * Validates the given annotation has the "value" attribute within the
         * set of allowed values for this particular member.
         *
         * @param[in]    annotation          Validated annotation.
         *
         * @return
         *            #ER_OK if the input is correct.
         *            #ER_XML_MALFORMED otherwise.
         */
        QStatus ValidateAnnotationAllowedForMember(qcc::XmlElement* annotation);

        /**
         * Validates if the "Deny" annotation is not present or is the only
         * annotation definied for this member.
         *
         * @param[in]    presentAnnotations  Values of previous annotations in the given memeber.
         *
         * @return
         *            #ER_OK if the input is correct.
         *            #ER_XML_MALFORMED otherwise.
         */
        QStatus ValidateDenyAnnotation(std::unordered_set<std::string>& presentAnnotations);

        /**
         * Validates the XML member's name is correct according to the schema.
         *
         * @param[in]    name    Member's name.
         *
         * @return
         *            #ER_OK if the input is correct.
         *            #ER_FAIL otherwise.
         */
        QStatus ValidateMemberName(AJ_PCSTR name);

        /**
         * Validates the XML member's action mask is allowed for the given member type.
         *
         * @param[in]    actionMask  Member's action mask.
         *
         * @return
         *            #ER_OK if the input is correct.
         *            #ER_FAIL otherwise.
         */
        QStatus ValidateActionMask(uint8_t actionMask);
    };

    class MethodsValidator : public MemberValidator {
      public:

        /*
         * Initializes the static members.
         */
        static void Init();

      private:

        /**
         * A string -> action mask map of actions valid for methods.
         */
        static std::unordered_map<std::string, uint8_t> s_actionsMap;

        virtual const std::unordered_map<std::string, uint8_t>& GetActionsMap();

        virtual uint8_t GetValidActions();
    };

    class PropertiesValidator : public MemberValidator {
      public:

        /*
         * Initializes the static members.
         */
        static void Init();

      private:

        /**
         * A string -> action mask map of actions valid for properties.
         */
        static std::unordered_map<std::string, uint8_t> s_actionsMap;

        virtual const std::unordered_map<std::string, uint8_t>& GetActionsMap();

        virtual uint8_t GetValidActions();
    };

    class SignalsValidator : public MemberValidator {
      public:

        /*
         * Initializes the static members.
         */
        static void Init();

      private:

        /**
         * A string -> action mask map of actions valid for signals.
         */
        static std::unordered_map<std::string, uint8_t> s_actionsMap;

        virtual const std::unordered_map<std::string, uint8_t>& GetActionsMap();

        virtual uint8_t GetValidActions();
    };

    /*
     * A factory class to return proper MemberValidators according to their type.
     */
    class MemberValidatorFactory {
      public:

        MemberValidatorFactory()
        {
            m_validators[PermissionPolicy::Rule::Member::MemberType::METHOD_CALL] = new MethodsValidator();
            m_validators[PermissionPolicy::Rule::Member::MemberType::PROPERTY] = new PropertiesValidator();
            m_validators[PermissionPolicy::Rule::Member::MemberType::SIGNAL] = new SignalsValidator();
        }

        ~MemberValidatorFactory()
        {
            for (auto validatorsElement : m_validators) {
                delete validatorsElement.second;
            }
        }

        /**
         * Constructs a validator for given member type.
         *
         * @param[in]    type  Member's type.
         *
         * @return A validator for given member type.
         */
        MemberValidator* ForType(PermissionPolicy::Rule::Member::MemberType type);

      private:

        /**
         * Member validators returned by the factory.
         */
        std::map<PermissionPolicy::Rule::Member::MemberType, MemberValidator*> m_validators;
    };
};
} /* namespace ajn */
#endif
