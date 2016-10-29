/**
 * @file
 *
 * Define a class that abstracts Linux threads.
 */

/******************************************************************************
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
#include <qcc/platform.h>

#include <algorithm>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <map>

#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/Mutex.h>
#include <qcc/Thread.h>
#include <qcc/LockLevel.h>
#include <qcc/PerfCounters.h>

#include <Status.h>

using namespace std;

/** @internal */
#define QCC_MODULE "THREAD"

namespace qcc {

#ifndef NDEBUG
static volatile int32_t started = { 0 };
static volatile int32_t running = { 0 };
static volatile int32_t joined = { 0 };
#endif

/** Global thread list */
Mutex* Thread::threadListLock = NULL;
map<ThreadId, Thread*>* Thread::threadList = NULL;

static pthread_key_t cleanExternalThreadKey;
bool Thread::initialized = false;

void Thread::CleanExternalThread(void* t)
{
    /* This function will not be called if value of key is NULL */
    if (!t) {
        return;
    }

    Thread* thread = reinterpret_cast<Thread*>(t);
    {
        ScopedMutexLock lock(*threadListLock);
        map<ThreadId, Thread*>::iterator it = threadList->find(thread->handle);
        if (it != threadList->end()) {
            if (it->second->threadState.isExternal()) {
                delete it->second;
                threadList->erase(it);
            }
        }
    }
}

QStatus Thread::StaticInit()
{
    if (!initialized) {
        /* Disable LockChecker for the threadListLock, thus allowing LockChecker to call GetThread() */
        Thread::threadListLock = new Mutex(LOCK_LEVEL_CHECKING_DISABLED);
        Thread::threadList = new map<ThreadId, Thread*>();
        int ret = pthread_key_create(&cleanExternalThreadKey, Thread::CleanExternalThread);
        if (ret != 0) {
            QCC_LogError(ER_OS_ERROR, ("Creating TLS key: %s", strerror(ret)));
            delete threadList;
            delete threadListLock;
            return ER_OS_ERROR;
        }
        initialized = true;
    }
    return ER_OK;
}

QStatus Thread::StaticShutdown()
{
    if (initialized) {
        void* thread = pthread_getspecific(cleanExternalThreadKey);
        // pthread_key_delete will not call CleanExternalThread for this thread,
        // so we manually call it here.
        Thread::CleanExternalThread(thread);
        int ret = pthread_key_delete(cleanExternalThreadKey);
        if (ret != 0) {
            QCC_LogError(ER_OS_ERROR, ("Deleting TLS key: %s", strerror(ret)));
        }

        /* A common root cause of a failed assertion here is the app forgetting to call BusAttachment::Join() */

        Thread::threadListLock = new Mutex(LOCK_LEVEL_CHECKING_DISABLED);
        if (Thread::threadList->size() != 0) {
            printf("size: %u >%s<\n", Thread::threadList->size(), Thread::threadList->begin()->second->funcName);
        }
        QCC_ASSERT(Thread::threadList->size() == 0);
        delete Thread::threadList;
        Thread::threadList = nullptr;
        delete Thread::threadListLock;
        Thread::threadListLock = nullptr;
        initialized = false;
    }
    return ER_OK;
}

QStatus AJ_CALL Sleep(uint32_t ms) {
    usleep(1000 * ms);
    return ER_OK;
}

Thread* Thread::GetThread()
{
    Thread* ret = nullptr;

    {
        /* Find thread on Thread::threadList */
        ScopedMutexLock lock(*threadListLock);

        map<ThreadHandle, Thread*>::const_iterator iter = threadList->find(pthread_self());
        if (iter != threadList->end()) {
            ret = iter->second;
        }
    }

    /* If the current thread isn't on the list, then create an external (wrapper) thread */
    if (nullptr == ret) {
        ret = new Thread("external", nullptr, true);
    }

    return ret;
}

const char* Thread::GetThreadName()
{
    Thread* thread = NULL;

    {
        /* Find thread on Thread::threadList */
        ScopedMutexLock lock(*threadListLock);
        map<ThreadId, Thread*>::const_iterator iter = threadList->find(pthread_self());
        if (iter != threadList->end()) {
            thread = iter->second;
        }
    }

    /* If the current thread isn't on the list, then don't create an external (wrapper) thread */
    if (thread == NULL) {
        return "external";
    }

    return thread->GetName();
}

void Thread::CleanExternalThreads()
{
    ScopedMutexLock lock(*threadListLock);
    map<ThreadId, Thread*>::iterator it = threadList->begin();
    while (it != threadList->end()) {
        if (it->second->threadState.isExternal()) {
            delete it->second;
            threadList->erase(it++);
        } else {
            ++it;
        }
    }
}

Thread::Thread(qcc::String name, Thread::ThreadFunction func, bool isExternal) :
    stopEvent(),
    function(isExternal ? NULL : func),
    handle(isExternal ? pthread_self() : 0),
    exitValue(NULL),
    threadArg(NULL),
    threadListener(NULL),
    platformContext(NULL),
    alertCode(0),
    auxListeners(),
    auxListenersLock(LOCK_LEVEL_THREAD_AUXLISTENERSLOCK),
    privateDataLock(LOCK_LEVEL_THREAD_PRIVATE_DATA),
    threadState(isExternal)
{
    IncrementPerfCounter(PERF_COUNTER_THREAD_CREATED);

    /* qcc::String is not thread safe.  Don't use it here. */
    funcName[0] = '\0';
    strncpy(funcName, name.c_str(), sizeof(funcName));
    funcName[sizeof(funcName) - 1] = '\0';

    /* If this is an external thread, add it to the thread list here since Run will not be called */
    if (isExternal) {
        QCC_ASSERT(func == NULL);
        ScopedMutexLock lock(*threadListLock);
        (*threadList)[handle] = this;
        if (pthread_getspecific(cleanExternalThreadKey) == NULL) {
            int ret = pthread_setspecific(cleanExternalThreadKey, this);
            if (ret != 0) {
                QCC_LogError(ER_OS_ERROR, ("Setting TLS key: %s", strerror(ret)));
            }
            QCC_ASSERT(ret == 0);
        }
    }
    QCC_DbgHLPrintf(("Thread::Thread() created %s - %x -- started:%d running:%d joined:%d",
                     funcName, handle, AtomicFetch(&started), AtomicFetch(&running), AtomicFetch(&joined)));
}


Thread::~Thread(void)
{
    QCC_DbgHLPrintf(("Thread::~Thread() destroying %s - %x", funcName, handle));

    if (!threadState.isExternal()) {
        Stop();
        Join();
    }

    QCC_DbgHLPrintf(("Thread::~Thread() destroyed %s - %x -- started:%d running:%d joined:%d",
                     funcName, handle, AtomicFetch(&started), AtomicFetch(&running), AtomicFetch(&joined)));
    IncrementPerfCounter(PERF_COUNTER_THREAD_DESTROYED);
}


ThreadInternalReturn Thread::RunInternal(void* arg)
{
    void* retVal = nullptr;
    Thread* thread(static_cast<Thread*>(arg));
    sigset_t newmask;

    sigemptyset(&newmask);
    sigaddset(&newmask, SIGUSR1);

    QCC_ASSERT(thread != NULL);
    QCC_ASSERT(thread->threadState.getCurrentState() == ThreadState::STARTING);

    /* Add this Thread to list of running threads */
    {
        ScopedMutexLock lock(*threadListLock);
        (*threadList)[pthread_self()] = thread;
        pthread_sigmask(SIG_UNBLOCK, &newmask, NULL);
    }
    {
        ScopedMutexLock lock(thread->privateDataLock);
        /* Plug race condition between Start and Run. (pthread_create may not write handle before run is called) */
        thread->handle = pthread_self();
    }

    QCC_DEBUG_ONLY(IncrementAndFetch(&started));

    QCC_DbgPrintf(("Thread::RunInternal: %s (pid=%x)", thread->funcName, (unsigned long) thread->handle));

    thread->threadState.started();

    ThreadReturn tmpExitValue = nullptr;
    ThreadState::State currentState = thread->threadState.getCurrentState();
    /* Start the thread if it hasn't been stopped */
    if (currentState == ThreadState::STARTING ||
        currentState == ThreadState::RUNNING) {
        QCC_DbgPrintf(("Starting thread: %s", thread->funcName));
        QCC_DEBUG_ONLY(IncrementAndFetch(&running));
        tmpExitValue = thread->Run(thread->threadArg);
        QCC_DEBUG_ONLY(DecrementAndFetch(&running));
        QCC_DbgPrintf(("Thread function exited: %s --> %p", thread->funcName, thread->exitValue));
    }

    thread->threadState.stop();

    ThreadHandle handle;
    {
        ScopedMutexLock lock(thread->privateDataLock);
        thread->exitValue = tmpExitValue;
        /*
         * Call thread exit callback if specified. Note that ThreadExit may dellocate the thread so the
         * members of thread may not be accessed after this call
         */
        retVal = thread->exitValue;
        handle = thread->handle;
    }
    thread->stopEvent.ResetEvent();

    {
        /* Call aux listeners before main listener since main listner may delete the thread */
        ScopedMutexLock lock(thread->auxListenersLock);
        ThreadListeners::iterator it = thread->auxListeners.begin();
        while (it != thread->auxListeners.end()) {
            ThreadListener* listener = *it;
            listener->ThreadExit(thread);
            it = thread->auxListeners.upper_bound(listener);
        }
    }

    if (thread->threadListener) {
        thread->threadListener->ThreadExit(thread);
    }

    /* This also means no QCC_DbgPrintf as they try to get context on the current thread */

    {
        /* Remove this Thread from list of running threads */
        ScopedMutexLock lock(*threadListLock);
        threadList->erase(handle);
    }

    thread->threadState.stopped();

    return reinterpret_cast<ThreadInternalReturn>(retVal);
}

static const uint32_t stacksize = 256 * 1024;

QStatus Thread::Start(void* arg, ThreadListener* listener)
{
    QStatus status = ER_OK;

    ThreadState::Rc stateRc = threadState.start();
    if (stateRc == ThreadState::IsExternalThread) {
        status = ER_EXTERNAL_THREAD;
    } else if (stateRc == ThreadState::AlreadyStopped) {
        status = ER_THREAD_STOPPING;
    } else if (stateRc == ThreadState::AlreadyRunning) {
        status = ER_THREAD_RUNNING;
    } else if (stateRc == ThreadState::Ok) {
        int ret;

        /*  Reset the stop event so the thread doesn't start out alerted. */
        stopEvent.ResetEvent();
        /* Create OS thread */
        this->threadArg = arg;
        this->threadListener = listener;

        pthread_attr_t attr;
        ret = pthread_attr_init(&attr);
        if (ret != 0) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Initializing thread attr: %s", strerror(ret)));
        }
        ret = pthread_attr_setstacksize(&attr, stacksize);
        if (ret != 0) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Setting stack size: %s", strerror(ret)));
        }
        ScopedMutexLock lock(privateDataLock);
        ret = pthread_create(&handle, &attr, RunInternal, this);
        QCC_DbgTrace(("Thread::Start() [%s] pid = %x", funcName, handle));
        if (ret != 0) {
            threadState.error();
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Creating thread %s: %s", funcName, strerror(ret)));
        }
        pthread_attr_destroy(&attr);
    }
    return status;
}

