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

#include <SQLStorage.h>
#include <SQLStorageSettings.h>
#include <qcc/Debug.h>
#include <qcc/GUID.h>
#include <qcc/CertificateECC.h>

#include "alljoyn/securitymgr/Util.h"

#define QCC_MODULE "SECMGR_STORAGE"

using namespace qcc;
using namespace std;

namespace ajn {
namespace securitymgr {
#define LOGSQLERROR(a) { QCC_LogError((a), ((string("SQL Error: ") + (sqlite3_errmsg(nativeStorageDB))).c_str())); \
}

QStatus SQLStorage::StoreApplication(const Application& app, const bool update, const bool updatePolicy)
{
    storageMutex.Lock(__FILE__, __LINE__);

    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement;
    QStatus funcStatus = ER_FAIL;
    string sqlStmtText;
    int keyPosition = 1;
    int updateStatePos = 1;
    size_t keyInfoExportSize;
    uint8_t* publicKeyInfo = nullptr;

    if (update) {
        Application tmp = app;
        if (ER_OK != GetManagedApplication(tmp)) {
            QCC_LogError(funcStatus, ("Trying to update a non-existing application !"));
            storageMutex.Unlock(__FILE__, __LINE__);
            return funcStatus;
        }

        sqlStmtText = "UPDATE ";
        sqlStmtText.append(CLAIMED_APPS_TABLE_NAME);
        sqlStmtText.append(
            " SET SYNC_STATE = ? WHERE APPLICATION_PUBKEY = ?");
        keyPosition++;
    } else {
        sqlStmtText = "INSERT INTO ";
        sqlStmtText.append(CLAIMED_APPS_TABLE_NAME);
        sqlStmtText.append(
            " (APPLICATION_PUBKEY, SYNC_STATE) VALUES (?, ?)");
        updateStatePos++;
    }

    if (app.keyInfo.empty()) {
        funcStatus = ER_FAIL;
        QCC_LogError(funcStatus, ("Empty key info!"));
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }

    funcStatus = ExportKeyInfo(app.keyInfo, &publicKeyInfo, keyInfoExportSize);
    if (ER_OK != funcStatus) {
        QCC_LogError(funcStatus, ("Failed to export public keyInfo"));
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }

    do {
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, nullptr);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }

        sqlRetCode = sqlite3_bind_blob(statement, keyPosition,
                                       publicKeyInfo, keyInfoExportSize,
                                       SQLITE_TRANSIENT);

        sqlRetCode |= sqlite3_bind_int(statement,
                                       updateStatePos,
                                       static_cast<int>(app.syncState));
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }
    } while (0);

    funcStatus = StepAndFinalizeSqlStmt(statement);
    delete[]publicKeyInfo;
    publicKeyInfo = nullptr;
    if (ER_OK == funcStatus && updatePolicy) {
        PermissionPolicy policy;
        funcStatus = GetPolicy(app, policy);
        if (ER_OK == funcStatus) {
            policy.SetVersion(policy.GetVersion() + 1);
            funcStatus = StorePolicy(app, policy);
        } else if (ER_END_OF_DATA == funcStatus) {
            funcStatus = ER_OK;  //No policy defined, so we can't increase the version.
        }
    }
    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

QStatus SQLStorage::RemoveApplication(const Application& app)
{
    storageMutex.Lock(__FILE__, __LINE__);

    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement = nullptr;
    const char* sqlStmtText = nullptr;
    QStatus funcStatus = ER_FAIL;
    size_t keyInfoExportSize;
    uint8_t* publicKeyInfo = nullptr;

    if (app.keyInfo.empty()) {
        QCC_LogError(funcStatus, ("Empty key info!"));
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }

    do {
        sqlStmtText =
            "DELETE FROM " CLAIMED_APPS_TABLE_NAME " WHERE APPLICATION_PUBKEY = ?";
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText, -1,
                                        &statement, nullptr);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }

        funcStatus = ExportKeyInfo(app.keyInfo, &publicKeyInfo, keyInfoExportSize);
        if (ER_OK != funcStatus) {
            QCC_LogError(funcStatus, ("Failed to export public keyInfo"));
            break;
        }
        sqlRetCode = sqlite3_bind_blob(statement, 1,
                                       publicKeyInfo, keyInfoExportSize,
                                       SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }
    } while (0);

    funcStatus = StepAndFinalizeSqlStmt(statement);
    delete[]publicKeyInfo;
    publicKeyInfo = nullptr;
    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

