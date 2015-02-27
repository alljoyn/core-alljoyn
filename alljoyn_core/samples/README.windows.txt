AllJoyn C++ Samples for Windows README
--------------------------------------

This directory contains the following AllJoyn C++ API samples using a simple
command line UI:

basic -
         Contains a set of samples that show how to advertise, discover, connect
         and communicate over AllJoyn using the C++ AllJoyn API.

chat -   
         This sample shows how to use AllJoyn's C++ API to discover and connect
         with other AllJoyn enabled devices implementing a specific kind of
         peer service.  In particular this sample demonstrates the use of Signal
         as the basis for the peer-to-peer communiation.

secure -
         This sample shows how to use AllJoyn's C++ API to set up encryption for
         the remote connection.

TrustedTLSampleRN -
         This sample provides a bundled AllJoyn Routing Node, primaily to
         demonstrate how provide credentials that enable thin library 
         applications to form a trusted connection.

         Additionally, the sample can be passed a name via the command-line to
         be quietly advertised.  This demonstrates how a thin library
         applications may be "bound" to a specific Routing Node bu using the
         initial Thin Library Routing Node discovery process.