QStatus Thread::Stop(void)
{
    ThreadState::Rc stateRc = threadState.stop();
    if (stateRc == ThreadState::IsExternalThread) {
        QCC_LogError(ER_EXTERNAL_THREAD, ("Cannot stop an external thread"));
        return ER_EXTERNAL_THREAD;
    } else if ((stateRc == ThreadState::AlreadyJoined) ||
               (stateRc == ThreadState::InInitialState)) {
        QCC_DbgPrintf(("Thread::Stop() thread is dead [%s]", funcName));
        return ER_OK;
    } else {
        QCC_DbgTrace(("Thread::Stop() %x [%s]", handle, funcName));

        QStatus rc = stopEvent.SetEvent();
        return rc;
    }
}

QStatus Thread::Alert()
{
    if (threadState.getCurrentState() == ThreadState::DEAD) {
        return ER_DEAD_THREAD;
    }
    QCC_DbgTrace(("Thread::Alert() [%s:%srunning]", funcName, IsRunning() ? " " : " not "));
    return stopEvent.SetEvent();
}

QStatus Thread::Alert(uint32_t alertCode)
{
    if (threadState.getCurrentState() == ThreadState::DEAD) {
        return ER_DEAD_THREAD;
    }
    {
        ScopedMutexLock lock(privateDataLock);
        this->alertCode = alertCode;
    }
    QCC_DbgTrace(("Thread::Alert(%u) [%s:%srunning]", alertCode, funcName, IsRunning() ? " " : " not "));
    return stopEvent.SetEvent();
}

