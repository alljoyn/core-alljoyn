/******************************************************************************
 * Copyright (c) 2011, AllSeen Alliance. All rights reserved.
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
        QStatus status = DeleteFile(*pathname);
        EXPECT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status) << " File: " << *pathname;
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
        FileSink f = FileSink(*pathname, FileSink::PRIVATE);
        EXPECT_FALSE(f.IsValid());
    }
    //Cleanup files after test
    //QStatus status = DeleteFile(foofile);
    //EXPECT_EQ(ER_OK, status) << "Status: " << QCC_StatusText(status) << " File: " << foofile.c_str();
}