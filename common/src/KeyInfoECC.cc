/**
 * @file KeyInfoECC.cc
 *
 * Class for ECC Public Key Info utilities
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
#include <qcc/KeyInfoECC.h>
#include <qcc/StringUtil.h>

#include <Status.h>

using namespace std;

namespace qcc {

#define QCC_MODULE "CRYPTO"

const size_t KeyInfo::GetExportSize() const
{
    return sizeof(FormatType) + sizeof(uint32_t) + keyIdLen * sizeof(uint8_t);
}

QStatus KeyInfo::Export(uint8_t* buf) const
{
    memcpy(buf, &format, sizeof(FormatType));
    buf += sizeof(FormatType);
    memcpy(buf, &keyIdLen, sizeof(uint32_t));
    buf += sizeof(uint32_t);
    if (keyIdLen > 0) {
        memcpy(buf, keyId, keyIdLen);
    }
    return ER_OK;
}

QStatus KeyInfo::Import(const uint8_t* buf, size_t count)
{
    if (count < GetExportSize()) {
        return ER_INVALID_DATA;
    }
    uint8_t* p = (uint8_t*) buf;
    FormatType srcFormat;
    memcpy(&srcFormat, p, sizeof(FormatType));
    if (srcFormat != format) {
        return ER_INVALID_DATA;
    }
    p += sizeof(FormatType);
    uint32_t len;
    memcpy(&len, p, sizeof(uint32_t));
    p += sizeof(uint32_t);
    SetKeyId(p, len);
    return ER_OK;
}

const size_t KeyInfoECC::GetExportSize() const
{
    return KeyInfo::GetExportSize() + sizeof(uint8_t);
}

QStatus KeyInfoECC::Export(uint8_t* buf) const
{
    KeyInfo::Export(buf);
    buf += KeyInfo::GetExportSize();
    memcpy(buf, &curve, sizeof(curve));
    return ER_OK;
}

QStatus KeyInfoECC::Import(const uint8_t* buf, size_t count)
{
    if (count < GetExportSize()) {
        return ER_INVALID_DATA;
    }
    KeyInfo::Import(buf, count);
    uint8_t* p = (uint8_t*) buf;
    p += KeyInfo::GetExportSize();
    memcpy(&curve, p, sizeof(curve));
    return ER_OK;
}

qcc::String KeyInfoECC::ToString(size_t indent) const
{
    qcc::String str;
    qcc::String in = qcc::String(indent, ' ');
    str += in + "<keyInfo>\n";
    str += in + "  <format>" + U32ToString(GetFormat()) + "</format>\n";
    str += in + "  <algorithm>" + U32ToString(GetAlgorithm()) + "</algorithm>\n";
    str += in + "  <curve>" + U32ToString(GetCurve()) + "</curve>\n";
    if (GetKeyIdLen() > 0) {
        str += in + "  <id>" + BytesToHexString(GetKeyId(), GetKeyIdLen()) + "</id>\n";
    }
    str += in + "</keyInfo>\n";
    return str;
}

qcc::String KeyInfoNISTP256::ToString(size_t indent) const
{
    qcc::String str;
    qcc::String in = qcc::String(indent, ' ');
    str += in + "<NISP256KeyInfo>\n";
    str += in + "  <format>" + U32ToString(GetFormat()) + "</format>\n";
    str += in + "  <algorithm>" + U32ToString(GetAlgorithm()) + "</algorithm>\n";
    str += in + "  <curve>" + U32ToString(GetCurve()) + "</curve>\n";
    if (GetKeyIdLen() > 0) {
        str += in + "  <id>" + BytesToHexString(GetKeyId(), GetKeyIdLen()) + "</id>\n";
    }
    str += in + "  <publickey>";
    str += BytesToHexString((const uint8_t*) GetPublicKey(), sizeof(ECCPublicKey));
    str += "</publickey>\n";
    str += in + "</NISP256KeyInfo>\n";
    return str;
}

const size_t KeyInfoNISTP256::GetExportSize() const
{
    return KeyInfoECC::GetExportSize() + sizeof(pubkey.form) + pubkey.key.GetSize();
}

QStatus KeyInfoNISTP256::Export(uint8_t* buf) const
{
    KeyInfoECC::Export(buf);
    buf += KeyInfoECC::GetExportSize();
    memcpy(buf, &pubkey.form, sizeof(uint8_t));
    buf += sizeof(uint8_t);
    size_t keySize = pubkey.key.GetSize();
    QStatus status = pubkey.key.Export(buf, &keySize);
    if (ER_OK != status) {
        return status;
    }
    if (keySize != pubkey.key.GetSize()) {
        return ER_INVALID_DATA;
    }
    return ER_OK;
}

QStatus KeyInfoNISTP256::Import(const uint8_t* buf, size_t count)
{
    if (count < GetExportSize()) {
        return ER_INVALID_DATA;
    }
    KeyInfoECC::Import(buf, count);
    uint8_t* p = (uint8_t*) buf;
    p += KeyInfoECC::GetExportSize();
    p += sizeof(uint8_t);
    return pubkey.key.Import(p, pubkey.key.GetSize());
}

} /* namespace qcc */
