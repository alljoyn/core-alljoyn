#ifndef _ALLJOYN_MSGARG_H
#define _ALLJOYN_MSGARG_H
/**
 * @file
 * This file defines a class for message bus data types and values
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

#ifndef __cplusplus
#error Only include MsgArg.h in C++ code.
#endif

#include <qcc/platform.h>
#include <qcc/String.h>
#include <stdarg.h>
#include <alljoyn/Status.h>

namespace ajn {

/**
 * Forward definitions
 */
class MsgArg;

/**
 * Enumeration of the various message arg types.
 * @remark Most of these map directly to the values used in the
 * DBus wire protocol but some are specific to the AllJoyn implementation.
 */
typedef enum {
    ALLJOYN_INVALID          =  0,     ///< AllJoyn INVALID typeId
    ALLJOYN_ARRAY            = 'a',    ///< AllJoyn array container type
    ALLJOYN_BOOLEAN          = 'b',    ///< AllJoyn boolean basic type, @c 0 is @c FALSE and @c 1 is @c TRUE - Everything else is invalid
    ALLJOYN_DOUBLE           = 'd',    ///< AllJoyn IEEE 754 double basic type
    ALLJOYN_DICT_ENTRY       = 'e',    ///< AllJoyn dictionary or map container type - an array of key-value pairs
    ALLJOYN_SIGNATURE        = 'g',    ///< AllJoyn signature basic type
    ALLJOYN_HANDLE           = 'h',    ///< AllJoyn socket handle basic type
    ALLJOYN_INT32            = 'i',    ///< AllJoyn 32-bit signed integer basic type
    ALLJOYN_INT16            = 'n',    ///< AllJoyn 16-bit signed integer basic type
    ALLJOYN_OBJECT_PATH      = 'o',    ///< AllJoyn Name of an AllJoyn object instance basic type
    ALLJOYN_UINT16           = 'q',    ///< AllJoyn 16-bit unsigned integer basic type
    ALLJOYN_STRUCT           = 'r',    ///< AllJoyn struct container type
    ALLJOYN_STRING           = 's',    ///< AllJoyn UTF-8 NULL terminated string basic type
    ALLJOYN_UINT64           = 't',    ///< AllJoyn 64-bit unsigned integer basic type
    ALLJOYN_UINT32           = 'u',    ///< AllJoyn 32-bit unsigned integer basic type
    ALLJOYN_VARIANT          = 'v',    ///< AllJoyn variant container type
    ALLJOYN_INT64            = 'x',    ///< AllJoyn 64-bit signed integer basic type
    ALLJOYN_BYTE             = 'y',    ///< AllJoyn 8-bit unsigned integer basic type

    ALLJOYN_STRUCT_OPEN      = '(', /**< Never actually used as a typeId: specified as ALLJOYN_STRUCT */
    ALLJOYN_STRUCT_CLOSE     = ')', /**< Never actually used as a typeId: specified as ALLJOYN_STRUCT */
    ALLJOYN_DICT_ENTRY_OPEN  = '{', /**< Never actually used as a typeId: specified as ALLJOYN_DICT_ENTRY */
    ALLJOYN_DICT_ENTRY_CLOSE = '}', /**< Never actually used as a typeId: specified as ALLJOYN_DICT_ENTRY */

    ALLJOYN_BOOLEAN_ARRAY    = ('b' << 8) | 'a',   ///< AllJoyn array of booleans
    ALLJOYN_DOUBLE_ARRAY     = ('d' << 8) | 'a',   ///< AllJoyn array of IEEE 754 doubles
    ALLJOYN_INT32_ARRAY      = ('i' << 8) | 'a',   ///< AllJoyn array of 32-bit signed integers
    ALLJOYN_INT16_ARRAY      = ('n' << 8) | 'a',   ///< AllJoyn array of 16-bit signed integers
    ALLJOYN_UINT16_ARRAY     = ('q' << 8) | 'a',   ///< AllJoyn array of 16-bit unsigned integers
    ALLJOYN_UINT64_ARRAY     = ('t' << 8) | 'a',   ///< AllJoyn array of 64-bit unsigned integers
    ALLJOYN_UINT32_ARRAY     = ('u' << 8) | 'a',   ///< AllJoyn array of 32-bit unsigned integers
    ALLJOYN_INT64_ARRAY      = ('x' << 8) | 'a',   ///< AllJoyn array of 64-bit signed integers
    ALLJOYN_BYTE_ARRAY       = ('y' << 8) | 'a',   ///< AllJoyn array of 8-bit unsigned integers

    ALLJOYN_WILDCARD         = '*'     ///< This never appears in a signature but is used for matching arbitrary message args

} AllJoynTypeId;


