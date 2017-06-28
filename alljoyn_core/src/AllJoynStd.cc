/**
 * @file
 *
 * This file provides definitions for standard AllJoyn interfaces
 *
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
#include <qcc/platform.h>
#include <qcc/Debug.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/InterfaceDescription.h>

#include "SessionInternal.h"

#define QCC_MODULE  "ALLJOYN"

namespace ajn {

/** org.alljoyn.About interface definitions */
const char* org::alljoyn::About::ObjectPath = "/About";
const char* org::alljoyn::About::InterfaceName = "org.alljoyn.About";
const char* org::alljoyn::About::WellKnownName = "org.alljoyn.About";

/** org.alljoyn.About.Icon interface definitions */
const char* org::alljoyn::Icon::ObjectPath = "/About/DeviceIcon";
const char* org::alljoyn::Icon::InterfaceName = "org.alljoyn.Icon";
const char* org::alljoyn::Icon::WellKnownName = "org.alljoyn.Icon";

/** org.alljoyn.Bus interface definitions */
const char* org::alljoyn::Bus::ErrorName = "org.alljoyn.Bus.ErStatus";
const char* org::alljoyn::Bus::ObjectPath = "/org/alljoyn/Bus";
const char* org::alljoyn::Bus::InterfaceName = "org.alljoyn.Bus";
const char* org::alljoyn::Bus::WellKnownName = "org.alljoyn.Bus";
const char* org::alljoyn::Bus::Secure = "org.alljoyn.Bus.Secure";
const char* org::alljoyn::Bus::Peer::ObjectPath = "/org/alljoyn/Bus/Peer";

/** org.alljoy.Bus.Application interface definitions */
const char* org::alljoyn::Bus::Application::InterfaceName = "org.alljoyn.Bus.Application";

/** org.alljoyn.Bus.Peer.* interface definitions */
const char* org::alljoyn::Bus::Peer::HeaderCompression::InterfaceName = "org.alljoyn.Bus.Peer.HeaderCompression";
const char* org::alljoyn::Bus::Peer::Authentication::InterfaceName = "org.alljoyn.Bus.Peer.Authentication";
const char* org::alljoyn::Bus::Peer::Session::InterfaceName = "org.alljoyn.Bus.Peer.Session";

const char* org::alljoyn::Bus::Security::ObjectPath = "/org/alljoyn/Bus/Security";
const char* org::alljoyn::Bus::Security::Application::InterfaceName = "org.alljoyn.Bus.Security.Application";
const char* org::alljoyn::Bus::Security::ClaimableApplication::InterfaceName = "org.alljoyn.Bus.Security.ClaimableApplication";
const char* org::alljoyn::Bus::Security::ManagedApplication::InterfaceName = "org.alljoyn.Bus.Security.ManagedApplication";

/** org.alljoyn.Daemon interface definitions */
const char* org::alljoyn::Daemon::ErrorName = "org.alljoyn.Daemon.ErStatus";
const char* org::alljoyn::Daemon::ObjectPath = "/org/alljoyn/Bus";
const char* org::alljoyn::Daemon::InterfaceName = "org.alljoyn.Daemon";
const char* org::alljoyn::Daemon::WellKnownName = "org.alljoyn.Daemon";

/** org.alljoyn.Daemon.Debug interface definitions */
const char* org::alljoyn::Daemon::Debug::ObjectPath = "/org/alljoyn/Debug";
const char* org::alljoyn::Daemon::Debug::InterfaceName = "org.alljoyn.Debug";

/** org.allseen.Introspectable interface definitions */
const char* org::allseen::Introspectable::InterfaceName = "org.allseen.Introspectable";
const char* org::allseen::Introspectable::IntrospectDocType =
    "<!DOCTYPE"
    " node PUBLIC \"-//allseen//DTD ALLJOYN Object Introspection 1.1//EN\"\n"
    "\"http://www.allseen.org/alljoyn/introspect-1.1.dtd\""
    ">\n";

