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
#include "MyAllJoynCode.h"

using namespace std;
using namespace ajn;
using namespace qcc;

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
        aboutData.SetDefaultLanguage("en");
        aboutData.SetDeviceName(packageName);
        srand(time(NULL));
        qcc::String devId = "";
        for (int i = 0; i < 16; i++) {
            /* Rand val from 0-9 */
            devId += '0' + rand() % 10;
        }
        aboutData.SetDeviceId(devId.c_str());
        aboutData.SetDescription("This is a sample rule application for developers to use as a simple reference application.");
        uint8_t appId[16];
        HexStringToBytes(aboutOpts.GetAppID(), appId, sizeof(appId) / sizeof(*appId));
        aboutData.SetAppId(appId, sizeof(appId) / sizeof(*appId));
        aboutData.SetAppName("SampleRuleEngine");
        aboutData.SetManufacturer("AllSeen Developer Sample");
        aboutData.SetModelNumber("Sample-1");
        aboutData.SetDateOfManufacture("2014-06-06");
        aboutData.SetSoftwareVersion(".001");
        aboutData.SetHardwareVersion("Stuffing01");
        aboutData.SetSupportUrl("http://www.allseenalliance.org");

        aboutObj = new AboutObj(*mBusAttachment);

        ruleBusObject = new RuleBusObject(mBusAttachment, "/ruleengine", &ruleEngine);
        status = mBusAttachment->RegisterBusObject(*ruleBusObject);
        if (ER_OK == status) {
            LOGTHIS("Registered BusObjects");

        } else {
            LOGTHIS("Registering BusObject failed :(");
        }

        mBusAttachment->RegisterAboutListener(*this);
        status = mBusAttachment->WhoImplements("*");
        if (ER_OK != status) {
            LOGTHIS("Failed WhoImplements method call: %s\n", QCC_StatusText(status));
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

        status = aboutObj->Announce(sp, aboutData);
        if (status != ER_OK) {
            LOGTHIS("Failed to Announce: %s", QCC_StatusText(status));
        } else {
            LOGTHIS("Announce signal was successful.");
        }

    }
}

// Print out the fields found in the AboutData. Only fields with known signatures
// are printed out.  All others will be treated as an unknown field.
void LogAboutData(AboutData& aboutData, const char* language)
{
    size_t count = aboutData.GetFields();

    const char** fields = new const char*[count];
    aboutData.GetFields(fields, count);

    for (size_t i = 0; i < count; ++i) {
        MsgArg* tmp;
        aboutData.GetField(fields[i], tmp, language);
        if (tmp->Signature() == "s") {
            const char* tmp_s;
            tmp->Get("s", &tmp_s);
            LOGTHIS("(AnnounceListener) aboutData (key, val) (%s, %s)", fields[i], tmp_s);
        } else if (tmp->Signature() == "as") {
            size_t las;
            MsgArg* as_arg;
            tmp->Get("as", &las, &as_arg);
            qcc::String langs = "";
            for (size_t j = 0; j < las; ++j) {
                const char* tmp_s;
                as_arg[j].Get("s", &tmp_s);
                langs += tmp_s;
                if (j < las - 1) {
                    langs += ", ";
                }
            }
            LOGTHIS("(AnnounceListener) aboutData (key, val) (%s, [%s])", fields[i], langs.c_str());
        } else if (tmp->Signature() == "ay") {
            uint8_t* appIdBuffer;
            size_t numElements;
            tmp->Get("ay", &numElements, &appIdBuffer);
            char appIdStr[(numElements + 1) * 2];               //*2 due to hex format
            for (size_t i = 0; i < numElements; i++) {
                sprintf(appIdStr + (i * 2), "%02x", appIdBuffer[i]);
            }
            appIdStr[numElements * 2] = '\0';
            LOGTHIS("(AnnounceListener) aboutData (key, val) (%s, %s)", fields[i], appIdStr);
        } else {
            LOGTHIS("(AnnounceListener) aboutData (key, val) (%s, User Defined Value with '%s' signature value)", fields[i], tmp->Signature().c_str());
        }
    }
    delete [] fields;
}

void MyAllJoynCode::Announced(const char* busName, uint16_t version,
                              SessionPort port, const MsgArg& objectDescriptionArg,
                              const MsgArg& aboutDataArg)
{
    LOGTHIS("Found about application with busName, port %s, %d", busName, port);
    if (mBusAttachment->GetUniqueName().compare(busName) == 0) {
        LOGTHIS("Found myself :)");
    }
    //For now lets just assume everything has events and actions and join
    AboutData aboutData(aboutDataArg);
    char* deviceName;
    aboutData.GetDeviceName(&deviceName);
    mBusFriendlyMap.insert(std::pair<qcc::String, qcc::String>(busName, deviceName));

    LogAboutData(aboutData, NULL);

    //pass through to ruleEngine
    ruleEngine.Announce(busName, version, port, objectDescriptionArg, aboutDataArg);
}


void MyAllJoynCode::shutdown()
{
    /* Cancel the advertisement */
    /* Unregister the Bus Listener */
    mBusAttachment->UnregisterBusListener(*((BusListener*)this));

    delete aboutObj;
    aboutObj = NULL;

    /* Deallocate the BusAttachment */
    delete mBusAttachment;
    mBusAttachment = NULL;
}

/* From SessionPortListener */
bool MyAllJoynCode::AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
{
    QCC_UNUSED(sessionPort);
    QCC_UNUSED(joiner);
    QCC_UNUSED(opts);
    return true;
}

void MyAllJoynCode::SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner)
{
    QCC_UNUSED(sessionPort);
    QCC_UNUSED(id);
    QCC_UNUSED(joiner);
    LOGTHIS("SessionJoined!");
}

/* From SessionListener */
void MyAllJoynCode::SessionLost(ajn::SessionId sessionId)
{
    QCC_UNUSED(sessionId);
}

void MyAllJoynCode::SessionMemberAdded(ajn::SessionId sessionId, const char* uniqueName)
{
    QCC_UNUSED(sessionId);
    QCC_UNUSED(uniqueName);
}

void MyAllJoynCode::SessionMemberRemoved(ajn::SessionId sessionId, const char* uniqueName)
{
    QCC_UNUSED(sessionId);
    QCC_UNUSED(uniqueName);
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
