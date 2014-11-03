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

#include <NativeStorage.h>
#include <NativeStorageSettings.h>
#include <qcc/Crypto.h>
#include <qcc/Debug.h>
#include <qcc/GUID.h>
#include <qcc/X509Certificate.h>

#define QCC_MODULE "SEC_MGR"

namespace ajn {
namespace securitymgr {
#define LOGSQLERROR(a) { QCC_LogError((a), ((qcc::String("SQL Error: ") + (sqlite3_errmsg(nativeStorageDB))).c_str())); \
}

QStatus NativeStorage::StoreApplication(const ManagedApplicationInfo& managedApplicationInfo, const bool update)
{
    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement;
    QStatus funcStatus = ER_OK;
    qcc::String sqlStmtText;
    qcc::String updateStr = "?";

    if (update) {
        sqlStmtText = "INSERT OR REPLACE INTO ";
        updateStr = "(SELECT APPLICATION_PUBKEY FROM ";
        updateStr.append(CLAIMED_APPS_TABLE_NAME);
        updateStr.append(" WHERE APPLICATION_PUBKEY = ?)");
    } else {
        sqlStmtText = "INSERT INTO ";
    }

    sqlStmtText.append(CLAIMED_APPS_TABLE_NAME);

    sqlStmtText.append(
        " (APPLICATION_PUBKEY, APP_NAME, PEER_ID, DEV_NAME, USER_DEF_NAME, MANIFEST, POLICY) VALUES (" + updateStr +
        ", ?, ?, ?, ?, ?, ?)");

    if (managedApplicationInfo.peerID.empty()) {
        QCC_LogError(ER_FAIL, ("Empty peer ID !"));
        return ER_FAIL;
    }

    uint8_t publicKey[qcc::ECC_COORDINATE_SZ + qcc::ECC_COORDINATE_SZ];
    size_t size = sizeof(publicKey);
    funcStatus = managedApplicationInfo.publicKey.Export(publicKey, &size);
    if (funcStatus != ER_OK) {
        QCC_LogError(funcStatus, ("Failed to export public key"));
        return funcStatus;
    }

    do {
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            LOGSQLERROR(ER_FAIL);
            funcStatus = ER_FAIL;
            break;
        }

        sqlRetCode = sqlite3_bind_blob(statement, 1,
                                       publicKey, qcc::ECC_COORDINATE_SZ + qcc::ECC_COORDINATE_SZ,
                                       SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_text(statement, 2,
                                        managedApplicationInfo.appName.c_str(), -1, SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_text(statement, 3,
                                        managedApplicationInfo.peerID.c_str(), -1, SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_text(statement, 4,
                                        managedApplicationInfo.deviceName.c_str(), -1,
                                        SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_text(statement, 5,
                                        managedApplicationInfo.userDefinedName.c_str(), -1,
                                        SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_blob(statement, 6,
                                        managedApplicationInfo.manifest.data(),
                                        managedApplicationInfo.manifest.size(), SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_blob(statement, 7,
                                        managedApplicationInfo.policy.data(),
                                        managedApplicationInfo.policy.size(), SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            LOGSQLERROR(ER_FAIL);
            funcStatus = ER_FAIL;
            break;
        }
    } while (0);

    funcStatus = StepAndFinalizeSqlStmt(statement);

    return funcStatus;
}

QStatus NativeStorage::RemoveApplication(const ManagedApplicationInfo& managedApplicationInfo)
{
    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement = NULL;
    const char* sqlStmtText = NULL;
    QStatus funcStatus = ER_OK;

    do {
        sqlStmtText =
            "DELETE FROM " CLAIMED_APPS_TABLE_NAME " WHERE APPLICATION_PUBKEY = ?";
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText, -1,
                                        &statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            LOGSQLERROR(ER_FAIL);
            funcStatus = ER_FAIL;
            break;
        }

        uint8_t publicKey[qcc::ECC_COORDINATE_SZ + qcc::ECC_COORDINATE_SZ];
        size_t size = sizeof(publicKey);
        funcStatus = managedApplicationInfo.publicKey.Export(publicKey, &size);
        if (funcStatus != ER_OK) {
            QCC_LogError(funcStatus, ("Failed to export public key"));
            break;
        }
        sqlRetCode = sqlite3_bind_blob(statement, 1,
                                       publicKey, qcc::ECC_COORDINATE_SZ + qcc::ECC_COORDINATE_SZ,
                                       SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            LOGSQLERROR(ER_FAIL);
            funcStatus = ER_FAIL;
            break;
        }
    } while (0);

    funcStatus = StepAndFinalizeSqlStmt(statement);

    return funcStatus;
}

QStatus NativeStorage::GetManagedApplications(std::vector<ManagedApplicationInfo>& managedApplications) const
{
    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement = NULL;
    qcc::String sqlStmtText = "";
    QStatus funcStatus = ER_OK;

    sqlStmtText = "SELECT * FROM ";
    sqlStmtText.append(CLAIMED_APPS_TABLE_NAME);

    /* prepare the sql query */
    sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(), -1,
                                    &statement, NULL);
    if (SQLITE_OK != sqlRetCode) {
        LOGSQLERROR(ER_FAIL);
        funcStatus = ER_FAIL;
    }

    /* iterate over all the rows in the query */
    while (SQLITE_ROW == (sqlRetCode = sqlite3_step(statement))) {
        ManagedApplicationInfo info;
        Keys keys;

        info.publicKey.Import((const uint8_t*)sqlite3_column_blob(statement,
                                                                  0), qcc::ECC_COORDINATE_SZ +
                              qcc::ECC_COORDINATE_SZ);

        info.appName.assign(
            (const char*)sqlite3_column_text(statement, 1));
        info.peerID.assign(
            (const char*)sqlite3_column_text(statement, 2));
        info.deviceName.assign(
            (const char*)sqlite3_column_text(statement, 3));
        info.userDefinedName.assign(
            (const char*)sqlite3_column_text(statement, 4));

        keys.appECCPublicKey = &(info.publicKey);
        keys.guildID = NULL;
        info.manifest = qcc::String(
            (const char*)sqlite3_column_blob(statement, 5),
            GetBlobSize(CLAIMED_APPS_TABLE_NAME, "MANIFEST", &keys));
        info.policy = qcc::String((const char*)sqlite3_column_blob(statement,
                                                                   6),
                                  (size_t)GetBlobSize(CLAIMED_APPS_TABLE_NAME, "POLICY", &keys));

        managedApplications.push_back(info);
    }

    sqlRetCode = sqlite3_finalize(statement);
    if (SQLITE_OK != sqlRetCode) {
        LOGSQLERROR(ER_FAIL);;
        funcStatus = ER_FAIL;
    }

    return funcStatus;
}

QStatus NativeStorage::InitSerialNumber()
{
    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement = NULL;
    qcc::String sqlStmtText = "";
    QStatus funcStatus = ER_OK;

    sqlStmtText = "SELECT VALUE FROM ";
    sqlStmtText.append(SERIALNUMBER_TABLE_NAME);

    /* prepare the sql query */
    sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(), -1,
                                    &statement, NULL);
    if (SQLITE_OK != sqlRetCode) {
        LOGSQLERROR(ER_FAIL);
        funcStatus = ER_FAIL;
        return funcStatus;
    }

    /* iterate over all the rows in the query */
    if (SQLITE_ROW == (sqlRetCode = sqlite3_step(statement))) {
        sqlRetCode = sqlite3_finalize(statement);
    } else {
        //insert a single entry with the initial serial number.
        sqlRetCode = sqlite3_finalize(statement);
        sqlStmtText = "INSERT INTO ";
        sqlStmtText.append(SERIALNUMBER_TABLE_NAME);
        sqlStmtText.append(" (VALUE) VALUES (?)");
        statement = NULL;
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, NULL);
        sqlRetCode |= sqlite3_bind_int(statement, 1, INITIAL_SERIAL_NUMBER);
        funcStatus = StepAndFinalizeSqlStmt(statement);
    }
    if (SQLITE_OK != sqlRetCode) {
        LOGSQLERROR(ER_FAIL);
        funcStatus = ER_FAIL;
    }
    return funcStatus;
}

QStatus NativeStorage::GetNewSerialNumber(qcc::String& serialNumber) const
{
    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement = NULL;
    qcc::String sqlStmtText = "";
    QStatus funcStatus = ER_OK;
    sqlStmtText = "SELECT * FROM ";
    sqlStmtText.append(SERIALNUMBER_TABLE_NAME);

    /* prepare the sql query */
    sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(), -1,
                                    &statement, NULL);
    if (SQLITE_OK != sqlRetCode) {
        LOGSQLERROR(ER_FAIL);
        funcStatus = ER_FAIL;
        return funcStatus;
    }

    /* iterate over all the rows in the query */
    if (SQLITE_ROW == (sqlRetCode = sqlite3_step(statement))) {
        int value = sqlite3_column_int(statement, 0);
        sqlRetCode = sqlite3_finalize(statement);
        char buffer[33];
        snprintf(buffer, 32, "%x", value);
        buffer[32] = 0; //make sure we have a trailing 0.
        serialNumber.clear();
        serialNumber.append(buffer);

        //update the value in the database.
        sqlStmtText = "UPDATE ";
        sqlStmtText.append(SERIALNUMBER_TABLE_NAME);
        sqlStmtText.append(" SET VALUE = ?");
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(), -1,
                                        &statement, NULL);

        sqlRetCode |= sqlite3_bind_int(statement, 1, value + 1);
        funcStatus = StepAndFinalizeSqlStmt(statement);

        if (SQLITE_OK != sqlRetCode) {
            LOGSQLERROR(ER_FAIL);
            funcStatus = ER_FAIL;
        }
    } else {
        LOGSQLERROR(ER_FAIL);
        funcStatus = ER_FAIL;
    }
    return funcStatus;
}

