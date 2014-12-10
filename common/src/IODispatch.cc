/**
 * @file
 *
 * IODispatch listens on a set of file descriptors and provides callbacks for read/write
 */

/******************************************************************************
 * Copyright (c) 2012-2014, AllSeen Alliance. All rights reserved.
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
#include <qcc/IODispatch.h>
#include <qcc/StringUtil.h>
#define QCC_MODULE "IODISPATCH"

using namespace qcc;
using namespace std;

int32_t IODispatch::iodispatchCnt = 0;

IODispatch::IODispatch(const char* name, uint32_t concurrency) :
    timer((String(name) + U32ToString(IncrementAndFetch(&iodispatchCnt)).c_str()), true, concurrency, false, 96),
    reload(false),
    isRunning(false),
    numAlarmsInProgress(0),
    crit(false)
{

}
IODispatch::~IODispatch()
{
    reload = true;
    Stop();
    Join();

    /* All endpoints should have already been stopped and joined.
     * so, there should be no dispatch entries.
     * Just a sanity check.
     */
    assert(dispatchEntries.size() == 0);
}
QStatus IODispatch::Start(void* arg, ThreadListener* listener)
{
    /* Start the timer thread */
    QStatus status = timer.Start();

    if (status != ER_OK) {
        timer.Stop();
        timer.Join();
        return status;
    } else {
        isRunning = true;
        /* Start the main thread */
        return Thread::Start(arg, listener);
    }
}

QStatus IODispatch::Stop()
{
    lock.Lock();
    isRunning = false;
    map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.begin();
    Stream* stream;
    while (it != dispatchEntries.end()) {

        stream = it->first;
        lock.Unlock();
        StopStream(stream);
        lock.Lock();
        it = dispatchEntries.upper_bound(stream);
    }
    lock.Unlock();

    Thread::Stop();
    timer.Stop();
    return ER_OK;
}

QStatus IODispatch::Join()
{
    lock.Lock();

    map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.begin();
    Stream* stream;
    while (it != dispatchEntries.end()) {
        stream = it->first;
        lock.Unlock();
        JoinStream(stream);
        lock.Lock();
        it = dispatchEntries.upper_bound(stream);
    }
    lock.Unlock();

    Thread::Join();
    timer.Join();
    return ER_OK;
}

QStatus IODispatch::StartStream(Stream* stream, IOReadListener* readListener, IOWriteListener* writeListener, IOExitListener* exitListener, bool readEnable, bool writeEnable)
{
    QCC_DbgTrace(("StartStream %p", stream));
    lock.Lock();
    /* Dont attempt to register a stream if the IODispatch is shutting down */
    if (!isRunning) {
        lock.Unlock();
        return ER_IODISPATCH_STOPPING;
    }
    if (dispatchEntries.find(stream) != dispatchEntries.end()) {
        lock.Unlock();
        return ER_INVALID_STREAM;

    }
    dispatchEntries[stream] = IODispatchEntry(stream, readListener, writeListener, exitListener, readEnable, writeEnable);
    dispatchEntries[stream].readCtxt = new CallbackContext(stream, IO_READ);
    dispatchEntries[stream].writeCtxt = new CallbackContext(stream, IO_WRITE);
    dispatchEntries[stream].writeTimeoutCtxt = new CallbackContext(stream, IO_WRITE_TIMEOUT);
    dispatchEntries[stream].readTimeoutCtxt = new CallbackContext(stream, IO_READ_TIMEOUT);
    dispatchEntries[stream].exitCtxt = new CallbackContext(stream, IO_EXIT);

    /* Set reload to false and alert the IODispatch::Run thread */
    reload = false;
    lock.Unlock();

    Thread::Alert();
    /* Dont need to wait for the IODispatch::Run thread to reload
     * the set of file descriptors since we are adding a new stream.
     */
    return ER_OK;
}


