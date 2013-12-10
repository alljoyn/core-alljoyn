# Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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

# This doxygen builder does not parse the input file and populate the file list
# using an emmitter for this reason to use this builder it is best to specify
# env.Doxygen(source='<doxygen_config_file>', target=Dir('tmp')) 
#    Where the tmp directory is never created meaning that doxygen is run every
#    time the SCons is run
# env.Clean('Doxygen_html', Dir('html'))
#    Where 'html' is the output directory if building latex the output directory 
#    would be latex 

def generate(env):
    # Add Builders for the Doxygen documentation tool
    import SCons.Builder
    doxygen_builder = SCons.Builder.Builder(
        action = "cd ${SOURCE.dir}  &&  ${DOXYGEN} ${SOURCE.file}",
    )

    env.Append(BUILDERS = {
        'Doxygen': doxygen_builder,
    })

    env.AppendUnique(
        DOXYGEN = 'doxygen',
    )

def exists(env):
    """
    Make sure doxygen exists.
    """
    return env.Detect("doxygen")
