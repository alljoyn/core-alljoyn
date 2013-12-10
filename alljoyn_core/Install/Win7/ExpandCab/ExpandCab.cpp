/**
 * @file
 * @brief  Defines the entry point for the console application.
 * This implements "expand AJN.CAB -F:* <destination>" and
 * deleting all the files and subdirectories during the uninstall.
 */

/******************************************************************************
 *
 *
 * Copyright (c) 2013, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include "stdafx.h"

/**
 * Display the proper usage of this program to stdout.
 */
void Usage(void)
{
    fputs("Usage: ExpandCab <[/commit] || [/uninstall]> Directory.\n", stdout);
}

/**
 * Implement the commit phase of the installation.
 * This means "expand AJN.CAB -F:* destination" for the commit phase.
 * Outputs a user friendly message upon completion.
 *
 * @param destination    The destination directory to expand the .CAB file
 *                       into.
 *
 * @return - EXIT_SUCCESS if successful.
 */
int DoCommit(const char* destination);

/**
 * Implements, in essence, "rm /S /Q directory" for the uninstall phase.
 * Outputs a user friendly message if there was an error.
 *
 * @param directory      The destination directory to remove.
 * @param deleteCWD      If deleteCWD is true then none of the files in
 *                       the current directory are deleted and 'directory'
 *                       is not deleted. The subdirectories ARE traversed
 *                       and removed.
 *
 * @return - EXIT_SUCCESS if successful.
 */
int DeleteFilesAndDirectory(const char* directory, bool deleteCWD);

/**
 * Get the complete path for the expand.exe program which is in
 * the Windows System32 directory.
 *
 * @return - The complete path to expand.exe if successful.
 *           Returns an empty string if not found.
 */
const char* GetExpandExe(void);

/**
 * From arg0 make the destination path.
 * arg0 is in the form of "c:\AllJoyn\SDK\ExpandCab.exe".
 * The .MSI always puts this program into the destination directory.
 * This enables us to determine the destination directory for other
 * purposes.
 *
 * @param destination     The string buffer to put the destination
 *                        directory in.
 * @param destSizeInBytes The size of the buffer.
 * @param arg0            arg[0] from the command line to this program.
 *
 * @return - EXIT_SUCCESS if successful or EXIT_FAILURE if not.
 */
int MakeDestination(char* destination, size_t destSizeInBytes, const char* arg0);

/**
 * Entry point for the program.
 * Implements "expand AJN.CAB -F:* destination" for the commit phase.
 * Implements "rm /S /Q directory" for the uninstall phase.
 * @param argc     The number of arguments passed to the program.
 * @param argv     The arguments passed to the program.
 *
 * @return - EXIT_SUCCESS if successful.
 */
int main(int argc, char* argv[])
{
    int returnValue = EXIT_FAILURE;

    if (argc >= 2) {
        const static char commit[] = "/Commit";
        const static char uninstall[] = "/Uninstall";
        char destination[_MAX_PATH];

        returnValue = MakeDestination(destination, sizeof(destination), argv[0]);

        if (0 == returnValue) {
            if (0 == _strcmpi(argv[1], commit)) {
                returnValue = DoCommit(destination);
            } else {
                if (0 == _strcmpi(argv[1], uninstall)) {
                    returnValue = DeleteFilesAndDirectory(destination, false);
                } else {
                    Usage();
                }
            }
        }
    } else {
        Usage();
    }

    fputs("Press any key to continue...", stdout);

    int c = _getch();

    // If an function or arrow key get the second char as well.
    if (0 == c || 0xE0 == c) {
        _getch();
    }

    return returnValue;
}

int DoCommit(const char* destination)
{
    int returnValue = _chdir(destination);

    if (0 == returnValue) {
        const char* expand = GetExpandExe();

        if (expand[0] == '\0') {
            fputs("Unable to find Windows expand.exe.\n", stdout);
        } else {
            returnValue = _spawnl(_P_WAIT, expand, expand, "AJN.CAB", "-F:*", ".", NULL);

            if (-1 == returnValue) {
                perror("_spawnl() returned -1");
            } else {
                fputs("Files were extracted successfully.\n", stdout);
            }
        }
    } else {
        fputs("Unable to make '", stdout);
        fputs(destination, stdout);
        fputs("' the current working directory.\n", stdout);
    }

    return returnValue;
}

