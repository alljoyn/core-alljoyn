/*
 * Copyright (c) 2011 - 2012, AllSeen Alliance. All rights reserved.
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

//  Basic Service Object that will advertise the well-known interface and provide the 
//  interface to the client app
function BasicServiceBusObject(busAtt, busObjectPath) {

    //busObject = new AllJoyn.BusObject(busAtt, busObjectPath, false);
    this.busObject = new AllJoyn.BusObject(busAtt, busObjectPath, false);

    /* Add the interface to this object */
    this.serviceInterface = busAtt.getInterface(BASIC_SERVICE_INTERFACE_NAME);
    this.busObject.addInterface(this.serviceInterface);
    var catMember = this.serviceInterface.getMember("cat");
    this.catReceiver = new AllJoyn.MessageReceiver(busAtt);

    //catReceiver.methodHandler += new AllJoyn.MessageReceiverMethodHandler(CatMethodHandler);


    /* Add the 'cat' method receiver/handler */
    //this.catMethodReceiver = new AllJoyn.MessageReceiver(busAtt);
    function CatMethodHandler(event) {
        try {
            var ifaceMember = GetEventArg(event, 0);
            var message = GetEventArg(event, 1);;
            var arg1 = message.getArg(0).value;
            var arg2 = message.getArg(1).value;
            var sender = message.sender;
            var ret = arg1 + arg2;
            OutputLine("//////////////////////////////////////////////////////////////////");
            OutputLine("The 'cat' method was called with the arguments '" + arg1 + "' and '" + arg2 + "' from sender '" + sender + "'");

            var array = new Array(1);
            array[0] = new Object(ret);
            //var retMessageArg = new AllJoyn.MsgArg("s", new Array("ret"));
            var retMessageArg = new AllJoyn.MsgArg("s", array);

            OutputLine("Made the messageArg");
            this.busObject.MethodReplyASync(message, new Array(retMessageArg));

            OutputLine("The value '" + ret + "' was return.\n");
        } catch (error) {
            OutputLine("Error handling the 'cat' method call. No return.");
            OutputLine(error);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////
    this.catReceiver.addEventListener('methodhandler', CatMethodHandler);
    this.busObject.addMethodHandler(catMember, this.catReceiver);
    ///////////////////////////////////////////////////////////////////////////////////

}

//  This bus listener will handle bus and session events
function MyBusListener(busAtt) {
    this.sessionListener = new AllJoyn.SessionListener(busAtt);
    this.sessionPortListener = new AllJoyn.SessionPortListener(busAtt);
    this.busListener = new AllJoyn.BusListener(busAtt);

    this.BusListenerBusDisconnected = function () {
        var i = 0;
    }

    this.BusListenerBusStopping = function () {
        var i = 0;
    }

    this.BusListenerListenerRegistered = function (event) {
    }

    this.BusListenerListenerUnregistered = function () {
        var i = 0;
    }

    this.BusListenerFoundAdvertisedName = function (event) {
        //var name = GetEventArg(event, 0);
        //var transport = GetEventArg(event, 1);
        //var namePrefix = GetEventArg(event, 2);
        //alljoyn.addChannelName(name);
        //alljoyn.onname();
    }

    this.BusListenerLostAdvertisedName = function (event) {
        //var name = GetEventArg(event, 0);
        //var previousOwner = GetEventArg(event, 1);
        //var newOwner = GetEventArg(event, 2);
        //alljoyn.removeChannelName(name);
        //alljoyn.onname();
    }

    this.BusListenerNameOwnerChanged = function (event) {
        var name = GetEventArg(event, 0);
        var previousOwner = GetEventArg(event, 1);
        var newOwner = GetEventArg(event, 2);
    };

    this.BusListenerListenerUnregistered = function () {
        var i = 0;
    }

    this.SessionPortListenerSessionJoined = function (event) {
        var sessionPort = GetEventArg(event, 0);
        if (sessionId == 0) {
            sessionId = GetEventArg(event, 1);
        }
        var joiner = GetEventArg(event, 2);
        OutputLine("A session has been established with: " + joiner + " (sessionId=" + sessionId + ").");
    }

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

    this.SessionListenerSessionLost = function (event) {
        var sessionId = GetEventArg(event, 0);
    };

    this.SessionListenerSessionMemberAdded = function (event) {
        var sessionId = GetEventArg(event, 0);
        var uniqueName = GetEventArg(event, 1);
    };

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