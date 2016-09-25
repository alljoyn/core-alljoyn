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
 *
 ******************************************************************************/
#ifndef _ALLJOYN_JAPPLICATIONSTATELISTENER_H
#define _ALLJOYN_JAPPLICATIONSTATELISTENER_H

#include <jni.h>
#include <alljoyn/ApplicationStateListener.h>

class JApplicationStateListener : public ajn::ApplicationStateListener {
  public:

    JApplicationStateListener(jobject jlistener);
    ~JApplicationStateListener();
    void State(const char* busName, const qcc::KeyInfoNISTP256& publicKeyInfo, ajn::PermissionConfigurator::ApplicationState state);
    jobject jasListener;

  private:

    jmethodID MID_state;
};

#endif
