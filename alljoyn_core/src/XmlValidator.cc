/**
 * @file
 * XmlValidatorHelper is a base class for XML documents validation
 */

/******************************************************************************
 * Copyright (c) 2016 Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright 2016 Open Connectivity Foundation and Contributors to
 *    AllSeen Alliance. All rights reserved.
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

#include "XmlValidator.h"

#define QCC_MODULE "XML_CONVERTER"

#define DBUS_ANNOTATION_ELEMENT_NAME "annotation"

using namespace qcc;
using namespace std;

namespace ajn {

QStatus XmlValidator::ValidateNameAttributeValue(const XmlElement* xmlElement, AJ_PCSTR name)
{
    string nameAttribute;
    ExtractAttributeOrWildcard(xmlElement, NAME_XML_ATTRIBUTE, &nameAttribute);

    if (nameAttribute != name) {
        QCC_LogError(ER_XML_INVALID_ATTRIBUTE_VALUE,
                     ("%s: Unexpected \"%s\" element's \"name\" attribute value. Expected: %s. Was: %s.",
                      __FUNCTION__, xmlElement->GetName().c_str(), name, nameAttribute.c_str()));

        return ER_XML_INVALID_ATTRIBUTE_VALUE;
    }

    return ER_OK;
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

    QStatus status = InsertUniqueOrFail(attribute, valuesSet);
    if (ER_OK != status) {
        QCC_LogError(status,
                     ("%s: The \"%s\" element's attribute value(%s) not unique.",
                      __FUNCTION__, xmlElement->GetName().c_str(), attribute.c_str()));

        return status;
    }

    return ER_OK;
}

#ifdef REGEX_SUPPORTED

QStatus XmlValidator::ValidateNameAttributeValue(const XmlElement* xmlElement, const regex& namePattern, size_t maxNameLength)
{
    string nameAttribute;
    ExtractAttributeOrWildcard(xmlElement, NAME_XML_ATTRIBUTE, &nameAttribute);

    QStatus status = ValidateString(nameAttribute, namePattern, maxNameLength);
    if (ER_OK != status) {
        QCC_LogError(ER_XML_INVALID_ATTRIBUTE_VALUE,
                     ("%s: The \"%s\" element's \"name\" attribute value(%s) did not match the expected pattern or exceeded %u characters.",
                      __FUNCTION__, xmlElement->GetName().c_str(), nameAttribute.c_str(), maxNameLength));

        return ER_XML_INVALID_ATTRIBUTE_VALUE;
    }

    return ER_OK;
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
    String actualName = xmlElement->GetName();
    if (actualName != name) {
        QCC_LogError(ER_XML_INVALID_ELEMENT_NAME,
                     ("%s: Unexpected XML element name. Expected: %s. Was: %s.",
                      __FUNCTION__, name, actualName.c_str()));

        return ER_XML_INVALID_ELEMENT_NAME;
    }

    return ER_OK;
}

QStatus XmlValidator::ValidateChildrenCountPositive(const XmlElement* xmlElement)
{
    if (xmlElement->GetChildren().size() == 0) {
        QCC_LogError(ER_XML_INVALID_ELEMENT_CHILDREN_COUNT,
                     ("%s: XML element \"%s\" should have at least one child.",
                      __FUNCTION__, xmlElement->GetName().c_str()));

        return ER_XML_INVALID_ELEMENT_CHILDREN_COUNT;
    }

    return ER_OK;
}

QStatus XmlValidator::ValidateChildrenCountEqual(const XmlElement* xmlElement, size_t expectedChildrenCount)
{
    size_t childrenCount = xmlElement->GetChildren().size();
    if (childrenCount != expectedChildrenCount) {
        QCC_LogError(ER_XML_INVALID_ELEMENT_CHILDREN_COUNT,
                     ("%s: XML element \"%s\" has got an invalid number of children. Expected: %u. Was: %u.",
                      __FUNCTION__, xmlElement->GetName().c_str(), expectedChildrenCount, childrenCount));

        return ER_XML_INVALID_ELEMENT_CHILDREN_COUNT;
    }

    return ER_OK;
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