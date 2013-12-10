
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

alljoyn_core/       core AllJoyn functionality, implemented in C++
                      - built from Git projects alljoyn_core, common, alljoyn_objc
                      - required for all AllJoyn applications

    build/darwin/.../dist/cpp/inc/          headers and client libraries,
    build/darwin/.../dist/cpp/lib/            in sub-folders by target platform.

    docs/html/                              AllJoyn Core API documentation
    AllJoyn_API_Changes_cpp.txt


alljoyn_objc/       Objective-C language binding for OSX/iOS
                      - Git project alljoyn_objc, as source library

    AllJoynFramework/                       OBJC language binding
    AllJoynFramework_iOS/

    alljoyn_darwin.xcodeproj/               Xcode support

    AllJoynCodeGenerator/                   Development aid

    Unity/                                  Unity language binding

    samples/OSX/                            sample apps
    samples/iOS/