/**
 * Type for initializing an invalid MsgArg
 */
typedef struct {
    void* unused[3]; /**< Fields for zero-initializing an invalid MsgArg */
} AllJoynInvalid;

/**
 * Type for the various string types
 */
typedef struct {
    uint32_t len;              /**<  Length of AllJoynString */
    const char* str;           /**<  The actual string */
} AllJoynString;


/**
 * Type for a signature
 * The same as AllJoynString except the length is a single byte (thus signatures have a maximum length of 255).
 * The content must be a valid signature.
 */
typedef struct {
    uint8_t len;               /**< Length of AllJoyn signature   */
    const char* sig;           /**< The  signature   */
} AllJoynSignature;


/**
 * Type for an array
 */
class AllJoynArray {

    friend class MsgArg;
    friend class SignatureUtils;
    friend class _Message;

  public:

    /**
     * Set the array value. Note that arrays must be initialized using this function otherwise they
     * cannot be marshaled.
     *
     * @param elemSig     The signature for the array element type.
     * @param numElements The size of the array.
     * @param elements    Pointer to an array of MsgArgs for the elements. Must be NULL if numElements
     *                    is zero. This array must be dynamically allocated because it will be
     *                    deleted when the MsgArg destructor is called.
     *
     * @return ER_OK if the array was successfully initialized.
     */
    QStatus SetElements(const char* elemSig, size_t numElements, MsgArg* elements);

    /**
     * Accessor function to return the number of array elements
     *
     * @return  The number of array elements
     */
    size_t GetNumElements() const { return numElements; }

    /**
     * Accessor function to return the number of array elements
     *
     * @return  The array elements
     */
    const MsgArg* GetElements() const { return elements; }

    /**
     * Accessor function to return the array element signature.
     *
     * @return  The array element signature or an empty string.
     */
    const char* GetElemSig() const { return elemSig ? elemSig : ""; }

  private:
    char* elemSig;      /**< Element signature */
    size_t numElements; /**< Number of elements in the AllJoyn array   */
    MsgArg* elements;   /**< Pointer to array  */
};

/**
 * Type for a variant
 */
typedef struct {
    MsgArg* val;               /**< Pointer to value of type MsgArg*/
} AllJoynVariant;


/**
 * Type for a struct
 */
typedef struct {
    size_t numMembers;         /**< Number of members in structure */
    MsgArg* members;           /**< Pointer to members of the structure */
} AllJoynStruct;

/**
 * Type for a handle. A handle is an abstraction of a platform-specific socket or file descriptor.
 *
 * @note Handles associated with in a message received by the application will be closed when the
 * message destructor is called or when a method call is converted into a method reply. If the
 * application code needs to continue using the handle the handle must be duplicated by calling
 * qcc:SocketDup() or the appropriate platform-specific APIs.  Handles that are passed in when
 * creating a message to be sent are duplicated internally and can be closed by the caller after the
 * message has been created. Handles that have been duplicated using qcc::SocketDup() must be closed
 * by calling qcc::Close(). Handles duplicated by calling a native platform "dup" API must be closed
 * using the native "close" API.
 */
typedef struct {
    qcc::SocketFd fd;          /**< A platform-specific socket file descriptor */
} AllJoynHandle;

/**
 * Type for a dictionary entry
 */
typedef struct {
    MsgArg* key;               /**< Key in the dictionary entry */
    MsgArg* val;               /**< Value in the dictionary entry */
} AllJoynDictEntry;

/**
 * Type for arrays of scalars
 */
typedef struct {
    size_t numElements;     /**< Number of elements in the AllJoynScalar Array */
    union {
        const uint8_t* v_byte;
        const int16_t* v_int16;
        const uint16_t* v_uint16;
        const bool* v_bool;
        const uint32_t* v_uint32;
        const int32_t* v_int32;
        const int64_t* v_int64;
        const uint64_t* v_uint64;
        const double* v_double;
    };
} AllJoynScalarArray;

