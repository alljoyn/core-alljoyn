/**
 * @file
 * AllJoyn-Daemon Config file database class
 */

/******************************************************************************
 * Copyright (c) 2010-2011, 2014, AllSeen Alliance. All rights reserved.
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
#ifndef _CONFIGDB_H
#define _CONFIGDB_H

#include <qcc/platform.h>

#include <assert.h>
#include <set>
#include <map>

#include <qcc/ManagedObj.h>
#include <qcc/RWLock.h>
#include <qcc/String.h>
#include <qcc/StringMapKey.h>
#include <qcc/XmlElement.h>

#include "Bus.h"
#ifdef ENABLE_POLICYDB
#include "NameTable.h"
#include "PolicyDB.h"
#endif

/**
 * Configuration file database class.
 */
namespace ajn {

#ifdef ENABLE_POLICYDB
class ConfigDB : public NameListener {
#else
class ConfigDB {
#endif
  public:
    /** Typedef for list of daemon listen addresses. */
    typedef std::set<qcc::String> _ListenList;
    typedef qcc::ManagedObj<_ListenList> ListenList;

    /** Typedef for map of certain resource limits - also used for flags. */
    typedef std::unordered_map<qcc::StringMapKey, uint32_t> LimitMap;

    /** Typedef for map of properties. */
    typedef std::unordered_map<qcc::StringMapKey, qcc::String> PropertyMap;

    /**
     * Get a pointer to the ConfigDB singleton object.
     *
     * @return  A pointer to the ConfigDB singleton object.
     */
    static ConfigDB* GetConfigDB() { assert(singleton); return singleton; }

    /**
     * Constructor.  This must be called before any other code can call
     * ConifgDB::GetConfigDB().  This should only be called once in a routing
     * node program and the object's lifetime should be that of the routing
     * node program.  The easiest way to do this is to just instantiate
     * ConfigDB once as an object on the stack at the begining of main().
     * Other code that accesses the ConfigDB should get a pointer from
     * ConfigDB::GetConfigDB().
     *
     * @param   defaultXml      XML with the default configuration for limits,
     *                          flags, and properties.
     * @param   fileName        [optional] Name of the config file to load.
     */
    ConfigDB(const qcc::String defaultXml, const qcc::String fileName = "");

    /**
     * Destructor.
     */
    ~ConfigDB();

    /**
     * Shutdown configDB
     */
    void Shutdown() { stopping = true; }

#ifdef ENABLE_POLICYDB
    /**
     * Get a reference to the PolicyDB managed object.
     *
     * @return  A reference to the PolicyDB managed object.
     */
    PolicyDB GetPolicyDB() const
    {
        rwlock.RDLock();
        PolicyDB pdb = db->policyDB;
        rwlock.Unlock();
        return pdb;
    }
#endif

    /**
     * (Re)Load the configuration.
     *
     * @param   bus     [optional] Pointer to the Bus object (may be NULL if
     *                  the Bus has not been created yet.)
     *
     * @return  true if config file loaded successfully.
     */
    bool LoadConfig(Bus* bus = NULL);

    /**
     * Get the bus type specified in the config file.
     *
     * @return  The bus type.
     */
    qcc::String GetType() const
    {
        rwlock.RDLock();
        qcc::String type = db->type;
        rwlock.Unlock();
        return type;
    }

    /**
     * Get the username the daemon should run as.
     *
     * @return  The username.
     */
    qcc::String GetUser() const
    {
        rwlock.RDLock();
        qcc::String user = db->user;
        rwlock.Unlock();
        return user;
    }

    /**
     * Get the path to the file where the PID should be stored.
     *
     * @return  The file path for storing the PID.
     */
    qcc::String GetPidfile() const
    {
        rwlock.RDLock();
        qcc::String pidfile = db->pidfile;
        rwlock.Unlock();
        return pidfile;
    }

    /**
     * Get whether the daemon should fork off into its own autonomous process
     * or not.
     *
     * @return  true if daemon should fork.
     */
    bool GetFork() const
    {
        rwlock.RDLock();
        bool fork = db->fork;
        rwlock.Unlock();
        return fork;
    }

    /**
     * Get whether the daemon should keep its umask setting or not when forking.
     *
     * @return  true if daemon should keep its umask.
     */
    bool GetKeepUmask() const
    {
        rwlock.RDLock();
        bool keepUmask = db->keepUmask;
        rwlock.Unlock();
        return keepUmask;
    }

    /**
     * Get whether the daemon should send log messages to syslog.
     *
     * @return  true if daemon should send log mesages to syslog.
     */
    bool GetSyslog() const
    {
        rwlock.RDLock();
        bool syslog = db->syslog;
        rwlock.Unlock();
        return syslog;
    }

    /**
     * Get the list of listen address specifications.
     *
     * @return  List of listen address specifications.
     */
    ListenList GetListen() const
    {
        rwlock.RDLock();
        ListenList l = db->listenList;
        rwlock.Unlock();
        return l;
    }

    /**
     * Get the list of supported authentication mechanisms.
     *
     * @return  List of supported authentication mechanisms.
     */
    qcc::String GetAuth() const
    {
        rwlock.RDLock();
        qcc::String authList = db->authList;
        rwlock.Unlock();
        return authList;
    }

    /**
     * Get the value of a specific limit.
     *
     * @param key       The name of the limit to retrieve.
     * @param errVal    [OPTIONAL] Value to use if key not found.
     *
     * @return  Limit value.
     */
    const uint32_t GetLimit(const qcc::String& key, uint32_t errVal = 0) const
    {
        rwlock.RDLock();
        LimitMap::const_iterator it = db->limitMap.find(key);
        uint32_t limit = (it == db->limitMap.end()) ? errVal : it->second;
        rwlock.Unlock();
        return limit;
    }