QStatus IODispatch::StopStream(Stream* stream) {
    lock.Lock();
    QCC_DbgTrace(("StopStream %p", stream));
    map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.find(stream);

    /* Check if stream is still present in dispatchEntries. */
    if (it == dispatchEntries.end()) {
        lock.Unlock();
        return ER_INVALID_STREAM;
    }
    if (it->second.stopping_state == IO_STOPPED) {
        lock.Unlock();
        return ER_FAIL;
    }
    IODispatchEntry dispatchEntry = it->second;

    /* Disable further read and writes on this stream */
    it->second.stopping_state = IO_STOPPING;

    /* Set reload to false and alert the IODispatch::Run thread */
    reload = false;
    int when = 0;
    AlarmListener* listener = this;
    if (isRunning) {
        /* The main thread is running, so we must wait for it to reload the events.
         * The main thread is responsible for adding the exit alarm in this case.
         */
        Thread::Alert();

        /* Wait until the IODispatch::Run thread reloads the set of check events */
        while (!reload && crit && isRunning) {
            lock.Unlock();
            Sleep(1);
            lock.Lock();
        }
        lock.Unlock();
    } else {

        /* If the main thread has been asked to stopped, it may or may not have
         * added the exit alarm for this stream. The exit alarm makes the exit callback
         * which ensures that the RemoteEndpoint can be joined.
         */
        if (it->second.stopping_state == IO_STOPPING) {
            /* Add the exit alarm since it has not added by the main IODispatch::Run thread. */
            it->second.stopping_state = IO_STOPPED;
            /* We dont need to keep track of the exit alarm, since we never remove
             * the exit alarm. Hence it is not a part of IODispatchEntry.
             */
            Alarm exitAlarm = Alarm(when, listener, it->second.exitCtxt);
            lock.Unlock();
            /* At this point, the IODispatch::Run thread will not add any more alarms since it
             * has been told to stop, so it is ok to call the blocking version of AddAlarm
             */
            timer.AddAlarm(exitAlarm);
        }       else {
            lock.Unlock();
        }
    }

    return ER_OK;
}
QStatus IODispatch::JoinStream(Stream* stream) {
    lock.Lock();
    QCC_DbgTrace(("JoinStream %p", stream));

    /* Wait until the exit callback is complete and the
     * entry is removed from the dispatchEntries map
     */
    map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.find(stream);
    while (it != dispatchEntries.end()) {
        lock.Unlock();
        qcc::Sleep(10);
        lock.Lock();
        it = dispatchEntries.find(stream);
    }
    lock.Unlock();
    return ER_OK;
}
void IODispatch::AlarmTriggered(const Alarm& alarm, QStatus reason)
{
    lock.Lock();
    /* Find the stream associated with this alarm */
    CallbackContext* ctxt = static_cast<CallbackContext*>(alarm->GetContext());
    Stream* stream = ctxt->stream;

    /* Only correct values of type are IO_READ, IO_READ_TIMEOUT,
     * IO_WRITE, IO_WRITE_TIMEOUT, IO_EXIT
     */
    assert(ctxt->type >= IO_READ && ctxt->type <= IO_EXIT);

    if (!isRunning && ctxt->type != IO_EXIT) {
        /* If IODispatch is being shut down, only service exit alarms.
         * Ignore read/write/timeout alarms
         */
        lock.Unlock();
        return;
    }

    map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.find(stream);
    if (it == dispatchEntries.end()) {
        /* If stream is not found(should never happen since the exit alarm ensures that
         * read and write alarms are removed before deleting the entry from the map)
         */
        assert(false);
        /*
         * asserts are compiled out so me must return to prevent segfaults in
         * release code.
         */
        QCC_LogError(ER_FAIL, ("Unexpected error, stream is not found. The dispatchEntries map should always have a stream."));
        lock.Unlock();
        return;
    }
    if (((it->second.stopping_state != IO_RUNNING) && ctxt->type != IO_EXIT)) {
        /* If stream is being stopped and this is not an exit alarm, return.
         */
        lock.Unlock();
        return;
    }

    IODispatchEntry dispatchEntry = it->second;
    switch (ctxt->type) {
    case IO_READ_TIMEOUT:
        /* If this is the read timeout callback, then we must set readInProgress to true
         * to indicate to the main thread to remove this FD from the set of events it is
         * waiting for.
         * Also, we wait for the main thread to return from Event::Wait and/or reload the set
         * of descriptors.
         */
        it->second.readInProgress = true;
        while (!reload && crit && isRunning) {
            lock.Unlock();
            Sleep(1);
            lock.Lock();
        }

    case IO_READ:
        IncrementAndFetch(&numAlarmsInProgress);

        lock.Unlock();
        if (dispatchEntry.readEnable) {
            /* Ensure read has not been disabled */
            dispatchEntry.readListener->ReadCallback(*stream, ctxt->type == IO_READ_TIMEOUT);
        }
        DecrementAndFetch(&numAlarmsInProgress);
        break;

    case IO_WRITE_TIMEOUT:
        /* If this is the write timeout callback, then we must set writeInProgress to true
         * to indicate to the main thread to remove this FD from the set of events it is
         * waiting for.
         * Also, we wait for the main thread to return from Event::Wait and/or reload the set
         * of descriptors.
         */
        it->second.writeInProgress = true;
        while (!reload && crit && isRunning) {
            lock.Unlock();
            Sleep(1);
            lock.Lock();
        }

    case IO_WRITE:

        IncrementAndFetch(&numAlarmsInProgress);

        lock.Unlock();

        /* Make the write callback */
        if (dispatchEntry.writeEnable) {
            /* Ensure write has not been disabled */
            dispatchEntry.writeListener->WriteCallback(*stream, ctxt->type == IO_WRITE_TIMEOUT);
        }
        DecrementAndFetch(&numAlarmsInProgress);
        break;

    case IO_EXIT:

        lock.Unlock();
        /* Remove any pending alarms */
        timer.ForceRemoveAlarm(dispatchEntry.readAlarm, true /* blocking */);
        timer.ForceRemoveAlarm(dispatchEntry.writeAlarm, true /* blocking */);
        lock.Lock();
        /* If IODispatch has been stopped,
         * RemoveAlarms may not have successfully removed the alarm.
         * In that case, wait for any alarms that are in progress to finish.
         */
        while (!isRunning && numAlarmsInProgress) {
            lock.Unlock();
            Sleep(2);
            lock.Lock();
        }

        /* Make the exit callback */
        lock.Unlock();
        dispatchEntry.exitListener->ExitCallback();
        lock.Lock();
        /* Find and erase the stream entry */
        it = dispatchEntries.find(stream);
        if (it == dispatchEntries.end()) {
            /* This should never happen - it means that the entry was deleted
             * while the exit callback was being made
             */
            assert(false);
            /*
             * since asserts are compiled out or release code we must add code
             * to handle this if it occurs.
             */
            QCC_LogError(ER_FAIL, ("The IO stream entry was not found on IO_EXIT"));
            lock.Unlock();
            return;
        }
        if (it->second.readCtxt) {
            delete it->second.readCtxt;
            it->second.readCtxt = NULL;
        }
        if (it->second.writeCtxt) {
            delete it->second.writeCtxt;
            it->second.writeCtxt = NULL;
        }
        if (it->second.writeTimeoutCtxt) {
            delete it->second.writeTimeoutCtxt;
            it->second.writeTimeoutCtxt = NULL;
        }
        if (it->second.exitCtxt) {
            delete it->second.exitCtxt;
            it->second.exitCtxt = NULL;
        }
        if (it->second.readTimeoutCtxt) {
            delete it->second.readTimeoutCtxt;
            it->second.readTimeoutCtxt = NULL;
        }
        dispatchEntries.erase(it);
        lock.Unlock();
        break;

    default:
        break;

    }
}

