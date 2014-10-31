/******************************************************************************
 *
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#include <deque>
#include <list>
#include <algorithm>

#include <qcc/Condition.h>
#include <qcc/Thread.h>

#include <Status.h>

using namespace qcc;

TEST(ConditionTest, construction_destruction)
{
    /*
     * Make sure we can construct a Condition Variable on the heap and destroy
     * it when we go out of scope without blowing up.
     */
    Condition c;
}

TEST(ConditionTest, signal)
{
    /*
     * Make sure we can construct a Condition Variable on the heap and call Signal
     * on it without blowing up.
     */
    Condition c;
    QStatus status = c.Signal();
    ASSERT_EQ(ER_OK, status);
}

TEST(ConditionTest, broadcast)
{
    /*
     * Make sure we can construct a Condition Variable on the heap and call Broadcast
     * on it without blowing up.
     */
    Condition c;
    QStatus status = c.Broadcast();
    ASSERT_EQ(ER_OK, status);
}

/*
 * The Condition variable was invented to solve the bounded buffer problem;
 * so this is the canonical use case.  We should probably make sure that use
 * case actually works.  The idea is that there is a finite buffer with a
 * producer that adds stuff to the buffer until it is full and then it blocks;
 * and there also a consumer that takes  stuff off of the buffer until it is
 * empty and then it blocks.
 *
 * We'll dissect this use case and make sure that it works for various sequences
 * of events and number of threads.  Admittedly, we'll mostly be testing the OS
 * implementations of the underlying mechansims, but we will make sure that
 * everything is working the same and the way we expect.
 *
 * First, we set up the basic protected buffer which is a deque as we might
 * expect it to be in real implementations.  We have nothing to prove there, so
 * it is just a deque of integers, the values of which we can test to make sure
 * things are coming and going properly
 */
std::deque<int32_t> prot;

/*
 * Since the telling cases are zero, one and more than one, we select two as the
 * maximum depth of the buffer.
 */
const uint32_t B_MAX = 2;

/*
 * We are going to define a function called Produce() that puts a thing on the
 * protected buffer; and a function Consume() that takes things off of the
 * buffer.  Both of these functions need to coordinate their work, so there will
 * be one Mutex that they both use.  By definition, the producer needs to block
 * if the buffer is full and the consumer needs to block if the buffer is empty;
 * so there are two Condition variables (of the same name) in play along with
 * the one Mutex,
 */
void Produce(qcc::Condition& empty, qcc::Condition& full, qcc::Mutex& m, uint32_t thing)
{
    m.Lock();
    while (prot.size() == B_MAX) {
        full.Wait(m);
    }
    prot.push_back(thing);
    empty.Signal();
    m.Unlock();
}

/*
 * We define a function called consume() that takes a thing off of the protected
 * buffer.
 */
uint32_t Consume(qcc::Condition& empty, qcc::Condition& full, qcc::Mutex& m)
{
    m.Lock();
    while (prot.size() == 0) {
        empty.Wait(m);
    }
    uint32_t thing = prot.front();
    prot.pop_front();
    full.Signal();
    m.Unlock();
    return thing;
}

/*
 * We have a protected buffer (prot) that is the buffer protected by the
 * condition variables and mutex.  We are going to flow data through the
 * protected queue and into a test data buffer which we declare here.
 */
std::deque<int32_t> data;

/*
 * Since we have a thread pulling data out of the protected buffer and putting
 * it into the test data buffer; and the gtest main thread will also be looking
 * at that test data, we need to protect it with a mutex -- a Test Lock.
 */
qcc::Mutex tl;

/*
 * We really don't want to introduce arbitrary times during which we hope that a
 * thread will run and execute to a certain point.  If we pick a time that is
 * too short, random unexpected processing delays on the host might cause the
 * tests to fail randomly.  This is bad.  To avoid this, we have generic states
 * that our threads will run through and the main test can examine the states
 * and see directly that something expected or unexpected has happened.  We can
 * introduce very long watchdog timers to catch cases where the test never
 * completes, we can also be very fast when the host is very fast, and we will
 * not be dependent on arbitrary guesses about compromises.
 */
enum GenericState {
    IDLE = 1,
    RUN_ENTERED,
    IN_LOOP,
    CALLING,
    CALLED,
    DONE
};

/*
 * Convert seconds to milliseconds by multiplying the milli count by SEC
 */
const uint32_t SEC = 1000;

/*
 * The basic resolution of the timer waits is two milliseconds.
 */
const uint32_t TICK = 2;

/*
 * When we are waiting for another thread to do something, we need to make sure
 * we don't wait forever.  This is the limit for the test.  If the host can't
 * spin up a thread and so something simple in 60 seconds, we are broken.  Note
 * that WATCHDOG must be a multiple of TICK.
 */
const uint32_t WATCHDOG = 60 * SEC;

/*
 * We need more than one thread, so we provide a consumer thread to pull data
 * out of the protected buffer and stick it in the test data buffer.  For the
 * simple tests, the gtest main thread will serve as the producer thread.
 *
 * Note that the thread checks for a done bit at the end of its main loop, so it
 * will execute Consume() at least once.
 */
