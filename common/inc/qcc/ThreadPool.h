/**
 * @file
 *
 * Simple ThreadPool using Timers and Alarms
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

#ifndef _QCC_THREADPOOL_H
#define _QCC_THREADPOOL_H

#include <map>

#include <qcc/Timer.h>
#include <qcc/Event.h>
#include <qcc/Ptr.h>

namespace qcc {

class ThreadPool;

/**
 * A class in the spirit of the Java Runnable object that is used to define an
 * object which is executable by a ThreadPool.
 *
 * In order to ask the ThreadPool to execute a task, inherit from the Runnable
 * class and provide a Run() method.
 *
 * Typically one extends the Runnable object with member data corresponding to
 * the deferred variables needed by the Run() method.  This kind of object is
 * called a closure.  Since the data must be kept outside of the context of the
 * function or method that asks for execution of the Runnable, it must be kept
 * on the heap.
 *
 * The AllJoyn common OS abstractions library provides a ManagedObj (similar to
 * smart pointers but using references) to help with this kind of need, but it
 * has problems if you want to cast from one kind of ManagedObj to another (see
 * the single parameter constructor and explicit call to the destructor of the
 * enclosed type.  Since we need the cast behavior, we use the Ptr intrusive
 * smart pointer class to manage our runnable closures.
 */
class Runnable : public qcc::RefCountBase, public qcc::AlarmListener {
  public:

    /**
     * Construct a Runnable object suitable for use by a ThreadPool
     */
    Runnable() : m_threadpool(NULL) { }

    /**
     * Destroy a Runnable object.
     */
    virtual ~Runnable() { m_threadpool = NULL; }

    /**
     * This method is called by the ThreadPool when the Runnable object is
     * dispatched to a thread.  A client of the thread pool is expected to
     * define a method in a derived class that does useful work when Run() is
     * called.
     */
    virtual void Run(void) { };

  private:
    /**
     * ThreadPool must be a friend in order to make AlarmTriggered an accessible
     * base class method of a user's Runnable, and to allow the ThreadPool to
     * set a pointer to itself here to allow for notifications.
     */
    friend class ThreadPool;

    /**
     * AlarmTriggered is the method that is called to dispatch an alarm.
     * Our Run() methods are driven by alarm expirations that happen to
     * always occur immediately.
     *
     * Note that the enclosing runnable object is automatically deleted
     * afte the Run method is executed.
     */
    virtual void AlarmTriggered(const Alarm& alarm, QStatus reason);

    /**
     * Private method used by the thread pool to tell this object how to contact
     * the thread pool when the reference to the runnable closure is no longer
     * needed.
     */
    void SetThreadPool(ThreadPool* threadpool)
    {
        m_threadpool = threadpool;
    }

    /**
     * A reference back to the thread pool that causes the run method to be
     * called.  We need this reference so the thread pool can hook a "done"
     * event since it would otherwise be unaware of the execution of the
     * runderlying timer alarm callback that it arranged to be called.
     */
    ThreadPool* m_threadpool;
};

/**
 * A class in the spirit of the Java ThreadPoolExecutor object that is used
 * to provide a simple way to execute tasks in the context of a separate
 * thread.
 *
 * In order to ask a ThreadPool to execute a task, one must inherit from the
 * Runnable class and provide a Run() method.
 */
class ThreadPool {
  public:
    /**
     * Construct a thread pool with a given name and pool size.
     *
     * @param name     The name of the thread pool (used in logging).
     * @param poolsize The number of threads available in the pool.
     */
    ThreadPool(const char* name, uint32_t poolsize);

    /**
     * Destroy a thread pool.
     */
    virtual ~ThreadPool();

    /**
     * Request that we cancel all of our dispatched threads.
     */
    QStatus Stop();

    /**
     * Wait for all of the threads in our associated timer to exit.  Once
     * this happens, it is safe for us to finish tearing down our object.
     * Note that this call can block or a time limited only by the execution
     * time of the threads dispatched.
     */
    QStatus Join();

    /**
     * Determine the underlying concurrency of the thread pool.
     *
     * Convenience function to get the number of threads available in the thread pool.
     * This is different than GetN() which returns the number of threads that are
     * currently executing or waiting to execute a closure.
     *
     * @return The number of concurrently execution tasks possible in this thread pool.
     */
    uint32_t GetConcurrency(void)
    {
        return m_poolsize;
    }