QStatus NativeStorage::GetManagedApplication(ManagedApplicationInfo& managedApplicationInfo) const
{
    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement = NULL;
    qcc::String sqlStmtText = "";
    QStatus funcStatus = ER_OK;

    do {
        sqlStmtText = "SELECT * FROM ";
        sqlStmtText.append(CLAIMED_APPS_TABLE_NAME);
        sqlStmtText.append(" WHERE APPLICATION_PUBKEY LIKE ?");

        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            break;
        }

        uint8_t publicKey[qcc::ECC_COORDINATE_SZ + qcc::ECC_COORDINATE_SZ];
        size_t size = sizeof(publicKey);
        funcStatus = managedApplicationInfo.publicKey.Export(publicKey, &size);
        if (funcStatus != ER_OK) {
            QCC_LogError(funcStatus, ("Failed to export public key"));
            break;
        }
        sqlRetCode = sqlite3_bind_blob(statement, 1, publicKey,
                                       qcc::ECC_COORDINATE_SZ + qcc::ECC_COORDINATE_SZ, SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            break;
        }
        sqlRetCode = sqlite3_step(statement);
        if (sqlRetCode == SQLITE_ROW) {
            Keys keys;

            managedApplicationInfo.appName.assign(
                (const char*)sqlite3_column_text(statement, 1));
            managedApplicationInfo.peerID.assign((const char*)sqlite3_column_text(statement, 2));
            managedApplicationInfo.deviceName.assign(
                (const char*)sqlite3_column_text(statement, 3));
            managedApplicationInfo.userDefinedName.assign(
                (const char*)sqlite3_column_text(statement, 4));

            keys.appECCPublicKey = &(managedApplicationInfo.publicKey);
            keys.guildID = NULL;
            managedApplicationInfo.manifest = qcc::String(
                (const char*)sqlite3_column_blob(statement, 5),
                GetBlobSize(CLAIMED_APPS_TABLE_NAME, "MANIFEST", &keys));
            managedApplicationInfo.policy = qcc::String((const char*)sqlite3_column_blob(statement,
                                                                                         6),
                                                        (size_t)GetBlobSize(CLAIMED_APPS_TABLE_NAME, "POLICY", &keys));
        } else {
            QCC_LogError(ER_FAIL, ("Error in getting entry from database !"));
            funcStatus = ER_FAIL;
            break;
        }
    } while (0);

    if (ER_OK != funcStatus) {
        LOGSQLERROR(funcStatus);
    }

    sqlRetCode = sqlite3_finalize(statement);
    if (SQLITE_OK != sqlRetCode) {
        QCC_LogError(ER_FAIL, ("Error on finalizing SQL statement !"));
        funcStatus = ER_FAIL;
    }

    return funcStatus;
}

