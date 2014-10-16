AllSeen Security Manager
========================

Welcome to the AllSeen Security Manager (SecurityMgr).

SecurityMgr allows an administrator to configure the security settings of 
remote AllSeen applications. More specifically, it allows an administrator to:

* manage guilds
* claim remote applications
* install identity certificates on remote applications
* install membership certificates on remote applications
* remove membership certificates from remote applications
* install policies on remote applications

More details on the different Security 2.0 concepts can be found on the 
[AllSeen Wiki|https://wiki.allseenalliance.org/core/security_enhancements].

Status
------

SecurityMgr is still under heavy development. It allows other developers to 
have first-hand experiences with its public API, but this API might still 
change without prior notice.

Dependencies
------------

SecurityMgr depends on the following software packages. Make sure you have them 
installed on your system before building SecurityMgr.

* [gtest 1.6.0|https://code.google.com/p/googletest/] available under the [New 
BSD License|http://opensource.org/licenses/BSD-3-Clause]
* [sqlite 3.7.9 2011-11-01|http://www.sqlite.org/] available in the [public
domain|http://www.sqlite.org/copyright.html]

Building
--------

You can build SecurityMmgr using `scons`.

Running the sample CLI
----------------------

You can use the sample CLI of the SecurityMgr to manage any number of 
application stubs.

To start the CLI, execute the following command:

```
cd <securitymgr/build/.../debug/>
LD_LIBRARY_PATH=./dist/cpp/lib/:./dist/security/lib/ ./samples/cli/secmgr
```

To start an application stub, executing the following command from the same 
directory:

```
LD_LIBRARY_PATH=./dist/cpp/lib/:./dist/security/lib/ 
./dist/security/bin/stub/securitystub
```

Running the tests
-----------------

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