class ConsumerThread : public qcc::Thread {
  public:
    ConsumerThread(Condition* e, Condition* f, Mutex* m)
        : qcc::Thread(qcc::String("C")), m_e(e), m_f(f), m_m(m), m_done(false), m_state(IDLE), m_loops(0) { }
    void Done() { m_done = true; }
    GenericState GetState() { return m_state; }
    uint32_t GetLoops() { return m_loops; }
  protected:
    qcc::ThreadReturn STDCALL Run(void* arg)
    {
        m_state = RUN_ENTERED;
        do {
            m_state = IN_LOOP;

            /*
             * Consume something from the protected buffer.
             */
            m_state = CALLING;
            int thing = Consume(*m_e, *m_f, *m_m);
            m_state = CALLED;

            /*
             * And put that something on the gtest data buffer.
             */
            tl.Lock();
            data.push_back(thing);
            tl.Unlock();

            ++m_loops;
        } while (m_done == false);
        m_state = DONE;
        return 0;
    }
  private:
    Condition* m_e;
    Condition* m_f;
    Mutex* m_m;
    bool m_done;
    GenericState m_state;
    uint32_t m_loops;
};

TEST(ConditionTest, simple_empty_protected_buffer)
{
    /*
     * Create a condition variable corresponding to an empty buffer which
     * the consumer waits on and the producer signals when it puts something
     * in the buffer.
     */
    Condition emtpy;

    /*
     * Create a condition variable corresponding to a full buffer which the
     * producer waits on and the consumer signals when it pulls something off
     * the buffer.
     */
    Condition full;

    /*
     * Create a mutex to protect the shared buffer.
     */
    Mutex m;

    /*
     * Create a consumer thread to pull data out of the protected buffer.
     */
    ConsumerThread consumer(&emtpy, &full, &m);

    /*
     * clear the protected buffer and the test data buffer before starting.
     */
    prot.clear();
    data.clear();

    /*
     * Set the done bit on the consumer thread so it only executes one Consume
     * operation and then quits.
     */
    consumer.Done();

    /*
     * Start the consumer thread.  We expect that it will begin running and
     * notice that there is nothing in the protected buffer and block waiting
     * for something to appear.
     */
    consumer.Start();

    /*
     * We assume below that WATCHDOG is an even multiple of TICK.  Make sure we
     * test that assumption explicitly instead of letting someone figure it out
     * the hard way.
     */
    assert(WATCHDOG % TICK == 0 && "WATCHDOG must be an even multiple of TICK");

    /*
     * Wait for the consumer thread to actually run and block, then make sure
     * that is in fact what it did by verifying that it hasn't consumed
     * anything.
     */
    for (uint32_t i = 0; i <= WATCHDOG; i += TICK) {
        ASSERT_LT((uint32_t)i, WATCHDOG);
        if (consumer.GetState() == CALLING) {
            break;
        }
        qcc::Sleep(TICK);
    }

    /*
     * We can tell that the consumer thread has not actually consumed anything
     * by asking it how many times it has completed its work loop.  Zero means
     * it has blocked and not returned in its call to Consume().
     */
    uint32_t loops = consumer.GetLoops();
    ASSERT_EQ((uint32_t)0, loops);

    tl.Lock();
    uint32_t consumed = data.size();
    tl.Unlock();
    ASSERT_EQ((uint32_t)0, consumed);

    /*
     * Now produce one "thing" -- an integer with a particular value.
     */
    Produce(emtpy, full, m, 0xaffab1e);

    /*
     * Since we have a consumer thread running, after the producer puts that
     * integer on the protected buffer, the consumer should be awakened and pull
     * the integer off the protected buffer and stick it on the test data
     * buffer.  The consumer thread should have exited since its done bit was
     * set.
     */
    for (uint32_t i = 0; i <= WATCHDOG; i += TICK) {
        ASSERT_LT((uint32_t)i, WATCHDOG);
        if (consumer.GetState() == DONE) {
            break;
        }
        qcc::Sleep(TICK);
    }

    /*
     * We still need to play by the AllJoyn Thread rules and run through the
     * rest of the life cycle of the thread even though we know it exited.
     */
    consumer.Stop();
    consumer.Join();

    /*
     * We should now find exactly one "thing" on the test data buffer.
     */
    consumed = data.size();
    ASSERT_EQ((uint32_t)1, consumed);

    /*
     * That one thing should be an integer of the same value as the single integer
     * we Produced earlier.
     */
    int32_t thing = data.front();
    data.pop_front();
    ASSERT_EQ((int32_t)0xaffab1e, thing);
}