/**
 * Class definition for a message arg.
 * This class deals with the message bus types and the operations on them
 *
 * MsgArgs are designed to be light-weight. A MsgArg will normally hold references to the data
 * (strings etc.) it wraps and will only copy that data if the MsgArg is assigned. For example no
 * additional memory is allocated for an #ALLJOYN_STRING that references an existing const char*.
 * If a MsgArg is assigned the destination receives a copy of the contents of the source. The
 * Stabilize() methods can also be called to explicitly force contents of the MsgArg to be copied.
 */
class MsgArg {
    friend class _Message;
    friend class MsgArgUtils;

  public:

    /**
     * The flag value that indicates that the MsgArg owns the data it references so is responsible
     * for freeing that data in the destructor. This applies to any MsgArg that has a pointer to a
     * string or other data.
     * @return value that indicates the MsgArg owns the data.
     */
    static const uint8_t OwnsData = 1;

    /**
     * The flag value that indicates that the MsgArg owns the nested MsgArgs it references so is responsible
     * for freeing those MsgArgs in the destructor. This applies to MsgArgs of type #ALLJOYN_ARRAY,
     * #ALLJOYN_STRUCT, #ALLJOYN_DICT_ENTRY, and #ALLJOYN_VARIANT.
     * @return value that indicates the MsgArg owns the nested MsgArgs it references.
     */
    static const uint8_t OwnsArgs = 2;

    /**
     * The type of this arg
     */
    AllJoynTypeId typeId;

    /**
     * Union of the various argument values
     */
    union {
        uint8_t v_byte;
        int16_t v_int16;
        uint16_t v_uint16;
        bool v_bool;
        uint32_t v_uint32;
        int32_t v_int32;
        int64_t v_int64;
        uint64_t v_uint64;
        double v_double;
        AllJoynString v_string;
        AllJoynString v_objPath;
        AllJoynSignature v_signature;
        AllJoynHandle v_handle;
        AllJoynArray v_array;
        AllJoynStruct v_struct;
        AllJoynDictEntry v_dictEntry;
        AllJoynVariant v_variant;
        AllJoynScalarArray v_scalarArray;
        AllJoynInvalid v_invalid;
    };

    /**
     * Returns a string for the signature of this value
     *
     * @return The signature string for this MsgArg
     */
    qcc::String Signature() const { return Signature(this, 1); }

    /**
     * Returns a string representation of the signature of an array of message args.
     *
     * @param values     A pointer to an array of message arg values
     * @param numValues  Length of the array
     *
     * @return The signature string for the message args.
     */
    static qcc::String AJ_CALL Signature(const MsgArg* values, size_t numValues);

    /**
     * Returns an XML string representation of this type
     *
     * @param indent  Number of spaces to indent the generated xml
     *
     * @return  The XML string
     */
    qcc::String ToString(size_t indent = 0) const;

    /**
     * Returns an XML string representation for an array of message args.
     *
     * @param args     The message arg array.
     * @param numArgs  The size of the message arg array.
     * @param indent   Number of spaces to indent the generated xml
     *
     * @return The XML string representation of the message args.
     */
    static qcc::String AJ_CALL ToString(const MsgArg* args, size_t numArgs, size_t indent = 0);

    /**
     * Checks the signature of this arg.
     *
     * @param signature  The signature to check
     *
     * @return  true if this arg has the specified signature, otherwise returns false.
     */
    bool HasSignature(const char* signature) const;

    /**
     * Assignment operator
     *
     * @param other  The source MsgArg for the assignment
     *
     * @return  The assigned MsgArg
     */
    MsgArg& operator=(const MsgArg& other) {
        if (this != &other) {
            Clone(*this, other);
        }
        return *this;
    }

    /**
     * Copy constructor
     *
     * @param other  The source MsgArg for the copy
     */
    MsgArg(const MsgArg& other) : typeId(ALLJOYN_INVALID) { Clone(*this, other); }

    /**
     * Destructor
     */
    virtual ~MsgArg() { Clear(); }

    /**
     * Constructor
     *
     * @param typeId  The type for the MsgArg
     */
    MsgArg(AllJoynTypeId typeId) : typeId(typeId), flags(0) { v_invalid.unused[0] = v_invalid.unused[1] = v_invalid.unused[2] = NULL; }