ThreadReturn STDCALL IODispatch::Run(void* arg) {

    vector<qcc::Event*> checkEvents, signaledEvents;
    int32_t when =  0;
    AlarmListener* listener = this;

    while (!IsStopping()) {
        checkEvents.clear();
        signaledEvents.clear();
        /* Add the Thread's stop event to list of events to check for */
        checkEvents.push_back(&stopEvent);

        /* Set reload to true to indicate that this thread is not in the Event::Wait and is
         * reloading the set of source and sink events
         */
        lock.Lock();
        reload = true;
        map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.begin();
        while (it != dispatchEntries.end() && isRunning) {
            if (it->second.stopping_state == IO_RUNNING) {
                /* Check this stream only if it has not been stopped */
                if (it->second.readEnable && !it->second.readInProgress) {
                    /* If read is enabled and not in progress, add the source event for the stream to the
                     * set of check events
                     */
                    checkEvents.push_back(&it->first->GetSourceEvent());
                }
                if (it->second.writeEnable && !it->second.writeInProgress) {
                    /* If write is enabled and not in progress, add the sink event for the stream to the
                     * set of check events
                     */
                    checkEvents.push_back(&it->first->GetSinkEvent());
                }
            }
            it++;
        }
        crit = true;
        lock.Unlock();

        /* Wait for an event to occur */
        qcc::Event::Wait(checkEvents, signaledEvents);

        lock.Lock();
        crit = false;
        reload = true;

        lock.Unlock();
        for (vector<qcc::Event*>::iterator i = signaledEvents.begin(); i != signaledEvents.end(); ++i) {
            if (*i == &stopEvent) {
                /* This thread has been alerted or is being stopped. Will check the IsStopping()
                 * flag when the while condition is encountered.
                 * Note that the stop event must be reset before adding the exit alarms to ensure that
                 * exit alarms are added for all streams that are stopped within close duration of each other.
                 */
                lock.Lock();
                stopEvent.ResetEvent();

                it = dispatchEntries.begin();
                /* Add exit alarms for any streams that are being stopped.
                 * We dont need to keep track of the exit alarm, since we never remove
                 * the exit alarm. Hence it is not a part of IODispatchEntry.
                 */
                while (it != dispatchEntries.end() && isRunning) {
                    if (it->second.stopping_state == IO_STOPPING) {
                        Alarm exitAlarm = Alarm(when, listener, it->second.exitCtxt);
                        Stream* lookup = it->first;
                        QStatus status = ER_TIMER_FULL;
                        while (isRunning && status == ER_TIMER_FULL && it != dispatchEntries.end() && it->second.stopping_state != IO_STOPPED) {
                            /* Call the non-blocking version of AddAlarm, while holding the
                             * locks to ensure that the state of the dispatchEntry is valid.
                             */
                            status = timer.AddAlarmNonBlocking(exitAlarm);

                            if (status == ER_TIMER_FULL) {
                                lock.Unlock();
                                qcc::Sleep(2);
                                lock.Lock();
                            }
                            it = dispatchEntries.find(lookup);
                        }
                        if (status == ER_OK && it != dispatchEntries.end()) {
                            it->second.stopping_state = IO_STOPPED;
                            it++;
                        }

                    } else {
                        it++;
                    }
                }
                lock.Unlock();
                continue;
            } else {
                lock.Lock();
                map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.begin();
                while (it != dispatchEntries.end()) {

                    Stream* stream = it->first;

                    if (it->second.stopping_state == IO_RUNNING) {
                        if (&stream->GetSourceEvent() == *i) {

                            if (it->second.readEnable && !it->second.readInProgress) {
                                /* If the source event for a particular stream has been signalled,
                                 * add a readAlarm to fire now, and set readInProgress to true.
                                 */
                                Alarm prevAlarm = it->second.readAlarm;
                                Alarm readAlarm = Alarm(when, listener, it->second.readCtxt);
                                it->second.readInProgress = true;
                                it->second.mainAddingRead = true;
                                lock.Unlock();
                                /* Remove the read timeout alarm if any first */
                                timer.RemoveAlarm(prevAlarm, true);
                                lock.Lock();
                                it->second.mainAddingRead = false;

                                QStatus status = ER_TIMER_FULL;
                                it = dispatchEntries.find(stream);

                                while (isRunning && status == ER_TIMER_FULL && it != dispatchEntries.end() && it->second.stopping_state == IO_RUNNING) {
                                    /* Call the non-blocking version of AddAlarm, while holding the
                                     * locks to ensure that the state of the dispatchEntry is valid.
                                     */
                                    status = timer.AddAlarmNonBlocking(readAlarm);

                                    if (status == ER_TIMER_FULL) {
                                        lock.Unlock();
                                        qcc::Sleep(2);
                                        lock.Lock();
                                    }

                                    it = dispatchEntries.find(stream);
                                }
                                if (status == ER_OK && it != dispatchEntries.end()) {
                                    it->second.readAlarm = readAlarm;

                                }

                                break;
                            }

                        } else if (&stream->GetSinkEvent() == *i) {
                            if (it->second.writeEnable && !it->second.writeInProgress) {
                                /* If the sink event for a particular stream has been signalled,
                                 * add a writeAlarm to fire now, and set writeInProgress to true.
                                 */
                                Alarm prevAlarm = it->second.writeAlarm;

                                Alarm writeAlarm = Alarm(when, listener, it->second.writeCtxt);
                                it->second.writeInProgress = true;
                                it->second.mainAddingWrite = true;

                                lock.Unlock();
                                /* Remove the write timeout alarm if any first */
                                timer.RemoveAlarm(prevAlarm, true);
                                lock.Lock();
                                it->second.mainAddingWrite = false;

                                QStatus status = ER_TIMER_FULL;

                                map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.find(stream);
                                while (isRunning && status == ER_TIMER_FULL &&  it != dispatchEntries.end() && it->second.stopping_state == IO_RUNNING) {
                                    /* Call the non-blocking version of AddAlarm, while holding the
                                     * locks to ensure that the state of the dispatchEntry is valid.
                                     */
                                    status = timer.AddAlarmNonBlocking(writeAlarm);

                                    if (status == ER_TIMER_FULL) {
                                        lock.Unlock();
                                        qcc::Sleep(2);
                                        lock.Lock();
                                    }

                                    it = dispatchEntries.find(stream);
                                }
                                if (status == ER_OK && it != dispatchEntries.end()) {

                                    it->second.writeAlarm = writeAlarm;
                                }

                                break;
                            }
                        }
                    }
                    it++;
                }
                lock.Unlock();
            }
        }
    }
    lock.Lock();
    /* Set isRunning flag and reload flag. */
    reload = true;
    QCC_DbgPrintf(("IODispatch::Run exiting"));
    lock.Unlock();

    return (ThreadReturn) 0;
}


