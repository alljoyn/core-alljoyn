As the samples are built with the builtin router, you do not need to have the daemon running.

The following are samples of a basic mqtt service and client.
    +mqtt_service.cc
    +mqtt_client.cc

mqtt_service.cc:
	will create a service that will join the bus and request the name 'com.sample.test'
	it will create a single method called 'cat' that will concatenate two strings together and return the resulting string to the client.

mqtt_client.cc:
	will connect to the bus discover the interface of 'com.sample.test' then it will make a call to the 'cat' method.

    Example output
    $ ./mqtt_service &
    AllJoyn Library version: v1.2.1
    AllJoyn Library build info: AllJoyn Library v1.2.1 (Built Mon Apr 19 18:44:15 UTC 2010 on ubuntu ...
    $ ./mqtt_client
    AllJoyn Library version: v1.2.1
    AllJoyn Library build info: AllJoyn Library v1.2.1 (Built Mon Apr 19 18:44:15 UTC 2010 on ubuntu ...
    org.alljoyn.method_sample.cat ( path=/org/alljoyn) returned "Hello World!"
    mqtt client exiting with status 0 (ER_OK)    