QStatus NativeStorage::StoreCertificate(const qcc::Certificate& certificate,
                                        const bool update)
{
    sqlite3_stmt* statement = NULL;
    qcc::String sqlStmtText = "";
    QStatus funcStatus = ER_OK;

    if (update) {
        sqlStmtText = "INSERT OR REPLACE INTO ";
    } else {
        sqlStmtText = "INSERT INTO ";
    }

    switch (dynamic_cast<const qcc::X509CertificateECC&>(certificate).GetType()) {
    case qcc::IDENTITY_CERTIFICATE: {
            sqlStmtText.append(IDENTITY_CERTS_TABLE_NAME);
            sqlStmtText.append(" (SUBJECT, VERSION, ISSUER"
                               ", VALIDITYFROM"
                               ", VALIDITYTO"
                               ", SN"
                               ", DATAID"
                               ", ALIAS"
                               ",USERNAME) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
        }
        break;

    case qcc::MEMBERSHIP_CERTIFICATE: {
            sqlStmtText.append(MEMBERSHIP_CERTS_TABLE_NAME);
            sqlStmtText.append(" (SUBJECT, VERSION, ISSUER"
                               ", VALIDITYFROM"
                               ", VALIDITYTO"
                               ", SN"
                               ", DATAID"
                               ", DELEGATE"
                               ", GUID) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
        }
        break;

    case qcc::USER_EQUIVALENCE_CERTIFICATE: {
            sqlStmtText.append(USER_EQ_CERTS_TABLE_NAME);
            sqlStmtText.append(" (SUBJECT, VERSION, ISSUER"
                               ", VALIDITYFROM"
                               ", VALIDITYTO"
                               ", SN) VALUES (?, ?, ?, ?, ?, ?)");
        }
        break;

    default: {
            QCC_LogError(ER_FAIL, ("Unsupported certificate type !"));
            return ER_FAIL;
        }
    }

    funcStatus = BindCertForStorage(certificate, sqlStmtText.c_str(),
                                    &statement);
    funcStatus = StepAndFinalizeSqlStmt(statement);

    return funcStatus;
}

QStatus NativeStorage::StoreAssociatedData(const qcc::Certificate& certificate,
                                           const qcc::String& data, bool update)
{
    sqlite3_stmt* statement = NULL;
    qcc::String sqlStmtText = "";
    QStatus funcStatus = ER_OK;
    const char* dataId = NULL;
    const std::size_t dataIdSize = qcc::Crypto_SHA256::DIGEST_SIZE;         //Or should we rely on data.size() ?
    int sqlRetCode = SQLITE_OK;
    qcc::String updateStr = "?";

    dataId =
        dynamic_cast<const qcc::X509CertificateECC&>(certificate).GetDataDigest().data();

    if (data.empty() || (NULL == dataId)) {
        QCC_LogError(ER_FAIL, ("NULL data argument"));
        return ER_FAIL;
    }

    if (update) {
        sqlStmtText = "INSERT OR REPLACE INTO ";
// TODO: this select doesn't work. Do we really need?
//        updateStr = "(SELECT ID FROM ";
//        updateStr.append(CERTSDATA_TABLE_NAME);
//        updateStr.append(" WHERE ID = ?)");
    } else {
        sqlStmtText = "INSERT INTO ";
    }
    sqlStmtText.append(CERTSDATA_TABLE_NAME);

    sqlStmtText.append(" (ID, DATA) VALUES (" + updateStr + ", ?)");

    do {
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            break;
        }

        sqlRetCode = sqlite3_bind_blob(statement, 1, dataId, dataIdSize,
                                       SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_blob(statement, 2, data.data(), data.size(),
                                        SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            break;
        }
    } while (0);

    funcStatus = StepAndFinalizeSqlStmt(statement);

    return funcStatus;
}

QStatus NativeStorage::GetCertificate(qcc::Certificate& certificate)
{
    sqlite3_stmt* statement = NULL;
    int sqlRetCode = SQLITE_OK;
    QStatus funcStatus = ER_OK;
    qcc::String sqlStmtText = "";
    qcc::String tableName = "";
    const qcc::ECCPublicKey* appECCPublicKey = NULL;
    qcc::String guildId = "";

    qcc::X509CertificateECC& cert =
        dynamic_cast<qcc::X509CertificateECC&>(certificate);
    appECCPublicKey = cert.GetSubject();

    if (NULL == appECCPublicKey) {
        QCC_LogError(ER_FAIL, ("Null application public key."));
        return ER_FAIL;
    }

    sqlStmtText = "SELECT * FROM ";
    do {
        switch (cert.GetType()) {
        case qcc::IDENTITY_CERTIFICATE: {
                tableName = IDENTITY_CERTS_TABLE_NAME;
                sqlStmtText += IDENTITY_CERTS_TABLE_NAME;
                sqlStmtText += " WHERE SUBJECT = ? ";
            }
            break;

        case qcc::MEMBERSHIP_CERTIFICATE: {
                tableName = MEMBERSHIP_CERTS_TABLE_NAME;
                sqlStmtText += MEMBERSHIP_CERTS_TABLE_NAME;
                sqlStmtText += " WHERE SUBJECT = ? AND GUID = ? ";
                guildId =
                    dynamic_cast<qcc::X509MemberShipCertificate&>(certificate).GetGuildId();
            }
            break;

        case qcc::USER_EQUIVALENCE_CERTIFICATE: {
                tableName = USER_EQ_CERTS_TABLE_NAME;
                sqlStmtText += USER_EQ_CERTS_TABLE_NAME;
                sqlStmtText += " WHERE SUBJECT = ? ";
            }
            break;

        default: {
                QCC_LogError(ER_FAIL, ("Unsupported certificate type !"));
                return ER_FAIL;
            }
        }

        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            LOGSQLERROR(ER_FAIL);
            funcStatus = ER_FAIL;
            break;
        }

        uint8_t publicKey[qcc::ECC_COORDINATE_SZ + qcc::ECC_COORDINATE_SZ];
        size_t size = sizeof(publicKey);
        funcStatus = appECCPublicKey->Export(publicKey, &size);
        if (funcStatus != ER_OK) {
            QCC_LogError(funcStatus, ("Failed to export public key"));
            break;
        }
        sqlRetCode = sqlite3_bind_blob(statement, 1, publicKey,
                                       sizeof(publicKey), SQLITE_TRANSIENT);

        if (qcc::MEMBERSHIP_CERTIFICATE == cert.GetType()) {
            sqlRetCode |= sqlite3_bind_text(statement, 2, guildId.c_str(), -1,
                                            SQLITE_TRANSIENT);
        }

        if (SQLITE_ROW == (sqlRetCode = sqlite3_step(statement))) {
            int column = 1;             // Starting at Version
            Keys keys;
            keys.appECCPublicKey = appECCPublicKey;
            keys.guildID = NULL;
            ECCPublicKey issuer;
            qcc::Certificate::ValidPeriod validity;

            /*********************Common to all certificates*****************/
            cert.SetVersion(sqlite3_column_int(statement, column));

            issuer.Import((const uint8_t*)sqlite3_column_blob(statement,
                                                              ++column), qcc::ECC_COORDINATE_SZ +
                          qcc::ECC_COORDINATE_SZ);
            cert.SetIssuer(&issuer);

            validity.validFrom = sqlite3_column_int64(statement, ++column);
            validity.validTo = sqlite3_column_int64(statement, ++column);
            cert.SetValidity(&validity);

            if (qcc::MEMBERSHIP_CERTIFICATE
                == cert.GetType()) {
                keys.guildID = &guildId;
            }

            cert.SetSerialNumber(
                qcc::String(
                    (const char*)sqlite3_column_blob(statement,
                                                     ++column),
                    GetBlobSize(tableName.c_str(), "SN", &keys)));

            if (qcc::USER_EQUIVALENCE_CERTIFICATE
                != cert.GetType()) {
                cert.SetDataDigest(
                    qcc::String(
                        (const char*)sqlite3_column_blob(statement,
                                                         ++column),
                        GetBlobSize(tableName.c_str(), "DATAID",
                                    &keys)));
            }

            /****************************************************************/

            switch (cert.GetType()) {
            case qcc::IDENTITY_CERTIFICATE: {
                    qcc::X509IdentityCertificate& idCert = dynamic_cast<qcc::X509IdentityCertificate&>(certificate);
                    idCert.SetAlias(
                        qcc::String(
                            (const char*)sqlite3_column_blob(statement,
                                                             ++column),
                            GetBlobSize(IDENTITY_CERTS_TABLE_NAME, "ALIAS",
                                        &keys)));
                    idCert.SetName((const char*)sqlite3_column_text(statement, ++column));
                    certificate = dynamic_cast<qcc::Certificate&>(idCert);
                }
                break;

            case qcc::MEMBERSHIP_CERTIFICATE: {
                    qcc::X509MemberShipCertificate&  memCert =
                        dynamic_cast<qcc::X509MemberShipCertificate&>(certificate);
                    memCert.SetDelegate(sqlite3_column_int(statement, ++column));

                    certificate = dynamic_cast<qcc::Certificate&>(memCert);
                }
                break;

            case qcc::USER_EQUIVALENCE_CERTIFICATE: {
                    //Nothing special to set for now so cast back to Certificate&
                    certificate = dynamic_cast<qcc::Certificate&>(cert);
                }
                break;

            default: {
                    QCC_LogError(ER_FAIL, ("Unsupported certificate type !"));
                    return ER_FAIL;
                }
            }
        } else {
            funcStatus = ER_FAIL;
        }
    } while (0);

    sqlRetCode = sqlite3_finalize(statement);
    if (SQLITE_OK != sqlRetCode) {
        LOGSQLERROR(ER_FAIL);;
        funcStatus = ER_FAIL;
    }

    return funcStatus;
}

QStatus NativeStorage::GetAssociatedData(const qcc::Certificate& certificate,
                                         qcc::String& data) const
{
    sqlite3_stmt* statement = NULL;
    qcc::String sqlStmtText = "";
    QStatus funcStatus = ER_OK;
    const char* dataId = NULL;
    int sqlRetCode = SQLITE_OK;
    const std::size_t dataIdSize = qcc::Crypto_SHA256::DIGEST_SIZE;         // Or should we rely on data.size() ?

    dataId =
        dynamic_cast<const qcc::X509CertificateECC&>(certificate).GetDataDigest().data();

    sqlStmtText = "SELECT LENGTH(DATA), DATA FROM ";
    sqlStmtText.append(CERTSDATA_TABLE_NAME);
    sqlStmtText.append(" WHERE ID = ?");
    do {
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            LOGSQLERROR(ER_FAIL);
            funcStatus = ER_FAIL;
            break;
        }

        sqlRetCode = sqlite3_bind_blob(statement, 1, dataId, dataIdSize,
                                       SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            LOGSQLERROR(ER_FAIL);
            funcStatus = ER_FAIL;
            break;
        }

        sqlRetCode = sqlite3_step(statement);
        if (sqlRetCode == SQLITE_ROW) {
            int dataSize = sqlite3_column_int(statement, 0);
            data = qcc::String((const char*)sqlite3_column_blob(statement, 1), dataSize);
        } else {
            LOGSQLERROR(ER_FAIL);
            funcStatus = ER_FAIL;
            break;
        }
    } while (0);

    sqlRetCode = sqlite3_finalize(statement);
    if (SQLITE_OK != sqlRetCode) {
        LOGSQLERROR(funcStatus);
    }

    return funcStatus;
}

