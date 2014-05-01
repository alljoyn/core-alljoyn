/******************************************************************************
 * Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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

#include <alljoyn/about/AnnouncementRegistrar.h>
#include <qcc/Debug.h>
#include <qcc/String.h>

#define QCC_MODULE "ALLJOYN_ABOUT_ANNOUNCEMENT_REGISTRAR"

using namespace ajn;
using namespace services;

QStatus AnnouncementRegistrar::RegisterAnnounceHandler(ajn::BusAttachment& bus, AnnounceHandler& handler, char** implementsInterfaces, size_t numberInterfaces) {
    QCC_DbgTrace(("AnnouncementRegistrar::%s", __FUNCTION__));
    QStatus status = ER_OK;
    // the only time number of interfaces should be zero is if implements
    // interfaces is a NULL pointer
    // If a valid pointer is passed in then the number of interfaces should not be
    // zero.
    if ((implementsInterfaces == NULL && numberInterfaces != 0) ||
        (implementsInterfaces != NULL && numberInterfaces == 0)) {
        return ER_BAD_ARG_4;
    }
    const InterfaceDescription* getIface = NULL;
    getIface = bus.GetInterface("org.alljoyn.About");
    if (!getIface) {
        InterfaceDescription* createIface = NULL;
        status = bus.CreateInterface("org.alljoyn.About", createIface, false);
        if (status != ER_OK) {
            return status;
        }
        if (!createIface) {
            return ER_BUS_CANNOT_ADD_INTERFACE;
        }

        status = createIface->AddMethod("GetAboutData", "s", "a{sv}", "languageTag,aboutData");
        if (status != ER_OK) {
            return status;
        }
        status = createIface->AddMethod("GetObjectDescription", NULL, "a(oas)", "Control");
        if (status != ER_OK) {
            return status;
        }
        status = createIface->AddProperty("Version", "q", PROP_ACCESS_READ);
        if (status != ER_OK) {
            return status;
        }
        status = createIface->AddSignal("Announce", "qqa(oas)a{sv}", "version,port,objectDescription,aboutData", 0);
        if (status != ER_OK) {
            return status;
        }

        createIface->Activate();
        handler.announceSignalMember = createIface->GetMember("Announce");
    } else {
        handler.announceSignalMember = getIface->GetMember("Announce");
    }

    status = bus.RegisterSignalHandler(&handler,
                                       static_cast<MessageReceiver::SignalHandler>(&AnnounceHandler::AnnounceSignalHandler),
                                       handler.announceSignalMember,
                                       0);
    if (status != ER_OK) {
        return status;
    }

    qcc::String matchRule = "type='signal',interface='org.alljoyn.About',member='Announce',sessionless='t'";
    for (size_t i = 0; i < numberInterfaces; ++i) {
        matchRule += qcc::String(",implements='") + implementsInterfaces[i] + qcc::String("'");
    }

    status = bus.AddMatch(matchRule.c_str());
    if (status != ER_OK) {
        return status;
    }


    QCC_DbgPrintf(("AnnouncementRegistrar::%s result %s", __FUNCTION__, QCC_StatusText(status)));
    return status;
}

QStatus AnnouncementRegistrar::UnRegisterAnnounceHandler(ajn::BusAttachment& bus, AnnounceHandler& handler) {
    QCC_DbgTrace(("AnnouncementRegistrar::%s", __FUNCTION__));
    QStatus status = ER_OK;

    status = bus.UnregisterSignalHandler(&handler, static_cast<MessageReceiver::SignalHandler>(&AnnounceHandler::AnnounceSignalHandler),
                                         handler.announceSignalMember, NULL);
    if (status != ER_OK) {
        return status;
    }

    QCC_DbgPrintf(("AnnouncementRegistrar::%s result %s", __FUNCTION__, QCC_StatusText(status)));
    return status;
}
