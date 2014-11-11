////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for any
//    purpose with or without fee is hereby granted, provided that the above
//    copyright notice and this permission notice appear in all copies.
//
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

#import "AJNObject.h"
#import "AJNType.h"
#import "AJNStatus.h"

/**
 * Class definition for a message arg.
 * This class deals with the message bus types and the operations on them
 *
 * MsgArg's are designed to be light-weight. A MsgArg will normally hold references to the data
 * (strings etc.) it wraps and will only copy that data if the MsgArg is assigned. For example no
 * additional memory is allocated for an #ALLJOYN_STRING that references an existing const char*.
 * If a MsgArg is assigned the destination receives a copy of the contents of the source. The
 * Stabilize() methods can also be called to explicitly force contents of the MsgArg to be copied.
 */
@interface AJNMessageArgument : AJNObject

/**
 * The type of this arg
 */
@property (nonatomic, readonly) AJNType type;

/**
 * Returns a string for the signature of this value
 *
 * @return The signature string for this MsgArg
 */
@property (nonatomic, readonly) NSString *signature;

/**
 * Returns an XML string representation of this type
 *
 * @return  The XML string
 */
@property (nonatomic, readonly) NSString *xml;

/**
 * Returns a string representation of the signature of an array of message args.
 *
 * @param arguments     A pointer to an array of message arg values
 *
 * @return The signature string for the message args.
 */
- (NSString *)signatureFromMessageArguments:(NSArray *)arguments;

/**
 * Returns an XML string representation for an array of message args.
 *
 * @param arguments     The message arg array.
 *
 * @return The XML string representation of the message args.
 */
- (NSString *)xmlFromMessageArguments:(NSArray *)arguments;

/**
 * Checks the signature of this arg.
 *
 * @param signature  The signature to check
 *
 * @return  true if this arg has the specified signature, otherwise returns false.
 */
- (BOOL)conformsToSignature:(NSString *)signature;

/**
 * Set value of a message arg from a signature and a list of values. Note that any values or
 * MsgArg pointers passed in must remain valid until this MsgArg is freed.
 *
 *  - 'a'  The array length followed by:
 *         - If the element type is a basic type a pointer to an array of values of that type.
 *         - If the element type is string a pointer to array of const char*,
 *         - If the element type is an @ref ALLJOYN_ARRAY "ARRAY", @ref ALLJOYN_STRUCT "STRUCT",
 *           @ref ALLJOYN_DICT_ENTRY "DICT_ENTRY" or @ref ALLJOYN_VARIANT "VARIANT" a pointer to an
 *           array of MsgArgs where each MsgArg has the signature specified by the element type.
 *         - If the element type is specified using the wildcard character '*', a pointer to
 *           an  array of MsgArgs. The array element type is determined from the type of the
 *           first MsgArg in the array, all the elements must have the same type.
 *         - If the element type is specified using a '$' character, a pointer to an array of
 *           qcc::String. This is a convenience case for initializing a string array, actual
 *           signature "as" from a qcc::String array.
 *  - 'b'  A bool value
 *  - 'd'  A double (64 bits)
 *  - 'g'  A pointer to a NUL terminated string (pointer must remain valid for lifetime of the MsgArg)
 *  - 'h'  A qcc::SocketFd
 *  - 'i'  An int (32 bits)
 *  - 'n'  An int (16 bits)
 *  - 'o'  A pointer to a NUL terminated string (pointer must remain valid for lifetime of the MsgArg)
 *  - 'q'  A uint (16 bits)
 *  - 's'  A pointer to a NUL terminated string (pointer must remain valid for lifetime of the MsgArg)
 *  - 't'  A uint (64 bits)
 *  - 'u'  A uint (32 bits)
 *  - 'v'  Not allowed, the actual type must be provided.
 *  - 'x'  An int (64 bits)
 *  - 'y'  A byte (8 bits)
 *
 *  - '(' and @c ')'  The list of values that appear between the parentheses using the notation above
 *  - '{' and @c '}'  A pair values using the notation above.
 *
 *  - '*'  A pointer to a MsgArg.
 *
 * Examples:
 *
 * An array of strings
 *
 *     char* fruits[3] =  { "apple", "banana", "orange" };
 *     MsgArg bowl;
 *     bowl.Set("as", 3, fruits);
 *
 * A struct with a uint and two string elements.
 *
 *     arg.Set("(uss)", 1024, "hello", "world"); @endcode
 *
 * An array of uint_16's
 *
 *     uint16_t aq[] = { 1, 2, 3, 5, 6, 7 };
 *     arg.Set("aq", sizeof(aq) / sizeof(uint16_t), aq);
 *
 * @param signature   The signature for MsgArg value
 * @param ...         One or more values to initialize the MsgArg.
 *
 * @return Returns ER_OK if the MsgArg was successfully set or an error status otherwise.
 */
