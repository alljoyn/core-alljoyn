/**
 * @file
 * This contains the AboutObjectDescription class responsible holding a list of
 * all BusObject interfaces and their paths.
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
     * @param[in]  interfaceNames an array of interface names to add to the AboutObjectDescription
     * @param[in]  numInterfaces the number of interfaces in the interfaceNames array
     * @return ER_OK if successful.
     */
    QStatus Add(qcc::String const& path, const char** interfaceNames, size_t numInterfaces);

    /**
     * Merge the contents of the supplied AboutObjectDescription into this
     * AboutObjectDescription the supplied AboutObjectDescription will not
     * be modified.
     *
     * If paths or interfaces are listed in both AboutObjectDescriptions
     * the resulting AboutObjectDescription will only list them once.
     *
     * @param[in] aboutObjectDescription the AboutObjectDescription that will be
     * merged with this AboutObjectDescription
     *
     * @return ER_OK on success
     */
    QStatus Merge(AboutObjectDescription& aboutObjectDescription);

    /**
     * Get a list of the paths that are added to this AboutObjectDescription.
     *
     * usage example
     * @code
     * size_t numPaths = aboutObjectDescription.GetPaths(NULL, 0);
     * const char** paths = new const char*[numPaths];
     * aboutObjectDescription.GetPaths(paths, numPaths);
     * @endcode
     *
     * @param[out] paths an char* array pointer
     * @param[in]  numPaths the size of the char* array
     *
     * @return
     *    The total number of paths found in the AboutObjectDescription.  If this
     *    number is larger than `numPaths` then only `numPaths` of paths will be
     *    returned in the `paths` array.
     *
     */
    size_t GetPaths(const char** paths, size_t numPaths) const;

    /**
     * Get a list of interfaces advertised at the given path that are part of
     * this AboutObjectDescription.
     *
     * usage example
     * @code
     * size_t numInterfaces = aboutObjectDescription.GetInterfaces(NULL, 0);
     * const char** interfaces = new const char*[numInterfaces];
     * aboutObjectDescription.GetInterfaces("/example", interfaces, numInterfaces);
     * @endcode
     *
     * @param[in]  path the path we want to get a list of interfaces for
     * @param[out] interfaces a char* array pointer
     * @param[in]  numInterfaces the size of the char* array
     *
     * @return
     *    The total number of interfaces found in the AboutObjectDescription for
     *    the specified path.  If this number is larger than `numInterfaces`
     *    then only `numInterfaces` of interfaces will be returned in the
     *    `interfaces` array.
     */
    size_t GetInterfaces(qcc::String const& path, const char** interfaces, size_t numInterfaces) const;

    /**
     * Get a list of the paths for a given interface. Its possible to have the
     * same interface listed under multiple paths.
     *
     * Usage example
     * @code
     * size_t numPaths = GetInterfacePaths("com.example.interface", NULL, 0);
     * const char** paths = new const char*[numPaths];
     * GetInterfacePaths("com.example.interface", paths, numPaths);
     * @endcode
     *
     * @param[in]  interface the interface we want to get a list of paths for
     * @param[out] paths a char* array pointer
     * @param[in]  numPaths the size of the char* array
     *
     * @return
     *    The total number of paths found in the AboutObjectDescription for
     *    the specified path.  If this number it larger than the `numPaths`
     *    then only `numPaths` of interfaces will be returned in the `paths`
     *    array
     *
     */
    size_t GetInterfacePaths(qcc::String const& interface, const char** paths, size_t numPaths) const;

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
     *
     * @param[in]  path of the interface.
     * @param[in]  interfaceNames an array of interfaces names to remove from the AboutObjectDescription
     * @param[in]  numInterfaces the number of interface names in the interfaceNames array
     *
     * @return ER_OK if successful.
     */
    QStatus Remove(qcc::String const& path, const char** interfaceNames, size_t numInterfaces);

    /**
     * Clear all the contents of this AboutObjectDescription
     *
     * @return ER_OK
     */
    void Clear();
    /**
     * Returns true if the given path is found
     *
     * @param[in] path BusObject path
     *
     * @return true if the path is found
     */
    bool HasPath(qcc::String const& path) const;

    /**
     * Returns true if the given interface name is found in any path
     *
     * @param[in] interfaceName the name of the interface you are looking for
     *
     * @return true if the interface is found
     */
    bool HasInterface(qcc::String const& interfaceName) const;

    /**
     * Returns true if the given interface name is found at the given path
     * @param[in] path of the interface
     * @param[in] interfaceName the name of the interface you are looking for
     *
     * @return true if the interface is found at the given path
     */
    bool HasInterface(qcc::String const& path, qcc::String const& interfaceName) const;
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
     *
     * this is marked a mutable so we can grab the lock to prevent the Objects
     * map being modified while its being read.
     */
    mutable qcc::Mutex m_AnnounceObjectsMapLock;

    /**
     *  map that holds interfaces that will be announced
     */
    std::map<qcc::String, std::set<qcc::String> > m_AnnounceObjectsMap;
};
}

#endif
