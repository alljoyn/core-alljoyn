# Copyright AllSeen Alliance. All rights reserved.
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
import sys
import os
sys.path.append('tools')

Import('env')
if 'cpp' not in env['bindings']:
    print('Not building security_api because cpp was not specified in bindings')
    Return()

if env['OS_GROUP'] == 'windows':
    if not os.path.exists('external/sqlite3/sqlite3.c') or not os.path.exists('external/sqlite3/sqlite3.h'):
        print('sqlite3.c and sqlite3.h must be present under external/sqlite3. SQLite amalgamation can be obtained from http://www.sqlite.org/download.html.')
        Return()

if env.has_key('_ALLJOYNCORE_'):
    from_alljoyn_core=1
else:
    from_alljoyn_core=0

# Needed for dynamic casting of polymorphic certificates
if env['OS_GROUP'] == 'windows':
    env.Append(CCFLAGS = '-GR')
else:
    env.Append(CXXFLAGS = '-frtti')

if env['OS_GROUP'] == 'windows':
    # Use the proper x86 calling conventions in SQLite
    env.Append(CCFLAGS = '-DSQLITE_CDECL=__cdecl -DSQLITE_STDCALL=__stdcall')

# Indicate that this SConscript file has been loaded already
env['_SECURITY_'] = True

if env['CXX'][:5] == 'clang':
    #clang cannot always deal with gnu++0x
    env.Append(CXXFLAGS = '-D__STRICT_ANSI__')

if not env.has_key('_ALLJOYN_ABOUT_') and os.path.exists(env['AJ_SCL_SRC_DIR'] + '/services/about/SConscript'):
    env.SConscript(env['AJ_SCL_SRC_DIR'] + '/services/about/SConscript')

if 'cpp' in env['bindings'] and not env.has_key('_ALLJOYNCORE_') and os.path.exists(env['AJ_SCL_SRC_DIR'] + '/alljoyn_core/SConscript'):
    env.SConscript(env['AJ_SCL_SRC_DIR'] + '/alljoyn_core/SConscript')

# Make config library and header file paths available to the global environment
env.Append(LIBPATH = '$DISTDIR/security/lib');
env.Append(CPPPATH = '$DISTDIR/security/inc');

secenv = env.Clone(tools = ['textfile'])

sys.path.append(str(secenv.Dir(env['AJ_SCL_SRC_DIR'] + '/build_core/tools/scons').srcnode()))

if env['OS_GROUP'] == 'windows':
    secenv.Append(CCFLAGS = '-WX')
else:
    secenv.Append(CCFLAGS = '-Werror')
    from configurejni import ConfigureJNI
    if not ConfigureJNI(secenv):
        if not GetOption('help'):
            Exit(1)

def create_libpath(self):
    libs = map(lambda s: self.subst(s).replace('#', Dir('#').abspath + '/'), self['LIBPATH'])
    return ':'.join(libs)

AddMethod(secenv, create_libpath, "CreateLibPath")

# Make security C++ and its jar dist a sub-directory of the alljoyn dist.
secenv['SEC_DISTDIR'] = env['DISTDIR'] + '/security'
secenv['JARDIR'] = secenv['SEC_DISTDIR'] + '/jar'

secenv.Append(LIBS = ['alljoyn_about'])

buildroot = secenv.subst('build/${OS}/${CPU}/${VARIANT}')

#Install CPP header files
secenv.Install('$SEC_DISTDIR/security/inc', secenv.Glob('security/inc/*.h'))

secenv.Append(CPPPATH = ['$SEC_DISTDIR/security/inc'])
secenv.Append(CPPPATH = ['#agent/inc/'])
secenv.Append(CPPPATH = ['#storage/inc'])

if env['OS_GROUP'] == 'windows':
    sqlite_objs = secenv.SConscript('external/sqlite3/SConscript', exports = ['secenv'], variant_dir=buildroot+'/ext/lib/sqlite3', duplicate=0)
    secenv.Install('$SEC_DISTDIR/lib', secenv.SConscript('storage/src/SConscript', exports = ['secenv', 'sqlite_objs'], variant_dir=buildroot+'/lib/storage/native', duplicate=0))
else:
    secenv.Install('$SEC_DISTDIR/lib', secenv.SConscript('storage/src/SConscript', exports = ['secenv'], variant_dir=buildroot+'/lib/storage/native', duplicate=0))
secenv.Install('$SEC_DISTDIR/lib', secenv.SConscript('agent/src/SConscript', exports = ['secenv'], variant_dir=buildroot+'/lib/agent', duplicate=0))

# Security Manager App building
secenv.Install('$SEC_DISTDIR/bin/samples', secenv.SConscript('samples/cli/SConscript', exports=['secenv'], variant_dir=buildroot+'/samples/cli', duplicate=0))

# Security core tests building (are not installed)
if secenv['OS_CONF'] != 'android':
     secenv.SConscript('agent/unit_test/SConscript', variant_dir=buildroot+'/test/agent/unit_test', duplicate=0, exports = {'env':secenv})

# Multipeer test
if secenv['OS_CONF'] == 'linux':
     secenv.SConscript('agent/test/multipeer_claim/SConscript', variant_dir=buildroot+'/test/agent/multipeer_claim', duplicate=0, exports = {'env':secenv})


# Security storage tests building (are not installed)
#secenv.SConscript('storage/unit_test/SConscript', exports=['secenv'], variant_dir=buildroot+'/test/storage/unit_test', duplicate=0)
#for test in Glob('storage/test/*', strings=True):
#    testdir = buildroot+'/test/'+'sec'+test
#    secenv.SConscript(test + '/SConscript', exports=['secenv'], variant_dir=testdir, duplicate=0)

# Whitespace policy (only when we don't build from alljoyn)
if from_alljoyn_core == 0:
    if secenv['WS'] != 'off' and not secenv.GetOption('clean'):
        import whitespace
        import time
    
        def wsbuild(target, source, env):
            print "Evaluating whitespace compliance..."
            curdir = os.path.abspath(os.path.dirname(wsbuild.func_code.co_filename))
            config = os.path.join(curdir, 'tools', 'ajuncrustify.cfg')
            print "Config:", config
            print "Note: enter 'scons -h' to see whitespace (WS) options"
            olddir = os.getcwd()
            os.chdir(curdir) #We only want to run uncrustify on datadriven_api
            time.sleep(1) #very dirty hack to prevent concurrent running of uncrustify
            retval = whitespace.main([env['WS'], config])
            os.chdir(olddir)
            return retval
    
        secenv.Command('#/ws_sec', Dir('$SEC_DISTDIR'), wsbuild)
    if secenv['WS'] != 'off':
        secenv.Clean(secenv.File('SConscript'), secenv.File('#/whitespace.db'))
    
