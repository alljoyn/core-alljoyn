/**
 * @file
 *
 * Platform specific logger for WinRT
 */

/******************************************************************************
 *
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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
 *****************************************************************************/

#include <qcc/platform.h>

#include <qcc/OSLogger.h>

#include <list>

#include <qcc/Mutex.h>
#include <qcc/winrt/utility.h>
#include <qcc/winrt/exception.h>
#include <ctxtcall.h>
#include <ppltasks.h>

#define LOG_FILE_NAME L"alljoyn.log"
#define LOG_MAX_BACKLOG 1000

std::list<qcc::String> debugQueue;
qcc::Mutex debugMutex;

static void _WinRTFileLogCB()
{
    auto ranStream = std::make_shared<Windows::Storage::Streams::IRandomAccessStream ^>(nullptr);
    auto buf = std::make_shared<Windows::Storage::Streams::IBuffer ^>(nullptr);
    try {
        Windows::Foundation::IAsyncOperation<Windows::Storage::StorageFile ^> ^ op =
            Windows::Storage::KnownFolders::DocumentsLibrary->CreateFileAsync(LOG_FILE_NAME, Windows::Storage::CreationCollisionOption::OpenIfExists);
        concurrency::task<Windows::Storage::StorageFile ^> taskCreateFile(op);
        taskCreateFile.then([ranStream, buf] (concurrency::task<Windows::Storage::StorageFile ^> fileTask) {
                                Windows::Storage::StorageFile ^ file = fileTask.get();
                                return file->OpenAsync(Windows::Storage::FileAccessMode::ReadWrite);
                            }).then([ranStream, buf] (concurrency::task<Windows::Storage::Streams::IRandomAccessStream ^> rsTask) {
                                        (*ranStream) = rsTask.get();
                                        (*ranStream)->Seek((*ranStream)->Size);
                                        Windows::Storage::Streams::DataWriter ^ writer = ref new Windows::Storage::Streams::DataWriter();
                                        if (nullptr == writer) {
                                            QCC_THROW_EXCEPTION(ER_OUT_OF_MEMORY);
                                        }
                                        // Grab a buffer to write
                                        debugMutex.Lock();
                                        qcc::String strBytes = debugQueue.front();
                                        debugMutex.Unlock();
                                        Platform::ArrayReference<uint8> arrRef((unsigned char*)strBytes.c_str(),
                                                                               strBytes.length());
                                        writer->WriteBytes(arrRef);
                                        (*buf) = writer->DetachBuffer();
                                        return (*ranStream)->WriteAsync((*buf));
                                    }).then([ranStream, buf] (concurrency::task<unsigned int> writeTask) {
                                                writeTask.get();
                                                return (*ranStream)->FlushAsync();
                                            }).then([ranStream, buf] (concurrency::task<bool> flushTask) {
                                                        flushTask.get();
                                                        if (nullptr != ranStream && nullptr != (*ranStream)) {
                                                            delete (*ranStream);
                                                            (*ranStream) = nullptr;
                                                        }
                                                        if (nullptr != buf && nullptr != (*buf)) {
                                                            delete (*buf);
                                                            (*buf) = nullptr;
                                                        }
                                                        debugMutex.Lock();
                                                        debugQueue.pop_front();
                                                        size_t queueLength = debugQueue.size();
                                                        debugMutex.Unlock();
                                                        if (queueLength != 0) {
                                                            _WinRTFileLogCB();
                                                        }
                                                    });
    } catch (...) {
        // Error. Cleanup and try to log the next message in queue.
        if (nullptr != ranStream && nullptr != (*ranStream)) {
            delete (*ranStream);
            (*ranStream) = nullptr;
        }
        if (nullptr != buf && nullptr != (*buf)) {
            delete (*buf);
            (*buf) = nullptr;
        }
        debugMutex.Lock();
        debugQueue.pop_front();
        if (debugQueue.size() != 0) {
            _WinRTFileLogCB();
        }
        debugMutex.Unlock();
    }
}

static void WinRTFileLogCB(DbgMsgType type, const char* module, const char* msg, void* context)
{
    debugMutex.Lock();
    if (debugQueue.size() < LOG_MAX_BACKLOG) {
        debugQueue.push_back(msg);
        if (debugQueue.size() == 1) {
            _WinRTFileLogCB();
        }
    }
    debugMutex.Unlock();
}

static void WinRTTraceLogCB(DbgMsgType type, const char* module, const char* msg, void* context)
{
    wchar_t* wmsg = MultibyteToWideString(msg);
    if (NULL != wmsg) {
        OutputDebugStringW(wmsg);
        delete [] wmsg;
        wmsg = NULL;
    }
}

QCC_DbgMsgCallback QCC_GetOSLogger(bool useOSLog)
{
    return useOSLog ? WinRTTraceLogCB : WinRTFileLogCB;
}