QStatus IODispatch::EnableReadCallback(const Source* source, uint32_t timeout)
{
    lock.Lock();
    /* Dont attempt to modify an entry if the IODispatch is shutting down */
    if (!isRunning) {
        lock.Unlock();
        return ER_IODISPATCH_STOPPING;
    }
    Stream* lookup = (Stream*)source;
    map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.find(lookup);

    /* Ensure stream is valid and still running */
    if (it == dispatchEntries.end() || (it->second.stopping_state != IO_RUNNING)) {
        lock.Unlock();
        return ER_INVALID_STREAM;
    }

    it->second.readEnable = true;
    if (it->second.mainAddingRead) {
        lock.Unlock();
        return ER_OK;
    }
    if (timeout != 0) {
        /* If timeout is non-zero, add a timeout alarm */
        uint32_t temp = timeout * 1000;
        AlarmListener* listener = this;

        Alarm readAlarm = Alarm(temp, listener, it->second.readTimeoutCtxt);
        QStatus status = ER_TIMER_FULL;
        while (isRunning && status == ER_TIMER_FULL &&  it != dispatchEntries.end() && it->second.stopping_state == IO_RUNNING) {
            /* Call the non-blocking version of AddAlarm, while holding the
             * locks to ensure that the state of the dispatchEntry is valid.
             */
            status = timer.AddAlarmNonBlocking(readAlarm);

            if (status == ER_TIMER_FULL) {
                lock.Unlock();
                qcc::Sleep(2);
                lock.Lock();
            }

            it = dispatchEntries.find(lookup);
        }
        if (status == ER_OK && it != dispatchEntries.end()) {
            it->second.readAlarm = readAlarm;
            /* Set readInProgress to false only after adding the alarm
             * This is to ensure that there is no race condition due to the main thread
             * trying to remove this alarm before it has been added and assuming
             * it was successful
             */
            it->second.readInProgress = false;
        }
    } else {
        /* Timeout = 0 indicates that no timeout alarm is required for this stream */
        it->second.readInProgress = false;
    }
    lock.Unlock();

    Thread::Alert();
    /* Dont need to wait for the IODispatch::Run thread to reload
     * the set of file descriptors since we're enabling read.
     */
    return ER_OK;
}

