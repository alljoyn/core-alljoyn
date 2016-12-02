/**
 * @file
 * MessageReceiver is a base class implemented by any class
 * which wishes to receive AllJoyn messages
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
#ifndef _ALLJOYN_C_MESSAGERECEIVER_H
#define _ALLJOYN_C_MESSAGERECEIVER_H

#include <alljoyn_c/AjAPI.h>
#include <alljoyn_c/Message.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ALLJOYN_OPAQUE_BUSOBJECT_
#define _ALLJOYN_OPAQUE_BUSOBJECT_
/**
 * Message bus object
 */
typedef struct _alljoyn_busobject_handle*                   alljoyn_busobject;
#endif

/**
 * MethodHandlers are %MessageReceiver functions which are called by AllJoyn library
 * to forward AllJoyn method_calls to AllJoyn library users.
 *
 * @param bus       The bus object triggering this callback.
 * @param member    Method interface member entry.
 * @param message   The received method call message.
 */
typedef void (AJ_CALL * alljoyn_messagereceiver_methodhandler_ptr)(alljoyn_busobject bus,
                                                                   const alljoyn_interfacedescription_member* member,
                                                                   alljoyn_message message);

/**
 * ReplyHandlers are %MessageReceiver functions which are called by AllJoyn library
 * to forward AllJoyn method_reply and error responses to AllJoyn library users.
 *
 * @param message   The received message.
 * @param context   User-defined context passed to MethodCall and returned upon reply.
 */
typedef void (AJ_CALL * alljoyn_messagereceiver_replyhandler_ptr)(alljoyn_message message, void* context);

/**
 * SignalHandlers are %MessageReceiver functions which are called by AllJoyn library
 * to forward AllJoyn received signals to AllJoyn library users.
 *
 * @param member    Method or signal interface member entry.
 * @param srcPath   Object path of signal emitter.
 * @param message   The received message.
 */
typedef void (AJ_CALL * alljoyn_messagereceiver_signalhandler_ptr)(const alljoyn_interfacedescription_member* member,
                                                                   const char* srcPath, alljoyn_message message);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif