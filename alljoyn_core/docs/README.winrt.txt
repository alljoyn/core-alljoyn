
AllJoyn SDK for WinRT
---------------------

This subtree contains one complete copy of the AllJoyn SDK for WinRT, built
for a single CPU (x86, x64, or arm) and VARIANT (either debug or release).

The CPU and VARIANT are normally incorporated into the name of the package or
folder containing this SDK, as follows:

    Package Name    CPU
    ------------    ---
    winrt           arm
    win8x64         x86_64
    win8x86         x86

This SDK was built on a Windows 8 x86_64 system, using Microsoft Visual Studio 2012.

NOTE:

    This SDK works with the Microsoft Windows Runtime architecture, aka
    WinRT, and supports the new generation of Windows touch-screen devices.

    This SDK does NOT work with Microsoft's Win32 architecture, used in
    traditional Windows desktops and servers. For that, see AllJoyn SDK for
    Windows 7.


Windows, Win32, WinRT, and Visual Studio are registered trademarks or trademarks
of the Microsoft group of companies.


Please see ReleaseNotes.txt for the applicable AllJoyn release version and
related information on new features and known issues.

AllJoyn for WinRT sample apps are not found in this SDK. They are maintained in a
separate package.


Summary of files
----------------

winRT/bin/AllJoyn.winmd         metadata file
winRT/bin/AllJoyn.dll           assembly
winRT/bin/AllJoyn.xml           API documentation (Intellisense)

winRT/bin/AllJoyn.pdb           Debug symbols
winRT/bin/AllJoyn.exp           special build support
winRT/bin/AllJoyn.lib           special build support

winRT/docs/AllJoyn.chm          stand-alone API documentation
