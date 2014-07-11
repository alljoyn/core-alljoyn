#!/bin/bash

# Copyright (c) 2012 - 2014, AllSeen Alliance. All rights reserved.
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

function Usage() {
	set +ex
	echo >&2 "
Runs GTest executables created by scons in alljoyn_core and common, on an Android device using adb.

Usage: $(basename -- "$0") [ -s -d alljoyn_dist ] [ -t alljoyn_test ] [-c configfile ] -- \"adb device\" [ ajtest ] [ cmtest ] ...
where
    -s              # start and stop our own AllJoyn-Daemon (default internal transport address, --no-bt)

    -d alljoyn_dist # path to the build/.../dist tree, used to find cpp/bin/alljoyn-daemon exe, if used

    -t alljoyn_test # path to the build/.../test tree, used to find cpp/bin/cmtest exe, ajtest exe, ...
                    # Note: do not include the 'cpp/bin' subpath in alljoyn_dist or alljoyn_test

    -c configfile   # name of config file(s) (an embedded wildcard is replaced by the gtest test name)
                    #  default '*.conf'

    \"adb device\"  # string needed by adb to contact the device, like \"-s XXXXXXX\" (use quotes if necessary)

    ajtest, cmtest, and/or abouttest
                    # simple file name(s) of the gtest exe(s) to be run (found in -t path/cpp/bin)
                    #  default runs ajtest and cmtest
"
	exit 2
}

: read commandline options

set -e

start_daemon=false
alljoyn_dist=''
alljoyn_test=''
configfile='*.conf'

while getopts sSc:d:t: option
do
	case "$option" in
	( c ) configfile="$OPTARG" ;;
	( d ) alljoyn_dist="$OPTARG" ;;
	( t ) alljoyn_test=$OPTARG ;;
	( s ) start_daemon=true ;;
	( S ) start_daemon=false ;;
	( \? ) Usage ;;
	esac
done

device=''
gtests=''
if test "$OPTIND" -gt 0 -a "$OPTIND" -le $#
then
	shift ` expr $OPTIND - 1 `
	while test $# -ge 1
	do
		case "$device" in
		( '' )
			case "$1" in
			( *[!\ a-zA-Z0-9_-]* | '' )
				echo >&2 "error, $1:  allowed characters are [\ a-zA-Z0-9_-]"
				Usage
				;;
			( * )
				device="$1"
				;;
			esac
			;;
		( * )
			case "$1" in
			( *[!a-zA-Z0-9_-]* | '' )
				echo >&2 "error, $1:  allowed characters are [a-zA-Z0-9_-]"
				Usage
				;;
			( * )
				gtests="$gtests $1"
				;;
			esac
			;;
		esac
		shift
	done
fi

if test -z "$device"
then
	echo >&2 'error, "adb device" argument is required' ; Usage
fi

if test -z "$gtests"
then
	gtests="ajtest cmtest"
fi

: check commandline options

# alljoyn_dist, alljoyn_test paths
if test -z "$alljoyn_dist" -a -z "$alljoyn_test"
then
	echo >&2 "error, -d and/or -t path is required"
	Usage
elif test -z "$alljoyn_dist"
then
	alljoyn_dist="$alljoyn_test/../dist"
elif test -z "$alljoyn_test"
then
	alljoyn_test="$alljoyn_dist/../test"
fi

ckbin() {
	ckvar=$1
	binvar=$2
	eval ckval="\$$1"
	binval=` cd "$ckval/cpp/bin" > /dev/null && pwd ` || : ok
	if test -z "$binval"
	then
		echo >&2 "error, could not find 'cpp/bin' subdirectory in $ckvar=$ckval"
		exit 2
	fi
	eval $binvar="'$binval'"
}

ckbin alljoyn_dist daemon_bin
ckbin alljoyn_test gtest_bin

options='--internal --no-bt --verbosity=5'

echo "# runall test plan:"
if $start_daemon
then
	ls >/dev/null -ld "$daemon_bin/alljoyn-daemon" || ls >/dev/null -ld "$daemon_bin/alljoyn-daemon.exe" || {
		echo >&2 "error, alljoyn-daemon exe not found"
		exit 2
	}
	echo "# $daemon_bin/alljoyn-daemon $options > alljoyn-daemon.log &"
fi

for i in $gtests
do
	ls >/dev/null -ld "$gtest_bin/$i" || ls >/dev/null -ld "$gtest_bin/$i.exe" || {
		echo >&2 "error, $i exe not found"
		exit 2
	}
	c="$( echo "$configfile" | sed -e 's,\*,'$i',g' )"
	ls >/dev/null -ld "$c" || {
		echo >&2 "error, configfile $c not found"
		exit 2
	}
	echo "# python test_harness.py -c $c -t $i -p $gtest_bin > $i.log"
done

kill-alljoyn() {

	: kill any alljoyn processes running on device

	p=$( adb < /dev/null $device shell ps | awk '$NF ~ /alljoyn/ { print $2; exit; }' )
	while test -n "$p"
	do
		adb < /dev/null $device shell kill -9 $p
		sleep 5
		p=$( adb < /dev/null $device shell ps | awk '$NF ~ /alljoyn/ { print $2; exit; }' )
	done
}

: begin

kill-alljoyn || : ok

if $start_daemon
then
	: start alljoyn-daemon

	rm -f alljoyn-daemon.log

	adb < /dev/null $device shell mkdir                       /data/alljoyn || : ok
	adb < /dev/null $device shell rm                          /data/alljoyn/alljoyn-daemon || : ok
	adb < /dev/null $device push "$daemon_bin/alljoyn-daemon" /data/alljoyn
	(
		date
		adb < /dev/null $device shell                             /data/alljoyn/alljoyn-daemon $options
		date
	) > alljoyn-daemon.log 2>&1 </dev/null &

	sleep 5
fi

: run gtest executables

xit=0
for i in $gtests
do
	# configfile for gtest $i
	c="$( echo "$configfile" | sed -e 's,\*,'$i',g' )"

	rm -f $i.adb-sh
	python < /dev/null -u test_harness-android.py -c $c -t $i -o $i.adb-sh -p /data/alljoyn

	: setup $i

	rm -f $i.log $i.xml

	adb < /dev/null $device shell mkdir          /data/alljoyn || : ok
	adb < /dev/null $device shell rm             /data/alljoyn/$i || : ok
	adb < /dev/null $device shell rm             /data/alljoyn/$i.adb-sh || : ok
	adb < /dev/null $device shell rm             /data/alljoyn/$i.xml    || : ok
	adb < /dev/null $device push "$gtest_bin/$i" /data/alljoyn
	adb < /dev/null $device push $i.adb-sh       /data/alljoyn

	: run $i

	date
	adb < /dev/null $device shell sh /data/alljoyn/$i.adb-sh > $i.log 2>&1
	date
	tail -1 $i.log | grep "exiting with status 0" || xit=1

	case "$xit" in
	0 ) echo $i PASSED ;;
	* ) echo $i FAILED, see $i.log for info ;;
	esac

	: get xml file, maybe

	adb < /dev/null $device pull  /data/alljoyn/$i.xml || : ok

	sleep 5
done

kill-alljoyn || : ok

echo exit status $xit
exit $xit
