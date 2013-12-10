/**
 * @file
 * AllJoyn-Daemon Config file database class
 */

/******************************************************************************
 * Copyright (c) 2010-2011, AllSeen Alliance. All rights reserved.
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

#include <set>
#include <map>

#include <qcc/String.h>
#include <qcc/StringMapKey.h>
#include <qcc/XmlElement.h>

#include "NameTable.h"
#include "PolicyDB.h"
#include "ServiceDB.h"
#include "PropertyDB.h"

/**
 * Configuration file database class.
 */
namespace ajn {

class ConfigDB : public ajn::NameListener {
  public:
    /** Typedef for list of daemon listen addresses. */
    typedef std::set<qcc::String> ListenList;

    /** Typedef for map of certain resource limits. */
    typedef std::map<qcc::StringMapKey, uint32_t> LimitMap;

    /** Typedef for map of SELinux settings. */
    typedef std::map<qcc::StringMapKey, qcc::String> SELinuxMap;

    /**
     * Get a pointer to the ConfigDB singleton object.
     *
     * @return  A pointer to the ConfigDB singleton object.
     */
    static ConfigDB* GetConfigDB();

    /**
     * Destructor.
     */
    ~ConfigDB();

    /**
     * Shutdown configDB
     */
    void Shutdown() { stopping = true; }

    /**
     * Get a reference to the PolicyDB managed object.
     *
     * @return  A reference to the PolicyDB managed object.
     */
    ajn::PolicyDB GetPolicyDB() const { return db->policyDB; }

    /**
     * Get a reference to the PolicyDB managed object.
     *
     * @return  A reference to the PolicyDB managed object.
     */
    ajn::ServiceDB GetServiceDB() const { return db->serviceDB; }

    /**
     * Get a reference to the PropertyDB managed object.
     *
     * @return  A reference to the PropertyDB managed object.
     */
    ajn::PropertyDB GetPropertyDB() const { return db->propertyDB; }

    /**
     * Name owner changed listener.  This is really just a proxy for calling
     * the name owner changed methods for the current PolicyDB and ServiceDB.
     *
     * @param alias     The well known name
     * @param oldOwner  Unique name of previous owner or NULL if no previous owner
     * @param newOwner  Unique name of new owner or NULL of no new owner
     */
    void NameOwnerChanged(const qcc::String& alias,
                          const qcc::String* oldOwner,
                          const qcc::String* newOwner);

    /**
     * Set the config file to read the configuration from.
     *
     * @param configFile    The pathname for the config file.
     */
    void SetConfigFile(qcc::String configFile) { this->configFile = configFile; }

    /**
     * Load the configuration from the file specified from ConfigDB::SetConfigFile().
     *
     * @return  true if config file loaded successfully.
     */
    bool LoadConfigFile();

    /**
     * Load the configuration from an arbitrary "source".
     *
     * @param src   A qcc::Source object that provides the config in XML format.
     *
     * @return  true if config file loaded successfully.
     */
    bool LoadSource(qcc::Source& src);

    /**
     * Indicates if the configuration has been loaded.
     *
     * @return  true if config is loaded.
     */
    bool ConfigLoaded() const { return db->loaded; }

    /**
     * Get the bus type specified in the config file.
     *
     * @return  The bus type.
     */
    const qcc::String GetType() const { return db->type; }

    /**
     * Get the username the daemon should run as.
     *
     * @return  The username.
     */
    const qcc::String GetUser() const { return db->user; }

    /**
     * Get the path to the file where the PID should be stored.
     *
     * @return  The file path for storing the PID.
     */
    const qcc::String GetPidfile() const { return db->pidfile; }

    /**
     * Get whether the daemon should fork off into its own autonomous process
     * or not.
     *
     * @return  true if daemon should fork.
     */
    bool GetFork() const { return db->fork; }

    /**
     * Get whether the daemon should keep its umask setting or not when forking.
     *
     * @return  true if daemon should keep its umask.
     */
    bool GetKeepUmask() const { return db->keepUmask; }

    /**
     * Get whether the daemon should send log messages to syslog.
     *
     * @return  true if daemon should send log mesages to syslog.
     */
    bool GetSyslog() const { return db->syslog; }

    /**
     * Get the list of listen address specifications.
     *
     * @return  List of listen address specifications.
     */
    const ListenList& GetListen() const
    {
        return db->listenList;
    }

    /**
     * Get the list of supported authentication mechanisms.
     *
     * @return  List of supported authentication mechanisms.
     */
    const qcc::String& GetAuth() const
    {
        return db->authList;
    }

    /**
     * Get the mapping of resource limits.
     *
     * @return  Mapping of resource limits.
     */
    const LimitMap& GetLimit() const
    {
        return db->limitMap;
    }

    /**
     * Get the value of a specific limit.
     *
     * @param key       The name of the limit to retrieve.
     * @param errVal    [OPTIONAL] Value to use if key not found.
     *
     * @return  Limit value.
     */
    const uint32_t GetLimit(qcc::String key, uint32_t errVal = 0) const
    {
        LimitMap::const_iterator it(db->limitMap.find(key));
        return (it == db->limitMap.end()) ? errVal : it->second;
    }