QStatus NativeStorage::RemoveCertificate(qcc::Certificate& certificate)
{
    sqlite3_stmt* statement = NULL;
    qcc::String sqlStmtText = "";
    int sqlRetCode = SQLITE_OK;
    QStatus funcStatus = ER_OK;
    qcc::String certTableName = "";
    qcc::String whereKeys = "";
    const qcc::ECCPublicKey* appECCPublicKey = NULL;

    qcc::X509CertificateECC& cert =
        dynamic_cast<qcc::X509CertificateECC&>(certificate);
    appECCPublicKey = cert.GetSubject();

    if (NULL == appECCPublicKey) {
        QCC_LogError(ER_FAIL, ("Null application public key."));
        return ER_FAIL;
    }

    switch (cert.GetType()) {
    case qcc::IDENTITY_CERTIFICATE: {
            certTableName = IDENTITY_CERTS_TABLE_NAME;
            whereKeys = " WHERE SUBJECT = ? ";
        }
        break;

    case qcc::MEMBERSHIP_CERTIFICATE: {
            certTableName = MEMBERSHIP_CERTS_TABLE_NAME;
            whereKeys = " WHERE SUBJECT = ? AND GUID = ? ";
        }
        break;

    case qcc::USER_EQUIVALENCE_CERTIFICATE: {
            certTableName = USER_EQ_CERTS_TABLE_NAME;
            whereKeys = " WHERE SUBJECT = ? ";
        }
        break;

    default: {
            QCC_LogError(ER_FAIL, ("Unsupported certificate type !"));
            return ER_FAIL;
        }
    }

    do {
        sqlStmtText = "DELETE FROM ";
        sqlStmtText.append(certTableName);
        sqlStmtText.append(whereKeys);

        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            break;
        }

        uint8_t publicKey[qcc::ECC_COORDINATE_SZ + qcc::ECC_COORDINATE_SZ];
        size_t size = sizeof(publicKey);
        funcStatus = appECCPublicKey->Export(publicKey, &size);
        if (funcStatus != ER_OK) {
            QCC_LogError(funcStatus, ("Failed to export public key"));
            break;
        }
        sqlRetCode = sqlite3_bind_blob(statement, 1, publicKey,
                                       sizeof(publicKey), SQLITE_TRANSIENT);
        if (qcc::MEMBERSHIP_CERTIFICATE == cert.GetType()) {
            sqlRetCode |=
                sqlite3_bind_text(statement, 2,
                                  dynamic_cast<const qcc::X509MemberShipCertificate&>(certificate).GetGuildId().
                                  c_str(),
                                  -1, SQLITE_TRANSIENT);
        }

        if (SQLITE_OK != sqlRetCode) {
            LOGSQLERROR(ER_FAIL);
            funcStatus = ER_FAIL;
            break;
        }
    } while (0);

    funcStatus = StepAndFinalizeSqlStmt(statement);

    return funcStatus;
}

