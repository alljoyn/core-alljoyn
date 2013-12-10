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
var busAtt = null, busListener = null, busObject = null, sessionId = 0, running = false;

// object which controls the construction of the alljoyn service
var alljoyn = (function () {
    var that = new Object();

    // Sets up the alljoyn bus, objects and listeners
    that.start = function start() {
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

            busListener = new MyBusListener(busAtt);
            busAtt.registerBusListener(busListener.busListener);
            OutputLine("Bus Listener was created and registered.");

            busObject = new SignalServiceBusObject(busAtt, OBJECT_PATH, false);
            OutputLine("Bus Object created.");

            busAtt.start();

            /* Connect bus attachment to D-Bus */
            busAtt.connectAsync(CONNECT_SPECS).then(function () {
                OutputLine("Successfully connected to AllJoyn.");
                that.advertiseName();
            });
        } catch (err) {
            OutputLine("Setup of the service application was unsuccessful.");
            OutputLine("Error: " + err.message.toString());
            busAtt = null;
            busListener = null;
            busObject = null;
            running = false;
        }
    }

    // Bind the session port and advertise the well known service name for clients to discover.
    that.advertiseName = function advertiseName() {
        OutputLine("Bus Attachment connected to " + CONNECT_SPECS + " successfully.");
        try {
            /* Make a SessionPort available for external BusAttachments to join. */
            var sessionOptIn = new AllJoyn.SessionOpts(AllJoyn.TrafficType.traffic_MESSAGES,
                false, AllJoyn.ProximityType.proximity_ANY, AllJoyn.TransportMaskType.transport_ANY)
            var sessionPortOut = new Array(1);

            busAtt.bindSessionPort(SESSION_PORT, sessionPortOut, sessionOptIn, busListener.sessionPortListener);
            OutputLine("Bound Session Port (Port#=" + SESSION_PORT + ").");

            /* Request a well-known name from the bus... */
            busAtt.requestName(WELL_KNOWN_NAME, AllJoyn.RequestNameType.dbus_NAME_DO_NOT_QUEUE);
            OutputLine("Obtained the well-known name: " + WELL_KNOWN_NAME + ".");

            /* Advertise the well-known name of the service so client can discover */
            busAtt.advertiseName(WELL_KNOWN_NAME, AllJoyn.TransportMaskType.transport_ANY);
            OutputLine("Currently advertising the well-known name: " + WELL_KNOWN_NAME);

        } catch (err) {
            OutputLine("Establishing the service with the alljoyn bus was unsuccessful.");
            OutputLine("Error: " + err.message.toString());
        }
    }

    return that;

}());

// Called when the start service button is click to start the application.
startButton.onclick = function () {
    if (!running && busAtt == null) {
        alljoyn.start();
    }
}

//  Called by stop button click event to tear down the bus attachment and terminate the service
stopButton.onclick = function () {
    if (running && busAtt != null) {
        try {
            busAtt.disconnectAsync(CONNECT_SPECS).then(function () {
                busAtt.stopAsync();
            }).then(function () {
                busAtt = null;
                busListener = null;
                busObject = null;
                running = false;
                OutputLine("The service application has exited.");
            });

        } catch (err) {
            OutputLine("Couldn't properly shut down the application.");
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