
AllJoyn SDK for Windows 7
-------------------------

This subtree contains one complete copy of the AllJoyn SDK for Windows 7, built
for a single CPU type (x86 or x86_64), VARIANT (either debug or release),
and for a single version of Microsoft Visual Studio (Visual Studio 2012, or Visual
Studio 2013 depending on the version of the AllSeen SDK that you downloaded).

The CPU, VARIANT, and MSVS version is normally incorporated into the name of the
package or folder containing this SDK.

NOTE:

    This SDK works with the Microsoft Win32 architecture, used in
    traditional Microsoft Windows desktops and servers with x86 or
    x86_64 CPUs.

    This SDK does NOT work with the Microsoft Windows 10 UWP (Universal
    Windows Platform) architecture. Please download the Windows 10 SDK
    in order to develop UWP applications for Windows 10.

Windows, Win32, UWP, and Visual Studio are registered trademarks or trademarks
of Microsoft.


Please see ReleaseNotes.txt for the applicable AllJoyn release version and
related information on new features and known issues.


Summary of file and directory structure:
----------------------------------------

The contents of this SDK are arranged into the following top level folders:

cpp/    core AllJoyn functionality, implemented in C++
          - built from alljoyn.git source subtrees alljoyn_core and common
          - required for all AllJoyn applications
java/   optional Java language binding          (built from alljoyn_java)
c/      optional ANSI C language binding        (built from alljoyn_c)


The contents of each top level folder are further arranged into sub-folders:

        ----------------------------------------------
cpp/    core AllJoyn functionality, implemented in C++
        ----------------------------------------------

    bin/                        executable binaries

    bin/samples/                pre-built sample programs

    docs/html/                  AllJoyn Core API documentation

    inc/alljoyn/                AllJoyn Core (C++) headers
    inc/qcc/

    lib/                        AllJoyn Core (C++) client libraries

        alljoyn.lib             Implements core API
        alljoyn_about.lib       Implements About interface
        ajrouter.lib            Implements bundled-router
        daemonlib.lib           AllJoyn router node library

    samples/                    C++ sample programs (see README)


        ---------------------
java/   Java language binding
        ---------------------

    docs/html/                  API documentation

    jar/                        client library, misc jar files

    lib/alljoyn_java.dll        client libraries
    lib/alljoyn_java.exp
    lib/alljoyn_java.lib

    samples/                    sample programs (see README)


        -----------------------
c/      ANSI C language binding
        -----------------------

    docs/html/                  API documentation

    inc/alljoyn_c/              ANSI C headers
    inc/qcc/

    lib/                        client libraries


Windows 10 AllJoyn resources
----------------------------
Preliminary Windows 10 AllJoyn documentation:
https://msdn.microsoft.com/en-us/library/windows/apps/windows.devices.alljoyn.aspx

Windows 10 AllJoyn secure samples:
http://go.microsoft.com/fwlink/p/?LinkID=534021
http://go.microsoft.com/fwlink/p/?LinkId=534025

Building Windows 10 AllJoyn Apps step-by-step:
bit.ly/1zkgrvU