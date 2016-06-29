#ifndef _ALLJOYN_XML_VALIDATOR_H
#define _ALLJOYN_XML_VALIDATOR_H
/**
 * @file
 * This file defines a base class for XML validators.
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
#error Only include XmlValidator.h in C++ code.
#endif

#include <unordered_set>
#include <unordered_map>
#include <string>

#include <qcc/platform.h>
#include <qcc/XmlElement.h>
#include <alljoyn/Status.h>

/*
 * Workaround for ASACORE-2712
 */
#if !defined(__GNUC__) || \
    ((__GNUC__ >= 4) && \
    (__GNUC_MINOR__ >= 9))
#define REGEX_SUPPORTED
#endif

#ifdef REGEX_SUPPORTED
#include <regex>
#endif /* REGEX_SUPPORTED */

#define MANIFEST_XML_ELEMENT "manifest"
#define RULES_XML_ELEMENT "rules"
#define NAME_XML_ATTRIBUTE "name"
#define WILDCARD_XML_VALUE "*"

namespace ajn {

class XmlValidator {
  public:

    /**
     * Default destructor.
     */
    virtual ~XmlValidator() { }

    /**
     * Retrieves "annotation" and other XML elements to separate containers.
     *
     * @param[in]   xmlElement   XML element to process.
     * @param[out]  annotations  Vector to store extracted annotations.
     * @param[out]  other        Vector to store other extracted XML elements.
     */
    static void SeparateAnnotations(const qcc::XmlElement* xmlElement, std::vector<qcc::XmlElement*>& annotations, std::vector<qcc::XmlElement*>& other);

    /**
     * Helper method to extract an attribute from qcc::XmlElement or
     * a wildcard "*" if the argument is not present.
     *
     * @param[in]   xmlElement       XML element to extract the attribute from.
     * @param[in]   attributeName    Name of the extracted attribute.
     * @param[out]  attribute        The extracted attribute value.
     */
    static void ExtractAttributeOrWildcard(const qcc::XmlElement* xmlElement, AJ_PCSTR attributeName, std::string* attribute);

  protected:

    /**
     * Helper method to determine if the input XML element is an annotation in D-Bus sense.
     *
     * @param[in]   xmlElement Input XML element.
     *
     * @return True if the XML element is an "annotation" element.
     */
    static bool IsAnnotation(const qcc::XmlElement* xmlElement);

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
    static QStatus ValidateAttributeValueUnique(const qcc::XmlElement* xmlElement, std::unordered_set<std::string>& valuesSet, AJ_PCSTR attributeName);

    /**
     * Validates if the qcc::XmlElement's name is equal to the expected name.
     *
     * @param[in]    xmlElement  Xml element being verified.
     * @param[in]    name        Expeceted element name.
     *
     * @return
     *            #ER_OK if the input is correct.
     *            #ER_XML_MALFORMED otherwise.
     */
    static QStatus ValidateElementName(const qcc::XmlElement* xmlElement, AJ_PCSTR name);

    /**
     * Validates if the qcc::XmlElement contains at least one child.
     *
     * @param[in]    xmlElement              Xml element being verified.
     *
     * @return
     *            #ER_OK if the input is correct.
     *            #ER_XML_MALFORMED otherwise.
     */
    static QStatus ValidateChildrenCountPositive(const qcc::XmlElement* xmlElement);

    /**
     * Validates if the qcc::XmlElement contains the correct number of children.
     *
     * @param[in]    xmlElement              Xml element being verified.
     * @param[in]    expectedChildrenCount   Expeceted number of children elements.
     *
     * @return
     *            #ER_OK if the input is correct.
     *            #ER_XML_MALFORMED otherwise.
     */
    static QStatus ValidateChildrenCountEqual(const qcc::XmlElement* xmlElement, uint8_t expectedChildrenCount);

    /**
     * Helper function to add a value to the set or detect duplicates.
     *
     * @param[in]    value     The value that is supposed to be added to the set.
     * @param[out]   valuesSet Set accepting the new value.
     *
     * @return #ER_OK if successfull.
     *         #ER_FAUL if a value already exists in the set.
     */
    static QStatus InsertUniqueOrFail(const std::string& value, std::unordered_set<std::string>& valuesSet);
};
} /* namespace ajn */
#endif
