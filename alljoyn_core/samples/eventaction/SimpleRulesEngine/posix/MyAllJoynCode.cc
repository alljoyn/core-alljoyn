/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#include "MyAllJoynCode.h"

using namespace std;
using namespace ajn;
using namespace qcc;
using namespace services;

void MyAllJoynCode::initialize(const char* packageName) {
    QStatus status = ER_OK;

    /* Initialize AllJoyn only once */
    if (!mBusAttachment) {
        /*
         * All communication through AllJoyn begins with a BusAttachment.
         *
         * A BusAttachment needs a name. The actual name is unimportant except for internal
         * security. As a default we use the class name as the name.
         *
         * By default AllJoyn does not allow communication between devices (i.e. bus to bus
         * communication).  The second argument must be set to Receive to allow
         * communication between devices.
         */
        mBusAttachment = new BusAttachment(packageName, true);
        /* Start the msg bus */
        if (ER_OK == status) {
            status = mBusAttachment->Start();
        } else {
            LOGTHIS("BusAttachment::Start failed");
        }
        /* Connect to the daemon */
        if (ER_OK == status) {
            status = mBusAttachment->Connect();
            if (ER_OK != status) {
                LOGTHIS("BusAttachment Connect failed.");
            }
        }
        LOGTHIS("Created BusAttachment and connected");

        /* Register the BusListener */
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, true, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        SessionPort sp = 800;
        QStatus status = mBusAttachment->BindSessionPort(sp, opts, (*this));
        if (ER_OK != status) {
            LOGTHIS("BindSessionPort failed");
        } else {
            LOGTHIS("Bind Session Port to was successful ");
        }

        OptParser aboutOpts(0, NULL);
        std::multimap<qcc::String, PropertyStoreImpl::Property> data;
        FillPropertyStoreData(aboutOpts, data, packageName);
        propertyStoreImpl = new PropertyStoreImpl(data);

        AboutServiceApi::Init(*mBusAttachment, *propertyStoreImpl);

        aboutService = AboutServiceApi::getInstance();

        status = aboutService->Register(800);
        if (ER_OK != status) {
            LOGTHIS("Failed to register about! %d", status);
        } else {
            LOGTHIS("Should have registered about");
        }

        status = mBusAttachment->RegisterBusObject(*aboutService);
        if (ER_OK == status) {
            LOGTHIS("Registered BusObjects");

        } else {
            LOGTHIS("Registering BusObject failed :(");
        }

        ruleBusObject = new RuleBusObject(mBusAttachment, "/ruleengine", &ruleEngine);
        status = mBusAttachment->RegisterBusObject(*ruleBusObject);
        if (ER_OK == status) {
            LOGTHIS("Registered BusObjects");

        } else {
            LOGTHIS("Registering BusObject failed :(");
        }

        std::vector<String> interfaces;
        interfaces.push_back("org.allseen.sample.rule.engine");
        aboutService->AddObjectDescription("/ruleengine", interfaces);

        AnnouncementRegistrar::RegisterAnnounceHandler(*mBusAttachment, *this, NULL, 0);

        status = mBusAttachment->AddMatch("sessionless='t'");

        if (ER_OK != status) {
            LOGTHIS("Failed to addMatch for sessionless signals: %s\n", QCC_StatusText(status));
        }

        LOGTHIS("Going to setup rule Engine");
        status = ruleEngine.initialize("simple", mBusAttachment);
        if (ER_OK != status) {
            LOGTHIS("Failed to start rule engine");
        }

        status = mBusAttachment->AdvertiseName(mBusAttachment->GetUniqueName().c_str(), opts.transports);
        if (status != ER_OK) {
            LOGTHIS("Failed to advertise name");
        } else {
            LOGTHIS("Advertisement was successfully advertised");
        }

        aboutService->Announce();
    }
}

void MyAllJoynCode::Announce(unsigned short version, unsigned short port, const char* busName,
                             const ObjectDescriptions& objectDescs,
                             const AboutData& aboutData)
{
    LOGTHIS("Found about application with busName, port %s, %d", busName, port);
    if (mBusAttachment->GetUniqueName().compare(busName) == 0) {
        LOGTHIS("Found myself :)");
    }
    //For now lets just assume everything has events and actions and join
    for (AboutClient::AboutData::const_iterator it = aboutData.begin(); it != aboutData.end(); ++it) {
        qcc::String key = it->first;
        ajn::MsgArg value = it->second;
        if (value.typeId == ALLJOYN_STRING) {
            if (key.compare("DeviceName") == 0) {
                mBusFriendlyMap.insert(std::pair<qcc::String, qcc::String>(busName, value.v_string.str));
            }
            LOGTHIS("(Announce handler) aboutData (key, val) (%s, %s)", key.c_str(), value.v_string.str);
        }
    }
    //pass through to ruleEngine
    ruleEngine.Announce(version, port, busName, objectDescs, aboutData);
}


void MyAllJoynCode::shutdown()
{
    /* Cancel the advertisement */
    /* Unregister the Bus Listener */
    mBusAttachment->UnregisterBusListener(*((BusListener*)this));
    /* Deallocate the BusAttachment */
    if (mBusAttachment) {
        delete mBusAttachment;
        mBusAttachment = NULL;
    }
}

