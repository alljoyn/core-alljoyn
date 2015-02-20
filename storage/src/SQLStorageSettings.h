/******************************************************************************
 * Copyright (c) 2015, AllSeen Alliance. All rights reserved.
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

#ifndef NATIVESTORAGESETTINGS_H_
#define NATIVESTORAGESETTINGS_H_

#define GUILDS_TABLE_NAME "GUILDS"
#define IDENTITY_TABLE_NAME "IDENTITIES"
#define CLAIMED_APPS_TABLE_NAME "CLAIMED_APPLICATIONS"
#define IDENTITY_CERTS_TABLE_NAME "IDENTITY_CERTS"
#define MEMBERSHIP_CERTS_TABLE_NAME "MEMBERSHIP_CERTS"
#define USER_EQ_CERTS_TABLE_NAME "USER_EQ_CERTS"
#define CERTSDATA_TABLE_NAME "CERTS_DATA"
#define SERIALNUMBER_TABLE_NAME "SERIALNUMBER"

#define GUILDS_TABLE_SCHEMA \
    "CREATE TABLE IF NOT EXISTS " GUILDS_TABLE_NAME \
    " (\
        AUTHORITY  BLOB NOT NULL,\
        ID         TEXT NOT NULL,\
        NAME       TEXT,\
        DESC       TEXT,\
        PRIMARY KEY(AUTHORITY, ID)\
); "

#define IDENTITY_TABLE_SCHEMA \
    "CREATE TABLE IF NOT EXISTS " IDENTITY_TABLE_NAME \
    " (\
        AUTHORITY BLOB NOT NULL,\
        ID        TEXT NOT NULL,\
        NAME      TEXT,\
        PRIMARY KEY(AUTHORITY, ID)\
); "

#define CLAIMED_APPLICATIONS_TABLE_SCHEMA \
    "CREATE TABLE IF NOT EXISTS " CLAIMED_APPS_TABLE_NAME \
    " (\
        APPLICATION_PUBKEY BLOB PRIMARY KEY    NOT NULL,\
        APP_NAME   TEXT,    \
        PEER_ID   TEXT,    \
        DEV_NAME   TEXT,    \
        USER_DEF_NAME   TEXT,\
        MANIFEST BLOB,\
        POLICY BLOB\
); "

#define IDENTITY_CERTS_TABLE_SCHEMA \
    "CREATE TABLE IF NOT EXISTS " IDENTITY_CERTS_TABLE_NAME \
    " (\
        SUBJECT  BLOB NOT NULL,\
        VERSION INT NOT NULL,\
        ISSUER BLOB NOT NULL,\
        VALIDITYFROM UNSIGNED BIG INT NOT NULL,\
        VALIDITYTO UNSIGNED BIG INT NOT NULL,\
        SN BLOB NOT NULL,\
        DATAID BLOB NOT NULL,\
        ALIAS BLOB NOT NULL,\
        USERNAME TEXT NOT NULL,\
        PRIMARY KEY(SUBJECT, DATAID),\
        FOREIGN KEY(SUBJECT) REFERENCES " CLAIMED_APPS_TABLE_NAME                                                                                                                                                                                                                                                                                                                                                                                    \
    " (APPLICATION_PUBKEY) ON DELETE CASCADE\
); "

#define MEMBERSHIP_CERTS_TABLE_SCHEMA \
    "CREATE TABLE IF NOT EXISTS " MEMBERSHIP_CERTS_TABLE_NAME \
    " (\
        SUBJECT BLOB NOT NULL,\
        VERSION TEXT NOT NULL,\
        ISSUER BLOB NOT NULL,\
        VALIDITYFROM UNSIGNED BIG INT NOT NULL,\
        VALIDITYTO UNSIGNED BIG INT NOT NULL,\
        SN BLOB NOT NULL,\
        DATAID BLOB NOT NULL,\
        DELEGATE BOOLEAN NOT NULL,\
        GUID TEXT NOT NULL,\
        PRIMARY KEY(SUBJECT, GUID),\
        FOREIGN KEY(SUBJECT) REFERENCES " CLAIMED_APPS_TABLE_NAME                                                                                                                                                                                                                                                                                                                                                                                    \
    " (APPLICATION_PUBKEY) ON DELETE CASCADE\
); "

#define USER_EQ_CERTS_TABLE_SCHEMA \
    "CREATE TABLE IF NOT EXISTS " USER_EQ_CERTS_TABLE_NAME \
    " (\
        SUBJECT BLOB NOT NULL,\
        VERSION TEXT NOT NULL,\
        ISSUER BLOB NOT NULL,\
        VALIDITYFROM UNSIGNED BIG INT NOT NULL,\
        VALIDITYTO UNSIGNED BIG INT NOT NULL,\
        SN BLOB NOT NULL,\
        PRIMARY KEY(SUBJECT),\
        FOREIGN KEY(SUBJECT) REFERENCES " CLAIMED_APPS_TABLE_NAME                                                                                                                                                                                                                                                                                                                                                                                    \
    " (APPLICATION_PUBKEY) ON DELETE CASCADE\
); "

#define CERTSDATA_TABLE_SCHEMA \
    "CREATE TABLE IF NOT EXISTS " CERTSDATA_TABLE_NAME \
    " (\
        ID BLOB NOT NULL,\
        DATA BLOB NOT NULL,\
		PRIMARY KEY(ID)\
); "

#define SERIALNUMBER_TABLE_SCHEMA \
    "CREATE TABLE IF NOT EXISTS " SERIALNUMBER_TABLE_NAME \
    " (\
        VALUE INT\
); "

#define DEFAULT_PRAGMAS \
    "PRAGMA encoding = \"UTF-8\";\
    PRAGMA foreign_keys = ON;\
    PRAGMA journal_mode = OFF; "

#endif
