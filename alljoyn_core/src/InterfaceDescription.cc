/*
 * @file
 *
 * This file implements the InterfaceDescription class
 */

/******************************************************************************
 * Copyright (c) 2009-2011,2013,2014 AllSeen Alliance. All rights reserved.
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

#include <qcc/platform.h>
#include <qcc/String.h>
#include <qcc/StringMapKey.h>
#include <qcc/XmlElement.h>
#include <map>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/Status.h>

#include "SignatureUtils.h"

#define QCC_MODULE "ALLJOYN"

using namespace std;
using namespace qcc;

namespace ajn {


static size_t GetAnnotationsWithValues(
    const std::map<qcc::String, qcc::String>& annotations,
    qcc::String* names,
    qcc::String* values,
    size_t size)
{
    size_t count = annotations.size();

    if (names && values) {
        typedef std::map<qcc::String, qcc::String> AnnotationsMap;
        count = std::min(count, size);
        AnnotationsMap::const_iterator mit = annotations.begin();
        for (size_t i = 0; i < count && mit != annotations.end(); ++i, ++mit) {
            names[i] = mit->first;
            values[i] = mit->second;
        }
    }

    return count;
}

class InterfaceDescription::AnnotationsMap : public std::map<qcc::String, qcc::String> { };

class InterfaceDescription::ArgumentDescriptions : public std::map<qcc::String, qcc::String> { };

qcc::String InterfaceDescription::NextArg(const char*& signature, qcc::String& argNames, bool inOut,
                                          size_t indent, Member const& member, bool withDescriptions,
                                          const char* langTag, Translator* translator) const
{
    qcc::String name;
    qcc::String in(indent, ' ');
    qcc::String arg = in + "<arg";
    qcc::String argType;
    const char* start = signature;
    SignatureUtils::ParseCompleteType(signature);
    size_t len = signature - start;
    argType.append(start, len);

    if (!argNames.empty()) {
        size_t pos = argNames.find_first_of(',');
        name = argNames.substr(0, pos);
        arg += " name=\"" + name + "\"";
        if (pos == qcc::String::npos) {
            argNames.clear();
        } else {
            argNames.erase(0, pos + 1);
        }
    }
    arg += " type=\"" + argType + "\" direction=\"";
    arg += inOut ? "in\"" : "out\"";

    const char* myDesc = NULL;
    if (withDescriptions) {
        ArgumentDescriptions::const_iterator search = member.argumentDescriptions->find(name);
        if (member.argumentDescriptions->end() != search) {
            myDesc = search->second.c_str();
        }
    }
    if (myDesc) {
        arg += ">\n";
        AppendDescriptionXml(arg, langTag, myDesc, translator, in);
        arg += in + "</arg>\n";
    } else {
        arg += "/>\n";
    }
    return arg;
}

void InterfaceDescription::AppendDescriptionXml(qcc::String& xml, const char* language,
                                                const char* localDescription, Translator* translator, qcc::String const& indent) const
{
    qcc::String buffer;
    const char* d = Translate(language, localDescription, buffer, translator);
    if (!d || d[0] == '\0') {
        return;
    }
    xml += indent + "  <description>" + XmlElement::EscapeXml(d) + "</description>\n";
}

InterfaceDescription::Member::Member(const InterfaceDescription* iface,
                                     AllJoynMessageType type,
                                     const char* name,
                                     const char* signature,
                                     const char* returnSignature,
                                     const char* argNames,
                                     uint8_t annotation,
                                     const char* accessPerms) :
    iface(iface),
    memberType(type),
    name(name),
    signature(signature ? signature : ""),
    returnSignature(returnSignature ? returnSignature : ""),
    argNames(argNames ? argNames : ""),
    annotations(new AnnotationsMap()),
    accessPerms(accessPerms ? accessPerms : ""),
    description(),
    argumentDescriptions(new ArgumentDescriptions()),
    isSessionlessSignal(false)
{

    if (annotation & MEMBER_ANNOTATE_DEPRECATED) {
        (*annotations)[org::freedesktop::DBus::AnnotateDeprecated] = "true";
    }

    if (annotation & MEMBER_ANNOTATE_NO_REPLY) {
        (*annotations)[org::freedesktop::DBus::AnnotateNoReply] = "true";
    }
}

InterfaceDescription::Member::Member(const Member& other)
    : iface(other.iface),
    memberType(other.memberType),
    name(other.name),
    signature(other.signature),
    returnSignature(other.returnSignature),
    argNames(other.argNames),
    annotations(new AnnotationsMap(*(other.annotations))),
    accessPerms(other.accessPerms),
    description(other.description),
    argumentDescriptions(new ArgumentDescriptions(*(other.argumentDescriptions))),
    isSessionlessSignal(other.isSessionlessSignal)
{
}

InterfaceDescription::Member& InterfaceDescription::Member::operator=(const Member& other)
{
    if (this != &other) {
        iface = other.iface;
        memberType = other.memberType;
        name = other.name;
        signature = other.signature;
        returnSignature = other.returnSignature;
        argNames = other.argNames;
        delete annotations;
        annotations = new AnnotationsMap(*(other.annotations));
        accessPerms = other.accessPerms;
        description = other.description;
        *argumentDescriptions = *(other.argumentDescriptions);
        isSessionlessSignal = other.isSessionlessSignal;
    }
    return *this;
}

InterfaceDescription::Member::~Member()
{
    delete annotations;
    delete argumentDescriptions;
}

size_t InterfaceDescription::Member::GetAnnotations(qcc::String* names, qcc::String* values, size_t size) const
{
    return GetAnnotationsWithValues(*annotations, names, values, size);
}

bool InterfaceDescription::Member::operator==(const Member& o) const {
    return ((memberType == o.memberType) && (name == o.name) && (signature == o.signature)
            && (returnSignature == o.returnSignature) && (*annotations == *(o.annotations))
            && (description == o.description) && (*argumentDescriptions == *(o.argumentDescriptions))
            && (isSessionlessSignal == o.isSessionlessSignal));
}


InterfaceDescription::Property::Property(const char* name, const char* signature, uint8_t access) :
    name(name),
    signature(signature ? signature : ""),
    access(access),
    annotations(new AnnotationsMap()),
    description()
{
}


InterfaceDescription::Property::Property(const char* name, const char* signature, uint8_t access, uint8_t annotation) :
    name(name),
    signature(signature ? signature : ""),
    access(access),
    annotations(new AnnotationsMap())
{
    if (annotation & PROP_ANNOTATE_EMIT_CHANGED_SIGNAL) {
        (*annotations)[org::freedesktop::DBus::AnnotateEmitsChanged] = "true";
    }
    if (annotation & PROP_ANNOTATE_EMIT_CHANGED_SIGNAL_INVALIDATES) {
        (*annotations)[org::freedesktop::DBus::AnnotateEmitsChanged] = "invalidates";
    }
}

InterfaceDescription::Property::Property(const Property& other)
    : name(other.name),
    signature(other.signature),
    access(other.access),
    annotations(new AnnotationsMap(*(other.annotations))),
    description(other.description)
{

}

InterfaceDescription::Property& InterfaceDescription::Property::operator=(const Property& other)
{
    if (this != &other) {
        name = other.name;
        signature = other.signature;
        access = other.access;
        delete annotations;
        annotations = new AnnotationsMap(*(other.annotations));
        description = other.description;
    }
    return *this;
}

InterfaceDescription::Property::~Property()
{
    delete annotations;
}

size_t InterfaceDescription::Property::GetAnnotations(qcc::String* names, qcc::String* values, size_t size) const
{
    return GetAnnotationsWithValues(*annotations, names, values, size);
}

/** Equality */
bool InterfaceDescription::Property::operator==(const Property& o) const {
    return ((name == o.name) && (signature == o.signature) && (access == o.access) && (*annotations == *(o.annotations))
            && description == o.description);
}


