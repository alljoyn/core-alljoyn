The following are samples of a basic service and client that will use the
already existing AllJoyn Service to transfer messages.
    +basic_service.cc
    +basic_client.cc

basic_service.cc:
    Will create a service that will join the bus and request the name
    'org.alljoyn.Bus.sample' it will create a single method called 'cat'
    that will concatenate two strings together and return the resulting
    string to the client.

    Example output:
    basic_service
        AllJoyn Library version: v3.3.0.
        AllJoyn Library build info: AllJoyn Library v3.3.0 (Built Mon Jul 08 21:22:32 UTC 2013 by User).
        Interface created.
        BusAttachment started.
        RegisterBusObject succeeded.
        Initialized winsock
        ObjectRegistered has been called.
        Connect to 'null:' succeeded.
        RequestName('org.alljoyn.Bus.sample') succeeded.
        NameOwnerChanged: name=org.alljoyn.Bus.sample, oldOwner=<none>, newOwner=:kr2UKQ9n.2.
        BindSessionPort succeeded.
        Advertisement of the service name 'org.alljoyn.Bus.sample' succeeded.
        Accepting join session request from :APzsgCvo.2 (opts.proximity=ff, opts.traffic=1, opts.transports=4).

basic_client.cc:
    Will connect to the bus discover the interface of 'org.alljoyn.Bus.sample'
    then it will make a call to the 'cat' method.

    Example output:
    basic_client
        AllJoyn Library version: v3.3.0.
        AllJoyn Library build info: AllJoyn Library v3.3.0 (Built Mon Jul 08 21:22:32 UTC 2013 by User).
        Interface 'org.alljoyn.Bus.sample' created.
        BusAttachment started.
        Initialized winsock
        BusAttachment connected to 'null:'.
        BusListener Registered.
        org.alljoyn.Bus.FindAdvertisedName ('org.alljoyn.Bus.sample') succeeded.
        Waited 0 seconds for JoinSession completion.
        FoundAdvertisedName(name='org.alljoyn.Bus.sample', prefix='org.alljoyn.Bus.sample')
        NameOwnerChanged: name='org.alljoyn.Bus.sample', oldOwner='<none>', newOwner=':kr2UKQ9n.2'.
        JoinSession SUCCESS (Session id=736495887).
        'org.alljoyn.Bus.sample.cat' (path='/sample') returned 'Hello World!'.
        Basic client exiting with status 0x0000 (ER_OK).


The following show an example of how to emit and subscribe to an AllJoyn signal.
    +signal_service.cc
    +signalConsumer_client.cc
    +nameChange_client.cc
    
signal_service.cc:
    Will create a service that will join the bus and request the name
    'org.alljoyn.signal_sample'. This service will have a property 'name' the
    name can be changed using Get and Set methods. When the value for 'name'
    is changed a 'nameChanged' signal will be sent with the new name assigned
    to the 'name' property.
    
signalConsumer_client.cc:
    Will connect to the bus and subscribe to the 'nameChanged' signal. When the
    name has been changed it will print out a message indicating it has received
    the signal and indicate the new name.
     
