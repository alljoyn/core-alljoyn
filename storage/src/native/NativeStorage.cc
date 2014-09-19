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
#include <qcc/Debug.h>
#include <cstdio>

#define QCC_MODULE "SEC_MGR"

namespace ajn {
namespace securitymgr {
#define DEFAULT_STORAGE_PATH "~/.secmgr/secmgr.db"

#define CLAIMED_APP_TABLE_NAME "CLAIMED_APPLICATIONS"
#define IDENTITY_CERTS_TABLE_NAME "IDENTITY_CERTS"
#define MEMBERSHIP_CERTS_TABLE_NAME "MEMBERSHIP_CERTS"
#define POLICY_CERTS_TABLE_NAME "POLICY_CERTS"
#define MEMBERSHIP_EQ_CERTS_TABLE_NAME "MEMBERSHIP_EQ_CERTS"
#define USER_EQ_CERTS_TABLE_NAME "USER_EQ_CERTS"

#define CLAIMED_APPLICATIONS_TABLE_SCHEMA \
    "CREATE TABLE IF NOT EXISTS " CLAIMED_APP_TABLE_NAME \
    " (\
        APPLICATION_PUBKEY BLOB PRIMARY KEY    NOT NULL,\
        APP_NAME   TEXT,    \
        APP_ID   TEXT,    \
        DEV_NAME   TEXT,    \
        USER_DEF_NAME   TEXT    \
); "

#define IDENTITY_CERTS_TABLE_SCHEMA \
    "CREATE TABLE IF NOT EXISTS " IDENTITY_CERTS_TABLE_NAME \
    " (\
        SUBJECT  BLOB PRIMARY KEY   NOT NULL,\
        VERSION INT NOT NULL,\
        ISSUER BLOB NOT NULL,\
        VALIDITYFROM UNSIGNED BIG INT NOT NULL,\
        VALIDITYTO UNSIGNED BIG INT NOT NULL,\
        DELEGATE BOOLEAN NOT NULL,\
        ALIASLEN INT NOT NULL,\
        ALIAS BLOB NOT NULL,\
        DIGEST BLOB NOT NULL,\
        SIG BLOB NOT NULL,\
        FOREIGN KEY(SUBJECT) REFERENCES " CLAIMED_APP_TABLE_NAME                                                                                                                                                                                                                                                                                                                                                                                    \
    " (APPLICATION_PUBKEY)\
); "

#define DEFAULT_PRAGMAS \
    "PRAGMA encoding = \"UTF-8\";\
    PRAGMA journal_mode = OFF; "

QStatus NativeStorage::TrackClaimedApplication(const ClaimedApplicationInfo& claimedApplicationInfo)
{
    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement;
    QStatus funcStatus = ER_OK;

    qcc::String sqlStmtText("INSERT OR REPLACE INTO ");
    sqlStmtText.append(CLAIMED_APP_TABLE_NAME);
    sqlStmtText.append(" (APPLICATION_PUBKEY, APP_NAME, APP_ID, DEV_NAME, USER_DEF_NAME) VALUES (?, ?, ?, ?, ?)");

    do {
        QCC_DbgHLPrintf((sqlStmtText.c_str()));
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(), -1, &statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            QCC_LogError(ER_FAIL, ("prepare failed %d, %s", sqlRetCode, sqlite3_errmsg(nativeStorageDB)));
            funcStatus = ER_FAIL;
            break;
        }

        sqlRetCode = sqlite3_bind_blob(statement, 1, claimedApplicationInfo.publicKey.data, qcc::ECC_PUBLIC_KEY_SZ,
                                       SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_text(statement, 2, claimedApplicationInfo.appName.c_str(), -1, SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_text(statement, 3, claimedApplicationInfo.appID.c_str(), -1, SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_text(statement, 4, claimedApplicationInfo.deviceName.c_str(), -1, SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_text(statement, 5, claimedApplicationInfo.userDefinedName.c_str(), -1,
                                        SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            QCC_LogError(ER_FAIL, ("bind failed %d, %s", sqlRetCode, sqlite3_errmsg(nativeStorageDB)));
            funcStatus = ER_FAIL;
            break;
        }

        funcStatus = ExecuteGenericSqlStmt(statement);
    } while (0);

    return funcStatus;
}

QStatus NativeStorage::UnTrackClaimedApplication(const ClaimedApplicationInfo& claimedApplicationInfo)
{
    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement = NULL;
    const char* sqlStmtText = NULL;
    QStatus funcStatus = ER_OK;

    do {
        sqlStmtText = "DELETE FROM " CLAIMED_APP_TABLE_NAME " WHERE APPLICATION_PUBKEY = ?";
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText, -1, &statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            QCC_LogError(ER_FAIL, ("prepare failed %d, %s", sqlRetCode, sqlite3_errmsg(nativeStorageDB)));
            funcStatus = ER_FAIL;
            break;
        }

        sqlRetCode = sqlite3_bind_blob(statement, 1, claimedApplicationInfo.publicKey.data, qcc::ECC_PUBLIC_KEY_SZ,
                                       SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            QCC_LogError(ER_FAIL, ("bind failed %d, %s", sqlRetCode, sqlite3_errmsg(nativeStorageDB)));
            funcStatus = ER_FAIL;
            break;
        }

        funcStatus = ExecuteGenericSqlStmt(statement);
    } while (0);

    return funcStatus;
}

QStatus NativeStorage::GetClaimedApplications(std::vector<ClaimedApplicationInfo>* claimedApplications)
{
    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement = NULL;
    qcc::String sqlStmtText = "";
    QStatus funcStatus = ER_OK;

    sqlStmtText = "SELECT * FROM ";
    sqlStmtText.append(CLAIMED_APP_TABLE_NAME);

    /* prepare the sql query */
    sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(), -1, &statement, NULL);
    if (SQLITE_OK != sqlRetCode) {
        QCC_LogError(ER_FAIL, ("prepare failed %d, %s", sqlRetCode, sqlite3_errmsg(nativeStorageDB)));
        funcStatus = ER_FAIL;
    }

    /* iterate over all the rows in the query */
    while (SQLITE_ROW == (sqlRetCode = sqlite3_step(statement))) {
        ClaimedApplicationInfo info;
        memcpy(&info.publicKey.data, sqlite3_column_blob(statement, 0), qcc::ECC_PUBLIC_KEY_SZ);
        info.appName = qcc::String((const char*)sqlite3_column_text(statement, 1));
        info.appID = qcc::String((const char*)sqlite3_column_text(statement, 2));
        info.deviceName = qcc::String((const char*)sqlite3_column_text(statement, 3));
        info.userDefinedName = qcc::String((const char*)sqlite3_column_text(statement, 4));
        claimedApplications->push_back(info);
    }

    if (ER_FAIL == funcStatus) {
        qcc::String error = "SQL error: ";
        error.append(sqlRetCode);
        error.append(" - ERROR: ");
        error.append(sqlite3_errmsg(nativeStorageDB));
        QCC_LogError(ER_FAIL, (error.c_str()));
    }

    return funcStatus;
}

QStatus NativeStorage::ClaimedApplication(const qcc::ECCPublicKey& appECCPublicKey, bool* claimed)
{
    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement = NULL;
    qcc::String sqlStmtText = "";
    QStatus funcStatus = ER_OK;
    int count = 0;

    do {
        if (NULL == claimed) {
            funcStatus = ER_FAIL;
            break;
        }

        sqlStmtText = "SELECT COUNT(APPLICATION_PUBKEY) FROM ";
        sqlStmtText.append(CLAIMED_APP_TABLE_NAME);
        sqlStmtText.append(" WHERE APPLICATION_PUBKEY LIKE ?");

        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(), -1, &statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            QCC_LogError(ER_FAIL, ("prepare failed %d, %s", sqlRetCode, sqlite3_errmsg(nativeStorageDB)));
            funcStatus = ER_FAIL;
            break;
        }

        sqlRetCode = sqlite3_bind_blob(statement, 1, appECCPublicKey.data, qcc::ECC_PUBLIC_KEY_SZ, SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            QCC_LogError(ER_FAIL, ("bind failed %d, %s", sqlRetCode, sqlite3_errmsg(nativeStorageDB)));
            funcStatus = ER_FAIL;
            break;
        }
        sqlRetCode = sqlite3_step(statement);
        if (sqlRetCode == SQLITE_ROW) {
            count = sqlite3_column_int(statement, 0);
            sqlRetCode = sqlite3_finalize(statement);
            if (SQLITE_OK != sqlRetCode) {
                QCC_LogError(ER_FAIL, ("Error on finalizing SQL statement !"));
                funcStatus = ER_FAIL;
            }
        } else {
            QCC_LogError(ER_FAIL, ("Error in counting entries in database !"));
            funcStatus = ER_FAIL;
            break;
        }
    } while (0);

    if (ER_FAIL == funcStatus) {
        qcc::String error = "SQL error: ";
        error.append(sqlRetCode);
        error.append(" - ERROR: ");
        error.append(sqlite3_errmsg(nativeStorageDB));
        QCC_LogError(ER_FAIL, (error.c_str()));
    }

    *claimed = (count == 1);

    return funcStatus;
}

QStatus NativeStorage::StoreCertificate(const qcc::ECCPublicKey& appECCPublicKey, const qcc::Certificate& certificate)
{
    sqlite3_stmt* statement = NULL;
    qcc::String sqlStmtText = "";
    QStatus funcStatus = ER_OK;

    switch (GetCertificateType(certificate)) {
    case IDENTITY: {
            sqlStmtText = "INSERT OR REPLACE INTO ";
            sqlStmtText.append(IDENTITY_CERTS_TABLE_NAME);
            sqlStmtText.append(" (SUBJECT, VERSION, ISSUER"
                               ", VALIDITYFROM"
                               ", VALIDITYTO"
                               ", DELEGATE"
                               ", ALIASLEN"
                               ", ALIAS"
                               ", DIGEST"
                               ", SIG) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

            funcStatus = BindIdentityCertForStorage(certificate, sqlStmtText.c_str(), &statement);
        }
        break;

    case MEMBERSHIP: {
            QCC_LogError(ER_FAIL, ("Unsupported certificate type !"));
            funcStatus = ER_FAIL;
        }
        break;

    case POLICY: {
            QCC_LogError(ER_FAIL, ("Unsupported certificate type !"));
            funcStatus = ER_FAIL;
        }
        break;

    case MEMBERSHIP_EQ: {
            QCC_LogError(ER_FAIL, ("Unsupported certificate type !"));
            funcStatus = ER_FAIL;
        }
        break;

    case USER_EQ: {
            QCC_LogError(ER_FAIL, ("Unsupported certificate type !"));
            funcStatus = ER_FAIL;
        }
        break;

    default: {
            QCC_LogError(ER_FAIL, ("Unsupported certificate type !"));
            funcStatus = ER_FAIL;
        }
    }

    if (funcStatus == ER_OK) {
        funcStatus = ExecuteGenericSqlStmt(statement);
    }
    QCC_LogError(ER_FAIL, ("Unsupported certificate type !"));
    return ER_FAIL;
    return funcStatus;
}

qcc::Certificate NativeStorage::GetCertificate(const qcc::ECCPublicKey& appECCPublicKey,
                                               const CertificateType certificateType)
{
    return qcc::Certificate(0);         //TODO STUFF
}

QStatus NativeStorage::RemoveCertificate(const qcc::ECCPublicKey& appECCPublicKey,
                                         const CertificateType certificateType)
{
    sqlite3_stmt* statement = NULL;
    qcc::String sqlStmtText = "";
    int sqlRetCode = SQLITE_OK;
    QStatus funcStatus = ER_OK;
    qcc::String certTableName = "";

    switch (certificateType) {
    case IDENTITY: {
            certTableName = IDENTITY_CERTS_TABLE_NAME;
        }
        break;

    case MEMBERSHIP: {
            certTableName = MEMBERSHIP_CERTS_TABLE_NAME;
        }
        break;

    case POLICY: {
            certTableName = POLICY_CERTS_TABLE_NAME;
        }
        break;

    case MEMBERSHIP_EQ: {
            certTableName = MEMBERSHIP_EQ_CERTS_TABLE_NAME;
        }
        break;

    case USER_EQ: {
            certTableName = USER_EQ_CERTS_TABLE_NAME;
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
        sqlStmtText.append(" WHERE APPLICATION_PUBKEY = ?");

        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(), -1, &statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            break;
        }

        sqlRetCode = sqlite3_bind_blob(statement, 1, appECCPublicKey.data, qcc::ECC_PUBLIC_KEY_SZ, SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            break;
        }
    } while (0);

    if (funcStatus == ER_OK) {
        funcStatus = ExecuteGenericSqlStmt(statement);
    }

    return funcStatus;
}

NativeStorage::~NativeStorage()
{
    int retval;
    if ((retval = sqlite3_close(nativeStorageDB)) != SQLITE_OK) {
        QCC_LogError(
            ER_FAIL,
            ((qcc::String("SQL error: ") + retval + " - ERROR: " + sqlite3_errmsg(nativeStorageDB)).c_str()));
    }
}

/*************************************************PRIVATE*********************************************************/

CertificateType NativeStorage::GetCertificateType(const qcc::Certificate& certificate)
{
    return IDENTITY;      // TODO: Identity is returned for now until we figure out the proper mapping to cert versions.
}

QStatus NativeStorage::BindIdentityCertForStorage(const qcc::Certificate& certificate,
                                                  const char* sqlStmtText,
                                                  sqlite3_stmt** statement)
{
    int sqlRetCode = SQLITE_OK;         // Equal zero
    QStatus funcStatus = ER_OK;

    do {
        //TODO enable whenever we have CertificateType4
        //CertificateType4 &idCert = dynamic_cast<CertificateType4&>(certificate);

        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText, -1, statement, NULL);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            break;
        }

        //TODO add all vars to be bound once this type of cert is implemented
        //e.g., sqlRetCode = sqlite3_bind_blob(*statement, 1, certificate.GetSubject()->data, qcc::ECC_PUBLIC_KEY_SZ, SQLITE_TRANSIENT);
        //sqlRetCode |= sqlite3_bind_text(*statement, 2, certificate->GetVersion(), -1, SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            break;
        }
    } while (0);

