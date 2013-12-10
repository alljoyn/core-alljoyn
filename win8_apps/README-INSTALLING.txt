
    Installing the AllJoyn WinRT Samples

1.  Create a workspace directory to store the AllJoyn SDK. 
    Example: C:\Workspace\AllJoyn

2.  Download the AllJoyn SDK zip file for the target platform (x86, x64, arm)
    and variant (debug, release) and extract the files to the workspace directory. 

3.  Download the zip file that contains AllJoyn WinRT sample apps. 

4.  Extract the sample apps zip file as another subdirectory in the workspace. 
    Example: C:\Workspace\AllJoyn\win8_apps. 
    This will contain subdirectories cpp, csharp, and javascript, for each type of app.

5.  Browse to a sample app and open the Visual Studio solution file. 

6.  Create an environment variable AllJoynSDKPath and set it equal to dist directory
    for the platform and variant downloaded in step 2.
    Example: set AllJoynSDKPath=C:\Workspace\AllJoyn\alljoyn-3.1.0-win8x64-sdk-dbg

7.  Configure the projects in the sample solution to match the target platform and
    variant, using the guidance in the document "Building the AllJoyn WinRT Samples on
    different platforms." 

    You should now be able to build and run the sample solution.
