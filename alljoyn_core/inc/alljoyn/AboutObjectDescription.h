/**
 * @file
 * This contains the AboutObjectDescription class responsible
 */
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
#ifndef _ALLJOYN_ABOUTOBJECTDESCRIPTION_H
#define _ALLJOYN_ABOUTOBJECTDESCRIPTION_H

#include <alljoyn/MsgArg.h>
#include <alljoyn/Status.h>
#include <qcc/String.h>
#include <qcc/Mutex.h>

#include <set>
#include <map>

namespace ajn {
/**
 * AboutObjectDescription Class is responsible for holding path and interface
 * information for objects registered with the AllJoyn bus.
 *
 * This is intended to be used with the org.alljoy.About interface for announcing
 * supported interfaces.
 */
class AboutObjectDescription {
  public:
//    /**
//     * constructor
//     */
//    AboutObjectDescription();
//
//    /**
//     * Construct an AboutObjectDescription using a MsgArg
//     */
//    AboutObjectDescription(const MsgArg& arg);
//
//    /**
//     * destructor
//     */
//    ~AboutObjectDescription() {};

    /**
     * Fill in the ObjectDescription fields using a MsgArg
     *
     * The MsgArg must contain an array of type a(oas) The expected use of this
     * class is to fill in the ObjectDescription using a MsgArg obtain from the Announce
     * signal or the GetObjectDescription method from org.alljoyn.about interface.
     *
     * @param arg MsgArg contain AboutData dictionary
     *
     * @return ER_OK on success
     */
    QStatus Initialize(const MsgArg& arg);

    /**
     * Add an interface to the ObjectDescription. This can be called multiple
     * times.
     *
     * @param[in] path of the interface
     * @param[in] interfaceName name of the interface
     *
     * @return ER_OK is successful
     */
    QStatus Add(qcc::String const& path, qcc::String const& interfaceName);

    /**
     * AddObjectDescription adds objects Description for the AboutService announcement.
     * @param[in]  path of the interface.
     * @param[in]  interfaceNames
     * @return ER_OK if successful.
     */
    QStatus Add(qcc::String const& path, const char** interfaceNames, size_t numInterfaces);

    /**
     * Remove an interface from the object description for the AboutService
     * @param[in] path of the interface
     * @param[in] interfaceName name of the interface
     *
     * @return ER_OK is successful
     */
    QStatus Remove(qcc::String const& path, qcc::String const& interfaceName);

    /**
     * Remove object Description for the AboutService announcement.
     * @param[in]  path of the interface.
     * @param[in]  interfaceNames
     * @return ER_OK if successful.
     *
     */
    QStatus Remove(qcc::String const& path, const char** interfaceNames, size_t numInterfaces);

    /**
     * Returns true if the given path is found
     *
     * @param[in] path BusObject path
     *
     * @return true if the path is found
     */
    bool HasPath(qcc::String const& path);

    /**
     * Returns true if the given interface name is found in any path
     *
     * @param[in] interfaceName the name of the interface you are looking for
     *
     * @return true if the interface is found
     */
    bool HasInterface(qcc::String const& interfaceName);

    /**
     * Returns true if the given interface name is found at the given path
     * @param[in] path of the interface
     * @param[in] interfaceName the name of the interface you are looking for
     *
     * @return true if the interface is found at the given path
     */
    bool HasInterface(qcc::String const& path, qcc::String const& interfaceName);
    /**
     * @param[out] msgArg containing a signature a(oas)
     *                    an array of object paths and an array of interfaces
     *                    found on that object path
     *
     * @return ER_OK if successful
     */
    QStatus GetMsgArg(MsgArg* msgArg);

  private:

    /**
     * Mutex that protects the m_AnnounceObjectsMap
     */
    qcc::Mutex m_AnnounceObjectsMapLock;

    /**
     *  map that holds interfaces that will be announced
     */
    std::map<qcc::String, std::set<qcc::String> > m_AnnounceObjectsMap;
};
}

#endif
