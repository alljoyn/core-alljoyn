/******************************************************************************
 * Copyright (c) AllSeen Alliance. All rights reserved.
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

#include <SQLStorage.h>
#include <SQLStorageSettings.h>
#include <qcc/Crypto.h>
#include <qcc/Debug.h>
#include <qcc/GUID.h>
#include <qcc/CertificateECC.h>

#define QCC_MODULE "SEC_MGR"

namespace ajn {
namespace securitymgr {
#define LOGSQLERROR(a) { QCC_LogError((a), ((qcc::String("SQL Error: ") + (sqlite3_errmsg(nativeStorageDB))).c_str())); \
}

QStatus SQLStorage::StoreApplication(const ManagedApplicationInfo& managedApplicationInfo, const bool update)
{
    storageMutex.Lock(__FILE__, __LINE__);

    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement;
    QStatus funcStatus = ER_OK;
    qcc::String sqlStmtText;
    int keyPosition = 1;
    int othersStartPosition = 1;

    if (update) {
        sqlStmtText = "UPDATE ";
        sqlStmtText.append(CLAIMED_APPS_TABLE_NAME);
        sqlStmtText.append(
            " SET APP_NAME = ?, PEER_ID = ?, DEV_NAME = ?, USER_DEF_NAME = ?, MANIFEST = ?, POLICY = ?, UPDATES_PENDING = ? WHERE APPLICATION_PUBKEY = ?");
        keyPosition = 8;
    } else {
        sqlStmtText = "INSERT INTO ";
        sqlStmtText.append(CLAIMED_APPS_TABLE_NAME);
        sqlStmtText.append(
            " (APPLICATION_PUBKEY, APP_NAME, PEER_ID, DEV_NAME, USER_DEF_NAME, MANIFEST, POLICY, UPDATES_PENDING) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
        othersStartPosition++;
    }

    if (managedApplicationInfo.peerID.empty()) {
        funcStatus = ER_FAIL;
        QCC_LogError(funcStatus, ("Empty peer ID !"));
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }

    uint8_t publicKey[qcc::ECC_COORDINATE_SZ + qcc::ECC_COORDINATE_SZ];
    size_t size = sizeof(publicKey);
    funcStatus = managedApplicationInfo.publicKey.Export(publicKey, &size);
    if (funcStatus != ER_OK) {
        QCC_LogError(funcStatus, ("Failed to export public key"));
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }
    do {
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }

        sqlRetCode = sqlite3_bind_blob(statement, keyPosition,
                                       publicKey, qcc::ECC_COORDINATE_SZ + qcc::ECC_COORDINATE_SZ,
                                       SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_text(statement, othersStartPosition,
                                        managedApplicationInfo.appName.c_str(), -1, SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_text(statement, ++othersStartPosition,
                                        managedApplicationInfo.peerID.c_str(), -1, SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_text(statement, ++othersStartPosition,
                                        managedApplicationInfo.deviceName.c_str(), -1,
                                        SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_text(statement, ++othersStartPosition,
                                        managedApplicationInfo.userDefinedName.c_str(), -1,
                                        SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_blob(statement, ++othersStartPosition,
                                        managedApplicationInfo.manifest.data(),
                                        managedApplicationInfo.manifest.size(), SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_blob(statement, ++othersStartPosition,
                                        managedApplicationInfo.policy.data(),
                                        managedApplicationInfo.policy.size(), SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_int(statement,
                                       ++othersStartPosition,
                                       (const bool)managedApplicationInfo.updatesPending);

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }
    } while (0);

    funcStatus = StepAndFinalizeSqlStmt(statement);

    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

QStatus SQLStorage::RemoveApplication(const ManagedApplicationInfo& managedApplicationInfo)
{
    storageMutex.Lock(__FILE__, __LINE__);

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
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
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
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }
    } while (0);

    funcStatus = StepAndFinalizeSqlStmt(statement);

    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

QStatus SQLStorage::GetManagedApplications(std::vector<ManagedApplicationInfo>& managedApplications) const
{
    storageMutex.Lock(__FILE__, __LINE__);

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
        funcStatus = ER_FAIL;
        LOGSQLERROR(funcStatus);
    }

    /* iterate over all the rows in the query */
    while (SQLITE_ROW == (sqlRetCode = sqlite3_step(statement))) {
        ManagedApplicationInfo managedApplicationInfo;
        Keys keys;

        managedApplicationInfo.publicKey.Import((const uint8_t*)sqlite3_column_blob(statement,
                                                                                    0), qcc::ECC_COORDINATE_SZ +
                                                qcc::ECC_COORDINATE_SZ);

        managedApplicationInfo.appName.assign(
            (const char*)sqlite3_column_text(statement, 1));
        managedApplicationInfo.peerID.assign(
            (const char*)sqlite3_column_text(statement, 2));
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
        managedApplicationInfo.updatesPending = sqlite3_column_int(statement, 7);
        managedApplications.push_back(managedApplicationInfo);
    }

    sqlRetCode = sqlite3_finalize(statement);
    if (SQLITE_OK != sqlRetCode) {
        funcStatus = ER_FAIL;
        LOGSQLERROR(funcStatus);
    }

    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

QStatus SQLStorage::InitSerialNumber()
{
    storageMutex.Lock(__FILE__, __LINE__);

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
        funcStatus = ER_FAIL;
        LOGSQLERROR(funcStatus);
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }

    if (SQLITE_ROW == (sqlRetCode = sqlite3_step(statement))) {
        sqlRetCode = sqlite3_finalize(statement);
    } else if (SQLITE_DONE == sqlRetCode) {
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
        funcStatus = ER_FAIL;
        LOGSQLERROR(funcStatus);
    }

    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