TEST(ConditionTest, simple_full_protected_buffer)
{
    /*
     * Create a condition variable corresponding to an empty buffer which
     * the consumer waits on and the producer signals when it puts something
     * in the buffer.
     */
    Condition emtpy;

    /*
     * Create a condition variable corresponding to a full buffer which the
     * producer waits on and the consumer signals when it pulls something off
     * the buffer.
     */
    Condition full;

    /*
     * Create a mutex to protect the shared buffer.
     */
    Mutex m;

    /*
     * Create a consumer thread to pull data out of the protected buffer.
     */
    ConsumerThread consumer(&emtpy, &full, &m);

    /*
     * clear the protected buffer and the test data buffer before starting.
     */
    prot.clear();
    data.clear();

    /*
     * Now produce one "thing" -- an integer with a particular value so that it
     * is there waiting when the consumer starts.
     */
    Produce(emtpy, full, m, 0xdefaced);

    /*
     * Start the consumer thread.  We expect that it will begin running and
     * notice that there is something in the protected buffer and move it to the
     * test data buffer and then block as it starts its second loop.  The
     * "signature" for this state is that the consumer is in state CALLING which
     * means it has called Consume() and it has completed one loop (meaning it
     * is in its second loop).
     */
    consumer.Start();
    for (uint32_t i = 0; i <= WATCHDOG; i += TICK) {
        ASSERT_LT((uint32_t)i, WATCHDOG);
        if (consumer.GetState() == CALLING && consumer.GetLoops() == 1) {
            break;
        }
        qcc::Sleep(TICK);
    }

    /*
     * Verify that the consumer thread has consumed exactly one thing.
     */
    tl.Lock();
    uint32_t consumed = data.size();
    tl.Unlock();
    ASSERT_EQ((uint32_t)1, consumed);

    /*
     * That one thing should be an integer of the same value as the single integer
     * we Produced earlier.
     */
    int32_t thing = data.front();
    data.pop_front();
    ASSERT_EQ((int32_t)0xdefaced, thing);

    /*
     * Now clear the test data buffer, set the done bit and produce one more
     * "thing", another integer with another particular value, that should kick
     * start the consumer and cause it to break out of its loop after it
     * consumes the thing.  The DONE state indicates that the consumer thread
     * has exited.
     */
    data.clear();
    consumer.Done();
    Produce(emtpy, full, m, 0xcafebabe);

    for (uint32_t i = 0; i <= WATCHDOG; i += TICK) {
        ASSERT_LT((uint32_t)i, WATCHDOG);
        if (consumer.GetState() == DONE) {
            break;
        }
        qcc::Sleep(TICK);
    }

    /*
     * The consumer thread should have exited since its done bit was set.  We
     * still need to play by the AllJoyn Thread rules and run through the rest
     * of the life cycle.
     */
    consumer.Stop();
    consumer.Join();

    /*
     * We should now find exactly one "thing" on the test data buffer.
     */
    consumed = data.size();
    ASSERT_EQ((uint32_t)1, consumed);

    /*
     * That one thing should be an integer of the same value as the single integer
     * we Produced earlier.
     */
    thing = data.front();
    data.pop_front();
    ASSERT_EQ((int32_t)0xcafebabe, thing);
}

TEST(ConditionTest, throughput_protected_buffer)
{
    /*
     * Create a condition variable corresponding to an empty buffer which
     * the consumer waits on and the producer signals when it puts something
     * in the buffer.
     */
    Condition emtpy;

    /*
     * Create a condition variable corresponding to a full buffer which the
     * producer waits on and the consumer signals when it pulls something off
     * the buffer.
     */
    Condition full;

    /*
     * Create a mutex to protect the shared buffer.
     */
    Mutex m;

    /*
     * Create a consumer thread to pull data out of the protected buffer.
     */
    ConsumerThread consumer(&emtpy, &full, &m);

    /*
     * clear the protected buffer and the test data buffer before starting.
     */
    prot.clear();
    data.clear();

    /*
     * Start the consumer thread.  We expect that it will begin running and
     * notice that there is nothing in the protected buffer and block waiting
     * for something to appear.
     */
    consumer.Start();

    /*
     * In a tight loop, produce 100 things.  Although not very deterministic
     * we exercise the limits and blocking of both the producer and consumer
     * through the very limited resource, the protected bound buffer.
     */
    for (uint32_t i = 0; i < 100; ++i) {
        Produce(emtpy, full, m, i);
    }

    /*
     * Since we have a consumer thread running, it should follow the producer
     * and pull the integers off the protected buffer and stick them on the test
     * data buffer.  If it runs through the loop 100 times, it should have
     * pulled out 100 things.
     */
    uint32_t consumed;
    for (uint32_t i = 0; i <= WATCHDOG; i += TICK) {
        ASSERT_LT((uint32_t)i, WATCHDOG);
        if (consumer.GetLoops() == 100) {
            break;
        }
        qcc::Sleep(TICK);
    }

    /*
     * We should now find exactly one hundred "things" in the test data buffer.
     */
    consumed = data.size();
    ASSERT_EQ((uint32_t)100, consumed);

    /*
     * Those things should be the integers in the same counting pattern we used
     * when we stuck them on.
     */
    for (uint32_t i = 0; i < 100; ++i) {
        int32_t thing = data.front();
        data.pop_front();
        ASSERT_EQ((int32_t)i, thing);
    }

    /*
     * The consumer thread is now blocked waiting for something to be produced.
     * We set the Done bit and produce something to kick start it so it will
     * exit.
     */
    consumer.Done();
    Produce(emtpy, full, m, 0xaccede);

    /*
     * Wait for the consumer thread to actually run and finish.
     */
    for (uint32_t i = 0; i <= WATCHDOG; i += TICK) {
        ASSERT_LT((uint32_t)i, WATCHDOG);
        if (consumer.GetState() == DONE) {
            break;
        }
        qcc::Sleep(TICK);
    }

    /*
     * We still need to play by the AllJoyn Thread rules and run through the
     * rest of the life cycle of the thread even though we know it exited.
     */
    consumer.Stop();
    consumer.Join();

    /*
     * make sure that the thread stopped the way we think it should have
     * (pulling an item off the protected buffer and saving it on the test
     * deque).
     */
    consumed = data.size();
    ASSERT_EQ((uint32_t)1, consumed);

    int32_t thing = data.front();
    data.pop_front();
    ASSERT_EQ((int32_t)0xaccede, thing);
}

/*
 * We need to test multithreaded production so we provide a producer thread to
 * put data on the protected buffer.  The thread will just run for a short time
 * and produce the number of things it was told.
 */
