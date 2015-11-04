Interface Description {#interface_description}
=======================
<!------------------------------------------------------------------------------
Copyright AllSeen Alliance

THIS DOCUMENT AND ALL INFORMATION CONTAINED HEREIN ARE PROVIDED ON AN AS-IS
BASIS WITHOUT WARRANTY OF ANY KIND.  
------------------------------------------------------------------------------->

@brief specifying and activating AllJoyn interfaces

@tableofcontents

Contents of an interface {#interface_description_contents}
=======================
An AllJoyn interface consists of three types of elements.

 - methods
 - signals
 - properties

Methods {#interface_description_contents_methods}
-----------------------
Methods in AllJoyn map naturally to a C++ member functions. Method is a member
of the interface and indicates that an action can be performed on an
implementation of the interface.

AllJoyn methods enable you to make remote method calls. In other words you are
able to call a member function that exist in on a different device or in an
different process. Unlike C++ member functions AllJoyn methods can have multiple
return values.

AllJoyn methods should return a reply to the caller. Even if a method has no
return values the method should to respond to the caller so that the caller
will know the method was successfully processed. The reply may indicate that
there was a failure.

It is possible to indicate that the caller does not require a response to the
method call by indicating that no reply is expected (`NO_REPLY_EXPECTED`).
If no reply is expected the method call may choose to omit the method reply.

Signals {#interface_description_contents_signals}
-----------------------
Unlike [Methods](@ref interface_description_contents_methods) AllJoyn signals do
not have a return value. AllJoyn signals also do not return any indication that
the signal was received.

AllJoyn provides two types of signals. Sessionless signals (i.e. store and
forward) signals and in-session signals.

Sessionless signals are sent to the routing node where signal is stored.
Sessionless signals can be received by other devices as long as the routing node
continues to run.

When sending a sessionless signal, the the sender (i.e. leaf node) will assign
that signal a serial number. The sessionless signal can be removed from the
routing node at any time by calling
`BusObject::CancelSessionlessMessage(<serial_number>)`.

The code used to send a sessionless signal is similar to the code need to send a
in-session signal all that is needed is to add the `ALLJOYN_FLAG_SESSIONLESS`
flag when emitting the signal.

Properties {#interface_description_contents_properties}
-----------------------
Properties in AllJoyn map naturally to C++ member variables.  The hold information
that can be read or written.

AllJoyn adds the ability to remotely implement get and set functionality for all
interface properties. AllJoyn properties are labelled with read, write, or
readwrite access. This access option can help ensure remote devices only have
the specified read write access.

Specifying an interface {#interface_description_specify}
=======================
Each interface must have an interface name. The name used for interface names
should start with the reversed DNS domain name of the interfaces author.  This
is similar to interface names in Java.

a few rules apply to interface names:

  - interface names have a maximum name length of 255 characters
  - interface names contain 2 or more elements separated by a period ('`.`') character.
  - Each element must contain at least one character.
  - Each element must contain only the ASCII characters `"[A-Z][a-z][0-9]_"`.
  - An element must not begin with a digit.
  - interface names must not begin with a '`.`' (period) character.

<!--
Enter the following EBNF grammar into bottlecaps.de/rr/ui to generate railroad
diagrams

interface_name ::= (element) ('.' element)+
element ::= ([A-Za-z] | '_') ([A-Za-z0-9] | '_')+

-->
![Interface Name](interface_name.png)
![Element](element.png)

An interface is made up of as many methods, signals, and properties as needed.

There is no limit on the number of interfaces that can be added to a BusAttachment.


Specify using xml notation {#interface_description_specify_xml}
-----------------------
The `BusAttachment` member function `CreateInterfacesFromXml` can be used to
initialize one or more interface descriptions using an XML string using the [DBus
introspection format](http://dbus.freedesktop.org/doc/dbus-specification.html#introspection-format).

The root tag of the XML can be a `<node>` or an `<interface>` tag. To
initialize more than one interface the interfaces need to be nested in a
`<node>` tag.

initializing and interface using xml notation has an advantage that its simple
to write the interface in human readable form.  The disadvantage is if the call
to `CreateInterfacesFromXml` fails to parse the xml.  Any interfaces
which were successfully parsed prior to the failure may be registered with the
bus. There is no way to know unless you try to add the interface again and get
the `ER_BUS_IFACE_ALREADY_EXISTS` status.

Example interface with top `<node>` element:

    <node name='/example/xml/interface'>
      <interface name='com.example.interface'>
        <method name='Echo'>
          <arg name='input_arg' type='s' direction='in' />
          <arg name='return_arg' type='s' direction='out' />
        </method>
        <signal name='Chirp'>
          <arg name='sound' type='s' />
        </signal>
        <property name='Volume' type='i' access='readwrite'/>
      </interface>
    </node>

Adding the XML node to code:

@snippet create_interface_from_xml_node.cpp xml_node_adding_namespace

@snippet create_interface_from_xml_node.cpp xml_node_adding_to_busattachment

The node can contain multiple interfaces. It is also possible to add just an
individual interface.

@snippet create_interface_from_xml_interface.cpp xml_interface_adding_namespace

@snippet create_interface_from_xml_interface.cpp xml_interface_adding_to_busattachment

Specify using code {#interface_description_specify_code}
-----------------------
In addition to specifying the interface using XML its also possible to fully
specify the interface using only code.

@snippet create_interface_using_code.cpp code_interface_adding_namespace

@snippet create_interface_using_code.cpp code_interface_adding_to_busAttachment

Interface Design Guidelines {#interface_design_guidelines}
=======================
The Interface Review Board, a sub-committee of the technical steering committee, has
created a set of guidelines for designers of AllJoyn interfaces. Designers of
interfaces that are not intended to be standardized are still encouraged to
reference the Interface Design Guidelines.

[Interface Design Guidelines v1.0](https://wiki.allseenalliance.org/irb/interface_design_guidelines_1.0)