struct InterfaceDescription::Definitions {
    typedef std::map<qcc::StringMapKey, Member> MemberMap;
    typedef std::map<qcc::StringMapKey, Property> PropertyMap;

    MemberMap members;              /**< Interface members */
    PropertyMap properties;         /**< Interface properties */
    AnnotationsMap annotations;     /**< Interface Annotations */
    qcc::String languageTag;
    qcc::String description;
    Translator* translator;
    bool hasDescription;


    Definitions() :
        translator(NULL), hasDescription(false)
    { }

    Definitions(const MemberMap& m, const PropertyMap& p, const AnnotationsMap& a,
                const qcc::String& langTag, const qcc::String& desc, Translator* dt, bool hd) :
        members(m), properties(p), annotations(a),
        languageTag(langTag), description(desc), translator(dt), hasDescription(hd)
    { }
};

bool InterfaceDescription::Member::GetAnnotation(const qcc::String& name, qcc::String& value) const
{
    AnnotationsMap::const_iterator it = annotations->find(name);
    return (it != annotations->end() ? value = it->second, true : false);
}

bool InterfaceDescription::Property::GetAnnotation(const qcc::String& name, qcc::String& value) const
{
    AnnotationsMap::const_iterator it = annotations->find(name);
    return (it != annotations->end() ? value = it->second, true : false);
}

