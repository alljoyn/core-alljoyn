/**
 * @file
 * XmlManifestTemplateValidator is a class handling validation of manifest templates
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
#include "XmlManifestTemplateValidator.h"

using namespace qcc;
using namespace std;

namespace ajn {

XmlManifestTemplateValidator* XmlManifestTemplateValidator::s_validator = nullptr;
map<string, PermissionPolicy::Rule::SecurityLevel> XmlManifestTemplateValidator::s_securityLevelMap;

void XmlManifestTemplateValidator::Init()
{
    QCC_ASSERT(nullptr == s_validator);
    s_validator = new XmlManifestTemplateValidator();

    s_securityLevelMap[PRIVILEGED_SECURITY_LEVEL] = PermissionPolicy::Rule::SecurityLevel::PRIVILEGED;
    s_securityLevelMap[NON_PRIVILEGED_SECURITY_LEVEL] = PermissionPolicy::Rule::SecurityLevel::NON_PRIVILEGED;
    s_securityLevelMap[UNAUTHORIZED_SECURITY_LEVEL] = PermissionPolicy::Rule::SecurityLevel::UNAUTHORIZED;
}

void XmlManifestTemplateValidator::Shutdown()
{
    delete s_validator;
    s_validator = nullptr;
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
    QStatus status = (annotationsSize < 2) ? ER_OK : ER_XML_MALFORMED;

    if ((ER_OK == status) && (annotationsSize > 0)) {
        status = ValidateSecurityLevelAnnotation(annotations[0]);
    }

    return status;
}

QStatus XmlManifestTemplateValidator::ValidateInterfaceAnnotations(const vector<XmlElement*>& annotations)
{
    // They share the same constraints.
    return ValidateNodeAnnotations(annotations);
}

QStatus XmlManifestTemplateValidator::ValidateSecurityLevelAnnotation(const XmlElement* annotation)
{
    QStatus status = ValidateElementName(annotation, ANNOTATION_XML_ELEMENT);

    if (ER_OK == status) {
        status = ValidateNameAttributeValue(annotation, SECURITY_LEVEL_ANNOTATION_NAME);
    }

    if (ER_OK == status) {
        status = ValidateSecurityLevelAnnotationValue(annotation);
    }

    return status;
}
QStatus XmlManifestTemplateValidator::ValidateSecurityLevelAnnotationValue(const XmlElement* annotation)
{
    String securityLevel = annotation->GetAttribute(VALUE_XML_ATTRIBUTE);
    auto foundSecurityLevel = s_securityLevelMap.find(securityLevel.c_str());

    return (foundSecurityLevel == s_securityLevelMap.end()) ? ER_XML_MALFORMED : ER_OK;
}
}