QStatus IODispatch::EnableTimeoutCallback(const Source* source, uint32_t timeout)
{
    lock.Lock();
    /* Dont attempt to modify an entry if the IODispatch is shutting down */
    if (!isRunning) {
        lock.Unlock();
        return ER_IODISPATCH_STOPPING;
    }

    Stream* lookup = (Stream*)source;
    map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.find(lookup);
    /* Ensure stream is valid and still running */
    if (it == dispatchEntries.end() || (it->second.stopping_state != IO_RUNNING)) {
        lock.Unlock();
        return ER_INVALID_STREAM;
    }

    /* If a read is in progress, the ReadCallback will take care of adding the
     * timeout callback for this stream.
     */
    if (it->second.readInProgress || it->second.mainAddingRead) {
        lock.Unlock();
        return ER_OK;
    }

    Alarm prevAlarm = it->second.readAlarm;
    if (timeout != 0) {
        /* If timeout is non-zero, add a timeout alarm */
        uint32_t temp = timeout * 1000;
        AlarmListener* listener = this;
        Alarm readAlarm = Alarm(temp, listener, it->second.readTimeoutCtxt);

        /* Remove previous read timeout alarm if any */
        timer.RemoveAlarm(prevAlarm, false);

        QStatus status = ER_TIMER_FULL;
        map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.find(lookup);
        while (isRunning && status == ER_TIMER_FULL &&  it != dispatchEntries.end() && !it->second.readInProgress && it->second.stopping_state == IO_RUNNING) {
            /* Call the non-blocking version of AddAlarm, while holding the
             * locks to ensure that the state of the dispatchEntry is valid.
             */
            status = timer.AddAlarmNonBlocking(readAlarm);

            if (status == ER_TIMER_FULL) {
                lock.Unlock();
                qcc::Sleep(2);
                lock.Lock();
            }

            it = dispatchEntries.find(lookup);
        }
        if (status == ER_OK && it != dispatchEntries.end()) {
            it->second.readAlarm = readAlarm;
        }

    } else {
        /* Zero timeout indicates no timeout alarm is required. */
        timer.RemoveAlarm(prevAlarm, false);

    }
    lock.Unlock();
    return ER_OK;
}
QStatus IODispatch::DisableReadCallback(const Source* source)
{
    lock.Lock();
    /* Dont attempt to modify an entry if the IODispatch is shutting down */
    if (!isRunning) {
        lock.Unlock();
        return ER_IODISPATCH_STOPPING;
    }

    Stream* lookup = (Stream*)source;
    map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.find(lookup);
    /* Ensure stream is valid and still running */
    if (it == dispatchEntries.end() || (it->second.stopping_state != IO_RUNNING)) {
        lock.Unlock();
        return ER_INVALID_STREAM;
    }
    it->second.readEnable = false;
    lock.Unlock();
    Thread::Alert();
    /* Wait until the IODispatch::Run thread reloads the set of check events
     * since we are disabling read.
     */
    while (!reload && crit && isRunning) {
        Sleep(10);
    }
    return ER_OK;
}

