# Copyright (c) 2013, AllSeen Alliance. All rights reserved.
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
import SCons.Builder
import os

def _jsdoc2_emitter(target, source, env):
    if len(target) > 1:
        print('scons: *** Only one target may be specified for the jsdoc2 Builder.')
        exit(1)
    if type(target[0]) != SCons.Node.FS.Dir:
        print('scons: *** Target MUST be a Dir node.')
        exit(1);
    # walk the JSDOC_TEMPLATE dir and add the files in the demplate dir to the
    # source list.  This way documentation will be rebuilt if a template file is
    # modified
    template_dir = env.Dir('$JSDOC_TEMPLATE')
    for root, dir, filenames in os.walk(str(template_dir)):
        for f in filenames:
            source.append(root + os.sep + f)
    # make sure the output directory is cleaned.
    env.Clean(source, target[0])
    return target, source

_jsdoc2_builder = SCons.Builder.Builder(
    action ='java -jar ${JSDOC_JSRUN} ${JSDOC_RUN} ${JSDOC_FLAGS} -t=${JSDOC_TEMPLATE} -d=${TARGET} ${SOURCES}',
    src_suffix = '$JSDOC_SUFFIX',
    emitter = _jsdoc2_emitter
)

def generate(env):
    env.Append(BUILDERS = {
        'jsdoc2': _jsdoc2_builder,
    })

    env.AppendUnique(
        # Suffixes/prefixes
        JSDOC_SUFFIX = '.js',
        # JSDoc 2 build flags
        JSDOC_FLAGS = '',
        # full path qualified location of jsrun.jar from JSDoc 2
        JSDOC_JSRUN = 'jsrun.jar',
        # full path qualified location of run.js from JSDoc 2
        JSDOC_RUN = 'run.js',
        # directory containing the publish.js and other template files.
        JSDOC_TEMPLATE = env.Dir('templates')
    )

def exists(env):
    """
    Make sure jsDoc exists.
    """
    java_exists = env.Detect('java')
    jsrun_exists = env.Detect('$JSDOC_JSRUN')
    run_exists = env.Detect('$JSDOC_RUN')
    return (java_exists and jsrun_exists and run_exists)
