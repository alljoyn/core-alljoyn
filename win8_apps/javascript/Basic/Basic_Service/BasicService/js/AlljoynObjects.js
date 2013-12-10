/*
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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
 */

//  Connection specifications for the bus attachment
var CONNECT_SPECS = "null:";

var APPLICATION_NAME = "basicService";

//  well-known name prefix
var WELL_KNOWN_NAME = "org.alljoyn.Bus.sample";

//  Interface name for the 'cat' method
var BASIC_SERVICE_INTERFACE_NAME = "org.alljoyn.Bus.sample";

//  Relative path for the service 
var OBJECT_PATH = "/sample";

//  Session Port Num for client-service communication
var SESSION_PORT = 25;

//  Basic Service Object that will advertise the well-known interface and provide the 
//  interface to the client app
function BasicServiceBusObject(busAtt, busObjectPath) {

    this.busObject = new AllJoyn.BusObject(busAtt, busObjectPath, false);

    /* Add the interface to this object */
    var iface = null;
    var ifaceArray = new Array(iface);
    busAtt.createInterface(BASIC_SERVICE_INTERFACE_NAME, ifaceArray, false);
    ifaceArray[0].addMethod("cat", "ss", "s", "inStr1,inStr2,outStr", 0, "");
    ifaceArray[0].activate();
    this.serviceInterface = ifaceArray[0];

    this.busObject.addInterface(this.serviceInterface);
    var catMember = this.serviceInterface.getMember("cat");
    this.catReceiver = new AllJoyn.MessageReceiver(busAtt);

    /* Method handler for the 'cat' method. Takes two string arguments and returns the 
        concatenation of the two to the method caller */
    this.CatMethodHandler = function CatMethodHandler(event) {
        try {
            var ifaceMember = GetEventArg(event, 0);
            var message = GetEventArg(event, 1);
            var arg1 = message.getArg(0).value;
            var arg2 = message.getArg(1).value;
            var sender = message.sender;
            var ret = arg1 + arg2;
            var retMessageArg = new AllJoyn.MsgArg("s", new Array(ret));

            OutputLine("//////////////////////////////////////////////////////////////////");
            OutputLine("The 'cat' method was called with the arguments '" + arg1 + "' and '" +
                arg2 + "' from sender '" + sender + "'");
            
            busObject.busObject.methodReply(message, new Array(retMessageArg));

            OutputLine("The value '" + ret + "' was return.\n");
        } catch (error) {
            OutputLine("Method Reply was unsuccessful, errors were produced.");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////
    this.catReceiver.addEventListener('methodhandler', this.CatMethodHandler);
    this.busObject.addMethodHandler(catMember, this.catReceiver);
    ///////////////////////////////////////////////////////////////////////////////////

    busAtt.registerBusObject(this.busObject);
}

//  This bus listener will handle bus and session events
function MyBusListener(busAtt) {
    this.sessionListener = new AllJoyn.SessionListener(busAtt);
    this.sessionPortListener = new AllJoyn.SessionPortListener(busAtt);
    this.busListener = new AllJoyn.BusListener(busAtt);

    // Called when a BusAttachment this listener is registered with is has become disconnected from the bus.
    this.BusListenerBusDisconnected = function () {
    }
    // Called when a BusAttachment this listener is registered with is stopping.
    this.BusListenerBusStopping = function () {
    }
    // Called by the bus when the listener is registered.
    this.BusListenerListenerRegistered = function (event) {
    }
    // Called by the bus when the listener is unregistered
    this.BusListenerListenerUnregistered = function () {
    }
    // Called by the bus when an external bus is discovered that is advertising a well-known name
    // that this attachment has registered interest in via a DBus call to org.alljoyn.Bus.FindAdvertisedName
    this.BusListenerFoundAdvertisedName = function (event) {
        var name = GetEventArg(event, 0);
        var transport = GetEventArg(event, 1);
        var namePrefix = GetEventArg(event, 2);
    }
    // Called by the bus when an advertisement previously reported through FoundName has become unavailable.
    this.BusListenerLostAdvertisedName = function (event) {
        var name = GetEventArg(event, 0);
        var transport = GetEventArg(event, 1);
        var namePrefix = GetEventArg(event, 2);
    }
    // Called by the bus when the ownership of any well-known name changes.
    this.BusListenerNameOwnerChanged = function (event) {
        var name = GetEventArg(event, 0);
        var previousOwner = GetEventArg(event, 1);
        var newOwner = GetEventArg(event, 2);
    };

    // Called when the session has been successfully joined.
    this.SessionPortListenerSessionJoined = function (event) {
        var sessionPort = GetEventArg(event, 0);
        if (sessionId == 0) {
            sessionId = GetEventArg(event, 1);
        }
        var joiner = GetEventArg(event, 2);
        OutputLine("A session has been established with: " + joiner + " (sessionId=" + sessionId + ").");
    }
    // Called when the a JoinSession request has been made.
    this.SessionPortListenerAcceptSessionJoiner = function (event) {
        var sessionPort = GetEventArg(event, 0);
        var sessionId = GetEventArg(event, 1);
        var joiner = GetEventArg(event, 2);
        return (sessionPort == SESSION_PORT);
    }

    // Called by the bus when an existing session becomes disconnected.
    this.SessionListenerSessionLost = function (event) {
        var sessionId = GetEventArg(event, 0);
    };
    // Called by the bus when a member of a multipoint session is added.
    this.SessionListenerSessionMemberAdded = function (event) {
        var sessionId = GetEventArg(event, 0);
        var uniqueName = GetEventArg(event, 1);
    };
    // Called by the bus when a member of a multipoint session is removed.
    this.SessionListenerSessionMemberRemoved = function (event) {
        var sessionId = GetEventArg(event, 0);
        var uniqueName = GetEventArg(event, 1);
    };

    this.busListener.addEventListener('busdisconnected', this.BusListenerBusDisconnected);
    this.busListener.addEventListener('busstopping', this.BusListenerBusStopping);
    this.busListener.addEventListener('listenerregistered', this.BusListenerListenerRegistered);
    this.busListener.addEventListener('listenerunregistered', this.BusListenerListenerUnregistered);
    this.busListener.addEventListener('foundadvertisedname', this.BusListenerFoundAdvertisedName);
    this.busListener.addEventListener('lostadvertisedname', this.BusListenerLostAdvertisedName);
    this.busListener.addEventListener('nameownerchanged', this.BusListenerNameOwnerChanged);

    this.sessionPortListener.addEventListener('acceptsessionjoiner', this.SessionPortListenerAcceptSessionJoiner);
    this.sessionPortListener.addEventListener("sessionjoined", this.SessionPortListenerSessionJoined);

    this.sessionListener.addEventListener('sessionlost', this.SessionListenerSessionLost);
    this.sessionListener.addEventListener('sessionmemberadded', this.SessionListenerSessionMemberAdded);
    this.sessionListener.addEventListener('sessionmemberremoved', this.SessionListenerSessionMemberRemoved);
}