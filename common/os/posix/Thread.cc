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

#include <Status.h>

using namespace std;

/** @internal */
#define QCC_MODULE "THREAD"

namespace qcc {

#ifndef NDEBUG
static volatile int32_t started = 0;
static volatile int32_t running = 0;
static volatile int32_t joined = 0;
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
    threadListLock->Lock(MUTEX_CONTEXT);
    map<ThreadId, Thread*>::iterator it = threadList->find(thread->handle);
    if (it != threadList->end()) {
        if (it->second->isExternal) {
            delete it->second;
            threadList->erase(it);
        }
    }
    threadListLock->Unlock(MUTEX_CONTEXT);
}

QStatus Thread::Init()
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

QStatus Thread::Shutdown()
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
        delete Thread::threadList;
        delete Thread::threadListLock;
        initialized = false;
    }
    return ER_OK;
}

QStatus Sleep(uint32_t ms) {
    usleep(1000 * ms);
    return ER_OK;
}

Thread* Thread::GetThread()
{
    Thread* ret = NULL;

    /* Find thread on Thread::threadList */
    threadListLock->Lock(MUTEX_CONTEXT);
    map<ThreadHandle, Thread*>::const_iterator iter = threadList->find(pthread_self());
    if (iter != threadList->end()) {
        ret = iter->second;
    }
    threadListLock->Unlock(MUTEX_CONTEXT);

    /* If the current thread isn't on the list, then create an external (wrapper) thread */
    if (NULL == ret) {
        ret = new Thread("external", NULL, true);
    }

    return ret;
}

const char* Thread::GetThreadName()
{
    Thread* thread = NULL;

    /* Find thread on Thread::threadList */
    threadListLock->Lock(MUTEX_CONTEXT);
    map<ThreadId, Thread*>::const_iterator iter = threadList->find(pthread_self());
    if (iter != threadList->end()) {
        thread = iter->second;
    }
    threadListLock->Unlock(MUTEX_CONTEXT);

    /* If the current thread isn't on the list, then don't create an external (wrapper) thread */
    if (thread == NULL) {
        return "external";
    }

    return thread->GetName();
}

void Thread::CleanExternalThreads()
{
    threadListLock->Lock(MUTEX_CONTEXT);
    map<ThreadId, Thread*>::iterator it = threadList->begin();
    while (it != threadList->end()) {
        if (it->second->isExternal) {
            delete it->second;
            threadList->erase(it++);
        } else {
            ++it;
        }
    }
    threadListLock->Unlock(MUTEX_CONTEXT);
}

Thread::Thread(qcc::String name, Thread::ThreadFunction func, bool isExternal) :
    stopEvent(),
    state(isExternal ? RUNNING : INITIAL),
    isStopping(false),
    function(isExternal ? NULL : func),
    handle(isExternal ? pthread_self() : 0),
    exitValue(NULL),
    threadArg(NULL),
    threadListener(NULL),
    isExternal(isExternal),
    platformContext(NULL),
    alertCode(0),
    auxListeners(),
    auxListenersLock(LOCK_LEVEL_THREAD_AUXLISTENERSLOCK),
    waitCount(0),
    waitLock(LOCK_LEVEL_THREAD_WAITLOCK),
    hasBeenJoined(false)
    hbjMutex(LOCK_LEVEL_THREAD_HBJMUTEX),
{
    /* qcc::String is not thread safe.  Don't use it here. */
    funcName[0] = '\0';
    strncpy(funcName, name.c_str(), sizeof(funcName));
    funcName[sizeof(funcName) - 1] = '\0';

    /* If this is an external thread, add it to the thread list here since Run will not be called */
    if (isExternal) {
        QCC_ASSERT(func == NULL);
        threadListLock->Lock(MUTEX_CONTEXT);
        (*threadList)[handle] = this;
        if (pthread_getspecific(cleanExternalThreadKey) == NULL) {
            int ret = pthread_setspecific(cleanExternalThreadKey, this);
            if (ret != 0) {
                QCC_LogError(ER_OS_ERROR, ("Setting TLS key: %s", strerror(ret)));
            }
            QCC_ASSERT(ret == 0);
        }
        threadListLock->Unlock(MUTEX_CONTEXT);
    }
    QCC_DbgHLPrintf(("Thread::Thread() created %s - %x -- started:%d running:%d joined:%d", funcName, handle, started, running, joined));
}


Thread::~Thread(void)
{
    QCC_DbgHLPrintf(("Thread::~Thread() destroying %s - %x", funcName, handle));

    if (!isExternal) {
        Stop();
        Join();
    }

    /* Keep object alive until waitCount goes to zero */
    while (waitCount) {
        qcc::Sleep(2);
    }

    QCC_DbgHLPrintf(("Thread::~Thread() destroyed %s - %x -- started:%d running:%d joined:%d", funcName, handle, started, running, joined));
}