InterfaceDescription::InterfaceDescription(const char* name, InterfaceSecurityPolicy secPolicy) :
    defs(new Definitions),
    name(name),
    isActivated(false),
    secPolicy(secPolicy)
{
    if (secPolicy != AJ_IFC_SECURITY_INHERIT) {
        /*
         * We don't allow a secure annotation on the standard DBus Interfaces
         */
        if ((strcmp(name, org::freedesktop::DBus::Introspectable::InterfaceName) != 0) &&
            (strcmp(name, org::freedesktop::DBus::Peer::InterfaceName)           != 0) &&
            (strcmp(name, org::freedesktop::DBus::Properties::InterfaceName)     != 0)) {
            defs->annotations[org::alljoyn::Bus::Secure] = (secPolicy == AJ_IFC_SECURITY_REQUIRED) ? "true" : "off";
        }
    }
}

InterfaceDescription::~InterfaceDescription()
{
    delete defs;
}

InterfaceDescription::InterfaceDescription(const InterfaceDescription& other) :
    defs(new Definitions(other.defs->members, other.defs->properties, other.defs->annotations,
                         other.defs->languageTag, other.defs->description, other.defs->translator, other.defs->hasDescription)),
    name(other.name),
    isActivated(false),
    secPolicy(other.secPolicy)
{
    /* Update the iface pointer in each member */
    Definitions::MemberMap::iterator mit = defs->members.begin();
    while (mit != defs->members.end()) {
        mit++->second.iface = this;
    }
}

InterfaceDescription& InterfaceDescription::operator=(const InterfaceDescription& other)
{
    if (this != &other) {
        name = other.name;
        secPolicy = other.secPolicy;
        isActivated = false;
        defs->members = other.defs->members;
        defs->properties = other.defs->properties;
        defs->annotations = other.defs->annotations;
        defs->languageTag = other.defs->languageTag;
        defs->description = other.defs->description;
        defs->translator = other.defs->translator;

        /* Update the iface pointer in each member */
        Definitions::MemberMap::iterator mit = defs->members.begin();
        while (mit != defs->members.end()) {
            mit++->second.iface = this;
        }
    }
    return *this;
}