QStatus NativeStorage::RemoveAssociatedData(const qcc::Certificate& certificate)
{
    sqlite3_stmt* statement = NULL;
    qcc::String sqlStmtText = "";
    QStatus funcStatus = ER_OK;
    const char* dataId = NULL;
    int sqlRetCode = SQLITE_OK;
    const std::size_t dataIdSize = qcc::Crypto_SHA256::DIGEST_SIZE;         // Or should we rely on data.size() ?

    dataId =
        dynamic_cast<const qcc::X509CertificateECC&>(certificate).GetDataDigest().data();

    if (NULL == dataId) {
        QCC_LogError(ER_FAIL, ("Null data ID."));
        return ER_FAIL;
    }

    sqlStmtText = "DELETE FROM ";
    sqlStmtText.append(CERTSDATA_TABLE_NAME);
    sqlStmtText.append(" WHERE ID = ?");
    do {
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            LOGSQLERROR(ER_FAIL);
            funcStatus = ER_FAIL;
            break;
        }

        sqlRetCode = sqlite3_bind_blob(statement, 1, dataId, dataIdSize,
                                       SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            LOGSQLERROR(ER_FAIL);
            funcStatus = ER_FAIL;
            break;
        }
    } while (0);

    funcStatus = StepAndFinalizeSqlStmt(statement);

    return funcStatus;
}

QStatus NativeStorage::StoreGuild(const GuildInfo& guildInfo, const bool update)
{
    return StoreItem(ITEM_GUILD, guildInfo.guid, guildInfo.name, guildInfo.desc, update);
}

QStatus NativeStorage::RemoveGuild(const GUID128& guildId)
{
    GuildInfo tmp;
    tmp.guid = guildId;
    if (ER_OK != GetGuild(tmp)) {
        QCC_LogError(ER_FAIL, ("Guild does not exist."));
        return ER_FAIL;
    }
    return RemoveItem(ITEM_GUILD, guildId);
}

QStatus NativeStorage::GetGuild(GuildInfo& guildInfo) const
{
    sqlite3_stmt* statement = NULL;
    qcc::String sqlStmtText = "";
    QStatus funcStatus = ER_OK;
    int sqlRetCode = SQLITE_OK;

    sqlStmtText = "SELECT GUILD_NAME, GUILD_DESC FROM ";
    sqlStmtText.append(GUILDS_TABLE_NAME);
    sqlStmtText.append(" WHERE ID LIKE ?");
    do {
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            LOGSQLERROR(ER_FAIL);
            funcStatus = ER_FAIL;
            break;
        }

        sqlRetCode = sqlite3_bind_text(statement, 1, guildInfo.guid.ToString().c_str(), -1,
                                       SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            LOGSQLERROR(ER_FAIL);
            funcStatus = ER_FAIL;
            break;
        }

        sqlRetCode = sqlite3_step(statement);
        if (sqlRetCode == SQLITE_ROW) {
            guildInfo.name.assign((const char*)sqlite3_column_text(statement, 0));
            guildInfo.desc.assign((const char*)sqlite3_column_text(statement, 1));
        } else if (SQLITE_DONE != sqlRetCode) {
            LOGSQLERROR(ER_FAIL);
            funcStatus = ER_FAIL;
            break;
        } else {  //SQLITE_DONE but no data
            funcStatus = ER_FAIL;
        }       break;
    } while (0);

    sqlRetCode = sqlite3_finalize(statement);
    if (SQLITE_OK != sqlRetCode) {
        LOGSQLERROR(funcStatus);
        funcStatus = ER_FAIL;
    }

    return funcStatus;
}

