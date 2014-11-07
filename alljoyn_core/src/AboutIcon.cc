/******************************************************************************
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

#include <alljoyn/AboutIcon.h>
namespace ajn {

AboutIcon::AboutIcon() : content(NULL), contentSize(0), mimetype(""), url(""), m_arg() {
    //empty constructor
}

QStatus AboutIcon::SetContent(const char* mimetype, uint8_t* data, size_t csize, bool ownsData)
{
    QStatus status = ER_OK;
    status = m_arg.Set("ay", csize, data);
    if (status != ER_OK) {
        return status;
    }
    this->mimetype = mimetype;
    this->content = data;
    this->contentSize = csize;

    if (ownsData) {
        m_arg.SetOwnershipFlags(MsgArg::OwnsData);
    }
    return status;
}

QStatus AboutIcon::SetUrl(const char* mimetype, const char* url)
{
    this->mimetype = mimetype;
    this->url = url;
    return ER_OK;
}

void AboutIcon::Clear() {
    m_arg.Clear();
    content = NULL;
    contentSize = 0;
    mimetype = "";
    url = "";
}

QStatus AboutIcon::SetContent(const MsgArg& arg) {
    m_arg = arg;
    m_arg.Stabilize();
    return m_arg.Get("ay", &contentSize, &content);
}
}