    /**
     * Determine how many Runnable tasks are pending on the thread pool.
     *
     * Convenience function to get the number of threads currently executing or
     * waiting to execute.  This is different than GetConcurrency() which
     * returns the number of threads in the thread pool (which may or may not be
     * currently executing or waiting to execute.
     *
     * @return The number of Runnable closures which represent either pending
     *         execution or are currently executing runnable tasks.
     */
    uint32_t GetN(void);

    /**
     * Execute a Runnable task one one of the threads of the thread pool.
     *
     * The execute method takes a pointer to a Runnable object.  This object
     * acts as a closure which essentialy is a pre-packaged function call.  The
     * function is called Run() in this case.  Since we need to keep the package
     * around until Run() actually executes, and the thread that calls Execute()
     * may be long gone by the time this happens, the Runnable must be allocated
     * on the heap.  Only the thread pool knows when the Runnable is no longer
     * needed, so it takes responsibility for managing the memory of the runnable
     * when Execute is called.
     *
     * Each call to Execute must provide a pointer to a unique Runnable
     *
     * @param runnable A Ptr (smart pointer) to a Runnable Object providing the
     *                 Run() method which one of the threads in this thread pool
     *                 will execute.
     *
     * @return
     *      - #ER_OK if the execute request was successful
     *      - #ER_THREADPOOL_EXHAUSTED if the thread pool has previously reached its specified concurrency.
     */
    QStatus Execute(Ptr<Runnable> runnable);

    /**
     * Wait for a thread to become available for use.
     *
     * Since AllJoyn is at its heart a distributed network application, and what
     * drives the execution of our threads will ultimately be network traffic,
     * we need to be able to apply backpressure to the network to avoid
     * exhausting all available resources.  We do this by providing a method which
     * allows a caller to put itself to sleep until a thread beomes available.
     *
     * If the caller thread happens to be the thread that is reading messages off
     * of the network, it will put itself to sleep.  This will stop the network
     * destination from pulling bits out of its queue and will result in, for example,
     * the TCP receive queue filling.  This will, in turn, apply backpressure to the
     * sender which will rate limit the system.  This prevents unbounded resource
     * allocation of corresponding threads and memory.
     *
     * @return ER_OK when a thread becomes available, or a general error if one happens.
     */
    QStatus WaitForAvailableThread(void);

  private:
    /**
     * Runnable must be a friend in order to call back and tell us that it has
     * finished executing.  Since this affects the memory management contract
     * between the Runnable and ThreadPool objects, we don't let anyone else
     * but Runnable make this call.
     */
    friend class Runnable;

    /**
     * Assignment operator is private - ThreadPools cannot be assigned.
     */
    ThreadPool& operator=(const ThreadPool& other);

    /**
     * Copy constructor is private - ThreadPools cannot be copied.
     */
    ThreadPool(const ThreadPool& other);

    /**
     * A flag to remind if the thread pool is stopping or stopped.
     */
    bool m_stopping;

    /**
     * A mutex to protect the underlying data structures.
     */
    qcc::Mutex m_lock;

    /**
     * An event to allow callers to wait until a thread becomes available
     * in the pool.
     */
    qcc::Event m_event;

    /**
     * The maximum number of concurrent threads executing in this thread pool.
     */
    uint32_t m_poolsize;

    /**
     * We need to hold a reference to an underlying Runnable object while we
     * wait for its Run() method to be executed.  The Runnable object acts as a
     * closure that a user will use to keep required variables.  When the Run()
     * method is done executing, we release the reference to the runnable managed
     * object by deleting the corresponding entry in the closure map.
     */
    typedef std::map<Runnable*, Ptr<Runnable> > RunnableEntry;
    RunnableEntry m_closures;

    /**
     * When the AlarmTriggered callback of the AlarmListener base class of the
     * Runnable is called by the Timer, it will call the user's Run() function
     * and then call back here to let us know that it has finished executing.
     * That tells us that we can clean up any resources we may be holding.  That
     * just means releasing our reference to the Ptr that holds the ulitmate
     * reference to the runnable.  If we can't find a closure, we have screwed
     * up massively and we should just die.
     */
    void Release(Runnable* runnable);

    /**
     * The timer that actually does all of the work of dispatching threads and
     * essentially doing all of the work of the thread pool.
     */
    qcc::Timer m_dispatcher;
};

} // namespace qcc

#endif