    /**
     * Get the value of a specific flag.
     *
     * @param key       The name of the flag to retrieve.
     *
     * @return  Flag value if set; false if not set.
     */
    const bool GetFlag(const qcc::String& key, bool errVal = false) const
    {
        return GetLimit(key, errVal ? 1 : 0) == 1;
    }

    /**
     * Get the value of a specific property.
     *
     * @param key       The name of the property.
     * @param errVal    [OPTIONAL] Value to use if key not found.
     *
     * @return The property value if set, or the empty string.
     */
    qcc::String GetProperty(const qcc::String& key, const qcc::String errVal = "") const
    {
        rwlock.RDLock();
        PropertyMap::const_iterator it = db->propertyMap.find(key);
        qcc::String property = (it == db->propertyMap.end()) ? errVal : it->second;
        rwlock.Unlock();
        return property;
    }

  private:

    /** Internal Config database data structure. */
    struct DB {
        bool fork;      /**< Indicates whether daemon should fork or not. */
        bool syslog;    /**< Indicates whether daemon should send to syslog or stdout. */
        bool keepUmask; /**< Indicates whether daemon should keep its umask or not. */

        qcc::String type;       /**< Bus type. */
        qcc::String user;       /**< User identity daemon should run as. */
        qcc::String pidfile;    /**< File to store PID. */

        qcc::String authList;       /**< Authentication mechanism list. */
        ListenList listenList;      /**< Listen address list. */
        LimitMap limitMap;          /**< Resource limit map. */
        PropertyMap propertyMap;    /**< Property map. */

#ifdef ENABLE_POLICYDB
        PolicyDB policyDB;    /**< The Policy database. */
#endif

        /**
         * Constructor.
         */
        DB() : fork(false), syslog(false), keepUmask(false) { }

        /**
         * This is called when all config items have been processed.
         *
         * @param   bus     Pointer to the Bus object (may be NULL if the Bus
         *                  has not been created yet.)
         */
        void Finalize(Bus* bus)
        {
#ifdef ENABLE_POLICYDB
            policyDB->Finalize(bus);
#endif
        }

        /**
         * Parse the XML source stream.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ParseSource(const qcc::String& fileName, qcc::Source& src);

        /**
         * Parse the specified XML file.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ParseFile(const qcc::String& fileName, bool ignore_missing = false);

        /**
         * Parse the <auth/> tag.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessAuth(const qcc::String& fileName, const qcc::XmlElement& auth);

        /**
         * Parse the <busconfig/>
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessBusconfig(const qcc::String& fileName, const qcc::XmlElement& busconfig);

        /**
         * Parse the <fork/> tag.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessFork(const qcc::String& fileName, const qcc::XmlElement& fork);

        /**
         * Parse the <include/> tag.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessInclude(const qcc::String& fileName, const qcc::XmlElement& include);

        /**
         * Parse the <includedir/> tag.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessIncludedir(const qcc::String& fileName, const qcc::XmlElement& includedir);

        /**
         * Parse the <keep_umask/> tag.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessKeepUmask(const qcc::String& fileName, const qcc::XmlElement& keepUmask);

        /**
         * Parse the <limit/> tag.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessLimit(const qcc::String& fileName, const qcc::XmlElement& limit);

        /**
         * Parse the <flag/> tag.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessFlag(const qcc::String& fileName, const qcc::XmlElement& limit);

        /**
         * Parse the <property/> tag.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessProperty(const qcc::String& fileName, const qcc::XmlElement& limit);

        /**
         * Parse the <listen/> tag.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessListen(const qcc::String& fileName, const qcc::XmlElement& listen);

        /**
         * Parse the <pidfile/> tag.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessPidfile(const qcc::String& fileName, const qcc::XmlElement& pidfile);

#ifdef ENABLE_POLICYDB
        /**
         * Parse the <policy/> tag.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessPolicy(const qcc::String& fileName, const qcc::XmlElement& policy);
#endif

        /**
         * Parse the <syslog/> tag.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessSyslog(const qcc::String& fileName, const qcc::XmlElement& syslog);

        /**
         * Parse the <type/> tag.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessType(const qcc::String& fileName, const qcc::XmlElement& type);

        /**
         * Parse the <user/> tag.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessUser(const qcc::String& fileName, const qcc::XmlElement& user);
    };

    /**
     * Constructor.
     */
    ConfigDB();

    /**
     * Copy constructor.
     * ConfigDB may not be copy constructed.
     *
     * @param other   sink being copied.
     */
    ConfigDB(const ConfigDB& other);

    /**
     * Assignment operator.
     * ConfigDB may not be assigned.
     *
     * @param other   RHS of assignment.
     */
    ConfigDB& operator=(const ConfigDB& other);

#ifdef ENABLE_POLICYDB
    void NameOwnerChanged(const qcc::String& alias,
                          const qcc::String* oldOwner, SessionOpts::NameTransferType oldOwnerNameTransfer,
                          const qcc::String* newOwner, SessionOpts::NameTransferType newOwnerNameTransfer);
#endif

    const qcc::String defaultXml;   /**< Default configuration. */
    const qcc::String fileName;     /**< Config file to read. */
    DB* db;                         /**< The current config database storage object. */
    bool stopping;
    static ConfigDB* singleton;
    mutable qcc::RWLock rwlock;
};

}

#endif