ThreadInternalReturn Thread::RunInternal(void* arg)
{
    Thread* thread(reinterpret_cast<Thread*>(arg));
    sigset_t newmask;

    sigemptyset(&newmask);
    sigaddset(&newmask, SIGUSR1);

    QCC_ASSERT(thread != NULL);

    /* Plug race condition between Start and Run. (pthread_create may not write handle before run is called) */
    thread->handle = pthread_self();

    if (thread->state != STARTED) {
        return NULL;
    }

    QCC_DEBUG_ONLY(IncrementAndFetch(&started));

    QCC_DbgPrintf(("Thread::RunInternal: %s (pid=%x)", thread->funcName, (unsigned long) thread->handle));

    /* Add this Thread to list of running threads */
    threadListLock->Lock(MUTEX_CONTEXT);
    (*threadList)[thread->handle] = thread;
    thread->state = RUNNING;
    pthread_sigmask(SIG_UNBLOCK, &newmask, NULL);
    threadListLock->Unlock(MUTEX_CONTEXT);

    /* Start the thread if it hasn't been stopped */
    if (!thread->isStopping) {
        QCC_DbgPrintf(("Starting thread: %s", thread->funcName));
        QCC_DEBUG_ONLY(IncrementAndFetch(&running));
        thread->exitValue = thread->Run(thread->threadArg);
        QCC_DEBUG_ONLY(DecrementAndFetch(&running));
        QCC_DbgPrintf(("Thread function exited: %s --> %p", thread->funcName, thread->exitValue));
    }

    thread->state = STOPPING;
    thread->stopEvent.ResetEvent();

    /*
     * Call thread exit callback if specified. Note that ThreadExit may dellocate the thread so the
     * members of thread may not be accessed after this call
     */
    void* retVal = thread->exitValue;
    ThreadHandle handle = thread->handle;


    /* Call aux listeners before main listener since main listner may delete the thread */
    thread->auxListenersLock.Lock(MUTEX_CONTEXT);
    ThreadListeners::iterator it = thread->auxListeners.begin();
    while (it != thread->auxListeners.end()) {
        ThreadListener* listener = *it;
        listener->ThreadExit(thread);
        it = thread->auxListeners.upper_bound(listener);
    }
    thread->auxListenersLock.Unlock(MUTEX_CONTEXT);

    if (thread->threadListener) {
        thread->threadListener->ThreadExit(thread);
    }

    /* This also means no QCC_DbgPrintf as they try to get context on the current thread */

    /* Remove this Thread from list of running threads */
    threadListLock->Lock(MUTEX_CONTEXT);
    threadList->erase(handle);
    threadListLock->Unlock(MUTEX_CONTEXT);

    return reinterpret_cast<ThreadInternalReturn>(retVal);
}

static const uint32_t stacksize = 256 * 1024;

QStatus Thread::Start(void* arg, ThreadListener* listener)
{
    QStatus status = ER_OK;

    /* Check that thread can be started */
    if (isExternal) {
        status = ER_EXTERNAL_THREAD;
    } else if (isStopping) {
        status = ER_THREAD_STOPPING;
    } else if (IsRunning()) {
        status = ER_THREAD_RUNNING;
    }

    if (status != ER_OK) {
        QCC_LogError(status, ("Thread::Start [%s]", funcName));
    } else {
        int ret;

        /* Clear/initialize the join context */
        hasBeenJoined = false;
        waitCount = 0;

        /*  Reset the stop event so the thread doesn't start out alerted. */
        stopEvent.ResetEvent();
        /* Create OS thread */
        this->threadArg = arg;
        this->threadListener = listener;

        state = STARTED;
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
        ret = pthread_create(&handle, &attr, RunInternal, this);
        QCC_DbgTrace(("Thread::Start() [%s] pid = %x", funcName, handle));
        if (ret != 0) {
            state = DEAD;
            isStopping = false;
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Creating thread %s: %s", funcName, strerror(ret)));
        }
        pthread_attr_destroy(&attr);
    }
    return status;
}

QStatus Thread::Stop(void)
{
    /* Cannot stop external threads */
    if (isExternal) {
        QCC_LogError(ER_EXTERNAL_THREAD, ("Cannot stop an external thread"));
        return ER_EXTERNAL_THREAD;
    } else if ((state == DEAD) || (state == INITIAL)) {
        QCC_DbgPrintf(("Thread::Stop() thread is dead [%s]", funcName));
        return ER_OK;
    } else {
        QCC_DbgTrace(("Thread::Stop() %x [%s]", handle, funcName));
        isStopping = true;
        return stopEvent.SetEvent();
    }
}

QStatus Thread::Alert()
{
    if (state == DEAD) {
        return ER_DEAD_THREAD;
    }
    QCC_DbgTrace(("Thread::Alert() [%s:%srunning]", funcName, IsRunning() ? " " : " not "));
    return stopEvent.SetEvent();
}

