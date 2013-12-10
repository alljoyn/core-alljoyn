=== How to build ===
1. Checkout win8_apps and place it at the same directory level as alljoyn_core and common

2. Configure environment variables because AllJoynStreaming has to reference the AllJoyn.dll and the header files in common. 

set ALLJOYN_DIST=C:\Workspace\AllJoyn\build\win8\x86_64\debug\dist\
set COMMON_DIR=C:\Workspace\AllJoyn\common

3. Build AllJoynStreaming.dll:

Scons WS=fix OS=win8 CPU=x86_64  MSVC_VERSION=11.0 STLPORT=false

4. After the build process finishes, AllJoynStreaming.dll is located at build/${OS}/${CPU}/${VARIANT}/dist/bin

=== Brief Introduction ===
The main purpose of this module is to facilitate developers to write Windows 8 style P2P media streaming application using AllJoyn. It allows
an Windows 8 application to serve as a media server which provides media stream source or as a media player which consumes media stream from a 
peer. The module references AllJoyn WinRT binding API. So AllJoyn.dll is required for building and using AllJoynStreaming.dll.
