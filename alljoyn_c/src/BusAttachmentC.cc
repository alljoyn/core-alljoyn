/******************************************************************************
 * Copyright (c) 2012-2014, AllSeen Alliance. All rights reserved.
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

#include "BusAttachmentC.h"
#include <alljoyn/BusAttachment.h>
#include <alljoyn/MessageReceiver.h>
#include <alljoyn_c/InterfaceDescription.h>
#include <stdio.h>
#include "DeferredCallback.h"
#include <qcc/Debug.h>
#include <Rule.h>

#define QCC_MODULE "ALLJOYN_C"

using namespace std;
using namespace qcc;


namespace ajn {

void BusAttachmentC::SignalHandlerC::SignalHandlerRemap(const InterfaceDescription::Member* member, const char* srcPath, Message& message)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    alljoyn_interfacedescription_member c_member;

    c_member.iface = (alljoyn_interfacedescription)member->iface;
    c_member.memberType = (alljoyn_messagetype)member->memberType;
    c_member.name = member->name.c_str();
    c_member.signature = member->signature.c_str();
    c_member.returnSignature = member->returnSignature.c_str();
    c_member.argNames = member->argNames.c_str();
    //TODO add back in annotations
    //c_member.annotation = member->annotation;
    c_member.internal_member = member;

    if (!DeferredCallback::sMainThreadCallbacksOnly) {
        handler(&c_member, srcPath, (alljoyn_message)(&message));
    } else {
        /*
         * if MainThreadCallbacksOnly is true the memory for dcb will
         * be freed by the DeferedCallback base class.
         */
        DeferredCallback_3<void, const alljoyn_interfacedescription_member*, const char*, alljoyn_message>* dcb =
            new DeferredCallback_3<void, const alljoyn_interfacedescription_member*, const char*, alljoyn_message>(handler, &c_member, srcPath, (alljoyn_message) & message);
        DEFERRED_CALLBACK_EXECUTE(dcb);
    }
}


QStatus BusAttachmentC::RegisterSignalHandlerC(alljoyn_messagereceiver_signalhandler_ptr signalHandler, const alljoyn_interfacedescription_member member, const char* srcPath)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    QStatus ret = ER_OK;
    const ajn::InterfaceDescription::Member* cpp_member = (const ajn::InterfaceDescription::Member*)(member.internal_member);

    signalHandlerMapLock.Lock(MUTEX_CONTEXT);

    SignalHandlerC* cppHandler;
    SignalHandlerMap::iterator iter = signalHandlerMap.find(signalHandler);

    if (iter == signalHandlerMap.end()) {
        cppHandler = new SignalHandlerC(this, signalHandler);
        signalHandlerMap.insert(std::make_pair(signalHandler, cppHandler));
    } else {
        cppHandler = iter->second;
    }

    cppHandler->AddSubscription(cpp_member, srcPath);
    ret = RegisterSignalHandler(cppHandler,
                                static_cast<ajn::MessageReceiver::SignalHandler>(&SignalHandlerC::SignalHandlerRemap),
                                cpp_member,
                                srcPath);

    signalHandlerMapLock.Unlock(MUTEX_CONTEXT);
    return ret;
}

QStatus BusAttachmentC::RegisterSignalHandlerWithRuleC(alljoyn_messagereceiver_signalhandler_ptr signalHandler, const alljoyn_interfacedescription_member member, const char* matchRule)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    QStatus ret = ER_OK;
    const ajn::InterfaceDescription::Member* cpp_member = (const ajn::InterfaceDescription::Member*)(member.internal_member);

    /* validate and canonicalize match rule */
    Rule rule(matchRule, &ret);
    if (ER_OK != ret) {
        return ret;
    }
    qcc::String canonicalStr = rule.ToString();

    signalHandlerMapLock.Lock(MUTEX_CONTEXT);

    SignalHandlerC* cppHandler;
    SignalHandlerMap::iterator iter = signalHandlerMap.find(signalHandler);

    if (iter == signalHandlerMap.end()) {
        cppHandler = new SignalHandlerC(this, signalHandler);
        signalHandlerMap.insert(std::make_pair(signalHandler, cppHandler));
    } else {
        cppHandler = iter->second;
    }

    cppHandler->AddSubscription(cpp_member, canonicalStr.c_str());
    ret = RegisterSignalHandlerWithRule(cppHandler,
                                        static_cast<ajn::MessageReceiver::SignalHandler>(&SignalHandlerC::SignalHandlerRemap),
                                        cpp_member,
                                        canonicalStr.c_str());

    signalHandlerMapLock.Unlock(MUTEX_CONTEXT);
    return ret;
}

