# Copyright (c) 2010-2011, 2013, 2014 AllSeen Alliance. All rights reserved.
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

Import('env')

# Set the ABI based on the CPU
if(env['CPU'] == 'arm'):
    android_eabi = 'armeabi'
else:
    android_eabi = 'x86'

# The return value is the collection of files installed in the build destination.
returnValue = []

# Application samples
for sample in ['simple/client', 
               'simple/service', 
               'simple/wfd_service',
               'chat', 
               'raw/client', 
               'raw/service', 
               'secure/logonclient', 
               'secure/rsaclient', 
               'secure/srpclient', 
               'secure/service', 
               'secure/ECDHEclient', 
               'properties/PropertiesClient',
               'properties/PropertiesService',
               'contacts/ContactsClient',
               'contacts/ContactsService',
               'sessionless/chat',
               'sessionless/client',
               'sessionless/service']:
    returnValue += env.Install('$JAVA_DISTDIR/samples/%s' % sample, 
                ['%s/AndroidManifest.xml' % sample,
                 '%s/.classpath' % sample,
                 '%s/project.properties' % sample,
                 '%s/.project' % sample,
                 '%s/res' % sample,
                 '%s/src' % sample])
    returnValue += env.Install('$JAVA_DISTDIR/samples/%s/libs' % sample, '$JARDIR/alljoyn.jar')
    returnValue += env.Install('%s/libs' % sample, '$JARDIR/alljoyn.jar')
    returnValue += env.Install('$JAVA_DISTDIR/samples/%s/libs/%s' % (sample, android_eabi), '$JAVA_DISTDIR/lib/liballjoyn_java.so')
    returnValue += env.Install('%s/libs/%s' % (sample, android_eabi), '$JAVA_DISTDIR/lib/liballjoyn_java.so')

returnValue += env.Install('$JAVA_DISTDIR/samples', 'README.txt')

# install the alljoyn_java.jar and liballjoyn_java.so into the shared 
# keystore samples at this time the shared keystore samples are not being placed
# in the dist folder if they are the samples could be moved into the Application 
# samples loop shown above.
for sample in ['secure/sharedks/service',
               'secure/sharedks/srpclientA',
               'secure/sharedks/srpclientB',
               'sessions']:
    returnValue += env.Install('%s/libs' % sample, '$JARDIR/alljoyn.jar')
    returnValue += env.Install('%s/libs/%s' % (sample, android_eabi), '$JAVA_DISTDIR/lib/liballjoyn_java.so')

# Unit test runner
env.Install('$JAVA_TESTDIR',
            ['test/AndroidManifest.xml',
             'test/.classpath',
             'test/project.properties',
             'test/.project',
             'test/res',
             'test/src'])
env.Install('$JAVA_TESTDIR' + '/libs', '$JARDIR/alljoyn.jar')
env.Install('$JAVA_TESTDIR' + '/libs', '$JARDIR/alljoyn_test.jar')
env.Install('$JAVA_TESTDIR' + '/libs/%s' % android_eabi, '$JAVA_DISTDIR/lib/liballjoyn_java.so')
env.Install('./test/libs/%s' % android_eabi, '$JAVA_DISTDIR/lib/liballjoyn_java.so')
env.Install('./test/libs', '$JARDIR/alljoyn.jar')
env.Install('./test/libs', '$JARDIR/alljoyn_test.jar')

Return('returnValue')
