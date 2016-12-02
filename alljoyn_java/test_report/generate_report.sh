#!/bin/bash

# # 
#    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
#    Source Project Contributors and others.
#    
#    All rights reserved. This program and the accompanying materials are
#    made available under the terms of the Apache License, Version 2.0
#    which accompanies this distribution, and is available at
#    http://www.apache.org/licenses/LICENSE-2.0

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