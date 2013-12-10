################################################################################
#
# SPLICE Library platform/os/linux/defs.mk makefile
#
################################################################################
# Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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
################################################################################

# Multiple include protection
ifeq ($(origin PLATFORM_OS_DEFS_MK), undefined)
PLATFORM_OS_DEFS_MK := 1

#$(info Reading $(lastword $(MAKEFILE_LIST)))


################################################################################
# OS dependent filename affix's

# Binary executable suffix (<nothing> for Linux)
BIN_EXT =


################################################################################
# Build mode defines and setup

# List of supported build modes for the platform
BUILD_MODES = debug release

################################################################################
# Addition options for targets.

# Undefine the "linux" macro when building the dependencies with cpp.
%.d: CPPFLAGS += $(CC_UNDEF_OPT)linux

endif