qcc::String InterfaceDescription::Introspect(size_t indent, const char* languageTag, Translator* translator) const
{
    qcc::String in(indent, ' ');
    const qcc::String close = "\">\n";

    Translator* myTranslator = defs->translator ? defs->translator : translator;

    bool withDescriptions = languageTag && defs->hasDescription;

    qcc::String xml = in + "<interface name=\"";
    xml += name + close;

    if (withDescriptions) {
        AppendDescriptionXml(xml, languageTag, defs->description.c_str(), myTranslator, in);
    }

    /*
     * Iterate over interface defs->members
     */
    Definitions::MemberMap::const_iterator mit = defs->members.begin();
    while (mit != defs->members.end()) {
        const Member& member = mit->second;
        qcc::String argNames = member.argNames;

        bool isMethod = (member.memberType == MESSAGE_METHOD_CALL);
        qcc::String mtype = isMethod ? "method" : "signal";
        xml += in + "  <" + mtype + " name=\"" + member.name;
        if (withDescriptions && !isMethod) {
            xml += "\" sessionless=\"" + (member.isSessionlessSignal ? qcc::String("true") : qcc::String("false"));
        }
        xml     += close;

        if (withDescriptions) {
            AppendDescriptionXml(xml, languageTag, member.description.c_str(), myTranslator, in + "  ");
        }

        /* Iterate over IN arguments */
        for (const char* sig = member.signature.c_str(); *sig;) {
            // always treat signals as direction=out
            xml += NextArg(sig, argNames, member.memberType != MESSAGE_SIGNAL,
                           indent + 4, member, withDescriptions, languageTag, myTranslator);
        }
        /* Iterate over OUT arguments */
        for (const char* sig = member.returnSignature.c_str(); *sig;) {
            xml += NextArg(sig, argNames, false, indent + 4, member, withDescriptions, languageTag, myTranslator);
        }
        /*
         * Add annotations
         */

        AnnotationsMap::const_iterator ait = member.annotations->begin();
        for (; ait != member.annotations->end(); ++ait) {
            xml += in + "    <annotation name=\"" + ait->first.c_str() + "\" value=\"" + ait->second + "\"/>\n";
        }

        xml += in + "  </" + mtype + ">\n";
        ++mit;
    }
    /*
     * Iterate over interface properties
     */
    Definitions::PropertyMap::const_iterator pit = defs->properties.begin();
    while (pit != defs->properties.end()) {
        const Property& property = pit->second;
        xml += in + "  <property name=\"" + property.name + "\" type=\"" + property.signature + "\"";
        if (property.access == PROP_ACCESS_READ) {
            xml += " access=\"read\"";
        } else if (property.access == PROP_ACCESS_WRITE) {
            xml += " access=\"write\"";
        } else {
            xml += " access=\"readwrite\"";
        }

        //Does this property have a description? Only if (a) we're doing descriptions,
        //(b) the property has some description text, and (c) that text is not a
        //lookup key (empty language tag)  with no Translator to produce a description string
        bool propWithDescription = withDescriptions && !property.description.empty()
                                   && !(defs->languageTag.empty() && !myTranslator);

        //Does this property element have any sub-elements?
        if (property.annotations->size() || propWithDescription) {
            xml += ">\n";

            if (property.annotations->size()) {
                // add annotations
                AnnotationsMap::const_iterator ait = property.annotations->begin();
                for (; ait != property.annotations->end(); ++ait) {
                    xml += in + "    <annotation name=\"" + ait->first.c_str() + "\" value=\"" + ait->second + "\"/>\n";
                }
            }

            if (propWithDescription) {
                AppendDescriptionXml(xml, languageTag, property.description.c_str(), myTranslator, in + "  ");
            }
            xml += in + "  </property>\n";
        } else {
            xml += "/>\n";
        }

        ++pit;
    }

    // add interface annotations
    AnnotationsMap::const_iterator ait = defs->annotations.begin();
    for (; ait != defs->annotations.end(); ++ait) {
        xml += in + "  <annotation name=\"" + ait->first.c_str() + "\" value=\"" + ait->second + "\"/>\n";
    }

    xml += in + "</interface>\n";
    return xml;
}

QStatus InterfaceDescription::AddMember(AllJoynMessageType type,
                                        const char* name,
                                        const char* inSig,
                                        const char* outSig,
                                        const char* argNames,
                                        uint8_t annotation,
                                        const char* accessPerms)
{
    if (isActivated) {
        return ER_BUS_INTERFACE_ACTIVATED;
    }

    StringMapKey key = qcc::String(name);
    Member member(this, type, name, inSig, outSig, argNames, annotation, accessPerms);
    pair<StringMapKey, Member> item(key, member);
    pair<Definitions::MemberMap::iterator, bool> ret = defs->members.insert(item);
    return ret.second ? ER_OK : ER_BUS_MEMBER_ALREADY_EXISTS;
}

QStatus InterfaceDescription::AddMemberAnnotation(const char* member, const qcc::String& name, const qcc::String& value)
{
    if (isActivated) {
        return ER_BUS_INTERFACE_ACTIVATED;
    }

    Definitions::MemberMap::iterator it = defs->members.find(StringMapKey(member));
    if (it == defs->members.end()) {
        return ER_BUS_INTERFACE_NO_SUCH_MEMBER;
    }

    Member& m = it->second;
    std::pair<AnnotationsMap::iterator, bool> ret = m.annotations->insert(AnnotationsMap::value_type(name, value));
    return (ret.second || (ret.first->first == name && ret.first->second == value)) ? ER_OK : ER_BUS_ANNOTATION_ALREADY_EXISTS;
}

