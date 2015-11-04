[TOC]

Overview  {#overview_section}
=============================
<!------------------------------------------------------------------------------
Copyright AllSeen Alliance.

THIS DOCUMENT AND ALL INFORMATION CONTAINED HEREIN ARE PROVIDED ON AN AS-IS 
BASIS WITHOUT WARRANTY OF ANY KIND.
------------------------------------------------------------------------------->

AllJoyn is a device-to-device communication protocol which enables mobile
devices to support peer-to-peer applications such as multi-player gaming and
social networking.

AllJoyn is designed as a backwards-compatible extension of DBus, a standard
protocol for inter-application communication in the Linux desktop environment.
AllJoyn offers the following features:

- Cross platform/OS/device
- Peer-to-peer communication
- Simplified service discovery
- Flexible security framework and trust models
- Transport agnostic / multi-protocol support
- Multi-language support for the AllJoyn API


This document describes the C++ language binding for the AllJoyn API.

@ref messags_and_msgargs

Type Signatures {#section2}
---------------------------

AllJoyn uses the same type signatures that are used by the Dbus protocol. The
type signature is made up of _type codes_.  Type code is an ASCII character that
represents a standard data type. See the @ref type_system table found on
the @ref messags_and_msgargs page. For a summary of all AllJoyn types.

Message {#section3}
---------------------------

A message is a unit of communication via the AllJoyn protocol.  A message can
send and receive any number of values.