/**
 * Given a path and a file name make a complete path and put it in the
 * destination buffer. It is assumed that the destination buffer is at
 * least _MAX_PATH characters long.
 *
 * @param destination    The destination buffer for the complete path.
 * @param path           The path to the file.
 * @param file           The file at the end of the path.
 */
void MakeCompletePath(char* destination, const char* path, const char* file)
{
    strcpy_s(destination, _MAX_PATH, path);
    strcat_s(destination, _MAX_PATH, "\\");
    strcat_s(destination, _MAX_PATH, file);
}

int DeleteFilesAndDirectory(const char* directory, bool deleteCWD)
{
    int returnValue = EXIT_SUCCESS;
    char fileSpec[_MAX_PATH];

    MakeCompletePath(fileSpec, directory, "*.*");

    struct _finddata_t fileInfo = {0};
    intptr_t handle = _findfirst(fileSpec, &fileInfo);

    if (-1 != handle) {
        do {
            MakeCompletePath(fileSpec, directory, fileInfo.name);

            // If the bit is set indicating a subdirectory then recurse.
            if (_A_SUBDIR & fileInfo.attrib) {
                // Ignore the current and parent directories.
                if (0 != strcmp(".", fileInfo.name) &&
                    0 != strcmp("..", fileInfo.name)) {

                    returnValue = DeleteFilesAndDirectory(fileSpec, true);

                    if (EXIT_SUCCESS == returnValue) {
                        fputs("Successfully deleted directory '", stdout);
                    } else {
                        fputs("Unable to delete directory '", stdout);
                    }

                    fputs(fileInfo.name, stdout);
                    fputs("'.\n", stdout);
                }
            } else {
                if (deleteCWD) {
                    returnValue = remove(fileSpec);

                    if (EXIT_SUCCESS != returnValue) {
                        fputs("Unable to delete file '", stdout);
                        fputs(fileInfo.name, stdout);
                        fputs("'.\n", stdout);
                    }
                }
            }
        } while (EXIT_SUCCESS == returnValue && -1 != _findnext(handle, &fileInfo));

        _findclose(handle);
    }

    if (deleteCWD) {
        returnValue = _rmdir(directory);
    }

    return returnValue;
}

const char* GetExpandExe(void)
{
    static char returnValue[_MAX_PATH + 1];

    if (returnValue[0] == '\0') {
        char* systemRoot;
        int status = _dupenv_s(&systemRoot, NULL, "SystemRoot");

        if (systemRoot) {
            strcpy_s(returnValue, sizeof(returnValue) / sizeof(returnValue[0]), systemRoot);
            strcat_s(returnValue, sizeof(returnValue) / sizeof(returnValue[0]), "\\system32\\expand.exe");

            // Make sure the file actually exists.
            int access = _access(returnValue, 0);

            if (0 != access) {
                returnValue[0] = '\0';
            }

            free(systemRoot);
        }
    }

    return returnValue;
}

int MakeDestination(char* destination, size_t destSizeInBytes, const char* arg0)
{
    int returnValue = EXIT_FAILURE;

    if (arg0 && destination && destSizeInBytes > 0) {
        const char* pathEnd = strrchr(arg0, '\\');

        if (!pathEnd) {
            pathEnd = strrchr(arg0, '/');
        }

        if (!pathEnd) {
            pathEnd = arg0;
        }

        if (pathEnd - arg0 < (int)(destSizeInBytes / sizeof(destination[0]))) {
            while (arg0 < pathEnd) {
                *destination++ = *arg0++;
            }

            returnValue = EXIT_SUCCESS;
        }

        *destination = '\0';
    }

    return returnValue;
}
