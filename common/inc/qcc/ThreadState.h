/**
 * @file
 * Implements a thread safe thread state calss
 *
 */
/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#ifndef _QCC_THREAD_STATE_H
#define _QCC_THREAD_STATE_H

#include <qcc/Mutex.h>
#include <qcc/Condition.h>

namespace qcc {

/**
 * Abstract encapsulation of the thread state
 * State graph:
 *
 *                                        ------------->call stop>------------------------>|
 *                                        | (must wait until state==RUNNING)               |
 *                                        |                                                |
 *        ************              ************                ***********             ************
 *  ----> * INITIAL  * >call start> * STARTING * >call started> * RUNNING * >call stop> * STOPPING *
 *        ************              ************                ***********             ************
 *                                    |      |                          |                  |   |
 *                                    |      |<-------------------------|<-----------------|   |
 *                                    |      |                                                 |
 *                                    |      |-------------|                                   |
 *                               >call start>         >call join>                         <call stopped<
 *                                    |             (must wait until state==STOPPED)           |
 *                                    |                    |--------|                          |
 *                                    |                             |                          |
 *                                  ************                ***********             ************
 *                                  *    DEAD  * <call joined<  * JOINING * <call join< * STOPPED  *
 *                                  ************                ***********             ************
 *
 *  ####################################################################################
 *
 *        ************              *******************               ******************
 *  ----> * EXTERNAL * >call join > * EXTERNALJOINING * >call joined> * EXTERNALJOINED *
 *        ************              *******************               ******************
 **/
class ThreadState {
  public:
    /**
     * Enumeration of thread states.
     */
    enum State {
        INITIAL,  /**< Initial thread state - no underlying OS thread */
        STARTING, /**< Thread is currently starting but not started */
        RUNNING,  /**< Thread is running the thread function */
        STOPPING, /**< Thread has completed the thread function and is cleaning up */
        STOPPED,  /**< Thread is stopped and cleaned but not joined */
        JOINING,  /**< Thread is currently joining */
        DEAD,     /**< Underlying OS thread is gone */
        CRITICALERROR,    /**< An critical error has been occurred, the thread is dead. */
        EXTERNAL,  /**< External thread, no handling allowed */
        EXTERNALJOINING,
        EXTERNALJOINED
    };

    enum Rc {
        Ok,
        InInitialState, /**< not started at all*/
        AlreadyRunning,
        AlreadyStopped,
        StopAlreadyHandled,
        AlreadyJoined,
        JoinAlreadyHandled,
        IsExternalThread,
        Error
    };
    /**
     *
     **/
    ThreadState(bool isExternal = false);

    /** Get the current state
     * @param no param
     * @return current state value
     **/
    State getCurrentState() const;

    /** Returns true if started as external
     * @param no param
     * @return current state value
     **/
    bool isExternal() const;

    /** Trigger state change to starting
     * @param no param
     * @return return value
     **/
    Rc start();

    /** Trigger state change to started/running
     * @param no param
     * @return return value
     **/
    Rc started();

    /** Trigger state change to stopping
     * @param no param
     * @return return value
     **/
    Rc stop();

    /** Trigger state change to stopped
     * @param no param
     * @return return value
     **/
    Rc stopped();

    /** Trigger state change to joining
     * @param no param
     * @return return value
     **/
    Rc join();

    /** Trigger state change to dead
     * @param no param
     * @return return value
     **/
    Rc joined();

    /** Trigger state change to dead
     * @param no param
     * @return return value
     **/
    Rc error();

  private:
    mutable Mutex mStateLock;
    State mCurrentState;
    Condition mStateCondition;
};

} //namespace qcc

#endif