class ProducerThread : public qcc::Thread {
  public:
    ProducerThread(Condition* e, Condition* f, Mutex* m)
        : qcc::Thread(qcc::String("P")), m_e(e), m_f(f), m_m(m), m_state(IDLE), m_loops(0) { }
    void Begin(uint32_t begin) { m_begin = begin; }
    void Count(uint32_t count) { m_count = count; }
    GenericState GetState() { return m_state; }
    uint32_t GetLoops() { return m_loops; }
  protected:
    qcc::ThreadReturn STDCALL Run(void* arg)
    {
        m_state = RUN_ENTERED;
        for (uint32_t i = 0; i < m_count; ++i) {
            m_state = IN_LOOP;
            /*
             * Produce something for the protected buffer.  We can tell which
             * thread it was from if we examine the offset of the count.
             */
            m_state = CALLING;
            Produce(*m_e, *m_f, *m_m, i + m_begin);
            m_state = CALLED;

            ++m_loops;
        }
        m_state = DONE;
        return 0;
    }
  private:
    Condition* m_e;
    Condition* m_f;
    Mutex* m_m;
    uint32_t m_begin;
    uint32_t m_count;
    GenericState m_state;
    uint32_t m_loops;
};

TEST(ConditionTest, multithread_throughput)
{
    /*
     * Create a condition variable corresponding to an empty buffer which
     * the consumer waits on and the producer signals when it puts something
     * in the buffer.
     */
    Condition emtpy;

    /*
     * Create a condition variable corresponding to a full buffer which the
     * producer waits on and the consumer signals when it pulls something off
     * the buffer.
     */
    Condition full;

    /*
     * Create a mutex to protect the shared buffer.
     */
    Mutex m;

    /*
     * Create two consumer threads to pull data out of the protected buffer.
     */
    ConsumerThread consumer1(&emtpy, &full, &m);
    ConsumerThread consumer2(&emtpy, &full, &m);

    /*
     * clear the protected buffer and the test data buffer before starting.
     */
    prot.clear();
    data.clear();

    /*
     * Start the consumer threads.
     */
    consumer1.Start();
    consumer2.Start();

    /*
     * Create two producer threads to put data into the protected buffer.  The
     * first producer will produce integers from 1000 to 1099 and the second
     * will produce integers from 2000 to 2099.
     */
    ProducerThread producer1(&emtpy, &full, &m);
    producer1.Begin(1000);
    producer1.Count(100);
    ProducerThread producer2(&emtpy, &full, &m);
    producer2.Begin(2000);
    producer2.Count(100);

    /*
     * Release the Kraken ... er, start the producers.
     */
    producer1.Start();
    producer2.Start();

    /*
     * Since we have consumer threads running, they should follow the producers
     * and pull the integers off the protected buffer and stick them on the test
     * data buffer.  We assume that the producers and consumers are working the
     * way we expect since they should have passed the previous tests, so we just
     * wait until all 200 things are consumed.
     */
    uint32_t consumed;
    for (uint32_t i = 0; i <= WATCHDOG; i += TICK) {
        ASSERT_LT((uint32_t)i, WATCHDOG);
        tl.Lock();
        consumed = data.size();
        tl.Unlock();
        if (consumed == 200) {
            break;
        }
        qcc::Sleep(TICK);
    }

    /*
     * We assert exactly two hundred "things" in the test data buffer.
     */
    ASSERT_EQ((uint32_t)200, consumed);

    /*
     * Those things will not be in any particular order since multiple threads
     * are pulling and pushing in whatever order they may run.  To verify that
     * what we think should be there is actually there, we need to sort the
     * deque.
     */
    std::sort(data.begin(), data.end());

    /*
     * Now we should see the integers 1000 to 1099 followed by 2000 to 2099 in that
     * order.
     */
    for (uint32_t i = 0; i < 100; ++i) {
        int32_t thing = data.front();
        data.pop_front();
        ASSERT_EQ((int32_t)i + 1000, thing);
    }

    for (uint32_t i = 0; i < 100; ++i) {
        int32_t thing = data.front();
        data.pop_front();
        ASSERT_EQ((int32_t)i + 2000, thing);
    }

    /*
     * The consumer threads are both now blocked waiting for something to be
     * produced.  We set the Done bit on both of them and produce two things for
     * the protected buffer.  The first item produced will be fetched by one of
     * the consumers which will then exit.  The second item will be fetched by
     * the remaining consumer which will then exit as well.  We might as well
     * test the output so clear the data buffer to make it easy to find the
     * resulting items.
     */
    data.clear();
    consumer1.Done();
    consumer2.Done();
    Produce(emtpy, full, m, 0xbabb1e);
    Produce(emtpy, full, m, 0xbabb1e);

    for (uint32_t i = 0; i <= WATCHDOG; i += TICK) {
        ASSERT_LT((uint32_t)i, WATCHDOG);
        if (consumer1.GetState() == DONE && consumer2.GetState() == DONE) {
            break;
        }
        qcc::Sleep(TICK);
    }

    /*
     * We still have to run through the AllJoyn Thread life cycle.
     */
    consumer1.Stop();
    consumer1.Join();
    consumer2.Stop();
    consumer2.Join();

    /*
     * The last two operations should have resuted in two items consumed.
     */
    consumed = data.size();
    ASSERT_EQ((uint32_t)2, consumed);

    int32_t thing = data.front();
    data.pop_front();
    ASSERT_EQ((int32_t)0xbabb1e, thing);
    thing = data.front();
    data.pop_front();
    ASSERT_EQ((int32_t)0xbabb1e, thing);

    /*
     * If all two hundred things have been produced and tested, the producers
     * should have exited, but we don't know that for sure; so we explicity
     * check to make sure they have done what we think they should have done.
     */
    for (uint32_t i = 0; i <= WATCHDOG; i += TICK) {
        ASSERT_LT((uint32_t)i, WATCHDOG);
        if (producer1.GetState() == DONE && producer2.GetState() == DONE) {
            break;
        }
        qcc::Sleep(TICK);
    }

    /*
     * We still have to run through the AllJoyn Thread life cycle.
     */
    producer1.Stop();
    producer1.Join();
    producer2.Stop();
    producer2.Join();
}