nameChange_client.cc:
    Will accept a text string as one of its command line arguments. The string
    will be sent to the 'org.alljoyn.signal_sample' object and will be used as
    the new 'name' property using the 'set' method. 
    

    Example output (three different command prompts):
    signal_service
        AllJoyn Library version: v3.3.0.
        AllJoyn Library build info: AllJoyn Library v3.3.0 (Built Mon Jul 08 21:22:32 UTC 2013 by User).
        BusAttachment started.
        Registering the bus object.
        Initialized winsock
        Connected to 'null:'.
        RequestName('org.alljoyn.Bus.signal_sample') succeeded.
        NameOwnerChanged: name=org.alljoyn.Bus.signal_sample, oldOwner=<none>, newOwner=:d3Ydq-gI.2
        BindSessionPort succeeded.
        Advertisement of the service name 'org.alljoyn.Bus.signal_sample' succeeded.
        Accepting join session request from :kfmTanbu.2 (opts.proximity=ff, opts.traffic=1, opts.transports=4)
        Accepting join session request from :cuBOXz15.2 (opts.proximity=ff, opts.traffic=1, opts.transports=4)
        Set 'name' property was called changing name to Hello World
        Emiting Name Changed Signal.
        Sending NameChanged signal without a session id
        Accepting join session request from :X7yTi8Hv.2 (opts.proximity=ff, opts.traffic=1, opts.transports=4)
        Set 'name' property was called changing name to foo
        Emiting Name Changed Signal.
        Sending NameChanged signal without a session id

    signalConsumer_client
        AllJoyn Library version: v3.3.0.
        AllJoyn Library build info: AllJoyn Library v3.3.0 (Built Mon Jul 08 21:22:32 UTC 2013 by User).
        BusAttachment started.
        Interface created successfully.
        Interface successfully added to the bus.
        Registered signal handler for org.alljoyn.Bus.signal_sample.nameChanged.
        Registering the bus object.
        Initialized winsock
        Connected to 'null:'.
        BusListener registered.
        org.alljoyn.Bus.FindAdvertisedName ('org.alljoyn.Bus.signal_sample') succeeded.
        Waited 0 seconds for JoinSession completion.
        FoundAdvertisedName(name='org.alljoyn.Bus.signal_sample', prefix='org.alljoyn.Bus.signal_sample')
        NameOwnerChanged: name='org.alljoyn.Bus.signal_sample', oldOwner='<none>', newOwner=':d3Ydq-gI.2'.
        JoinSession SUCCESS (Session id=1054893573).
        Successfully subscribed to the name changed signal.
        --==## signalConsumer: Name Changed signal Received ##==--
                New name: 'Hello World'.
        --==## signalConsumer: Name Changed signal Received ##==--
                New name: 'foo'.

    nameChange_client "Hello World"
        AllJoyn Library version: v3.3.0.
        AllJoyn Library build info: AllJoyn Library v3.3.0 (Built Mon Jul 08 21:22:32 UTC 2013 by User).
        BusAttachment started.
        Initialized winsock
        BusAttachment connected to 'null:'.
        BusListener registered.
        org.alljoyn.Bus.FindAdvertisedName ('org.alljoyn.Bus.signal_sample') succeeded.
        Waited 0 seconds for JoinSession completion.
        FoundAdvertisedName(name='org.alljoyn.Bus.signal_sample', prefix='org.alljoyn.Bus.signal_sample')
        NameOwnerChanged: name='org.alljoyn.Bus.signal_sample', oldOwner='<none>', newOwner=':d3Ydq-gI.2'.
        JoinSession SUCCESS (Session id=665747451).
        SetProperty to change the 'name' property to 'Hello World' was successful.
        Name change client exiting with status 0x0000 (ER_OK).

    nameChange_client.exe foo
        AllJoyn Library version: v3.3.0.
        AllJoyn Library build info: AllJoyn Library v3.3.0 (Built Mon Jul 08 21:22:32 UTC 2013 by User).
        BusAttachment started.
        Initialized winsock
        BusAttachment connected to 'null:'.
        BusListener registered.
        org.alljoyn.Bus.FindAdvertisedName ('org.alljoyn.Bus.signal_sample') succeeded.
        Waited 0 seconds for JoinSession completion.
        FoundAdvertisedName(name='org.alljoyn.Bus.signal_sample', prefix='org.alljoyn.Bus.signal_sample')
        NameOwnerChanged: name='org.alljoyn.Bus.signal_sample', oldOwner='<none>', newOwner=':d3Ydq-gI.2'.
        JoinSession SUCCESS (Session id=90361483).
        SetProperty to change the 'name' property to 'foo' was successful.
        Name change client exiting with status 0x0000 (ER_OK).