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

#include <alljoyn/securitymgr/Util.h>

#define QCC_MODULE "SECMGR_AGENT"

using namespace qcc;

namespace ajn {
namespace securitymgr {
/**Util Impl*/

BusAttachment* Util::ba = nullptr;
Mutex Util::mutex;

QStatus Util::Init(BusAttachment* _ba)
{
    mutex.Lock(__FILE__, __LINE__);
    QStatus status;
    if (_ba != nullptr && _ba->IsStarted() && _ba->IsConnected()) {
        ba = _ba;
        status = ER_OK;
    } else {
        ba = nullptr;
        status = ER_FAIL;
    }
    mutex.Unlock(__FILE__, __LINE__);

    return status;
}

QStatus Util::Fini()
{
    mutex.Lock(__FILE__, __LINE__);
    ba = nullptr;
    mutex.Unlock(__FILE__, __LINE__);
    return ER_OK;
}

DefaultPolicyMarshaller* Util::GetDefaultMarshaller(Message** msg)
{
    mutex.Lock(__FILE__, __LINE__);
    if (!msg) {
        mutex.Unlock(__FILE__, __LINE__);
        return nullptr;
    }
    if (!ba) {
        *msg = nullptr;
        mutex.Unlock(__FILE__, __LINE__);
        return nullptr;
    }
    *msg = new Message(*ba);
    mutex.Unlock(__FILE__, __LINE__);
    return new DefaultPolicyMarshaller(**msg);
}

QStatus Util::GetPolicyByteArray(const PermissionPolicy& policy, uint8_t** byteArray, size_t* size)
{
    mutex.Lock(__FILE__, __LINE__);

    if (!byteArray) {
        mutex.Unlock(__FILE__, __LINE__);
        return ER_BAD_ARG_2;
    }

    if (!size) {
        mutex.Unlock(__FILE__, __LINE__);
        return ER_BAD_ARG_3;
    }

    Message* msg = nullptr;
    DefaultPolicyMarshaller* marshaller = Util::GetDefaultMarshaller(&msg);
    QStatus status = ER_FAIL;

    if (marshaller) {
        status = const_cast<PermissionPolicy&>(policy).Export(*marshaller, byteArray, size);

        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to serialize policy"));
        }

        delete msg;
        msg = nullptr;
        delete marshaller;
        marshaller = nullptr;
    }
    mutex.Unlock(__FILE__, __LINE__);
    return status;
}

QStatus Util::GetPolicy(const uint8_t* byteArray, const size_t size, PermissionPolicy& policy)
{
    mutex.Lock(__FILE__, __LINE__);

    if (!byteArray) {
        mutex.Unlock(__FILE__, __LINE__);
        return ER_BAD_ARG_1;
    }

    if (size == 0) {
        mutex.Unlock(__FILE__, __LINE__);
        return ER_BAD_ARG_2;
    }

    Message* msg = nullptr;
    DefaultPolicyMarshaller* marshaller = Util::GetDefaultMarshaller(&msg);
    QStatus status = ER_FAIL;

    if (marshaller && byteArray) {
        status = policy.Import(*marshaller, byteArray, size);

        if (ER_OK != status) {
            QCC_LogError(status, ("Failed to de-serialize policy"));
        }
        delete msg;
        msg = nullptr;
        delete marshaller;
        marshaller = nullptr;
    }
    mutex.Unlock(__FILE__, __LINE__);
    return status;
}
}
}
#undef QCC_MODULE
