# # 
#    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
#    Source Project Contributors and others.
#    
#    All rights reserved. This program and the accompanying materials are
#    made available under the terms of the Apache License, Version 2.0
#    which accompanies this distribution, and is available at
#    http://www.apache.org/licenses/LICENSE-2.0

#

import os

def ConfigureJNI(env):
    """Configure the environment for building JNI native code. """

    if not env.get('JAVAC'):
        print "Java compiler not found"
        return 0

    java_home = os.environ.get('JAVA_HOME')
    if not java_home:
        print "JAVA_HOME not set"
        return 0

    java_include = [os.path.join(java_home, 'include')]
    java_lib = [os.path.join(java_home, 'lib')]

    java_include.append(os.path.join(java_include[0], 'win32'))
    java_include.append(os.path.join(java_include[0], 'linux'))
    java_include.append(os.path.join(java_include[0], 'darwin'))

    env.Append(CPPPATH = java_include)
    env.Append(LIBPATH = java_lib)

    return 1