/* From SessionPortListener */
bool MyAllJoynCode::AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
{
    return true;
}

void MyAllJoynCode::SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner)
{
    LOGTHIS("SessionJoined!");
}

/* From SessionListener */
void MyAllJoynCode::SessionLost(ajn::SessionId sessionId)
{

}

void MyAllJoynCode::SessionMemberAdded(ajn::SessionId sessionId, const char*uniqueName)
{

}

void MyAllJoynCode::SessionMemberRemoved(ajn::SessionId sessionId, const char*uniqueName)
{

}

char MyAllJoynCode::HexToChar(char c)
{
    if ('0' <= c && c <= '9') {
        return c - '0';
    } else if ('a' <= c && c <= 'f') {
        return c + 10 - 'a';
    } else if ('A' <= c && c <= 'F') {
        return c + 10 - 'A';
    }

    return -1;
}

void MyAllJoynCode::HexStringToBytes(const qcc::String& hex, uint8_t* outBytes, size_t len)
{
    unsigned char achar, bchar;
    for (size_t i = 0; i < len; i++) {
        achar = HexToChar(hex[i * 2]);
        bchar = HexToChar(hex[i * 2 + 1]);
        outBytes[i] = ((achar << 4) | bchar);
    }
}

void MyAllJoynCode::FillPropertyStoreData(OptParser const& opts, std::multimap<qcc::String, PropertyStoreImpl::Property>& data, const char* friendlyName)
{
    if (data.size() == 0) {
        data.insert(
            std::pair<qcc::String, PropertyStoreImpl::Property>("DefaultLanguage",
                                                                PropertyStoreImpl::Property("DefaultLanguage", MsgArg("s", "en"), true, true, true)));
        srand(time(NULL));
        qcc::String devName = friendlyName;
        data.insert(
            std::pair<qcc::String, PropertyStoreImpl::Property>("DeviceName",
                                                                PropertyStoreImpl::Property("DeviceName", MsgArg("s", devName.c_str()), true, true, true)));

        qcc::String devId = "";
        for (int i = 0; i < 16; i++) {
            /* Rand val from 0-9 */
            devId += '0' + rand() % 10;
        }
        data.insert(
            std::pair<qcc::String, PropertyStoreImpl::Property>("DeviceId",
                                                                PropertyStoreImpl::Property("DeviceId", MsgArg("s", devId.c_str()), true, false, true)));
        data.insert(
            std::pair<qcc::String, PropertyStoreImpl::Property>("Description",
                                                                PropertyStoreImpl::Property("Description", MsgArg("s", "This is a sample rule application for developers to use as a simple reference application."), true, false, false)));
    }

    uint8_t AppId[16];
    HexStringToBytes(opts.GetAppID(), AppId, 16);

    data.insert(
        std::pair<qcc::String, PropertyStoreImpl::Property>("AppId",
                                                            PropertyStoreImpl::Property("AppId", MsgArg("ay", sizeof(AppId) / sizeof(*AppId), AppId), true, false,
                                                                                        true)));

    data.insert(
        std::pair<qcc::String, PropertyStoreImpl::Property>("AppName",
                                                            PropertyStoreImpl::Property("AppName", MsgArg("s", "SampleRuleEngine"), true, false, true)));
    data.insert(
        std::pair<qcc::String, PropertyStoreImpl::Property>("Manufacturer",
                                                            PropertyStoreImpl::Property("Manufacturer", MsgArg("s", "AllSeen Developer Sample"), true, false, true)));
    data.insert(
        std::pair<qcc::String, PropertyStoreImpl::Property>("ModelNumber",
                                                            PropertyStoreImpl::Property("ModelNumber", MsgArg("s", "Sample-1"), true, false, true)));
    const char*languages[] = { "en" };
    data.insert(
        std::pair<qcc::String, PropertyStoreImpl::Property>("SupportedLanguages",
                                                            PropertyStoreImpl::Property("SupportedLanguages",
                                                                                        MsgArg("as", sizeof(languages) / sizeof(*languages), languages), true, false, false)));
    data.insert(
        std::pair<qcc::String, PropertyStoreImpl::Property>("DateOfManufacture",
                                                            PropertyStoreImpl::Property("DateOfManufacture", MsgArg("s", "06/06/2014"), true, false, false)));
    data.insert(
        std::pair<qcc::String, PropertyStoreImpl::Property>("SoftwareVersion",
                                                            PropertyStoreImpl::Property("SoftwareVersion", MsgArg("s", ".001"), true, false, false)));
    data.insert(
        std::pair<qcc::String, PropertyStoreImpl::Property>("AJSoftwareVersion",
                                                            PropertyStoreImpl::Property("AJSoftwareVersion", MsgArg("s", ajn::GetVersion()), true, false, false)));
    data.insert(
        std::pair<qcc::String, PropertyStoreImpl::Property>("HardwareVersion",
                                                            PropertyStoreImpl::Property("HardwareVersion", MsgArg("s", "Stuffing01"), true, false, false)));
    data.insert(
        std::pair<qcc::String, PropertyStoreImpl::Property>("SupportUrl",
                                                            PropertyStoreImpl::Property("SupportUrl", MsgArg("s", "http://www.allseenalliance.org"), true, false, false)));
}
