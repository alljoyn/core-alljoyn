@Echo OFF
Rem @file
Rem @brief  Make the .cab file for Win7 installation.

Rem ****************************************************************************
Rem     Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
Rem     Project (AJOSP) Contributors and others.
Rem
Rem     SPDX-License-Identifier: Apache-2.0
Rem
Rem     All rights reserved. This program and the accompanying materials are
Rem     made available under the terms of the Apache License, Version 2.0
Rem     which accompanies this distribution, and is available at
Rem     http://www.apache.org/licenses/LICENSE-2.0
Rem
Rem     Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
Rem     Alliance. All rights reserved.
Rem
Rem     Permission to use, copy, modify, and/or distribute this software for
Rem     any purpose with or without fee is hereby granted, provided that the
Rem     above copyright notice and this permission notice appear in all
Rem     copies.
Rem
Rem     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
Rem     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
Rem     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
Rem     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
Rem     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
Rem     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
Rem     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
Rem     PERFORMANCE OF THIS SOFTWARE.
Rem ****************************************************************************

setlocal ENABLEDELAYEDEXPANSION

Rem The first argument to this script is the directory to work out of.
cd %1
if '%2'=='' goto NoMsDevEnv

set DDF_FILE=ajn.ddf

Rem Overwrite any existing DDF_FILE with the configuration/constant
Rem portion of the file.
Echo ; Configuration file for building AJN.CAB.> %DDF_FILE%
Echo .OPTION EXPLICIT    ; Generate errors on variable typos>> %DDF_FILE%
Echo .Set MaxDiskSize=0    ; All files go in a single cabinet>> %DDF_FILE%
Echo .Set CabinetNameTemplate=AJN.CAB>> %DDF_FILE%
Echo .set DiskDirectoryTemplate=.    ; All cabinets go in a single directory>> %DDF_FILE%
Echo .Set CompressionType=MSZIP>> %DDF_FILE%
Echo .Set Cabinet=on>> %DDF_FILE%
Echo .Set Compress=on>> %DDF_FILE%
Echo .Set UniqueFiles=on>> %DDF_FILE%

set BUILD_SRC=..\..\..\build\win7

if not exist %BUILD_SRC% goto NoBuild

@Echo OFF
Echo Adding x86 files.
for /R %BUILD_SRC%\x86 %%f in (*) do (
        for /F "usebackq tokens=3,4,5,6,7,8,9,10* delims=\" %%h in ('%%f') do (
                call :AddFile %%f %%h %%i %%j %%k %%l %%m %%n %%o %%p
        )
)
Echo Adding x86_64 files.
for /R %BUILD_SRC%\x86_64 %%f in (*) do (
        for /F "usebackq tokens=3,4,5,6,7,8,9,10* delims=\" %%h in ('%%f') do (
                call :AddFile %%f %%h %%i %%j %%k %%l %%m %%n %%o %%p
        )
)

makecab /f %DDF_FILE%
if ERRORLEVEL 1 goto MakeCabFail

Rem ******************************************************
Rem We now have the cab file. Build ExpandCab.exe and AllJoynWinInstaller.msi.
Rem ******************************************************

Echo %2 AllJoynWinInstaller.sln /build "Release|Win32"
%2 AllJoynWinInstaller.sln /build "Release|Win32"
if ERRORLEVEL 1 goto MakeMsiFail

goto End
:NoMsDevEnv
Echo The second argument must be the full path to Visual Studio "devenv.exe".
endlocal
Exit /B 1

:NoProgFiles
Echo The environment does not include either of the expected variables 'ProgramFiles' or 'ProgramFiles(x86)'.
endlocal
Exit /B 1

:MakeMsiFail
Echo Creation of AllJoynWinInstaller.msi failed.
endlocal
Exit /B 1

:MakeCabFail
Echo MakeCab failed.
endlocal
Exit /B 1

:NoBuild
Echo Build '%BUILD_SRC%' directory not found.
endlocal
Exit /B 1


:End
endlocal
Exit /B 0

Rem ******************************************************
Rem This is a "subroutine" called from above.
Rem ******************************************************
:AddFile
setlocal ENABLEDELAYEDEXPANSION
set SRC_FILE=%1
shift

if /I "%1"=="x86_64" goto FoundProc
if /I "%1"=="x86" goto FoundProc
shift

if /I "%1"=="x86_64" goto FoundProc
if /I "%1"=="x86" goto FoundProc
shift

if /I "%1"=="x86_64" goto FoundProc
if /I "%1"=="x86" goto FoundProc
shift

if /I "%1"=="x86_64" goto FoundProc
if /I "%1"=="x86" goto FoundProc
shift

if /I "%1"=="x86_64" goto FoundProc
if /I "%1"=="x86" goto FoundProc
shift

if /I "%1"=="x86_64" goto FoundProc
if /I "%1"=="x86" goto FoundProc
shift

if /I "%1"=="x86_64" goto FoundProc
if /I "%1"=="x86" goto FoundProc

Echo Unable to find processor directory in
Echo %1
pause
Exit /B 1

:FoundProc

Rem At this point:
Rem %1 is x86 or x86_64,
Rem %2 is debug or release,
Rem %3 is dist.
Rem %4... is the rest of the path and file name.

Rem Ignore all contents not in the 'dist' directory.
if /I "%3" NEQ "dist" Exit /B 0

Rem Remove "dist" while creating the destination file path and name.
set DEST_FILE=%1\%2\%4

Rem Move %5 into the %1 position.
shift
shift
shift
shift

Rem Check to see if there were any further path and/or filenames that
Rem need to be added to the destination.

if "%1" NEQ "" set DEST_FILE=%DEST_FILE%\%1
shift
if "%1" NEQ "" set DEST_FILE=%DEST_FILE%\%1
shift
if "%1" NEQ "" set DEST_FILE=%DEST_FILE%\%1
shift
if "%1" NEQ "" set DEST_FILE=%DEST_FILE%\%1
shift
if "%1" NEQ "" set DEST_FILE=%DEST_FILE%\%1
shift
if "%1" NEQ "" set DEST_FILE=%DEST_FILE%\%1
shift

Rem echo "%SRC_FILE%" "%DEST_FILE%"
echo "%SRC_FILE%" "%DEST_FILE%">> %DDF_FILE%

Exit /B 0
