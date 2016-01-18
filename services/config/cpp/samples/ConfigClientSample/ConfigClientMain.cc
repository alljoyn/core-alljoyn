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

#include <signal.h>
#include <set>

#include <alljoyn/config/ConfigClient.h>
#include <alljoyn/AboutData.h>
#include <alljoyn/AboutListener.h>
#include <alljoyn/AboutObjectDescription.h>
#include <alljoyn/AboutProxy.h>
#include <alljoyn/AboutIconProxy.h>
#include <alljoyn/version.h>
#include <alljoyn/config/LogModule.h>
#include "../common/SrpKeyXListener.h"
#include "../common/AJInitializer.h"
#include "AsyncSessionJoiner.h"
#include "SessionListenerImpl.h"
#include <iostream>
#include <iomanip>

using namespace ajn;

#define INITIAL_PASSCODE "000000"
#define NEW_PASSCODE "12345678"

static BusAttachment* busAttachment = 0;
static SrpKeyXListener* srpKeyXListener = 0;
static std::set<qcc::String> handledAnnouncements;

static volatile sig_atomic_t s_interrupt = false;
static volatile sig_atomic_t s_stopped = false;

static void CDECL_CALL SigIntHandler(int sig) {
    QCC_UNUSED(sig);
    s_interrupt = true;
}

// Print out the fields found in the AboutData. Only fields with known signatures
// are printed out.  All others will be treated as an unknown field.
void printAboutData(AboutData& aboutData, const char* language)
{
    size_t count = aboutData.GetFields();

    const char** fields = new const char*[count];
    aboutData.GetFields(fields, count);

    for (size_t i = 0; i < count; ++i) {
        std::cout << "\tKey: " << fields[i];

        MsgArg* tmp;
        aboutData.GetField(fields[i], tmp, language);
        std::cout << "\t";
        if (tmp->Signature() == "s") {
            const char* tmp_s;
            tmp->Get("s", &tmp_s);
            std::cout << tmp_s;
        } else if (tmp->Signature() == "as") {
            size_t las;
            MsgArg* as_arg;
            tmp->Get("as", &las, &as_arg);
            for (size_t j = 0; j < las; ++j) {
                const char* tmp_s;
                as_arg[j].Get("s", &tmp_s);
                std::cout << tmp_s << " ";
            }
        } else if (tmp->Signature() == "ay") {
            size_t lay;
            uint8_t* pay;
            tmp->Get("ay", &lay, &pay);
            for (size_t j = 0; j < lay; ++j) {
                std::cout << std::hex << static_cast<int>(pay[j]) << " ";
            }
        } else {
            std::cout << "User Defined Value\tSignature: " << tmp->Signature().c_str();
        }
        std::cout << std::endl;
    }
    delete [] fields;
    std::cout << std::endl;
}

void printAllAboutData(AboutProxy& aboutProxy)
{
    MsgArg aArg;
    QStatus status = aboutProxy.GetAboutData(NULL, aArg);
    if (status == ER_OK) {
        std::cout << "*********************************************************************************" << std::endl;
        std::cout << "GetAboutData: (Default Language)" << std::endl;
        AboutData aboutData(aArg);
        printAboutData(aboutData, NULL);
        size_t lang_num;
        lang_num = aboutData.GetSupportedLanguages();
        std::cout << "Number of supported languages: " << lang_num << std::endl;
        // If the lang_num == 1 we only have a default language
        if (lang_num > 1) {
            const char** langs = new const char*[lang_num];
            aboutData.GetSupportedLanguages(langs, lang_num);
            char* defaultLanguage;
            aboutData.GetDefaultLanguage(&defaultLanguage);
            // print out the AboutData for every language but the
            // default it has already been printed.
            for (size_t i = 0; i < lang_num; ++i) {
                std::cout << "language=" << i << " " << langs[i] << std::endl;
                if (strcmp(defaultLanguage, langs[i]) != 0) {
                    std::cout << "Calling GetAboutData: language=" << langs[i] << std::endl;
                    status = aboutProxy.GetAboutData(langs[i], aArg);
                    if (ER_OK == status) {
                        AboutData localized(aArg, langs[i]);
                        std::cout <<  "GetAboutData: (" << langs[i] << ")" << std::endl;
                        printAboutData(localized, langs[i]);
                    } else {
                        std::cout <<  "GetAboutData failed " << QCC_StatusText(status)  << std::endl;
                    }
                }
            }
            delete [] langs;
        }
        std::cout << "*********************************************************************************" << std::endl;
    }
}

