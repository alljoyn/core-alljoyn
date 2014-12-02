#!/bin/bash

# Copyright (c) 2010-2012, 2014, AllSeen Alliance. All rights reserved.
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


set -x
killall -9 -v alljoyn-daemon
rm -rf alljoyn-daemon.log junit.log

# set ALLJOYN_JAVA and/or ALLJOYN_DIST either through environment variables...
# .. or by putting ALLJOYN_JAVA=something and/or ALLJOYN_DIST=something as commandline parameters
while test $# -gt 0;
do
	if test -n "$1"; then
		eval "$1"
	fi
	shift
done

if test -z "$ALLJOYN_JAVA"
then
	: setting default ALLJOYN_JAVA...
	ALLJOYN_JAVA=` cd .. > /dev/null && pwd `
fi
t=` cd "$ALLJOYN_JAVA" > /dev/null && pwd `
if test -z "$t"
then
	echo >&2 "error, ALLJOYN_JAVA=$ALLJOYN_JAVA not found."
	exit 2
else
	ALLJOYN_JAVA=$t
fi

if test -z "$ALLJOYN_DIST"
then
	: setting default ALLJOYN_DIST...
	ALLJOYN_DIST=` cd "$ALLJOYN_JAVA"/../build/*/*/*/dist > /dev/null && pwd `
fi
t=` cd "$ALLJOYN_DIST" > /dev/null && pwd `
if test -z "$t"
then
	echo >&2 "error, ALLJOYN_DIST=$ALLJOYN_DIST not found."
	exit 2
else
	ALLJOYN_DIST=$t
fi

# sometimes Windows "home" does not work for JUnit tests ClearKeyStore, DefaultKeyStoreListener
if cygpath -wa . > /dev/null 2>&1
then
	: Cygwin
	export USERPROFILE="$( cygpath -wa . )"
	export LOCALAPPDATA="$USERPROFILE"
fi

: start two copies of alljoyn-daemon

( cd "$ALLJOYN_DIST/bin" && ls -l alljoyn-daemon ) || { echo >&2 "error, alljoyn-daemon not found." ; exit 2 ; }

: get target_os/target_cpu/variant
os_cpu_variant=` cd "$ALLJOYN_DIST/.." > /dev/null && pwd | awk -F/ 'NR==1 && NF>=3 { print $(NF-2) "/" $(NF-1) "/" $NF; }' `
if test -z "$os_cpu_variant"
then
	echo >&2 "error, cannot get target_os/target_cpu/variant from ALLJOYN_DIST=$ALLJOYN_DIST"
fi

(
	:
	: first alljoyn-daemon on port 5342
	:

	if cygpath -wa . > /dev/null 2>&1
	then
		: Cygwin
		configfile=$( cygpath -wa "$ALLJOYN_JAVA/test_report/junit-win.conf" )
	else
		: Linux
		configfile="$ALLJOYN_JAVA/test_report/junit-unix.conf"
	fi

	cd "$ALLJOYN_DIST/bin" || exit 2
	pwd
	date

	./alljoyn-daemon --config-file="$configfile"; xit=$?

	date
	set +x
	echo exit status $xit
) > alljoyn-daemon.log 2>&1 </dev/null &

sleep 5

: run ant junit

( cd "$ALLJOYN_JAVA" && ls -l build.xml ) || { echo >&2 "error, build.xml not found." ; exit 2 ; }
ant -version || { echo >&2 "error, ant not found." ; exit 2 ; }

pwd
date

(
	test=$PWD
	build=` cd "$ALLJOYN_DIST/.." > /dev/null && pwd `
	classes="$ALLJOYN_JAVA/test/build/$os_cpu_variant/obj/classes"
	cd "$ALLJOYN_JAVA" || exit 2
	pwd
	date

	# 2011-01-06 Added explicit build, dist, classes properties definitions.
	# 2011-05-11 Cleaned up, corrected errors in bus server address properties.
	# Windows tests now succeed so long as you run both bbdaemons externally.
	# If running both bbdaemons externally for Windows and Android,
	# then may as well do so for Linux too- at least we will see the log file.
	# Prevent JUnit from starting the second bbdaemon internally,
	# by NOT adding ALLJOYN_DIST/bin to the PATH seen by Ant.

	if cygpath -wa . > /dev/null 2>&1
	then
		: Cygwin
		ant > test_report/junit.log 2>&1 < /dev/null \
			-Dtest="$( cygpath -wa "$test" )" \
			-Dbuild="$( cygpath -wa "$build" )" \
			-Ddist="$( cygpath -wa "$ALLJOYN_DIST/java" )" \
			-Dclasses="$( cygpath -wa "$classes" )" \
			-Dorg.alljoyn.bus.address="tcp:addr=127.0.0.1,port=5342" \
			test
		xit=$?
	else
		: Linux
		ant > test_report/junit.log 2>&1 < /dev/null \
			-Dtest="$test" \
			-Dbuild="$build" \
			-Ddist="$ALLJOYN_DIST/java" \
			-Dclasses="$classes" \
			-Dorg.alljoyn.bus.address="unix:abstract=alljoyn" \
			test
		xit=$?
	fi

	date
	set +x
	echo exit status $xit
	exit $xit
) ; xit=$?

killall -9 -v alljoyn-daemon

echo exit status $xit
exit $xit