QStatus NativeStorage::GetManagedGuilds(std::vector<GuildInfo>& guildsInfo) const
{
    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement = NULL;
    qcc::String sqlStmtText = "";
    QStatus funcStatus = ER_OK;

    sqlStmtText = "SELECT * FROM ";
    sqlStmtText.append(GUILDS_TABLE_NAME);

    /* Prepare the sql query */
    sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(), -1,
                                    &statement, NULL);
    if (SQLITE_OK != sqlRetCode) {
        LOGSQLERROR(ER_FAIL);
        funcStatus = ER_FAIL;
    }

    /* Iterate over all the rows in the query */
    while (SQLITE_ROW == (sqlRetCode = sqlite3_step(statement))) {
        GuildInfo info;

        info.guid = qcc::GUID128((const char*)sqlite3_column_text(statement, 0));
        info.name.assign((const char*)sqlite3_column_text(statement, 1));
        info.desc.assign((const char*)sqlite3_column_text(statement, 2));
        guildsInfo.push_back(info);
    }

    sqlRetCode = sqlite3_finalize(statement);
    if (SQLITE_OK != sqlRetCode) {
        LOGSQLERROR(ER_FAIL);;
        funcStatus = ER_FAIL;
    }

    return funcStatus;
}

QStatus NativeStorage::StoreIdentity(const IdentityInfo& idInfo,
                                     const bool update)
{
    return StoreItem(ITEM_IDENTITY, idInfo.guid, idInfo.name, qcc::String(""), update);
}

QStatus NativeStorage::RemoveIdentity(const GUID128& identityID)
{
    IdentityInfo tmp;
    tmp.guid = identityID;
    if (ER_OK != GetIdentity(tmp)) {
        QCC_LogError(ER_FAIL, ("Identity does not exist."));
        return ER_FAIL;
    }
    return RemoveItem(ITEM_IDENTITY, identityID);
}

QStatus NativeStorage::GetIdentity(IdentityInfo& idInfo) const
{
    sqlite3_stmt* statement = NULL;
    qcc::String sqlStmtText = "";
    QStatus funcStatus = ER_OK;
    int sqlRetCode = SQLITE_OK;

    sqlStmtText = "SELECT ID_NAME FROM ";
    sqlStmtText.append(IDENTITY_TABLE_NAME);
    sqlStmtText.append(" WHERE ID LIKE ?");
    do {
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            LOGSQLERROR(ER_FAIL);
            funcStatus = ER_FAIL;
            break;
        }

        sqlRetCode = sqlite3_bind_text(statement, 1, idInfo.guid.ToString().c_str(), -1,
                                       SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            LOGSQLERROR(ER_FAIL);
            funcStatus = ER_FAIL;
            break;
        }

        sqlRetCode = sqlite3_step(statement);
        if (sqlRetCode == SQLITE_ROW) {
            idInfo.name.assign((const char*)sqlite3_column_text(statement, 0));
        } else if (SQLITE_DONE != sqlRetCode) {
            LOGSQLERROR(ER_FAIL);
            funcStatus = ER_FAIL;
            break;
        } else {  //SQLITE_DONE but no data
            funcStatus = ER_FAIL;
        }       break;
    } while (0);

    sqlRetCode = sqlite3_finalize(statement);
    if (SQLITE_OK != sqlRetCode) {
        LOGSQLERROR(funcStatus);
        funcStatus = ER_FAIL;
    }

    return funcStatus;
}

QStatus NativeStorage::GetManagedIdentities(std::vector<IdentityInfo>& idInfos) const
{
    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement = NULL;
    qcc::String sqlStmtText = "";
    QStatus funcStatus = ER_OK;

    sqlStmtText = "SELECT * FROM ";
    sqlStmtText.append(IDENTITY_TABLE_NAME);

    /* Prepare the sql query */
    sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(), -1,
                                    &statement, NULL);
    if (SQLITE_OK != sqlRetCode) {
        LOGSQLERROR(ER_FAIL);
        funcStatus = ER_FAIL;
    }

    /* Iterate over all the rows in the query */
    while (SQLITE_ROW == (sqlRetCode = sqlite3_step(statement))) {
        IdentityInfo info;

        info.guid = qcc::GUID128((const char*)sqlite3_column_text(statement, 0));
        info.name.assign((const char*)sqlite3_column_text(statement, 1));
        idInfos.push_back(info);
    }

    sqlRetCode = sqlite3_finalize(statement);
    if (SQLITE_OK != sqlRetCode) {
        LOGSQLERROR(ER_FAIL);;
        funcStatus = ER_FAIL;
    }

    return funcStatus;
}

void NativeStorage::Reset()
{
    sqlite3_close(nativeStorageDB);
    remove(GetStoragePath().c_str());
}

NativeStorage::~NativeStorage()
{
    int sqlRetCode;
    if ((sqlRetCode = sqlite3_close(nativeStorageDB)) != SQLITE_OK) {                                   //TODO :: change to sqlite3_close_v2 once Jenkins machines allow for it
        LOGSQLERROR(ER_FAIL);
    }
}

/*************************************************PRIVATE*********************************************************/