QStatus SQLStorage::GetNewSerialNumber(qcc::String& serialNumber) const
{
    storageMutex.Lock(__FILE__, __LINE__);

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
        funcStatus = ER_FAIL;
        LOGSQLERROR(funcStatus);
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }

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
    } else if (SQLITE_DONE == sqlRetCode) {
        funcStatus = ER_END_OF_DATA;
        QCC_LogError(ER_END_OF_DATA, ("Serial number was not initialized!"));
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }

    if (SQLITE_OK != sqlRetCode) {
        funcStatus = ER_FAIL;
        LOGSQLERROR(funcStatus);
    }

    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

QStatus SQLStorage::GetManagedApplication(ManagedApplicationInfo& managedApplicationInfo) const
{
    storageMutex.Lock(__FILE__, __LINE__);

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
            LOGSQLERROR(funcStatus);
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
            LOGSQLERROR(funcStatus);
            break;
        }
        sqlRetCode = sqlite3_step(statement);
        if (SQLITE_ROW == sqlRetCode) {
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
            managedApplicationInfo.updatesPending = sqlite3_column_int(statement, 7);
        } else if (SQLITE_DONE == sqlRetCode) {
            QCC_DbgHLPrintf(("No managed application was found !"));
            funcStatus = ER_END_OF_DATA;
            break;
        } else {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }
    } while (0);

    sqlRetCode = sqlite3_finalize(statement);
    if (SQLITE_OK != sqlRetCode) {
        funcStatus = ER_FAIL;
        LOGSQLERROR(funcStatus);
    }

    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