    /**
     * Constructor to build a message arg. If the constructor fails for any reason the type will be
     * set to #ALLJOYN_INVALID. See the description of the #Set() method for information about the
     * signature and parameters. For initializing complex values it is recommended to use the
     * default constructor and the #Set() method so the success of setting the value can be
     * explicitly checked.
     *
     * @param signature   The signature for MsgArg value.
     * @param ...         One or more values to initialize the MsgArg.
     */
    MsgArg(const char* signature, ...);

    /**
     * Set value of a message arg from a signature and a list of values. Note that any values or
     * MsgArg pointers passed in must remain valid until this MsgArg is freed.
     *
     *  - @c 'a'  The array length followed by:
     *            - If the element type is a basic type a pointer to an array of values of that type.
     *            - If the element type is string a pointer to array of const char*,
     *            - If the element type is an @ref ALLJOYN_ARRAY "ARRAY", @ref ALLJOYN_STRUCT "STRUCT",
     *              @ref ALLJOYN_DICT_ENTRY "DICT_ENTRY" or @ref ALLJOYN_VARIANT "VARIANT" a pointer to an
     *              array of MsgArgs where each MsgArg has the signature specified by the element type.
     *            - If the element type is specified using the wildcard character '*', a pointer to
     *              an  array of MsgArgs. The array element type is determined from the type of the
     *              first MsgArg in the array, all the elements must have the same type.
     *            - If the element type is specified using a '$' character, a pointer to an array of
     *              qcc::String. This is a convenience case for initializing a string array, actual
     *              signature "as" from a qcc::String array.
     *  - @c 'b'  A bool value
     *  - @c 'd'  A double (64 bits)
     *  - @c 'g'  A pointer to a NUL terminated string (pointer must remain valid for lifetime of the MsgArg)
     *  - @c 'h'  A qcc::SocketFd
     *  - @c 'i'  An int (32 bits)
     *  - @c 'n'  An int (16 bits)
     *  - @c 'o'  A pointer to a NUL terminated string (pointer must remain valid for lifetime of the MsgArg)
     *  - @c 'q'  A uint (16 bits)
     *  - @c 's'  A pointer to a NUL terminated string (pointer must remain valid for lifetime of the MsgArg)
     *  - @c 't'  A uint (64 bits)
     *  - @c 'u'  A uint (32 bits)
     *  - @c 'v'  Not allowed, the actual type must be provided.
     *  - @c 'x'  An int (64 bits)
     *  - @c 'y'  A byte (8 bits)
     *
     *  - @c '(' and @c ')'  The list of values that appear between the parentheses using the notation above
     *  - @c '{' and @c '}'  A pair values using the notation above.
     *
     *  - @c '*'  A pointer to a MsgArg.
     *
     * Examples:
     *
     * An array of strings
     *
     *     @code
     *     char* fruits[3] =  { "apple", "banana", "orange" };
     *     MsgArg bowl;
     *     bowl.Set("as", 3, fruits);
     *     @endcode
     *
     * A struct with a uint and two string elements.
     *
     *     @code arg.Set("(uss)", 1024, "hello", "world"); @endcode
     *
     * An array of 3 dictionary entries where each entry has an integer key and string value.
     *
     *     @code
     *     MsgArg dict[3];
     *     dict[0].Set("{is}", 1, "red");
     *     dict[1].Set("{is}", 2, "green");
     *     dict[2].Set("{is}", 3, "blue");
     *     arg.Set("a{is}", 3, dict);
     *     @endcode
     *
     * An array of uint_16's
     *
     *     @code
     *     uint16_t aq[] = { 1, 2, 3, 5, 6, 7 };
     *     arg.Set("aq", sizeof(aq) / sizeof(uint16_t), aq);
     *     @endcode
     *
     * @param signature   The signature for MsgArg value
     * @param ...         One or more values to initialize the MsgArg.
     *
     * @return
     *      - #ER_OK if the MsgArg was successfully set
     *      - An error status otherwise
     */
    QStatus Set(const char* signature, ...);