bool InterfaceDescription::GetMemberAnnotation(const char* member, const qcc::String& name, qcc::String& value) const
{
    Definitions::MemberMap::const_iterator mit = defs->members.find(StringMapKey(member));
    if (mit == defs->members.end()) {
        return false;
    }

    const Member& m = mit->second;
    AnnotationsMap::const_iterator ait = m.annotations->find(name);
    return (ait != m.annotations->end() ? value = ait->second, true : false);
}


QStatus InterfaceDescription::AddProperty(const char* name, const char* signature, uint8_t access)
{
    if (isActivated) {
        return ER_BUS_INTERFACE_ACTIVATED;
    }

    StringMapKey key = qcc::String(name);
    Property prop(name, signature, access);
    pair<StringMapKey, Property> item(key, prop);
    pair<Definitions::PropertyMap::iterator, bool> ret = defs->properties.insert(item);
    return ret.second ? ER_OK : ER_BUS_PROPERTY_ALREADY_EXISTS;
}

QStatus InterfaceDescription::AddPropertyAnnotation(const qcc::String& p_name, const qcc::String& name, const qcc::String& value)
{
    if (isActivated) {
        return ER_BUS_INTERFACE_ACTIVATED;
    }

    Definitions::PropertyMap::iterator pit = defs->properties.find(StringMapKey(p_name));
    if (pit == defs->properties.end()) {
        return ER_BUS_NO_SUCH_PROPERTY;
    }

    Property& property = pit->second;
    std::pair<AnnotationsMap::iterator, bool> ret = property.annotations->insert(AnnotationsMap::value_type(name, value));
    return (ret.second || (ret.first->first == name && ret.first->second == value)) ? ER_OK : ER_BUS_ANNOTATION_ALREADY_EXISTS;
}

bool InterfaceDescription::GetPropertyAnnotation(const qcc::String& p_name, const qcc::String& name, qcc::String& value) const
{
    Definitions::PropertyMap::const_iterator pit = defs->properties.find(StringMapKey(p_name));
    if (pit == defs->properties.end()) {
        return false;
    }

    const Property& property = pit->second;
    AnnotationsMap::const_iterator ait = property.annotations->find(name);
    return (ait != property.annotations->end() ? value = ait->second, true : false);
}

QStatus InterfaceDescription::AddAnnotation(const qcc::String& name, const qcc::String& value)
{
    if (isActivated) {
        return ER_BUS_INTERFACE_ACTIVATED;
    }

    std::pair<AnnotationsMap::iterator, bool> ret = defs->annotations.insert(std::make_pair(name, value));
    return (ret.second || (ret.first->first == name && ret.first->second == value)) ? ER_OK : ER_BUS_ANNOTATION_ALREADY_EXISTS;
}

bool InterfaceDescription::GetAnnotation(const qcc::String& name, qcc::String& value) const
{
    AnnotationsMap::const_iterator it = defs->annotations.find(name);
    return (it != defs->annotations.end() ? value = it->second, true : false);
}

size_t InterfaceDescription::GetAnnotations(qcc::String* names, qcc::String* values, size_t size) const
{
    return GetAnnotationsWithValues(defs->annotations, names, values, size);
}

bool InterfaceDescription::operator==(const InterfaceDescription& other) const
{
    if (name != other.name) {
        return false;
    }

    if (defs->members != other.defs->members ||
        defs->properties != other.defs->properties ||
        defs->annotations != other.defs->annotations) {
        return false;
    }

    return true;
}

size_t InterfaceDescription::GetProperties(const Property** props, size_t numProps) const
{
    size_t count = defs->properties.size();
    if (props) {
        count = min(count, numProps);
        Definitions::PropertyMap::const_iterator pit = defs->properties.begin();
        for (size_t i = 0; i < count && pit != defs->properties.end(); ++i, ++pit) {
            props[i] = &(pit->second);
        }
    }
    return count;
}

