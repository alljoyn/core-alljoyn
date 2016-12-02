/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/

#ifndef ALLJOYN_SECMGR_TASKQUEUE_H_
#define ALLJOYN_SECMGR_TASKQUEUE_H_

#include <vector>

#include <qcc/Mutex.h>
#include <qcc/Condition.h>
#include <qcc/Thread.h>

using namespace std;
using namespace qcc;

namespace ajn {
namespace securitymgr {
template <typename TASK, typename HANDLER>
class TaskQueue {
  public:

    TaskQueue(HANDLER* handler) :
        running(false),
        stopped(false),
        taskHandler(handler),
        thread(nullptr),
        cond(new Condition())
    {
    }

    ~TaskQueue()
    {
        delete cond;
        cond = nullptr;
    }

    void Stop()
    {
        mutex.Lock();
        stopped = true; //Indicate that no more task should be scheduled and the current thread should stop.
        while (running) {
            //If we are running, wait for the thread to signal it has completed.
            cond->Wait(mutex);
        }
        if (thread) {
            thread->Join();
            delete thread;
            thread = nullptr;
        }
        mutex.Unlock();
    }

    void AddTask(TASK task)
    {
        mutex.Lock();
        if (!stopped) { // Only add task when we are not stopped.
            list.push_back(task);
            if (!running) {
                if (thread != nullptr) {
                    thread->Join();
                    delete thread;
                    thread = nullptr;
                }
                thread = new QueueThread(this);
                thread->Start();
                running = true;
            }
        }
        mutex.Unlock();
    }

    class QueueThread :
        public Thread {
      public:

        QueueThread(TaskQueue* tq) :
            queue(tq) { }

        virtual ThreadReturn STDCALL Run(void* arg)
        {
            QCC_UNUSED(arg);

            queue->HandleTasks();
            return nullptr;
        }

      private:
        TaskQueue* queue;
    };

    void HandleTasks()
    {
        mutex.Lock();
        while (list.size() > 0) {
            TASK task = list[0];
            list.erase(list.begin());
            if (!stopped) { //Only handle task when not stopped.
                mutex.Unlock();
                taskHandler->HandleTask(task);
                mutex.Lock();
            }
            delete task;
            task = nullptr;
        }
        running = false;
        cond->Broadcast();
        mutex.Unlock();
    }

  private:
    volatile bool running; //To true indicates that a thread is handling the tasks.

    /*
     * True to indicate no thread should be started anymore
     * and the active thread should stop ASAP.
     */
    volatile bool stopped;
    HANDLER* taskHandler;
    vector<TASK> list;
    Mutex mutex;
    QueueThread* thread;
    Condition* cond;
};
}
}

#endif /* ALLJOYN_SECMGR_TASKQUEUE_H_ */