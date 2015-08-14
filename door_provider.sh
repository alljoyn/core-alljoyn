#! /bin/sh
LD_LIBRARY_PATH=$PWD/build/linux/x86_64/debug/lib/core/\
:$PWD/build/linux/x86_64/debug/dist/cpp/lib/\
:$PWD/build/linux/x86_64/debug/dist/security/lib\
:$PWD/build/linux/x86_64/debug/dist/about/lib/\
 ./build/linux/x86_64/debug/samples/door/doorprovider $1