const InterfaceDescription::Property* InterfaceDescription::GetProperty(const char* name) const
{
    Definitions::PropertyMap::const_iterator pit = defs->properties.find(qcc::StringMapKey(name));
    return (pit == defs->properties.end()) ? NULL : &(pit->second);
}

size_t InterfaceDescription::GetMembers(const Member** members, size_t numMembers) const
{
    size_t count = defs->members.size();
    if (members) {
        count = min(count, numMembers);
        Definitions::MemberMap::const_iterator mit = defs->members.begin();
        for (size_t i = 0; i < count && mit != defs->members.end(); ++i, ++mit) {
            members[i] = &(mit->second);
        }
    }
    return count;
}

const InterfaceDescription::Member* InterfaceDescription::GetMember(const char* name) const
{
    Definitions::MemberMap::const_iterator mit = defs->members.find(qcc::StringMapKey(name));
    return (mit == defs->members.end()) ? NULL : &(mit->second);
}

bool InterfaceDescription::HasMember(const char* name, const char* inSig, const char* outSig)
{
    const Member* member = GetMember(name);
    if (member == NULL) {
        return false;
    } else if ((inSig == NULL) && (outSig == NULL)) {
        return true;
    } else {
        bool found = true;
        if (inSig) {
            found = found && (member->signature.compare(inSig) == 0);
        }
        if (outSig && (member->memberType == MESSAGE_METHOD_CALL)) {
            found = found && (member->returnSignature.compare(outSig) == 0);
        }
        return found;
    }
}

void InterfaceDescription::SetDescriptionLanguage(const char* language)
{
    defs->languageTag = language;
}

const char* InterfaceDescription::GetDescriptionLanguage() const
{
    return defs->languageTag.c_str();
}

void InterfaceDescription::SetDescription(const char* desc)
{
    defs->description = desc;
    defs->hasDescription = true;
}

QStatus InterfaceDescription::SetMemberDescription(const char* member, const char* desc, bool isSessionlessSignal)
{
    if (isActivated) {
        return ER_BUS_INTERFACE_ACTIVATED;
    }

    Definitions::MemberMap::iterator it = defs->members.find(StringMapKey(member));
    if (it == defs->members.end()) {
        return ER_BUS_INTERFACE_NO_SUCH_MEMBER;
    }

    Member& m = it->second;
    m.description.assign(desc);
    m.isSessionlessSignal = isSessionlessSignal;
    defs->hasDescription = true;

    return ER_OK;
}


QStatus InterfaceDescription::SetArgDescription(const char* member, const char* arg, const char* desc)
{
    if (isActivated) {
        return ER_BUS_INTERFACE_ACTIVATED;
    }

    Definitions::MemberMap::iterator it = defs->members.find(StringMapKey(member));
    if (it == defs->members.end()) {
        return ER_BUS_INTERFACE_NO_SUCH_MEMBER;
    }

    Member& m = it->second;
    m.argumentDescriptions->insert(ArgumentDescriptions::value_type(arg, desc));
    defs->hasDescription = true;

    return ER_OK;
}

QStatus InterfaceDescription::SetPropertyDescription(const char* propName, const char* desc)
{
    if (isActivated) {
        return ER_BUS_INTERFACE_ACTIVATED;
    }

    Definitions::PropertyMap::iterator pit = defs->properties.find(StringMapKey(propName));
    if (pit == defs->properties.end()) {
        return ER_BUS_NO_SUCH_PROPERTY;
    }

    Property& property = pit->second;
    property.description.assign(desc);
    defs->hasDescription = true;

    return ER_OK;
}

bool InterfaceDescription::HasDescription() const
{
    return defs->hasDescription;
}

const char* InterfaceDescription::Translate(const char* toLanguage, const char* text, qcc::String& buffer, Translator* translator) const
{
    if (NULL == text) {
        return NULL;
    }

    if (translator) {
        const char* ret = translator->Translate(defs->languageTag.c_str(), toLanguage, text, buffer);
        if (ret) {
            return ret;
        }
    }

    if (text && text[0] != '\0' && !defs->languageTag.empty()) {
        return text;
    }

    return NULL;
}

void InterfaceDescription::SetDescriptionTranslator(Translator* translator) {
    defs->translator = translator;
}

Translator* InterfaceDescription::GetDescriptionTranslator() const
{
    return defs->translator;
}

}
