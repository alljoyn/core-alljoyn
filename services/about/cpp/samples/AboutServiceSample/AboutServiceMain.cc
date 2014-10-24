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

#include <alljoyn/BusAttachment.h>
#include <alljoyn/about/AboutIconService.h>
#include <alljoyn/about/AboutServiceApi.h>
#include <alljoyn/about/AboutPropertyStoreImpl.h>

#include <signal.h>
#include "BusListenerImpl.h"
#include "OptParser.h"

using namespace ajn;
using namespace services;

#define SERVICE_EXIT_OK       0
#define SERVICE_OPTION_ERROR  1
#define SERVICE_CONFIG_ERROR  2

static SessionPort SERVICE_PORT;

static volatile sig_atomic_t s_interrupt = false;

static BusListenerImpl s_busListener(SERVICE_PORT);

/** Top level message bus object. */
static BusAttachment* s_msgBus = NULL;

static void SigIntHandler(int sig) {
    s_interrupt = true;
}

/** Register the bus object and connect, report the result to stdout, and return the status code. */
QStatus RegisterBusObject(AboutService* obj) {
    QStatus status = s_msgBus->RegisterBusObject(*obj);

    if (ER_OK == status) {
        std::cout << "RegisterBusObject succeeded." << std::endl;
    } else {
        std::cout << "RegisterBusObject failed (" << QCC_StatusText(status) << ")." << std::endl;
    }

    return status;
}

/** Connect to the daemon, report the result to stdout, and return the status code. */
QStatus ConnectToDaemon() {
    QStatus status;
    status = s_msgBus->Connect();

    if (ER_OK == status) {
        std::cout << "Daemon connect succeeded." << std::endl;
    } else {
        std::cout << "Failed to connect daemon (" << QCC_StatusText(status) << ")." << std::endl;
    }

    return status;
}

/** Start the message bus, report the result to stdout, and return the status code. */
QStatus StartMessageBus(void) {
    QStatus status = s_msgBus->Start();

    if (ER_OK == status) {
        std::cout << "BusAttachment started." << std::endl;
    } else {
        std::cout << "Start of BusAttachment failed (" << QCC_StatusText(status) << ")." << std::endl;
    }

    return status;
}

/** Create the session, report the result to stdout, and return the status code. */
QStatus BindSession(TransportMask mask) {
    SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, mask);
    SessionPort sp = SERVICE_PORT;
    QStatus status = s_msgBus->BindSessionPort(sp, opts, s_busListener);

    if (ER_OK == status) {
        std::cout << "BindSessionPort succeeded." << std::endl;
    } else {
        std::cout << "BindSessionPort failed (" << QCC_StatusText(status) << ")." << std::endl;
    }

    return status;
}

/** Advertise the service name, report the result to stdout, and return the status code. */
QStatus AdvertiseName(TransportMask mask) {
    QStatus status = ER_BUS_ESTABLISH_FAILED;
    if (s_msgBus->IsConnected() && s_msgBus->GetUniqueName().size() > 0) {
        status = s_msgBus->AdvertiseName(s_msgBus->GetUniqueName().c_str(), mask);
        std::cout << "AdvertiseName " << s_msgBus->GetUniqueName().c_str() << " =" << status << std::endl;
    }
    return status;
}

