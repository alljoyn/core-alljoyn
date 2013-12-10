/**
 * @file
 * Constants used in the Bluetooth transport code.
 */

/******************************************************************************
 * Copyright (c) 2011, AllSeen Alliance. All rights reserved.
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
#ifndef _ALLJOYN_BTTRANSPORTCONSTS_H
#define _ALLJOYN_BTTRANSPORTCONSTS_H

#include <qcc/platform.h>

namespace ajn {

namespace bt {

const uint16_t INVALID_PSM = 0;          /**< Invalid L2CAP PSM value */
const uint16_t INCOMING_PSM = 0xffff;    /**< PSM value to indicate incomming connection */
const uint32_t INVALID_UUIDREV = 0;      /**< Invalide UUID revision value */

/** Bluetooth connection roles */
enum BluetoothRole {
    MASTER,
    SLAVE
};

} // namespace bt
} // namespace ajn



#endif