QStatus Thread::Alert(uint32_t alertCode)
{
    this->alertCode = alertCode;
    if (state == DEAD) {
        return ER_DEAD_THREAD;
    }
    QCC_DbgTrace(("Thread::Alert(%u) [%s:%srunning]", alertCode, funcName, IsRunning() ? " " : " not "));
    return stopEvent.SetEvent();
}

void Thread::AddAuxListener(ThreadListener* listener)
{
    auxListenersLock.Lock(MUTEX_CONTEXT);
    auxListeners.insert(listener);
    auxListenersLock.Unlock(MUTEX_CONTEXT);
}

void Thread::RemoveAuxListener(ThreadListener* listener)
{
    auxListenersLock.Lock(MUTEX_CONTEXT);
    ThreadListeners::iterator it = auxListeners.find(listener);
    if (it != auxListeners.end()) {
        auxListeners.erase(it);
    }
    auxListenersLock.Unlock(MUTEX_CONTEXT);
}

QStatus Thread::Join(void)
{
    QStatus status = ER_OK;

    QCC_DbgTrace(("Thread::Join() [%s - %x :%srunning]", funcName, handle, IsRunning() ? " " : " not "));

    QCC_DbgPrintf(("[%s - %x] Joining thread [%s - %x]",
                   GetThread()->funcName, GetThread()->handle,
                   funcName, handle));

    /*
     * Nothing to join if the thread is dead
     */
    if (state == DEAD) {
        QCC_DbgPrintf(("Thread::Join() thread is dead [%s]", funcName));
        isStopping = false;
        return ER_OK;
    }
    /*
     * There is a race condition where the underlying OS thread has not yet started to run. We need
     * to wait until the thread is actually running before we can free it.
     */
    while (state == STARTED) {
        usleep(1000 * 5);
    }

    /* Threads that join themselves must detach without blocking */
    if (handle == pthread_self()) {
        int32_t waiters = IncrementAndFetch(&waitCount);
        hbjMutex.Lock(MUTEX_CONTEXT);
        if ((waiters == 1) && !hasBeenJoined) {
            hasBeenJoined = true;
            hbjMutex.Unlock(MUTEX_CONTEXT);
            int ret = 0;
            if (state != INITIAL) {
                QCC_ASSERT(handle);
                ret = pthread_detach(handle);
            }
            if (ret == 0) {
                QCC_DEBUG_ONLY(IncrementAndFetch(&joined));
            } else {
                status = ER_OS_ERROR;
                QCC_LogError(status, ("Detaching thread: %d - %s", ret, strerror(ret)));
            }
            handle = 0;
        } else {
            hbjMutex.Unlock(MUTEX_CONTEXT);

        }
        DecrementAndFetch(&waitCount);
        isStopping = false;
    } else {
        /*
         * Unfortunately, POSIX pthread_join can only be called once for a given thread. This is quite
         * inconvenient in a system of multiple threads that need to synchronize with each other.
         * This ugly looking code allows multiple threads to Join a thread. All but one thread block
         * in a Mutex. The first thread to obtain the mutex is the one that is allowed to call pthread_join.
         * All other threads wait for that join to complete. Then they are released.
         */
        int ret = 0;
        int32_t waiters = IncrementAndFetch(&waitCount);
        waitLock.Lock(MUTEX_CONTEXT);
        hbjMutex.Lock(MUTEX_CONTEXT);
        if ((waiters == 1) && !hasBeenJoined) {
            hasBeenJoined = true;
            hbjMutex.Unlock(MUTEX_CONTEXT);
            if (state != INITIAL) {
                QCC_ASSERT(handle);
                ret = pthread_join(handle, NULL);
            }
            handle = 0;
            QCC_DEBUG_ONLY(IncrementAndFetch(&joined));
        } else {
            hbjMutex.Unlock(MUTEX_CONTEXT);

        }
        waitLock.Unlock(MUTEX_CONTEXT);
        DecrementAndFetch(&waitCount);

        if (ret != 0) {
            status = ER_OS_ERROR;
            QCC_LogError(status, ("Joining thread: %d - %s", ret, strerror(ret)));
        }
        isStopping = false;
    }
    QCC_DbgPrintf(("Joined thread %s", funcName));
    /* once the state is changed to DEAD, we must not touch any member of this class anymore */
    state = DEAD;
    return status;
}

ThreadReturn STDCALL Thread::Run(void* arg)
{
    QCC_DbgTrace(("Thread::Run() [%s:%srunning]", funcName, IsRunning() ? " " : " not "));
    QCC_ASSERT(NULL != function);
    QCC_ASSERT(!isExternal);
    return (*function)(arg);
}

ThreadId Thread::GetCurrentThreadId()
{
    return pthread_self();
}

} /* namespace */