void interruptibleDelay(int seconds) {
    for (int i = 0; !s_interrupt && i < seconds; i++) {
#ifdef _WIN32
        Sleep(1000);
#else
        usleep(1000 * 1000);
#endif
    }
}

void sessionJoinedCallback(qcc::String const& busName, SessionId id)
{
    std::cout << "sessionJoinedCallback(" << "busName=" << busName.c_str() << " SessionId=" << id << ")" << std::endl;
    QStatus status = ER_OK;
    busAttachment->EnableConcurrentCallbacks();
    AboutProxy aboutProxy(*busAttachment, busName.c_str(), id);

    MsgArg objArg;
    aboutProxy.GetObjectDescription(objArg);
    std::cout << "AboutProxy.GetObjectDescriptions:\n" << objArg.ToString().c_str() << "\n\n" << std::endl;

    AboutObjectDescription objectDescription;
    objectDescription.CreateFromMsgArg(objArg);

    bool isIconInterface = false;
    if (!s_interrupt) {
        isIconInterface = objectDescription.HasInterface("/About/DeviceIcon", "org.alljoyn.Icon");

        if (isIconInterface) {
            std::cout << "The given interface 'org.alljoyn.Icon' is found in a given path '/About/DeviceIcon'" << std::endl;
        } else {
            std::cout << "WARNING - The given interface 'org.alljoyn.Icon' is not found in a given path '/About/DeviceIcon'" << std::endl;
        }
    }

    bool isConfigInterface = false;
    if (!s_interrupt) {
        isConfigInterface = objectDescription.HasInterface("/Config", "org.alljoyn.Config");
        if (isConfigInterface) {
            std::cout << "The given interface 'org.alljoyn.Config' is found in a given path '/Config'" << std::endl;
        } else {
            std::cout << "WARNING - The given interface 'org.alljoyn.Config' is not found in a given path '/Config'" << std::endl;
        }

        printAllAboutData(aboutProxy);
    }

    if (!s_interrupt) {
        std::cout << "aboutProxy GetVersion " << std::endl;
        std::cout << "-----------------------" << std::endl;

        uint16_t version = 0;
        status = aboutProxy.GetVersion(version);
        if (status != ER_OK) {
            std::cout << "WARNING - Call to getVersion failed " << QCC_StatusText(status) << std::endl;
        } else {
            std::cout << "Version=" << version << std::endl;
        }
    }

    if (!s_interrupt) {
        if (isIconInterface) {
            AboutIconProxy aiProxy(*busAttachment, busName.c_str(), id);
            AboutIcon aboutIcon;

            std::cout << std::endl << busName.c_str() << " AboutIconProxy GetIcon" << std::endl;
            std::cout << "-----------------------------------" << std::endl;

            status = aiProxy.GetIcon(aboutIcon);
            if (status != ER_OK) {
                std::cout << "WARNING - Call to GetIcon failed: " << QCC_StatusText(status) << std::endl;
            }

            std::cout << "url=" << aboutIcon.url.c_str() << std::endl;
            std::cout << "Content size = " << aboutIcon.contentSize << std::endl;
            std::cout << "Content =\t";
            for (size_t i = 0; i < aboutIcon.contentSize; i++) {
                if (i % 8 == 0 && i > 0) {
                    std::cout << "\n\t\t";
                }
                std::cout << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (unsigned int)aboutIcon.content[i]
                          << std::nouppercase << std::dec;

                //std::cout << std::endl;
            }
            std::cout << std::endl;
            std::cout << "Mimetype =\t" << aboutIcon.mimetype.c_str() << std::endl;
            std::cout << std::endl << busName.c_str() << " AboutIcontClient GetVersion" << std::endl;
            std::cout << "-----------------------------------" << std::endl;

            uint16_t version;
            status = aiProxy.GetVersion(version);
            if (status != ER_OK) {
                std::cout << "WARNING - Call to getVersion failed: " << QCC_StatusText(status) << std::endl;
            } else {
                std::cout << "Version=" << version << std::endl;
            }
        }
    }

    services::ConfigClient* configClient = NULL;
    if (!s_interrupt && isConfigInterface) {
        configClient = new services::ConfigClient(*busAttachment);
        if (!s_interrupt && configClient) {
            std::cout << "\nConfigClient GetVersion" << std::endl;
            std::cout << "-----------------------------------" << std::endl;
            int version;
            if ((status = configClient->GetVersion(busName.c_str(), version, id)) == ER_OK) {
                std::cout << "Success GetVersion. Version=" << version << std::endl;
            } else {
                std::cout << "WARNING - Call to getVersion failed: " << QCC_StatusText(status) << std::endl;
            }

            if (!s_interrupt) {
                services::ConfigClient::Configurations configurations;
                std::cout << "\nConfigClient GetConfigurations (en)" << std::endl;
                std::cout << "-----------------------------------" << std::endl;

                if ((status = configClient->GetConfigurations(busName.c_str(), "en", configurations, id))
                    == ER_OK) {

                    for (services::ConfigClient::Configurations::iterator it = configurations.begin();
                         it != configurations.end(); ++it) {
                        qcc::String key = it->first;
                        ajn::MsgArg value = it->second;
                        if (value.typeId == ALLJOYN_STRING) {
                            std::cout << "Key name=" << key.c_str() << " value=" << value.v_string.str << std::endl;
                        } else if (value.typeId == ALLJOYN_ARRAY && value.Signature().compare("as") == 0) {
                            std::cout << "Key name=" << key.c_str() << " values: ";
                            const MsgArg* stringArray;
                            size_t fieldListNumElements;
                            status = value.Get("as", &fieldListNumElements, &stringArray);
                            for (unsigned int i = 0; i < fieldListNumElements; i++) {
                                char* tempString;
                                stringArray[i].Get("s", &tempString);
                                std::cout << tempString << " ";
                            }
                            std::cout << std::endl;
                        }
                    }
                } else {
                    std::cout << "WARNING - Call to GetConfigurations failed: " << QCC_StatusText(status) << std::endl;
                }
            }

            if (!s_interrupt) {
                // In order to make the Restart call here, the client would have to be
                // written to expect the session to be lost and to restablish it.
                //
                //std::cout << "\nGoing to call to ConfigClient Restart" << std::endl;
                //std::cout << "-----------------------------------" << std::endl;
                //
                //if ((status = configClient->Restart(busName.c_str(), id)) == ER_OK) {
                //    std::cout << "Restart succeeded" << std::endl;
                //} else {
                //    std::cout << "WARNING - Call to Restart failed: " << QCC_StatusText(status) << std::endl;
                //}
            }

            if (!s_interrupt) {
                std::cout << "\nGoing to call to UpdateConfigurations: key=DeviceName value=This is my new English name ! ! ! !" << std::endl;
                std::cout << "-----------------------------------------------------------------------------------------------" << std::endl;
                services::ConfigClient::Configurations updateConfigurations;
                updateConfigurations.insert(
                    std::pair<qcc::String, ajn::MsgArg>("DeviceName",
                                                        MsgArg("s", "This is my new English name ! ! ! !")));

                if ((status = configClient->UpdateConfigurations(busName.c_str(), "en", updateConfigurations, id)) == ER_OK) {
                    std::cout << "UpdateConfigurations succeeded" << std::endl;
                } else {
                    std::cout << "WARNING - Call to UpdateConfigurations failed: " << QCC_StatusText(status) << std::endl;
                }

                printAllAboutData(aboutProxy);
            }

            interruptibleDelay(3);

            if (!s_interrupt) {
                std::cout << "\nGoing to call to UpdateConfigurations: key=DefaultLanguage value=es" << std::endl;
                std::cout << "-------------------------------------------------------------------" << std::endl;
                services::ConfigClient::Configurations updateConfigurations;
                updateConfigurations.insert(
                    std::pair<qcc::String, ajn::MsgArg>("DefaultLanguage", MsgArg("s", "es")));
                if ((status = configClient->UpdateConfigurations(busName.c_str(), NULL, updateConfigurations,
                                                                 id)) == ER_OK) {
                    std::cout << "UpdateConfigurations succeeded" << std::endl;
                } else {
                    std::cout << "WARNING - Call to UpdateConfigurations failed: " << QCC_StatusText(status) << std::endl;
                }

                printAllAboutData(aboutProxy);
            }

            interruptibleDelay(3);

            if (!s_interrupt) {
                std::vector<qcc::String> configNames;
                configNames.push_back("DeviceName");

                std::cout << "\nGoing to call to ConfigClient ResetConfigurations: key='DeviceName' lang='en'" << std::endl;
                std::cout << "-----------------------------------" << std::endl;

                if ((status = configClient->ResetConfigurations(busName.c_str(), "en", configNames, id)) == ER_OK) {
                    std::cout << "ResetConfigurations succeeded" << std::endl;
                } else {
                    std::cout << "WARNING - Call to ResetConfigurations failed: " << QCC_StatusText(status) << std::endl;
                }

                printAllAboutData(aboutProxy);
            }

            interruptibleDelay(3);

            if (!s_interrupt) {
                std::cout << "\nGoing to call to ConfigClient SetPasscode" << std::endl;
                std::cout << "-----------------------------------" << std::endl;
                if ((status = configClient->SetPasscode(busName.c_str(), "MyDeamonRealm", 8,
                                                        (const uint8_t*) NEW_PASSCODE, id)) == ER_OK) {
                    std::cout << "SetPasscode succeeded" << std::endl;
                    srpKeyXListener->setPassCode(NEW_PASSCODE);
                    qcc::String guid;
                    status = busAttachment->GetPeerGUID(busName.c_str(), guid);
                    if (status == ER_OK) {

                        status = busAttachment->ClearKeys(guid);
                        std::cout << "busAttachment->ClearKey for " << guid.c_str() << ". Status: " << QCC_StatusText(status) << std::endl;
                    }
                } else {
                    std::cout << "WARNING - Call to SetPasscode failed: " << QCC_StatusText(status) << std::endl;
                }
            }

            if (!s_interrupt) {
                std::cout << "\nGoing to call to ConfigClient FactoryReset" << std::endl;
                std::cout << "-----------------------------------" << std::endl;

                if ((status = configClient->FactoryReset(busName.c_str(), id)) == ER_OK) {
                    std::cout << "FactoryReset succeeded" << std::endl;
                    srpKeyXListener->setPassCode(INITIAL_PASSCODE);
                    qcc::String guid;
                    status = busAttachment->GetPeerGUID(busName.c_str(), guid);
                    if (status == ER_OK) {
                        busAttachment->ClearKeys(guid);
                    }

                } else {
                    std::cout << "WARNING - Call to FactoryReset failed: " << QCC_StatusText(status) << std::endl;
                }

                printAllAboutData(aboutProxy);
            }
        } //if (configClient)
    } //if (isConfigInterface)

    status = busAttachment->LeaveSession(id);
    std::cout << "Leaving session id = " << id << " with " << busName.c_str() << " status: " << QCC_StatusText(status) << std::endl;

    if (configClient) {
        delete configClient;
        configClient = NULL;
    }

    s_stopped = true;
}

