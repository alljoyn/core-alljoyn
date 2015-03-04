#!/bin/bash

# Copyright (c) 2010 - 2015, AllSeen Alliance. All rights reserved.
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
Runs JUnit tests with Ant

Usage: $(basename -- "$0") [ -s -f FILE ] -o OS -c CPU -v VARIANT
where
	-s		# start and stop our own AllJoyn-Daemon (requires config file)
	-f		# daemon config file
	-o OS	# same meaning as SCONS
	-c CPU
	-v VARIANT
"
	exit 2
}

: read commandline options

set -e

start_daemon=false
config_file=junit-unix.conf
target_cpu=$CPU
target_os=$OS
variant=$VARIANT

while getopts sSf:c:o:v: option
do
	case "$option" in
	( c ) target_cpu="$OPTARG" ;;
	( o ) target_os="$OPTARG" ;;
	( v ) variant="$OPTARG" ;;
	( f ) config_file="$OPTARG" ;;
	( s ) start_daemon=true ;;
	( S ) start_daemon=false ;;
	( \? ) Usage ;;
	esac
done

: check commandline options

case "/$target_os/$target_cpu/$variant/" in ( *//* | *\ * ) echo >&2 -- "error, -o OS, -c CPU, -v VARIANT are required"; Usage;; esac

if cygpath -wa . > /dev/null 2>&1
then
	: Cygwin, which means Windows

	bus_address="null:"
	if $start_daemon; then
		echo >&2 "error, start_daemon=true but this is Windows and daemon is not supported"
		exit 2
	fi

	# sometimes Windows "home" does not work for keystore tests
	export USERPROFILE="$( cygpath -wa . )"
	export LOCALAPPDATA="$USERPROFILE"
elif pwd -W > /dev/null 2>&1
then
	: MSysGit, which also means Windows

	bus_address="null:"
	if $start_daemon; then
		echo >&2 "error, start_daemon=true but this is Windows and daemon is not supported"
		exit 2
	fi

	# sometimes Windows "home" does not work for keystore tests
	export USERPROFILE="$( pwd -W )"
	export LOCALAPPDATA="$USERPROFILE"
fi

if $start_daemon
then
	: set up for standalone alljoyn-daemon

	case "$config_file" in ( "" ) echo >&2 -- "error, -f CONFIG_FILE is required"; Usage;; esac
	ls -ld "$config_file" || {
		echo >&2 "error, -f config_file=$config_file not found"
		exit 2
	}
	daemon_bin=../../build/$target_os/$target_cpu/$variant/dist/cpp/bin
	"$daemon_bin/alljoyn-daemon" --version || {
		echo >&2 "error, $daemon_bin/alljoyn-daemon exe not found"
		exit 2
	}

	: generate a unique bus address and munge into daemon config file

	bus_address="unix:abstract=$( uuidgen )"
	config_uuid="$PWD/junit-uuid.tmp"
	sed < "$config_file" > "$config_uuid" -e "s/unix:abstract=alljoyn/$bus_address/"
	options="--config-file=$config_uuid --no-bt --no-udp --verbosity=5 --print-address"
else
	: no alljoyn-daemon, use null transport

	bus_address="null:"
fi

ls -ld "../../build.xml" || {
	echo >&2 "error, ../../build.xml not found"
	exit 2
}

kill-alljoyn() {

	: kill any alljoyn-daemon

	killall -9 -v alljoyn-daemon || : ok
	sleep 5
	killall -9 -v alljoyn-daemon || : ok
	sleep 5
}

: begin

rm -f alljoyn-daemon.log

if $start_daemon
then
	kill-alljoyn
	: start alljoyn-daemon

	(
		set -x
		cd "$daemon_bin"
		pwd
		date

		xit=0
		./alljoyn-daemon $options || xit=$?

		date
		set +x
		echo >&2 exit status $xit
	) > alljoyn-daemon.log 2>&1 </dev/null &

	sleep 5
fi

: run Ant JUnit

xit=0
ant > junit.log 2>&1 < /dev/null -f ../../build.xml -Dtest=alljoyn_java/test_report -Dorg.alljoyn.bus.address=$bus_address \
	-DOS=$target_os -DCPU=$target_cpu -DVARIANT=$variant test || xit=$?

sleep 5

if $start_daemon
then
	kill-alljoyn
fi

echo exit status $xit
exit $xit
