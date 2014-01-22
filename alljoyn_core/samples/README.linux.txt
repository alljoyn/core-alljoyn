AllJoyn C++ Samples for Linux README
------------------------------------

This directory contains the following AllJoyn C++ API samples:

basic -
         Contains a set of samples that show how to advertise, discover, connect
         and communicate over AllJoyn using the C++ AllJoyn API.

chat -   
         This sample shows how to use AllJoyn's C++ API to discover and connect
         with other AllJoyn enabled devices.

FileTransfer -
         This sample contains a set of samples that shows how to share a file 
         over AllJoyn using the C++ AllJoyn API. The service accepts a single
         file name as a command line argument and sends the contents of that 
         file to the client.                 


SampleDaemon -
         This sample runs a bundled AllJoyn router. It can be passed a name
         via the command-line, that is quietly advertised, to be discovered
         by thin-client applications (eg. apps running on Arduino Due)
