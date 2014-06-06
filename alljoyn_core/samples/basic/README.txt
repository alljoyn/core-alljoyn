As the samples are built with the builtin router, you do not need to have the daemon running.

The following are samples of a basic service and client.
    +basic_service.cc
    +basic_client.cc

basic_service.cc:
	will create a service that will join the bus and request the name 'com.sample.test'
	it will create a single method called 'cat' that will concatenate two strings together and return the resulting string to the client.

basic_client.cc:
	will connect to the bus discover the interface of 'com.sample.test' then it will make a call to the 'cat' method.

    Example output
    $ ./basic_service &
    AllJoyn Library version: v1.2.1
    AllJoyn Library build info: AllJoyn Library v1.2.1 (Built Mon Apr 19 18:44:15 UTC 2010 on ubuntu ...
    $ ./basic_client
    AllJoyn Library version: v1.2.1
    AllJoyn Library build info: AllJoyn Library v1.2.1 (Built Mon Apr 19 18:44:15 UTC 2010 on ubuntu ...
    org.alljoyn.method_sample.cat ( path=/org/alljoyn) returned "Hello World!"
    basic client exiting with status 0 (ER_OK)    



The following show an example of how to emit and subscribe to an AllJoyn signal.
    +signal_service.cc
    +signalConsumer_client.cc
    +nameChange_client.cc
    
signal_service.cc:
    will create a service that will join the bus and request the name 'org.alljoyn.signal_sample'.
    this service will have a property 'name' the name can be changed using Get and Set methods.
    when the value for 'name' is changed a 'nameChanged' signal will be sent with the new name assigned to the 'name' property.
    
signalConsumer_client.cc:
     will connect to the bus and subscribe to the 'nameChanged' signal.  When the name has been changed it will print out a message indicating it has received the signal and indicate the new name.
     
nameChange_client.cc:
    will accept a text string as one of its command line arguments.
    The string will be sent to the 'org.alljoyn.signal_sample' object and will be used as the new 'name' property using the 'set' method. 
    

        Example output:
        $ ./signal_service &
        AllJoyn Library version: v1.2.1
        $ ./signalConsumer_client &
        AllJoyn Library version: v1.2.1
        $ ./nameChange_client "Hello World"
        AllJoyn Library version: v1.2.1
        --==## signalConsumer: Name Changed signal Received ##==--
            New name: Hello World
        name Change client exiting with status 0 (ER_OK)
        $ ./nameChange_client foo
        AllJoyn Library version: v1.2.1
        --==## signalConsumer: Name Changed signal Received ##==--
            New name: foo
        name Change client exiting with status 0 (ER_OK)
        $ ./nameChange_client bar
        AllJoyn Library version: v1.2.1
        --==## signalConsumer: Name Changed signal Received ##==--
            New name: bar
        name Change client exiting with status 0 (ER_OK)


        