/*
 * The second sort of canonical use for Condition variable is the multithreaded
 * memory allocation problem.  This led to the addition of the Broadcast
 * signaling method.  Just as we made sure that the first canonical use case
 * actually worked here in the unit tests, we'll do the same with the second.
 *
 * The idea is that there is a fragmented area of memory and multiple threads
 * are queueing up to allocate memory.  If the first thread to arrive needs to
 * allocate a chunk of memory that is 1000 bytes long, and there is no memory
 * available, that first thread needs to block until 1000 bytes are available.
 * If a second thread arrives looking for a smaller chunk size, say 100 bytes,
 * which is again not available, it needs to block until those 100 bytes become
 * available.
 *
 * What happens if a third thread frees 100 bytes.  If the blocked threads are
 * served in order, the second thread looking for 100 bytes would not be able
 * to run until the first thread looking for 1000 bytes is satisfied.  In order
 * to deal with this problem, all threads need to be awakened and contend again
 * for the resource.
 *
 * Instead of trying to invent an equivalent test which may overlook something
 * we just implement the problem as posed.
 *
 * We need something to simulate the memory so we can fragment it to our hearts
 * content. We just cook up a freelist that contains an integer N that corresponds
 * to a region of N bytes of free memory.
 */
std::list<uint32_t> freeList;

/*
 * A counter for the number of times that the loop in the Allocate() function
 * has run.  Assumed to be protected by the test lock mutex (tl).
 */
uint32_t allocateLoops = 0;

/*
 * Allocate() looks through the free list and if it finds an entry corresponding
 * to a requested memory chunk size, it removes it from the free list and
 * returns that size.  If it cannot find a chunk of equal size it Wait()s until
 * something is put on the free list.  Allocate() in this model corresponds to
 * Consume() in the previous tests.
 */
uint32_t Allocate(qcc::Condition& c, qcc::Mutex& m, uint32_t n) {
    m.Lock();
    while (true) {
        for (std::list<uint32_t>::iterator i = freeList.begin(); i != freeList.end(); ++i) {
            if (*i == n) {
                freeList.erase(i);
                m.Unlock();
                return n;
            }
        }
        c.Wait(m);
        tl.Lock();
        ++allocateLoops;
        tl.Unlock();
    }
    assert(false && "Allocator(): This can't happen\n");
    return 0;
}

/*
 * Free() adds memory chunks to the free list.  It always Broadcast signals the
 * condition variable to wake up all of the waiting Allocate() instances that
 * then contend for the limited resource.  Free() in this model corresponds to
 * Produce() in the previous tests.
 */
void Free(qcc::Condition& c, qcc::Mutex& m, uint32_t n)
{
    m.Lock();
    freeList.push_back(n);
    c.Broadcast();
    m.Unlock();
}

/*
 * We want to be able to track what happened in the test, so once a region of
 * memory is allocated, we stick it on a list of used memory so we can verify
 * that the correct allocation happened at the right time.
 */
std::list<uint32_t> testList;

/*
 * We need more than one thread, so we provide an Allocator thread to get items
 * off of the free list.  For the simple tests, the gtest main thread will serve
 * as the Freer thread.
 *
 * Note that the thread checks for a done bit at the end of its main loop, so it
 * will execute Allocate() at least once.
 */
class AllocatorThread : public qcc::Thread {
  public:
    AllocatorThread(Condition* c, Mutex* m)
        : qcc::Thread(qcc::String("A")), m_c(c), m_m(m), m_done(false), m_state(IDLE), m_loops(0) { }
    void Size(uint32_t size) { m_size = size; }
    void Done() { m_done = true; }
    GenericState GetState() { return m_state; }
    uint32_t GetLoops() { return m_loops; }

  protected:
    qcc::ThreadReturn STDCALL Run(void* arg)
    {
        m_state = RUN_ENTERED;
        do {
            m_state = IN_LOOP;

            /*
             * Allocate a "chunk"of the provided size.
             */
            m_state = CALLING;
            int32_t n = Allocate(*m_c, *m_m, m_size);
            m_state = CALLED;

            /*
             * And put that something on the test list.  Borrow the tl mutex created
             * for the previous series of tests to protect it.
             */
            tl.Lock();
            testList.push_back(n);
            tl.Unlock();

            ++m_loops;
        } while (m_done == false);
        m_state = DONE;
        return 0;
    }
  private:
    Condition* m_c;
    Mutex* m_m;
    bool m_done;
    uint32_t m_size;
    GenericState m_state;
    uint32_t m_loops;
};

