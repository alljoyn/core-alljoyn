BusAttachment {#using_bus_attachment}
=========================

@brief BusAttachment connects your code to the AllJoyn framework

@tableofcontents

<!------------------------------------------------------------------------------
Copyright AllSeen Alliance.

THIS DOCUMENT AND ALL INFORMATION CONTAINED HEREIN ARE PROVIDED ON AN AS-IS 
BASIS WITHOUT WARRANTY OF ANY KIND.
------------------------------------------------------------------------------->

Start with a BusAttachment {#start_with_a_busattachment}
-------------------------
The BusAttachment in Alljoyn is the interface between your code and the AllJoyn
framework.  The BusAttachment is used to perform the following actions

 - Create new interfaces
 - Register BusObjects
 - Setup peer security
 - Register signal handlers
 - Setup advertising and discovery
 - Register event listeners associated with bus communication

Every AllJoyn enabled application must contain at least one BusAttachment.
BusAttachments can be very resource intensive.  For this reason it is strongly
recommended that applications only contain a single BusAttachment. It is possible
to do everything needed in AllJoyn with only a single BusAttachment.

The following image gives a summary of many of the components that are associated
with a BusAttachment.

![AllJoyn Endpoint](AllJoynEndpoint.png)