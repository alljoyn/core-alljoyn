/*
 * ActiveObject.cc
 *
 *  Created on: May 14, 2015
 *      Author: erongo
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

#include "ActiveObject.h"

#include <assert.h>

namespace nio {

ActiveObject::ActiveObject(uint32_t num_threads) : m_running(true)
{
    assert(num_threads > 0);
    m_threads.resize(num_threads);

    for (size_t i = 0; i < m_threads.size(); ++i) {
        m_threads[i] = std::thread(&ActiveObject::Run, this);
    }
}

ActiveObject::~ActiveObject()
{
    Stop();
}


void ActiveObject::Dispatch(Function f)
{
    if (m_running) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_closures.push(f);
        m_condition.notify_one();
    }
}

void ActiveObject::Stop()
{
    if (m_running.exchange(false)) {
        m_condition.notify_all();
        for (size_t i = 0; i < m_threads.size(); ++i) {
            m_threads[i].join();
        }
    }
}

void ActiveObject::Run()
{
    while (m_running) {
        Function closure;

        {
            std::unique_lock<std::mutex> lock(m_mutex);

            //printf("ActiveObject::Run(): Wait for work\n");
            while (m_running == true && m_closures.empty()) {
                m_condition.wait(lock);
            }

            if (!m_running) {
                break;
            }

            // TODO: check the size of m_closures()

            closure = m_closures.front();
            m_closures.pop();
        }

        closure();
    }
}

} /* namespace nio */
