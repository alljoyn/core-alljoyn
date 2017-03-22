AllJoyn iOS and macOS SDK README

************************************************************************************

PLEASE READ THIS DOCUMENT

AllJoyn Programming Guide for the Objective-C Language

************************************************************************************

Minimum Requirements for AllJoyn Development System

1. macOS 10.11 or higher
2. Xcode 8 or higher (it is available for free in the Mac App Store at the following web address:
   https://itunes.apple.com/us/app/xcode/id497799835?mt=12)

Installation

1. Unzip the AllJoyn SDK package to a folder on your development system.

2. Open Terminal and change the current directory to <AllJoyn>/alljoyn_objc

3. Build the AllJoyn SDK by running one of the following scripts:
   ./build_macos.sh       - for macOS with Objective-C bindings
   ./build_ios.sh         - for iOS with Objective-C bindings
   ./build_core_macos.sh  - for macOS with using C++
   ./build_core_ios.sh    - for iOS with using C++

4. Locate AllJoyn SDK build products in the <AllJoyn>/build folder:

   AllJoyn SDK for macOS
      Header files: <AllJoyn>/build/darwin/AllJoynFramework/include
      Static libraries: <AllJoyn>/build/darwin/AllJoynFramework/<Debug|Release>

   AllJoyn SDK for iOS
      Header files: <AllJoyn>/build/iOS/AllJoynFramework/include
      Static libraries: <AllJoyn>/build/iOS/AllJoynFramework/<Debug|Release>

   AllJoyn SDK for macOS (C++ only)
      Header files: <AllJoyn>/build/darwin/x86_64/<debug|release>/dist/cpp/inc
      Static libraries: <AllJoyn>/build/darwin/x86_64/<debug|release>/dist/cpp/lib

   AllJoyn SDK for iOS (C++ only)
      Header files: <AllJoyn>/build/iOS/universal/<iphoneos|iphonesimulator>/<debug|release>/dist/cpp/inc
      Static libraries: <AllJoyn>/build/iOS/universal/<iphoneos|iphonesimulator>/<debug|release>/dist/cpp/lib

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
