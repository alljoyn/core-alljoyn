/******************************************************************************
 * Copyright (c) 2010 - 2011, AllSeen Alliance. All rights reserved.
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
 *
 ******************************************************************************/
#define QCC_OS_GROUP_POSIX
#include <string.h>
#include <jni.h>
#include <android/log.h>
#define LOG_TAG "daemon-jni"

// The AllJoyn daemon has an alternate personality in that it is built as a
// static library, liballjoyn-daemon.a.  In this case, the entry point main() is
// replaced by a function called DaemonMain.  Calling DaemonMain() here
// essentially runs the AllJoyn daemon like it had been run on the command line.
//

namespace ajn {
extern const char* GetVersion();        /**< Gives the version of AllJoyn Library */
extern const char* GetBuildInfo();      /**< Gives build information of AllJoyn Library */
};

extern int DaemonMain(int argc, char** argv, char* serviceConfig);


void do_log(const char* format, ...)
{
    va_list arglist;

    va_start(arglist, format);
    __android_log_vprint(ANDROID_LOG_DEBUG, LOG_TAG, format, arglist);
    va_end(arglist);

    return;
}

extern "C" JNIEXPORT void JNICALL Java_org_alljoyn_bus_alljoyn_AllJoynDaemon_runDaemon(JNIEnv* env, jobject thiz, jobjectArray jargv, jstring jconfig)
{
    int i;
    jsize argc;
    do_log("runDaemon()\n");

    argc = env->GetArrayLength(jargv);
    do_log("runDaemon(): argc = %d\n", argc);
    char const** argv  = (char const**)malloc(argc * sizeof(char*));

    for (i = 0; i < argc; ++i) {
        jstring jstr = (jstring)env->GetObjectArrayElement(jargv, i);
        argv[i] = env->GetStringUTFChars(jstr, 0);
        do_log("runDaemon(): argv[%d] = %s\n", i, argv[i]);
    }

    char const* config = env->GetStringUTFChars(jconfig, 0);
    do_log("runDaemon(): config = %s\n", config);

    //
    // Run the alljoyn-daemon we have built as a library.
    //
    do_log("runDaemon(): calling DaemonMain()\n");
    int rc = DaemonMain(argc, (char**)argv, (char*)config);

    free(argv);
}

extern "C" {

JNIEXPORT jstring JNICALL Java_org_alljoyn_bus_alljoyn_AllJoynDaemon_getDaemonVersion(JNIEnv* env, jobject thiz)
{
    return env->NewStringUTF(ajn::GetVersion());
}

JNIEXPORT jstring JNICALL Java_org_alljoyn_bus_alljoyn_AllJoynDaemon_getDaemonBuildInfo(JNIEnv* env, jobject thiz)
{
    return env->NewStringUTF(ajn::GetBuildInfo());
}

}
