
AllJoyn SDK for OS X and iOS
----------------------------

This subtree contains one complete copy of the AllJoyn SDK for OS X and iOS.

This SDK includes all supported combinations of
    target type (OS X, iphone OS, iphone OS simulator)
    CPU         (x86, arm, armv7, armv7s)
    VARIANT     (Debug or Release)

Please see alljoyn_objc/README-INSTALLING.txt for instructions on how to
install this SDK and use it with Xcode.

Please see ReleaseNotes.txt for the applicable AllJoyn release version and
related information on new features and known issues.

OS X, iPhone, and Xcode are trademarks or registered trademarks of Apple Inc.
IOS is a trademark or registered trademark of Cisco in the U.S. and other countries.


Summary of file and directory structure:
----------------------------------------

build/              core AllJoyn functionality, implemented in C++
                      - built from alljoyn.git subtrees alljoyn_core, common, alljoyn_objc
                      - required for all AllJoyn applications

    darwin/.../dist/cpp/inc/                headers and client libraries,
    darwin/.../dist/cpp/lib/                  in sub-folders by target platform.

    docs/html/                              AllJoyn Core API documentation


alljoyn_objc/       Objective-C language binding for core AllJoyn functionality
                      - a direct copy of alljoyn.git subtree alljoyn_objc

    AllJoynFramework/                       OBJC language binding
    AllJoynFramework_iOS/

    alljoyn_darwin.xcodeproj/               Xcode support

    AllJoynCodeGenerator/                   Development aid

    Unity/                                  Unity language binding

    samples/OSX/                            sample apps
    samples/iOS/

services/about/     AllJoyn "About" Service
                      - a direct copy of alljoyn.git subtree services/about

    cpp                                     CPP implementation
    ios                                     OBJC binding
