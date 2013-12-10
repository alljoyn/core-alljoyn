
AllJoyn Java Samples README
---------------------------

This directory contains the following set of AllJoyn usage samples:

JavaSDKDoc  - This is a large collection of samples that were written to 
              correspond with the 'Guide to AllJoyn Development Using the Java SDK'.
              This section contains several projects that can be imported into the
              Eclipse IDE.
              
              A built version of each sample can be found in the jar folder of the
              AllJoyn distribution. For example typing the following will run the
              MethodsService and the MethodsClient samples
              java -Djava.library.path=..\lib -jar JavaSDKDocMethodsService.jar
              java -Djava.library.path=..\lib -jar JavaSDKDocMethodsClient.jar
              
              The java.library.path is the path to the alljoyn_java dynamic library. 
              On Windows this is a .dll file in Linux a .so file on Mac it would 
              be a .dylib file.