void Thread::AddAuxListener(ThreadListener* listener)
{
    ScopedMutexLock lock(auxListenersLock);
    auxListeners.insert(listener);
}

void Thread::RemoveAuxListener(ThreadListener* listener)
{
    ScopedMutexLock lock(auxListenersLock);
    ThreadListeners::iterator it = auxListeners.find(listener);
    if (it != auxListeners.end()) {
        auxListeners.erase(it);
    }
}

QStatus Thread::Join(void)
{
    QStatus status = ER_OK;

    QCC_DbgTrace(("Thread::Join() [%s - %x :%srunning]", funcName, handle, IsRunning() ? " " : " not "));

    QCC_DbgPrintf(("[%s - %x] Joining thread [%s - %x]",
                   GetThread()->funcName, GetThread()->handle,
                   funcName, handle));

    ThreadState::Rc stateRc = threadState.join();
    if (stateRc == ThreadState::Ok) {
        ThreadHandle currentThreadHandle;
        {
            ScopedMutexLock lock(privateDataLock);
            currentThreadHandle = handle;
        }
        QCC_ASSERT(currentThreadHandle);
        /* Threads that join themselves must detach without blocking */
        if (currentThreadHandle == pthread_self()) {
            int ret = pthread_detach(currentThreadHandle);
            if (ret != 0) {
                status = ER_OS_ERROR;
                QCC_LogError(status, ("Detaching thread: %d - %s", ret, strerror(ret)));
            }
        } else {
            int ret = pthread_join(currentThreadHandle, NULL);
            if (ret != 0) {
                status = ER_OS_ERROR;
                QCC_LogError(status, ("Joining thread: %d - %s", ret, strerror(ret)));
            }
        }

        QCC_DbgPrintf(("Joined thread %s", funcName));

        QCC_DEBUG_ONLY(IncrementAndFetch(&joined));
        {
            ScopedMutexLock lock(privateDataLock);
            handle = 0;
        }
        /* once the state is changed to JOINED/DEAD, we must not touch any member of this class anymore */
        threadState.joined();
    }

    return status;
}

