/**
 * @file
 * XmlManifestTemplateConverter is a class handling conversion of rules and rules templates
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
#include "XmlManifestTemplateConverter.h"
#include "XmlManifestTemplateValidator.h"

using namespace qcc;
using namespace std;

namespace ajn {

XmlManifestTemplateConverter* XmlManifestTemplateConverter::s_converter = nullptr;
std::map<PermissionPolicy::Rule::SecurityLevel, std::string> XmlManifestTemplateConverter::s_inverseSecurityLevelMap;

void XmlManifestTemplateConverter::Init()
{
    QCC_ASSERT(nullptr == s_converter);
    s_converter = new XmlManifestTemplateConverter();

    s_inverseSecurityLevelMap[PermissionPolicy::Rule::SecurityLevel::PRIVILEGED] = PRIVILEGED_SECURITY_LEVEL;
    s_inverseSecurityLevelMap[PermissionPolicy::Rule::SecurityLevel::NON_PRIVILEGED] = NON_PRIVILEGED_SECURITY_LEVEL;
    s_inverseSecurityLevelMap[PermissionPolicy::Rule::SecurityLevel::UNAUTHENTICATED] = UNAUTHENTICATED_SECURITY_LEVEL;
}

void XmlManifestTemplateConverter::Shutdown()
{
    delete s_converter;
    s_converter = nullptr;
}

XmlManifestTemplateConverter* XmlManifestTemplateConverter::GetInstance()
{
    return s_converter;
}

XmlManifestTemplateConverter::~XmlManifestTemplateConverter()
{
}

XmlManifestTemplateConverter::XmlManifestTemplateConverter()
{
}

XmlManifestTemplateConverter::XmlManifestTemplateConverter(const XmlManifestTemplateConverter& other)
{
    QCC_UNUSED(other);
}

XmlManifestTemplateConverter& XmlManifestTemplateConverter::operator=(const XmlManifestTemplateConverter& other)
{
    QCC_UNUSED(other);
    return *this;
}

string XmlManifestTemplateConverter::GetRootElementName()
{
    return MANIFEST_XML_ELEMENT;
}

XmlRulesValidator* XmlManifestTemplateConverter::GetValidator()
{
    return XmlManifestTemplateValidator::GetInstance();
}

PermissionPolicy::Rule::RuleType XmlManifestTemplateConverter::GetRuleType()
{
    return PermissionPolicy::Rule::RuleType::MANIFEST_TEMPLATE_RULE;
}

void XmlManifestTemplateConverter::BuildXmlInterfaceAnnotations(const PermissionPolicy::Rule& rule, XmlElement* interfaceElement)
{
    AddChildAnnotation(interfaceElement, SECURITY_LEVEL_ANNOTATION_NAME, s_inverseSecurityLevelMap.at(rule.GetRecommendedSecurityLevel()).c_str());
}
}