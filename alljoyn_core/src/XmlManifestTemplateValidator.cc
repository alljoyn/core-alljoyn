/**
 * @file
 * XmlManifestTemplateValidator is a class handling validation of manifest templates
 * in XML format.
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

#include <alljoyn/Status.h>
#include <qcc/StringSource.h>
#include <qcc/StringUtil.h>
#include "KeyInfoHelper.h"
#include "XmlManifestTemplateValidator.h"

#define QCC_MODULE "XML_CONVERTER"

using namespace qcc;
using namespace std;

namespace ajn {

XmlManifestTemplateValidator* XmlManifestTemplateValidator::s_validator = nullptr;
map<string, PermissionPolicy::Rule::SecurityLevel>* XmlManifestTemplateValidator::s_securityLevelMap = nullptr;

void XmlManifestTemplateValidator::Init()
{
    QCC_DbgTrace(("%s: Performing validator init.", __FUNCTION__));

    QCC_ASSERT((nullptr == s_validator) && (nullptr == s_securityLevelMap));
    s_validator = new XmlManifestTemplateValidator();

    s_securityLevelMap = new map<string, PermissionPolicy::Rule::SecurityLevel>();
    (*s_securityLevelMap)[PRIVILEGED_SECURITY_LEVEL] = PermissionPolicy::Rule::SecurityLevel::PRIVILEGED;
    (*s_securityLevelMap)[NON_PRIVILEGED_SECURITY_LEVEL] = PermissionPolicy::Rule::SecurityLevel::NON_PRIVILEGED;
    (*s_securityLevelMap)[UNAUTHENTICATED_SECURITY_LEVEL] = PermissionPolicy::Rule::SecurityLevel::UNAUTHENTICATED;
}

void XmlManifestTemplateValidator::Shutdown()
{
    QCC_DbgTrace(("%s: Performing validator cleanup.", __FUNCTION__));

    delete s_validator;
    s_validator = nullptr;

    delete s_securityLevelMap;
    s_securityLevelMap = nullptr;
}

XmlManifestTemplateValidator* XmlManifestTemplateValidator::GetInstance()
{
    return s_validator;
}

string XmlManifestTemplateValidator::GetRootElementName()
{
    return MANIFEST_XML_ELEMENT;
}

QStatus XmlManifestTemplateValidator::ValidateNodeAnnotations(const vector<XmlElement*>& annotations)
{
    size_t annotationsSize = annotations.size();

    if (annotationsSize >= 2) {
        QCC_LogError(ER_XML_INVALID_ANNOTATIONS_COUNT,
                     ("%s: Node contains more than one (%u) annotation.",
                      __FUNCTION__, annotationsSize));

        return ER_XML_INVALID_ANNOTATIONS_COUNT;
    }

    return annotations.empty() ? ER_OK : ValidateSecurityLevelAnnotation(annotations[0]);
}

QStatus XmlManifestTemplateValidator::ValidateInterfaceAnnotations(const vector<XmlElement*>& annotations)
{
    // They share the same constraints.
    return ValidateNodeAnnotations(annotations);
}

QStatus XmlManifestTemplateValidator::ValidateSecurityLevelAnnotation(const XmlElement* annotation)
{
    QStatus status = ValidateElementName(annotation, ANNOTATION_XML_ELEMENT);
    if (ER_OK != status) {
        return status;
    }

    status = ValidateNameAttributeValue(annotation, SECURITY_LEVEL_ANNOTATION_NAME);
    if (ER_OK != status) {
        return status;
    }

    return ValidateSecurityLevelAnnotationValue(annotation);
}
QStatus XmlManifestTemplateValidator::ValidateSecurityLevelAnnotationValue(const XmlElement* annotation)
{
    String securityLevel = annotation->GetAttribute(VALUE_XML_ATTRIBUTE);
    auto foundSecurityLevel = s_securityLevelMap->find(securityLevel.c_str());

    if (foundSecurityLevel == s_securityLevelMap->end()) {
        QCC_LogError(ER_XML_INVALID_SECURITY_LEVEL_ANNOTATION_VALUE,
                     ("%s: Unexpected security level value(%s).",
                      __FUNCTION__, securityLevel.c_str()));

        return ER_XML_INVALID_SECURITY_LEVEL_ANNOTATION_VALUE;
    }

    return ER_OK;
}
}