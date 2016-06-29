/**
 * @file
 * XmlValidatorHelper is a base class for XML documents validation
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

#include "XmlValidator.h"

#define DBUS_ANNOTATION_ELEMENT_NAME "annotation"

using namespace qcc;
using namespace std;

namespace ajn {

QStatus XmlValidator::ValidateNameAttributeValue(const XmlElement* xmlElement, AJ_PCSTR name)
{
    string nameAttribute;
    ExtractAttributeOrWildcard(xmlElement, NAME_XML_ATTRIBUTE, &nameAttribute);

    return (nameAttribute == name) ? ER_OK : ER_FAIL;
}

void XmlValidator::SeparateAnnotations(const XmlElement* xmlElement, vector<XmlElement*>& annotations, vector<XmlElement*>& other)
{
    for (auto child : xmlElement->GetChildren()) {
        if (IsAnnotation(child)) {
            annotations.push_back(child);
        } else {
            other.push_back(child);
        }
    }
}

void XmlValidator::ExtractAttributeOrWildcard(const XmlElement* xmlElement, AJ_PCSTR attributeName, string* attribute)
{
    *attribute = xmlElement->GetAttribute(attributeName).c_str();
    if (attribute->empty()) {
        *attribute = WILDCARD_XML_VALUE;
    }
}

bool XmlValidator::IsAnnotation(const XmlElement* xmlElement)
{
    return xmlElement->GetName() == DBUS_ANNOTATION_ELEMENT_NAME;
}

QStatus XmlValidator::ValidateAttributeValueUnique(const XmlElement* xmlElement, unordered_set<string>& valuesSet, AJ_PCSTR attributeName)
{
    string attribute;
    ExtractAttributeOrWildcard(xmlElement, attributeName, &attribute);

    return InsertUniqueOrFail(attribute, valuesSet);
}

#ifdef REGEX_SUPPORTED

QStatus XmlValidator::ValidateNameAttributeValue(const XmlElement* xmlElement, const regex& namePattern, size_t maxNameLength)
{
    string nameAttribute;
    ExtractAttributeOrWildcard(xmlElement, NAME_XML_ATTRIBUTE, &nameAttribute);

    return ValidateString(nameAttribute, namePattern, maxNameLength);
}

QStatus XmlValidator::ValidateString(string input, const regex& pattern, size_t maxLength)
{
    if (input.empty() ||
        (input.size() > maxLength) ||
        !regex_match(input, pattern)) {
        return ER_FAIL;
    }

    return ER_OK;
}

#endif // REGEX_SUPPORTED

QStatus XmlValidator::ValidateElementName(const XmlElement* xmlElement, AJ_PCSTR name)
{
    return (xmlElement->GetName() == name) ? ER_OK : ER_XML_MALFORMED;
}

QStatus XmlValidator::ValidateChildrenCountPositive(const XmlElement* xmlElement)
{
    return (xmlElement->GetChildren().size() > 0) ? ER_OK : ER_XML_MALFORMED;
}

QStatus XmlValidator::ValidateChildrenCountEqual(const XmlElement* xmlElement, uint8_t expectedChildrenCount)
{
    return (xmlElement->GetChildren().size() == expectedChildrenCount) ? ER_OK : ER_XML_MALFORMED;
}

QStatus XmlValidator::InsertUniqueOrFail(const string& value, unordered_set<string>& valuesSet)
{
    if (valuesSet.find(value) == valuesSet.end()) {
        valuesSet.insert(value);
    } else {
        return ER_FAIL;
    }

    return ER_OK;
}
}