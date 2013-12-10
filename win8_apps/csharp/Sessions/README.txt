(This is documentation for use of the Sessions app)

The sessions application is meant to be a testing tool which tests specific situations 
and scenarios. Intermediate conceptual knoweledge of how AllJoyn works is needed to run 
this tool properly.

Sessions gives the user a list of commands which can allow the user to create AllJoyn 
clients, services or a combination of both. To get a list of commands enter help into 
the input box and press enter. You should see the following output:

/////////////////////////////////////////////////////////////////////////////////////
debug <module_name> <level>					- Set debug level for a module
requestname <name>							- Request a well-known name
releasename <name>							- Release a well-known name
bind <port> [traffic] [isMultipoint] [proximity] [transports]	- Bind a session port
unbind <port>								- Unbind a session port
advertise <name> [transports]				- Advertise a name
canceladvertise <name> [transports]			- Cancel an advertisement
find <name_prefix>							- Discover names that begin with prefix
cancelfind <name_prefix>					- Cancel discovering names that begins with prefix
list										- List port bindings, discovered names and active sessions
join <name> <port> [traffic] [isMultipoint] [proximity] [transports]	- Join a session
leave <sessionId>							- Leave a session
chat <sessionId> <msg>						- Send a message over a given session
cchat <sessionId> <msg>						- Send a message over a given session with compression
timeout <sessionId> <linkTimeout>			- Set link timeout for a session
chatecho [on|off]							- Turn on/off chat messages
exit										- Exit this program

SessionIds can be specified by value or by #<idx> where <idx> is the session index + 
printed with "list" command
/////////////////////////////////////////////////////////////////////////////////////

If familiar with AllJoyn you'll see these commands contain the standard functionality
for using AllJoyn applications. By entering any sequence of commands you can test 
AllJoyn situations for future development or to learn how AllJoyn works as the errors
have an explicit output to the user. When specifying the Session Opts for commands 
such as 'bind' or 'join' refer to the table below for a list of valid input.

How to specify the enum types for SessionOpts (Provide decimal value as argument):

TrafficType:
	TRAFFIC_MESSAGES 		= 0x01 		= 1
	TRAFFIC_RAW_UNRELIABLE 	= 0x02 		= 2
	TRAFFIC_RAW_RELIABLE	= 0x04		= 4
	
Proximity:
	PROXIMITY_ANY 			= 0xFF		= 255
	PROXIMITY_PHYSICAL		= 0x01		= 1
	PROXIMITY_NETWORK		= 0x02		= 2
	
TransportMask (The final value should be a bit wise OR of all masks to apply):
	TRANSPORT_NONE			= 0x0000	= 0
	TRANSPORT_ANY			= 0xFFFF	= 65535
	TRANSPORT_LOCAL			= 0x0001	= 1
	TRANSPORT_BLUETOOTH		= 0x0002	= 2
	TRANSPORT_WLAN			= 0x0004	= 4
	TRANSPORT_WWAN			= 0x0008	= 8
	TRANSPORT_LAN			= 0x0010	= 16
	
Examples: 
To run a basic client and service chat interaction run the following commands on 
two different devices. 

Dev1 (Service):

>>>>> bind 55
>>>>> requestname test.service
>>>>> advertise test.service
.... (After the client has executed the join request)
>>>>> chat #0 This is a message sent from the service!

Dev2 (Client):

>>>>> chatecho on
>>>>> requestname test.client
>>>>> find test.service
>>>>> join test.service 55

Output:

Dev1 (Service):
>>>>> bind 55

>>>>> requestname test.service
NameOwnerChanged: name=<test.service>, oldOwner=<>, newOwner=<:63TnEP6I.2>

>>>>> advertise test.service
NameOwnerChanged: name=<:ceTO7HDT.1>, oldOwner=<>, newOwner=<:ceTO7HDT.1>
NameOwnerChanged: name=<:ceTO7HDT.2>, oldOwner=<>, newOwner=<:ceTO7HDT.2>
NameOwnerChanged: name=<test.client>, oldOwner=<>, newOwner=<:ceTO7HDT.2>
Accepting join request on 55 from :ceTO7HDT.2
SessionJoined with :ceTO7HDT.2 (sessionId=3735382421)
RX message from :xyqteCpy.2[3338433761]: This is a message sent from the client!

Dev2 (Client):
>>>>> chatecho on

>>>>> requestname test.client
NameOwnerChanged: name=<test.client>, oldOwner=<>, newOwner=<:0-NvluYR.2>

>>>>> find test.service
Discovered name : test.service

>>>>> join test.service 55
NameOwnerChanged: name=<:dDBmUw-R.1>, oldOwner=<>, newOwner=<:dDBmUw-R.1>
NameOwnerChanged: name=<:dDBmUw-R.2>, oldOwner=<>, newOwner=<:dDBmUw-R.2>
NameOwnerChanged: name=<test.service>, oldOwner=<>, newOwner=<:dDBmUw-R.2>
BusAttachment.JoinSessionAsync(test.service, 55, ...) succeeded with id=3735382421

>>>>> chat #0 This is a message sent from the client!


