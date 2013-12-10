/**
 * @file
 * AllJoyn-Daemon serivce launcher file database class
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
#ifndef _SERVICEDB_H
#define _SERVICEDB_H

#include <qcc/platform.h>

#include <list>
#include <map>

#include <qcc/ManagedObj.h>
#include <qcc/String.h>
#include <qcc/StringMapKey.h>
#include <qcc/Timer.h>
#include <qcc/Util.h>

#include <alljoyn/Status.h>

#include "Bus.h"
#include "NameTable.h"


namespace ajn {

/**
 * Listener design pattern for informing when a specified service has started
 * via ServiceDB::BusStartService().
 */
class ServiceStartListener {
    friend class _ServiceDB;

  public:
    /**
     * Virtual destructor for derivable class.
     */
    virtual ~ServiceStartListener() { }

  private:
    /**
     * Method to be implemented to receive notification of the start of the
     * service (or failure thereof.)
     *
     * @param serviceName   Name of the service that was started.
     * @param result        ER_OK if service started successfully.  ER_TIMEOUT
     *                      if service failed to start within
     *                      service_start_timeout. Anything else indicates
     *                      service start failed.
     */
    virtual void ServiceStarted(const qcc::String& serviceName, QStatus result) = 0;
};


namespace servicedb {
/**
 * Information for launching a service.
 */
struct ServiceInfo {
    qcc::String exec;   /**< executable name */
    qcc::ExecArgs args; /**< arguments passed to the executable */
    qcc::String user;   /**< user to run as */

    /** List of service start listeners waiting for the service to start. */
    std::list<ServiceStartListener*> waiting;
    mutable qcc::Mutex lock;    /**< Mutex to protect the waiting list. */
};

/** ServiceMap typedef */
typedef std::map<qcc::StringMapKey, ServiceInfo> ServiceMap;

/** const_iterator typedef */
typedef ServiceMap::const_iterator const_iterator;
}


class _ServiceDB;

/**
 * Managed object wrapper for service database class.
 */
typedef qcc::ManagedObj<_ServiceDB> ServiceDB;


/**
 * This maintains the list of launchable services.
 */
class _ServiceDB : public ajn::NameListener, public qcc::AlarmListener {
  public:
    typedef servicedb::ServiceInfo ServiceInfo;

    /** ServiceMap typedef */
    typedef servicedb::ServiceMap ServiceMap;

    /** const_iterator typedef */
    typedef ServiceMap::const_iterator const_iterator;

    /**
     * Constructor.
     */
    _ServiceDB() { }

    /**
     * Parse the .service files in the specified directory.  (Called by
     * ConfigDB to fill the ServiceDB based on the config files.)
     *
     * @param dir   Directory containing .service files to parse.
     *
     * @return  true if directory was processed successfully.
     */
    bool ParseServiceFiles(qcc::String dir);

    /**
     * Indicates if a given service is in the list of launchable services.
     *
     * @param serviceName   Name of the service to check.
     *
     * @return  true if the service name is in the list.
     */
    bool IsStartable(const char* serviceName) const { return serviceMap.find(serviceName) != serviceMap.end(); }

    /**
     * Start the specified service.  If the service is in the process of being
     * started then the ServiceStartListener object will be added to the list
     * of listeners waiting fo the service to start.
     *
     * @param serviceName   Name of the service to be started.
     * @param cb            [OPTIONAL] Pointer to the ServiceStartListener
     *                      object waiting for the service to start.
     * @param bus           [OPTIONAL] Pointer to the bus object iff the code is
     *                      called from the AllJoyn daemon.
     *
     * @return  ER_OK only indicates that processing up to the underlying fork
     *          succeeded.  It does not indicate the status of actually starting
     *          the service.  That is only provided to the ServiceStartListener.
     */
    QStatus BusStartService(const char* serviceName, ServiceStartListener* cb = NULL, const Bus* bus = NULL);

    /**
     * Get an iterator to the first startable service.
     *
     * @return  Iterator to the first startable service.
     */
    const_iterator begin() const { return serviceMap.begin(); }

    /**
     * Get an iterator to one past the last startable service.
     *
     * @return  Iterator to one past the last startable service.
     */
    const_iterator end() const { return serviceMap.end(); }

    /**
     * Get the number of startable services.
     *
     * @return  The number of startable services.
     */
    size_t size() const { return serviceMap.size(); }

    /**
     * Called by the NameTable to indicate when a well known name changes
     * ownership.  Used by this class to determine if a service started that
     * has ServiceStartListeners waiting.
     *
     * @param alias     Well-known bus name now owned by newOwner.
     * @param oldOwner  Unique name of old owner of alias or NULL if none existed.
     * @param newOwner  Unique name of new owner of alias or NULL if none (now) exists.
     */
    void NameOwnerChanged(const qcc::String& alias,
                          const qcc::String* oldOwner,
                          const qcc::String* newOwner);

  private:
    /**
     * Parses the executable and arguments to that executable from the Exec
     * line of a .service file.
     *
     * @param execLine  The line to be parsed.
     */
    static void ParseExecLine(const qcc::String& execLine, std::list<qcc::String>& execTokens);

    /**
     * Service start timeout alarm handler.
     *
     * @param alarm  The alarm object for the timeout that expired.
     */
    void AlarmTriggered(const qcc::Alarm& alarm, QStatus reason);


    ServiceMap serviceMap;  /**< Service map storage. */
    qcc::Timer timer;       /**< Timer object for the service start timeouts. */
};

}

#endif