QStatus SQLStorage::SetAppMetaData(const Application& app, const ApplicationMetaData& appMetaData)
{
    storageMutex.Lock(__FILE__, __LINE__);

    QStatus funcStatus = ER_FAIL;
    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement;
    string sqlStmtText;
    size_t keyInfoExportSize;
    uint8_t* publicKeyInfo = nullptr;

    Application tmp = app;
    if (ER_OK != (funcStatus = GetManagedApplication(tmp))) {
        QCC_LogError(funcStatus, ("Trying to update meta data for a non-existing application !"));
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }

    sqlStmtText = "UPDATE ";
    sqlStmtText.append(CLAIMED_APPS_TABLE_NAME);
    sqlStmtText.append(" SET APP_NAME = ?, DEV_NAME = ?, USER_DEF_NAME = ? WHERE APPLICATION_PUBKEY = ?");

    funcStatus = ExportKeyInfo(app.keyInfo, &publicKeyInfo, keyInfoExportSize);

    if (ER_OK != funcStatus) {
        QCC_LogError(funcStatus, ("Failed to export public keyInfo"));
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }
    do {
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, nullptr);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }

        sqlRetCode = sqlite3_bind_blob(statement, 4,
                                       publicKeyInfo, keyInfoExportSize,
                                       SQLITE_TRANSIENT);

        sqlRetCode |= sqlite3_bind_text(statement, 1,
                                        appMetaData.appName.c_str(), -1, SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_text(statement, 2,
                                        appMetaData.deviceName.c_str(), -1, SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_text(statement, 3,
                                        appMetaData.userDefinedName.c_str(), -1, SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }
    } while (0);

    funcStatus = StepAndFinalizeSqlStmt(statement);
    delete[]publicKeyInfo;
    publicKeyInfo = nullptr;
    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

QStatus SQLStorage::GetAppMetaData(const Application& app, ApplicationMetaData& appMetaData) const
{
    storageMutex.Lock(__FILE__, __LINE__);

    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement = nullptr;
    string sqlStmtText = "";
    QStatus funcStatus = ER_FAIL;
    size_t keyInfoExportSize;
    uint8_t* publicKeyInfo = nullptr;

    Application tmp = app;
    if (ER_OK != (funcStatus = GetManagedApplication(tmp))) {
        QCC_LogError(funcStatus, ("Trying to get meta data for a non-existing application !"));
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }

    do {
        sqlStmtText = "SELECT APP_NAME, DEV_NAME, USER_DEF_NAME FROM ";
        sqlStmtText.append(CLAIMED_APPS_TABLE_NAME);
        sqlStmtText.append(" WHERE APPLICATION_PUBKEY = ?");

        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, nullptr);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }

        funcStatus = ExportKeyInfo(app.keyInfo, &publicKeyInfo, keyInfoExportSize);

        if (ER_OK != funcStatus) {
            QCC_LogError(funcStatus, ("Failed to export public keyInfo"));
            break;
        }
        sqlRetCode = sqlite3_bind_blob(statement, 1, publicKeyInfo, keyInfoExportSize, SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }
        sqlRetCode = sqlite3_step(statement);
        if (SQLITE_ROW == sqlRetCode) {
            const char* appName = (const char*)sqlite3_column_text(statement, 0);
            appMetaData.appName.assign(appName == nullptr ? "" : appName);
            const char* deviceName = (const char*)sqlite3_column_text(statement, 1);
            appMetaData.deviceName.assign(deviceName == nullptr ? "" : deviceName);
            const char* userDefinedName = (const char*)sqlite3_column_text(statement, 2);
            appMetaData.userDefinedName.assign(userDefinedName == nullptr ? "" : userDefinedName);
        } else if (SQLITE_DONE == sqlRetCode) {
            QCC_DbgHLPrintf(("No meta data was found !"));
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
    delete[]publicKeyInfo;
    publicKeyInfo = nullptr;
    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

QStatus SQLStorage::GetManagedApplications(vector<Application>& apps) const
{
    storageMutex.Lock(__FILE__, __LINE__);

    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement = nullptr;
    string sqlStmtText = "";
    QStatus funcStatus = ER_OK;

    sqlStmtText = "SELECT LENGTH(APPLICATION_PUBKEY), * FROM ";
    sqlStmtText.append(CLAIMED_APPS_TABLE_NAME);

    /* prepare the sql query */
    sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(), -1,
                                    &statement, nullptr);
    if (SQLITE_OK != sqlRetCode) {
        funcStatus = ER_FAIL;
        LOGSQLERROR(funcStatus);
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }

    /* iterate over all the rows in the query */
    while (SQLITE_ROW == (sqlRetCode = sqlite3_step(statement))) {
        Application app;
        size_t pubKeyInfoImportSize = (size_t)sqlite3_column_int(statement, 0);
        funcStatus = app.keyInfo.Import((const uint8_t*)sqlite3_column_blob(statement, 1), pubKeyInfoImportSize);

        if (ER_OK != funcStatus) {
            QCC_LogError(funcStatus, ("Failed to import keyInfo"));
            break;
        }

        app.syncState = static_cast<ApplicationSyncState>(sqlite3_column_int(statement, 7));
        apps.push_back(app);
    }

    sqlRetCode = sqlite3_finalize(statement);
    if (SQLITE_OK != sqlRetCode) {
        funcStatus = ER_FAIL;
        LOGSQLERROR(funcStatus);
    }

    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

QStatus SQLStorage::GetManifest(const Application& app,
                                Manifest& manifest) const
{
    storageMutex.Lock(__FILE__, __LINE__);
    uint8_t* byteArray = nullptr;
    size_t size = 0;
    QStatus funcStatus = GetPolicyOrManifest(app, "MANIFEST", &byteArray, &size);

    if (ER_OK == funcStatus) {
        funcStatus = manifest.SetFromByteArray(byteArray, size);
    }

    if (ER_OK != funcStatus) {
        QCC_LogError(funcStatus, ("Failed to get manifest"));
    }

    delete byteArray;
    byteArray = nullptr;
    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

QStatus SQLStorage::GetPolicy(const Application& app, PermissionPolicy& policy) const
{
    storageMutex.Lock(__FILE__, __LINE__);
    uint8_t* byteArray = nullptr;
    size_t size = 0;

    QStatus funcStatus = GetPolicyOrManifest(app, "POLICY", &byteArray, &size);
    if (ER_OK == funcStatus) {
        funcStatus  = Util::GetPolicy(byteArray, size, policy);         // Util reports error on de-serialization issues
    } else {
        QCC_DbgHLPrintf(("Failed to get policy"));
    }

    delete byteArray;
    byteArray = nullptr;
    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

QStatus SQLStorage::GetNewSerialNumber(CertificateX509& cert) const
{
    storageMutex.Lock(__FILE__, __LINE__);

    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement = nullptr;
    string sqlStmtText = "";
    QStatus funcStatus = ER_OK;
    sqlStmtText = "SELECT * FROM ";
    sqlStmtText.append(SERIALNUMBER_TABLE_NAME);

    /* prepare the sql query */
    sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(), -1,
                                    &statement, nullptr);
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
        if (snprintf(buffer, 32, "%x", value) > 0) {
            buffer[32] = 0; //make sure we have a trailing 0.
            cert.SetSerial((const uint8_t*)buffer, strlen(buffer));
        } else {
            QCC_LogError(ER_FAIL, ("Failed to format the serial number"));
        }

        //update the value in the database.
        sqlStmtText = "UPDATE ";
        sqlStmtText.append(SERIALNUMBER_TABLE_NAME);
        sqlStmtText.append(" SET VALUE = ?");
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(), -1,
                                        &statement, nullptr);

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

QStatus SQLStorage::GetManagedApplication(Application& app) const
{
    storageMutex.Lock(__FILE__, __LINE__);

    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement = nullptr;
    string sqlStmtText = "";
    QStatus funcStatus = ER_OK;
    size_t keyInfoExportSize;
    uint8_t* publicKeyInfo = nullptr;

    if (app.keyInfo.empty()) {
        funcStatus = ER_FAIL;
        QCC_LogError(funcStatus, ("Empty key info!"));
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }

    if (app.keyInfo.empty()) {
        funcStatus = ER_FAIL;
        QCC_LogError(funcStatus, ("Empty key info!"));
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }

    do {
        sqlStmtText = "SELECT * FROM ";
        sqlStmtText.append(CLAIMED_APPS_TABLE_NAME);
        sqlStmtText.append(" WHERE APPLICATION_PUBKEY = ?");

        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, nullptr);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }

        funcStatus = ExportKeyInfo(app.keyInfo, &publicKeyInfo, keyInfoExportSize);

        if (ER_OK != funcStatus) {
            QCC_LogError(funcStatus, ("Failed to export public keyInfo"));
            break;
        }
        sqlRetCode = sqlite3_bind_blob(statement, 1, publicKeyInfo,
                                       keyInfoExportSize, SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }
        sqlRetCode = sqlite3_step(statement);
        if (SQLITE_ROW == sqlRetCode) {
            app.syncState = static_cast<ApplicationSyncState>(sqlite3_column_int(statement, 6));
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
    delete[]publicKeyInfo;
    publicKeyInfo = nullptr;
    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

QStatus SQLStorage::StoreManifest(const Application& app, const Manifest& manifest)
{
    storageMutex.Lock(__FILE__, __LINE__);
    QStatus funcStatus = ER_FAIL;
    uint8_t* manifestByteArray = nullptr;
    size_t size = 0;

    do {
        if (ER_OK != (funcStatus = manifest.GetByteArray(&manifestByteArray, &size))) {
            QCC_LogError(funcStatus, ("Failed to export manifest data"));
            break;
        }

        if (ER_OK != (funcStatus = StorePolicyOrManifest(app,  manifestByteArray, size, "MANIFEST"))) {
            QCC_LogError(funcStatus, ("Failed to store manifest !"));
            break;
        }
    } while (0);

    delete[]manifestByteArray;
    manifestByteArray = nullptr;
    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

QStatus SQLStorage::RemovePolicy(const Application& app)
{
    QStatus funcStatus = ER_FAIL;
    storageMutex.Lock(__FILE__, __LINE__);

    Application tmp = app;
    funcStatus = GetManagedApplication(tmp);
    if (ER_OK != funcStatus) {
        QCC_LogError(funcStatus, ("Unknown application !"));
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }

    if (app.keyInfo.empty()) {
        funcStatus = ER_FAIL;
        QCC_LogError(funcStatus, ("Empty key info!"));
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }

    size_t keyInfoExportSize;
    uint8_t* publicKeyInfo = nullptr;
    funcStatus = ExportKeyInfo(app.keyInfo, &publicKeyInfo, keyInfoExportSize);
    if (ER_OK != funcStatus) {
        QCC_LogError(funcStatus, ("Failed to export public keyInfo"));
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }

    sqlite3_stmt* statement = nullptr;
    string sqlStmtText = "UPDATE ";
    sqlStmtText.append(CLAIMED_APPS_TABLE_NAME);
    sqlStmtText.append(" SET POLICY = NULL WHERE APPLICATION_PUBKEY = ?");

    do {
        int sqlRetCode = SQLITE_OK;
        int keyPosition = 1;

        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, nullptr);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }

        sqlRetCode = sqlite3_bind_blob(statement, keyPosition,
                                       publicKeyInfo, keyInfoExportSize,
                                       SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }
    } while (0);

    funcStatus = StepAndFinalizeSqlStmt(statement);
    delete[]publicKeyInfo;
    publicKeyInfo = nullptr;

    storageMutex.Unlock(__FILE__, __LINE__);
    return funcStatus;
}

QStatus SQLStorage::StorePolicy(const Application& app, const PermissionPolicy& policy)
{
    storageMutex.Lock(__FILE__, __LINE__);
    QStatus funcStatus = ER_FAIL;
    uint8_t* byteArray = nullptr;
    size_t size = 0;

    if (ER_OK == (funcStatus = Util::GetPolicyByteArray(policy, &byteArray, &size))) { // Util reports in case of serialization errors
        if (ER_OK != (funcStatus = StorePolicyOrManifest(app,  byteArray, size, "POLICY"))) {
            QCC_LogError(funcStatus, ("Failed to store policy !"));
        }
    }

    delete[]byteArray;
    byteArray = nullptr;

    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

QStatus SQLStorage::StoreCertificate(const Application& app, CertificateX509& certificate,
                                     const bool update)
{
    storageMutex.Lock(__FILE__, __LINE__);

    sqlite3_stmt* statement = nullptr;
    string sqlStmtText = "";
    QStatus funcStatus = ER_OK;

    if (app.keyInfo.empty()) {
        funcStatus = ER_FAIL;
        QCC_LogError(funcStatus, ("Empty key info!"));
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }

    if (update) {
        sqlStmtText = "INSERT OR REPLACE INTO ";
    } else {
        sqlStmtText = "INSERT INTO ";
    }

    switch (certificate.GetType()) {
    case CertificateX509::IDENTITY_CERTIFICATE: {
            sqlStmtText.append(IDENTITY_CERTS_TABLE_NAME);
            sqlStmtText.append(" (SUBJECT_KEYINFO, ISSUER"
                               ", DER"
                               ", GUID) VALUES (?, ?, ?, ?)");
        }
        break;

    case CertificateX509::MEMBERSHIP_CERTIFICATE: {
            sqlStmtText.append(MEMBERSHIP_CERTS_TABLE_NAME);
            sqlStmtText.append(" (SUBJECT_KEYINFO, ISSUER"
                               ", DER"
                               ", GUID) VALUES (?, ?, ?, ?)");
        }
        break;

    default: {
            funcStatus = ER_FAIL;
            QCC_LogError(funcStatus, ("Unsupported certificate type !"));
            storageMutex.Unlock(__FILE__, __LINE__);
            return funcStatus;
        }
    }

    funcStatus = BindCertForStorage(app, certificate, sqlStmtText.c_str(),
                                    &statement);

    if (ER_OK != funcStatus) {
        QCC_LogError(funcStatus, ("Binding values of certificate for storage has failed"));
        StepAndFinalizeSqlStmt(statement);
    } else {
        funcStatus = StepAndFinalizeSqlStmt(statement);
    }
    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

QStatus SQLStorage::GetMembershipCertificates(const Application& app, const MembershipCertificate& certificate,
                                              MembershipCertificateChain& certificates) const
{
    storageMutex.Lock(__FILE__, __LINE__);

    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement = nullptr;
    QStatus funcStatus = ER_FAIL;

    if (app.keyInfo.empty()) {
        QCC_LogError(funcStatus, ("Empty key info!"));
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }

    funcStatus = PrepareMembershipCertificateQuery(app, certificate, &statement);
    if (ER_OK != funcStatus) {
        storageMutex.Unlock(__FILE__, __LINE__);
        QCC_LogError(funcStatus, ("PrepareMembershipCertificateQuery"));
        return funcStatus;
    }

    int derColumn = 2;
    int derSizeColumn =  sqlite3_column_count(statement) - 1; // Length of DER encoding is returned at the end.

    while (SQLITE_ROW == (sqlRetCode = sqlite3_step(statement))) {
        MembershipCertificate cert;
        size_t size = sqlite3_column_int(statement, derSizeColumn);
        qcc::String der((const char*)sqlite3_column_blob(statement, derColumn), size);

        funcStatus = cert.DecodeCertificateDER(der);

        if (ER_OK != funcStatus) {
            QCC_LogError(funcStatus, ("Failed to load certificate!"));
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

QStatus SQLStorage::GetCertificate(const Application& app, CertificateX509& cert)
{
    storageMutex.Lock(__FILE__, __LINE__);

    sqlite3_stmt* statement = nullptr;
    int sqlRetCode = SQLITE_OK;
    QStatus funcStatus = ER_OK;
    string sqlStmtText = "";
    string tableName = "";
    string groupId = "";
    size_t keyInfoExportSize;
    uint8_t* publicKeyInfo = nullptr;

    if (app.keyInfo.empty()) {
        funcStatus = ER_FAIL;
        QCC_LogError(funcStatus, ("Empty key info!"));
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }

    do {
        sqlStmtText = "SELECT DER, LENGTH(DER) FROM ";

        switch (cert.GetType()) {
        case CertificateX509::IDENTITY_CERTIFICATE: {
                tableName = IDENTITY_CERTS_TABLE_NAME;
                sqlStmtText += IDENTITY_CERTS_TABLE_NAME;
                sqlStmtText += " WHERE SUBJECT_KEYINFO = ? ";
            }
            break;

        case CertificateX509::MEMBERSHIP_CERTIFICATE: {
                tableName = MEMBERSHIP_CERTS_TABLE_NAME;
                sqlStmtText += MEMBERSHIP_CERTS_TABLE_NAME;
                sqlStmtText += " WHERE SUBJECT_KEYINFO = ? AND GUID = ? ";
                groupId =
                    dynamic_cast<MembershipCertificate&>(cert).GetGuild().ToString().c_str();
            }
            break;

        default: {
                funcStatus = ER_FAIL;
                QCC_LogError(funcStatus, ("Unsupported certificate type !"));
                storageMutex.Unlock(__FILE__, __LINE__);
                return funcStatus;
            }
        }

        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, nullptr);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }

        funcStatus = ExportKeyInfo(app.keyInfo, &publicKeyInfo, keyInfoExportSize);

        if (ER_OK != funcStatus) {
            QCC_LogError(funcStatus, ("Failed to export public keyInfo"));
            break;
        }
        sqlRetCode = sqlite3_bind_blob(statement, 1, publicKeyInfo,
                                       keyInfoExportSize, SQLITE_TRANSIENT);

        if (CertificateX509::MEMBERSHIP_CERTIFICATE == cert.GetType()) {
            sqlRetCode |= sqlite3_bind_text(statement, 2, groupId.c_str(), -1,
                                            SQLITE_TRANSIENT);
        }

        if (SQLITE_ROW == (sqlRetCode = sqlite3_step(statement))) {
            /*********************Common to all certificates*****************/
            int derSize = sqlite3_column_int(statement, 1);     // DER is never empty
            qcc::String der((const char*)sqlite3_column_blob(statement, 0), derSize);
            funcStatus =  cert.DecodeCertificateDER(der);
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
    delete[]publicKeyInfo;
    publicKeyInfo = nullptr;
    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

QStatus SQLStorage::RemoveCertificate(const Application& app, CertificateX509& cert)
{
    storageMutex.Lock(__FILE__, __LINE__);

    sqlite3_stmt* statement = nullptr;
    string sqlStmtText = "";
    int sqlRetCode = SQLITE_OK;
    QStatus funcStatus = ER_FAIL;
    string certTableName = "";
    string whereKeys = "";
    size_t keyInfoExportSize;
    uint8_t* publicKeyInfo = nullptr;

    if (app.keyInfo.empty()) {
        QCC_LogError(funcStatus, ("Empty key info!"));
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }

    switch (cert.GetType()) {
    case CertificateX509::IDENTITY_CERTIFICATE: {
            certTableName = IDENTITY_CERTS_TABLE_NAME;
            whereKeys = " WHERE SUBJECT_KEYINFO = ? ";
        }
        break;

    case CertificateX509::MEMBERSHIP_CERTIFICATE: {
            certTableName = MEMBERSHIP_CERTS_TABLE_NAME;
            whereKeys = " WHERE SUBJECT_KEYINFO = ? AND GUID = ? ";
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
                                        -1, &statement, nullptr);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            break;
        }

        funcStatus = ExportKeyInfo(app.keyInfo, &publicKeyInfo, keyInfoExportSize);
        if (ER_OK != funcStatus) {
            QCC_LogError(funcStatus, ("Failed to export public keyInfo"));
            break;
        }
        sqlRetCode = sqlite3_bind_blob(statement, 1, publicKeyInfo,
                                       keyInfoExportSize, SQLITE_TRANSIENT);
        if (CertificateX509::MEMBERSHIP_CERTIFICATE == cert.GetType()) {
            sqlRetCode |=
                sqlite3_bind_text(statement, 2,
                                  dynamic_cast<MembershipCertificate&>(cert).GetGuild().
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
    delete[]publicKeyInfo;
    publicKeyInfo = nullptr;
    storageMutex.Unlock(__FILE__, __LINE__);

    return funcStatus;
}

QStatus SQLStorage::StoreGroup(const GroupInfo& groupInfo)
{
    QStatus funcStatus = ER_FAIL;
    storageMutex.Lock(__FILE__, __LINE__);

    bool update;
    GroupInfo tmp = groupInfo; // to avoid const cast
    funcStatus = GetGroup(tmp);
    if (ER_OK == funcStatus) {
        update = true;
    } else if (ER_END_OF_DATA == funcStatus) {
        update = false;
    } else {
        QCC_LogError(funcStatus, ("Could not determine update status for group."));
        storageMutex.Unlock(__FILE__, __LINE__);
        return funcStatus;
    }

    funcStatus = StoreInfo(INFO_GROUP,
                           groupInfo.authority, groupInfo.guid, groupInfo.name, groupInfo.desc, update);
    storageMutex.Unlock(__FILE__, __LINE__);
    return funcStatus;
}

QStatus SQLStorage::RemoveGroup(const GroupInfo& groupInfo, vector<Application>& appsToSync)
{
    QStatus funcStatus = ER_FAIL;
    storageMutex.Lock(__FILE__, __LINE__);

    GroupInfo tmp = groupInfo; // to avoid const cast
    if (ER_OK != (funcStatus = GetGroup(tmp))) {
        QCC_LogError(funcStatus, ("Group does not exist."));
    } else {
        funcStatus = RemoveInfo(INFO_GROUP, groupInfo.authority, groupInfo.guid, appsToSync);
    }

    storageMutex.Unlock(__FILE__, __LINE__);
    return funcStatus;
}

QStatus SQLStorage::GetGroup(GroupInfo& groupInfo) const
{
    QStatus funcStatus = ER_FAIL;
    storageMutex.Lock(__FILE__, __LINE__);

    funcStatus =
        GetInfo(INFO_GROUP, groupInfo.authority, groupInfo.guid, groupInfo.name, groupInfo.desc);

    storageMutex.Unlock(__FILE__, __LINE__);
    return funcStatus;
}

QStatus SQLStorage::GetGroups(vector<GroupInfo>& groupsInfo) const
{
    storageMutex.Lock(__FILE__, __LINE__);

    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement = nullptr;
    string sqlStmtText = "";
    QStatus funcStatus = ER_OK;

    sqlStmtText = "SELECT NAME, DESC, AUTHORITY, LENGTH(AUTHORITY), ID FROM ";
    sqlStmtText.append(GROUPS_TABLE_NAME);

    /* Prepare the sql query */
    sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(), -1,
                                    &statement, nullptr);
    if (SQLITE_OK != sqlRetCode) {
        funcStatus = ER_FAIL;
        LOGSQLERROR(funcStatus);
    }

    /* Iterate over all the rows in the query */
    while (SQLITE_ROW == (sqlRetCode = sqlite3_step(statement))) {
        GroupInfo info;

        info.name.assign((const char*)sqlite3_column_text(statement, 0));
        info.desc.assign((const char*)sqlite3_column_text(statement, 1));
        funcStatus = info.authority.Import((const uint8_t*)sqlite3_column_blob(statement, 2),
                                           sqlite3_column_int(statement, 3));
        if (ER_OK != funcStatus) {
            QCC_LogError(funcStatus, ("Failed to import auth Info %i", sqlite3_column_int(statement, 3)));
            break;
        }
        info.guid = GUID128((const char*)sqlite3_column_text(statement, 4));

        groupsInfo.push_back(info);
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

    string desc; // placeholder
    funcStatus = StoreInfo(INFO_IDENTITY, idInfo.authority, idInfo.guid, idInfo.name, desc, update);
    storageMutex.Unlock(__FILE__, __LINE__);
    return funcStatus;
}

QStatus SQLStorage::RemoveIdentity(const IdentityInfo& idInfo, vector<Application>& appsToSync)
{
    QStatus funcStatus = ER_FAIL;

    storageMutex.Lock(__FILE__, __LINE__);

    IdentityInfo tmp = idInfo; // to avoid const cast
    if (ER_OK != (funcStatus = GetIdentity(tmp))) {
        QCC_LogError(funcStatus, ("Identity does not exist."));
    } else {
        funcStatus = RemoveInfo(INFO_IDENTITY, idInfo.authority, idInfo.guid, appsToSync);
    }
    storageMutex.Unlock(__FILE__, __LINE__);
    return funcStatus;
}

QStatus SQLStorage::GetIdentity(IdentityInfo& idInfo) const
{
    QStatus funcStatus = ER_FAIL;
    storageMutex.Lock(__FILE__, __LINE__);

    string desc; // placeholder
    funcStatus = GetInfo(INFO_IDENTITY, idInfo.authority, idInfo.guid, idInfo.name, desc);

    storageMutex.Unlock(__FILE__, __LINE__);
    return funcStatus;
}

QStatus SQLStorage::GetIdentities(vector<IdentityInfo>& idInfos) const
{
    storageMutex.Lock(__FILE__, __LINE__);

    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement = nullptr;
    string sqlStmtText = "";
    QStatus funcStatus = ER_OK;

    sqlStmtText = "SELECT NAME, AUTHORITY, LENGTH(AUTHORITY), ID FROM ";
    sqlStmtText.append(IDENTITY_TABLE_NAME);

    /* Prepare the sql query */
    sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(), -1,
                                    &statement, nullptr);
    if (SQLITE_OK != sqlRetCode) {
        funcStatus = ER_FAIL;
        LOGSQLERROR(funcStatus);
    }

    /* Iterate over all the rows in the query */
    KeyInfoNISTP256 authority;
    while (SQLITE_ROW == (sqlRetCode = sqlite3_step(statement))) {
        IdentityInfo info;

        info.name.assign((const char*)sqlite3_column_text(statement, 0));
        funcStatus = authority.Import((const uint8_t*)sqlite3_column_blob(statement, 1),
                                      sqlite3_column_int(statement, 2));
        if (ER_OK != funcStatus) {
            break;
        }
        info.authority = authority;
        info.guid = GUID128((const char*)sqlite3_column_text(statement, 3));

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

    if (nativeStorageDB != nullptr) {
        if (sqlite3_close(nativeStorageDB) != SQLITE_OK) {
            LOGSQLERROR(ER_FAIL);
        }
        nativeStorageDB = nullptr;
    }

    remove(GetStoragePath().c_str());
    storageMutex.Unlock(__FILE__, __LINE__);
}

SQLStorage::~SQLStorage()
{
    storageMutex.Lock(__FILE__, __LINE__);

    //TODO :: change to sqlite3_close_v2 once Jenkins machines allow for it
    if (nativeStorageDB != nullptr) {
        if (sqlite3_close(nativeStorageDB) != SQLITE_OK) {
            LOGSQLERROR(ER_FAIL);
        }
        nativeStorageDB = nullptr;
    }

    storageMutex.Unlock(__FILE__, __LINE__);
}

/*************************************************PRIVATE*********************************************************/

QStatus SQLStorage::BindCertForStorage(const Application& app, CertificateX509& cert,
                                       const char* sqlStmtText, sqlite3_stmt*
                                       * statement)
{
    int sqlRetCode = SQLITE_OK;                                        // Equal zero
    QStatus funcStatus = ER_OK;
    int column = 1;
    size_t keyInfoExportSize;
    uint8_t* publicKeyInfo = nullptr;

    do {
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText, -1,
                                        statement, nullptr);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            break;
        }

        /*********************Common to all certificates*****************/

        if (*(cert.GetSubjectPublicKey()) != *(app.keyInfo.GetPublicKey())) {
            funcStatus = ER_FAIL;
            QCC_LogError(funcStatus, ("Public key mismatch!"));
            break;
        }

        funcStatus = ExportKeyInfo(app.keyInfo, &publicKeyInfo, keyInfoExportSize);
        if (ER_OK != funcStatus) {
            QCC_LogError(funcStatus, ("Failed to export public key Info"));
            break;
        }

        sqlRetCode = sqlite3_bind_blob(*statement, column,
                                       publicKeyInfo, keyInfoExportSize,
                                       SQLITE_TRANSIENT);

        sqlRetCode |= sqlite3_bind_blob(*statement, ++column,
                                        cert.GetAuthorityKeyId().data(),
                                        cert.GetAuthorityKeyId().size(),
                                        SQLITE_TRANSIENT);
        qcc::String der;
        funcStatus = cert.EncodeCertificateDER(der);
        if (funcStatus == ER_OK) {
            sqlRetCode |= sqlite3_bind_blob(*statement, ++column,
                                            (const uint8_t*)der.data(),
                                            der.size(),
                                            SQLITE_TRANSIENT);
        } else {
            funcStatus = ER_FAIL;
            break;
        }

        /****************************************************************/

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            break;
        }

        switch (cert.GetType()) {
        case CertificateX509::IDENTITY_CERTIFICATE: {
                const IdentityCertificate& idCert =
                    dynamic_cast<const IdentityCertificate&>(cert);
                sqlRetCode |= sqlite3_bind_text(*statement, ++column,
                                                idCert.GetAlias().c_str(), -1,
                                                SQLITE_TRANSIENT);

                if (SQLITE_OK != sqlRetCode) {
                    funcStatus = ER_FAIL;
                }
            }
            break;

        case CertificateX509::MEMBERSHIP_CERTIFICATE: {
                CertificateX509& c = const_cast<CertificateX509&>(cert);
                MembershipCertificate& memCert = dynamic_cast<MembershipCertificate&>(c);
                sqlRetCode |= sqlite3_bind_text(*statement, ++column,
                                                memCert.GetGuild().ToString().c_str(), -1,
                                                SQLITE_TRANSIENT);

                if (SQLITE_OK != sqlRetCode) {
                    funcStatus = ER_FAIL;
                }
            }
            break;

        default: {
                delete[]publicKeyInfo;
                publicKeyInfo = nullptr;
                QCC_LogError(ER_FAIL, ("Unsupported certificate type !"));
                return ER_FAIL;
            }
        }
    } while (0);

    if (SQLITE_OK != sqlRetCode) {
        funcStatus = ER_FAIL;
        LOGSQLERROR(funcStatus);
    }
    delete[]publicKeyInfo;
    publicKeyInfo = nullptr;
    return funcStatus;
}

QStatus SQLStorage::StepAndFinalizeSqlStmt(sqlite3_stmt* statement) const
{
    int sqlRetCode = SQLITE_OK;
    QStatus funcStatus = ER_OK;

    if (nullptr == statement) {
        QCC_LogError(ER_FAIL, ("nullptr statement argument"));
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

string SQLStorage::GetStoragePath() const
{
    string pathkey(STORAGE_FILEPATH_KEY);
    string storagePath;
    map<string, string>::const_iterator pathItr =
        storageConfig.settings.find(pathkey);
    if (pathItr != storageConfig.settings.end()) {
        storagePath = pathItr->second;
    }

    return storagePath;
}

QStatus SQLStorage::Init()
{
    int sqlRetCode = SQLITE_OK;
    string sqlStmtText = "";
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

        string storagePath = GetStoragePath();
        if (storagePath.empty()) {
            funcStatus = ER_FAIL;
            QCC_DbgHLPrintf(("Invalid path to be used for storage !!"));
            break;
        }
        sqlRetCode = sqlite3_open(storagePath.c_str(), &nativeStorageDB);

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            if (nativeStorageDB != nullptr) {
                sqlite3_close(nativeStorageDB);
            }
            break;
        }

        sqlStmtText = CLAIMED_APPLICATIONS_TABLE_SCHEMA;
        sqlStmtText.append(IDENTITY_CERTS_TABLE_SCHEMA);
        sqlStmtText.append(MEMBERSHIP_CERTS_TABLE_SCHEMA);
        sqlStmtText.append(GROUPS_TABLE_SCHEMA);
        sqlStmtText.append(IDENTITY_TABLE_SCHEMA);
        sqlStmtText.append(SERIALNUMBER_TABLE_SCHEMA);
        sqlStmtText.append(DEFAULT_PRAGMAS);

        sqlRetCode = sqlite3_exec(nativeStorageDB, sqlStmtText.c_str(), nullptr, 0,
                                  nullptr);

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }
        funcStatus = InitSerialNumber();
    } while (0);

    return funcStatus;
}

QStatus SQLStorage::StoreInfo(InfoType type,
                              const KeyInfoNISTP256& auth,
                              const GUID128& guid,
                              const string& name,
                              const string& desc,
                              bool update)
{
    size_t authoritySize;
    uint8_t* authority = nullptr;
    QStatus funcStatus = ExportKeyInfo(auth, &authority, authoritySize);
    if (ER_OK != funcStatus) {
        return funcStatus;
    }

    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement;

    string sqlStmtText;
    if (update) {
        sqlStmtText = "UPDATE ";
        sqlStmtText.append(type == INFO_GROUP ? GROUPS_TABLE_NAME : IDENTITY_TABLE_NAME);
        sqlStmtText.append(" SET NAME = ?");
        if (type == INFO_GROUP) {
            sqlStmtText.append(", DESC = ?");
        }
        sqlStmtText.append(" WHERE AUTHORITY = ? AND ID = ?");
    } else {
        sqlStmtText = "INSERT INTO ";
        sqlStmtText.append(type == INFO_GROUP ? GROUPS_TABLE_NAME : IDENTITY_TABLE_NAME);
        sqlStmtText.append(" (NAME, ");
        if (type == INFO_GROUP) {
            sqlStmtText.append("DESC, ");
        }
        sqlStmtText.append("AUTHORITY, ID) VALUES (?, ?, ?");
        if (type == INFO_GROUP) {
            sqlStmtText.append(", ?");
        }
        sqlStmtText.append(")");
    }
    do {
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, nullptr);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }

        int i = 1;
        sqlRetCode |= sqlite3_bind_text(statement, i++, name.c_str(), -1, SQLITE_TRANSIENT);
        if (type == INFO_GROUP) {
            sqlRetCode |= sqlite3_bind_text(statement, i++, desc.c_str(), -1, SQLITE_TRANSIENT);
        }
        sqlRetCode |= sqlite3_bind_blob(statement, i++, authority, authoritySize, SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_text(statement, i++, guid.ToString().c_str(), -1, SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }
    } while (0);

    funcStatus = StepAndFinalizeSqlStmt(statement);
    delete[] authority;
    authority = nullptr;

    return funcStatus;
}

QStatus SQLStorage::GetInfo(InfoType type,
                            const KeyInfoNISTP256& auth,
                            const GUID128& guid,
                            string& name,
                            string& desc) const
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

    size_t authoritySize;
    uint8_t* authority = nullptr;
    funcStatus = ExportKeyInfo(auth, &authority, authoritySize);
    if (ER_OK != funcStatus) {
        return funcStatus;
    }

    sqlite3_stmt* statement = nullptr;
    string sqlStmtText = "";
    int sqlRetCode = SQLITE_OK;

    sqlStmtText = "SELECT NAME";
    if (type == INFO_GROUP) {
        sqlStmtText.append(", DESC");
    }
    sqlStmtText.append(" FROM ");
    sqlStmtText.append(type == INFO_GROUP ? GROUPS_TABLE_NAME : IDENTITY_TABLE_NAME);
    sqlStmtText.append(" WHERE AUTHORITY = ? AND ID = ?");
    do {
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, nullptr);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }

        sqlRetCode |= sqlite3_bind_blob(statement, 1, authority, authoritySize, SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_text(statement, 2, guid.ToString().c_str(), -1, SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }

        sqlRetCode = sqlite3_step(statement);
        if (SQLITE_ROW == sqlRetCode) {
            name.assign((const char*)sqlite3_column_text(statement, 0));
            if (type == INFO_GROUP) {
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
    delete[] authority;
    authority = nullptr;
    return funcStatus;
}

QStatus SQLStorage::ExportKeyInfo(const KeyInfoNISTP256& keyInfo, uint8_t** byteArray, size_t& byteArraySize)
{
    QStatus funcStatus = ER_OK;

    *byteArray = nullptr;
    byteArraySize = 0;
    byteArraySize = keyInfo.GetExportSize();

    if (byteArraySize > 0) {
        *byteArray = new uint8_t[byteArraySize];
        keyInfo.Export(*byteArray);
        return funcStatus;
    }

    funcStatus = ER_FAIL;
    QCC_LogError(funcStatus, ("Failed to export keyInfo"));

    return funcStatus;
}

QStatus SQLStorage::RemoveInfo(InfoType type,
                               const KeyInfoNISTP256& auth,
                               const GUID128& guid,
                               vector<Application>& appsToSync)
{
    size_t authoritySize;
    uint8_t* authority = nullptr;
    QStatus funcStatus = ExportKeyInfo(auth, &authority, authoritySize);
    if (ER_OK != funcStatus) {
        return funcStatus;
    }

    if (ER_OK != GetApplicationsPerGuid(type, guid, appsToSync)) {
        QCC_DbgHLPrintf(("No affected managed application(s) was/were found..."));
    }

    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement;

    string sqlStmtText = "DELETE FROM ";
    sqlStmtText.append(type == INFO_GROUP ? GROUPS_TABLE_NAME : IDENTITY_TABLE_NAME);
    sqlStmtText.append(" WHERE AUTHORITY = ? AND ID = ?");

    do {
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, nullptr);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }

        sqlRetCode |= sqlite3_bind_blob(statement, 1, authority,
                                        authoritySize, SQLITE_TRANSIENT);
        sqlRetCode |= sqlite3_bind_text(statement, 2, guid.ToString().c_str(),
                                        -1, SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }
    } while (0);

    funcStatus = StepAndFinalizeSqlStmt(statement);
    delete[] authority;
    authority = nullptr;
    return funcStatus;
}

QStatus SQLStorage::GetPolicyOrManifest(const Application& app,
                                        const char* type,
                                        uint8_t** byteArray,
                                        size_t* size) const
{
    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement = nullptr;
    string sqlStmtText = "";
    QStatus funcStatus = ER_OK;
    size_t keyInfoExportSize;
    uint8_t* publicKeyInfo = nullptr;

    *size = 0;
    *byteArray = nullptr;

    if (app.keyInfo.empty()) {
        funcStatus = ER_FAIL;
        QCC_LogError(funcStatus, ("Empty key info!"));
        return funcStatus;
    }

    if ((strcmp(type, "MANIFEST") != 0) &&  (strcmp(type, "POLICY") != 0)) {
        funcStatus = ER_FAIL;
        QCC_LogError(funcStatus, ("Invalid field type to retrieve!"));
        return funcStatus;
    }

    do {
        sqlStmtText = "SELECT ";
        sqlStmtText += type;
        sqlStmtText += ", LENGTH(" + string(type) + ")";
        sqlStmtText += (" FROM ");
        sqlStmtText += CLAIMED_APPS_TABLE_NAME;
        sqlStmtText += " WHERE APPLICATION_PUBKEY = ?";

        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, nullptr);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }

        funcStatus = ExportKeyInfo(app.keyInfo, &publicKeyInfo, keyInfoExportSize);
        if (ER_OK != funcStatus) {
            QCC_LogError(funcStatus, ("Failed to export public key"));
            break;
        }
        sqlRetCode = sqlite3_bind_blob(statement, 1, publicKeyInfo,
                                       keyInfoExportSize, SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }
        sqlRetCode = sqlite3_step(statement);
        if (SQLITE_ROW == sqlRetCode) {
            *size = sqlite3_column_int(statement, 1);
            if (*size > 0) {
                *byteArray = new uint8_t[*size];
                memcpy(*byteArray, (const uint8_t*)sqlite3_column_blob(statement, 0), *size);
            } else {
                funcStatus = ER_END_OF_DATA;
                QCC_DbgHLPrintf(("Application has no %s !", type));
                break;
            }
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
    delete[]publicKeyInfo;
    publicKeyInfo = nullptr;
    return funcStatus;
}

QStatus SQLStorage::StorePolicyOrManifest(const Application& app,  const uint8_t* byteArray,
                                          const size_t size, const char* type)
{
    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement = nullptr;
    QStatus funcStatus = ER_FAIL;
    string sqlStmtText;
    size_t keyInfoExportSize;
    uint8_t* publicKeyInfo = nullptr;

    sqlStmtText = "UPDATE ";
    sqlStmtText.append(CLAIMED_APPS_TABLE_NAME);

    do {
        if (strcmp(type, "MANIFEST") == 0) {
            sqlStmtText.append(" SET MANIFEST = ? ");
        } else if (strcmp(type, "POLICY") == 0) {
            sqlStmtText.append(" SET POLICY = ? ");
        } else {
            funcStatus = ER_FAIL;
            QCC_LogError(funcStatus, ("Invalid field type to store!"));
            break;
        }

        sqlStmtText.append("WHERE APPLICATION_PUBKEY = ?");

        if (app.keyInfo.empty()) {
            funcStatus = ER_FAIL;
            QCC_LogError(funcStatus, ("Empty key info !"));
            break;
        }

        funcStatus = ExportKeyInfo(app.keyInfo, &publicKeyInfo, keyInfoExportSize);

        if (ER_OK != funcStatus) {
            QCC_LogError(funcStatus, ("Failed to export public key"));
            break;
        }

        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, nullptr);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }

        sqlRetCode |= sqlite3_bind_blob(statement, 1, byteArray, size, SQLITE_TRANSIENT);

        sqlRetCode = sqlite3_bind_blob(statement, 2,
                                       publicKeyInfo, keyInfoExportSize,
                                       SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }
    } while (0);

    funcStatus = StepAndFinalizeSqlStmt(statement);
    delete[]publicKeyInfo;
    publicKeyInfo = nullptr;
    return funcStatus;
}

QStatus SQLStorage::InitSerialNumber()
{
    storageMutex.Lock(__FILE__, __LINE__);

    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement = nullptr;
    string sqlStmtText = "";
    QStatus funcStatus = ER_OK;

    sqlStmtText = "SELECT VALUE FROM ";
    sqlStmtText.append(SERIALNUMBER_TABLE_NAME);

    /* prepare the sql query */
    sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(), -1,
                                    &statement, nullptr);
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
        statement = nullptr;
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, nullptr);
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

QStatus SQLStorage::PrepareMembershipCertificateQuery(const Application& app,
                                                      const MembershipCertificate& certificate,
                                                      sqlite3_stmt** statement) const
{
    QStatus funcStatus = ER_OK;
    int sqlRetCode = SQLITE_OK;
    size_t keyInfoExportSize;
    uint8_t* publicKeyInfo = nullptr;

    funcStatus = ExportKeyInfo(app.keyInfo, &publicKeyInfo, keyInfoExportSize);
    if (ER_OK != funcStatus) {
        QCC_LogError(funcStatus, ("Failed to export public keyInfo"));
        return funcStatus;
    }

    MembershipCertificate& cert = const_cast<MembershipCertificate&>(certificate);
    string groupId;
    if (cert.IsGuildSet()) {
        groupId = cert.GetGuild().ToString().c_str();
    }

    string sqlStmtText = "SELECT *, LENGTH(DER) FROM ";
    sqlStmtText += MEMBERSHIP_CERTS_TABLE_NAME;

    if ((!app.keyInfo.empty()) && groupId.empty()) {
        sqlStmtText += " WHERE SUBJECT_KEYINFO = ?";
    } else if ((!app.keyInfo.empty()) && (!groupId.empty())) {
        sqlStmtText += " WHERE SUBJECT_KEYINFO = ? AND GUID = ? ";
    } else if ((app.keyInfo.empty()) && (!groupId.empty())) {
        sqlStmtText += " WHERE GUID = ?";
    }

    do {
        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, statement, nullptr);
        if (SQLITE_OK != sqlRetCode) {
            break;
        }

        if (!app.keyInfo.empty()) {
            sqlRetCode = sqlite3_bind_blob(*statement, 1, publicKeyInfo,
                                           keyInfoExportSize, SQLITE_TRANSIENT);
            if (SQLITE_OK != sqlRetCode) {
                break;
            }
        }

        if (!groupId.empty()) {
            sqlRetCode = sqlite3_bind_text(*statement, 2, groupId.c_str(), -1,
                                           SQLITE_TRANSIENT);
        }
    } while (0);

    if (SQLITE_OK != sqlRetCode) {
        funcStatus = ER_FAIL;
        LOGSQLERROR(funcStatus);
    }
    delete[]publicKeyInfo;
    publicKeyInfo = nullptr;
    return funcStatus;
}

QStatus SQLStorage::GetApplicationsPerGuid(const InfoType type, const GUID128& guid, vector<Application>& apps)
{
    int sqlRetCode = SQLITE_OK;
    sqlite3_stmt* statement = nullptr;
    string sqlStmtText = "";
    QStatus funcStatus = ER_OK;

    string certTableWhere;
    switch (type) {
    case INFO_GROUP:
        certTableWhere = MEMBERSHIP_CERTS_TABLE_NAME;
        break;

    case INFO_IDENTITY:
        certTableWhere = IDENTITY_CERTS_TABLE_NAME;
        break;

    default:
        return ER_FAIL;
    }

    do {
        sqlStmtText = "SELECT LENGTH(APPLICATION_PUBKEY), APPLICATION_PUBKEY, SYNC_STATE FROM ";
        sqlStmtText += CLAIMED_APPS_TABLE_NAME;
        sqlStmtText += " WHERE APPLICATION_PUBKEY IN ";
        sqlStmtText += "( SELECT  SUBJECT_KEYINFO FROM " + certTableWhere + " WHERE GUID = ?);";

        sqlRetCode = sqlite3_prepare_v2(nativeStorageDB, sqlStmtText.c_str(),
                                        -1, &statement, nullptr);
        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }

        sqlRetCode = sqlite3_bind_text(statement, 1, guid.ToString().c_str(),
                                       -1, SQLITE_TRANSIENT);

        if (SQLITE_OK != sqlRetCode) {
            funcStatus = ER_FAIL;
            LOGSQLERROR(funcStatus);
            break;
        }
        sqlRetCode = sqlite3_step(statement);

        switch (sqlRetCode) {
        case SQLITE_DONE:
            funcStatus = ER_END_OF_DATA;
            break;

        case SQLITE_ROW:
            while (SQLITE_ROW == sqlRetCode) {
                Application app;
                size_t pubKeyInfoImportSize = (size_t)sqlite3_column_int(statement, 0);
                funcStatus =
                    app.keyInfo.Import((const uint8_t*)sqlite3_column_blob(statement, 1), pubKeyInfoImportSize);
                app.syncState = static_cast<ApplicationSyncState>(sqlite3_column_int(statement, 2));
                apps.push_back(app);
                sqlRetCode = sqlite3_step(statement);
            }
            break;

        default:
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
}
}
#undef QCC_MODULE