QStatus NativeStorage::BindCertForStorage(const qcc::Certificate& certificate,
                                          const char* sqlStmtText, sqlite3_stmt*
                                          * statement)
{
    int sqlRetCode = SQLITE_OK;                                        // Equal zero
    QStatus funcStatus = ER_OK;
    int column = 1;

    //TODO Very bad casting to non-const but as long as some get functions are not defined as const in CertificateECC then
    //	   there's no other option.
    qcc::X509CertificateECC& cert =
        dynamic_cast<qcc::X509CertificateECC&>((qcc::Certificate &)certificate);

    do {
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText, -1,
                                        statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            break;
        }

        /*********************Common to all certificates*****************/
        uint8_t publicKey[qcc::ECC_COORDINATE_SZ + qcc::ECC_COORDINATE_SZ];
        size_t size = sizeof(publicKey);
        funcStatus = cert.GetSubject()->Export(publicKey, &size);
        if (funcStatus != ER_OK) {
            QCC_LogError(funcStatus, ("Failed to export public key"));
            break;
        }

        sqlRetCode = sqlite3_bind_blob(*statement, column,
                                       publicKey, sizeof(publicKey),
                                       SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_int(*statement, ++column, cert.GetVersion());

        uint8_t publicKeyIssuer[qcc::ECC_COORDINATE_SZ + qcc::ECC_COORDINATE_SZ];
        size = sizeof(publicKey);
        funcStatus = cert.GetIssuer()->Export(publicKey, &size);
        if (funcStatus != ER_OK) {
            QCC_LogError(funcStatus, ("Failed to export public key"));
            break;
        }

        sqlRetCode |= sqlite3_bind_blob(*statement, ++column,
                                        publicKeyIssuer,
                                        sizeof(publicKeyIssuer),
                                        SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_int64(*statement, ++column,
                                         cert.GetValidity()->validFrom);
        sqlRetCode |= sqlite3_bind_int64(*statement, ++column,
                                         cert.GetValidity()->validTo);
        sqlRetCode |= sqlite3_bind_blob(*statement, ++column,
                                        cert.GetSerialNumber().data(),
                                        cert.GetSerialNumber().size(),
                                        SQLITE_TRANSIENT);

        if (qcc::USER_EQUIVALENCE_CERTIFICATE
            != cert.GetType()) {
            sqlRetCode |= sqlite3_bind_blob(*statement, ++column,
                                            cert.GetDataDigest().data(),
                                            cert.GetDataDigest().size(),
                                            SQLITE_TRANSIENT);
        }
        /****************************************************************/

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            break;
        }

        switch (cert.GetType()) {
        case qcc::IDENTITY_CERTIFICATE: {
                const qcc::X509IdentityCertificate& idCert =
                    dynamic_cast<const qcc::X509IdentityCertificate&>(cert);
                qcc::String alias = idCert.GetAlias().ToString();
                sqlRetCode |= sqlite3_bind_blob(*statement, ++column,
                                                alias.data(),
                                                alias.size(),
                                                SQLITE_TRANSIENT);
                sqlRetCode |= sqlite3_bind_text(*statement, ++column,
                                                idCert.GetName().c_str(), -1,
                                                SQLITE_TRANSIENT);

                if (SQLITE_OK != sqlRetCode) {
                    funcStatus = ER_FAIL;
                }
            }
            break;

        case qcc::MEMBERSHIP_CERTIFICATE: {
                qcc::X509MemberShipCertificate& memCert =
                    dynamic_cast<qcc::X509MemberShipCertificate&>(cert);

                sqlRetCode |= sqlite3_bind_int(*statement, ++column,
                                               memCert.IsDelegate());
                sqlRetCode |= sqlite3_bind_text(*statement, ++column,
                                                memCert.GetGuildId().c_str(), -1,
                                                SQLITE_TRANSIENT);

                if (SQLITE_OK != sqlRetCode) {
                    funcStatus = ER_FAIL;
                }
            }
            break;

        case qcc::USER_EQUIVALENCE_CERTIFICATE: {
                //Nothing extra for now
            }
            break;

        default: {
                QCC_LogError(ER_FAIL, ("Unsupported certificate type !"));
                return ER_FAIL;
            }
        }
    } while (0);

    if (ER_OK != funcStatus) {
        LOGSQLERROR(funcStatus);
    }

    return funcStatus;
}

QStatus NativeStorage::StepAndFinalizeSqlStmt(sqlite3_stmt* statement) const
{
    int sqlRetCode = SQLITE_OK;
    QStatus funcStatus = ER_OK;

    if (NULL == statement) {
        QCC_LogError(ER_FAIL, ("NULL statement argument"));
        return ER_FAIL;
    }

    sqlRetCode = sqlite3_step(statement);

    if (SQLITE_DONE != sqlRetCode) {
        LOGSQLERROR(ER_FAIL);
        funcStatus = ER_FAIL;
    }

    sqlRetCode = sqlite3_finalize(statement);
    if (SQLITE_OK != sqlRetCode) {
        LOGSQLERROR(ER_FAIL);;
        funcStatus = ER_FAIL;
    }

    if (ER_OK != funcStatus) {
        LOGSQLERROR(funcStatus);
    }

    return funcStatus;
}

qcc::String NativeStorage::GetStoragePath() const
{
    qcc::String storagePath(DEFAULT_STORAGE_PATH);
    qcc::String pathkey("STORAGE_PATH");
    std::map<qcc::String, qcc::String>::const_iterator pathItr =
        storageConfig.settings.find(pathkey);
    if (pathItr != storageConfig.settings.end()) {
        storagePath = pathItr->second;
    }

    return storagePath;
}

QStatus NativeStorage::Init()
{
    int sqlRetCode = SQLITE_OK;
    qcc::String sqlStmtText = "";
    QStatus funcStatus = ER_OK;

    do {
        qcc::String storagePath = GetStoragePath();
        sqlRetCode = sqlite3_open(storagePath.c_str(), &nativeStorageDB);

        if (SQLITE_OK != sqlRetCode) {
            LOGSQLERROR(ER_FAIL);
            sqlite3_close(nativeStorageDB);
            funcStatus = ER_FAIL;
            break;
        }

        sqlStmtText = CLAIMED_APPLICATIONS_TABLE_SCHEMA;
        sqlStmtText.append(IDENTITY_CERTS_TABLE_SCHEMA);
        sqlStmtText.append(MEMBERSHIP_CERTS_TABLE_SCHEMA);
        sqlStmtText.append(USER_EQ_CERTS_TABLE_SCHEMA);
        sqlStmtText.append(CERTSDATA_TABLE_SCHEMA);
        sqlStmtText.append(GUILDS_TABLE_SCHEMA);
        sqlStmtText.append(IDENTITY_TABLE_SCHEMA);
        sqlStmtText.append(SERIALNUMBER_TABLE_SCHEMA);
        sqlStmtText.append(DEFAULT_PRAGMAS);

        sqlRetCode = sqlite3_exec(nativeStorageDB, sqlStmtText.c_str(), NULL, 0,
                                  NULL);

        if (SQLITE_OK != sqlRetCode) {
            LOGSQLERROR(ER_FAIL);
            funcStatus = ER_FAIL;
            break;
        }
        funcStatus = InitSerialNumber();
    } while (0);

    return funcStatus;
}