TEST(ConditionTest, simple_alloc)
{
    /*
     * Create a condition variable corresponding to a change in the contents of
     * the memory free list.  The Allocator() function waits on the condition
     * and the Freer() function signals when it frees memory and puts something
     * on the free list.
     */
    Condition c;

    /*
     * Create a mutex to protect the shared free list.
     */
    Mutex m;

    /*
     * Clear the free list so there is no available memory and the allocator
     * will certainly block; and clear the testList so there are no results
     * left around.
     */
    freeList.clear();
    testList.clear();

    /*
     * Create an allocator thread to allocate regions out of the free list.
     * Tell it to look for a region of 100 bytes.
     */
    AllocatorThread allocator(&c, &m);
    allocator.Size(100);

    /*
     * Set the done bit on the allocator thread so it only executes one Allocate
     * operation and then quits.
     */
    allocator.Done();

    /*
     * Start the allocator thread.  We expect that it will begin running and
     * notice that there is nothing in the free list and block waiting
     * for something to appear.
     */
    allocator.Start();

    /*
     * Wait for the allocator thread to actually run and block, then make sure
     * that is in fact what it did by verifying that it hasn't allocated
     * anything.
     */
    for (uint32_t i = 0; i <= WATCHDOG; i += TICK) {
        ASSERT_LT((uint32_t)i, WATCHDOG);
        if (allocator.GetState() == CALLING) {
            break;
        }
        qcc::Sleep(TICK);
    }

    /*
     * We can tell that the allocator thread has not actually allocated anything
     * by asking it how many times it has completed its work loop.  Zero means
     * it has blocked and not returned in its call to Allocate().
     */
    uint32_t loops = allocator.GetLoops();
    ASSERT_EQ((uint32_t)0, loops);

    tl.Lock();
    uint32_t allocated = testList.size();
    tl.Unlock();
    ASSERT_EQ((uint32_t)0, allocated);

    /*
     * Now free a chunk of memory corresponding to the chunk the allocator is
     * looking for -- 100 bytes.
     */
    Free(c, m, 100);

    /*
     * Since we have an allocator thread running looking for that size chunk,
     * after Free() puts that size of chunk on the free list, the allocator
     * should be awakened and pull the chunk off the free list and stick it on
     * the test list.  We need to wait for a short time until that happens.
     * Since we have the done bit set, the allocator should reflect that.
     */
    for (uint32_t i = 0; i <= WATCHDOG; i += TICK) {
        ASSERT_LT((uint32_t)i, WATCHDOG);
        if (allocator.GetState() == DONE) {
            break;
        }
        qcc::Sleep(TICK);
    }

    /*
     * The allocator thread should have exited since its done bit was set.  We
     * still need to play by the AllJoyn Thread rules and run through the rest
     * of the life cycle.
     */
    allocator.Stop();
    allocator.Join();

    /*
     * We should now find exactly one "chunk" on the test list.
     */
    allocated = testList.size();
    ASSERT_EQ((uint32_t)1, allocated);

    /*
     * That one thing should be an chunk of the same value as we Free()d and Allocate()d -- 100 bytes.
     */
    uint32_t size = testList.front();
    testList.pop_front();
    ASSERT_EQ((uint32_t)100, size);
}

