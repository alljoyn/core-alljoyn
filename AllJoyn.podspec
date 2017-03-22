# Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
# Project (AJOSP) Contributors and others.

# SPDX-License-Identifier: Apache-2.0

# All rights reserved. This program and the accompanying materials are
# made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0

# Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
# Alliance. All rights reserved.

# Permission to use, copy, modify, and/or distribute this software for
# any purpose with or without fee is hereby granted, provided that the
# above copyright notice and this permission notice appear in all
# copies.

# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
# WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
# AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
# DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
# PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
# TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.

Pod::Spec.new do |s|

  s.name         = "AllJoyn"
  s.version      = "16.10a"
  s.summary      = "AllJoyn in an open source framework that makes it easy for devices and apps to discover and securely communicate with each other."

  s.description  = <<-DESC
                  AllJoyn in an open source framework that makes it easy for devices 
                  and apps to discover and securely communicate with each other.
                  Developers can use the standard interfaces defined by the AllSennAllicance
                  working group or add their own interfaces to create new experiences.
                   DESC

  s.homepage     = "https://allseenalliance.org/"

  s.license      = {
      :type => 'Copyright',
      :text => <<-LICENCE
      Copyright 2017 (c) Open Connectivity Foundation (OCF), AllJoyn Open Source Project (AJOSP) Contributors and others.
      LICENCE
  }

  s.author             = { "OCF" => "admin@openconnectivity.org",
                           "AllSeenAlliance" => "support@allseenalliance.org"
   }

  s.source       = { :git => "https://opengamer@bitbucket.org/opengamer/alljoyn-test.git", :tag => "v16.10a" }

  s.prepare_command = <<-COMMANDS
  cd alljoyn_objc
  sh ./build_ios.sh
  sh ./build_macos.sh
  COMMANDS

  s.ios.source_files = "build/iOS/AllJoynFramework/include/**/*.h"
  s.ios.public_header_files = "build/iOS/AllJoynFramework/include/AJNVersion.h"
  s.ios.vendored_libraries = "build/iOS/AllJoynFramework/Release/*.a"

  s.ios.user_target_xcconfig = { 'OTHER_LDFLAGS' => '$(inherited) -lalljoyn -lajrouter -lAllJoynFramework_iOS -lc++',
                             'OTHER_CFLAGS'  => '$(inherited) -DNS_BLOCK_ASSERTIONS=1 -DQCC_OS_GROUP_POSIX -DQCC_OS_DARWIN',
                             'HEADER_SEARCH_PATHS' => '${PODS_ROOT}/AllJoyn/build/iOS/AllJoynFramework/include/'
  }

  s.ios.deployment_target = '9.0'

  s.osx.source_files = "build/darwin/AllJoynFramework/include/**/*.h"
  s.osx.public_header_files = "build/darwin/AllJoynFramework/include/AJNVersion.h"
  s.osx.vendored_libraries = "build/darwin/AllJoynFramework/Release/*.a"

  s.osx.user_target_xcconfig = { 'OTHER_LDFLAGS' => '$(inherited) -lalljoyn -lajrouter -lAllJoynFramework_macOS -lc++',
                             'OTHER_CFLAGS'  => '$(inherited) -DNS_BLOCK_ASSERTIONS=1 -DQCC_OS_GROUP_POSIX -DQCC_OS_DARWIN',
                             'HEADER_SEARCH_PATHS' => '${PODS_ROOT}/AllJoyn/build/darwin/AllJoynFramework/include'
  }

  s.osx.deployment_target = '10.10'

end
