/*
 * Copyright (c) 2011-2013, AllSeen Alliance. All rights reserved.
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
 */
#include "InterfaceDescriptionNative.h"

#include "TypeMapping.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

QStatus InterfaceDescriptionNative::CreateInterface(Plugin& plugin, BusAttachment& busAttachment, InterfaceDescriptionNative* interfaceDescriptionNative)
{
    QCC_DbgTrace(("%s", __FUNCTION__));

    qcc::String name;
    bool typeError = false;
    ajn::InterfaceDescription* interface;
    ajn::InterfaceSecurityPolicy secPolicy;
    NPVariant length;
    NPVariant method;
    NPVariant signal;
    NPVariant property;
    NPVariant npSecPolicy;
    NPVariant npname;
    QStatus status = ER_OK;

    VOID_TO_NPVARIANT(npname);
    if (!NPN_GetProperty(plugin->npp, interfaceDescriptionNative->objectValue, NPN_GetStringIdentifier("name"), &npname)) {
        status = ER_FAIL;
        QCC_LogError(status, ("Failed to get 'name' property"));
        goto exit;
    }
    name = ToDOMString(plugin, npname, typeError);
    if (typeError) {
        goto exit;
    }


    VOID_TO_NPVARIANT(npSecPolicy);
    if (!NPN_GetProperty(plugin->npp, interfaceDescriptionNative->objectValue, NPN_GetStringIdentifier("secPolicy"), &npSecPolicy)) {
        QCC_LogError(ER_FAIL, ("Failed to get 'secPolicy' property, defaulting to INHERIT"));
        INT32_TO_NPVARIANT(ajn::AJ_IFC_SECURITY_INHERIT, npSecPolicy);
    }

    if (NPVARIANT_IS_VOID(npSecPolicy)) {
        NPVariant secure;
        bool sec = false;
        QCC_DbgPrintf(("'secPolicy' property not specified, check for deprecated 'secure' property."));

        VOID_TO_NPVARIANT(secure);
        if (NPN_GetProperty(plugin->npp, interfaceDescriptionNative->objectValue, NPN_GetStringIdentifier("secure"), &secure)) {
            sec = ToBoolean(plugin, secure, typeError);
            assert(!typeError); /* ToBoolean should never fail */
            if (typeError) {
                QCC_LogError(ER_FAIL, ("ToBoolean failed"));
                goto exit;
            }
        } else {
            QCC_DbgPrintf(("Failed to get 'secure' property, defaulting secPolicy to INHERIT"));
        }

        INT32_TO_NPVARIANT(sec ? ajn::AJ_IFC_SECURITY_REQUIRED : ajn::AJ_IFC_SECURITY_INHERIT, npSecPolicy);
    }

    secPolicy = static_cast<ajn::InterfaceSecurityPolicy>(ToLong(plugin, npSecPolicy, typeError));
    if (typeError) {
        QCC_LogError(ER_FAIL, ("ToLong failed"));
        goto exit;
    }

    status = busAttachment->CreateInterface(name.c_str(), interface, secPolicy);
    if (ER_OK != status) {
        goto exit;
    }

    typedef std::map<qcc::String, qcc::String> AnnotationsMap;

    VOID_TO_NPVARIANT(method);
    VOID_TO_NPVARIANT(length);
    if (NPN_GetProperty(plugin->npp, interfaceDescriptionNative->objectValue, NPN_GetStringIdentifier("method"), &method) &&
        NPVARIANT_IS_OBJECT(method) &&
        NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(method), NPN_GetStringIdentifier("length"), &length) &&
        (NPVARIANT_IS_INT32(length) || NPVARIANT_IS_DOUBLE(length))) {

        bool ignored;
        int32_t n = ToLong(plugin, length, ignored);
        for (int32_t i = 0; (ER_OK == status) && (i < n); ++i) {
            NPVariant element = NPVARIANT_VOID;
            if (NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(method), NPN_GetIntIdentifier(i), &element) &&
                NPVARIANT_IS_OBJECT(element)) {
                qcc::String name, signature, returnSignature, argNames;
                AnnotationsMap annotations;

                NPIdentifier* properties = NULL;
                uint32_t propertiesCount = 0;
                if (NPN_Enumerate(plugin->npp, NPVARIANT_TO_OBJECT(element), &properties, &propertiesCount)) {
                    for (uint32_t i = 0; (ER_OK == status) && (i < propertiesCount); ++i) {
                        if (!NPN_IdentifierIsString(properties[i])) {
                            continue;
                        }
                        NPUTF8* property = NPN_UTF8FromIdentifier(properties[i]);
                        if (!property) {
                            status = ER_OUT_OF_MEMORY;
                            break;
                        }
                        NPVariant npvalue = NPVARIANT_VOID;
                        if (!NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(element), properties[i], &npvalue)) {
                            continue;
                        }
                        qcc::String value = ToDOMString(plugin, npvalue, typeError);
                        if (typeError) {
                            continue;
                        }
                        NPN_ReleaseVariantValue(&npvalue);

                        if (!strcmp(property, "name")) {
                            name = value;
                        } else if (!strcmp(property, "signature")) {
                            signature = value;
                        } else if (!strcmp(property, "returnSignature")) {
                            returnSignature = value;
                        } else if (!strcmp(property, "argNames")) {
                            argNames = value;
                        } else {
                            annotations[property] = value;
                        }
                        NPN_MemFree(property);
                    }
                    NPN_MemFree(properties);
                }

                status = interface->AddMember(ajn::MESSAGE_METHOD_CALL,
                                              name.empty() ? 0 : name.c_str(),
                                              signature.empty() ? 0 : signature.c_str(),
                                              returnSignature.empty() ? 0 : returnSignature.c_str(),
                                              argNames.empty() ? 0 : argNames.c_str());

                for (AnnotationsMap::const_iterator it = annotations.begin(); it != annotations.end(); ++it) {
                    interface->AddMemberAnnotation(name.c_str(), it->first, it->second);
                }
            }
            NPN_ReleaseVariantValue(&element);
        }
    }
    NPN_ReleaseVariantValue(&length);
    NPN_ReleaseVariantValue(&method);
    if (ER_OK != status) {
        goto exit;
    }

    VOID_TO_NPVARIANT(signal);
    VOID_TO_NPVARIANT(length);
    if (NPN_GetProperty(plugin->npp, interfaceDescriptionNative->objectValue, NPN_GetStringIdentifier("signal"), &signal) &&
        NPVARIANT_IS_OBJECT(signal) &&
        NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(signal), NPN_GetStringIdentifier("length"), &length) &&
        (NPVARIANT_IS_INT32(length) || NPVARIANT_IS_DOUBLE(length))) {

        bool ignored;
        int32_t n = ToLong(plugin, length, ignored);
        for (int32_t i = 0; (ER_OK == status) && (i < n); ++i) {
            NPVariant element = NPVARIANT_VOID;
            if (NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(signal), NPN_GetIntIdentifier(i), &element) &&
                NPVARIANT_IS_OBJECT(element)) {
                qcc::String name, signature, argNames;
                AnnotationsMap annotations;

                NPIdentifier* properties = NULL;
                uint32_t propertiesCount = 0;
                if (NPN_Enumerate(plugin->npp, NPVARIANT_TO_OBJECT(element), &properties, &propertiesCount)) {
                    for (uint32_t i = 0; (ER_OK == status) && (i < propertiesCount); ++i) {
                        if (!NPN_IdentifierIsString(properties[i])) {
                            continue;
                        }
                        NPUTF8* property = NPN_UTF8FromIdentifier(properties[i]);
                        if (!property) {
                            status = ER_OUT_OF_MEMORY;
                            break;
                        }
                        NPVariant npvalue = NPVARIANT_VOID;
                        if (!NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(element), properties[i], &npvalue)) {
                            continue;
                        }
                        qcc::String value = ToDOMString(plugin, npvalue, typeError);
                        if (typeError) {
                            continue;
                        }
                        NPN_ReleaseVariantValue(&npvalue);

                        if (!strcmp(property, "name")) {
                            name = value;
                        } else if (!strcmp(property, "signature")) {
                            signature = value;
                        } else if (!strcmp(property, "argNames")) {
                            argNames = value;
                        } else {
                            annotations[property] = value;
                        }
                        NPN_MemFree(property);
                    }
                    NPN_MemFree(properties);
                }

                status = interface->AddMember(ajn::MESSAGE_SIGNAL,
                                              name.empty() ? 0 : name.c_str(),
                                              signature.empty() ? 0 : signature.c_str(),
                                              NULL,
                                              argNames.empty() ? 0 : argNames.c_str());

                for (AnnotationsMap::const_iterator it = annotations.begin(); it != annotations.end(); ++it) {
                    interface->AddMemberAnnotation(name.c_str(), it->first, it->second);
                }
            }
            NPN_ReleaseVariantValue(&element);
        }
    }
    NPN_ReleaseVariantValue(&length);
    NPN_ReleaseVariantValue(&signal);
    if (ER_OK != status) {
        goto exit;
    }

    VOID_TO_NPVARIANT(property);
    VOID_TO_NPVARIANT(length);
    if (NPN_GetProperty(plugin->npp, interfaceDescriptionNative->objectValue, NPN_GetStringIdentifier("property"), &property) &&
        NPVARIANT_IS_OBJECT(property) &&
        NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(property), NPN_GetStringIdentifier("length"), &length) &&
        (NPVARIANT_IS_INT32(length) || NPVARIANT_IS_DOUBLE(length))) {

        bool ignored;
        int32_t n = ToLong(plugin, length, ignored);
        for (int32_t i = 0; (ER_OK == status) && (i < n); ++i) {
            NPVariant element = NPVARIANT_VOID;
            if (NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(property), NPN_GetIntIdentifier(i), &element) &&
                NPVARIANT_IS_OBJECT(element)) {
                qcc::String name, signature;
                uint8_t accessFlags = 0;
                AnnotationsMap annotations;

                NPIdentifier* properties = NULL;
                uint32_t propertiesCount = 0;
                if (NPN_Enumerate(plugin->npp, NPVARIANT_TO_OBJECT(element), &properties, &propertiesCount)) {
                    for (uint32_t i = 0; (ER_OK == status) && (i < propertiesCount); ++i) {
                        if (!NPN_IdentifierIsString(properties[i])) {
                            continue;
                        }
                        NPUTF8* property = NPN_UTF8FromIdentifier(properties[i]);
                        if (!property) {
                            status = ER_OUT_OF_MEMORY;
                            break;
                        }
                        NPVariant npvalue = NPVARIANT_VOID;
                        if (!NPN_GetProperty(plugin->npp, NPVARIANT_TO_OBJECT(element), properties[i], &npvalue)) {
                            continue;
                        }
                        qcc::String value = ToDOMString(plugin, npvalue, typeError);
                        if (typeError) {
                            continue;
                        }
                        NPN_ReleaseVariantValue(&npvalue);

                        if (!strcmp(property, "name")) {
                            name = value;
                        } else if (!strcmp(property, "signature")) {
                            signature = value;
                        } else if (!strcmp(property, "access")) {
                            if (value == "readwrite") {
                                accessFlags = ajn::PROP_ACCESS_RW;
                            } else if (value == "read") {
                                accessFlags = ajn::PROP_ACCESS_READ;
                            } else if (value == "write") {
                                accessFlags = ajn::PROP_ACCESS_WRITE;
                            }
                        } else {
                            annotations[property] = value;
                        }
                        NPN_MemFree(property);
                    }
                    NPN_MemFree(properties);
                }

                status = interface->AddProperty(name.empty() ? 0 : name.c_str(),
                                                signature.empty() ? 0 : signature.c_str(),
                                                accessFlags);
                for (AnnotationsMap::const_iterator it = annotations.begin();
                     (ER_OK == status) && (it != annotations.end()); ++it) {
                    status = interface->AddPropertyAnnotation(name, it->first, it->second);
                }
            }
            NPN_ReleaseVariantValue(&element);
        }
    }
    NPN_ReleaseVariantValue(&length);
    NPN_ReleaseVariantValue(&property);
    if (ER_OK != status) {
        goto exit;
    }

    interface->Activate();
