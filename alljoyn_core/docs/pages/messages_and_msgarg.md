Messages and Message Args {#messags_and_msgargs}
=========================

@brief Guide for using AllJoyn Messages and MsgArgs.

@tableofcontents

<!------------------------------------------------------------------------------
Copyright AllSeen Alliance.

THIS DOCUMENT AND ALL INFORMATION CONTAINED HEREIN ARE PROVIDED ON AN AS-IS
BASIS WITHOUT WARRANTY OF ANY KIND.
------------------------------------------------------------------------------->

Message {#section1}
==================

A message is a unit of communication via the AllJoyn protocol.  A message can
send and receive any number of values.

An AllJoyn message contains a header and body.  The body of the message is user
defined by an array of one or more `MsgArg`s.  The message header contains
information about the message and its contents.

  - endianness of the contained data
  - the message type
  - the protocol version
  - a serial identifier
  - an array of header fields

Typically you will not have to concern your self with the contents of the message
header.  Most values are filled in by AllJoyn.

There are 5 Message Types {#message_types}
------------------------------------------

Conventional Name | Decimal value| Description
------------------|:------------:|------------
INVALID           |0             |This is an invalid type
METHOD\_CALL      |1             |Method call
METHOD\_RETURN    |2             |Method reply with return data
ERROR             |3             |Error reply. If the first argument exists and is a string it is an error message
SIGNAL            |4             |Signal emission

###Method Calls {#method_calls}

In AllJoyn a method call is a remote method call.  The method call invokes a
method on a remote object of a peer.  Remote objects that implement a method
that can be invoked and typically return a METHOD_RETURN or an ERROR. If the
caller has specified `NO_REPLY_EXPECTED` then there is no need to respond
to the method call.


Header Fields {#header_fields}
------------------------------

Header fields are passed in as an array at the end of a header.  Not all header
fields are required in all circumstances.

Once again most header fields are implicitly handled by AllJoyn.  Header fields
that need information provided by you are done using AllJoyn methods and will
not need to be modified directly.

Conventional Name | Decimal Code     | Type         | Required In           | Description
------------------|:----------------:|--------------|-----------------------|------------
INVALID           | 0                | N/A          | not allowed           | Not a valid field name (error if it appears in a message)
PATH              | 1                | OBJECT\_PATH | METHOD_CALL, SIGNAL   | The object to send a call to, or the object a signal is emitted from. The special path /org/freedesktop/DBus/Local is reserved; implementations should not send messages with this path, and the reference implementation of the bus daemon will disconnect any application that attempts to do so.
INTERFACE         | 2                | STRING       | SIGNAL                | The interface to invoke a method call on, or that a signal is emitted from. Optional for method calls, required for signals. The special interface org.freedesktop.DBus.Local is reserved; implementations should not send messages with this interface, and the reference implementation of the bus daemon will disconnect any application that attempts to do so.
MEMBER            | 3                | STRING       | METHOD\_CALL, SIGNAL  | The member, either the method name or signal name.
ERROR_NAME        | 4                | STRING       | ERROR                 | The name of the error that occurred, for errors
REPLY\_SERIAL     | 5                | UINT32       | ERROR, METHOD\_RETURN | The serial number of the message this message is a reply to. (The serial number is the second UINT32 in the header.)
DESTINATION       | 6                | STRING       | optional              | The name of the connection this message is intended for. Only used in combination with the message bus, see the section called “Message Bus Specification”.
SENDER            | 7                | STRING       | optional              | Unique name of the sending connection. The message bus fills in this field so it is reliable; the field is only meaningful in combination with the message bus.
SIGNATURE         | 8                | SIGNATURE    | optional              | The signature of the message body. If omitted, it is assumed to be the empty signature "" (i.e. the body must be 0-length).
UNIX\_FDS         | 9                | UINT32       | optional              | The number of Unix file descriptors that accompany the message. If omitted, it is assumed that no Unix file descriptors accompany the message. The actual file descriptors need to be transferred via platform specific mechanism out-of-band. They must be sent at the same time as part of the message itself. They may not be sent before the first byte of the message itself is transferred or after the last byte of the message itself.


Type System {#type_system}
---------------------------


AllJoyn uses the same type signatures that are used by the D-Bus wire protocol.
The type signature is made up of _type codes_ .  Type code is an ASCII character
that represents a standard data type.

The following table summarizes the AllJoyn types.

Conventional Name|Code                                              |Description
-----------------|--------------------------------------------------|----------------------------------------------------------
INVALID          |0 (ASCII NUL)                                     |Not a valid type code, used to terminate signatures
BYTE             |121 (ASCII 'y')                                   |8-bit unsigned integer
BOOLEAN          |98 (ASCII 'b')                                    |Boolean value, 0 is `FALSE` and 1 is `TRUE`. Everything else is invalid.
INT16            |110 (ASCII 'n')                                   |16-bit signed integer
UINT16           |113 (ASCII 'q')                                   |16-bit unsigned integer
INT32            |105 (ASCII 'i')                                   |32-bit signed integer
UINT32           |117 (ASCII 'u')                                   |32-bit unsigned integer
INT64            |120 (ASCII 'x')                                   |64-bit signed integer
UINT64           |116 (ASCII 't')                                   |64-bit unsigned integer
DOUBLE           |100 (ASCII 'd')                                   |IEEE 754 double
STRING           |115 (ASCII 's')                                   |UTF-8 string (_must_ be valid UTF-8). Must be nul terminated and contain no other nul bytes.
OBJECT_PATH      |111 (ASCII 'o')                                   |Name of an object instance
SIGNATURE        |103 (ASCII 'g')                                   |A type signature
ARRAY            |97 (ASCII 'a')                                    |Array
STRUCT           |114 (ASCII 'r'), 40 (ASCII '('), 41 (ASCII ')')   |Struct
VARIANT          |118 (ASCII 'v')                                   |Variant type (the type of the value is part of the value itself)
DICT_ENTRY       |101 (ASCII 'e'), 123 (ASCII '{'), 125 (ASCII '}') |Entry in a dict or map (array of key-value pairs)

Four of the types are _container_ types. `STRUCT`, `ARRAY`, `VARIANT`,and
`DICT_ENTRY`.  All other types are a common basic data type.

Message argument or MsgArg {#msg_argument_or_msgarg}
===========

Message arguments are used to hold and manage bus types (see AllJoyn type codes
above).  Each time a call is made using an AllJoyn message the parameters will
be passed in using a Message Arg or an array of Messsage Args. This section will
demonstrate how to construct Message Args of all AllJoyn basic types and some
examples of how to build Message Args for container types.

This section shows many code samples. Since doing any work with AllJoyn will
require use of Message Args, its good to have an understanding on how to build
any message type needed.

MsgArg for Basic AllJoyn types {#basic_types}
-----------------------------------

Below is some code showing the letter that corresponds to each data type.  The
code below shows fixed width types.

  @snippet usingmsgarg.cpp msgarg_basic_types

Three types take a `char*` string.  The string, object path, and signature.
Object path and signature have additional processing to verify the string is
of the right type.  The object path must be a Linux style path starting with a
'/' character.  The object path cannot end in a '/' character unless it is
the only character in the path.  The signature must be a valid AllJoyn signature.

The value of a new `MsgArg` can be set by the constructor as follows:

  @snippet usingmsgarg.cpp create_basic_types

If the constructor fails for any reason the type for the message arg will be set
to `ALLJOYN_INVALID`.  For basic types we don't expect a failure however we can
use the `Set` method if we want.

  @snippet usingmsgarg.cpp set_basic_types

For the strings (`"s"`), object paths (`"o"`), and signatures (`"g"`) we only
pass in a char pointer of a NUL terminated string into the MsgArg.  The pointer
**must** remain valid for the lifetime of the MsgArg.
MsgArg arg;

  @snippet usingmsgarg.cpp str_pointer_not_stabilized

If you want to create a `MsgArg` but don't want to maintain a separate pointer
for its contents a the `Stabilize` method can be used to copy all the contents
that the `MsgArg` is referencing in to the memory managed by the `MsgArg` its
self.

  @snippet usingmsgarg.cpp str_pointer_stabilized

**Note**, MsgArg.Stabilize works for any data type being pointed to by the
`MsgArg` not just strings.  When constructing nested container types the use of
temporary `MsgArg`s may require the use of `MsgArg.Stabilize`.  
See [Nested Containers](@ref nested_containers) section.

Not only do we need to know how to put information into a `MsgArg` we need to
know how to pull information out of a `MsgArg`.  `MsgArg`s also contain a `Get`
method for obtaining values from the `MsgArg`.

  @snippet usingmsgarg.cpp get_basic_types

The `MsgArg`s `Get()` and `Set()` methods can return an error if it is unable to read
or write the contents of the message arg.  For this reason it is good practice
to check the return value when using these methods to ensure the call was
successful.  Using `MsgArg.Set` or `MsgArg.Get` for basic types is unlikely to
return an error. The most common error would be when using the wrong signature
for the data.

  @snippet usingmsgarg.cpp basic_get_set_with_error_checking


MsgArgs for AllJoyn Container types {#container_types}
------------------------------------------------------

AllJoyn defines four types of container types: ARRAY, STRUCT, DICT, and VARIANT.

###Arrays {#array_container_types}

Creating a `MsgArg` that holds the values for an array is similar to creating
the `MsgArg` for a basic type.  Use the `MsgArg.Set()` method however this time
pass in the size of the array and an array pointer.

  @snippet usingmsgarg.cpp array_container_types

The code above does not show UINT16, UINT32, and UINT64.  Making an array for
those types is identical to what has been shown just the signature and data
would differ.

AllJoyn actually has two types of arrays.  One is called an array (or non scalar
array) the other is a scalar array.  It is important to understand the
differences between these two arrays when trying to retrieve information from an
array. Most of the basic types are placed in a scalar array the exception being
string, object path, and signature types.  A scalar array is entirely made up of
fixed width types. With strings, object paths, and signatures the width in bytes
of each entry is not known till it assigned. The string "foo" is smaller than
the string "Hello world".  Scalar arrays can be retrieved from a `MsgArg`
directly into the appropriate container. Non-scalar arrays, however, must be
retrieved though another `MsgArg`.  When retrieving an array of values that are
non-scalar they are unpacked as an array of `MsgArgs`.  An array that contains
container types will be non-scalar and will also be unpacked as an array of
`MsgArg`s.  (See [Nested Containers](@ref nested_containers) below)

Retrieving values from scalar arrays:

  @snippet usingmsgarg.cpp get_arrays_of_scalars

The process of retrieving an array of type Boolean 'b', UINT16 'q', UINT32, 'u',
and UINT64 't' is identical except the actual type would be changed. The first
parameter is the array signature, the second parameter will return the array
size, third will point to the array its self.

Retrieving values from non-scalar arrays:

  @snippet usingmsgarg.cpp get_arrays_of_non_scalars

###Structs {#structs}

Making a struct in AllJoyn is similar to filling in a basic type.  Except that
the entire struct is filled all at once.

  @snippet usingmsgarg.cpp set_get_structs

An Alljoyn struct can contain any number of values as long as the values can be
represented using a valid type signature.  Those values include all of the basic
Alljoyn types as well as other container types.  See
[Nested Containers](@ref nested_containers) below for examples on creating structs that
contain container types.

###Dictionaries {#dictionaries}

In AllJoyn a dictionary is an array of key/value pairs.  When sending a
dictionary of key/value pairs you must construct an array.  This is required
even if there is a single key/value pair. To create the key value pair you
create an intermediate `MsgArg` that will hold the a single key/value pair
(or a dictionary entry) that will then be put into an AllJoyn array.

  @snippet usingmsgarg.cpp set_get_dictionary

`MsgArg` contains an additional helper function specifically designed for
working with AllJoyn dictionary entries.  The `GetElement` method can be used to read an
individual value from the dictionary given its key. Using The `monthDictionary`
created in the sample code above we could get the name of the 10th month as follows.

  @snippet usingmsgarg.cpp dictionary_getelement

###Variant {#variant}
Variant is an interesting container because it is a container that can be any
other single valid AllJoyn type. This allows for the reuse of the same container
or creating an array that the actual data type in the array could be any data
type.

One of the common uses of the variant data type is for sending an array of
key/value pairs where the values are not all the same type.  More will be shown
in section [Nested Containers](@ref nested_containers).

One problem encountered using a variant type is how do you know what the
contents of the variant is when you receive a message that contains a variant.
You must check the type of the variant against any type that you may be
expecting.

  @snippet usingmsgarg.cpp get_set_variant

Nested Containers {#nested_containers}
--------------------------------------

So far all of the samples of container types have contained only a basic AllJoyn
data types.  What if we want an array of arrays or a struct that contains
another struct.  Each of the containers may contain another container. All of
the following signatures are valid containers. "(si(sii))", "aai", "a{sv}", or
"a(sii)"

When using nested containers there are limitations.

 - The maximum struct depth is 32
 - The maximum array depth is 32.
 - The maximum combined depth of a container with both arrays and structs is 64.

Note, when counting container depth dictionaries (i.e. signature "a{ss}") will
count as an array and a struct. In practice it is unlikely to encounter this
nesting limitation. If a container has a variant the contents of the variant are
also counted as part of the nesting depth.

###Nested structs {#nested_structs}

Nested structs may be the simplest of the multiple container types to handle.
To create a nested struct all you need to do is pass in each value as they show
up in the structure. Same goes when reading from a struct.

  @snippet usingmsgarg.cpp nested_structs

###Structs that contain arrays {#structs_of_arrays}

  @snippet usingmsgarg.cpp nested_structs_of_arrays

###Nested Arrays or arrays of arrays {#nested_arrays}

Arrays of arrays in AllJoyn is essentially an array of `MsgArg`s where each of
those `MsgArg`s are also arrays.  When a multi dimensional array is constructed
for AllJoyn only the amount of memory needed to store the elements will be used.
This means that multi-dimensional arrays in AllJoyn are ragged or jagged arrays.
`MsgArg` does not give any accessor methods for reading a specific element of
one of the arrays. Each array needs to be individually extracted.

  @snippet usingmsgarg.cpp nested_arrays

###Array of structs {#array_of_structs}

  @snippet usingmsgarg.cpp array_of_structs

###Variant that is a container type {#variant_of_container}

  @snippet usingmsgarg.cpp variant_of_struct

###Strategy for using nested containers {#nested_container_strategy}

Nested containers can have almost an infinite variety of possible combinations.
It is almost certain that a combination not covered here will be needed by
someone someday.  Remember containers are built from the inside to the outside.
Make the `MsgArg` for the inner most container then place that `MsgArg` inside
the next up container.

When building the containers the contents are not stored inside the `MsgArg`
itself.  A pointer to the container contents is what is stored. When you have
a nested array the outer most array is only pointing to the inner array elements.
The inner array elements must be accessible for the entire lifetime of the outer
array element.  If the inner array element is going to be deleted ether manually
or automatically (maybe the inner element has left scope) and you still want to
access the elements using the outer array then `MsgArg.Stabilize` should be used
before the inner array elements are deleted. See
[MsgArg for Basic AllJoyn types](@ref basic_types) under the section on `char*`
strings for an example using `MsgArg.Stabilize`.
