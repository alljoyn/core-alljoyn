################################################################################
#
# SPLICE Library platform/os/windows/defs.mk makefile
#
################################################################################
# # 
#    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
#    Source Project Contributors and others.
#    
#    All rights reserved. This program and the accompanying materials are
#    made available under the terms of the Apache License, Version 2.0
#    which accompanies this distribution, and is available at
#    http://www.apache.org/licenses/LICENSE-2.0

#
################################################################################

# Multiple include protection
ifeq ($(origin PLATFORM_OS_DEFS_MK), undefined)
PLATFORM_OS_DEFS_MK := 1

#$(info Reading $(lastword $(MAKEFILE_LIST)))


################################################################################
# OS dependent filename affix's

# Binary executable suffix (<nothing> for Linux)
BIN_EXT = .exe


################################################################################
# Build mode defines and setup

# List of supported build modes for the platform
BUILD_MODES = debug release

################################################################################
# Addition options for targets.

endif