QStatus IODispatch::EnableWriteCallbackNow(Sink* sink)
{
    lock.Lock();
    /* Dont attempt to modify an entry if the IODispatch is shutting down */
    if (!isRunning) {
        lock.Unlock();
        return ER_IODISPATCH_STOPPING;
    }

    Stream* lookup = (Stream*)sink;
    map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.find(lookup);
    /* Ensure stream is valid and still running */
    if (it == dispatchEntries.end() || (it->second.stopping_state != IO_RUNNING)) {
        lock.Unlock();
        return ER_INVALID_STREAM;
    }
    if (it->second.writeEnable || it->second.mainAddingWrite) {
        lock.Unlock();
        return ER_OK;
    }
    it->second.writeEnable = true;
    it->second.writeInProgress = true;

    int32_t when = 0;
    AlarmListener* listener = this;

    /* Add a write alarm to fire now, there is data ready to be written */
    it->second.writeAlarm = Alarm(when, listener, it->second.writeCtxt);
    Alarm writeAlarm = it->second.writeAlarm;
    QStatus status = timer.AddAlarmNonBlocking(writeAlarm);
    if (status == ER_TIMER_FULL) {
        /* Since the timer is full, just alert the main thread, so that
         * it can add a write alarm for this stream when possible.
         * Do not block here, since it can create deadlocks.
         */
        it->second.writeInProgress = false;
        Thread::Alert();
    }
    lock.Unlock();
    return ER_OK;
}