#if !defined(NDEBUG)
    {
        qcc::String str = interface->Introspect();
        QCC_DbgTrace(("%s", str.c_str()));
    }
#endif

exit:
    if (typeError) {
        return ER_FAIL;
    } else {
        return status;
    }
}

InterfaceDescriptionNative* InterfaceDescriptionNative::GetInterface(Plugin& plugin, BusAttachment& busAttachment, const qcc::String& name)
{
    QCC_DbgTrace(("%s(name=%s)", __FUNCTION__, name.c_str()));

    NPVariant npname = NPVARIANT_VOID;
    NPVariant value = NPVARIANT_VOID;
    size_t numMembers = 0;
    const ajn::InterfaceDescription::Member** members = NULL;
    size_t numMethods = 0;
    size_t numSignals = 0;
    size_t numProps = 0;
    NPVariant methodArray = NPVARIANT_VOID;
    NPVariant method = NPVARIANT_VOID;
    NPVariant secPolicy = NPVARIANT_VOID;
    NPVariant signalArray = NPVARIANT_VOID;
    NPVariant signal = NPVARIANT_VOID;
    const ajn::InterfaceDescription::Property** props = NULL;
    NPVariant propertyArray = NPVARIANT_VOID;
    NPVariant property = NPVARIANT_VOID;
    QStatus status = ER_OK;
    InterfaceDescriptionNative* interfaceDescriptionNative = NULL;
    bool typeError = false;

    const ajn::InterfaceDescription* iface = busAttachment->GetInterface(name.c_str());
    if (iface) {
        if (!NewObject(plugin, value)) {
            status = ER_FAIL;
            QCC_LogError(status, ("NewObject failed"));
            goto exit;
        }

        ToDOMString(plugin, name, npname);
        if (!NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(value), NPN_GetStringIdentifier("name"), &npname)) {
            status = ER_FAIL;
            QCC_LogError(status, ("NPN_SetProperty failed"));
            goto exit;
        }
        NPN_ReleaseVariantValue(&npname);
        VOID_TO_NPVARIANT(npname);

        if (iface->GetSecurityPolicy() != ajn::AJ_IFC_SECURITY_INHERIT) {
            INT32_TO_NPVARIANT(iface->GetSecurityPolicy(), secPolicy);
            if (!NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(value), NPN_GetStringIdentifier("secPolicy"), &secPolicy)) {
                status = ER_FAIL;
                QCC_LogError(status, ("NPN_SetProperty failed"));
                goto exit;
            }
        }

        numMembers = iface->GetMembers();
        members = new const ajn::InterfaceDescription::Member *[numMembers];
        iface->GetMembers(members, numMembers);
        for (size_t i = 0; i < numMembers; ++i) {
            switch (members[i]->memberType) {
            case ajn::MESSAGE_METHOD_CALL:
                ++numMethods;
                break;

            case ajn::MESSAGE_SIGNAL:
                ++numSignals;
                break;

            default:
                break;
            }
        }
        numProps = iface->GetProperties();

        if (numMethods) {
            if (!NewArray(plugin, methodArray)) {
                status = ER_FAIL;
                QCC_LogError(status, ("NewArray failed"));
                goto exit;
            }
            for (size_t i = 0, j = 0; i < numMembers; ++i) {
                if (ajn::MESSAGE_METHOD_CALL == members[i]->memberType) {
                    if (!NewObject(plugin, method)) {
                        status = ER_FAIL;
                        QCC_LogError(status, ("NewObject failed"));
                        goto exit;
                    }
                    NPVariant name, signature, returnSignature, argNames;
                    STRINGZ_TO_NPVARIANT(members[i]->name.c_str(), name);
                    STRINGZ_TO_NPVARIANT(members[i]->signature.c_str(), signature);
                    STRINGZ_TO_NPVARIANT(members[i]->returnSignature.c_str(), returnSignature);
                    STRINGZ_TO_NPVARIANT(members[i]->argNames.c_str(), argNames);
                    bool set =
                        NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(method), NPN_GetStringIdentifier("name"), &name) &&
                        NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(method), NPN_GetStringIdentifier("signature"), &signature) &&
                        NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(method), NPN_GetStringIdentifier("returnSignature"), &returnSignature) &&
                        NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(method), NPN_GetStringIdentifier("argNames"), &argNames);
                    if (!set) {
                        status = ER_FAIL;
                        QCC_LogError(status, ("NPN_SetProperty failed"));
                        goto exit;
                    }

                    const size_t numAnnotations = members[i]->GetAnnotations();
                    qcc::String* names = new qcc::String[numAnnotations];
                    qcc::String* values = new qcc::String[numAnnotations];
                    members[i]->GetAnnotations(names, values, numAnnotations);

                    for (size_t ann = 0; ann < numAnnotations; ++ann) {
                        NPVariant annotation;
                        STRINGZ_TO_NPVARIANT(values[ann].c_str(), annotation);
                        if (!NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(method), NPN_GetStringIdentifier(names[ann].c_str()), &annotation)) {
                            status = ER_FAIL;
                            QCC_LogError(status, ("NPN_SetProperty failed"));
                            goto exit;
                        }
                    }
                    delete [] names;
                    delete [] values;

                    if (!NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(methodArray), NPN_GetIntIdentifier(j++), &method)) {
                        status = ER_FAIL;
                        QCC_LogError(status, ("NPN_SetProperty failed"));
                        goto exit;
                    }
                    NPN_ReleaseVariantValue(&method);
                    VOID_TO_NPVARIANT(method);
                }
            }
            if (!NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(value), NPN_GetStringIdentifier("method"), &methodArray)) {
                status = ER_FAIL;
                QCC_LogError(status, ("NPN_SetProperty failed"));
                goto exit;
            }
        }

        if (numSignals) {
            if (!NewArray(plugin, signalArray)) {
                status = ER_FAIL;
                QCC_LogError(status, ("NewArray failed"));
                goto exit;
            }
            for (size_t i = 0, j = 0; i < numMembers; ++i) {
                if (ajn::MESSAGE_SIGNAL == members[i]->memberType) {
                    if (!NewObject(plugin, signal)) {
                        status = ER_FAIL;
                        QCC_LogError(status, ("NewObject failed"));
                        goto exit;
                    }
                    NPVariant name, signature, argNames;
                    STRINGZ_TO_NPVARIANT(members[i]->name.c_str(), name);
                    STRINGZ_TO_NPVARIANT(members[i]->signature.c_str(), signature);
                    STRINGZ_TO_NPVARIANT(members[i]->argNames.c_str(), argNames);
                    bool set =
                        NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(signal), NPN_GetStringIdentifier("name"), &name) &&
                        NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(signal), NPN_GetStringIdentifier("signature"), &signature) &&
                        NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(signal), NPN_GetStringIdentifier("argNames"), &argNames);
                    if (!set) {
                        status = ER_FAIL;
                        QCC_LogError(status, ("NPN_SetProperty failed"));
                        goto exit;
                    }

                    const size_t numAnnotations = members[i]->GetAnnotations();
                    qcc::String* names = new qcc::String[numAnnotations];
                    qcc::String* values = new qcc::String[numAnnotations];
                    members[i]->GetAnnotations(names, values, numAnnotations);
                    for (size_t ann = 0; ann < numAnnotations; ++ann) {
                        NPVariant annotation;
                        STRINGZ_TO_NPVARIANT(values[ann].c_str(), annotation);
                        if (!NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(signal), NPN_GetStringIdentifier(names[ann].c_str()), &annotation)) {
                            status = ER_FAIL;
                            QCC_LogError(status, ("NPN_SetProperty failed"));
                            goto exit;
                        }
                    }
                    delete [] names;
                    delete [] values;

                    if (!NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(signalArray), NPN_GetIntIdentifier(j++), &signal)) {
                        status = ER_FAIL;
                        QCC_LogError(status, ("NPN_SetProperty failed"));
                        goto exit;
                    }
                }
                NPN_ReleaseVariantValue(&signal);
                VOID_TO_NPVARIANT(signal);
            }
            if (!NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(value), NPN_GetStringIdentifier("signal"), &signalArray)) {
                status = ER_FAIL;
                QCC_LogError(status, ("NPN_SetProperty failed"));
                goto exit;
            }
        }

        if (numProps) {
            props = new const ajn::InterfaceDescription::Property *[numProps];
            iface->GetProperties(props, numProps);
            if (!NewArray(plugin, propertyArray)) {
                status = ER_FAIL;
                QCC_LogError(status, ("NewArray failed"));
                goto exit;
            }
            for (size_t i = 0, j = 0; i < numProps; ++i) {
                if (!NewObject(plugin, property)) {
                    status = ER_FAIL;
                    QCC_LogError(status, ("NewObject failed"));
                    goto exit;
                }
                NPVariant name, signature;
                STRINGZ_TO_NPVARIANT(props[i]->name.c_str(), name);
                STRINGZ_TO_NPVARIANT(props[i]->signature.c_str(), signature);
                bool set =
                    NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(property), NPN_GetStringIdentifier("name"), &name) &&
                    NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(property), NPN_GetStringIdentifier("signature"), &signature);
                if (!set) {
                    status = ER_FAIL;
                    QCC_LogError(status, ("NPN_SetProperty failed"));
                    goto exit;
                }

                NPVariant access;
                if (props[i]->access == ajn::PROP_ACCESS_RW) {
                    STRINGZ_TO_NPVARIANT("readwrite", access);
                } else if (props[i]->access == ajn::PROP_ACCESS_READ) {
                    STRINGZ_TO_NPVARIANT("read", access);
                } else if (props[i]->access == ajn::PROP_ACCESS_WRITE) {
                    STRINGZ_TO_NPVARIANT("write", access);
                }
                if (!NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(property), NPN_GetStringIdentifier("access"), &access)) {
                    status = ER_FAIL;
                    QCC_LogError(status, ("NPN_SetProperty failed"));
                    goto exit;
                }

                const size_t numAnnotations = props[i]->GetAnnotations();
                qcc::String* names = new qcc::String[numAnnotations];
                qcc::String* values = new qcc::String[numAnnotations];
                props[i]->GetAnnotations(names, values, numAnnotations);
                for (size_t ann = 0; ann < numAnnotations; ++ann) {
                    NPVariant annotation;
                    STRINGZ_TO_NPVARIANT(values[ann].c_str(), annotation);
                    if (!NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(property), NPN_GetStringIdentifier(names[ann].c_str()), &annotation)) {
                        status = ER_FAIL;
                        QCC_LogError(status, ("NPN_SetProperty failed"));
                        goto exit;
                    }
                }
                delete [] names;
                delete [] values;

                if (!NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(propertyArray), NPN_GetIntIdentifier(j++), &property)) {
                    status = ER_FAIL;
                    QCC_LogError(status, ("NPN_SetProperty failed"));
                    goto exit;
                }
                NPN_ReleaseVariantValue(&property);
                VOID_TO_NPVARIANT(property);
            }
            if (!NPN_SetProperty(plugin->npp, NPVARIANT_TO_OBJECT(value), NPN_GetStringIdentifier("property"), &propertyArray)) {
                status = ER_FAIL;
                QCC_LogError(status, ("NPN_SetProperty failed"));
                goto exit;
            }
        }

        interfaceDescriptionNative = ToNativeObject<InterfaceDescriptionNative>(plugin, value, typeError);
        if (typeError || !interfaceDescriptionNative) {
            typeError = true;
            goto exit;
        }
    }

exit:
    NPN_ReleaseVariantValue(&property);
    NPN_ReleaseVariantValue(&propertyArray);
    delete[] props;
    NPN_ReleaseVariantValue(&signal);
    NPN_ReleaseVariantValue(&signalArray);
    NPN_ReleaseVariantValue(&method);
    NPN_ReleaseVariantValue(&methodArray);
    delete[] members;
    NPN_ReleaseVariantValue(&value);
    return interfaceDescriptionNative;
}

InterfaceDescriptionNative::InterfaceDescriptionNative(Plugin& plugin, NPObject* objectValue) :
    NativeObject(plugin, objectValue)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

InterfaceDescriptionNative::InterfaceDescriptionNative(InterfaceDescriptionNative* other) :
    NativeObject(other->plugin, other->objectValue)
{
}

InterfaceDescriptionNative::~InterfaceDescriptionNative()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}