QStatus BusAttachmentC::UnregisterSignalHandlerC(alljoyn_messagereceiver_signalhandler_ptr signalHandler, const alljoyn_interfacedescription_member member, const char* srcPath)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    QStatus ret = ER_FAIL;
    const ajn::InterfaceDescription::Member* cpp_member = (const ajn::InterfaceDescription::Member*)(member.internal_member);

    signalHandlerMapLock.Lock(MUTEX_CONTEXT);
    SignalHandlerMap::iterator iter = signalHandlerMap.find(signalHandler);
    if (iter != signalHandlerMap.end()) {
        ret = UnregisterSignalHandler(iter->second,
                                      static_cast<ajn::MessageReceiver::SignalHandler>(&SignalHandlerC::SignalHandlerRemap),
                                      cpp_member,
                                      srcPath);
        if (ret == ER_OK) {
            bool removeHandler = iter->second->RemoveSubscription(cpp_member, srcPath);
            if (removeHandler) {
                delete iter->second;
                signalHandlerMap.erase(iter);
            }
        }
    }
    signalHandlerMapLock.Unlock(MUTEX_CONTEXT);

    return ret;
}

QStatus BusAttachmentC::UnregisterSignalHandlerWithRuleC(alljoyn_messagereceiver_signalhandler_ptr signalHandler, const alljoyn_interfacedescription_member member, const char* matchRule)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
    QStatus ret = ER_FAIL;
    const ajn::InterfaceDescription::Member* cpp_member = (const ajn::InterfaceDescription::Member*)(member.internal_member);

    /* validate and canonicalize match rule */
    Rule rule(matchRule, &ret);
    if (ER_OK != ret) {
        return ret;
    }
    qcc::String canonicalStr = rule.ToString();

    ret = ER_FAIL; /* return FAIL if we can't find the handler to remove */
    signalHandlerMapLock.Lock(MUTEX_CONTEXT);
    SignalHandlerMap::iterator iter = signalHandlerMap.find(signalHandler);
    if (iter != signalHandlerMap.end()) {
        ret = UnregisterSignalHandlerWithRule(iter->second,
                                              static_cast<ajn::MessageReceiver::SignalHandler>(&SignalHandlerC::SignalHandlerRemap),
                                              cpp_member,
                                              canonicalStr.c_str());
        if (ret == ER_OK) {
            bool removeHandler = iter->second->RemoveSubscription(cpp_member, canonicalStr.c_str());
            if (removeHandler) {
                delete iter->second;
                signalHandlerMap.erase(iter);
            }
        }
    }
    signalHandlerMapLock.Unlock(MUTEX_CONTEXT);

    return ret;
}

QStatus BusAttachmentC::UnregisterAllHandlersC() {
    QCC_DbgTrace(("%s", __FUNCTION__));
    QStatus ret = ER_OK;

    signalHandlerMapLock.Lock(MUTEX_CONTEXT);
    SignalHandlerMap::iterator iter;
    for (iter = signalHandlerMap.begin(); iter != signalHandlerMap.end(); ++iter) {
        QStatus tret = UnregisterAllHandlers(iter->second);
        delete iter->second;
        if (ret == ER_OK) {
            ret = tret;
        }
    }
    signalHandlerMap.clear();
    signalHandlerMapLock.Unlock(MUTEX_CONTEXT);

    return ret;
}

}
