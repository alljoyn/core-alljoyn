Building the AllJoyn.unitypackage file.

Starting with AllJoyn release 14.06, all Core Standard Library SDK's are built by the AllSeen Alliance.
For AllJoyn's Unity binding, this means that the pre-built AllJoyn.unitypackage file can no longer be provided.
The new SDK's include everything needed to create the AllJoyn.unitypackage, except the Unity IDE itself.
Therefore, Unity developers can easily create their own AllJoyn.unitypackage file(s).
The following describes how to do this.

1. Identify a Windows workstation with a registered copy of Unity IDE.

2. Identify the AllJoyn Core SL SDK's you wish to package for Unity.
    Only the SDKs for Android and Windows support Unity binding.

3. Unzip the AllJoyn SDK on the Windows workstation.

4. Identify the folders containing the Unity binding.
    a. In the AllJoyn 14.12 SDK's for Android, these folders are
        alljoyn-android\core\alljoyn-14.12.00-dbg\unity ("debug" variant)
        alljoyn-android\core\alljoyn-14.12.00-rel\unity ("release" variant)
    b. In the AllJoyn 14.12 SDK's for Windows, they are
        alljoyn-14.12.00-win7x64vs2013-sdk\alljoyn-14.12.00-win7x64vs2013-sdk-dbg\unity ("debug" variant)
        alljoyn-14.12.00-win7x64vs2013-sdk\alljoyn-14.12.00-win7x64vs2013-sdk-rel\unity ("release" variant)

5. Open a CMD window.
    Add the path to Unity.exe (the Unity IDE) to your PATH, if it is not there already.
        For example, set PATH=%PATH%;C:\Program Files\Unity\Editor
    CD to each "unity" folder in the SDK, and run the following command in each location:
        Unity.exe -batchmode -nographics -quit -projectPath %CD% -exportPackage Assets AllJoyn.unitypackage
    This command creates the AllJoyn.unitypackage file.

6. Repeat the above as needed for Android, Windows, debug, release.
