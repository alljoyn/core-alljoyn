# Copyright (c) 2013, AllSeen Alliance. All rights reserved.
#
#    Permission to use, copy, modify, and/or distribute this software for any
#    purpose with or without fee is hereby granted, provided that the above
#    copyright notice and this permission notice appear in all copies.
#
#    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
#    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
#    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
#    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
#    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
#    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
#    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

function cleanup {
  killall -1 chrome alljoyn-daemon java >/dev/null 2>&1 && s
  killall -9 chrome alljoyn-daemon java >/dev/null 2>&1 && s
}

function s {
  if [ "`hostname | grep isk | wc -l`" == "1" ] ; then
    sleep 30
  fi
  if [ "$1" == "" ] ; then
    for i in {1..3}
    do
      sleep 1
      echo -n .
    done
  else
    for i in $(eval echo "{1..$1}");
    do
      sleep 1
      echo -n .
    done
  fi
  echo .
}

function startservers {
  java -jar /opt/JsTestDriver/JsTestDriver.jar --port 4224 --browserTimeout 90000 > /dev/null 2>&1 &
  JAVASERVERPID=$!
  s
  if [ "`ps aux | grep $JAVASERVERPID | grep -v grep | wc -l`" == "0" ] ; then
    bail "java server could not start on port 4224; need to kill old java process"
  fi
  if [ "`ps aux | grep alljoyn-daemon | grep -v grep | wc -l`" != "0" ] ; then
    bail "old alljoyn-daemons hanging around"
  fi
  $INSTALLDIR/dist/bin/alljoyn-daemon > /dev/null 2>&1 &
  ALLJOYNDAEMONPID=$!
  if [ "`ps aux | grep $ALLJOYNDAEMONPID | grep -v grep | wc -l`" == "0" ] ; then
    bail "alljoyn-daemon could not start"
  fi
  s
}

function startbrowsers {
  google-chrome --load-plugin=$INSTALLDIR/dist/js/lib/libnpalljoyn.so http://localhost:4224/capture > /dev/null 2>&1 &
  if [ "`hostname | grep isk | wc -l`" == "1" ] ; then
    s 90
  else
    s 20
  fi
}

function executetests {
  echo TEST OF $1 BEGINS
  java -jar /opt/JsTestDriver/JsTestDriver.jar --config jsTestDriver-npapi.conf --tests $1 --testOutput $TESTDIR
  if [ $? -ne 0 ] ; then
    FAIL=YES
  fi
  s
  echo TEST OF $1 ENDS
}

function testjs {
  cleanup
  startservers
  startbrowsers
  executetests $1
  cleanup
}

function bail {
  echo "FAILFAILFAIL: " $1
  cleanup
  exit 1
}

function results {
  TC=`ls $TESTDIR/*xml|wc -l`
  if [ "$TC" -eq "0" ] ; then
    bail "No Test Results."
  fi

  NF=`grep \<testsuite $TESTDIR/*xml |grep -v  failures=\"0\" | wc -l`
  if [ "$NF" -ne "0" ] ; then
    bail "Some tests failed."
  fi

  if [ "$FAIL" == "YES" ] ; then
    bail "Some tests failed in ways that didn't record a failure in the test results. Probably a browser error."
  else
    echo WINWINWIN No tests failed.
    exit 0
  fi
}

echo TESTING BEGINS
FAIL=NO
if [ "$1" == "" ] ; then
  bail "MISSING ARGUMENT"
fi

INSTALLDIR=$1

if [ ! -d "$INSTALLDIR" ]; then
  bail "INSTALLDIR $INSTALLDIR is not a directory"
fi

LD_LIBRARY_PATH=$INSTALLDIR/dist/lib
export LD_LIBRARY_PATH

TESTDIR=$INSTALLDIR/testOutput

mkdir $TESTDIR
if [ $? -ne 0 ] ; then
  bail "MKDIR FAILED"
fi

# Disabled tests.
# SessionTest,MarshalTest
for tn in AddressBookTest,AuthListenerTest,BusAttachmentArgCountTest,BusAttachmentHelpersTest,BusAttachmentTest,BusErrorTest \
          BusListenerTest,BusObjectTest,ConstantsTest,GameTest,InterfaceDescriptionTest,KeyStoreTest,MessageArgCountTest \
          MessageTest,PropsTest,ProxyBusObjectArgCountTest,ProxyBusObjectTest,ProxyInterfaceTest,ProxyMethodArgCountTest \
          ProxyMethodTest,RequestPermission,SimpleTest,SocketFdArgCountTest,SocketFdTest,VersionTest;
do
  testjs $tn
done
echo TESTING ENDS

cleanup
results