class MyAboutListener : public AboutListener {
    void Announced(const char* busName, uint16_t version, SessionPort port, const MsgArg& objectDescriptionArg, const MsgArg& aboutDataArg)
    {
        QCC_UNUSED(version);
        QCC_UNUSED(objectDescriptionArg);
        QCC_UNUSED(aboutDataArg);
        std::set<qcc::String>::iterator searchIterator = handledAnnouncements.find(qcc::String(busName));
        if (searchIterator == handledAnnouncements.end()) {
            handledAnnouncements.insert(busName);

            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
            SessionListenerImpl* sessionListener = new SessionListenerImpl(busName);
            AsyncSessionJoiner* joincb = new AsyncSessionJoiner(busName, sessionJoinedCallback);

            QStatus status = busAttachment->JoinSessionAsync(busName, port, sessionListener, opts, joincb,
                                                             sessionListener);

            if (status != ER_OK) {
                std::cout << "Unable to JoinSession with " << busName << std::endl;
            }
        } else {
            std::cout << "WARNING - " << busName  << " has already been handled" << std::endl;
        }
    }
};

void WaitForSigInt(void) {
    while (s_interrupt == false && s_stopped == false) {
#ifdef _WIN32
        Sleep(100);
#else
        usleep(100 * 1000);
#endif
    }
}

