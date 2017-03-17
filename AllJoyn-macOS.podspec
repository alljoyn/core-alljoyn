Pod::Spec.new do |s|

  s.name         = "AllJoyn-macOS"
  s.version      = "16.10.0"
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

  s.source       = { :git => "https://git.allseenalliance.org/gerrit/core/alljoyn.git", :tag => "master" }

  s.prepare_command = <<-COMMANDS 
  cd alljoyn_objc
  sh ./build_macos.sh
  COMMANDS

  s.source_files = "build/darwin/AllJoynFramework/include/**/*.h"
  s.public_header_files = "build/darwin/AllJoynFramework/include/*.h"
  s.private_header_files = "build/darwin/AllJoynFramework/include/*.h"
  s.vendored_libraries = "build/darwin/AllJoynFramework/Release/*.a"

  s.user_target_xcconfig = { 'OTHER_LDFLAGS' => '$(inherited) -lalljoyn -lajrouter -lAllJoynFramework_macOS -lstdc++',
                             'OTHER_CFLAGS'  => '$(inherited) -DNS_BLOCK_ASSERTIONS=1 -DQCC_OS_GROUP_POSIX -DQCC_OS_DARWIN',
                             'HEADER_SEARCH_PATHS' => '${PODS_ROOT}/AllJoyn-macOS/build/darwin/AllJoynFramework/include'
  }

  s.platform = :osx

  s.osx.deployment_target = '10.10'

end
