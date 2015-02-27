/**
 * @file
 * This contains the AboutObjectDescription class responsible holding a list of
 * all BusObject interfaces and their paths.
 */
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
#ifndef _ALLJOYN_ABOUTOBJECTDESCRIPTION_H
#define _ALLJOYN_ABOUTOBJECTDESCRIPTION_H

#include <alljoyn/MsgArg.h>
#include <alljoyn/Status.h>
#include <qcc/String.h>

namespace ajn {
/**
 * AboutObjectDescription Class is a helper class for accessing the
 * ObjectDescription MsgArg that is sent as part of an announce signal.
 * It contains helper functions for quickly determining an About ObjectDescrption
 * contains a specified path or interface.
 */
class AboutObjectDescription {
  public:
    /**
     * constructor to create an empty AboutObjectDescrption.
     * use the CreateFromMsgArg method to fill in the AboutObjectDescription
     * class.
     */
    AboutObjectDescription();
    /**
     * Fill in the ObjectDescription fields using a MsgArg
     *
     * The MsgArg must contain an array of type a(oas) The expected use of this
     * class is to fill in the ObjectDescription using a MsgArg obtain from the
     * Announce signal or the GetObjectDescription method from org.alljoyn.About
     * interface.
     *
     * If the arg came from the org.alljoyn.About.Announce signal or the
     * org.alljoyn.AboutGetObjectDescrption method then it can be used to create
     * the AboutObjectDescription. If the arg came from any other source its best
     * to create an empty AboutObjectDescrption class and use the CreateFromMsgArg
     * class to access the MsgArg. Since it can be checked for errors while parsing
     * the MsgArg.
     *
     * @param arg MsgArg contain About ObjectDescription
     */
    AboutObjectDescription(const MsgArg& arg);

    /**
     * Copy constructor
     *
     * @param src The AboutObjectDescription instance to be copied
     */
    AboutObjectDescription(const AboutObjectDescription& src);

    /**
     * Assignment operator
     *
     * @param src The AboutObjectDescription instance to be assigned
     *
     * @return copy of the AboutObjectDescription src
     */
    AboutObjectDescription& operator=(const AboutObjectDescription& src);
    /**
     * Destructor
     */
    ~AboutObjectDescription();

    /**
     * Fill in the ObjectDescription fields using a MsgArg
     *
     * The MsgArg must contain an array of type a(oas) The expected use of this
     * class is to fill in the ObjectDescription using a MsgArg obtain from the
     * Announce signal or the GetObjectDescription method from org.alljoyn.about
     * interface.
     *
     * @param arg MsgArg contain AboutData dictionary
     *
     * @return ER_OK on success
     */
    QStatus CreateFromMsgArg(const MsgArg& arg);

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
    size_t GetInterfaces(const char* path, const char** interfaces, size_t numInterfaces) const;

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
     * @param[in]  iface the interface we want to get a list of paths for
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
    size_t GetInterfacePaths(const char* iface, const char** paths, size_t numPaths) const;

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
    bool HasPath(const char* path) const;

    /**
     * Returns true if the given interface name is found in any path
     *
     * @param[in] interfaceName the name of the interface you are looking for
     *
     * @return true if the interface is found
     */
    bool HasInterface(const char* interfaceName) const;

    /**
     * Returns true if the given interface name is found at the given path
     * @param[in] path of the interface
     * @param[in] interfaceName the name of the interface you are looking for
     *
     * @return true if the interface is found at the given path
     */
    bool HasInterface(const char* path, const char* interfaceName) const;

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
     * Add an interface to the ObjectDescription. This can be called multiple
     * times.
     *
     * @param[in] path of the interface
     * @param[in] interfaceName name of the interface
     *
     * @return ER_OK is successful
     */
    QStatus Add(const char* path, const char* interfaceName);

    /// @cond ALLJOYN_DEV
    /**
     * @internal
     * Class for internal state for AboutObjectDescription.
     */
    class Internal;

    Internal* aodInternal;    /**< Internal state information */
    /// @endcond ALLJOYN_DEV
};
}

#endif