    /**
     * Set an array of MsgArgs by applying the Set() method to each MsgArg in turn.
     *
     * @param args     An array of MsgArgs to set.
     * @param numArgs  [in,out] On input the size of the args array. On output the number of MsgArgs
     *                 that were set. There must be at least enough MsgArgs to completely
     *                 initialize the signature.
     *
     * @param signature   The signature for MsgArg values
     * @param ...         One or more values to initialize the MsgArg list.
     *
     * @return
     *       - #ER_OK if the MsgArgs were successfully set.
     *       - #ER_BUS_TRUNCATED if the signature was longer than expected.
     *       - Other error status codes indicating a failure.
     */
    static QStatus AJ_CALL Set(MsgArg* args, size_t& numArgs, const char* signature, ...);

    /**
     * Matches a signature to the MsArg and if the signature matches unpacks the component values of a MsgArg. Note that the values
     * returned are references into the MsgArg itself so unless copied will become invalid if the MsgArg is freed or goes out of scope.
     * This function resolved through variants, so if the MsgArg is a variant that references a 32 bit integer is can be unpacked
     * directly into a 32 bit integer pointer.
     *
     *  - @c 'a'  A pointer to a length of type size_t that returns the number of elements in the array followed by:
     *            - If the element type is a scalar type a pointer to a pointer of the correct type for the values.
     *            - Otherwise a pointer to a pointer to a MsgArg.
     *
     *  - @c 'b'  A pointer to a bool
     *  - @c 'd'  A pointer to a double (64 bits)
     *  - @c 'g'  A pointer to a char*  (character string is valid for the lifetime of the MsgArg)
     *  - @c 'h'  A pointer to a qcc::SocketFd
     *  - @c 'i'  A pointer to an int32_t
     *  - @c 'n'  A pointer to an int16_t
     *  - @c 'o'  A pointer to a char*  (character string is valid for the lifetime of the MsgArg)
     *  - @c 'q'  A pointer to a uint16_t
     *  - @c 's'  A pointer to a char*  (character string is valid for the lifetime of the MsgArg)
     *  - @c 't'  A pointer to a uint64_t
     *  - @c 'u'  A pointer to a uint32_t
     *  - @c 'v'  A pointer to a pointer to a MsgArg, matches to a variant but returns a pointer to
     *            the MsgArg of the underlying real type.
     *  - @c 'x'  A pointer to an int64_t
     *  - @c 'y'  A pointer to a uint8_t
     *
     *  - @c '(' and @c ')'  A list of pointers as required for each of the struct members.
     *  - @c '{' and @c '}'  Pointers as required for the key and value members.
     *
     *  - @c '*' A pointer to a pointer to a MsgArg. This matches any value type.
     *
     * Examples:
     *
     * A struct with and uint32 and two string elements.
     *
     *     @code
     *     struct {
     *         uint32_t i;
     *         char *hello;
     *         char *world;
     *     } myStruct;
     *     arg.Get("(uss)", &myStruct.i, &myStruct.hello, &myStruct.world);
     *     @endcode
     *
     * A variant where it is known that the value is a uint32, a string, or double. Note that the
     * variant is resolved away.
     *
     *     @code
     *     uint32_t i;
     *     double d;
     *     char *str;
     *     QStatus status = arg.Get("i", &i);
     *     if (status == ER_BUS_SIGNATURE_MISMATCH) {
     *         status = arg.Get("s", &str);
     *         if (status == ER_BUS_SIGNATURE_MISMATCH) {
     *             status = arg.Get("d", &d);
     *         }
     *     }
     *     @endcode
     *
     * An array of dictionary entries where each entry has an integer key and variant. Find the
     * entries where the variant value is a string or a struct of 2 strings.
     *
     *     @code
     *     MsgArg *entries;
     *     size_t num;
     *     arg.Get("a{iv}", &num, &entries);
     *     for (size_t i = 0; i > num; ++i) {
     *         char *str1;
     *         char *str2;
     *         uint32_t key;
     *         status = entries[i].Get("{is}", &key, &str1);
     *         if (status == ER_BUS_SIGNATURE_MISMATCH) {
     *             status = entries[i].Get("{i(ss)}", &key, &str1, &str2);
     *         }
     *     }
     *     @endcode
     *
     * An array of uint_16's
     *
     *     @code
     *     uint16_t *vals;
     *     size_t numVals;
     *     arg.Get("aq", &numVals, &vals);
     *     @endcode
     *
     * @param signature   The signature for MsgArg value
     * @param ...         Pointers to return the unpacked values.
     *
     * @return
     *      - #ER_OK if the signature matched and MsgArg was successfully unpacked.
     *      - #ER_BUS_SIGNATURE_MISMATCH if the signature did not match.
     *      - An error status otherwise
     */
    QStatus Get(const char* signature, ...) const;

