AllSeen Security Manager
========================

Welcome to the AllSeen Security Manager (SecurityMgr).

SecurityMgr allows an administrator to configure the security configuration of
other AllSeen applications. It allows an administrator to:

* claim/reset applications
* update identity certificates
* update policies
* install and remove membership certificates

More details on the different Security 2.0 concepts can be found on the
[AllSeen Wiki|https://wiki.allseenalliance.org/core/security_enhancements]

Status
------

SecurityMgr is still under heavy development. It allows other developers to
have first-hand experiences with its public API, but this API might still
change without prior notice.

Dependencies
------------

SecurityMgr depends on the following software packages. Make sure you have them
installed on your system before building SecurityMgr.

* [gtest 1.7.0|https://code.google.com/p/googletest/] available under the [New
BSD License|http://opensource.org/licenses/BSD-3-Clause]
* [sqlite 3.7.9 2011-11-01|http://www.sqlite.org/] available in the [public
domain|http://www.sqlite.org/copyright.html]

Building
--------

You can build SecurityMmgr using `scons`.

Samples
-------

SecurityMgr comes with a set of samples, that can be used to demonstrate the
functionality of the security agent:

Secure door consumer and provider (see alljoyn_core/samples/secure/door):
* a secure door provider which implements a door that can be opened and closed over
  a secure interface
* a secure door consumer which monitors all doors on the local network

A sample CLI:
* a sample CLI that allows an administrator to change the security configuration
  of any AllSeen application that supports Security 2.0, including the door
  provider and consumer

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

* Start a secure door consumer. The secure door consumer does not publish
  any about data, but you should notice its ApplicationId.

    >> Old application state = null
    >> New application state = OnlineApplication: Busname: :-ARcsaj-.140, Claim state: CLAIMABLE, Updates pending: false
    >> ApplicationId: 807a6b1d7d387be5fe86cafa4ea57e0f

* Claim the secure door provider and consumer, accepting the manifest requested
  by the applications

    > c 92a2f6d44b030173d307dfeb2fd9c0cc
    The application requests the following rights:
    Rule:
      interfaceName: sample.securitymgr.door.Door
    Member:
      memberName: *
      action mask: Provide
    Accept (y/n)? y

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

Make sure an AllJoyn deamon is running on your system. You can start one using
the following script:

```
<alljoyn_root>/tools/jenkins/start-alljoyn-daemon.sh
```

You can run all the tests using the following script:

```
<securitymgr>/tools/run-test.sh
```
