#!/bin/bash

# Copyright (c) 2010 - 2011, AllSeen Alliance. All rights reserved.
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

test -n "$1" && title="$*" || title="Unit Test Report."

pwd
date

: re-run ant junit_report only, with noframes and optional title

ant -version || { echo >&2 "error, ant not found." ; exit 2 ; }
ls -l build_report.xml || { echo >&2 "error, build_report.xml not found." ; exit 2 ; }
ls -l data/TEST*.xml || { echo >&2 "error, data/TEST*.xml not found." ; exit 2 ; }
rm -f junit-noframes.html test_report.html || : ok

xit=0
ant -f build_report.xml -Dtitle="$title" testreport || xit=$?

mv junit-noframes.html test_report.html

date
echo exit status $xit
exit $xit
