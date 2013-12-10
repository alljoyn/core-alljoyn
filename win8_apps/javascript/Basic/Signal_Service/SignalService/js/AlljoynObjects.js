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

var APPLICATION_NAME = "signalService";

//  well-known name prefix
var WELL_KNOWN_NAME = "org.alljoyn.Bus.signal_sample";

//  Interface name for the signal service sample
var BASIC_SERVICE_INTERFACE_NAME = "org.alljoyn.Bus.signal_sample";

//  Relative path for the service 
var OBJECT_PATH = "/";

//  Session Port Num for client-service communication
var SESSION_PORT = 25;

//  Basic Service Object that will advertise the well-known interface and provide the 
//  interface to the client app
function SignalServiceBusObject(busAtt, busObjectPath) {

    this.busObject = new AllJoyn.BusObject(busAtt, busObjectPath, false);

    /* Add the interface to this object */
    var iface = null;
    var ifaceArray = new Array(iface);
    busAtt.createInterface(BASIC_SERVICE_INTERFACE_NAME, ifaceArray, false);
    ifaceArray[0].addSignal("nameChanged", "s", "newName", 0, "");
    ifaceArray[0].addProperty("name", "s", AllJoyn.PropAccessType.prop_ACCESS_RW);
    ifaceArray[0].activate();
    this.busObject.addInterface(ifaceArray[0]);
    this.serviceInterface = ifaceArray[0];

    /* This is the property value which the client will query to change */
    this.name = "";
    
    /* Called when the client calls the set property method with a string as the new
        value. Changes the property value and emits a 'nameChanged' signal */
    this.setPropertyHandler = function setPropertyHandler(event) {
        var itfName = GetEventArg(event, 0);
        var propertyName = GetEventArg(event, 1);
        var newValue = GetEventArg(event, 2);

        if (propertyName == 'name') {
            var itfName = GetEventArg(event, 0);
            var propertyName = GetEventArg(event, 1);
            var newValue = GetEventArg(event, 2).value;

            OutputLine("The client has queried to change the '" + propertyName + "' property (newName='" + newValue + "'\toldName='"+busObject.name+"')");
            busObject.name = newValue;

            try {
                /* Emit 'nameChanged' signal to anyone subscribed */
                var messageArg = new AllJoyn.MsgArg("s", new Array(newValue));
                busObject.busObject.signal("", 0, busObject.serviceInterface.getMember('nameChanged'), new Array(messageArg), 0, AllJoyn.AllJoynFlagType.alljoyn_FLAG_GLOBAL_BROADCAST);
                OutputLine("The 'nameChanged' signal was sent out as a global broadcast.");
            } catch (err) {
                OutputLine("Error occured when sending 'nameChanged' signal.");
                OutputLine(err);
            }
        }
    }
    // add and register the event and event listener from the 'set' method call
    this.busObject.addEventListener('set', this.setPropertyHandler);
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
        var previousOwner = GetEventArg(event, 1);
        var newOwner = GetEventArg(event, 2);
    }
    // Called by the bus when the ownership of any well-known name changes.
    this.BusListenerNameOwnerChanged = function (event) {
        var name = GetEventArg(event, 0);
        var transport = GetEventArg(event, 1);
        var namePrefix = GetEventArg(event, 2);
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
        if (sessionPort == SESSION_PORT) {
            return true;
        } else {
            return false;
        }
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