QStatus org::alljoyn::CreateInterfaces(BusAttachment& bus)
{
    QStatus status;
    {
        /* Create the org.alljoyn.About interface */
        InterfaceDescription* ifc = NULL;
        status = bus.CreateInterface(org::alljoyn::About::InterfaceName, ifc);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to create interface \"%s\"", org::alljoyn::About::InterfaceName));
            return status;
        }
        QCC_VERIFY(ER_OK == ifc->AddMethod("GetAboutData", "s", "a{sv}", "languageTag,aboutData"));
        QCC_VERIFY(ER_OK == ifc->AddMethod("GetObjectDescription", NULL, "a(oas)", "Control"));
        QCC_VERIFY(ER_OK == ifc->AddProperty("Version", "q", PROP_ACCESS_READ));
        QCC_VERIFY(ER_OK == ifc->AddSignal("Announce", "qqa(oas)a{sv}", "version,port,objectDescription,servMetadata", 0));

        ifc->Activate();
    }
    {
        /* Create the org.alljoyn.Icon interface */
        InterfaceDescription* ifc = NULL;
        status = bus.CreateInterface(org::alljoyn::Icon::InterfaceName, ifc);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to create interface \"%s\"", org::alljoyn::Icon::InterfaceName));
            return status;
        }
        QCC_VERIFY(ER_OK == ifc->AddMethod("GetUrl", NULL, "s", "url"));
        QCC_VERIFY(ER_OK == ifc->AddMethod("GetContent", NULL, "ay", "content"));
        QCC_VERIFY(ER_OK == ifc->AddProperty("Version", "q", PROP_ACCESS_READ));
        QCC_VERIFY(ER_OK == ifc->AddProperty("MimeType", "s", PROP_ACCESS_READ));
        QCC_VERIFY(ER_OK == ifc->AddProperty("Size", "u", PROP_ACCESS_READ));

        ifc->Activate();
    }
    {
        /* Create the org.alljoyn.Bus interface */
        InterfaceDescription* ifc = NULL;
        status = bus.CreateInterface(org::alljoyn::Bus::InterfaceName, ifc);

        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to create interface \"%s\"", org::alljoyn::Bus::InterfaceName));
            return status;
        }
        QCC_VERIFY(ER_OK == ifc->AddMethod("BusHello",                 "su",                "ssu",               "GUIDC,protoVerC,GUIDS,uniqueName,protoVerS", 0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("SimpleHello",              "su",                "ssu",               "GUIDC,protoVerC,GUIDS,uniqueName,protoVerS", 0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("BindSessionPort",          "q" SESSIONOPTS_SIG,  "uq",                "portIn,opts,disposition,portOut",           0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("UnbindSessionPort",        "q",                 "u",                 "port,disposition",                           0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("JoinSession",              "sq" SESSIONOPTS_SIG, "uu" SESSIONOPTS_SIG, "sessionHost,port,opts,disp,sessionId,opts", 0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("LeaveSession",             "u",                 "u",                 "sessionId,disposition",                      0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("LeaveHostedSession",       "u",                 "u",                 "sessionId,disposition",                      0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("LeaveJoinedSession",       "u",                 "u",                 "sessionId,disposition",                      0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("AdvertiseName",            "sq",                "u",                 "name,transports,disposition",                0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("CancelAdvertiseName",      "sq",                "u",                 "name,transports,disposition",                0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("FindAdvertisedName",       "s",                 "u",                 "name,disposition",                           0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("FindAdvertisedNameByTransport",       "sq",                "u",                 "name,transports,disposition",     0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("CancelFindAdvertisedName", "s",                 "u",                 "name,disposition",                           0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("CancelFindAdvertisedNameByTransport", "sq",                "u",                 "name,transports,disposition",     0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("GetSessionFd",             "u",                 "h",                 "sessionId,handle",                           0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("SetLinkTimeout",           "uu",                "uu",                "sessionId,inLinkTO,disposition,outLinkTO",   0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("AliasUnixUser",            "u",                 "u",                 "aliasUID, disposition",                      0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("OnAppSuspend",             "",                  "u",                 "disposition",                                0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("OnAppResume",              "",                  "u",                 "disposition",                                0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("CancelSessionlessMessage", "u",                 "u",                 "serialNum,disposition",                      0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("RemoveSessionMember",      "us",                "u",                 "sessionId,name,disposition",                 0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("GetHostInfo",              "u",                 "uss",               "sessionId,disposition,localipaddr,remoteipaddr", 0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("ReloadConfig",             "",                  "b",                 "loaded",                                     0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("Ping",                     "su",                "u",                 "name,timeout,disposition",                   0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("FindAdvertisementByTransport",       "sq",                "u",                 "matching,transports,disposition",     0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("CancelFindAdvertisementByTransport", "sq",                "u",                 "matching,transports,disposition",     0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("SetIdleTimeouts",     "uu",                "uuu",
                       "reqLinkTO,reqProbeTO,disposition,actLinkTO,actProbeTO", 0));


        QCC_VERIFY(ER_OK == ifc->AddSignal("FoundAdvertisedName",      "sqs",              "name,transport,prefix",                        0));
        QCC_VERIFY(ER_OK == ifc->AddSignal("LostAdvertisedName",       "sqs",              "name,transport,prefix",                        0));
        QCC_VERIFY(ER_OK == ifc->AddSignal("SessionLost",              "u",               "sessionId",                                     0));
        QCC_VERIFY(ER_OK == ifc->AddSignal("SessionLostWithReason",    "uu",               "sessionId,reason",                             0));
        QCC_VERIFY(ER_OK == ifc->AddSignal("SessionLostWithReasonAndDisposition",    "uuu",               "sessionId,reason,disposition",                             0));
        QCC_VERIFY(ER_OK == ifc->AddSignal("MPSessionChanged",         "usb",              "sessionId,name,isAdded",                       0));
        QCC_VERIFY(ER_OK == ifc->AddSignal("MPSessionChangedWithReason",         "usbu",              "sessionId,name,isAdded,reason",                       0));

        ifc->Activate();
    }
    {
        /* Create the org.alljoyn.Bus.Application interface */
        InterfaceDescription* ifc = NULL;
        status = bus.CreateInterface(org::alljoyn::Bus::Application::InterfaceName, ifc, AJ_IFC_SECURITY_OFF);

        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to create interface \"%s\"", org::alljoyn::Bus::Application::InterfaceName));
            return status;
        }
        QCC_VERIFY(ER_OK == ifc->AddProperty("Version", "q", PROP_ACCESS_READ));
        QCC_VERIFY(ER_OK == ifc->AddPropertyAnnotation("Version", org::freedesktop::DBus::AnnotateEmitsChanged, "const"));
        QCC_VERIFY(ER_OK == ifc->AddSignal("State", "(yyayay)q", "publicKey,state", 0));

        ifc->Activate();
    }
    {
        /* Create the org.alljoyn.Daemon interface */
        InterfaceDescription* ifc = NULL;
        status = bus.CreateInterface(org::alljoyn::Daemon::InterfaceName, ifc);

        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to create interface \"%s\"", org::alljoyn::Daemon::InterfaceName));
            return status;
        }
        QCC_VERIFY(ER_OK == ifc->AddMethod("AttachSession",  "qsssss" SESSIONOPTS_SIG, "uu" SESSIONOPTS_SIG "as", "port,joiner,creator,dest,b2b,busAddr,optsIn,status,id,optsOut,members", 0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("AttachSessionWithNames",  "qsssss" SESSIONOPTS_SIG "a(sas)", "uu" SESSIONOPTS_SIG "asa(sas)", "port,joiner,creator,dest,b2b,busAddr,optsIn,namesIn,status,id,optsOut,members,namesOut", 0));
        QCC_VERIFY(ER_OK == ifc->AddMethod("GetSessionInfo", "sq" SESSIONOPTS_SIG, "as", "creator,port,opts,busAddrs", 0));
        QCC_VERIFY(ER_OK == ifc->AddSignal("DetachSession",  "us",     "sessionId,joiner",       0));
        QCC_VERIFY(ER_OK == ifc->AddSignal("ExchangeNames",  "a(sas)", "uniqueName,aliases",     0));
        QCC_VERIFY(ER_OK == ifc->AddSignal("NameChanged",    "sss",    "name,oldOwner,newOwner", 0));
        QCC_VERIFY(ER_OK == ifc->AddSignal("ProbeReq",       "",       "",                       0));
        QCC_VERIFY(ER_OK == ifc->AddSignal("ProbeAck",       "",       "",                       0));
        ifc->Activate();
    }
    {
        /* Create the org.alljoyn.Daemon.Debug interface */
        InterfaceDescription* ifc = NULL;
        status = bus.CreateInterface(org::alljoyn::Daemon::Debug::InterfaceName, ifc);

        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to create interface \"%s\"", org::alljoyn::Daemon::Debug::InterfaceName));
            return status;
        }
        QCC_VERIFY(ER_OK == ifc->AddMethod("SetDebugLevel",  "su", NULL, "module,level", 0));
        ifc->Activate();
    }
    {
        /*
         * Create the org.alljoyn.Bus.Peer.HeaderCompression interface
         *
         * Note that header compression was deprecated in March 2015 for 15.04 release.
         */
        InterfaceDescription* ifc = NULL;
        status = bus.CreateInterface(org::alljoyn::Bus::Peer::HeaderCompression::InterfaceName, ifc);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to create %s interface", org::alljoyn::Bus::Peer::HeaderCompression::InterfaceName));
            return status;
        }
        QCC_VERIFY(ER_OK == ifc->AddMethod("GetExpansion", "u", "a(yv)", "token,headerFields"));
        ifc->Activate();
    }
    {
        /* Create the org.alljoyn.Bus.Peer.Authentication interface */
        InterfaceDescription* ifc = NULL;
        status = bus.CreateInterface(org::alljoyn::Bus::Peer::Authentication::InterfaceName, ifc, AJ_IFC_SECURITY_OFF);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to create %s interface", org::alljoyn::Bus::Peer::Authentication::InterfaceName));
            return status;
        }
        QCC_VERIFY(ER_OK == ifc->AddMethod("ExchangeGuids",     "su",  "su", "localGuid,localVersion,remoteGuid,remoteVersion"));
        QCC_VERIFY(ER_OK == ifc->AddMethod("GenSessionKey",     "sss", "ss", "localGuid,remoteGuid,localNonce,remoteNonce,verifier"));
        QCC_VERIFY(ER_OK == ifc->AddMethod("ExchangeGroupKeys", "ay",  "ay", "localKeyMatter,remoteKeyMatter"));
        QCC_VERIFY(ER_OK == ifc->AddMethod("AuthChallenge",     "s",   "s",  "challenge,response"));
        QCC_VERIFY(ER_OK == ifc->AddMethod("ExchangeSuites",     "au",   "au",  "localAuthList,remoteAuthList"));
        QCC_VERIFY(ER_OK == ifc->AddMethod("KeyExchange",     "uv",   "uv",  "localAuthMask,localPublicKey, remoteAuthMask, remotePublicKey"));
        QCC_VERIFY(ER_OK == ifc->AddMethod("KeyAuthentication",     "v",   "v",  "localVerifier,remoteVerifier"));
        QCC_VERIFY(ER_OK == ifc->AddMethod("SendManifests",     "a(ua(ssa(syy))saysay)",   "a(ua(ssa(syy))saysay)", "manifests,manifests"));
        QCC_VERIFY(ER_OK == ifc->AddMethod("SendMemberships",     "ya(yay)",   "ya(yay)", "sendCode,memberships,sendCode,memberships"));
        QCC_VERIFY(ER_OK == ifc->AddProperty("Mechanisms",  "s", PROP_ACCESS_READ));
        QCC_VERIFY(ER_OK == ifc->AddProperty("Version",     "u", PROP_ACCESS_READ));
        ifc->Activate();
    }
    {
        /* Create the org.alljoyn.Bus.Peer.Session interface */
        InterfaceDescription* ifc = NULL;
        status = bus.CreateInterface(org::alljoyn::Bus::Peer::Session::InterfaceName, ifc);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to create %s interface", org::alljoyn::Bus::Peer::Session::InterfaceName));
            return status;
        }
        QCC_VERIFY(ER_OK == ifc->AddMethod("AcceptSession", "qus" SESSIONOPTS_SIG, "b", "port,id,src,opts,accepted"));
        QCC_VERIFY(ER_OK == ifc->AddSignal("SessionJoined", "qus", "port,id,src", MEMBER_ANNOTATE_UNICAST));
        ifc->Activate();
    }
    {
        /* Create the org.alljoyn.Bus.Security.Application interface */
        InterfaceDescription* ifc = NULL;
        status = bus.CreateInterface(org::alljoyn::Bus::Security::Application::InterfaceName, ifc, AJ_IFC_SECURITY_REQUIRED);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to create interface \"%s\"", org::alljoyn::Bus::Security::Application::InterfaceName));
            return status;
        }
        QCC_VERIFY(ER_OK == ifc->AddAnnotation(org::alljoyn::Bus::Secure, "true"));
        QCC_VERIFY(ER_OK == ifc->AddProperty("Version", "q", PROP_ACCESS_READ));
        QCC_VERIFY(ER_OK == ifc->AddPropertyAnnotation("Version", org::freedesktop::DBus::AnnotateEmitsChanged, "const"));
        QCC_VERIFY(ER_OK == ifc->AddProperty("ApplicationState", "q", PROP_ACCESS_READ));
        QCC_VERIFY(ER_OK == ifc->AddPropertyAnnotation("ApplicationState", org::freedesktop::DBus::AnnotateEmitsChanged, "false"));
        QCC_VERIFY(ER_OK == ifc->AddProperty("ManifestTemplateDigest", "(yay)", PROP_ACCESS_READ));
        QCC_VERIFY(ER_OK == ifc->AddPropertyAnnotation("ManifestTemplateDigest", org::freedesktop::DBus::AnnotateEmitsChanged, "false"));
        QCC_VERIFY(ER_OK == ifc->AddProperty("EccPublicKey", "(yyayay)", PROP_ACCESS_READ));
        QCC_VERIFY(ER_OK == ifc->AddPropertyAnnotation("EccPublicKey", org::freedesktop::DBus::AnnotateEmitsChanged, "false"));
        QCC_VERIFY(ER_OK == ifc->AddProperty("ManufacturerCertificate", "a(yay)", PROP_ACCESS_READ));
        QCC_VERIFY(ER_OK == ifc->AddPropertyAnnotation("ManufacturerCertificate", org::freedesktop::DBus::AnnotateEmitsChanged, "false"));
        QCC_VERIFY(ER_OK == ifc->AddProperty("ManifestTemplate", "a(ssya(syy))", PROP_ACCESS_READ));
        QCC_VERIFY(ER_OK == ifc->AddPropertyAnnotation("ManifestTemplate", org::freedesktop::DBus::AnnotateEmitsChanged, "false"));
        QCC_VERIFY(ER_OK == ifc->AddProperty("ClaimCapabilities", "q", PROP_ACCESS_READ));
        QCC_VERIFY(ER_OK == ifc->AddPropertyAnnotation("ClaimCapabilities", org::freedesktop::DBus::AnnotateEmitsChanged, "false"));
        QCC_VERIFY(ER_OK == ifc->AddProperty("ClaimCapabilityAdditionalInfo", "q", PROP_ACCESS_READ));
        QCC_VERIFY(ER_OK == ifc->AddPropertyAnnotation("ClaimCapabilityAdditionalInfo", org::freedesktop::DBus::AnnotateEmitsChanged, "false"));

        ifc->Activate();
    }
    {
        /* Create the org.alljoyn.Bus.Security.ClaimableApplication interface */
        InterfaceDescription* ifc = NULL;
        status = bus.CreateInterface(org::alljoyn::Bus::Security::ClaimableApplication::InterfaceName, ifc, AJ_IFC_SECURITY_REQUIRED);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to create interface \"%s\"", org::alljoyn::Bus::Security::ClaimableApplication::InterfaceName));
            return status;
        }
        QCC_VERIFY(ER_OK == ifc->AddAnnotation(org::alljoyn::Bus::Secure, "true"));
        QCC_VERIFY(ER_OK == ifc->AddProperty("Version", "q", PROP_ACCESS_READ));
        QCC_VERIFY(ER_OK == ifc->AddPropertyAnnotation("Version", org::freedesktop::DBus::AnnotateEmitsChanged, "const"));
        QCC_VERIFY(ER_OK == ifc->AddMethod("Claim", "(yyayay)ayay(yyayay)aya(yay)a(ua(ssa(syy))saysay)", "",
                       "certificateAuthority,authorityKeyIdentifier,adminSecurityGroupId,adminSecurityGroupAuthority,adminGroupAuthorityKeyIdentifier,identityCertificateChain,manifests"));

        ifc->Activate();
    }
    {
        /* Create the org.alljoyn.Bus.Security.ManagedApplication interface */
        InterfaceDescription* ifc = NULL;
        status = bus.CreateInterface(org::alljoyn::Bus::Security::ManagedApplication::InterfaceName, ifc, AJ_IFC_SECURITY_REQUIRED);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to create interface \"%s\"", org::alljoyn::Bus::Security::ManagedApplication::InterfaceName));
            return status;
        }
        QCC_VERIFY(ER_OK == ifc->AddAnnotation(org::alljoyn::Bus::Secure, "true"));
        QCC_VERIFY(ER_OK == ifc->AddProperty("Version", "q", PROP_ACCESS_READ));
        QCC_VERIFY(ER_OK == ifc->AddPropertyAnnotation("Version", org::freedesktop::DBus::AnnotateEmitsChanged, "const"));
        QCC_VERIFY(ER_OK == ifc->AddMethod("Reset", "", "", ""));
        QCC_VERIFY(ER_OK == ifc->AddMethod("UpdateIdentity", "a(yay)a(ua(ssa(syy))saysay)", "", "certificateChain,manifests"));
        QCC_VERIFY(ER_OK == ifc->AddProperty("Identity", "a(yay)", PROP_ACCESS_READ));
        QCC_VERIFY(ER_OK == ifc->AddPropertyAnnotation("Identity", org::freedesktop::DBus::AnnotateEmitsChanged, "false"));
        QCC_VERIFY(ER_OK == ifc->AddProperty("Manifests", "a(ua(ssa(syy))saysay)", PROP_ACCESS_READ));
        QCC_VERIFY(ER_OK == ifc->AddPropertyAnnotation("Manifests", org::freedesktop::DBus::AnnotateEmitsChanged, "false"));
        QCC_VERIFY(ER_OK == ifc->AddProperty("IdentityCertificateId", "(ayay(yyayay))", PROP_ACCESS_READ));
        QCC_VERIFY(ER_OK == ifc->AddPropertyAnnotation("IdentityCertificateId", org::freedesktop::DBus::AnnotateEmitsChanged, "false"));
        QCC_VERIFY(ER_OK == ifc->AddProperty("PolicyVersion", "u", PROP_ACCESS_READ));
        QCC_VERIFY(ER_OK == ifc->AddPropertyAnnotation("PolicyVersion", org::freedesktop::DBus::AnnotateEmitsChanged, "false"));
        QCC_VERIFY(ER_OK == ifc->AddMethod("UpdatePolicy", "(qua(a(ya(yyayayay)ay)a(ssa(syy))))", "", "policy"));
        QCC_VERIFY(ER_OK == ifc->AddProperty("Policy", "(qua(a(ya(yyayayay)ay)a(ssa(syy))))", PROP_ACCESS_READ));
        QCC_VERIFY(ER_OK == ifc->AddPropertyAnnotation("Policy", org::freedesktop::DBus::AnnotateEmitsChanged, "false"));
        QCC_VERIFY(ER_OK == ifc->AddProperty("DefaultPolicy", "(qua(a(ya(yyayayay)ay)a(ssa(syy))))", PROP_ACCESS_READ));
        QCC_VERIFY(ER_OK == ifc->AddPropertyAnnotation("DefaultPolicy", org::freedesktop::DBus::AnnotateEmitsChanged, "false"));
        QCC_VERIFY(ER_OK == ifc->AddMethod("ResetPolicy", "", "", ""));
        QCC_VERIFY(ER_OK == ifc->AddMethod("InstallMembership", "a(yay)", "", "certificateChain"));
        QCC_VERIFY(ER_OK == ifc->AddMethod("RemoveMembership", "(ayay(yyayay))", "", "certificateId"));
        QCC_VERIFY(ER_OK == ifc->AddProperty("MembershipSummaries", "a(ayay(yyayay))", PROP_ACCESS_READ));
        QCC_VERIFY(ER_OK == ifc->AddPropertyAnnotation("MembershipSummaries", org::freedesktop::DBus::AnnotateEmitsChanged, "false"));
        QCC_VERIFY(ER_OK == ifc->AddProperty("MembershipCertificates", "aa(yay)", PROP_ACCESS_READ));
        QCC_VERIFY(ER_OK == ifc->AddPropertyAnnotation("MembershipCertificates", org::freedesktop::DBus::AnnotateEmitsChanged, "false"));
        QCC_VERIFY(ER_OK == ifc->AddMethod("StartManagement", "", "", ""));
        QCC_VERIFY(ER_OK == ifc->AddMethod("EndManagement", "", "", ""));
        QCC_VERIFY(ER_OK == ifc->AddMethod("InstallManifests", "a(ua(ssa(syy))saysay)", "", "manifests"));

        ifc->Activate();
    }
    {
        InterfaceDescription* introspectIntf = NULL;
        status = bus.CreateInterface(org::allseen::Introspectable::InterfaceName, introspectIntf, AJ_IFC_SECURITY_OFF);
        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to create interface \"%s\"", org::allseen::Introspectable::InterfaceName));
            return status;
        }

        QCC_VERIFY(ER_OK == introspectIntf->AddMethod("GetDescriptionLanguages",     "",   "as", "languageTags"));
        QCC_VERIFY(ER_OK == introspectIntf->AddMethod("IntrospectWithDescription",   "s",  "s",  "languageTag,data"));
        introspectIntf->Activate();
    }
    return status;
}


}

