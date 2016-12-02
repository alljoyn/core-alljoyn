/**
 * @file
 *
 * This file implements the BusAttachmentC class.
 */

/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/
#ifndef _ALLJOYN_C_MSGARGC_H
#define _ALLJOYN_C_MSGARGC_H

#include <alljoyn/Message.h>
#include <alljoyn/MsgArg.h>
#include <MsgArgUtils.h>
#include <alljoyn_c/Status.h>
namespace ajn {
/**
 * MsgArgC is used to map 'C' code to the 'C++' MsgArg code where additional handling
 * is needed to convert from a 'C' type to a 'C++' type.
 */
class MsgArgC : public MsgArg {
    friend class MsgArgUtils;
  public:
    /**
     * constructor
     */
    MsgArgC() : MsgArg() { }

    /**
     * copy constructor
     */
    MsgArgC(const MsgArgC& other) : MsgArg(static_cast<MsgArg>(other)) { }

    /**
     * Constructor
     *
     * @param typeId  The type for the MsgArg
     */
    MsgArgC(AllJoynTypeId typeId) : MsgArg(typeId) { }

    static QStatus MsgArgUtilsSetVC(MsgArg* args, size_t& numArgs, const char* signature, va_list* argp);

    static QStatus VBuildArgsC(const char*& signature, size_t sigLen, MsgArg* arg, size_t maxArgs, va_list* argp, size_t* count = NULL);
    static QStatus VParseArgsC(const char*& signature, size_t sigLen, const MsgArg* argList, size_t numArgs, va_list* argp);
};
}
#endif