    return funcStatus;
}

QStatus NativeStorage::ExecuteGenericSqlStmt(sqlite3_stmt* statement)
{
    int sqlRetCode = SQLITE_OK;
    QStatus funcStatus = ER_OK;

    do {
        sqlRetCode = sqlite3_step(statement);
        if (SQLITE_DONE != sqlRetCode) {
            QCC_LogError(ER_FAIL, ("Error while stepping the SQL statement !"));
            funcStatus = ER_FAIL;
            break;
        }

        sqlRetCode = sqlite3_finalize(statement);
        if (SQLITE_OK != sqlRetCode) {
            QCC_LogError(ER_FAIL, ("Error while finalizing SQL statement !"));
            funcStatus = ER_FAIL;
            break;
        }
    } while (0);

    if (ER_FAIL == funcStatus) {
        QCC_LogError(
            ER_FAIL,
            ((qcc::String("SQL error: ") + sqlRetCode + " - ERROR: " + sqlite3_errmsg(nativeStorageDB)).c_str()));
    }
    return funcStatus;
}

qcc::String NativeStorage::GetStoragePath() const
{
    qcc::String storagePath(DEFAULT_STORAGE_PATH);
    qcc::String pathkey("STORAGE_PATH");
    std::map<qcc::String, qcc::String>::const_iterator pathItr = storageConfig.settings.find(pathkey);
    if (pathItr != storageConfig.settings.end()) {
        storagePath = pathItr->second;
    }

    return storagePath;
}