    /**
     * Unpack an array of MsgArgs by applying the Get() method to each MsgArg in turn.
     *
     * @param args       An array of MsgArgs to unpack.
     * @param numArgs    The size of the MsgArgs array.
     * @param signature  The signature to match against the MsgArg values
     * @param ...         Pointers to return references to the unpacked values.
     *
     * @return
     *      - #ER_OK if the MsgArgs were successfully set.
     *      - #ER_BUS_SIGNATURE_MISMATCH if the signature did not match.
     *      - Other error status codes indicating a failure.
     */
    static QStatus AJ_CALL Get(const MsgArg* args, size_t numArgs, const char* signature, ...);

    /**
     * Helper function for accessing dictionary elements. The MsgArg must be an array of dictionary
     * elements. The second parameter is the key value, this is expressed according to the rules for
     * MsgArg::Set so is either a scalar, a pointer to a string, or for 64 bit values a pointer to
     * the value. This value is matched against the dictionary array to locate the matching element.
     * The third and subsequent parameters are unpacked according to the rules of MsgArg::Get.
     *
     * For example, where the key is a string and the values are structs:
     *
     *     @code
     *     uint8_t age;
     *     uint32_t height;
     *     const char* address;
     *     QStatus status = arg.GetElement("{s(yus)}", "fred", &age, &height,  &address);
     *     @endcode
     *
     * This function is particularly useful for extracting specific properties from the array of property
     * values returned by ProxyBusObject::GetAllProperties.
     *
     * @param elemSig  The expected signature for the dictionary element, e.g. "{su}"
     * @param ...      Pointers to return unpacked key values.
     *
     * @return
     *      - #ER_OK if the dictionary signature matched and MsgArg was successfully unpacked.
     *      - #ER_BUS_NOT_A_DICTIONARY if this method is called on a MsgArg that is not a dictionary.
     *      - #ER_BUS_SIGNATURE_MISMATCH if the signature did not match.
     *      - #ER_BUS_ELEMENT_NOT_FOUND if the key was not found in the dictionary.
     *      - An error status otherwise
     */
    QStatus GetElement(const char* elemSig, ...) const;

    /**
     * Equality operator.
     *
     * @param other  The other MsgArg to compare.
     *
     * @return  Returns true if the two message args have the same signatures and values.
     */
    bool operator==(const MsgArg& other);

    /**
     * Inequality operator.
     *
     * @param other  The other MsgArg to compare.
     *
     * @return  Returns true if the two message args do not have the same signatures and values.
     */
    bool operator!=(const MsgArg& other) { return !(*this == other); }

    /**
     * Clear the MsgArg setting the type to ALLJOYN_INVALID and freeing any memory allocated for the
     * MsgArg value.
     */
    void Clear();

    /**
     * Makes a MsgArg stable by completely copying the contents into locally
     * managed memory. After a MsgArg has been stabilized any values used to
     * initialize or set the message arg can be freed.
     */
    void Stabilize();

    /**
     * This method sets the ownership flags on this MsgArg, and optionally all
     * MsgArgs subordinate to this MsgArg. By setting the ownership flags the
     * caller can transfer responsibility for freeing nested data referenced
     * by this MsgArg to the MsgArg's destructor. The #OwnsArgs flag is
     * particularly useful for managing complex data structures such as arrays
     * of structs, nested structs, and variants where the inner MsgArgs are
     * dynamically allocated. The #OwnsData flag is useful for freeing
     * dynamically allocated strings, byte arrays, etc,.
     *
     * @param flags  A logical or of the applicable ownership flags (OwnsArgs and OwnsData).
     * @param deep   If true recursively sets the ownership flags on all MsgArgs owned by this MsgArg.
     */
    void SetOwnershipFlags(uint8_t flags, bool deep = false) { this->flags |= (flags & (OwnsData | OwnsArgs)); if (deep) { SetOwnershipDeep(); } }

    /**
     * Default constructor - arg instances start out invalid
     */
    MsgArg() : typeId(ALLJOYN_INVALID), flags(0) { v_invalid.unused[0] = v_invalid.unused[1] = v_invalid.unused[2] = NULL; }