static QStatus FillAboutPropertyStoreImplData(AboutPropertyStoreImpl* propStore, OptParser const& opts)
{
    QStatus status = ER_OK;

    status = propStore->setDeviceId(opts.GetDeviceId());
    if (status != ER_OK) {
        return status;
    }
    status = propStore->setAppId(opts.GetAppId());
    if (status != ER_OK) {
        return status;
    }


    std::vector<qcc::String> languages(3);
    languages[0] = "en";
    languages[1] = "sp";
    languages[2] = "fr";
    status = propStore->setSupportedLangs(languages);
    if (status != ER_OK) {
        return status;
    }
    status = propStore->setDefaultLang(opts.GetDefaultLanguage());
    if (status != ER_OK) {
        return status;
    }

    status = propStore->setAppName("About Config", "en");
    if (status != ER_OK) {
        return status;
    }

    status = propStore->setAppName("Acerca Config", "sp");
    if (status != ER_OK) {
        return status;
    }

    status = propStore->setAppName("À propos de la configuration", "fr");
    if (status != ER_OK) {
        return status;
    }

    status = propStore->setModelNumber("Wxfy388i");
    if (status != ER_OK) {
        return status;
    }

    status = propStore->setDateOfManufacture("2199-10-01");
    if (status != ER_OK) {
        return status;
    }

    status = propStore->setSoftwareVersion("12.20.44 build 44454");
    if (status != ER_OK) {
        return status;
    }

    status = propStore->setAjSoftwareVersion(ajn::GetVersion());
    if (status != ER_OK) {
        return status;
    }

    status = propStore->setHardwareVersion("355.499. b");
    if (status != ER_OK) {
        return status;
    }

    status = propStore->setDeviceName("My device name", "en");
    if (status != ER_OK) {
        return status;
    }

    status = propStore->setDeviceName("Mi nombre de dispositivo", "sp");
    if (status != ER_OK) {
        return status;
    }

    status = propStore->setDeviceName("Mon nom de l'appareil", "fr");
    if (status != ER_OK) {
        return status;
    }

    status = propStore->setDescription("This is an Alljoyn Application", "en");
    if (status != ER_OK) {
        return status;
    }

    status = propStore->setDescription("Esta es una Alljoyn aplicacion", "sp");
    if (status != ER_OK) {
        return status;
    }

    status = propStore->setDescription("C'est une Alljoyn application", "fr");
    if (status != ER_OK) {
        return status;
    }

    status = propStore->setManufacturer("Company", "en");
    if (status != ER_OK) {
        return status;
    }

    status = propStore->setManufacturer("Empresa", "sp");
    if (status != ER_OK) {
        return status;
    }

    status = propStore->setManufacturer("Entreprise", "fr");
    if (status != ER_OK) {
        return status;
    }

    status = propStore->setSupportUrl("http://www.alljoyn.org");
    if (status != ER_OK) {
        return status;
    }
    return status;
}

void WaitForSigInt(void) {
    while (s_interrupt == false) {
#ifdef _WIN32
        Sleep(100);
#else
        usleep(100 * 1000);
#endif
    }
}

static void shutdown(AboutPropertyStoreImpl*& aboutPropertyStore, AboutIconService*& aboutIconService)
{
    s_msgBus->CancelAdvertiseName(s_msgBus->GetUniqueName().c_str(), TRANSPORT_ANY);
    s_msgBus->UnregisterBusListener(s_busListener);
    s_msgBus->UnbindSessionPort(s_busListener.getSessionPort());

    AboutServiceApi::DestroyInstance();

    if (aboutPropertyStore) {
        delete aboutPropertyStore;
        aboutPropertyStore = NULL;
    }

    if (aboutIconService) {
        delete aboutIconService;
        aboutIconService = NULL;
    }

    s_msgBus->Stop();
    s_msgBus->Join();
    delete s_msgBus;
    s_msgBus = NULL;
}