int CDECL_CALL main(int argc, char**argv, char**envArg)
{
    QCC_UNUSED(argc);
    QCC_UNUSED(argv);
    QCC_UNUSED(envArg);
    // Initialize AllJoyn
    AJInitializer ajInit;
    if (ajInit.Initialize() != ER_OK) {
        return 1;
    }

    QStatus status = ER_OK;
    std::cout << "AllJoyn Library version: " << ajn::GetVersion() << std::endl;
    std::cout << "AllJoyn Library build info: " << ajn::GetBuildInfo() << std::endl;

    std::cout << "*********************************************************************************" << std::endl;
    std::cout << "PLEASE NOTE THAT AS OF NOW THIS PROGRAM DOES NOT SUPPORT INTERACTION WITH THE ALLJOYN THIN CLIENT BASED CONFIGSAMPLE. SO PLEASE USE THIS PROGRAM ONLY WITH ALLJOYN STANDARD CLIENT BASED CONFIGSERVICESAMPLE" << std::endl;
    std::cout << "*********************************************************************************" << std::endl;

    //Enable this line to see logs from config service:
    //QCC_SetDebugLevel(services::logModules::CONFIG_MODULE_LOG_NAME, services::logModules::ALL_LOG_LEVELS);

    /* Install SIGINT handler so Ctrl + C deallocates memory properly */
    signal(SIGINT, SigIntHandler);

    busAttachment = new BusAttachment("ConfigClient", true);

    status = busAttachment->Start();
    if (status == ER_OK) {
        std::cout << "BusAttachment started." << std::endl;
    } else {
        std::cout << "ERROR - Unable to start BusAttachment. Status: " << QCC_StatusText(status) << std::endl;
        return 1;
    }

    status = busAttachment->Connect();
    if (ER_OK == status) {
        std::cout << "Daemon Connect succeeded." << std::endl;
    } else {
        std::cout << "ERROR - Failed to connect daemon. Status: " << QCC_StatusText(status) << std::endl;
        return 1;
    }

    srpKeyXListener = new SrpKeyXListener();
    srpKeyXListener->setPassCode(INITIAL_PASSCODE);

    status = busAttachment->EnablePeerSecurity("ALLJOYN_SRP_KEYX ALLJOYN_ECDHE_PSK", srpKeyXListener,
                                               "/.alljoyn_keystore/central.ks", true);

    if (ER_OK == status) {
        std::cout << "EnablePeerSecurity called." << std::endl;
    } else {
        std::cout << "ERROR - EnablePeerSecurity call FAILED with status " << QCC_StatusText(status) << std::endl;
        return 1;
    }
    const char* interfaces[] = { "org.alljoyn.Config" };
    MyAboutListener* aboutListener = new MyAboutListener();
    busAttachment->RegisterAboutListener(*aboutListener);

    status = busAttachment->WhoImplements(interfaces, sizeof(interfaces) / sizeof(interfaces[0]));
    if (ER_OK == status) {
        std::cout << "WhoImplements called." << std::endl;
    } else {
        std::cout << "ERROR - WhoImplements call FAILED with status " << QCC_StatusText(status) << std::endl;
        return 1;
    }

    WaitForSigInt();

    std::cout << "Preparing to exit..." << std::endl;

    if (!s_stopped) {
        std::cout << "Waiting for a few seconds for commands to complete... " << std::endl;

        for (int i = 0; !s_stopped && i < 5; i++) {
#ifdef _WIN32
            Sleep(1000);
#else
            usleep(1000 * 1000);
#endif
        }
    }

    std::cout << "Cleaning up (press Ctrl-C to abort)... " << std::endl;

    busAttachment->CancelWhoImplements(interfaces, sizeof(interfaces) / sizeof(interfaces[0]));
    busAttachment->UnregisterAboutListener(*aboutListener);
    busAttachment->EnablePeerSecurity(NULL, NULL, NULL, true);

    delete srpKeyXListener;
    delete aboutListener;

    busAttachment->Stop();
    delete busAttachment;

    std::cout << "Done." << std::endl;

    return 0;

} /* main() */

