
=== Brief Description ===
WinRT is the new API surface area for Metro style apps in Windows 8. WinRT APIs are available across 
multiple languages ¨C C#, Visual Basic, C++ and JavaScript ¨C enabling developers to build Metro style
apps using the language and frameworks they are most familiar with. This documentation explains how
to write a alljoyn-powered Metro app in Javascript language.

=== COM Exception based Failure Status Report ===
In the AllJoyn wrapper layer for WinRT, an error status is translated to COM Exception and thrown.

// WinRT doesn't like exceptions without the S bit being set to 1 (failure)
#define THROW_EXCEPTION(e) throw ref new Platform::COMException(0x80000000 | (int)(e))

Thus it is necessary to catch the exception and get the status result in the Metro app.

=== Asynchronous Programming ===
WinRT javascript metro application has only the UI thread and the UI of a Metro style app runs in a single-threaded apartment (STA). 
Thus it is not allowed for the UI thread to sleep/wait for bus calls to finish. Thus, if a alljoyn bus call cannot return immediately either because
it is computation-intensive or because the call waits for reply over socket, you 
should use asynchronous bus call instead.

=== Casing and naming rules ===
JavaScript is case sensitive. Therefore, you must follow these casing conventions:
1. When you reference C++ namespaces and classes, use the same casing that's used on the C++ side. 
2. When you call methods, use camel casing even if the method name is capitalized on the C++ side. 
   For example, a C++ method BindSessionPort() must be called from JavaScript as bindSessionPort().
3. To use enumerator of enum class type like AllJoynNS.TrafficType, the whole first word of the enumerator should be in lower case.
   For example: 
        AllJoynNS.TrafficType.traffic_MESSAGES

  An example to make alljoyn bus call:
    
    var sessionOptIn = new AllJoynNS.SessionOpts(AllJoynNS.TrafficType.traffic_MESSAGES, true, AllJoynNS.ProximityType.proximity_ANY, AllJoynNS.TransportMaskType.transport_ANY)
    var sessionPortOut = new Array(1);
    bus.bindSessionPort(CONTACT_PORT, sessionPortOut, sessionOptIn, busListener._spListener);

=== To use bundled Daemon ===
The Metro app can bundle the alljoyn daemon in its package. To use it the application should 
asynchronously load the daemon before connecting to it using:

  AllJoynNS.Daemon.loadAsync();

=== Asynchronously Connect to Bus ====

    var connect = function () {
        /* Connect the BusAttachment to the bus. */
        var connectSpec = "tcp:addr=127.0.0.1,port=9955";
        try {
            bus.connectAsync(connectSpec).then(function () {
                bus.findAdvertisedName('org');
            });
        } catch (err) {
            setTimeout("alljoyn.connect()", 1000);
            return;
        }
    };

=== How to Register AllJoyn events ===
There are two ways to add event listener to alljoyn bus events:

1)  var bl = AllJoynNS.BusListener(bus);
    bl.addEventListener('foundadvertisedname', BusListenerFoundAdvertisedName);

2)  var bl = AllJoynNS.BusListener(bus);
    bl.onfoundadvertisedname = BusListenerFoundAdvertisedName;

As shown in above, when registering the events, all characters in the event name should be in lower case no matter how many words it is composed of.
Note that the handler BusListenerFoundAdvertisedName should be define before you register the foundadvertisedname event, otherwise the even handler is undefined.

=== How to Retrieve Event Arguments ===

BusListenerNameOwnerChanged = function (evt) {
    var name = GetEventArg(evt, 0);
    var previousOwner = GetEventArg(evt, 1);
    var newOwner = GetEventArg(evt, 2);
};

// A helper function
function GetEventArg(evt, index) {
    if (index == 0)
        return evt.target;
    else if (index > 0)
        return evt.detail[(index - 1)];
    else
        return nullptr;
};


=== Reference ===
http://msdn.microsoft.com/en-us/library/windows/apps/hh441569(v=vs.110).aspx
http://msdn.microsoft.com/en-us/library/windows/apps/hh755833(v=vs.110).aspx
http://social.msdn.microsoft.com/Forums/en-US/winappswithhtml5/threads
http://msdn.microsoft.com/en-us/library/windows/apps/hh780559.aspx
