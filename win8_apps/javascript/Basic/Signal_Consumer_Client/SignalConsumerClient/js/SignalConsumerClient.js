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

//  Macros for simplified javascript code
var $ = function (id) {
    return document.getElementById(id);
}
var $$ = function (tag) {
    return document.getElementsByTagName(tag);
}

//  Helper function for retrieving event arguments
function GetEventArg(evt, index) {
    if (index == 0)
        return evt.target;
    else if (index > 0)
        return evt.detail[(index - 1)];
    else
        return nullptr;
}

//  Helper function that will write a line to the textbox of the UI
function OutputLine(msg) {
    $("textBox").value += msg + "\n";
}

//  Controls in the UI
var startButton = $('startButton');
var stopButton = $('stopButton');

//  AllJoyn object references
var busAtt = null, busListener = null, sessionId = 0, running = false;

//  Provides the functions to connect to alljoyn and communicate over the bus
var alljoyn = (function () {

    var that = new Object();

    // Setup the alljoyn objects for the client app
    that.start = function () {
        try {
            running = true;

            /* Configure Alljoyn to allow debugging */
            AllJoyn.Debug.useOSLogging(true);
            AllJoyn.Debug.setDebugLevel("ALL", 1);
            AllJoyn.Debug.setDebugLevel("ALLJOYN", 7);

            //  Alljoyn should map JS weak type to Alljoyn strict data type
            AllJoyn.MsgArg.setTypeCoercionMode("weak");

            /* Create and connect all object and listeners of the service */
            busAtt = new AllJoyn.BusAttachment(APPLICATION_NAME, true, 4);
            OutputLine("BusAttachment was created.");

            /* create interface */
            var iface = null;
            var ifaceArray = new Array(iface);
            busAtt.createInterface(BASIC_SERVICE_INTERFACE_NAME, ifaceArray, false);
            ifaceArray[0].addSignal("nameChanged", "s", "newName", 0, "");
            ifaceArray[0].addProperty("name", "s", AllJoyn.PropAccessType.prop_ACCESS_RW);
            ifaceArray[0].activate();
            OutputLine("Created the Interface.");

            busListener = new MyBusListener(busAtt);
            busAtt.registerBusListener(busListener.busListener);
            OutputLine("Bus Listener was created and registered.");

            busAtt.start();

            /* Connect bus attachment to D-Bus */
            busAtt.connectAsync(CONNECT_SPECS).then(function () {
                /* Add rules which the received signal must match (subscribe to the 'nameChanged' signal)*/
                busAtt.addMatch("type='signal',interface='org.alljoyn.Bus.signal_sample',member='nameChanged'");
                busAtt.findAdvertisedName(WELL_KNOWN_NAME)
            });
        } catch (err) {
            OutputLine("Couldn't successfully establish the client application.");
            busAtt = null;
            busListener = null;
            running = false;
        }
    }

    // Request to join a session with the discovered service
    that.joinSession = function () {
        var sessionOptIn = new AllJoyn.SessionOpts(AllJoyn.TrafficType.traffic_MESSAGES,
            false, AllJoyn.ProximityType.proximity_ANY, AllJoyn.TransportMaskType.transport_ANY)
        var sessionPortOut = new Array(1);
            
        busAtt.joinSessionAsync(WELL_KNOWN_NAME, SESSION_PORT, busListener.sessionListener, sessionOptIn, sessionPortOut, null).then(function (joinResult) {
                if (AllJoyn.QStatus.er_OK == joinResult.status) {

                    OutputLine("Join Session was successful (SessionId=" + joinResult.sessionId + ")");
                    sessionId = joinResult.sessionId;
                } else {
                    var err = joinResult.status;
                    OutputLine("Join Session was unsuccessful.");
                    OutputLine("Error: " + err.toString());
                } 
            });
    }

    return that;
}());

//  Called when start button is clicked and starts the signal consumer client
startButton.onclick = function () {
    if (!running && busAtt == null) {
        alljoyn.start();
    }
}

//  Called when stop button is clicked and disconnects the bus attachment from the D-Bus
stopButton.onclick = function () {
    if (running && busAtt != null) {
        try {
            busAtt.disconnectAsync(CONNECT_SPECS).then(function () {
                busAtt.stopAsync();
            }).then(function () {
                busAtt = null;
                busListener = null;
                running = false;
                OutputLine("The client application has exited successfully.");
            });
        } catch (err) {
            OutputLine("Couldn't properly shut down the client application.");
            OutputLine("Error: " + err.message.toString());
        }
    }
}

// Hook the application suspending/resuming events to the handlers so that exclusively-held resource (eg., network port) is release and other applications are allowed to acquire the resource
Windows.UI.WebUI.WebUIApplication.addEventListener("suspending", suspendingHandler);
function suspendingHandler(eventArgs) {
    try {
        if (busAtt && busAtt.onAppSuspend != undefined) {
            busAtt.onAppSuspend();
        }
    } catch (err) {
        console.log(err.stack);
    }
}

Windows.UI.WebUI.WebUIApplication.addEventListener("resuming", resumingHandler);
function resumingHandler() {
    try {
        if (busAtt && busAtt.onAppResume != undefined) {
            busAtt.onAppResume();
        }
    } catch (err) {
        console.log(err.stack);
    }
}