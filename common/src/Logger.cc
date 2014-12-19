/**
 * @file
 * System logging facility for daemons.
 */

/******************************************************************************
 *
 *
 * Copyright (c) 2009-2011, 2014 AllSeen Alliance. All rights reserved.
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

#include <qcc/platform.h>

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#if defined(QCC_OS_GROUP_WINDOWS)
#elif defined(QCC_OS_ANDROID)
#include <android/log.h>
#else
#include <syslog.h>
#endif

#include <qcc/Debug.h>
#include <qcc/Logger.h>
#include <qcc/Util.h>

using namespace qcc;


#if defined(QCC_OS_ANDROID)
static const int androidPriorityMap[] = {
    ANDROID_LOG_FATAL,      // LOG_EMERG
    ANDROID_LOG_ERROR,      // LOG_ALERT
    ANDROID_LOG_ERROR,      // LOG_CRIT
    ANDROID_LOG_ERROR,      // LOG_ERR
    ANDROID_LOG_WARN,       // LOG_WARNING
    ANDROID_LOG_DEFAULT,    // LOG_NOTICE
    ANDROID_LOG_INFO,       // LOG_INFO
    ANDROID_LOG_DEBUG       // LOG_DEBUG
};

#endif


void qcc::Log(int priority, const char* format, ...)
{
    LoggerSetting* loggerSettings = LoggerSetting::GetLoggerSetting();
    va_list ap;


    loggerSettings->lock.Lock();

#if !defined(QCC_OS_GROUP_WINDOWS)
    if (loggerSettings->UseSyslog()) {

#if defined(QCC_OS_ANDROID)
        if (priority <= loggerSettings->GetLevel()) {
            va_start(ap, format);
            __android_log_vprint(androidPriorityMap[priority], loggerSettings->name, format, ap);
            va_end(ap);
        }
#else  // QCC_OS_LINUX || QCC_OS_DARWIN

        va_start(ap, format);
        vsyslog(priority, format, ap);
        va_end(ap);

#endif

    }
#endif

    if (loggerSettings->UseStdio()) {
        if (priority <= loggerSettings->GetLevel()) {
            va_start(ap, format);
            vfprintf(loggerSettings->GetFile(), format, ap);
            va_end(ap);
            fflush(loggerSettings->GetFile());
        }
    }

    loggerSettings->lock.Unlock();
}

LoggerSetting* LoggerSetting::singleton = NULL;

void LoggerSetting::SetSyslog(bool enable)
{
#if !defined(QCC_OS_GROUP_WINDOWS)
    lock.Lock();
#if !defined(QCC_OS_ANDROID)
    if (enable) {
        if (!useSyslog) {
            if (name) {
                openlog(name, 0, LOG_DAEMON);
            } else {
                enable = false;
            }
        }
    } else {
        if (useSyslog) {
            closelog();
        }
    }
#endif
    useSyslog = enable;
    lock.Unlock();
#endif
}


void LoggerSetting::SetFile(FILE* file)
{
    lock.Lock();
    if (UseStdio()) {
        fflush(this->file);
    }
    this->file = file;
    lock.Unlock();
}


void LoggerSetting::SetLevel(int level)
{
    lock.Lock();
    this->level = level;

#if !defined(QCC_OS_GROUP_WINDOWS) && !defined(QCC_OS_ANDROID)
    if (UseSyslog()) {
        setlogmask(LOG_UPTO(level));
    }

#endif

    lock.Unlock();
}


void LoggerSetting::SetName(const char* name)
{
    lock.Lock();
    this->name = name;
    lock.Unlock();
}


LoggerSetting::LoggerSetting(const char* name, int level, bool useSyslog, FILE* file) :
    name(name), level(level), useSyslog(useSyslog), file(file)
{
#if !defined(QCC_OS_GROUP_WINDOWS) && !defined(QCC_OS_ANDROID)
    if (useSyslog) {
        openlog(name, 0, LOG_DAEMON);
    }
#endif
}


LoggerSetting::~LoggerSetting()
{
#if !defined(QCC_OS_GROUP_WINDOWS) && !defined(QCC_OS_ANDROID)
    if (useSyslog) {
        closelog();
    }
#endif
}


LoggerSetting* LoggerSetting::GetLoggerSetting(const char* name, int level,
                                               bool useSyslog, FILE* file)
{
    if (!singleton) {
        singleton = new LoggerSetting(name, level, useSyslog, file);
    } else {
        singleton->lock.Lock();
        singleton->SetName(name);
        singleton->SetLevel(level);
        singleton->SetSyslog(useSyslog);
        singleton->SetFile(file);
        singleton->lock.Unlock();
    }
    return singleton;
}
int loggerInitCounter = 0;
bool LoggerInit::cleanedup = false;
LoggerInit::LoggerInit()
{
    loggerInitCounter++;
    //Do nothing. singleton will be initialized in the first call to GetLoggerSetting
}

LoggerInit::~LoggerInit()
{

    if (--loggerInitCounter == 0 && !cleanedup) {
        //Cleanup singleton
        if (LoggerSetting::singleton) {
            delete LoggerSetting::singleton;
            LoggerSetting::singleton = NULL;
        }
        cleanedup = true;
    }
}
void LoggerInit::Cleanup()
{
    if (!cleanedup) {
        //Cleanup singleton
        if (LoggerSetting::singleton) {
            delete LoggerSetting::singleton;
            LoggerSetting::singleton = NULL;
        }
        cleanedup = true;

    }
}