QStatus IODispatch::EnableWriteCallback(Sink* sink, uint32_t timeout)
{
    lock.Lock();
    /* Dont attempt to modify an entry if the IODispatch is shutting down */
    if (!isRunning) {
        lock.Unlock();
        return ER_IODISPATCH_STOPPING;
    }

    Stream* lookup = (Stream*)sink;
    map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.find(lookup);
    if (it == dispatchEntries.end() || (it->second.stopping_state != IO_RUNNING)) {
        lock.Unlock();
        return ER_INVALID_STREAM;
    }

    it->second.writeEnable = true;
    if (it->second.mainAddingWrite) {
        lock.Unlock();
        return ER_OK;
    }

    if (timeout != 0) {
        int32_t when = timeout * 1000;
        AlarmListener* listener = this;

        /* Add a write alarm to fire by default if there is no sink event after this amount of time */
        Alarm writeAlarm = Alarm(when, listener, it->second.writeTimeoutCtxt);
        QStatus status = ER_TIMER_FULL;

        map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.find(lookup);
        while (isRunning && status == ER_TIMER_FULL &&  it != dispatchEntries.end() && it->second.stopping_state == IO_RUNNING) {
            /* Call the non-blocking version of AddAlarm, while holding the
             * locks to ensure that the state of the dispatchEntry is valid.
             */
            status = timer.AddAlarmNonBlocking(writeAlarm);

            if (status == ER_TIMER_FULL) {
                lock.Unlock();
                qcc::Sleep(2);
                lock.Lock();
            }

            it = dispatchEntries.find(lookup);
        }
        if (status == ER_OK && it != dispatchEntries.end()) {

            it->second.writeAlarm = writeAlarm;
            it->second.writeInProgress = false;
        }
    } else {
        it->second.writeInProgress = false;
    }
    lock.Unlock();
    Thread::Alert();

    /* Dont need to wait for the IODispatch::Run thread to reload
     * the set of file descriptors, since we are enabling write callback.
     */
    return ER_OK;
}
QStatus IODispatch::DisableWriteCallback(const Sink* sink)
{
    lock.Lock();
    /* Dont attempt to modify an entry if the IODispatch is shutting down */
    if (!isRunning) {
        lock.Unlock();
        return ER_IODISPATCH_STOPPING;
    }

    Stream* lookup = (Stream*)sink;
    map<Stream*, IODispatchEntry>::iterator it = dispatchEntries.find(lookup);
    if (it == dispatchEntries.end() || (it->second.stopping_state != IO_RUNNING)) {
        lock.Unlock();
        return ER_INVALID_STREAM;
    }
    it->second.writeEnable = false;

    lock.Unlock();
    Thread::Alert();
    /* Wait until the IODispatch::Run thread reloads the set of check events
     * since we are disabling write.
     */
    while (!reload && crit && isRunning) {
        Sleep(10);
    }
    return ER_OK;
}


