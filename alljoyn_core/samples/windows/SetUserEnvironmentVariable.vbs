' SetUserEnvironmentVariable.vbs
' intent: this script is used by the SDK installer during the 
'         custom action stage to set the installation path environment variable
' usage:
' SetUserEnvironmentVariable.vbs /InstallPoint=<sdk path>|/AllUsers=<flag>
'
'

Dim bAllUsersInstall : bAllUsersInstall = 0
Dim strInstallPointPath : Set strInstallPointPath = Nothing
Dim strNamedArguments

strNamedArguments = Session.Property("CustomActionData")

' Parse the args from the input format:
Dim arrArgs
arrArgs = Split(strNamedArguments, "|")

' /InstallPoint
arrInstallPointParam = Split(arrArgs(0), "=")
strInstallPointPath = arrInstallPointParam(1)

' /AllUsers
arrAllUsersParam = Split(arrArgs(1), "=")
bAllUsersInstall = arrAllUsersParam(1)

' Choose the right type of environment (user or system)
Set WSHShell = CreateObject("WScript.Shell")

If (bAllUsersInstall = "1") Then
    Set WshEnv = WSHShell.Environment("SYSTEM")
Else 
    Set WshEnv = WSHShell.Environment("USER")
End If

' Use FileSystemObject to get a properly formatted path 
Set fso = CreateObject("Scripting.FileSystemObject")
Dim strInstallPointAbsolutePath
strInstallPointAbsolutePath = fso.GetAbsolutePathName(strInstallPointPath)

' Set the variable
WshEnv("ALLJOYN_SDK_HOME") = strInstallPointAbsolutePath


