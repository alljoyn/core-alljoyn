/**
 * @file
 *
 * Define a class that abstracts the alljoin thread state 
 * machine. 
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

#include <qcc/ThreadState.h>
#include <qcc/ScopedMutexLock.h>
#include <qcc/LockLevel.h>

using namespace std;

/** @internal */
#define QCC_MODULE "THREADSTATE"

namespace qcc {
  
ThreadState::ThreadState(bool isExternal)
    :mStateLock(LOCK_LEVEL_CHECKING_DISABLED),
     mCurrentState(isExternal ? EXTERNAL : INITIAL)
{
}
    
ThreadState::State ThreadState::getCurrentState() const
{
    ScopedMutexLock lock(mStateLock);
    return mCurrentState;
}

bool ThreadState::isExternal() const
{
    ScopedMutexLock lock(mStateLock);
    return (mCurrentState == EXTERNAL || 
            mCurrentState == EXTERNALJOINING || 
            mCurrentState == EXTERNALJOINED );
}

ThreadState::Rc ThreadState::start()
{
    ScopedMutexLock lock(mStateLock);
    switch (mCurrentState)
    {
        case EXTERNALJOINING:
        case EXTERNALJOINED:
        {
            return Error;
        }
        case INITIAL: 
        {
            mCurrentState = STARTING;
            return Ok;
        }
        case STARTING:
        case RUNNING:
        {
            return AlreadyRunning;
        }
        case STOPPING:
        case STOPPED:
        case JOINING:
        {
            return AlreadyStopped;
        }
        case DEAD:
        {
            mCurrentState = STARTING;
            return Ok;
        }
        case EXTERNAL:
        {
            return IsExternalThread;
        }
        case ERROR:
        {
            return Error;
        }
    }
    return Error;
}

ThreadState::Rc ThreadState::started()
{
    ScopedMutexLock lock(mStateLock);
    switch (mCurrentState)
    {
        case INITIAL:
        case EXTERNALJOINING:
        case EXTERNALJOINED:
        {
            return Error;
        }
        case STARTING:
        {
            mCurrentState = RUNNING;
            mStateCondition.Broadcast();
            return Ok;
        }
        case RUNNING:
        {
            return AlreadyRunning;
        }
        case STOPPING:
        case STOPPED:
        {
            return AlreadyStopped;
        }
        case JOINING:
        case DEAD:
        {
            return AlreadyJoined;
        }
        case EXTERNAL:
        {
            return IsExternalThread;
        }
        case ERROR:
        {
            return Error;
        }
    }
    return Error;
}

ThreadState::Rc ThreadState::stop()
{
    mStateLock.Lock();
    switch (mCurrentState)
    {
        case EXTERNALJOINING:
        case EXTERNALJOINED:
        {
            mStateLock.Unlock();
            return Error;
        }
        case INITIAL:
        {
            mStateLock.Unlock();
            return InInitialState;
        }
        case STARTING:
        {
            while (mCurrentState != RUNNING)
            {
                /*** if the state machine went one step further already
                  ** we can be sure that the process is handled by another thread
                  ***/
                if (mCurrentState == STOPPING || mCurrentState == STOPPED || 
                    mCurrentState == JOINING || mCurrentState == DEAD)
                {
                    mStateLock.Unlock();
                    return StopAlreadyHandled;
                }
                mStateCondition.Wait(mStateLock);
            }
        }
        // fall through
        case RUNNING:
        {
            mCurrentState = STOPPING;
            mStateLock.Unlock();
            return Ok;
        }
        case STOPPING:
        case STOPPED:
        {
            mStateLock.Unlock();
            return AlreadyStopped;
        }
        case JOINING:
        case DEAD:
        {
            mStateLock.Unlock();
            return AlreadyJoined;
        }
        case EXTERNAL:
        {
            mStateLock.Unlock();
            return IsExternalThread;
        }
        case ERROR:
        {
            mStateLock.Unlock();
            return Error;
        }
    }
    return Error;
}

ThreadState::Rc ThreadState::stopped()
{
    ScopedMutexLock lock(mStateLock);
    switch (mCurrentState)
    {
        case INITIAL:
        case STARTING:
        case RUNNING:
        case EXTERNALJOINING:
        case EXTERNALJOINED:
        {
            return Error;
        }
        case STOPPING:
        {
            mCurrentState = STOPPED;
            mStateCondition.Broadcast();
            return Ok;
        }
        case STOPPED:
        {
            return AlreadyStopped;
        }
        case JOINING:
        case DEAD:
        {
            return AlreadyJoined;
        }
        case EXTERNAL:
        {
            return IsExternalThread;
        }
        case ERROR:
        {
            return Error;
        }
    }
    return Error;
}

ThreadState::Rc ThreadState::join()
{
    mStateLock.Lock();
    switch (mCurrentState)
    {
        case INITIAL:
        {
            mStateLock.Unlock();
            return Error;
        }
        case STARTING:  
        case RUNNING:
        case STOPPING:
        {
            while (mCurrentState != STOPPED)
            {
                /*** if the state machine went one step further already
                  ** we can be sure that the process is handled by another thread
                  ***/
                if (mCurrentState == JOINING || mCurrentState == DEAD)
                {
                    mStateLock.Unlock();
                    return JoinAlreadyHandled;
                }
                mStateCondition.Wait(mStateLock);
            }
        }
        // fall through
        case STOPPED:
        {
            mCurrentState = JOINING;
            mStateLock.Unlock();
            return Ok;
        }
        case JOINING:
        case DEAD:
        {
            mStateLock.Unlock();
            return AlreadyJoined;
        }
        case EXTERNAL:
        {
            mCurrentState = EXTERNALJOINING;
            mStateLock.Unlock();
            return Ok;
        }
        case EXTERNALJOINING:
        case EXTERNALJOINED:
        {
            mStateLock.Unlock();
            return AlreadyJoined;
        }
        case ERROR:
        {
            mStateLock.Unlock();
            return Error;
        }    
    }
    return Error;
}

ThreadState::Rc ThreadState::joined()
{
    ScopedMutexLock lock(mStateLock);
    switch (mCurrentState)
    {
        case INITIAL:
        case STARTING:
        case RUNNING:
        case STOPPING:
        case STOPPED:
        case EXTERNAL:
        case EXTERNALJOINED:
        {
            return Error;
        }
        case JOINING:
        {
            mCurrentState = DEAD;
            return Ok;
        }
        case DEAD:
        {
            return AlreadyJoined;
        }
        case EXTERNALJOINING:
        {
            mCurrentState = EXTERNALJOINED;
            return Ok;
        }
        case ERROR:
        {
            return Error;
        }
    }
    return Error;
}

ThreadState::Rc ThreadState::error()
{
    ScopedMutexLock lock(mStateLock);
    mCurrentState = ERROR;
    return Ok;
}

} //namespace qcc