  protected:
    /**
     * Build up a MsgArg from a variable set of parameters.
     *
     * @param signature   The signature for MsgArg values
     * @param sigLen      Length of the specified signature
     * @param arg         MsgArg to initialize
     * @param maxArgs     Maximum number of arguments to parse in parameter list, argp
     * @param argp        List of parameters constructed from var args
     * @param count       Out parameter that contains the number of actual parsed var args
     *
     * @return
     *      - #ER_OK if the MsgArg was successfully created.
     *      - #ER_BUS_SIGNATURE_MISMATCH if the signature did not match.
     *      - Other error status codes indicating a failure.
     */
    static QStatus VBuildArgs(const char*& signature, size_t sigLen, MsgArg* arg, size_t maxArgs, va_list* argp, size_t* count = NULL);

    /**
     * Parse a MsgArg into a variable set of receiving parameters.
     *
     * @param signature   The signature for MsgArg values
     * @param sigLen      Length of the specified signature
     * @param argList     MsgArg to parse
     * @param numArgs     Maximum number of arguments to parse in parameter list, argp
     * @param argp        List of parameters to receive values constructed from var args
     *
     * @return
     *      - #ER_OK if the MsgArg was successfully parsed.
     *      - #ER_BUS_SIGNATURE_MISMATCH if the signature did not match.
     *      - Other error status codes indicating a failure.
     */
    static QStatus VParseArgs(const char*& signature, size_t sigLen, const MsgArg* argList, size_t numArgs, va_list* argp);

  private:

    /**
     * flags indicating owner ship
     *
     * @see OwnsData
     * @see OwnsArg
     */
    uint8_t flags;

    /**
     * Recursively sets the ownership flags on the entire MsgArg tree.
     * @see SetOwnershipFlags
     */
    void SetOwnershipDeep();

    /**
     * Clone one MsgArg into Another MsgArg
     *
     * On 32bit Windows the export convention for Scons and Visual Studio builds is __stdcall
     * while for Visual Studio alone is __cdecl. This requires the explicit __stdcall (via
     * the AJ_CALL macro) applied to the public C++ API methods. This method is not public,
     * but it is invoked from two public methods which are inline (the copy constructor
     * and the assignment operator of this class) thus it also needs the decoration.
     *
     * @see operator=
     * @see MsgArg(const MsgArg&)
     *
     * @param dest the MsgArg that will hold the clone
     * @param src the MsgArg that will be cloned
     *
     */
    static void AJ_CALL Clone(MsgArg& dest, const MsgArg& src);

    /**
     * Used to help build an alljoyn array
     *
     * AllJoyn arrays are any MsgArgs with the letter `a` in the signature
     *
     * Note that `arry` is not a typo - Some editors tag `array` as a reserved word.
     *
     * @param arry the  MsgArg that will hold the array
     * @param elemSig   The signature for MsgArg array element
     * @param argp      List of parameters to receive values constructed from var args
     *
     * @see Set
     * @see VBuildArgs
     *
     *
     * @return
     *      - #ER_OK if the MsgArg was successfully created.
     *      - #ER_BUS_BAD_SIGNATURE invalid value in the array signature
     *      - #ER_BUS_BAD_VALUE array element does not have an expected value
     *      - Other error status codes indicating a failure.
     */
    static QStatus BuildArray(MsgArg* arry, const qcc::String& elemSig, va_list* argp);

    /**
     * Used to help parse an alljoyn array
     *
     * AllJoyn arrays are any MsgArgs with the letter `a` in the signature
     *
     * Note that `arry` is not a typo - Some editors tag `array` as a reserved word.
     *
     * @see Get
     * @see VParseArgs
     *
     * @param arry the   MsgArg that will hold the array
     * @param elemSig    The signature for MsgArg array element
     * @param elemSigLen Length of the array element signature
     * @param argp       List of parameters to receive values constructed from var args
     *
     * @return
     *      - #ER_OK if the MsgArg was successfully created.
     *      - #ER_BUS_BAD_SIGNATURE invalid value in the array signature
     *      - #ER_BUS_BAD_VALUE array element does not have an expected value
     *      - #ER_INVALID_ADDRESS the argp value is invalid
     *      - Other error status codes indicating a failure
     */
    static QStatus ParseArray(const MsgArg* arry, const char* elemSig, size_t elemSigLen, va_list* argp);
};

}

#endif
