AllJoyn iOS and OS/X SDK README

************************************************************************************

PLEASE READ THIS DOCUMENT

AllJoyn Programming Guide for the Objective-C Language

************************************************************************************

Minimum Requirements for AllJoyn Development System

1. OS/X 10.7 (Lion) or higher

Prerequisites

1. Xcode is available for free from the Mac App Store at the following web address:
   http://itunes.apple.com/us/app/xcode/id497799835?mt=12
2. OpenSSL is required for iOS development and is available at the following web
   address: http://www.openssl.org/
   AllJoyn has been tested with version 1.0.1 of OpenSSL.
3. Download the Xcode project that can be used to build OpenSSL for iOS from GitHub,
   at the following web address: https://github.com/sqlcipher/openssl-xcode/

Installation

1. Unzip the AllJoyn SDK package to a folder on your development system.

2. Copy the OpenSSL source into a separate folder on your development system, not
   under the AllJoyn SDK.

3. Navigate to the OpenSSL source top folder in Finder, and copy the openssl.xcodeproj
   folder you downloaded from GitHub into this folder.

4. Open the openssl.xcodeproj in Xcode.

5. In Xcode, build the crypto target (libssl.a and libcrypto.a) for each
   combination of configuration (debug|release) and platform (iphoneos|iphonesimulator) that you
   need for your iOS project by selecting Product->Build For->(your desired configuration).

6. Create a new folder called "build" under the top-level OpenSSL folder created in step 1.

7. Locate your OpenSSL build products folders (i.e.: Debug-iphoneos) in the
   /Users/<your username>/Library/Developer/Xcode/DerivedData/XXXXXXXXXXXXX-openssl/Build/Products
   folder, and copy all the <configuration>-<platform> folders, like Debug-iphoneos, to the build
   folder created in step 6.

8. You should now have a folder structure similar to this containing libssl and libcrypto
   for each $(CONFIGURATION)-$(PLATFORM_NAME) you built in step 5:

      openssl-1.0.1c

            build
                  Debug-iphoneos
                        libssl.a
                        libcrypto.a

                  Debug-iphonesimulator
                        libssl.a
                        libcrypto.a

9. Define an environment variable OPENSSL_ROOT=<path to the OpenSSL source top folder>
   This environment variable needs to be present whenever you build projects using the
   AllJoyn SDK.

    9a. For Mac OS X 10.7 to 10.9, to set the environment variable, open a Terminal window and type the
        following:

        launchctl setenv OPENSSL_ROOT <path to top level folder containing openssl>

    9b. With Mac OS X 10.10, environment variable processing changed. Most importantly, OPENSSL_ROOT
        must be defined before launching Xcode (Xcode will not pick up new or changed variables
        after launching). Therefore, to set the environment variable, open a Terminal window and type
        the following:

        launchctl setenv OPENSSL_ROOT <path to top level folder containing openssl>
        sudo killall Finder
        sudo killall Dock

Tour

The SDK contains a "build" folder and an "alljoyn_objc" folder. "Build" contains the AllJoyn
Core Standard Client libraries for each of the supported binary types.  These libraries are used
by the AllJoyn Obj-C binding found in alljoyn_objc.

Samples for iOS and OS/X. The samples are located under alljoyn_objc/samples.

Code Generator. A code generator is included to assist your development of
AllJoyn-enabled apps for iOS and Mac OS/X. The source for this tool is located
under alljoyn_objc/AllJoynCodeGenerator.

Please take the time to read the "AllJoyn Programming Guide for the Objective-C
Language" document and follow the tutorial contained within, as it explains in easy
to follow steps how to create a new AllJoyn-enabled iOS app from scratch, in
addition to introducing you to some background concepts that will help you
understand AllJoyn.
