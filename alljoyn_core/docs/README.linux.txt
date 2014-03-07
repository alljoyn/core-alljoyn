
AllJoyn SDK for Linux
---------------------

This subtree contains one complete copy of the AllJoyn SDK for Linux, built for
a single CPU type (either x86 or x86_64) and VARIANT (either debug or release).
The CPU and VARIANT are normally incorporated into the name of the package or
folder containing this SDK.

Please see ReleaseNotes.txt for the applicable AllJoyn release version and
related information on new features and known issues.


Summary of file and directory structure:
----------------------------------------

The contents of this SDK are arranged into the following top level folders:

cpp/    core AllJoyn functionality, implemented in C++
          - built from alljoyn.git source subtrees alljoyn_core and common
          - required for all AllJoyn applications
java/   optional Java language binding          (built from alljoyn_java)
js/     optional Javascript binding             (built from alljoyn_js)
c/      optional ANSI C language binding        (built from alljoyn_c)
about/  implements AllJoyn About Feature. (built from services/about/(cpp and java))


The contents of each top level folder are further arranged into sub-folders:

        ----------------------------------------------
cpp/    core AllJoyn functionality, implemented in C++
        ----------------------------------------------

    bin/                        executable binaries

        alljoyn-daemon                  installable AllJoyn daemon

    bin/samples/                pre-built sample programs

        SampleDaemon                    Easy-to-use daemon for use with
                                        AllJoyn Thin Client

    docs/html/                  AllJoyn Core API documentation

    inc/alljoyn/                AllJoyn Core (C++) headers
    inc/qcc/

    lib/                        AllJoyn Core (C++) client libraries

        liballjoyn.a                    implements core API
        liballjoyn.so                   requires LD_LIBRARY_PATH=/path/to/lib/folder

        libajrouter.a                   implements bundled-router
        BundledRouter.o

    samples/                    C++ sample programs (see README)


        ---------------------
java/   Java language binding
        ---------------------

    docs/html/                  API documentation

    jar/                        client library, misc jar files

        alljoyn.jar                     AllJoyn Java API

    lib/liballjoyn_java.so      client library

    samples/                    sample programs (see README)


        ---------------------------
js/     Javascript language binding
        ---------------------------

    docs/html/                  API documentation

    lib/libnpalljoyn.so         Installable Netscape plugin for AllJoyn

    samples/                    sample Javascript apps


        -----------------------
c/      ANSI C language binding
        -----------------------

    docs/html/                  API documentation

    inc/alljoyn_c/              ANSI C headers
    inc/qcc/

    lib/                        client libraries


        ---------------------
about/  AllJoyn About Feature
        ---------------------

    bin/                        pre-built sample apps

    jar/                        client library, misc jar files

        alljoyn_about.jar               About Java API

    inc/alljoyn/about                   About C++ headers

    lib/                                About C++ client libraries

        liballjoyn_about.a
        liballjoyn_about.so
