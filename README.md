AllSeen Security Manager
========================

Welcome to the AllSeen Security Manager (SecurityMgr).

SecurityMgr allows an administrator to configure the security configuration of
other AllSeen applications. It allows an administrator to:

* claim/reset applications
* update identity certificates
* update policies
* install and remove membership certificates

Generic information on the different Security 2.0 concepts can be found on the
[AllSeen Wiki|https://wiki.allseenalliance.org/core/security_enhancements].
Details on the security manager architecture, its library and additions to the
framework can be found on the [security manager wiki page
|https://wiki.allseenalliance.org/core/security_manager_description].

Status
------

The basic SecurityMgr functionality is completed. It allows other developers to
have first-hand experiences with its public API. However, this API is marked
experimental and might still change.

Dependencies
------------

SecurityMgr depends on the following software packages. Make sure you have them
installed on your system before building SecurityMgr.

* [gtest 1.7.0|https://code.google.com/p/googletest/] available under the [New
BSD License|http://opensource.org/licenses/BSD-3-Clause] for tests
* [sqlite 3.7.9 2011-11-01|http://www.sqlite.org/] available in the [public
domain|http://www.sqlite.org/copyright.html] for the sample storage
implementation

Building
--------

You can build SecurityMmgr using `scons`. Please refer to [alljoyn build
instructions|https://allseenalliance.org/developers/develop/building] for more
details on setting up your environment.

Samples
-------

SecurityMgr comes with a CLI sample, that can be used to demonstrate the
functionality of the security agent. The CLI allows an administrator to
claim and manage the security configuration of any AllSeen application that
supports Security 2.0.

A secure door consumer and provider can be found in standard client library
in the alljoyn_core/samples/secure/door directory:
* The secure door provider implements a door that can be opened and closed over
  a secure interface
* The secure door consumer monitors all doors on the local network

Note
* Even though the door samples are part of core, they are built when compiling
the security manager. You can find them in:
'build/{OS}/{ARCH}/{MODE}/dist/cpp/bin/samples/'
* The samples will generate some artifacts. They create a keystore and the CLI
will also create a database file. In order to have a fresh restart you should
delete these files. Unless configured otherwise, they are created in your home
directory. Keystores under '.alljoyn\_keystore' and the database of the CLI is
named 'secmgrstorage.db'.

Example scenario:

* Start a CLI (cli.sh)
* Start a secure door provider. The secure door provider publishes some
  about data, which can be used to find its ApplicationId.

    >> Old application state = null
    >> New application state = OnlineApplication: Busname: :-ARcsaj-.139, Claim state: CLAIMABLE, Updates pending: false
    >> ApplicationId: 92a2f6d44b030173d307dfeb2fd9c0cc

    Received About signal:
     BusName          : :-ARcsaj-.139
     Application Name : DoorProvider
     Device Name      : cplx224

* Start a secure door consumer.

    >> Old application state = null
    >> New application state = OnlineApplication: Busname: :-ARcsaj-.140, Claim state: CLAIMABLE, Updates pending: false
    >> ApplicationId: 807a6b1d7d387be5fe86cafa4ea57e0f

* Claim the secure door provider and consumer, accepting the manifest requested
  by the applications. First the consumer. The consumer can only be claimed using
  NULL session. No input is required from the administrator.

    > c 807a6b1d7d387be5fe86cafa4ea57e0f
    The application requests the following rights:
    <rule>
      <objPath>*</objPath>
      <interfaceName>sample.securitymgr.door.Door</interfaceName>
      <member>
        <name>*</name>
        <action>Observe</action>
        <action>Modify</action>
      </member>
    </rule>
    Accept (y/n)? y

  The door provider also supports claiming by using a PSK. After approving the
  manifest, the security agent prompts for the desired claiming method: PSK
  or NULL. Press p to select PSK. The CLI will ask you to fill in the PSK. Copy
  then paste the value printed out by the door provider.

    > c 92a2f6d44b030173d307dfeb2fd9c0cc
    The application requests the following rights:
    <rule>
      <objPath>*</objPath>
      <interfaceName>sample.securitymgr.door.Door</interfaceName>
      <member>
        <name>*</name>
        <type>method call</type>
        <action>Provide</action>
      </member>
      <member>
        <name>*</name>
        <type>property</type>
        <action>Provide</action>
      </member>
    </rule>
    Accept (y/n)? y
    Select claim mechanism:
      'n' to claim over a ECDHE_NULL session
      'p' to claim over a ECDHE_PSK session
      others to abort claiming process
    p
    please enter the PSK provided by the application

* Define a security group.

    > g foo/bar
    Group was successfully added
    Group: (26141921a7c84a8bf736e4b461886d71 / foo / bar)

* Install a membership certificate for that guild on both provider and consumer.
  Once installation is complete the sync state should be SYNC_OK.

    > m 92a2f6d44b030173d307dfeb2fd9c0cc 26141921a7c84a8bf736e4b461886d71

    >> Old application state = OnlineApplication: Busname: :-ARcsaj-.139, Claim state: CLAIMED, Sync state: SYNC_PENDING
    >> New application state = OnlineApplication: Busname: :-ARcsaj-.139, Claim state: CLAIMED, Sync state: SYNC_OK
    >> ApplicationId: 92a2f6d44b030173d307dfeb2fd9c0cc

* Install a policy that provides other members of that guild full access on
  both provider and consumer. Again successful installation is indicated when
  the sync state is SYNC_OK.

    > o 92a2f6d44b030173d307dfeb2fd9c0cc 26141921a7c84a8bf736e4b461886d71

* Check whether the secure door consumer can open the secure door provided by the door provider.

    > o

    Calling Open on ':-ARcsaj-.139'
    AuthenticationComplete ALLJOYN_ECDHE_ECDSA
    Open called result = 1

Tests
-----

To build the tests, make sure to set the `GTEST_DIR` to your local Google Test
installation when building SecurityMgr.

Make sure an AllJoyn deamon is running on your system if you have compiled with
BR=off. You can start one using the following script:

```
<securitymgr>/tools/start-alljoyn-daemon.sh
```

You can run all the tests using the following script:

```
<securitymgr>/tools/run-test.sh
```
