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

#define VALID_RULES_ELEMENT \
    "<rules>" \
    "<node>" \
    "<interface>" \
    "<method>" \
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>" \
    "</method>" \
    "<property>" \
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>" \
    "</property>" \
    "<signal>" \
    "<annotation name = \"org.alljoyn.Bus.Action\" value = \"Provide\"/>" \
    "</signal>" \
    "</interface>" \
    "</node>" \
    "</rules>"

static AJ_PCSTR s_validManifest =
    "<manifest>"
    "<version>1</version>"
    VALID_RULES_ELEMENT
    "<thumbprint>"
    "<oid>2.16.840.1.101.3.4.2.1</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</thumbprint>"
    "<signature>"
    "<oid>1.2.840.10045.4.3.2</oid>"
    "<value>NjY2ZjZmNjI2MTcy</value>"
    "</signature>"
    "</manifest>";