    /**
     * Get the value of a specific property.
     *
     * @param module    The name of the module the property resides in.
     * @param property  The name of the property.
     *
     * @return The property value if set, or the empty string.
     */
    qcc::String GetProperty(qcc::String module, qcc::String property) const
    {
        return db->propertyDB->Get(module, property);
    }

    /**
     * Get the mapping of SELinux specifications.
     *
     * @return  Mapping of SELinux specifications.
     */
    const SELinuxMap& GetSELinux() const
    {
        return db->selinuxMap;
    }

    /**
     * Get the directory containing .service files.
     *
     * @return  Directory containing .service files.
     */
    const qcc::String GetServiceDir() const { return db->serviceDir; }

    /**
     * Get the executable name of the serice launcher helper application.
     *
     * @return  The executable name of the serice launcher helper application.
     */
    const qcc::String GetServicehelper() const { return db->serviceHelper; }

  private:

    /** Internal Config database data structure. */
    struct DB {
        bool fork;      /**< Indicates whether daemon should fork or not. */
        bool syslog;    /**< Indicates whether daemon should send to syslog or stdout. */
        bool keepUmask; /**< Indicates whether daemon should keep its umask or not. */
        bool loaded;    /**< Indicates if the configuration has been loaded or not. */

        qcc::String type;       /**< Bus type. */
        qcc::String user;       /**< User identity daemon should run as. */
        qcc::String pidfile;    /**< File to store PID. */

        qcc::String authList;   /**< Authentication mechanism list. */
        ListenList listenList;  /**< Listen address list. */
        LimitMap limitMap;      /**< Resource limit map. */
        SELinuxMap selinuxMap;  /**< SELinux setting map. */

        qcc::String serviceDir;     /**< Directory containing .service files. */
        qcc::String serviceHelper;  /**< Service launcher helper application. */

        ajn::PolicyDB policyDB;    /**< The Policy database. */
        ajn::ServiceDB serviceDB;  /**< The Service database. */
        ajn::PropertyDB propertyDB;  /**< The Service database. */

        /**
         * Constructor.
         */
        DB() : fork(false), syslog(false), keepUmask(false), loaded(false) { }

        /**
         * Parse the XML source stream.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ParseSource(qcc::String fileName, qcc::Source& src);

        /**
         * Parse the specified XML file.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ParseFile(qcc::String fileName, bool ignore_missing = false);

        /**
         * Parse the <alljoyn/> attribute.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessAlljoyn(const qcc::String fileName, const qcc::XmlElement& policy);

        /**
         * Parse the <associate/> attribute.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessAssociate(const qcc::String fileName, const qcc::XmlElement& associate);

        /**
         * Parse the <auth/> attribute.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessAuth(const qcc::String fileName, const qcc::XmlElement& auth);

        /**
         * Parse the <busconfig/>
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessBusconfig(const qcc::String fileName, const qcc::XmlElement& busconfig);

        /**
         * Parse the <fork/> attribute.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessFork(const qcc::String fileName, const qcc::XmlElement& fork);

        /**
         * Parse the <include/> attribute.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessInclude(const qcc::String fileName, const qcc::XmlElement& include);

        /**
         * Parse the <includedir/> attribute.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessIncludedir(const qcc::String fileName, const qcc::XmlElement& includedir);

        /**
         * Parse the <keep_umask/> attribute.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessKeepUmask(const qcc::String fileName, const qcc::XmlElement& keepUmask);

        /**
         * Parse the <limit/> attribute.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessLimit(const qcc::String fileName, const qcc::XmlElement& limit);

        /**
         * Parse the <listen/> attribute.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessListen(const qcc::String fileName, const qcc::XmlElement& listen);

        /**
         * Parse the <pidfile/> attribute.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessPidfile(const qcc::String fileName, const qcc::XmlElement& pidfile);

        /**
         * Parse the <policy/> attribute.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessPolicy(const qcc::String fileName, const qcc::XmlElement& policy);

        /**
         * Parse the <selinux/> attribute.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessSELinux(const qcc::String fileName, const qcc::XmlElement& selinux);

        /**
         * Parse the <servicedir/> attribute.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessServicedir(const qcc::String fileName, const qcc::XmlElement& servicedir);

        /**
         * Parse the <servicehelper/> attribute.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessServicehelper(const qcc::String fileName, const qcc::XmlElement& servicehelper);

        /**
         * Parse the <standard_session_servicedirs/> attribute.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessStandardSessionServicedirs(const qcc::String fileName,
                                               const qcc::XmlElement& standardSessionServicedirs);

        /**
         * Parse the <standard_system_servicedirs/> attribute.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessStandardSystemServicedirs(const qcc::String fileName,
                                              const qcc::XmlElement& standardSystemServicedirs);

        /**
         * Parse the <syslog/> attribute.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessSyslog(const qcc::String fileName, const qcc::XmlElement& syslog);

        /**
         * Parse the <type/> attribute.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessType(const qcc::String fileName, const qcc::XmlElement& type);

        /**
         * Parse the <user/> attribute.
         *
         * @param fileName  Name of the configuration file.
         *
         * @return  true if parsing was successful.
         */
        bool ProcessUser(const qcc::String fileName, const qcc::XmlElement& user);
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

    qcc::String configFile;     /**< Config file. */
    DB* db;                     /**< The current config database storage object. */
    bool stopping;
};

}

#endif
