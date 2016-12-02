/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/

#include <gtest/gtest.h>
#include <qcc/FileStream.h>
#include <qcc/String.h>
#include <Status.h>

using namespace qcc;

/*
 * This test assumes that ./alljoynTestFile, ./alljoynTestDir,
 * and //alljoynTestDir don't exist prior to running
 */
TEST(FileSinkTest, validFileSink) {
    const char* pass[] = {
        "alljoynTestFile",               /* Creation of file */
        "alljoynTestFile",               /* Open existing file */
        "alljoynTestDir/foo",            /* Creation of both directory and file */
        "alljoynTestDir/bar",            /* Creation of file in existing directory */
        "alljoynTestDir/../alljoynTestDir/foo", /* Normalize paths and open existing file */
        "alljoynTestDir//bar",           /* Normalize path for extra slashes */
#ifdef _WIN32                     /* Do not have permissions to create a file at root for other OSs without super user access */
        "//alljoynTestDir/foo",          /* Leading slashes */
#endif
        "alljoynTestDir/dir/foo",        /* Create multiple directories */
        "alljoynTestDir/dir/bar",        /* Creation of file in existing nested directory */
        NULL
    };
    for (const char** pathname = pass; *pathname; ++pathname) {
        FileSink f = FileSink(*pathname, FileSink::PRIVATE);
        EXPECT_TRUE(f.IsValid()) << *pathname;
    }

    const char* cleanup[] = {
        "alljoynTestFile",               /* Creation of file */
        "alljoynTestDir/foo",            /* Creation of both directory and file */
        "alljoynTestDir/bar",            /* Creation of file in existing directory */
#ifdef _WIN32                     /* Do not have permissions to create a file at root for other OSs without super user access */
        "/alljoynTestDir/foo",          /* Leading slashes */
#endif
        "alljoynTestDir/dir/foo",        /* Create multiple directories */
        "alljoynTestDir/dir/bar",        /* Creation of file in existing nested directory */
        NULL
    };
    // Cleanup files after test
    // This will not delete the directories
    for (const char** pathname = cleanup; *pathname; ++pathname) {
        QStatus status = FileExists(*pathname);
        EXPECT_EQ(ER_OK, status) << "FileExists Status: " << QCC_StatusText(status) << " File: " << *pathname;
        status = DeleteFile(*pathname);
        EXPECT_EQ(ER_OK, status) << "DeleteFile Status: " << QCC_StatusText(status) << " File: " << *pathname;
    }
}

TEST(FileSinkTest, invalidFileSink) {
    qcc::String foofile = "alljoynTestDir/dir/foo";
    FileSink f = FileSink(foofile, FileSink::PRIVATE);
    EXPECT_TRUE(f.IsValid());

    const char* xfail[] = {
        "alljoynTestDir/dir",  /* Create a file that is already an existing directory */
#if !defined(_WIN32)
        "//alljoynTestDir/foo", /* Do not have permissions to create a file at root for other OSs without super user access */
#endif
        NULL
    };
    for (const char** pathname = xfail; *pathname; ++pathname) {
        f = FileSink(*pathname, FileSink::PRIVATE);
        EXPECT_FALSE(f.IsValid());
    }
    //Cleanup files after test
    //QStatus status = DeleteFile(foofile);
    //EXPECT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status) << " File: " << foofile.c_str();
}