- (QStatus)setValue:(NSString *)signature, ...;

/**
 * Matches a signature to the MsArg and if the signature matches unpacks the component values of a MsgArg. Note that the values
 * returned are references into the MsgArg itself so unless copied will become invalid if the MsgArg is freed or goes out of scope.
 * This function resolved through variants, so if the MsgArg is a variant that references a 32 bit integer is can be unpacked
 * directly into a 32 bit integer pointer.
 *
 *  - 'a'  A pointer to a length of type size_t that returns the number of elements in the array followed by:
 *         - If the element type is a scalar type a pointer to a pointer of the correct type for the values.
 *         - Otherwise a pointer to a pointer to a MsgArg.
 *
 *  - 'b'  A pointer to a bool
 *  - 'd'  A pointer to a double (64 bits)
 *  - 'g'  A pointer to a char*  (character string is valid for the lifetime of the MsgArg)
 *  - 'h'  A pointer to a qcc::SocketFd
 *  - 'i'  A pointer to a uint16_t
 *  - 'n'  A pointer to an int16_t
 *  - 'o'  A pointer to a char*  (character string is valid for the lifetime of the MsgArg)
 *  - 'q'  A pointer to a uint16_t
 *  - 's'  A pointer to a char*  (character string is valid for the lifetime of the MsgArg)
 *  - 't'  A pointer to a uint64_t
 *  - 'u'  A pointer to a uint32_t
 *  - 'v'  A pointer to a pointer to a MsgArg, matches to a variant but returns a pointer to
 *         the MsgArg of the underlying real type.
 *  - 'x'  A pointer to an int64_t
 *  - 'y'  A pointer to a uint8_t
 *
 *  - '(' and @c ')'  A list of pointers as required for each of the struct members.
 *  - '{' and @c '}'  Pointers as required for the key and value members.
 *
 *  - '*' A pointer to a pointer to a MsgArg. This matches any value type.
 *
 * Examples:
 *
 * A struct with and uint32 and two string elements.
 *
 *     struct {
 *         uint32_t i;
 *         char *hello;
 *         char *world;
 *     } myStruct;
 *     arg.Get("(uss)", &myStruct.i, &myStruct.hello, &myStruct.world);
 *
 * A variant where it is known that the value is a uint32, a string, or double. Note that the
 * variant is resolved away.
 *
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
 *
 * An array of dictionary entries where each entry has an integer key and variant. Find the
 * entries where the variant value is a string or a struct of 2 strings.
 *
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
 *
 * An array of uint_16's
 *
 *     uint16_t *vals;
 *     size_t numVals;
 *     arg.Get("aq", &numVals, &vals);
 *
 * @param signature   The signature for MsgArg value
 * @param ...         Pointers to return the unpacked values.
 *
 * @return  - ER_OK if the signature matched and MsgArg was successfully unpacked.
 *          - ER_BUS_SIGNATURE_MISMATCH if the signature did not match.
 *          - An error status otherwise
 */
- (QStatus)value:(NSString *)signature, ...;

/**
 * Clear the MsgArg setting the type to ALLJOYN_INVALID and freeing any memory allocated for the
 * AJNMessageArgument value.
 */
- (void)clear;

/**
 * Makes an AJNMessageArgument stable by completely copying the contents into locally
 * managed memory. After a MsgArg has been stabilized any values used to
 * initialize or set the message arg can be freed.
 */
- (void)stabilize;

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
- (void)setOwnershipFlags:(uint8_t) flags shouldApplyRecursively:(BOOL)deep;


@end
