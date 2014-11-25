/**
 * @file
 * alljoyn_about_object_description holds a list of the bus object interfaces and their paths.
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
#ifndef _ALLJOYN_ABOUTOBJECTDESCRIPTION_C_H
#define _ALLJOYN_ABOUTOBJECTDESCRIPTION_C_H

#include <alljoyn_c/AjAPI.h>
#include <alljoyn_c/MsgArg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _alljoyn_about_object_description_handle* alljoyn_about_object_description;

/**
 * Allocate a new empty alljoyn_about_object_description object.
 * Use the alljoyn_about_object_description_createfrommsgarg method to fill in data.
 *
 * @return The allocated alljoyn_about_object_description.
 */
extern AJ_API alljoyn_about_object_description AJ_CALL alljoyn_about_object_description_create();

/*
 * Allocate a new alljoyn_about_data object filling in the fields of the About data
 * using an alljoyn_msgarg. The provided alljoyn_msgarg must contain a dictionary
 * with signature a{sv} with About data fields.
 *
 * If the passed in alljoyn_msgarg is an ill formed About data alljoyn_msgarg this call
 * will fail silently. If the alljoyn_msgarg does not come from About Announce signal
 * it is best to create an empty alljoyn_about_data object and use the
 * alljoyn_about_createfrommsgarg function to fill in the About data.

 * Allocate a new alljoyn_about_object_description object filling in the data
 * from an alljoyn_msgarg.
 *
 * The alljoyn_msgarg must contain an array of type a(oas) The expected use of this
 * object is to fill in the object description using an alljoyn_msgarg obtained from
 * the Announce signal or the GetObjectDescription method from org.alljoyn.About interface.
 *
 * If the arg came from the org.alljoyn.About.Announce signal or the
 * org.alljoyn.AboutGetObjectDescrption method then it can be used to create
 * the alljoyn_about_object_description. If the arg came from any other source
 * it is recommended to create first an empty alljoyn_about_object_description object
 *  and use the alljoyn_about_object_description_createfrommsgarg.
 *
 * @param arg alljoyn_msgarg contain About Object Description data
 *
 * @return The allocated alljoyn_about_object_description.
 */
extern AJ_API alljoyn_about_object_description AJ_CALL alljoyn_about_object_description_create_full(const alljoyn_msgarg arg);

/**
 * Fill in the ObjectDescription fields using an alljoyn_msgarg
 *
 * The alljoyn_msgarg must contain an array of type a(oas) The expected use
 * of this object is to fill in the ObjectDescription using a MsgArg obtain
 * from the Announce signal or the GetObjectDescription method from
 * the org.alljoyn.about interface.
 *
 * @param[in] description The alljoyn_about_object_description object this call is made for
 * @param[in] arg  alljoyn_msgarg contain About data dictionary
 *
 * @return ER_OK on success
 */
extern AJ_API QStatus AJ_CALL alljoyn_about_object_description_createfrommsgarg(alljoyn_about_object_description description,
                                                                                const alljoyn_msgarg arg);

/**
 * Free an alljoyn_about_object_description object.
 *
 * @param description The alljoyn_about_object_description to be freed.
 */
extern AJ_API void AJ_CALL alljoyn_about_object_description_destroy(alljoyn_about_object_description description);

/**
 * Get a list of the paths that are added to this AboutObjectDescription.
 *
 * @param[in]  description the alljoyn_about_object_description object this call is made for
 * @param[out] paths       array of paths
 * @param[in]  numPaths    the number of paths
 *
 * @return
 *    The total number of paths found in the AboutObjectDescription.  If this
 *    number is larger than `numPaths` then only `numPaths` of paths will be
 *    returned in the `paths` array.
 *
 */
extern AJ_API size_t AJ_CALL alljoyn_about_object_description_getpaths(alljoyn_about_object_description description,
                                                                       const char** paths,
                                                                       size_t numPaths);

/**
 * Get a list of interfaces advertised at the given path that are part of
 * this AboutObjectDescription.
 *
 * @param[in]  description   the alljoyn_about_object_description object this call is made for
 * @param[in]  path          the path we want to get a list of interfaces for
 * @param[out] interfaces    array of interface names
 * @param[in]  numInterfaces number of interface names
 *
 * @return
 *    The total number of interfaces found in the AboutObjectDescription for
 *    the specified path.  If this number is larger than `numInterfaces`
 *    then only `numInterfaces` of interfaces will be returned in the
 *    `interfaces` array.
 */
extern AJ_API size_t AJ_CALL alljoyn_about_object_description_getinterfaces(alljoyn_about_object_description description,
                                                                            const char* path,
                                                                            const char** interfaces,
                                                                            size_t numInterfaces);

/**
 * Get a list of the paths for a given interface. Its possible to have the
 * same interface listed under multiple paths.
 *
 * @param[in]  description   the alljoyn_about_object_description object this call is made for
 * @param[in]  interfaceName the interface we want to get a list of paths for
 * @param[out] paths         array of paths
 * @param[in]  numPaths      the number of paths
 *
 * @return
 *    The total number of paths found in the AboutObjectDescription for
 *    the specified path.  If this number its larger than the `numPaths`
 *    then only `numPaths` of interfaces will be returned in the `paths`
 *    array
 *
 */
extern AJ_API size_t AJ_CALL alljoyn_about_object_description_getinterfacepaths(alljoyn_about_object_description description,
                                                                                const char* interfaceName,
                                                                                const char** paths,
                                                                                size_t numPaths);

/**
 * Clear all the contents of this AboutObjectDescription
 *
 * @param[in] description The alljoyn_about_object_description object this call is made for
 *
 * @return ER_OK
 */
extern AJ_API void AJ_CALL alljoyn_about_object_description_clear(alljoyn_about_object_description description);

/**
 * Returns true if the given path is found
 *
 * @param[in] description    the alljoyn_about_object_description object this call is made for
 * @param[in] path BusObject path
 *
 * @return true if the path is found
 */
extern AJ_API bool AJ_CALL alljoyn_about_object_description_haspath(alljoyn_about_object_description description,
                                                                    const char* path);

/**
 * Returns true if the given interface name is found in any path
 *
 * @param[in] description   the alljoyn_about_object_description object this call is made for
 * @param[in] interfaceName the name of the interface you are looking for
 *
 * @return true if the interface is found
 */
extern AJ_API bool AJ_CALL alljoyn_about_object_description_hasinterface(alljoyn_about_object_description description,
                                                                         const char* interfaceName);

/**
 * Returns true if the given interface name is found at the given path
 *
 * @param[in] description   the alljoyn_about_object_description object this call is made for
 * @param[in] path          path name of the interface
 * @param[in] interfaceName the name of the interface you are looking for
 *
 * @return true if the interface is found at the given path
 */
extern AJ_API bool AJ_CALL alljoyn_about_object_description_hasinterfaceatpath(alljoyn_about_object_description description,
                                                                               const char* path,
                                                                               const char* interfaceName);

/**
 * @param[in] description the alljoyn_about_object_description object this call is made for
 * @param[out] msgArg     containing a signature a(oas) for an array of object paths and
 *                         an array of interfaces found on that object path
 *
 * @return ER_OK if successful
 */
extern AJ_API QStatus AJ_CALL alljoyn_about_object_description_getmsgarg(alljoyn_about_object_description description,
                                                                         alljoyn_msgarg* msgArg);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif //_ALLJOYN_ABOUTOBJECTDESCRIPTION_C_H