TEST(ConditionTest, inverted_alloc)
{
    /*
     * Create a condition variable corresponding to a change in the contents of
     * the memory free list.  The Allocator() function waits on the condition
     * and the Freer() function signals when it frees memory and puts something
     * on the free list.
     */
    Condition c;

    /*
     * Create a mutex to protect the shared free list.
     */
    Mutex m;

    /*
     * Clear the free list so there is no available memory and the allocator
     * will certainly block; and clear the testList so there are no results
     * left around.
     */
    freeList.clear();
    testList.clear();

    /*
     * Zero the allocator loops counter, and create an allocator thread to
     * allocate regions out of the free list.  Tell it to look for a region of
     * 1000 bytes.
     */
    allocateLoops = 0;
    AllocatorThread allocator(&c, &m);
    allocator.Size(1000);

    /*
     * Start the allocator thread.  We expect that it will begin running and
     * notice that there is nothing in the free list and block waiting
     * for something to appear.
     */
    allocator.Start();

    /*
     * Wait for the allocator thread to actually run and block, then make sure
     * that is in fact what it did by verifying that it hasn't allocated
     * anything.
     */
    for (uint32_t i = 0; i <= WATCHDOG; i += TICK) {
        ASSERT_LT((uint32_t)i, WATCHDOG);
        if (allocator.GetState() == CALLING) {
            break;
        }
        qcc::Sleep(TICK);
    }

    /*
     * We can tell that the allocator thread has not actually allocated anything
     * by asking it how many times it has completed its work loop.  Zero means
     * it has blocked and not returned in its call to Allocate().
     */
    uint32_t loops = allocator.GetLoops();
    ASSERT_EQ((uint32_t)0, loops);

    /*
     * There is also a loop counter for the Allocate() function that should be
     * zero; and we double-check that nothing has somehow magically made it onto
     * the list of allocated chunks.
     */
    tl.Lock();
    ASSERT_EQ((uint32_t)0, allocateLoops);
    uint32_t allocated = testList.size();
    tl.Unlock();
    ASSERT_EQ((uint32_t)0, allocated);

    /*
     * Now free a chunk of memory smaller than the chunk the allocator is
     * looking for -- 100 bytes.  The Free should Broadcast() a signal to
     * all threads waiting on the condition.
     */
    Free(c, m, 100);

    /*
     * Since we have an allocator thread running it should be awakened and look
     * for a chunk of size 1000 on the free list.  That size of chunk is not
     * there so it should go back to sleep and wait for another free.  This
     * isn't really testing anything new, we are just making sure that
     * everything is behaving as expected before looking at the Broadcast
     * behavior which is going to require some multithreaded orchestration.
     */
    for (uint32_t i = 0; i <= WATCHDOG; i += TICK) {
        ASSERT_LT((uint32_t)i, WATCHDOG);
        tl.Lock();
        uint32_t loops = allocateLoops;
        tl.Unlock();
        if (allocator.GetState() == CALLING && loops == 1) {
            break;
        }
        qcc::Sleep(TICK);
    }

    /*
     * We should now find exactly zero "chunks" on the test (allocated) list.
     */
    allocated = testList.size();
    ASSERT_EQ((uint32_t)0, allocated);

    /*
     * Now set the done bit on the allocator so it exits after its next
     * allocation and free a chunk of memory of the size the allocator is
     * looking for -- 1000 bytes.
     */
    allocator.Done();
    Free(c, m, 1000);

    /*
     * Since we have an allocator thread running it should be awakened and look
     * for a chunk of size 1000 on the free list.  That size of chunk is now
     * there so it should allocate it by pulling it off the free list and
     * sticking it on the test list.  The allocator should then notice its done
     * bit being set and exit.  The GetLoops() method on the allocator will
     * return one, not two since it is the Allocate() function that looped.  The
     * only time the loop counter fro the allocator is bumped is if a successful
     * allocation happened, and this only happened once.
     */
    for (uint32_t i = 0; i <= WATCHDOG; i += TICK) {
        ASSERT_LT((uint32_t)i, WATCHDOG);
        if (allocator.GetState() == DONE && allocator.GetLoops() == 1) {
            break;
        }
        qcc::Sleep(TICK);
    }

    /*
     * The allocator loop counter was bumped once since there was one successful
     * allocation, but the allocateLoops counter monitoring the Allocate() function
     * will be two, since it looped once after the 100 byte chunk and again to get
     * the 1000 byte chunk.
     */
    ASSERT_EQ((uint32_t)2, allocateLoops);

    /*
     * The allocator thread should have exited since its done bit was set.  We
     * still need to play by the AllJoyn Thread rules and run through the rest
     * of the life cycle.
     */
    allocator.Stop();
    allocator.Join();

    /*
     * We should now find exactly one "chunk" on the test (allocated) list.
     */
    allocated = testList.size();
    ASSERT_EQ((uint32_t)1, allocated);

    /*
     * That one thing should be an chunk of the same value as we Free()d and
     * Allocate()d -- 1000 bytes.
     */
    uint32_t size = testList.front();
    testList.pop_front();
    ASSERT_EQ((uint32_t)1000, size);
}

