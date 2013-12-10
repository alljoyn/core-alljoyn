The following is a sample of a file transfer service and client that use the
 existing AllJoyn Service to transfer a file.
    +FileTransferService.cc
    +FileTransferClient.cc

FileTransferService.cc:
    Will create a service that will join the bus, request the name
    'org.alljoyn.bus.samples.fileTransfer', bind to the session port. The service accepts a
    single file name as a command line argument and sends the contents of that file to the client.

    Example output:
    FileTransferService
        AllJoyn Library version: v3.3.0
        AllJoyn Library build info: AllJoyn Library v3.3.0 (Built Mon Jul 08 21:22:32 UTC 2013 by User)
        Interface Created.
        Interface successfully added to the bus.
        BusListener Registered.
        BusAttachment started.
        Initialized winsock
        Connect to 'null:' succeeded.
        RequestName('org.alljoyn.bus.samples.fileTransfer') succeeded.
        NameOwnerChanged: name=org.alljoyn.bus.samples.fileTransfer, oldOwner=<none>, newOwner=:CFxlk4cF.2
        BindSessionPort succeeded.
        Advertisement of the service name 'org.alljoyn.bus.samples.fileTransfer' succeeded.
        Accepting join session request from :rQBxUlD3.2 (opts.proximity=ff, opts.traffic=1, opts.transports=4)
        Session joined successfully with :rQBxUlD3.2 (sessionId=-883909296)
        Sent signal with Array#: 1 and returned status: ER_OK.
        Sent end of file signal and returned status: ER_OK.

FileTransferClient.cc:
    Will connect to the bus, discover the interface 'org.alljoyn.bus.samples.fileTransfer', and
    join the established session.  The client receives the contents of a file from the service.

    Example output:
    FileTransferClient
        AllJoyn Library version: v3.3.0
        AllJoyn Library build info: AllJoyn Library v3.3.0 (Built Mon Jul 08 21:22:32 UTC 2013 by User).
        Interface Created.
        Interface successfully added to the bus.
        Registered signal handler for org.alljoyn.bus.samples.fileTransfer.FileTransfer
        Registration of busObject succeeded.
        BusAttachment started.
        Initialized winsock
        BusAttachment connected to 'null:'.
        Registration of Buslistener completed.
        org.alljoyn.Bus.FindAdvertisedName ('org.alljoyn.bus.samples.fileTransfer') succeeded.
        Waited 0 seconds for JoinSession completion.
        FoundAdvertisedName(name=org.alljoyn.bus.samples.fileTransfer, prefix=org.alljoyn.bus.samples.fileTransfer)
        NameOwnerChanged: name=org.alljoyn.bus.samples.fileTransfer, oldOwner=<none>, newOwner=:CFxlk4cF.2
        JoinSession SUCCESS (Session id=-883909296)
        Opening the output stream to transfer the file 'README.txt'.
        Array Num : 1   Size : 757
        The file was transfered sucessfully.
        File Transfer Client exiting with status 0 (ER_OK)