ThreadReturn STDCALL Thread::Run(void* arg)
{
    QCC_ASSERT(NULL != function);
    QCC_ASSERT(!threadState.isExternal());
    QCC_DbgTrace(("Thread::Run() [%s:%srunning]", funcName, IsRunning() ? " " : " not "));
    return (*function)(arg);
}

ThreadId Thread::GetCurrentThreadId()
{
    return pthread_self();
}

bool Thread::IsStopping(void)
{
    ThreadState::State currentState = threadState.getCurrentState();
    return (currentState == ThreadState::STOPPING ||
            currentState == ThreadState::STOPPED  ||
            currentState == ThreadState::JOINING  ||
            currentState == ThreadState::EXTERNALJOINING);
}

ThreadReturn Thread::GetExitValue(void)
{
    ScopedMutexLock lock(privateDataLock);
    return exitValue;
}

bool Thread::IsRunning(void)
{
    ThreadState::State currentState = threadState.getCurrentState();
    return (currentState == ThreadState::RUNNING ||
            currentState == ThreadState::STOPPING ||
            currentState == ThreadState::EXTERNAL ||
            currentState == ThreadState::EXTERNALJOINING);
}

const char* Thread::GetName(void) const
{
    ScopedMutexLock lock(privateDataLock);
    return funcName;
}

ThreadHandle Thread::GetHandle(void)
{
    ScopedMutexLock lock(privateDataLock);
    return handle;
}

uint32_t Thread::GetAlertCode() const
{
    ScopedMutexLock lock(privateDataLock);
    return alertCode;
}

void Thread::ResetAlertCode()
{
    ScopedMutexLock lock(privateDataLock);
    alertCode = 0;
}

} /* namespace */