TEST(ConditionTest, broadcast_alloc)
{
    /*
     * Create a condition variable corresponding to a change in the contents of
     * the memory free list.  The Allocator() function waits on the condition
     * and the Freer() function signals when it frees memory and puts something
     * on the free list.
     */
    Condition c;

    /*
     * Create a mutex to protect the shared free list.
     */
    Mutex m;

    /*
     * Clear the free list so there is no available memory and the allocator
     * will certainly block; and clear the testList so there are no results
     * left around.  Zero the allocator loops counter.
     */
    freeList.clear();
    testList.clear();
    allocateLoops = 0;

    /*
     * Zero the allocator loops counter, and create some allocator threads
     * each looking for a separate differently-sized region.
     */
    AllocatorThread allocator1000(&c, &m);
    allocator1000.Size(1000);

    AllocatorThread allocator100(&c, &m);
    allocator100.Size(100);

    AllocatorThread allocator10(&c, &m);
    allocator10.Size(10);

    /*
     * Start the allocator threads.  We expect that they will all begin running,
     * notice that there is nothing in the free list and block waiting for
     * something to appear.
     *
     * We want to be careful about the order in which the allocators arrive and
     * wait on the condition variable, so we start each one in a specific order
     * and wait for it to arrive on station before proceeding.
     */
    allocator1000.Start();
    for (uint32_t i = 0; i <= WATCHDOG; i += TICK) {
        ASSERT_LT((uint32_t)i, WATCHDOG);
        if (allocator1000.GetState() == CALLING) {
            break;
        }
        qcc::Sleep(TICK);
    }

    allocator100.Start();
    for (uint32_t i = 0; i <= WATCHDOG; i += TICK) {
        ASSERT_LT((uint32_t)i, WATCHDOG);
        if (allocator100.GetState() == CALLING) {
            break;
        }
        qcc::Sleep(TICK);
    }

    allocator10.Start();
    for (uint32_t i = 0; i <= WATCHDOG; i += TICK) {
        ASSERT_LT((uint32_t)i, WATCHDOG);
        if (allocator10.GetState() == CALLING) {
            break;
        }
        qcc::Sleep(TICK);
    }

    /*
     * We can tell that the allocator threads have not actually allocated
     * anything by asking how many times each has completed its work loop.  Zero
     * means it has blocked and not returned in its call to Allocate().
     */
    uint32_t loops = allocator10.GetLoops();
    ASSERT_EQ((uint32_t)0, loops);

    loops = allocator100.GetLoops();
    ASSERT_EQ((uint32_t)0, loops);

    loops = allocator1000.GetLoops();
    ASSERT_EQ((uint32_t)0, loops);

    /*
     * There is also a loop counter for the Allocate() function that should be
     * zero since none of the threads should have made it through that loop; and
     * we double-check that nothing has somehow magically made it onto the list
     * of allocated chunks.
     */
    tl.Lock();
    ASSERT_EQ((uint32_t)0, allocateLoops);
    uint32_t allocated = testList.size();
    tl.Unlock();
    ASSERT_EQ((uint32_t)0, allocated);

    /*
     * We now have three threads verified to be wanting to allocate different
     * sized chunks.  We were careful to cause the requests to appear in inverse
     * order of size, so the 1000 byte request queued up first, the 100 byte chunk
     * second (in the middle) and the 10 byte request was queued up last.
     *
     * What we want to verify is that when we free a chunk it Broadcasts a
     * signal to all of the threads.  All of the threads must wake up and
     * contend for the resource.  One of the threads should find what it wants
     * and report it out in the testList (and we will ask it to exit), then the
     * remaining two threads should go back to sleep.  What we are going to
     * verify is that all threads wake up and do what is expected; and the order
     * in which they arrived on the condition variable is not significant -
     * i.e., we do not have to free in any particular order.
     *
     * So, free a chunk of memory smaller that the middle Allocator thread is
     * looking for -- 100 bytes.  The Free should Broadcast() a signal to all
     * threads waiting on the condition.  They should all wake up and look to
     * see what was freed, and the expected allocator should show one completed
     * allocation loop.  We tell the one we expect to cycle to exit when it has
     * gotten the deed done.
     */
    allocator100.Done();
    Free(c, m, 100);

    /*
     * Prior to the Free, all three threads have entered Allocate() in their own
     * contexts.  They have all looked for memory and not found it.  They have
     * have all called Wait() without incrementing the allocateLoops counter in
     * Allocate().  The Free() should have caused all three threads to wake up.
     * This will have caused Allocate() to increment the loops counter three
     * times indicating that all three threads have waken up and run.
     *
     * One of the threads will have found what it wanted.  In this case, Allocate()
     * will have returned and the thread's the loop counter will have incremented
     * indicating a successful allocation.  The other threads will have looped back
     * in Allocate() and blocked again on a Wait().
     *
     * So, we check for three allocateLoops completing and the middle allocator
     * thread exiting.
     */
    for (uint32_t i = 0; i <= WATCHDOG; i += TICK) {
        ASSERT_LT((uint32_t)i, WATCHDOG);
        tl.Lock();
        uint32_t loops = allocateLoops;
        tl.Unlock();
        if (allocator100.GetState() == DONE && loops == 3) {
            break;
        }
        qcc::Sleep(TICK);
    }

    /*
     * We should now find exactly one "chunk" on the test (allocated) list.
     */
    allocated = testList.size();
    ASSERT_EQ((uint32_t)1, allocated);

    /*
     * That one thing should be a chunk of the same value as we Free()d and
     * Allocate()d -- 100 bytes.
     */
    uint32_t size = testList.front();
    testList.pop_front();
    ASSERT_EQ((uint32_t)100, size);

    /*
     * Now set the done bit on the other two allocators so they exit.
     */
    allocator10.Done();
    allocator1000.Done();

    /*
     * Provide a 1000 byte chunk for the third allocator thread to snag.
     */
    Free(c, m, 1000);

    /*
     * Now wait for the two threads to be awakened and cause the allocateLoop
     * counter to be bumped twice more.  Since we set the done bit, one of
     * the threads should also exit.
     */
    for (uint32_t i = 0; i <= WATCHDOG; i += TICK) {
        ASSERT_LT((uint32_t)i, WATCHDOG);
        tl.Lock();
        uint32_t loops = allocateLoops;
        tl.Unlock();
        if (allocator1000.GetState() == DONE && loops == 5) {
            break;
        }
        qcc::Sleep(TICK);
    }

    /*
     * We should now find exactly one "chunk" on the test (allocated) list.
     */
    allocated = testList.size();
    ASSERT_EQ((uint32_t)1, allocated);

    /*
     * That one thing should be a chunk of the same value as we Free()d and
     * Allocate()d -- 100 bytes.
     */
    size = testList.front();
    testList.pop_front();
    ASSERT_EQ((uint32_t)1000, size);

    /*
     * Provide a 10 byte chunk for the first allocator thread to snag.
     */
    Free(c, m, 10);

    /*
     * Now wait for the single remaining thread to be awakened and cause the
     * allocateLoop counter to be bumped.  Since we set the done bit, the last
     * of the threads should also exit.
     */
    for (uint32_t i = 0; i <= WATCHDOG; i += TICK) {
        ASSERT_LT((uint32_t)i, WATCHDOG);
        tl.Lock();
        uint32_t loops = allocateLoops;
        tl.Unlock();
        if (allocator10.GetState() == DONE && loops == 6) {
            break;
        }
        qcc::Sleep(TICK);
    }

    /*
     * We should now find exactly one "chunk" on the test (allocated) list.
     */
    allocated = testList.size();
    ASSERT_EQ((uint32_t)1, allocated);

    /*
     * That one thing should be a chunk of the same value as we Free()d and
     * Allocate()d -- 10 bytes.
     */
    size = testList.front();
    testList.pop_front();
    ASSERT_EQ((uint32_t)10, size);

    /*
     * The allocator threads have exited, but we still need to play by the
     * AllJoyn Thread rules and run through the rest of the life cycle.
     */
    allocator10.Stop();
    allocator10.Join();

    allocator100.Stop();
    allocator100.Join();

    allocator1000.Stop();
    allocator1000.Join();
}
