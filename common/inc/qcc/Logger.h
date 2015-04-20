/**
 * @file
 * System logging facility for daemons.
 */

/******************************************************************************
 *
 *
 * Copyright AllSeen Alliance. All rights reserved.
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
#ifndef _LOGGER_H
#define _LOGGER_H

#include <qcc/platform.h>

#include <stdio.h>
#if !defined(QCC_OS_GROUP_WINDOWS)
#include <syslog.h>
#endif

#include <qcc/Mutex.h>

#if defined(QCC_OS_GROUP_WINDOWS)
// Define the same log levels as used by UNIX syslog facility.
#define LOG_EMERG       0       /**< system is unusable */
#define LOG_ALERT       1       /**< action must be taken immediately */
#define LOG_CRIT        2       /**< critical conditions */
#define LOG_ERR         3       /**< error conditions */
#define LOG_WARNING     4       /**< warning conditions */
#define LOG_NOTICE      5       /**< normal but significant condition */
#define LOG_INFO        6       /**< informational */
#define LOG_DEBUG       7       /**< debug-level messages */
#endif

#if defined(QCC_OS_ANDROID)
#define LOGGERSETTING_DEFAULT_NAME "alljoyn"
#define LOGGERSETTING_DEFAULT_SYSLOG true
#define LOGGERSETTING_DEFAULT_FILE NULL
#else
#define LOGGERSETTING_DEFAULT_NAME NULL
#define LOGGERSETTING_DEFAULT_SYSLOG false
#define LOGGERSETTING_DEFAULT_FILE stderr
#endif

namespace qcc {

/**
 * Send the specified message to the logging facility configured via LoggerSetting.
 *
 * @param priority  Logging priority level.
 * @param format    Formatting string supporting the same formatting as vfprintf().
 * @param ...       Parameters for the formatting string.
 */
void Log(int priority, const char* format, ...);

/**
 * This class sets up the logging facility.  It is for controlling how and
 * where log messages are delivered via the qcc:Log() function.  The form of
 * GetLoggerSetting() that takes parameters should be called once prior to
 * calling qcc::Log() so that the log output will go somewhere useful.
 */
class LoggerSetting {
  public:
    /**
     * Enable or disable delivery to syslog.  This only affects POSIX systems
     * that support syslog.
     *
     * @param enable    true = enable syslog; false = disable syslog
     */
    void SetSyslog(bool enable);

    /**
     * Specify the FILE stream to which logging will be delivered.
     *
     * @param file  An opened FILE stream for writing (NULL disables writing to files).
     */
    void SetFile(FILE* file);

    /**
     * Set the logging level for filtering output.  Log messages with a lower
     * level than that specified will be discarded rather than output.
     *
     * @param level Logging filter level.
     */
    void SetLevel(int level);

    /**
     * Set the name.
     *
     * @param level Logging filter level.
     */
    void SetName(const char* name);

    /**
     * Determine whether syslog is in use or not.
     *
     * @return  true = syslog in use; false = syslog not in use.
     */
    bool UseSyslog() const { return useSyslog; }

    /**
     * Determine whether STDIO FILE stream is in use or not.
     *
     * @return  true = STDIO FILE stream in use; false = STDIO FILE stream not in use.
     */
    bool UseStdio() const { return static_cast<bool>(file); }

    /**
     * Get the current logging filter level.
     *
     * @return  The current logging filter level.
     */
    int GetLevel() const { return level; }

    /**
     * Get the STDIO FILE stream pointer.
     *
     * @return  The STDIO FILE stream pointer (may be NULL).
     */
    FILE* GetFile() const { return file; }

    /**
     * Convenience function for getting access to the instantiated
     * LoggerSetting object and setting its attributes.  This is normally used
     * when initializing the logging facility.
     *
     * @return  Pointer to most recently instantiated LoggerSetting object.
     */
    static LoggerSetting * AJ_CALL GetLoggerSetting(const char* name = LOGGERSETTING_DEFAULT_NAME,
                                                    int level = LOG_DEBUG,
                                                    bool useSyslog = LOGGERSETTING_DEFAULT_SYSLOG,
                                                    FILE* file = LOGGERSETTING_DEFAULT_FILE);

  private:
    static void Init();
    static void Shutdown();
    friend class StaticGlobals;

    static LoggerSetting* singleton;    ///< Static pointer to most recent instance.
    const char* name;                   ///< Process name for syslog.
    int level;                          ///< Logging filter level.
    bool useSyslog;                     ///< Flag controlling use of syslog.
    FILE* file;                         ///< File stream pointer.
    qcc::Mutex lock;                    ///< Synchronization mutex.

    /**
     * Constructor for initializing useful values..
     *
     * @param name      Process name for use with syslog.
     * @param level     Logging filter level.
     * @param useSyslog Set whether to use syslog or not for writing log messages.
     * @param file      STDIO FILE stream pointer for writing log messages.
     */
    LoggerSetting(const char* name, int level, bool useSyslog, FILE* file);

    /**
     * Destructor.  Closes the syslog.  Does NOT close the file stream.
     */
    ~LoggerSetting();

    /**
     * The qcc:Log() function needs access to private members of LoggerSetting.
     */
    friend void qcc::Log(int priority, const char* format, ...);
};

}
#endif
