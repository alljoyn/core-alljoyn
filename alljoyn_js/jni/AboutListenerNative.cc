/*
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
 */
#include "AboutListenerNative.h"

#include "BusAttachmentHost.h"
#include "TypeMapping.h"
#include <qcc/Debug.h>

#define QCC_MODULE "ALLJOYN_JS"

AboutListenerNative::AboutListenerNative(Plugin& plugin, NPObject* objectValue) :
    NativeObject(plugin, objectValue)
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}

AboutListenerNative::~AboutListenerNative ()
{
    QCC_DbgTrace(("%s", __FUNCTION__));
}


void AboutListenerNative::onAnnounced(const qcc::String& busName, uint16_t version, ajn::SessionPort port, const ajn::MsgArg& objectDescriptionArg, const ajn::MsgArg& aboutDataArg)
{
    QCC_DbgTrace(("%s(busname=%s,version=%d,port=%d)", __FUNCTION__, busName.c_str(), version, port));
	NPIdentifier onAnnounced = NPN_GetStringIdentifier("onAnnounced");
	if(NPN_HasMethod(plugin->npp, objectValue, onAnnounced)) {
		NPVariant npargs[5];
		ToDOMString(plugin, busName, npargs[0]);
		ToUnsignedShort(plugin, version, npargs[1]);
		ToUnsignedShort(plugin, port, npargs[2]);

		QStatus status = ER_OK;
		ToAny(plugin, objectDescriptionArg, npargs[3], status);
		assert(status == ER_OK);
		ToAny(plugin, aboutDataArg, npargs[4], status);
		assert(status == ER_OK);

		NPVariant result = NPVARIANT_VOID;
		NPN_Invoke(plugin->npp, objectValue, onAnnounced, npargs, 5, &result);
		NPN_ReleaseVariantValue(&result);

		NPN_ReleaseVariantValue(&npargs[4]);
		NPN_ReleaseVariantValue(&npargs[3]);
		NPN_ReleaseVariantValue(&npargs[0]);
	}
}
