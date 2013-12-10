/**
 * @file
 * This file defines types for statically describing a message bus interface
 */

/******************************************************************************
 * Copyright (c) 2012-2013, AllSeen Alliance. All rights reserved.
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

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace AllJoynUnity
{
	public partial class AllJoyn
	{
		/**
		 * @class InterfaceDescription
		 * Class for describing message bus interfaces. %InterfaceDescription objects describe the methods,
		 * signals and properties of a BusObject or ProxyBusObject.
		 *
		 * Calling ProxyBusObject.AddInterface(const char*) adds the AllJoyn interface described by an
		 * %InterfaceDescription to a ProxyBusObject instance. After an  %InterfaceDescription has been
		 * added, the methods described in the interface can be called. Similarly calling
		 * BusObject.AddInterface adds the interface and its methods, properties, and signal to a
		 * BusObject. After an interface has been added method handlers for the methods described in the
		 * interface can be added by calling BusObject.AddMethodHandler or BusObject.AddMethodHandlers.
		 *
		 * An %InterfaceDescription can be constructed piecemeal by calling InterfaceDescription.AddMethod,
		 * InterfaceDescription.AddMember(), and InterfaceDescription.AddProperty(). Alternatively,
		 * calling ProxyBusObject.ParseXml will create the %InterfaceDescription instances for that proxy
		 * object directly from an XML string. Calling ProxyBusObject.IntrospectRemoteObject or
		 * ProxyBusObject.IntrospectRemoteObjectAsync also creates the %InterfaceDescription
		 * instances from XML but in this case the XML is obtained by making a remote Introspect method
		 * call on a bus object.
		 */
		public class InterfaceDescription
		{
			[Flags]
			/** Annotation flags */
			public enum AnnotationFlags : byte
			{
				Default = 0, /**< Default annotate flag */
				NoReply = 1, /**< No reply annotate flag */
				Deprecated = 2 /**< Deprecated annotate flag */
			}

			[Flags]
			/** Access type */
			public enum AccessFlags : byte
			{
				Read = 1, /**< Read Access type */
				Write = 2, /**< Write Access type */
				ReadWrite = 3 /**< Read-Write Access type */
			}

			/**
			 * The interface security policy can be inherit, required, or off. If security is
			 * required on an interface, methods on that interface can only be called by an authenticated peer
			 * and signals emitted from that interfaces can only be received by an authenticated peer. If
			 * security is not specified for an interface the interface inherits the security of the objects
			 * that implement it.  If security is not applicable (off) to an interface, authentication is never
			 * required even when implemented by a secure object. For example, security does not apply to
			 * the Introspection interface otherwise secure objects would not be introspectable.
			 */
			public enum SecurityPolicy : int
			{
				Inherit = 0, /**< Inherit the security of the object that implements the interface */
				Required = 1,/**< Security is required for an interface */
				Off = 2      /**< Security does not apply to this interface */
			}
			/** @cond ALLJOYN_DEV */
			/**
			 * @internal
			 * Constructor for InterfaceDescription.
			 * This constructor should only be used internally
			 * This will create a C# InterfaceDescription using a pointer to an already existing
			 * native C InterfaceDescription
			 *
			 * @param nativeMsgArg a pointer to a native C MsgArg
			 */
			public InterfaceDescription(IntPtr interfaceDescription)
			{
				_interfaceDescription = interfaceDescription;
			}
			/** @endcond */

			#region Equality
			/**
			 * Equality operation.
			 *
			 * @param one	InterfaceDescription to be compared to
			 * @param other	InterfaceDescription to be compared against
			 *
			 * @return true if InterfaceDescriptions are equal
			 */
			public static bool operator ==(InterfaceDescription one, InterfaceDescription other)
			{
			
				if((object)one == null && (object)other == null) return true;
				else if((object)one == null || (object)other == null) return false;
				return alljoyn_interfacedescription_eql(one.UnmanagedPtr, other.UnmanagedPtr);
			}

			/**
			 * Not Equality operation.
			 *
			 * @param one	InterfaceDescription to be compared to
			 * @param other	InterfaceDescription to be compared against
			 *
			 * @return true if InterfaceDescriptions are not equal
			 */public static bool operator !=(InterfaceDescription one, InterfaceDescription other)
			{
				return !(one == other);
			}

			/**
			 * Equality operation.
			 *
			 * @param o	InterfaceDescription to be compared against
			 *
			 * @return true if InterfaceDescriptions are equal
			 */
			public override bool Equals(object o) 
			{
				try
				{
					return (this == (InterfaceDescription)o);
				}
				catch
				{
					return false;
				}
			}

			/**
			 * Returns the raw pointer of the interfaceDescription.
			 *
			 * @return the raw pointer of the interfaceDescription
			 */
			public override int GetHashCode()
			{
				Member[] members = GetMembers();
				int hash = Name.GetHashCode();
				foreach (Member m in members)
				{
					hash = (hash * 13) + m.GetType().GetHashCode();
					hash = (hash * 13) + m.Name.GetHashCode();
					hash = (hash * 13) + m.Signature.GetHashCode();
					hash = (hash * 13) + m.ReturnSignature.GetHashCode();
					hash = (hash * 13) + m.ArgNames.GetHashCode();
				}

				Property[] properties = GetProperties();
				foreach (Property p in properties)
				{
					hash = (hash * 13) + p.Name.GetHashCode();
					hash = (hash * 13) + p.Signature.GetHashCode();
					hash = (hash * 13) + p.Access.GetHashCode();
				}
				return hash;
			}
			#endregion

			#region Properties
			/**
			 * Check for existence of any properties
			 *
			 * @return  true if interface has any properties.
			 */
			public bool HasProperties
			{
				get
				{
					return (alljoyn_interfacedescription_hasproperties(_interfaceDescription) == 1 ? true : false);
				}
			}

			/**
			 * Returns the name of the interface
			 *
			 * @return the interface name.
			 */
			public string Name
			{
				get
				{
					return Marshal.PtrToStringAnsi(alljoyn_interfacedescription_getname(_interfaceDescription));
				}
			}

			/**
			 * Indicates if this interface is secure. Secure interfaces require end-to-end authentication.
			 * The arguments for methods calls made to secure interfaces and signals emitted by secure
			 * interfaces are encrypted.
			 * @return true if the interface is secure.
			 */
			public bool IsSecure
			{
				get
				{
					return alljoyn_interfacedescription_issecure(_interfaceDescription);
				}
			}

			/**
			 * Get the security policy that applies to this interface.
			 *
			 * @return Returns the security policy for this interface.
			 */
			public SecurityPolicy GetSecurityPolicy
			{
				get
				{
					return alljoyn_interfacedescription_getsecuritypolicy(_interfaceDescription);
				}
			}

			#endregion
			/**
			 * Add an annotation to the interface.
			 *
			 * @param name       Name of annotation.
			 * @param value      Value of the annotation
			 * @return
			 *      - QStatus.OK if successful.
			 *      - QStatus.BUS_PROPERTY_ALREADY_EXISTS if the property can not be added
			 *                                        because it already exists.
			 */
			public QStatus AddAnnotation(string name, string value)
			{
				return alljoyn_interfacedescription_addannotation(_interfaceDescription, name, value);
			}

			/**
			 * Get the value of an annotation
			 *
			 * @param name       Name of annotation.
			 * @param value      Returned value of the annotation
			 * @return
			 *      - true if annotation found.
			 *      - false if annotation not found
			 */
			public bool GetAnnotation(string name, ref string value)
			{

				UIntPtr value_size = new UIntPtr(); ;
				alljoyn_interfacedescription_getannotation(_interfaceDescription, name, IntPtr.Zero, ref value_size);

				byte[] buffer = new byte[(int)value_size];
				GCHandle gch = GCHandle.Alloc(buffer, GCHandleType.Pinned);
				bool ret = alljoyn_interfacedescription_getannotation(_interfaceDescription, name, gch.AddrOfPinnedObject(), ref value_size);
				gch.Free();
				if (ret)
				{
					// The returned buffer will contain a nul character an so we must remove the last character.
					value = System.Text.ASCIIEncoding.ASCII.GetString(buffer, 0, (Int32)value_size - 1);
				}
				else
				{
					value = "";
				}

				return ret;
			}

			/**
			 * Get a Dictionary containing the names and values of all annotations
			 * associated with this interface
			 * 
			 * @return Dictionary containing the names and values of all annotations associated with this interface
			 */
			public Dictionary<string, string> GetAnnotations()
			{
				Dictionary<string, string> ret = new Dictionary<string, string>();
				UIntPtr annotation_size = alljoyn_interfacedescription_getannotationscount(_interfaceDescription);
				for (uint i = 0; i < (uint)annotation_size; ++i)
				{
					UIntPtr name_size = new UIntPtr();
					UIntPtr value_size = new UIntPtr();

					alljoyn_interfacedescription_getannotationatindex(_interfaceDescription, (UIntPtr)i, 
						IntPtr.Zero, ref name_size, 
						IntPtr.Zero, ref value_size);

					byte[] name_buffer = new byte[(int)name_size];
					byte[] value_buffer = new byte[(int)value_size];

					GCHandle name_gch = GCHandle.Alloc(name_buffer, GCHandleType.Pinned);
					GCHandle value_gch = GCHandle.Alloc(value_buffer, GCHandleType.Pinned);

					alljoyn_interfacedescription_getannotationatindex(_interfaceDescription, (UIntPtr)i,
						name_gch.AddrOfPinnedObject(), ref name_size,
						value_gch.AddrOfPinnedObject(), ref value_size);
					name_gch.Free();
					value_gch.Free();
					ret.Add(System.Text.ASCIIEncoding.ASCII.GetString(name_buffer, 0, (Int32)name_size - 1),
						System.Text.ASCIIEncoding.ASCII.GetString(value_buffer, 0, (Int32)value_size - 1));
				}
				return ret;
			}

			/**
			 * Add a member to the interface.
			 *
			 * @param type        Message type.
			 * @param name        Name of member.
			 * @param inputSignature    Signature of input parameters or NULL for none.
			 * @param outputSignature      Signature of output parameters or NULL for none.
			 * @param argNames    Comma separated list of input and then output arg names used in interface XML.
			 *
			 * @return
			 *      - QStatus.OK if successful
			 *      - QStatus.BUS_MEMBER_ALREADY_EXISTS if member already exists
			 */
			public QStatus AddMember(Message.Type type, string name, string inputSignature,
				string outputSignature, string argNames)
			{
			
				return alljoyn_interfacedescription_addmember(_interfaceDescription,
					(int)type, name, inputSignature, outputSignature, argNames, (byte)AnnotationFlags.Default);
			}

			/**
			 * Add an annotation to a member
			 *
			 * @param member      Name of member.
			 * @param name        Name of annotation
			 * @param value       Value for the annotation
			 *
			 * @return
			 *      - QStatus.OK if successful
			 *      - QStatus.BUS_MEMBER_ALREADY_EXISTS if member already exists
			 */
			public QStatus AddMemberAnnotation(string member, string name, string value)
			{
				return alljoyn_interfacedescription_addmemberannotation(_interfaceDescription, member, name, value);
			}

			/**
			 * Get annotation to an existing member (signal or method).
			 *
			 * @param member     Name of member
			 * @param name       Name of annotation
			 * @param value      Output value for the annotation
			 *
			 * @return
			 *      - true if found
			 *      - false if property not found
			 */
			public bool GetMemberAnnotation(string member, string name, ref string value)
			{
				UIntPtr value_size = new UIntPtr(); ;
				alljoyn_interfacedescription_getmemberannotation(_interfaceDescription, member, name, IntPtr.Zero, ref value_size);

				byte[] buffer = new byte[(int)value_size];
				GCHandle gch = GCHandle.Alloc(buffer, GCHandleType.Pinned);
				bool ret = alljoyn_interfacedescription_getmemberannotation(_interfaceDescription, member, name, gch.AddrOfPinnedObject(), ref value_size);
				gch.Free();
				if (ret)
				{
					// The returned buffer will contain a nul character an so we must remove the last character.
					value = System.Text.ASCIIEncoding.ASCII.GetString(buffer, 0, (Int32)value_size - 1);
				}
				else
				{
					value = "";
				}

				return ret;
			}

			/**
			 * Add a member to the interface.
			 *
			 * If Default annotation flag is used no annotations will be added to the method.
			 *
			 * Using the Depricated annotation flag will add the org.freedesktop.DBus.Depricated=true
			 * annotation to the member. This annotation is an indication that the member has been
			 * depricated.
			 *
			 * Using the NoReply annotation flag will add the org.freedesktop.DBus.Method.NoReply=true
			 * annotation to the member. If this annotation is set don't expect a reply to the method
			 * call. This annotation only applies to method type members.
			 *
			 * The Depricated and NoReply annotation flags can be ORed together to add both annotations.
			 *
			 * To add other annotations use the InterfaceDescription.AddMemberAnnotation method.
			 *
			 * @param type        Message type.
			 * @param name        Name of member.
			 * @param inputSignature    Signature of input parameters or NULL for none.
			 * @param outputSignature      Signature of output parameters or NULL for none.
			 * @param argNames    Comma separated list of input and then output arg names used in interface XML.
			 * @param annotation  Annotation flags.
			 *
			 * @return
			 *      - QStatus.OK if successful
			 *      - QStatus.BUS_MEMBER_ALREADY_EXISTS if member already exists
			 */
			public QStatus AddMember(Message.Type type, string name, string inputSignature,
				string outputSignature, string argNames, AnnotationFlags annotation)
			{
				return alljoyn_interfacedescription_addmember(_interfaceDescription,
					(int)type, name, inputSignature, outputSignature, argNames, (byte)annotation);
			}

			/**
			 * Add a member to the interface.
			 *
			 * @param name        Name of member.
			 * @param inputSig    Signature of input parameters or NULL for none.
			 * @param outSig      Signature of output parameters or NULL for none.
			 * @param argNames    Comma separated list of input and then output arg names used in annotation XML.
			 *
			 * @return
			 *      - QStatus.OK if successful
			 *      - QStatus.BUS_MEMBER_ALREADY_EXISTS if member already exists
			 */
			public QStatus AddMethod(string name, string inputSig, string outSig, string argNames)
			{
				return AddMember(Message.Type.MethodCall, name, inputSig, outSig, argNames);
			}

			/**
			 * Add a member to the interface. With one of the built in annotations.
			 *
			 * The possible annotations are
			 *    - AllJoyn.InterfaceDescription.AnnotationFlags.Default = 0x0
			 *    - AllJoyn.InterfaceDescription.AnnotationFlags.Deprecated = 0x1
			 *    - AllJoyn.InterfaceDescription.AnnotationFlags.NoReply = 0x2
			 *
			 * If Default annotation flag is used no annotations will be added to the method.
			 *
			 * Using the Depricated annotation flag will add the org.freedesktop.DBus.Depricated=true
			 * annotation to the method. This annotation is an indication that the method has been
			 * depricated.
			 *
			 * Using the NoReply annotation flag will add the org.freedesktop.DBus.Method.NoReply=true
			 * annotation to the method. If this annotation is set don't expect a reply to the method
			 * call.
			 *
			 * The Depricated and NoReply annotation flags can be ORed together to add both annotations.
			 *
			 * To add other annotations use the InterfaceDescription.AddMemberAnnotation method.
			 *
			 * @param name        Name of member.
			 * @param inputSig    Signature of input parameters or NULL for none.
			 * @param outSig      Signature of output parameters or NULL for none.
			 * @param argNames    Comma separated list of input and then output arg names used in annotation XML.
			 * @param annotation  Annotation flags.
			 *
			 * @return
			 *      - QStatus.OK if successful
			 *      - QStatus.BUS_MEMBER_ALREADY_EXISTS if member already exists
			 */
			public QStatus AddMethod(string name, string inputSig, string outSig, string argNames, AnnotationFlags annotation)
			{
				return AddMember(Message.Type.MethodCall, name, inputSig, outSig, argNames, annotation);
			}

			//TODO AddMethod with accessParams

			/**
			 * Lookup a member method description by name
			 *
			 * @param name  Name of the method to lookup
			 * @return
			 *      - An InterfaceDescription.Member.
			 *      - NULL if does not exist.
			 */
			public Member GetMethod(string name)
			{
				Member ret = GetMember(name);
				if (ret != null && ret.MemberType == Message.Type.MethodCall)
				{
					return ret;
				}
				else
				{
					return null;
				}
			}

			/**
			 * Add a signal member to the interface.
			 *
			 * If Default annotation flag is used no annotations will be added to the method.
			 *
			 * Using the Depricated annotation flag will add the org.freedesktop.DBus.Depricated=true
			 * annotation to the signal. This annotation is an indication that the signal has been
			 * depricated.
			 *
			 * To add other annotations use the InterfaceDescription.AddMemberAnnotation method.
			 *
			 * @param name        Name of method call member.
			 * @param sig         Signature of parameters or NULL for none.
			 * @param argNames    Comma separated list of arg names used in annotation XML.
			 *
			 * @return
			 *      - QStatus.OK if successful
			 *      - QStatus.BUS_MEMBER_ALREADY_EXISTS if member already exists
			 */
			public QStatus AddSignal(string name, string sig, string argNames)
			{
				return AddMember(Message.Type.Signal, name, sig, "", argNames, InterfaceDescription.AnnotationFlags.Default);
			}

			/**
			 * Add a signal member to the interface.
			 *
			 * @param name        Name of method call member.
			 * @param sig         Signature of parameters or NULL for none.
			 * @param argNames    Comma separated list of arg names used in annotation XML.
			 * @param annotation  Annotation flags.
			 *
			 * @return
			 *      - QStatus.OK if successful
			 *      - QStatus.BUS_MEMBER_ALREADY_EXISTS if member already exists
			 */
			public QStatus AddSignal(string name, string sig, string argNames, AnnotationFlags annotation)
			{
				return AddMember(Message.Type.Signal, name, sig, "", argNames, annotation);
			}

			/**
			 * Lookup a member signal description by name
			 *
			 * @param name  Name of the signal to lookup
			 * @return
			 *      - An InterfaceDescription.Member.
			 *      - NULL if does not exist.
			 */
			public InterfaceDescription.Member GetSignal(string name)
			{
				Member ret = GetMember(name);
				if (ret != null && ret.MemberType == Message.Type.Signal)
				{
					return ret;
				}
				else
				{
					return null;
				}
			}

			/**
			 * Returns a description of the interface in introspection XML format
			 *
			 * @return The interface description in introspection XML format.
			 */
			public string Introspect()
			{
				return Introspect(0);
			}

			/**
			 * Returns a description of the interface in introspection XML format
			 *
			 * @param indent   Number of space chars to use in XML indentation.
			 *
			 * @return The interface description in introspection XML format.
			 */
			public string Introspect(int indent)
			{
				UIntPtr sinkSz = alljoyn_interfacedescription_introspect(_interfaceDescription, IntPtr.Zero, UIntPtr.Zero, (UIntPtr)indent);
				byte[] sink = new byte[(int)sinkSz];

				GCHandle gch = GCHandle.Alloc(sink, GCHandleType.Pinned);
				alljoyn_interfacedescription_introspect(_interfaceDescription, gch.AddrOfPinnedObject(), sinkSz, (UIntPtr)indent);
				gch.Free();
				// The returned buffer will contain a nul character an so we must remove the last character.
				if ((int)sinkSz != 0)
				{
					return System.Text.ASCIIEncoding.ASCII.GetString(sink, 0, (int)sinkSz - 1);
				}
				else
				{
					return "";
				}
			}

			/**
			 * Activate this interface. An interface must be activated before it can be used. Activating an
			 * interface locks the interface so that is can no longer be modified.
			 */
			public void Activate()
			{
				alljoyn_interfacedescription_activate(_interfaceDescription);
			}

			/**
			 * Lookup a member description by name
			 *
			 * @param name  Name of the member to lookup
			 * @return
			 *      - Pointer to member.
			 *      - NULL if does not exist.
			 */
			public Member GetMember(string name)
			{
				_Member retMember = new _Member();
				if(alljoyn_interfacedescription_getmember(_interfaceDescription, name, ref retMember))
				{
					return new Member(retMember);
				}
				else
				{
					return null;
				}
			}

			/**
			 * Get all the members.
			 *
			 * @return  The members array to receive the members.
			 */
			public Member[] GetMembers()
			{
				UIntPtr numMembers = alljoyn_interfacedescription_getmembers(_interfaceDescription,
					IntPtr.Zero, (UIntPtr)0);
				_Member[] members = new _Member[(int)numMembers];
				GCHandle gch = GCHandle.Alloc(members, GCHandleType.Pinned);
				UIntPtr numFilledMembers = alljoyn_interfacedescription_getmembers(_interfaceDescription,
					gch.AddrOfPinnedObject(), numMembers);
				gch.Free();
				if(numMembers != numFilledMembers)
				{
					// Warn?
				}
				Member[] ret = new Member[(int)numFilledMembers];
				for(int i = 0; i < ret.Length; i++)
				{
					ret[i] = new Member(members[i]);
				}

				return ret;
			}

			/**
			 * Check for existence of a member. Optionally check the signature also.
			 * @remark
			 * if the a signature is not provided this method will only check to see if
			 * a member with the given @c name exists.  If a signature is provided a
			 * member with the given @c name and @c signature must exist for this to return true.
			 *
			 * @param name       Name of the member to lookup
			 * @param inSig      Input parameter signature of the member to lookup
			 * @param outSig     Output parameter signature of the member to lookup (leave NULL for signals)
			 * @return true if the member name exists.
			 */
			public bool HasMember(string name, string inSig, string outSig)
			{
				return (alljoyn_interfacedescription_hasmember(_interfaceDescription,
					name, inSig, outSig) == 1 ? true : false );
			}

			/**
			 * Lookup a property description by name
			 *
			 * @param name  Name of the property to lookup
			 * @return a structure representing the properties of the interface
			 */
			public Property GetProperty(string name)
			{
				_Property retProp = new _Property();
				if(alljoyn_interfacedescription_getproperty(_interfaceDescription, name, ref retProp) == 1)
				{
					return new Property(retProp);
				}
				else
				{
					return null;
				}
			}

			/**
			 * Get all the properties.
			 *
			 * @return  The property array that represents the available properties.
			 */
			public Property[] GetProperties()
			{
				UIntPtr numProperties = alljoyn_interfacedescription_getproperties(_interfaceDescription,
					IntPtr.Zero, (UIntPtr)0);
				_Property[] props = new _Property[(int)numProperties];
				GCHandle gch = GCHandle.Alloc(props, GCHandleType.Pinned);
				UIntPtr numFilledProperties = alljoyn_interfacedescription_getproperties(_interfaceDescription,
					gch.AddrOfPinnedObject(), numProperties);
				if(numProperties != numFilledProperties)
				{
					// Warn?
				}
				Property[] ret = new Property[(int)numFilledProperties];
				for(int i = 0; i < ret.Length; i++)
				{
					ret[i] = new Property(props[i]);
				}

				return ret;
			}

			/**
			 * Add a property to the interface.
			 *
			 * @param name       Name of property.
			 * @param signature  Property type.
			 * @param access     Read, Write or ReadWrite
			 * @return
			 *      - QStatus.OK if successful.
			 *      - QStatus.BUS_PROPERTY_ALREADY_EXISTS if the property can not be added
			 *                                        because it already exists.
			 */
			public QStatus AddProperty(string name, string signature, AccessFlags access)
			{
				return alljoyn_interfacedescription_addproperty(_interfaceDescription,
					name, signature, (byte)access);
			}

			/**
			 * Check for existence of a property.
			 *
			 * @param name       Name of the property to lookup
			 * @return true if the property exists.
			 */
			public bool HasProperty(string name)
			{
				return (alljoyn_interfacedescription_hasproperty(_interfaceDescription, name) == 1 ? true : false);
			}


			/**
			 * Add an annotation to a member
			 *
			 * @param property      Name of property
			 * @param name        Name of annotation
			 * @param value       Value for the annotation
			 *
			 * @return
			 *      - QStatus.OK if successful
			 *      - QStatus.BUS_PROPERTY_ALREADY_EXISTS if the annotation can not be added to the property because it already exists
			 */
			public QStatus AddPropertyAnnotation(string property, string name, string value)
			{
				return alljoyn_interfacedescription_addpropertyannotation(_interfaceDescription, property, name, value);
			}

			/**
			 * Get annotation to an existing property.
			 *
			 * @param property     Name of property
			 * @param name       Name of annotation
			 * @param value      Output value for the annotation
			 *
			 * @return
			 *      - true if found
			 *      - false if property not found
			 */
			public bool GetPropertyAnnotation(string property, string name, ref string value)
			{
				UIntPtr value_size = new UIntPtr(); ;
				alljoyn_interfacedescription_getpropertyannotation(_interfaceDescription, property, name, IntPtr.Zero, ref value_size);

				byte[] buffer = new byte[(int)value_size];
				GCHandle gch = GCHandle.Alloc(buffer, GCHandleType.Pinned);
				bool ret = alljoyn_interfacedescription_getpropertyannotation(_interfaceDescription, property, name, gch.AddrOfPinnedObject(), ref value_size);
				gch.Free();
				if (ret)
				{
					// The returned buffer will contain a nul character an so we must remove the last character.
					value = System.Text.ASCIIEncoding.ASCII.GetString(buffer, 0, (Int32)value_size - 1);
				}
				else
				{
					value = "";
				}

				return ret;
			}

			public class Member
			{
				/** Interface that this member belongs to */
				public InterfaceDescription Iface
				{
					get
					{
						return new InterfaceDescription(_member.iface);
					}
				}

				/** %Member type */
				public Message.Type MemberType
				{
					get
					{
						return (Message.Type)_member.memberType;
					}
				}

				/** %Member name */
				public string Name
				{
					get
					{
						return Marshal.PtrToStringAnsi(_member.name);
					}
				}

				/** Method call IN arguments (NULL for signals) */
				public string Signature
				{
					get
					{
						return Marshal.PtrToStringAnsi(_member.signature);
					}
				}

				/** Signal or method call OUT arguments */
				public string ReturnSignature
				{
					get
					{
						return Marshal.PtrToStringAnsi(_member.returnSignature);
					}
				}

				/** Comma separated list of argument names - can be NULL */
				public string ArgNames
				{
					get
					{
						return Marshal.PtrToStringAnsi(_member.argNames);
					}
				}

				/**
				 * Get the value of an annotation
				 *
				 * @param name       Name of annotation.
				 * @param value      Returned value of the annotation
				 * @return
				 *      - true if annotation found.
				 *      - false if annotation not found
				 */
				public bool GetAnnotation(string name, ref string value)
				{

					UIntPtr value_size = new UIntPtr(); ;
					alljoyn_interfacedescription_member_getannotation(_member, name, IntPtr.Zero, ref value_size);

					byte[] buffer = new byte[(int)value_size];
					GCHandle gch = GCHandle.Alloc(buffer, GCHandleType.Pinned);
					bool ret = alljoyn_interfacedescription_member_getannotation(_member, name, gch.AddrOfPinnedObject(), ref value_size);
					gch.Free();
					if (ret)
					{
						// The returned buffer will contain a nul character an so we must remove the last character.
						value = System.Text.ASCIIEncoding.ASCII.GetString(buffer, 0, (Int32)value_size - 1);
					}
					else
					{
						value = "";
					}

					return ret;
				}

				/**
				 * Get a Dictionary containing the names and values of all annotations
				 * associated with this member
				 * 
				 * @return Dictionary containing the names and values of all annotations associated with this member
				 */
				public Dictionary<string, string> GetAnnotations()
				{
					Dictionary<string, string> ret = new Dictionary<string, string>();
					UIntPtr annotation_size = alljoyn_interfacedescription_member_getannotationscount(_member);
					for (uint i = 0; i < (uint)annotation_size; ++i)
					{
						UIntPtr name_size = new UIntPtr();
						UIntPtr value_size = new UIntPtr();

						alljoyn_interfacedescription_member_getannotationatindex(_member, (UIntPtr)i,
							IntPtr.Zero, ref name_size,
							IntPtr.Zero, ref value_size);

						byte[] name_buffer = new byte[(int)name_size];
						byte[] value_buffer = new byte[(int)value_size];

						GCHandle name_gch = GCHandle.Alloc(name_buffer, GCHandleType.Pinned);
						GCHandle value_gch = GCHandle.Alloc(value_buffer, GCHandleType.Pinned);

						alljoyn_interfacedescription_member_getannotationatindex(_member, (UIntPtr)i,
							name_gch.AddrOfPinnedObject(), ref name_size,
							value_gch.AddrOfPinnedObject(), ref value_size);
						name_gch.Free();
						value_gch.Free();
						ret.Add(System.Text.ASCIIEncoding.ASCII.GetString(name_buffer, 0, (Int32)name_size - 1),
							System.Text.ASCIIEncoding.ASCII.GetString(value_buffer, 0, (Int32)value_size - 1));
					}
					return ret;
				}

				internal Member(_Member member)
				{
					_member = member;
				}

				internal Member(IntPtr memberPtr)
				{
					_member = (_Member)Marshal.PtrToStructure(memberPtr, typeof(_Member));
				}

				#region Data
				internal _Member _member;
				#endregion
			}

			/**
			 * Structure representing properties of the Interface
			 */
			public class Property
			{
				/** %Property name */
				public string Name
				{
					get
					{
						return Marshal.PtrToStringAnsi(_property.name);
					}
				}

				/** %Property type */
				public string Signature
				{
					get
					{
						return Marshal.PtrToStringAnsi(_property.signature);
					}
				}

				/** Access is Read, Write, or ReadWrite */
				public AccessFlags Access
				{
					get
					{
						return (AccessFlags)_property.access;
					}
				}

				internal Property(_Property property)
				{
					_property = property;
				}

				/**
				 * Get the value of an annotation
				 *
				 * @param name       Name of annotation.
				 * @param value      Returned value of the annotation
				 * @return
				 *      - true if annotation found.
				 *      - false if annotation not found
				 */
				public bool GetAnnotation(string name, ref string value)
				{
					UIntPtr value_size = new UIntPtr(); ;
					alljoyn_interfacedescription_property_getannotation(_property, name, IntPtr.Zero, ref value_size);

					byte[] buffer = new byte[(int)value_size];
					GCHandle gch = GCHandle.Alloc(buffer, GCHandleType.Pinned);
					bool ret = alljoyn_interfacedescription_property_getannotation(_property, name, gch.AddrOfPinnedObject(), ref value_size);
					gch.Free();
					if (ret)
					{
						// The returned buffer will contain a nul character an so we must remove the last character.
						value = System.Text.ASCIIEncoding.ASCII.GetString(buffer, 0, (Int32)value_size - 1);
					}
					else
					{
						value = "";
					}

					return ret;
				}

				/**
				 * Get a Dictionary containing the names and values of all annotations
				 * associated with this property
				 * 
				 * @return Dictionary containing the names and values of all annotations associated with this property
				 */
				public Dictionary<string, string> GetAnnotations()
				{
					Dictionary<string, string> ret = new Dictionary<string, string>();
					UIntPtr annotation_size = alljoyn_interfacedescription_property_getannotationscount(_property);
					for (uint i = 0; i < (uint)annotation_size; ++i)
					{
						UIntPtr name_size = new UIntPtr();
						UIntPtr value_size = new UIntPtr();

						alljoyn_interfacedescription_property_getannotationatindex(_property, (UIntPtr)i,
							IntPtr.Zero, ref name_size,
							IntPtr.Zero, ref value_size);

						byte[] name_buffer = new byte[(int)name_size];
						byte[] value_buffer = new byte[(int)value_size];

						GCHandle name_gch = GCHandle.Alloc(name_buffer, GCHandleType.Pinned);
						GCHandle value_gch = GCHandle.Alloc(value_buffer, GCHandleType.Pinned);

						alljoyn_interfacedescription_property_getannotationatindex(_property, (UIntPtr)i,
							name_gch.AddrOfPinnedObject(), ref name_size,
							value_gch.AddrOfPinnedObject(), ref value_size);
						name_gch.Free();
						value_gch.Free();
						ret.Add(System.Text.ASCIIEncoding.ASCII.GetString(name_buffer, 0, (Int32)name_size - 1),
							System.Text.ASCIIEncoding.ASCII.GetString(value_buffer, 0, (Int32)value_size - 1));
					}
					return ret;
				}

				internal Property(IntPtr propertyPointer)
				{
					_property = (_Property)Marshal.PtrToStructure(propertyPointer, typeof(_Property));
				}

				#region Data
				internal _Property _property;
				#endregion
			}

			
			#region DLL Imports
			//DLL imports for interface annotations
			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_interfacedescription_addannotation(
				IntPtr iface,
				[MarshalAs(UnmanagedType.LPStr)] string name,
				[MarshalAs(UnmanagedType.LPStr)] string value);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static bool alljoyn_interfacedescription_getannotation(
				IntPtr iface,
				[MarshalAs(UnmanagedType.LPStr)] string name,
				IntPtr value,
				ref UIntPtr value_size);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static UIntPtr alljoyn_interfacedescription_getannotationscount(IntPtr iface);
			[DllImport(DLL_IMPORT_TARGET)]
			private extern static void alljoyn_interfacedescription_getannotationatindex(IntPtr iface,
				UIntPtr index, IntPtr name, ref UIntPtr name_size,
				IntPtr value, ref UIntPtr value_size);

			//add member
			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_interfacedescription_addmember(
				IntPtr iface,
				int type,
				[MarshalAs(UnmanagedType.LPStr)] string name,
				[MarshalAs(UnmanagedType.LPStr)] string inputSig,
				[MarshalAs(UnmanagedType.LPStr)] string outSig,
				[MarshalAs(UnmanagedType.LPStr)] string argNames,
				byte annotation);

			//DLL imports for member annotations
			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_interfacedescription_addmemberannotation(
				IntPtr iface,
				[MarshalAs(UnmanagedType.LPStr)] string member,
				[MarshalAs(UnmanagedType.LPStr)] string name,
				[MarshalAs(UnmanagedType.LPStr)] string value);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static bool alljoyn_interfacedescription_getmemberannotation(
				IntPtr iface,
				[MarshalAs(UnmanagedType.LPStr)] string member,
				[MarshalAs(UnmanagedType.LPStr)] string name,
				IntPtr value,
				ref UIntPtr value_size);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static UIntPtr alljoyn_interfacedescription_member_getannotationscount(_Member member);
			[DllImport(DLL_IMPORT_TARGET)]
			private extern static void alljoyn_interfacedescription_member_getannotationatindex(_Member member,
				UIntPtr index, IntPtr name, ref UIntPtr name_size, 
				IntPtr value, ref UIntPtr value_size);
			[DllImport(DLL_IMPORT_TARGET)]
			private extern static bool alljoyn_interfacedescription_member_getannotation(_Member member,
				[MarshalAs(UnmanagedType.LPStr)] string name, IntPtr value, ref UIntPtr value_size);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static void alljoyn_interfacedescription_activate(IntPtr iface);

			[DllImport(DLL_IMPORT_TARGET)]
			[return: MarshalAs(UnmanagedType.U1)]
			private extern static bool alljoyn_interfacedescription_getmember(IntPtr iface,
				[MarshalAs(UnmanagedType.LPStr)] string name, 
				ref _Member member);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static UIntPtr alljoyn_interfacedescription_getmembers(IntPtr iface,
				IntPtr members, UIntPtr numMembers);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_interfacedescription_hasmember(IntPtr iface,
				[MarshalAs(UnmanagedType.LPStr)] string name,
				[MarshalAs(UnmanagedType.LPStr)] string inSig,
				[MarshalAs(UnmanagedType.LPStr)] string outSig);

			//add property
			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_interfacedescription_getproperty(IntPtr iface,
				[MarshalAs(UnmanagedType.LPStr)] string name,
				ref _Property property);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static UIntPtr alljoyn_interfacedescription_getproperties(IntPtr iface,
				IntPtr props, UIntPtr numProps);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_interfacedescription_addproperty(IntPtr iface,
				[MarshalAs(UnmanagedType.LPStr)] string name,
				[MarshalAs(UnmanagedType.LPStr)] string signature,
				byte access);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_interfacedescription_hasproperty(IntPtr iface,
				[MarshalAs(UnmanagedType.LPStr)] string name);

			//DLL imports for property annotations
			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_interfacedescription_addpropertyannotation(
				IntPtr iface,
				[MarshalAs(UnmanagedType.LPStr)] string property,
				[MarshalAs(UnmanagedType.LPStr)] string name,
				[MarshalAs(UnmanagedType.LPStr)] string value
				);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static bool alljoyn_interfacedescription_getpropertyannotation(
				IntPtr iface,
				[MarshalAs(UnmanagedType.LPStr)] string property,
				[MarshalAs(UnmanagedType.LPStr)] string name,
				IntPtr value,
				ref UIntPtr value_size);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static UIntPtr alljoyn_interfacedescription_property_getannotationscount(_Property property);
			[DllImport(DLL_IMPORT_TARGET)]
			private extern static void alljoyn_interfacedescription_property_getannotationatindex(_Property property,
				UIntPtr index, IntPtr name, ref UIntPtr name_size,
				IntPtr value, ref UIntPtr value_size);
			[DllImport(DLL_IMPORT_TARGET)]
			private extern static bool alljoyn_interfacedescription_property_getannotation(_Property property,
				[MarshalAs(UnmanagedType.LPStr)] string name, IntPtr value, ref UIntPtr value_size);

			[DllImport(DLL_IMPORT_TARGET)]
			[return: MarshalAs(UnmanagedType.U1)]
			private extern static bool alljoyn_interfacedescription_eql(IntPtr one, IntPtr other);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static int alljoyn_interfacedescription_hasproperties(IntPtr iface);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static IntPtr alljoyn_interfacedescription_getname(IntPtr iface);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static UIntPtr alljoyn_interfacedescription_introspect(IntPtr iface, IntPtr str, UIntPtr buf, UIntPtr indent);

			[DllImport(DLL_IMPORT_TARGET)]
			[return: MarshalAs(UnmanagedType.U1)]
			private extern static bool alljoyn_interfacedescription_issecure(IntPtr iface);

			[DllImport(DLL_IMPORT_TARGET)]
			private extern static SecurityPolicy alljoyn_interfacedescription_getsecuritypolicy(IntPtr iface);
			#endregion

			#region Internal Structures
			[StructLayout(LayoutKind.Sequential)]
			internal struct _Member
			{
				public IntPtr iface;
				public int memberType;
				public IntPtr name;
				public IntPtr signature;
				public IntPtr returnSignature;
				public IntPtr argNames;
#pragma warning disable 169 // Field is never used
				public IntPtr internal_member;
#pragma warning restore 169
			}

			[StructLayout(LayoutKind.Sequential)]
			internal struct _Property
			{
				public IntPtr name;
				public IntPtr signature;
				public byte access;
#pragma warning disable 169 // Field is never used
				private IntPtr internal_property;
#pragma warning restore 169
			}
			#endregion

			#region Internal Properties
			internal IntPtr UnmanagedPtr
			{
				get
				{
					return _interfaceDescription;
				}
			}
			#endregion

			#region Data
			IntPtr _interfaceDescription;
			#endregion
		}
	}
}