QStatus NativeStorage::Init()
{
    int sqlRetCode = SQLITE_OK;
    qcc::String sqlStmtText = "";
    char* errMsg = NULL;
    QStatus funcStatus = ER_OK;

    do {
        qcc::String storagePath = GetStoragePath();
        sqlRetCode = sqlite3_open(storagePath.c_str(), &nativeStorageDB);

        if (SQLITE_OK != sqlRetCode) {
            QCC_LogError(
                ER_FAIL,
                ((qcc::String("SQL error: ") + sqlRetCode + " - ERROR: " + sqlite3_errmsg(nativeStorageDB)).c_str()));
            sqlite3_close(nativeStorageDB);
            funcStatus = ER_FAIL;
            break;
        }

        sqlStmtText = CLAIMED_APPLICATIONS_TABLE_SCHEMA;
        sqlStmtText.append(IDENTITY_CERTS_TABLE_SCHEMA);
        sqlStmtText.append(DEFAULT_PRAGMAS);

        QCC_DbgHLPrintf((("Executing DB preparation SQL statement : " + sqlStmtText).c_str()));
        sqlRetCode = sqlite3_exec(nativeStorageDB, sqlStmtText.c_str(), NULL, 0, &errMsg);

        if (SQLITE_OK != sqlRetCode) {
            QCC_LogError(ER_FAIL, ((qcc::String("SQL error: ") + sqlRetCode + " - ERROR: " + errMsg).c_str()));
            sqlite3_free(errMsg);
            funcStatus = ER_FAIL;
            break;
        }
    } while (0);

    return funcStatus;
}

void NativeStorage::Reset()
{
    sqlite3_close(nativeStorageDB);
    remove(GetStoragePath().c_str());
}
}
}
