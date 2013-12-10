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
var CONNECT_SPECS = "tcp:addr=127.0.0.1,port=9956";

var APPLICATION_NAME = "Secure";

//  well-known service name prefix
var WELL_KNOWN_NAME = "org.alljoyn.bus.samples.secure";

//  Interface name for the 'ping' method
var SECURE_INTERFACE_NAME = "org.alljoyn.bus.samples.secure.SecureInterface";

//  Relative path for the service 
var OBJECT_PATH = "/SecureService";

//  Session Port Num for client-service communication
var SESSION_PORT = 42;

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
    $("output").value += msg + "\n";
}

// Run on startup to cofigure debugging and javascript's strict mode
var onLoad = (function () {
    $("sendBtn").disabled = "disabled";

    /* Configure Alljoyn to allow debugging */
    AllJoyn.Debug.useOSLogging(true);
    AllJoyn.Debug.setDebugLevel("ALL", 1);
    AllJoyn.Debug.setDebugLevel("ALLJOYN", 7);

    //  Alljoyn should map JS weak type to Alljoyn strict data type
    AllJoyn.MsgArg.setTypeCoercionMode("weak");
}());


//  Called on start/stop server button click, then starts a server if one isn't currently
//  running otherwise breaks down the server.
$("startServer").onclick = function () {
    if (client == null) {
        if ($("startServer").innerHTML == "Start Server" && service == null) {

            OutputLine("Establishing a server...");
            service = new Service();
            $("startServer").innerHTML = "Stop Server";

        } else if ($("startServer").innerHTML == "Stop Server" && service != null) {

            OutputLine("Tearing down the server...");
            service.tearDown();
            service = null;
            $("startServer").innerHTML = "Start Server";

        }
    }
}

// Called on start/stop client button click, then starts a client if one isn't currently
// running otherwise breaks down the client.
$("startClient").onclick = startClientClick;
function startClientClick() {
    if (service == null) {
        if ($("startClient").innerHTML == "Start Client" && client == null) {

            OutputLine("Establishing a Client...");
            client = new Client();
            $("startClient").innerHTML = "Stop Client";

        } else if ($("startClient").innerHTML == "Stop Client" && client != null) {

            OutputLine("Tearing down the client...");
            client.tearDown();
            client = null;
            $("startClient").innerHTML = "Start Client";

        }
    }
}