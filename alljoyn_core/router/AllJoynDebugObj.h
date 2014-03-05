/**
 * @file
 * BusObject responsible for implementing the AllJoyn methods (org.alljoyn.Debug)
 * for messages controlling debug output.
 */

/******************************************************************************
 * Copyright (c) 2011,2012,2014 AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_ALLJOYNDEBUGOBJ_H
#define _ALLJOYN_ALLJOYNDEBUGOBJ_H

// Include contents in debug builds only.
#ifndef NDEBUG

#include <qcc/platform.h>

#include <assert.h>

#include <map>

#include <qcc/String.h>
#include <qcc/StringMapKey.h>

#include <alljoyn/AllJoynStd.h>
#include <alljoyn/BusObject.h>

#include "Bus.h"

namespace ajn {

class BusController;

namespace debug {

class AllJoynDebugObjAddon {
  public:
    virtual ~AllJoynDebugObjAddon() { }

    typedef QStatus (AllJoynDebugObjAddon::* MethodHandler)(Message& message, std::vector<MsgArg>& replyArgs);
};


/**
 * BusObject responsible for implementing the AllJoyn methods at org.alljoyn.Debug
 * for messages controlling debug output.
 */
class AllJoynDebugObj : public BusObject {

    friend class ajn::BusController;  // Only the bus controller can instantiate us.

  public:
    class Properties {
      public:
        struct Info {
            const char* name;
            const char* signature;
            uint8_t access;
        };

        Properties() { }
        virtual ~Properties() { }
        virtual QStatus Get(const char* propName, MsgArg& val) const { return ER_BUS_NO_SUCH_PROPERTY; }
        virtual QStatus Set(const char* propName, MsgArg& val) { return ER_BUS_NO_SUCH_PROPERTY; }
        virtual void GetProperyInfo(const Info*& info, size_t& infoSize) = 0;
    };

    struct MethodInfo {
        const char* name;
        const char* inputSig;
        const char* outSig;
        const char* argNames;
        AllJoynDebugObjAddon::MethodHandler handler;
    };

  private:
    typedef std::map<qcc::StringMapKey, Properties*> PropertyStore;
    typedef std::pair<AllJoynDebugObjAddon*, AllJoynDebugObjAddon::MethodHandler> AddonMethodHandler;
    typedef std::map<const InterfaceDescription::Member*, AddonMethodHandler> AddonMethodHandlerMap;


  public:

    /*
     * This object is a quasi-singleton.  It gets instantiated and
     * destroyed by the BusController object.  Attempts to get a pointer
     * when there is no BusController is invalid.
     */
    static AllJoynDebugObj* GetAllJoynDebugObj();

    /**
     * Initialize and register this AllJoynDebugObj instance.
     *
     * @return ER_OK if successful.
     */
    QStatus Init();

    /**
     * Stop AlljoynDebugObj.
     *
     * @return ER_OK if successful.
     */
    QStatus Stop() { return ER_OK; }

    /**
     * Join AlljoynDebugObj.
     *
     * @return ER_OK if successful.
     */
    QStatus Join() { return ER_OK; }

    QStatus AddDebugInterface(AllJoynDebugObjAddon* addon,
                              const char* ifaceName,
                              const MethodInfo* methodInfo,
                              size_t methodInfoSize,
                              Properties& ifaceProperties);


    QStatus Get(const char* ifcName, const char* propName, MsgArg& val);


    QStatus Set(const char* ifcName, const char* propName, MsgArg& val);

    void GetProp(const InterfaceDescription::Member* member, Message& msg);

  private:

    /**
     * Constructor
     */
    AllJoynDebugObj(BusController* busController);

    /**
     * Destructor
     */
    ~AllJoynDebugObj();

    /**
     * Need to let the bus contoller know when the registration is complete
     */
    void ObjectRegistered();

    /**
     * Handles the SetDebugLevel method call.
     *
     * @param member    Member
     * @param msg       The incoming message
     */
    void SetDebugLevel(const InterfaceDescription::Member* member, Message& msg);

    void GenericMethodHandler(const InterfaceDescription::Member* member, Message& msg);

    BusController* busController;

    PropertyStore properties;

    AddonMethodHandlerMap methodHandlerMap;

    static AllJoynDebugObj* self;
};

} // namespace debug
} // namespace ajn

#endif
#endif