QStatus SQLStorage::StoreCertificate(const qcc::CertificateX509& certificate,
                                     const bool update)
{
    storageMutex.Lock(__FILE__, __LINE__);

    sqlite3_stmt* statement = NULL;
    qcc::String sqlStmtText = "";
    QStatus funcStatus = ER_OK;

    if (update) {
        sqlStmtText = "INSERT OR REPLACE INTO ";
    } else {
        sqlStmtText = "INSERT INTO ";
    }

    switch (certificate.GetType()) {
    case qcc::CertificateX509::IDENTITY_CERTIFICATE: {
            sqlStmtText.append(IDENTITY_CERTS_TABLE_NAME);
            sqlStmtText.append(" (SUBJECT, VERSION, ISSUER"
                               ", VALIDITYFROM"
                               ", VALIDITYTO"
                               ", SN"
                               ", DATAID"
                               ", ALIAS"
                               ", USERNAME) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
        }
        break;

    case qcc::CertificateX509::MEMBERSHIP_CERTIFICATE: {
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

    default: {
            QCC_LogError(ER_FAIL, ("Unsupported certificate type !"));
            storageMutex.Unlock(__FILE__, __LINE__);
            return ER_FAIL;
        }
    }

    funcStatus = BindCertForStorage(certificate, sqlStmtText.c_str(),
                                    &statement);

    if (ER_OK != funcStatus) {
        QCC_LogError(funcStatus, ("Binding values of certificate for storage has failed"));
    }

    funcStatus = StepAndFinalizeSqlStmt(statement);

    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

QStatus SQLStorage::StoreAssociatedData(const qcc::CertificateX509& certificate,
                                        const qcc::String& data, bool update)
{
    storageMutex.Lock(__FILE__, __LINE__);

    sqlite3_stmt* statement = NULL;
    qcc::String sqlStmtText = "";
    QStatus funcStatus = ER_OK;
    const uint8_t* dataId = NULL;
    const std::size_t dataIdSize = qcc::Crypto_SHA256::DIGEST_SIZE;         //Or should we rely on data.size() ?
    int sqlRetCode = SQLITE_OK;
    qcc::String updateStr = "?";

    dataId = certificate.GetDigest();

    if (data.empty() || (NULL == dataId)) {
        funcStatus = ER_FAIL;
        QCC_LogError(funcStatus, ("NULL data argument"));
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }
    do {
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

    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

QStatus SQLStorage::GetCertificateFromRow(sqlite3_stmt** statement,
                                          qcc::CertificateX509& cert,
                                          const qcc::String tableName,
                                          Keys keys) const
{
    QStatus funcStatus = ER_OK;
    int column = 0;

    ECCPublicKey subject;
    funcStatus = subject.Import((const uint8_t*)sqlite3_column_blob(*statement, column),
                                qcc::ECC_COORDINATE_SZ + qcc::ECC_COORDINATE_SZ);
    if (ER_OK != funcStatus) {
        return funcStatus;
    }
    cert.SetSubjectPublicKey(&subject);
    keys.appECCPublicKey = &subject;

    ++column; // skip version column

    ECCPublicKey issuer;
    funcStatus = issuer.Import((const uint8_t*)sqlite3_column_blob(*statement, ++column),
                               qcc::ECC_COORDINATE_SZ + qcc::ECC_COORDINATE_SZ);
    if (ER_OK != funcStatus) {
        return funcStatus;
    }
    //TODO: cert.SetIssuer(&issuer);

    qcc::CertificateX509::ValidPeriod validity;
    validity.validFrom = sqlite3_column_int64(*statement, ++column);
    validity.validTo = sqlite3_column_int64(*statement, ++column);
    cert.SetValidity(&validity);

    cert.SetSerial(
        qcc::String(
            (const char*)sqlite3_column_blob(*statement, ++column),
            GetBlobSize(tableName.c_str(), "SN", &keys)));

    cert.SetDigest(
        (const uint8_t*)sqlite3_column_blob(*statement, ++column),
        GetBlobSize(tableName.c_str(), "DATAID",
                    &keys));

    return funcStatus;
}

QStatus SQLStorage::GetCertificateFromRow(sqlite3_stmt** statement, qcc::MembershipCertificate& cert) const
{
    int column = 7;
    cert.SetCA(sqlite3_column_int(*statement, column));

    Keys keys;
    qcc::String guildID = qcc::String((const char*)sqlite3_column_text(*statement, ++column));
    cert.SetGuild(GUID128(guildID));
    keys.guildID = &guildID;

    return GetCertificateFromRow(statement, cert, MEMBERSHIP_CERTS_TABLE_NAME, keys);
}

QStatus SQLStorage::PrepareCertificateQuery(const qcc::MembershipCertificate& certificate,
                                            sqlite3_stmt** statement) const
{
    QStatus funcStatus = ER_OK;
    int sqlRetCode = SQLITE_OK;

    qcc::MembershipCertificate& cert = const_cast<qcc::MembershipCertificate&>(certificate);
    const qcc::ECCPublicKey* appPubKey = cert.GetSubjectPublicKey();
    qcc::String guildId;
    if (cert.IsGuildSet()) {
        guildId = cert.GetGuild().ToString();
    }

    qcc::String sqlStmtText = "SELECT * FROM ";
    sqlStmtText += MEMBERSHIP_CERTS_TABLE_NAME;

    if (appPubKey && guildId.empty()) {
        sqlStmtText += " WHERE SUBJECT = ?";
    } else if (appPubKey && (!guildId.empty())) {
        sqlStmtText += " WHERE SUBJECT = ? AND GUID = ? ";
    } else if (!appPubKey && (!guildId.empty())) {
        sqlStmtText += " WHERE GUID = ?";
    }

    do {
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            break;
        }

        if (appPubKey) {
            uint8_t publicKey[qcc::ECC_COORDINATE_SZ + qcc::ECC_COORDINATE_SZ];
            size_t size = sizeof(publicKey);
            funcStatus = const_cast<qcc::ECCPublicKey*>(appPubKey)->Export(publicKey, &size);

            if (ER_OK != funcStatus) {
                QCC_LogError(funcStatus, ("Failed to export public key"));
                break;
            }
            sqlRetCode = sqlite3_bind_blob(*statement, 1, publicKey,
                                           sizeof(publicKey), SQLITE_TRANSIENT);
            if (SQLITE_OK != sqlRetCode) {
                break;
            }
        }

        if (!guildId.empty()) {
            sqlRetCode = sqlite3_bind_text(*statement, 2, guildId.c_str(), -1,
                                           SQLITE_TRANSIENT);
        }
    } while (0);

    if (SQLITE_OK != sqlRetCode) {
        funcStatus = ER_FAIL;
        LOGSQLERROR(funcStatus);
    }

    return funcStatus;
}

QStatus SQLStorage::GetCertificates(const qcc::MembershipCertificate& certificate,
                                    std::vector<qcc::MembershipCertificate>& certificates) const
{
    storageMutex.Lock(__FILE__, __LINE__);

    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement = NULL;
    QStatus funcStatus = ER_OK;

    funcStatus = PrepareCertificateQuery(certificate, &statement);
    if (ER_OK != funcStatus) {
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }

    while (SQLITE_ROW == (sqlRetCode = sqlite3_step(statement))) {
        qcc::MembershipCertificate cert;
        funcStatus = GetCertificateFromRow(&statement, cert);
        if (ER_OK != funcStatus) {
            break;
        }
        certificates.push_back(cert);
    }

    sqlRetCode = sqlite3_finalize(statement);
    if (SQLITE_OK != sqlRetCode) {
        funcStatus = ER_FAIL;
        LOGSQLERROR(funcStatus);
    }

    storageMutex.Unlock(__FILE__, __LINE__);
    return funcStatus;
}

QStatus SQLStorage::GetCertificate(qcc::CertificateX509& certificate)
{
    storageMutex.Lock(__FILE__, __LINE__);

    sqlite3_stmt* statement = NULL;
    int sqlRetCode = SQLITE_OK;
    QStatus funcStatus = ER_OK;
    qcc::String sqlStmtText = "";
    qcc::String tableName = "";
    const qcc::ECCPublicKey* appECCPublicKey = NULL;
    qcc::String guildId = "";

    qcc::CertificateX509& cert =
        dynamic_cast<qcc::CertificateX509&>(certificate);
    appECCPublicKey = cert.GetSubjectPublicKey();

    if (NULL == appECCPublicKey) {
        funcStatus = ER_FAIL;
        QCC_LogError(funcStatus, ("Null application public key."));
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }
    do {
        sqlStmtText = "SELECT * FROM ";

        switch (cert.GetType()) {
        case qcc::CertificateX509::IDENTITY_CERTIFICATE: {
                tableName = IDENTITY_CERTS_TABLE_NAME;
                sqlStmtText += IDENTITY_CERTS_TABLE_NAME;
                sqlStmtText += " WHERE SUBJECT = ? ";
            }
            break;

        case qcc::CertificateX509::MEMBERSHIP_CERTIFICATE: {
                tableName = MEMBERSHIP_CERTS_TABLE_NAME;
                sqlStmtText += MEMBERSHIP_CERTS_TABLE_NAME;
                sqlStmtText += " WHERE SUBJECT = ? AND GUID = ? ";
                guildId =
                    dynamic_cast<qcc::MembershipCertificate&>(certificate).GetGuild().ToString();
            }
            break;

        default: {
                QCC_LogError(ER_FAIL, ("Unsupported certificate type !"));
                storageMutex.Unlock(__FILE__, __LINE__);
                return ER_FAIL;
            }
        }

        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
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

        if (qcc::CertificateX509::MEMBERSHIP_CERTIFICATE == cert.GetType()) {
            sqlRetCode |= sqlite3_bind_text(statement, 2, guildId.c_str(), -1,
                                            SQLITE_TRANSIENT);
        }

        if (SQLITE_ROW == (sqlRetCode = sqlite3_step(statement))) {
            int column = 1;             // Starting at Version
            Keys keys;
            keys.appECCPublicKey = appECCPublicKey;
            keys.guildID = NULL;
            ECCPublicKey issuer;
            qcc::CertificateX509::ValidPeriod validity;

            /*********************Common to all certificates*****************/
            issuer.Import((const uint8_t*)sqlite3_column_blob(statement,
                                                              ++column), qcc::ECC_COORDINATE_SZ +
                          qcc::ECC_COORDINATE_SZ);
            //TODO: cert.SetIssuer(&issuer);

            validity.validFrom = sqlite3_column_int64(statement, ++column);
            validity.validTo = sqlite3_column_int64(statement, ++column);
            cert.SetValidity(&validity);

            if (qcc::CertificateX509::MEMBERSHIP_CERTIFICATE
                == cert.GetType()) {
                keys.guildID = &guildId;
            }

            cert.SetSerial(
                qcc::String(
                    (const char*)sqlite3_column_blob(statement,
                                                     ++column),
                    GetBlobSize(tableName.c_str(), "SN", &keys)));

            cert.SetDigest((const uint8_t*)sqlite3_column_blob(statement,
                                                               ++column), GetBlobSize(
                               tableName.c_str(), "DATAID", &keys));

            /****************************************************************/

            switch (cert.GetType()) {
            case qcc::CertificateX509::IDENTITY_CERTIFICATE: {
                    qcc::IdentityCertificate& idCert = dynamic_cast<qcc::IdentityCertificate&>(certificate);
                    idCert.SetSubjectOU(
                        (const uint8_t*)sqlite3_column_blob(statement, ++column),
                        GetBlobSize(IDENTITY_CERTS_TABLE_NAME, "ALIAS", &keys));
                    idCert.SetAlias((const char*)sqlite3_column_text(statement, ++column));
                    certificate = dynamic_cast<qcc::CertificateX509&>(idCert);
                }
                break;

            case qcc::CertificateX509::MEMBERSHIP_CERTIFICATE: {
                    qcc::MembershipCertificate&  memCert =
                        dynamic_cast<qcc::MembershipCertificate&>(certificate);
                    memCert.SetCA(sqlite3_column_int(statement, ++column));

                    certificate = dynamic_cast<qcc::CertificateX509&>(memCert);
                }
                break;

            default: {
                    QCC_LogError(ER_FAIL, ("Unsupported certificate type !"));
                    storageMutex.Unlock(__FILE__, __LINE__);
                    return ER_FAIL;
                }
            }
        } else if (SQLITE_DONE == sqlRetCode) {
            QCC_DbgHLPrintf(("No certificate was found!"));
            funcStatus = ER_END_OF_DATA;
            break;
        } else {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }
    } while (0);

    sqlRetCode = sqlite3_finalize(statement);
    if (SQLITE_OK != sqlRetCode) {
        funcStatus = ER_FAIL;
        LOGSQLERROR(funcStatus);
    }

    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

QStatus SQLStorage::GetAssociatedData(const qcc::CertificateX509& certificate,
                                      qcc::String& data) const
{
    storageMutex.Lock(__FILE__, __LINE__);

    sqlite3_stmt* statement = NULL;
    qcc::String sqlStmtText = "";
    QStatus funcStatus = ER_OK;
    const uint8_t* dataId = NULL;
    int sqlRetCode = SQLITE_OK;
    const std::size_t dataIdSize = qcc::Crypto_SHA256::DIGEST_SIZE;         // Or should we rely on data.size() ?

    dataId = dynamic_cast<const qcc::CertificateX509&>(certificate).GetDigest();

    sqlStmtText = "SELECT LENGTH(DATA), DATA FROM ";
    sqlStmtText.append(CERTSDATA_TABLE_NAME);
    sqlStmtText.append(" WHERE ID = ?");
    do {
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }

        sqlRetCode = sqlite3_bind_blob(statement, 1, dataId, dataIdSize,
                                       SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }

        sqlRetCode = sqlite3_step(statement);
        if (SQLITE_ROW == sqlRetCode) {
            int dataSize = sqlite3_column_int(statement, 0);
            data = qcc::String((const char*)sqlite3_column_blob(statement, 1), dataSize);
        } else if (SQLITE_DONE == sqlRetCode) {
            QCC_DbgHLPrintf(("No associated data was found !"));
            funcStatus = ER_END_OF_DATA;
            break;
        } else {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }
    } while (0);

    sqlRetCode = sqlite3_finalize(statement);
    if (SQLITE_OK != sqlRetCode) {
        funcStatus = ER_FAIL;
        LOGSQLERROR(funcStatus);
    }

    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

QStatus SQLStorage::RemoveCertificate(qcc::CertificateX509& certificate)
{
    storageMutex.Lock(__FILE__, __LINE__);

    sqlite3_stmt* statement = NULL;
    qcc::String sqlStmtText = "";
    int sqlRetCode = SQLITE_OK;
    QStatus funcStatus = ER_OK;
    qcc::String certTableName = "";
    qcc::String whereKeys = "";
    const qcc::ECCPublicKey* appECCPublicKey = NULL;

    qcc::CertificateX509& cert = dynamic_cast<qcc::CertificateX509&>(certificate);
    appECCPublicKey = cert.GetSubjectPublicKey();

    if (NULL == appECCPublicKey) {
        QCC_LogError(ER_FAIL, ("Null application public key."));
        storageMutex.Unlock(__FILE__, __LINE__);
        return ER_FAIL;
    }

    switch (cert.GetType()) {
    case qcc::CertificateX509::IDENTITY_CERTIFICATE: {
            certTableName = IDENTITY_CERTS_TABLE_NAME;
            whereKeys = " WHERE SUBJECT = ? ";
        }
        break;

    case qcc::CertificateX509::MEMBERSHIP_CERTIFICATE: {
            certTableName = MEMBERSHIP_CERTS_TABLE_NAME;
            whereKeys = " WHERE SUBJECT = ? AND GUID = ? ";
        }
        break;

    default: {
            QCC_LogError(ER_FAIL, ("Unsupported certificate type !"));
            storageMutex.Unlock(__FILE__, __LINE__);
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
        if (qcc::CertificateX509::MEMBERSHIP_CERTIFICATE == cert.GetType()) {
            sqlRetCode |=
                sqlite3_bind_text(statement, 2,
                                  dynamic_cast<qcc::MembershipCertificate&>(certificate).GetGuild().
                                  ToString().c_str(),
                                  -1, SQLITE_TRANSIENT);
        }

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }
    } while (0);

    funcStatus = StepAndFinalizeSqlStmt(statement);

    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

QStatus SQLStorage::RemoveAssociatedData(const qcc::CertificateX509& certificate)
{
    storageMutex.Lock(__FILE__, __LINE__);

    sqlite3_stmt* statement = NULL;
    qcc::String sqlStmtText = "";
    QStatus funcStatus = ER_OK;
    const uint8_t* dataId = NULL;
    int sqlRetCode = SQLITE_OK;
    const std::size_t dataIdSize = qcc::Crypto_SHA256::DIGEST_SIZE;         // Or should we rely on data.size() ?

    dataId = dynamic_cast<const qcc::CertificateX509&>(certificate).GetDigest();

    if (NULL == dataId) {
        funcStatus = ER_FAIL;
        QCC_LogError(funcStatus, ("Null data ID."));
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }
    do {
        sqlStmtText = "DELETE FROM ";
        sqlStmtText.append(CERTSDATA_TABLE_NAME);
        sqlStmtText.append(" WHERE ID = ?");

        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }

        sqlRetCode = sqlite3_bind_blob(statement, 1, dataId, dataIdSize,
                                       SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }
    } while (0);

    funcStatus = StepAndFinalizeSqlStmt(statement);

    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

QStatus SQLStorage::StoreGuild(const GuildInfo& guildInfo)
{
    QStatus funcStatus = ER_FAIL;
    storageMutex.Lock(__FILE__, __LINE__);

    bool update;
    GuildInfo tmp = guildInfo; // to avoid const cast
    funcStatus = GetGuild(tmp);
    if (ER_OK == funcStatus) {
        update = true;
    } else if (ER_END_OF_DATA == funcStatus) {
        update = false;
    } else {
        QCC_LogError(funcStatus, ("Could not determine update status for guild."));
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }

    funcStatus = StoreInfo(INFO_GUILD, guildInfo.authority, guildInfo.guid, guildInfo.name, guildInfo.desc, update);
    storageMutex.Unlock(__FILE__, __LINE__);
    return funcStatus;
}

QStatus SQLStorage::RemoveGuild(const GuildInfo& guildInfo)
{
    QStatus funcStatus = ER_FAIL;
    storageMutex.Lock(__FILE__, __LINE__);

    GuildInfo tmp = guildInfo; // to avoid const cast
    if (ER_OK != (funcStatus = GetGuild(tmp))) {
        QCC_LogError(funcStatus, ("Guild does not exist."));
    } else {
        funcStatus = RemoveInfo(INFO_GUILD, guildInfo.authority, guildInfo.guid);
    }

    storageMutex.Unlock(__FILE__, __LINE__);
    return funcStatus;
}

QStatus SQLStorage::GetGuild(GuildInfo& guildInfo) const
{
    QStatus funcStatus = ER_FAIL;
    storageMutex.Lock(__FILE__, __LINE__);

    funcStatus = GetInfo(INFO_GUILD, guildInfo.authority, guildInfo.guid, guildInfo.name, guildInfo.desc);

    storageMutex.Unlock(__FILE__, __LINE__);
    return funcStatus;
}

QStatus SQLStorage::GetGuilds(std::vector<GuildInfo>& guildsInfo) const
{
    storageMutex.Lock(__FILE__, __LINE__);

    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement = NULL;
    qcc::String sqlStmtText = "";
    QStatus funcStatus = ER_OK;

    sqlStmtText = "SELECT NAME, DESC, AUTHORITY, ID FROM ";
    sqlStmtText.append(GUILDS_TABLE_NAME);

    /* Prepare the sql query */
    sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(), -1,
                                    &statement, NULL);
    if (SQLITE_OK != sqlRetCode) {
        funcStatus = ER_FAIL;
        LOGSQLERROR(funcStatus);
    }

    /* Iterate over all the rows in the query */
    ECCPublicKey authority;
    while (SQLITE_ROW == (sqlRetCode = sqlite3_step(statement))) {
        GuildInfo info;

        info.name.assign((const char*)sqlite3_column_text(statement, 0));
        info.desc.assign((const char*)sqlite3_column_text(statement, 1));
        funcStatus = authority.Import((const uint8_t*)sqlite3_column_blob(statement, 2),
                                      qcc::ECC_COORDINATE_SZ + qcc::ECC_COORDINATE_SZ);
        if (ER_OK != funcStatus) {
            break;
        }
        info.authority = authority;
        info.guid = qcc::GUID128((const char*)sqlite3_column_text(statement, 3));

        guildsInfo.push_back(info);
    }

    sqlRetCode = sqlite3_finalize(statement);
    if (SQLITE_OK != sqlRetCode) {
        funcStatus = ER_FAIL;
        LOGSQLERROR(funcStatus);
    }

    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

QStatus SQLStorage::StoreIdentity(const IdentityInfo& idInfo)
{
    QStatus funcStatus = ER_FAIL;
    storageMutex.Lock(__FILE__, __LINE__);

    bool update;
    IdentityInfo tmp = idInfo; // to avoid const cast
    funcStatus = GetIdentity(tmp);
    if (ER_OK == funcStatus) {
        update = true;
    } else if (ER_END_OF_DATA == funcStatus) {
        update = false;
    } else {
        QCC_LogError(funcStatus, ("Could not determine update status for identity."));
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }

    qcc::String desc; // placeholder
    funcStatus = StoreInfo(INFO_IDENTITY, idInfo.authority, idInfo.guid, idInfo.name, desc, update);
    storageMutex.Unlock(__FILE__, __LINE__);
    return funcStatus;
}

QStatus SQLStorage::RemoveIdentity(const IdentityInfo& idInfo)
{
    QStatus funcStatus = ER_FAIL;
    storageMutex.Lock(__FILE__, __LINE__);

    IdentityInfo tmp = idInfo; // to avoid const cast
    if (ER_OK != (funcStatus = GetIdentity(tmp))) {
        QCC_LogError(funcStatus, ("Identity does not exist."));
    } else {
        funcStatus = RemoveInfo(INFO_IDENTITY, idInfo.authority, idInfo.guid);
    }

    storageMutex.Unlock(__FILE__, __LINE__);
    return funcStatus;
}

QStatus SQLStorage::GetIdentity(IdentityInfo& idInfo) const
{
    QStatus funcStatus = ER_FAIL;
    storageMutex.Lock(__FILE__, __LINE__);

    qcc::String desc; // placeholder
    funcStatus = GetInfo(INFO_IDENTITY, idInfo.authority, idInfo.guid, idInfo.name, desc);

    storageMutex.Unlock(__FILE__, __LINE__);
    return funcStatus;
}

QStatus SQLStorage::GetIdentities(std::vector<IdentityInfo>& idInfos) const
{
    storageMutex.Lock(__FILE__, __LINE__);

    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement = NULL;
    qcc::String sqlStmtText = "";
    QStatus funcStatus = ER_OK;

    sqlStmtText = "SELECT NAME, AUTHORITY, ID FROM ";
    sqlStmtText.append(IDENTITY_TABLE_NAME);

    /* Prepare the sql query */
    sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(), -1,
                                    &statement, NULL);
    if (SQLITE_OK != sqlRetCode) {
        funcStatus = ER_FAIL;
        LOGSQLERROR(funcStatus);
    }

    /* Iterate over all the rows in the query */
    ECCPublicKey authority;
    while (SQLITE_ROW == (sqlRetCode = sqlite3_step(statement))) {
        IdentityInfo info;

        info.name.assign((const char*)sqlite3_column_text(statement, 0));
        funcStatus = authority.Import((const uint8_t*)sqlite3_column_blob(statement, 1),
                                      qcc::ECC_COORDINATE_SZ + qcc::ECC_COORDINATE_SZ);
        if (ER_OK != funcStatus) {
            break;
        }
        info.authority = authority;
        info.guid = qcc::GUID128((const char*)sqlite3_column_text(statement, 2));

        idInfos.push_back(info);
    }

    sqlRetCode = sqlite3_finalize(statement);
    if (SQLITE_OK != sqlRetCode) {
        funcStatus = ER_FAIL;
        LOGSQLERROR(funcStatus);
    }

    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

void SQLStorage::Reset()
{
    storageMutex.Lock(__FILE__, __LINE__);
    sqlite3_close(nativeStorageDB);
    remove(GetStoragePath().c_str());
    storageMutex.Unlock(__FILE__, __LINE__);
}

SQLStorage::~SQLStorage()
{
    storageMutex.Lock(__FILE__, __LINE__);
    int sqlRetCode;
    if ((sqlRetCode = sqlite3_close(nativeStorageDB)) != SQLITE_OK) { //TODO :: change to sqlite3_close_v2 once Jenkins machines allow for it
        LOGSQLERROR(ER_FAIL);
    }
    storageMutex.Unlock(__FILE__, __LINE__);
}

/*************************************************PRIVATE*********************************************************/

QStatus SQLStorage::BindCertForStorage(const qcc::CertificateX509& cert,
                                       const char* sqlStmtText, sqlite3_stmt*
                                       * statement)
{
    int sqlRetCode = SQLITE_OK;                                        // Equal zero
    QStatus funcStatus = ER_OK;
    int column = 1;

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
        funcStatus = cert.GetSubjectPublicKey()->Export(publicKey, &size);
        if (funcStatus != ER_OK) {
            QCC_LogError(funcStatus, ("Failed to export public key"));
            break;
        }

        sqlRetCode = sqlite3_bind_blob(*statement, column,
                                       publicKey, sizeof(publicKey),
                                       SQLITE_TRANSIENT);

        sqlRetCode |= sqlite3_bind_int(*statement, ++column, 2); // fixed version

        uint8_t publicKeyIssuer[qcc::ECC_COORDINATE_SZ + qcc::ECC_COORDINATE_SZ];
        size = sizeof(publicKeyIssuer);
        //TODO MAKE SURE WE HAVE THE ISSUER !!!!!!!!!
        funcStatus = cert.GetSubjectPublicKey()->Export(publicKeyIssuer, &size);
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
                                        cert.GetSerial().data(),
                                        cert.GetSerial().size(),
                                        SQLITE_TRANSIENT);

        sqlRetCode |= sqlite3_bind_blob(*statement, ++column,
                                        cert.GetDigest(),
                                        cert.GetDigestSize(),
                                        SQLITE_TRANSIENT);

        /****************************************************************/

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            break;
        }

        switch (cert.GetType()) {
        case qcc::CertificateX509::IDENTITY_CERTIFICATE: {
                const qcc::IdentityCertificate& idCert =
                    dynamic_cast<const qcc::IdentityCertificate&>(cert);
                sqlRetCode |= sqlite3_bind_blob(*statement, ++column,
                                                idCert.GetSubjectOU(),
                                                idCert.GetSubjectOULength(),
                                                SQLITE_TRANSIENT);
                sqlRetCode |= sqlite3_bind_text(*statement, ++column,
                                                idCert.GetAlias().c_str(), -1,
                                                SQLITE_TRANSIENT);

                if (SQLITE_OK != sqlRetCode) {
                    funcStatus = ER_FAIL;
                }
            }
            break;

        case qcc::CertificateX509::MEMBERSHIP_CERTIFICATE: {
                qcc::CertificateX509& c = const_cast<qcc::CertificateX509&>(cert);
                qcc::MembershipCertificate& memCert =
                    dynamic_cast<qcc::MembershipCertificate&>(c);

                sqlRetCode |= sqlite3_bind_int(*statement, ++column,
                                               memCert.IsCA());
                sqlRetCode |= sqlite3_bind_text(*statement, ++column,
                                                memCert.GetGuild().ToString().c_str(), -1,
                                                SQLITE_TRANSIENT);

                if (SQLITE_OK != sqlRetCode) {
                    funcStatus = ER_FAIL;
                }
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

QStatus SQLStorage::StepAndFinalizeSqlStmt(sqlite3_stmt* statement) const
{
    int sqlRetCode = SQLITE_OK;
    QStatus funcStatus = ER_OK;

    if (NULL == statement) {
        QCC_LogError(ER_FAIL, ("NULL statement argument"));
        return ER_FAIL;
    }

    sqlRetCode = sqlite3_step(statement);

    if (SQLITE_DONE != sqlRetCode) {
        funcStatus = ER_FAIL;
        LOGSQLERROR(funcStatus);
    }

    sqlRetCode = sqlite3_finalize(statement);
    if (SQLITE_OK != sqlRetCode) {
        funcStatus = ER_FAIL;
        LOGSQLERROR(funcStatus);
    }

    if (ER_OK != funcStatus) {
        LOGSQLERROR(funcStatus);
    }

    return funcStatus;
}

qcc::String SQLStorage::GetStoragePath() const
{
    qcc::String pathkey(STORAGE_FILEPATH_KEY);
    qcc::String storagePath;
    std::map<qcc::String, qcc::String>::const_iterator pathItr =
        storageConfig.settings.find(pathkey);
    if (pathItr != storageConfig.settings.end()) {
        storagePath = pathItr->second;
    }

    return storagePath;
}

QStatus SQLStorage::Init()
{
    int sqlRetCode = SQLITE_OK;
    qcc::String sqlStmtText = "";
    QStatus funcStatus = ER_OK;

    do {
        if (SQLITE_OK != (sqlRetCode = sqlite3_shutdown())) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            QCC_DbgHLPrintf(("Failed in shutting down sqlite in preparation for initialization !!"));
            break;
        }

        if (sqlite3_threadsafe() == 0) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            QCC_DbgHLPrintf(("Sqlite was compiled NOT to be thread-safe !!"));
            break;
        }

        if (SQLITE_OK != (sqlRetCode = sqlite3_config(SQLITE_CONFIG_SERIALIZED))) {
            funcStatus = ER_FAIL;
            QCC_DbgHLPrintf(("Serialize failed with %d", sqlRetCode));
            LOGSQLERROR(funcStatus);
            break;
        }

        if (SQLITE_OK != (sqlRetCode = sqlite3_initialize())) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            QCC_DbgHLPrintf(("Failed in sqlite initialization !!"));
            break;
        }

        qcc::String storagePath = GetStoragePath();
        if (storagePath.empty()) {
            funcStatus = ER_FAIL;
            QCC_DbgHLPrintf(("Invalid path to be used for storage !!"));
            break;
        }
        sqlRetCode = sqlite3_open(storagePath.c_str(), &nativeStorageDB);

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            sqlite3_close(nativeStorageDB);
            break;
        }

        sqlStmtText = CLAIMED_APPLICATIONS_TABLE_SCHEMA;
        sqlStmtText.append(IDENTITY_CERTS_TABLE_SCHEMA);
        sqlStmtText.append(MEMBERSHIP_CERTS_TABLE_SCHEMA);
        sqlStmtText.append(CERTSDATA_TABLE_SCHEMA);
        sqlStmtText.append(GUILDS_TABLE_SCHEMA);
        sqlStmtText.append(IDENTITY_TABLE_SCHEMA);
        sqlStmtText.append(SERIALNUMBER_TABLE_SCHEMA);
        sqlStmtText.append(DEFAULT_PRAGMAS);

        sqlRetCode = sqlite3_exec(nativeStorageDB, sqlStmtText.c_str(), NULL, 0,
                                  NULL);

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }
        funcStatus = InitSerialNumber();
    } while (0);

    return funcStatus;
}

int SQLStorage::GetBlobSize(const char* table, const char* columnName,
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

QStatus SQLStorage::StoreInfo(InfoType type,
                              const qcc::ECCPublicKey& auth,
                              const GUID128& guid,
                              const qcc::String& name,
                              const qcc::String& desc,
                              bool update)
{
    uint8_t authority[qcc::ECC_COORDINATE_SZ + qcc::ECC_COORDINATE_SZ];
    size_t size = sizeof(authority);
    QStatus funcStatus = auth.Export(authority, &size);
    if (funcStatus != ER_OK) {
        QCC_LogError(funcStatus, ("Failed to export authority"));
        return funcStatus;
    }

    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement;

    qcc::String sqlStmtText;
    if (update) {
        sqlStmtText = "UPDATE ";
        sqlStmtText.append(type == INFO_GUILD ? GUILDS_TABLE_NAME : IDENTITY_TABLE_NAME);
        sqlStmtText.append(" SET NAME = ?");
        if (type == INFO_GUILD) {
            sqlStmtText.append(", DESC = ?");
        }
        sqlStmtText.append(" WHERE AUTHORITY = ? AND ID LIKE ?");
    } else {
        sqlStmtText = "INSERT INTO ";
        sqlStmtText.append(type == INFO_GUILD ? GUILDS_TABLE_NAME : IDENTITY_TABLE_NAME);
        sqlStmtText.append(" (NAME, ");
        if (type == INFO_GUILD) {
            sqlStmtText.append("DESC, ");
        }
        sqlStmtText.append("AUTHORITY, ID) VALUES (?, ?, ?");
        if (type == INFO_GUILD) {
            sqlStmtText.append(", ?");
        }
        sqlStmtText.append(")");
    }

    do {
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }

        int i = 1;
        sqlRetCode |= sqlite3_bind_text(statement, i++, name.c_str(), -1, SQLITE_TRANSIENT);
        if (type == INFO_GUILD) {
            sqlRetCode |= sqlite3_bind_text(statement, i++, desc.c_str(), -1, SQLITE_TRANSIENT);
        }
        sqlRetCode |= sqlite3_bind_blob(statement, i++, authority, sizeof(authority), SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_text(statement, i++, guid.ToString().c_str(), -1, SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }
    } while (0);

    funcStatus = StepAndFinalizeSqlStmt(statement);

    return funcStatus;
}

QStatus SQLStorage::GetInfo(InfoType type,
                            const qcc::ECCPublicKey& auth,
                            const GUID128& guid,
                            qcc::String& name,
                            qcc::String& desc) const
{
    QStatus funcStatus = ER_FAIL;

    if (auth.empty()) {
        funcStatus = ER_FAIL;
        QCC_LogError(funcStatus, ("Empty authority!"));
        return funcStatus;
    }

    if (guid.ToString().empty()) {
        funcStatus = ER_FAIL;
        QCC_LogError(funcStatus, ("Empty GUID!"));
        return funcStatus;
    }

    uint8_t authority[qcc::ECC_COORDINATE_SZ + qcc::ECC_COORDINATE_SZ];
    size_t size = sizeof(authority);
    funcStatus = auth.Export(authority, &size);
    if (funcStatus != ER_OK) {
        QCC_LogError(funcStatus, ("Failed to export authority"));
        return funcStatus;
    }

    sqlite3_stmt* statement = NULL;
    qcc::String sqlStmtText = "";
    int sqlRetCode = SQLITE_OK;

    sqlStmtText = "SELECT NAME";
    if (type == INFO_GUILD) {
        sqlStmtText.append(", DESC");
    }
    sqlStmtText.append(" FROM ");
    sqlStmtText.append(type == INFO_GUILD ? GUILDS_TABLE_NAME : IDENTITY_TABLE_NAME);
    sqlStmtText.append(" WHERE AUTHORITY = ? AND ID LIKE ?");
    do {
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }

        sqlRetCode |= sqlite3_bind_blob(statement, 1, authority, sizeof(authority), SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_text(statement, 2, guid.ToString().c_str(), -1, SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }

        sqlRetCode = sqlite3_step(statement);
        if (SQLITE_ROW == sqlRetCode) {
            name.assign((const char*)sqlite3_column_text(statement, 0));
            if (type == INFO_GUILD) {
                desc.assign((const char*)sqlite3_column_text(statement, 1));
            }
        } else if (SQLITE_DONE == sqlRetCode) {
            funcStatus = ER_END_OF_DATA;
            break;
        } else {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }
    } while (0);

    sqlRetCode = sqlite3_finalize(statement);
    if (SQLITE_OK != sqlRetCode) {
        funcStatus = ER_FAIL;
        LOGSQLERROR(funcStatus);
    }

    return funcStatus;
}

QStatus SQLStorage::RemoveInfo(InfoType type,
                               const qcc::ECCPublicKey& auth,
                               const qcc::GUID128& guid)
{
    uint8_t authority[qcc::ECC_COORDINATE_SZ + qcc::ECC_COORDINATE_SZ];
    size_t size = sizeof(authority);
    QStatus funcStatus = auth.Export(authority, &size);
    if (funcStatus != ER_OK) {
        QCC_LogError(funcStatus, ("Failed to export authority"));
        return funcStatus;
    }

    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement;

    qcc::String sqlStmtText = "DELETE FROM ";
    sqlStmtText.append(type == INFO_GUILD ? GUILDS_TABLE_NAME : IDENTITY_TABLE_NAME);
    sqlStmtText.append(" WHERE AUTHORITY = ? AND ID LIKE ?");

    do {
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }

        sqlRetCode |= sqlite3_bind_blob(statement, 1, authority,
                                        sizeof(authority), SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_text(statement, 2, guid.ToString().c_str(),
                                        -1, SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }
    } while (0);

    funcStatus = StepAndFinalizeSqlStmt(statement);

    return funcStatus;
}
}
}
#undef QCC_MODULE