int main(int argc, char**argv, char**envArg) {
    QStatus status = ER_OK;
    std::cout << "AllJoyn Library version: " << ajn::GetVersion() << std::endl;
    std::cout << "AllJoyn Library build info: " << ajn::GetBuildInfo() << std::endl;
    // Uncoment to get additional logging information.
    //QCC_SetLogLevels("ALLJOYN_ABOUT_SERVICE=7;");
    //QCC_SetLogLevels("ALLJOYN_ABOUT_ICON_SERVICE=7;");

    OptParser opts(argc, argv);
    OptParser::ParseResultCode parseCode(opts.ParseResult());
    switch (parseCode) {
    case OptParser::PR_OK:
        break;

    case OptParser::PR_EXIT_NO_ERROR:
        return SERVICE_EXIT_OK;

    default:
        return SERVICE_OPTION_ERROR;
    }

    SERVICE_PORT = opts.GetPort();
    s_busListener.setSessionPort(SERVICE_PORT);
    std::cout << "using port " << opts.GetPort() << std::endl;

    if (!opts.GetAppId().empty()) {
        std::cout << "using appID " << opts.GetAppId().c_str() << std::endl;
    }

    /* Install SIGINT handler so Ctrl + C deallocates memory properly */
    signal(SIGINT, SigIntHandler);

    //set Daemon password only for bundled app
    #ifdef QCC_USING_BD
    PasswordManager::SetCredentials("ALLJOYN_PIN_KEYX", "000000");
    #endif

    /* Create message bus */
    s_msgBus = new BusAttachment("AboutServiceName", true);

    if (!s_msgBus) {
        status = ER_OUT_OF_MEMORY;
        return status;
    }

    if (ER_OK == status) {
        status = StartMessageBus();
    }

    if (ER_OK == status) {
        status = ConnectToDaemon();
    }

    if (ER_OK == status) {
        s_msgBus->RegisterBusListener(s_busListener);
    }

    AboutIconService* aboutIconService = NULL;
    AboutPropertyStoreImpl* aboutPropertyStore = NULL;

    if (ER_OK == status) {
        aboutPropertyStore = new AboutPropertyStoreImpl();
        status = FillAboutPropertyStoreImplData(aboutPropertyStore, opts);
        if (status != ER_OK) {
            shutdown(aboutPropertyStore, aboutIconService);
            return EXIT_FAILURE;
        }

        AboutServiceApi::Init(*s_msgBus, *aboutPropertyStore);
        if (!AboutServiceApi::getInstance()) {
            shutdown(aboutPropertyStore, aboutIconService);
            return EXIT_FAILURE;
        }

        AboutServiceApi::getInstance()->Register(SERVICE_PORT);
        status = s_msgBus->RegisterBusObject(*AboutServiceApi::getInstance());

        uint8_t aboutIconContent[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44,
                                       0x52, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x0A, 0x08, 0x02, 0x00, 0x00, 0x00, 0x02, 0x50, 0x58, 0xEA, 0x00,
                                       0x00, 0x00, 0x04, 0x67, 0x41, 0x4D, 0x41, 0x00, 0x00, 0xAF, 0xC8, 0x37, 0x05, 0x8A, 0xE9, 0x00, 0x00, 0x00, 0x19,
                                       0x74, 0x45, 0x58, 0x74, 0x53, 0x6F, 0x66, 0x74, 0x77, 0x61, 0x72, 0x65, 0x00, 0x41, 0x64, 0x6F, 0x62, 0x65, 0x20,
                                       0x49, 0x6D, 0x61, 0x67, 0x65, 0x52, 0x65, 0x61, 0x64, 0x79, 0x71, 0xC9, 0x65, 0x3C, 0x00, 0x00, 0x00, 0x18, 0x49,
                                       0x44, 0x41, 0x54, 0x78, 0xDA, 0x62, 0xFC, 0x3F, 0x95, 0x9F, 0x01, 0x37, 0x60, 0x62, 0xC0, 0x0B, 0x46, 0xAA, 0x34,
                                       0x40, 0x80, 0x01, 0x00, 0x06, 0x7C, 0x01, 0xB7, 0xED, 0x4B, 0x53, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E,
                                       0x44, 0xAE, 0x42, 0x60, 0x82 };

        qcc::String mimeType("image/png");
        qcc::String url(""); //put your url here

        std::vector<qcc::String> interfaces;
        interfaces.push_back("org.alljoyn.Icon");
        status = AboutServiceApi::getInstance()->AddObjectDescription("/About/DeviceIcon", interfaces);

        aboutIconService = new AboutIconService(*s_msgBus, mimeType, url, aboutIconContent,
                                                sizeof(aboutIconContent) / sizeof(*aboutIconContent));
        aboutIconService->Register();

        status = s_msgBus->RegisterBusObject(*aboutIconService);
    }

    const TransportMask SERVICE_TRANSPORT_TYPE = TRANSPORT_ANY;

    if (ER_OK == status) {
        status = BindSession(SERVICE_TRANSPORT_TYPE);
    }

    if (ER_OK == status) {
        status = AdvertiseName(SERVICE_TRANSPORT_TYPE);
    }

    if (ER_OK == status) {
        status = AboutServiceApi::getInstance()->Announce();

    }

    /* Perform the service asynchronously until the user signals for an exit. */
    if (ER_OK == status) {
        WaitForSigInt();
    }

    shutdown(aboutPropertyStore, aboutIconService);

    return 0;
} /* main() */