int NativeStorage::GetBlobSize(const char* table, const char* columnName,
                               const Keys* keys) const
{
    int sqlRetCode = SQLITE_OK;
    qcc::String sqlStmtText = "";
    sqlite3_stmt* statement = NULL;
    int size = 0;

    do {
        if ((NULL == keys->appECCPublicKey) || (NULL == table)
            || (NULL == columnName)) {
            QCC_LogError(ER_FAIL, ("Null argument"));
            break;
        }

        sqlStmtText += "SELECT LENGTH(" + qcc::String(columnName) + ")"
                       + " FROM " + qcc::String(table) + " WHERE ";

        if (qcc::String(table).compare(MEMBERSHIP_CERTS_TABLE_NAME) == 0) {
            if (NULL == keys->guildID) {
                break;
            }
            sqlStmtText += "SUBJECT = ? AND GUID = ? ";
        } else if (qcc::String(table).compare(CLAIMED_APPS_TABLE_NAME) == 0) {
            sqlStmtText += "APPLICATION_PUBKEY = ? ";
        } else {
            sqlStmtText += "SUBJECT = ?";
        }

        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            LOGSQLERROR(ER_FAIL);
            break;
        }

        uint8_t publicKey[qcc::ECC_COORDINATE_SZ + qcc::ECC_COORDINATE_SZ];
        size_t ksize = sizeof(publicKey);
        QStatus funcStatus = keys->appECCPublicKey->Export(publicKey, &ksize);
        if (funcStatus != ER_OK) {
            QCC_LogError(funcStatus, ("Failed to export public key"));
            sqlRetCode = SQLITE_ABORT;
            break;
        }

        sqlRetCode = sqlite3_bind_blob(statement, 1,
                                       publicKey, sizeof(publicKey),
                                       SQLITE_TRANSIENT);

        if (NULL != keys->guildID) {
            sqlRetCode = sqlite3_bind_text(statement, 2, keys->guildID->c_str(),
                                           -1, SQLITE_TRANSIENT);
        }

        if (SQLITE_OK != sqlRetCode) {
            LOGSQLERROR(ER_FAIL);
            break;
        }
        sqlRetCode = sqlite3_step(statement);
        if (SQLITE_ROW == sqlRetCode) {
            size = sqlite3_column_int(statement, 0);
        } else if (SQLITE_DONE != sqlRetCode) {
            LOGSQLERROR(ER_FAIL);
            break;
        }
    } while (0);

    sqlRetCode = sqlite3_finalize(statement);
    if (SQLITE_OK != sqlRetCode) {
        LOGSQLERROR(ER_FAIL);
    }

    return size;
}

QStatus NativeStorage::StoreItem(ItemType type,
                                 const GUID128& guid,
                                 const qcc::String& name,
                                 const qcc::String& description,
                                 bool update)
{
    const char* tableName = type == ITEM_GUILD ? GUILDS_TABLE_NAME : IDENTITY_TABLE_NAME;

    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement;
    QStatus funcStatus = ER_OK;
    qcc::String sqlStmtText("");
    qcc::String updateStr("?");
    qcc::String suffix = type == ITEM_GUILD ? ", ?, ?)" : ", ?)";

    if (update) {
        sqlStmtText.append("INSERT OR REPLACE INTO ");
        updateStr = "(SELECT ID FROM ";
        updateStr.append(tableName);
        updateStr.append(" WHERE ID = ?)");
    } else {
        sqlStmtText.append("INSERT INTO ");
    }

    sqlStmtText.append(tableName);
    qcc::String fields = type == ITEM_GUILD ? "GUILD_NAME, GUILD_DESC" : "ID_NAME";
    sqlStmtText.append(" (ID, " + fields + ") VALUES (" + updateStr + suffix);

    if (guid.ToString().empty()) {
        QCC_LogError(ER_FAIL, ("Empty GUID !"));
        return ER_FAIL;
    }

    do {
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            LOGSQLERROR(ER_FAIL);
            funcStatus = ER_FAIL;
            break;
        }

        sqlRetCode |= sqlite3_bind_text(statement, 1,
                                        guid.ToString().c_str(), -1, SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_text(statement, 2,
                                        name.c_str(), -1, SQLITE_TRANSIENT);
        if (type == ITEM_GUILD) {
            sqlRetCode |= sqlite3_bind_text(statement, 3,
                                            description.c_str(), -1,
                                            SQLITE_TRANSIENT);
        }
        if (SQLITE_OK != sqlRetCode) {
            LOGSQLERROR(ER_FAIL);
            funcStatus = ER_FAIL;
            break;
        }
    } while (0);

    funcStatus = StepAndFinalizeSqlStmt(statement);

    return funcStatus;
}

QStatus NativeStorage::RemoveItem(ItemType type, const qcc::GUID128& guid)
{
    sqlite3_stmt* statement = NULL;
    qcc::String sqlStmtText = "";
    QStatus funcStatus = ER_OK;
    int sqlRetCode = SQLITE_OK;

    sqlStmtText = "DELETE FROM ";
    sqlStmtText.append(type == ITEM_GUILD ? GUILDS_TABLE_NAME : IDENTITY_TABLE_NAME);
    sqlStmtText.append(" WHERE ID LIKE ?");

    do {
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            LOGSQLERROR(ER_FAIL);
            funcStatus = ER_FAIL;
            break;
        }

        sqlRetCode = sqlite3_bind_text(statement, 1, guid.ToString().c_str(), -1,
                                       SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            LOGSQLERROR(ER_FAIL);
            funcStatus = ER_FAIL;
            break;
        }
    } while (0);

    funcStatus = StepAndFinalizeSqlStmt(statement);

    return funcStatus;
}
}
}
#undef QCC_MODULE
