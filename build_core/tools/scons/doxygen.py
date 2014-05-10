# Copyright (c) 2012-2014, AllSeen Alliance. All rights reserved.
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
import SCons.Builder
import os
import ConfigParser
import fnmatch
import re

# this class will allow us to use Pythons built in config parser.
# The Config parser is designed to understand ini files with headers Doxygen's 
# Config file is in the form of an ini file without an headers.  this will add
# a fake section header to the file when it is being read.
class FakeSecHead(object):
   def __init__(self, fp):
     self.fp = fp
     self.sechead = '[doxy_config]\n'
   # when calling readline on the self.fp return the sechead then assign 'None'
   # all following calls to readline will then call self.fd.readline
   def readline(self):
     if self.sechead:
       try: return self.sechead
       finally: self.sechead = None
     else: return self.fp.readline()

# doxygen can read in environment varibles if they have the form '$(VAR)
# scons only understands environment vars of form $VAR.  This will find
# the doxygen expresion change is to an expression SCons understands and
# will do the environment variable substitution
def _doxygen_expand_environment_vars(str, env):
    env_var = re.match('(\$\()(\S*)(\))', str)
    if env_var != None:
        return re.sub('\$\(\S*\)', env.subst('${' + env_var.group(2) + '}'), str)
    else:
        return str

# if the path to the file of directory starts with '#' character this
# can only be interperated by scons.  We will change it to an object path
# so we can then walk the object path.
def _doxygen_replace_start_hash_if_found(str, env):
    if str and str[0] == '#':
        return os.path.join(env.Dir('#').abspath, str[1:])
    else:
        return str

def _doxygen_scanner(node, env, path):
    source = []
    #Default file pattern from doxygen 1.8
    file_patterns = ['*.c', '*.cc', '*.cxx', '*.cpp', '*.c++', '*.java',
                    '*.ii', '*.ixx', '*.ipp', '*.i++', '*.inl', '*.idl',
                    '*.ddl', '*.odl', '*.h', '*.hh', '*.hxx', '*.hpp',
                    '*.h++', '*.cs', '*.d','*.php', '*.php4', '*.php5',
                    '*.phtml', '*.inc', '*.m', '*.markdown', '*.md',
                    '*.mm', '*.dox', '*.py', '*.f90', '*.f', '*.for',
                    '*.tcl', '*.vhd', '*.vhdl', '*.ucf', '*.qsf', '*.as',
                    '*.js']
    # TODO figure out a way to change the default file pattern depending on the version
    #      of doxygen being used.
    #Default file pattern from doxygen 1.6
    #file_patterns = ['*.c', '*.cc', '*.cxx', '*.cpp', '*.c++', '*.d',
    #                 '*.java', '*.ii', '*.ixx', '*.ipp', '*.i++', '*.inl',
    #                 '*.h', '*.hh', '*.hxx', '*.hpp', '*.h++', '*.idl',
    #                 '*.odl', '*.cs', '*.php', '*.php3', '*.inc', '*.m',
    #                 '*.mm', '*.dox', '*.py', '*.f90', '*.f', '*.for',
    #                 '*.vhd', '*.vhdl']

    # all paths listed in the config file are relative to the config file
    config_file_path = '.'

    config = ConfigParser.SafeConfigParser()
    try:
        fp = open(str(node), 'r')
        config.readfp(FakeSecHead(fp))
    finally:
        fp.close()

    config_file_path = os.path.abspath(os.path.dirname(str(node)))

    recursive = False
    input = []

    generate_html = True
    generate_latex = True

    example_path = []
    example_patterns = []
    example_recursive = False

    ############################################################################
    # PROCESS INPUT FILES
    ############################################################################
    if config.has_option('doxy_config', 'file_patterns'):
         file_patterns_from_config = config.get('doxy_config', 'file_patterns').replace('\\', '').split()
         if file_patterns_from_config: 
             file_patterns = file_patterns_from_config

    if config.has_option('doxy_config', 'recursive'):
        if config.get('doxy_config', 'recursive') == 'YES':
            recursive = True

    if config.has_option('doxy_config', 'input'):
        input = config.get('doxy_config', 'input').replace('\\', '').split()

    # if no input is found in the config file the current directory is search
    # otherwise it will check to see if the input file found is a file or a 
    # directory.  If it is a directory is will search the directory. If recursive
    # search was specified in the config file a recursive directory search will
    # be done.  Only files that match the FILE_PATTERNS option will be added to 
    # the source list
    # TODO when processing the input take into account the following tags
    #    EXCLUDE
    #    EXLUDE_SYMLINKS
    #    EXLUDE_PATTERNS 
    if not input:
        for filename in os.listdir(os.path.abspath(config_file_path)):
                        for f_pattern in file_patterns:
                            if fnmatch.fnmatchcase(filename, f_pattern):
                                source.append(os.path.abspath(os.path.join(config_file_path, filename)))
    else:
        for i in input:
            i = _doxygen_expand_environment_vars(i, env)
            i = _doxygen_replace_start_hash_if_found(i, env)
            # relative paths are relative to the location of the config file
            if not os.path.isabs(i):
                i = os.path.join(config_file_path, i)
            if os.path.isfile(i):
                source.append(os.path.abspath(i))
            elif os.path.isdir(i):
                if recursive:
                    for root, dir, filenames in os.walk(i):
                        for f_pattern in file_patterns:
                            for pmatch in fnmatch.filter(filenames, f_pattern):
                                source.append(os.path.abspath(os.path.join(root, pmatch)))
                else:
                    for filename in os.listdir(os.path.abspath(i)):
                        for f_pattern in file_patterns:
                            if fnmatch.fnmatchcase(filename, f_pattern):
                                source.append(os.path.abspath(os.path.join(i, filename)))

    ############################################################################
    # PROCESS EXAMPLE TAGS
    ############################################################################
    if config.has_option('doxy_config', 'example_patterns'):
         example_patterns = config.get('doxy_config', 'example_patterns').replace('\\', '').split()

    if example_patterns == []:
        example_patterns = None

    if config.has_option('doxy_config', 'example_recursive'):
        if config.get('doxy_config', 'example_recursive') == 'YES':
            example_recursive = True

    if config.has_option('doxy_config', 'example_path'):
        example_path = config.get('doxy_config', 'example_path').replace('\\', '').split()

    # if the example_path don't add any files to the source list
    # if the example_path is not blank but example_patterns is blank then add all
    # files found in the path to the source list.  Use the example_recursive to
    # determine if the example_path should be recursivly searched.
    if example_path:
        for e in example_path:
            e = _doxygen_expand_environment_vars(e, env)
            e = _doxygen_replace_start_hash_if_found(e, env)
            # relative paths are relative to the location of the config file
            if not os.path.isabs(e):
                e = os.path.normpath(os.path.join(config_file_path, e))
            if os.path.isdir(e):
                if example_recursive:
                    for root, dir, filenames in os.walk(e):
                        if example_patterns:
                            for e_pattern in example_patterns:
                                for pmatch in fnmatch.filter(filenames, e_pattern):
                                    source.append(os.path.abspath(os.path.join(root, pmatch)))
                        else:
                            source.append(os.path.abspath(os.path.join(root, filenames)))
                else:
                    for filename in os.listdir(os.path.abspath(e)):
                        if example_patterns:
                            for e_pattern in example_patterns:
                                if fnmatch.fnmatchcase(filename, e_pattern):
                                    source.append(os.path.abspath(os.path.join(e, filename)))
                        else:
                            if os.path.isfile(os.path.join(e, filename)):
                                source.append(os.path.abspath(os.path.join(e, filename)))

    ############################################################################
    # PROCESS HTML TAGS
    ############################################################################
    if config.has_option('doxy_config', 'generate_html'):
        if config.get('doxy_config', 'generate_html') == 'NO':
            generate_html = False

    if generate_html:
        if config.has_option('doxy_config', 'html_header'):
            html_header = config.get('doxy_config', 'html_header')
            html_header = _doxygen_expand_environment_vars(html_header, env)
            html_header = _doxygen_replace_start_hash_if_found(html_header, env)
            # relative paths are relative to the location of the config file
            if not os.path.isabs(html_header):
                html_header = os.path.join(config_file_path, html_header)
            if os.path.isfile(html_header):
                source.append(os.path.abspath(html_header))
        if config.has_option('doxy_config', 'html_footer'):
            html_footer = config.get('doxy_config', 'html_footer')
            html_footer = _doxygen_expand_environment_vars(html_footer, env)
            html_footer = _doxygen_replace_start_hash_if_found(html_footer, env)
            # relative paths are relative to the location of the config file
            if not os.path.isabs(html_footer):
                html_footer = os.path.join(config_file_path, html_footer)
            if os.path.isfile(html_footer):
                source.append(os.path.abspath(html_footer))
        if config.has_option('doxy_config', 'html_stylesheet'):
            html_stylesheet = config.get('doxy_config', 'html_stylesheet')
            html_stylesheet = _doxygen_expand_environment_vars(html_stylesheet, env)
            html_stylesheet = _doxygen_replace_start_hash_if_found(html_stylesheet, env)
            # relative paths are relative to the location of the config file
            if not os.path.isabs(html_stylesheet):
                html_stylesheet = os.path.join(config_file_path, html_stylesheet)
            if os.path.isfile(html_stylesheet):
                source.append(os.path.abspath(html_stylesheet))
        if config.has_option('doxy_config', 'html_extra_stylesheet'):
            html_extra_stylesheet = config.get('doxy_config', 'html_extra_stylesheet')
            html_extra_stylesheet = _doxygen_expand_environment_vars(html_extra_stylesheet, env)
            html_extra_stylesheet = _doxygen_replace_start_hash_if_found(html_extra_stylesheet, env)
            # relative paths are relative to the location of the config file
            if not os.path.isabs(html_extra_stylesheet):
                html_extra_stylesheet = os.path.join(config_file_path, html_extra_stylesheet)
            if os.path.isfile(html_extra_stylesheet):
                source.append(os.path.abspath(html_extra_stylesheet))
        if config.has_option('doxy_config', 'html_extra_files'):
            html_extra_files = config.get('doxy_config', 'html_extra_files').replace('\\', '').split()
            for hef in html_extra_files:
                 hef = _doxygen_expand_environment_vars(hef, env)
                 hef = _doxygen_replace_start_hash_if_found(hef, env)
                 # relative paths are relative to the location of the config file
                 if not os.path.isabs(hef):
                     hef = os.path.join(config_file_path, hef)
                 if os.path.isfile(hef):
                     source.append(os.path.abspath(hef))

    ############################################################################
    # PROCESS IMAGE_PATH TAG
    ############################################################################
    # since doxygen's only limit on image formats is that it must be supported by
    # based on the document type being produced. We simply look at every file in
    # the folder specified in the image_path and add them to the source list.
    if config.has_option('doxy_config', 'image_path'):
        image_path = config.get('doxy_config','image_path')
        image_path = _doxygen_expand_environment_vars(image_path, env)
        image_path = _doxygen_replace_start_hash_if_found(image_path, env)
        if image_path:
            for filename in os.listdir(os.path.abspath(image_path)):
                if os.path.isfile(os.path.join(image_path, filename)):
                    source.append(os.path.abspath(os.path.join(image_path, filename)))

    ############################################################################
    # PROCESS LAYOUT_FILE TAG
    ############################################################################
    # if layout file found add is to the source list.  If not found see if the
    # default layout file is present 'DoxygenLayout.xml' if it is present add it
    # to the source files list.
    if config.has_option('doxy_config', 'layout_file'):
        layout_file = config.get('doxy_config', 'layout_file')
        if os.path.isfile(layout_file):
            source.append(os.path.abspath(layout_file))
        else:
            if os.path.isfile('DoxygenLayout.xml'):
                source.append(os.path.abspath('DoxygenLayout.xml'))

    ############################################################################
    # PROCESS LATEX / PDF input
    ############################################################################
    if config.has_option('doxy_config', 'generate_latex'):
        if config.get('doxy_config', 'generate_latex') == 'NO':
            generate_latex = False

    if generate_latex:
        if config.has_option('doxy_config', 'latex_header'):
            latex_header = config.get('doxy_config', 'latex_header')
            latex_header = _doxygen_expand_environment_vars(latex_header, env)
            latex_header = _doxygen_replace_start_hash_if_found(latex_header, env)
            # relative paths are relative to the location of the config file
            if not os.path.isabs(latex_header):
                latex_header = os.path.join(config_file_path, latex_header)
            if os.path.isfile(latex_header):
                source.append(os.path.abspath(latex_header))
        if config.has_option('doxy_config', 'latex_footer'):
            latex_footer = config.get('doxy_config', 'latex_footer')
            latex_footer = _doxygen_expand_environment_vars(latex_footer, env)
            latex_footer = _doxygen_replace_start_hash_if_found(latex_footer, env)
            # relative paths are relative to the location of the config file
            if not os.path.isabs(latex_footer):
                latex_footer = os.path.join(config_file_path, latex_footer)
            if os.path.isfile(latex_footer):
                source.append(os.path.abspath(latex_footer))
        if config.has_option('doxy_config', 'latex_extra_files'):
            latex_extra_files = config.get('doxy_config', 'latex_extra_files').replace('\\', '').split()
            for lef in latex_extra_files:
                 lef = _doxygen_expand_environment_vars(lef, env)
                 lef = _doxygen_replace_start_hash_if_found(lef, env)
                 # relative paths are relative to the location of the config file
                 if not os.path.isabs(lef):
                     lef = os.path.join(config_file_path, lef)
                 if os.path.isfile(lef):
                     source.append(os.path.abspath(lef))

    # TODO list of tags that effect the source and target list that are not 
    #    processed. Since none of our config files currently use these tags not 
    #    processing them should not affect the output. If any of these tags are
    #    used then this emitter may need to be expanded.
    #    EXCLUDE
    #    EXLUDE_SYMLINKS
    #    EXLUDE_PATTERNS
    #    INPUT_FILTER
    #    FILTER_PATTERNS
    #    FILTER_SOURCE_FILES
    #    FILTER_SOURCE_PATTERNS
    #    CITE_BIB_FILES
    #    GENERATE_HTMLHELP
    #    CHM_FILE
    #    HHC_LOCATION
    #    GENERATE_CHI
    #    GENERATE_QHP
    #    QCH_FILE
    #    QHC_LOCATION
    #    QHC_NAMESPACE
    #    QHP_VIRTUAL_FOLDER
    #    QHP_CUST_FILTER_NAME
    #    GENERATE_ECLIPSEHELP
    #    USE_MATHJAX
    #    MATHJAX_RELPATH
    #    MATHJAX_CODEFILE
    #    GENERATE_RTF
    #    RTF_OUTPUT
    #    RTF_STYLESHEET_FILE
    #    RTF_EXTENSIONS_FILE
    #    GENERATE_MAN
    #    MAN_OUPUT
    #    MAN_EXTENSION
    #    GENERATE_XML
    #    XML_OUTPUT
    #    XML_SCHEMA
    #    XML_DTD
    #    GENERATE_DOCBOOK
    #    DOCBOOK_OUTPUT
    #    GENERATE_AUTOGEN_DEF
    #    GENERATE_PERLMOD
    #    TAGFILES
    #    GENERATE_TAGFILE

    # Debug print statments used while developing
    #print "*** Doxygen Scanner ***"
    #print "*** list of source files ***"
    #for s in source:
    #     print str(s)
    #print "\n\n"any
    return source

def _doxygen_emitter(target, source, env):
    # work around its Safe to use the Doxygen Builder by only specifying the
    # config file as the source however SCons auto generates what it thinks is
    # the proper target.  The auto generated target is wrong if the config
    # file is specified as the source and no target is specified. This will
    # remove the source config file as a target. And prevents circular
    # dependencies
    for t in target:
        for s in source:
            if s == t:
                target.remove(s)

    config_file_path = '.'
    config = ConfigParser.SafeConfigParser()
    try:
        fp = open(str(source[0]), 'r')
        config.readfp(FakeSecHead(fp))
    finally:
        fp.close()

    config_file_path = os.path.abspath(os.path.dirname(str(source[0])))

    generate_html = True
    generate_latex = True

    output_directory = config_file_path
    html_output = 'html'
    latex_output = 'latex'

    if config.has_option('doxy_config', 'output_directory'):
        output_directory_from_config = config.get('doxy_config', 'output_directory')
        output_directory_from_config = _doxygen_expand_environment_vars(output_directory_from_config, env)
        output_directory_from_config = _doxygen_replace_start_hash_if_found(output_directory_from_config, env)
        # relative paths are relative to the location of the config file
        if not os.path.isabs(output_directory_from_config):
            output_directory_from_config = os.path.join(config_file_path, output_directory_from_config)
        if os.path.isdir(output_directory_from_config):
            output_directory = output_directory_from_config

    # Some versions of doxygen will fail to create the output directory if it does
    # not already exist. And will report that the directory does not exist and
    # cannot be created.  This will by pass this error by creating the dictionary
    # for doxygen
    if not os.path.exists(output_directory):
        output_directory_from_config = config.get('doxy_config', 'output_directory')
        output_directory_from_config = _doxygen_expand_environment_vars(output_directory_from_config, env)
        env.Mkdir(output_directory_from_config)

    ############################################################################
    # PROCESS HTML TAGS
    ############################################################################
    if config.has_option('doxy_config', 'generate_html'):
        if config.get('doxy_config', 'generate_html') == 'NO':
            generate_html = False

    if generate_html:
        if config.has_option('doxy_config', 'html_output'):
            html_output_from_config = config.get('doxy_config', 'html_output')
            html_output_from_config = _doxygen_expand_environment_vars(html_output_from_config, env)
            html_output_from_config = _doxygen_replace_start_hash_if_found(html_output_from_config, env)
            if html_output_from_config:
                html_output = html_output_from_config
            # SCons considers directory nodes upto date if they exist.  Past the
            # fist run of the script the directory exist preventing it from
            # generating a dependency graph on subsiquent runs. By adding a file
            # to the targets we are able to workaround this issue. Since index.html
            # is always updated with the build date it was chosen.
            target.append(env.File(os.path.abspath(os.path.join(output_directory, html_output + '/index.html'))))
            env.Clean(source, env.Dir(os.path.abspath(os.path.join(output_directory, html_output))))

    ############################################################################
    # PROCESS LATEX / PDF input
    ############################################################################
    if config.has_option('doxy_config', 'generate_latex'):
        if config.get('doxy_config', 'generate_latex') == 'NO':
            generate_latex = False

    if generate_latex:
        if config.has_option('doxy_config', 'latex_output'):
            latex_output_from_config = config.get('doxy_config', 'latex_output')
            latex_output_from_config = _doxygen_expand_environment_vars(latex_output_from_config, env)
            latex_output_from_config = _doxygen_replace_start_hash_if_found(latex_output_from_config, env)
            if latex_output_from_config:
                latex_output = latex_output_from_config
            # SCons considers directory nodes upto date if they exist.  Past the
            # fist run of the script the directory exist preventing it from
            # generating a dependency graph on subsiquent runs. By adding a file
            # to the targets we are able to workaround this issue. Since refman.tex
            # is always updated with the build date it was chosen.
            target.append(env.File(os.path.abspath(os.path.join(output_directory, latex_output + '/refman.tex'))))
            env.Clean(source, env.Dir(os.path.abspath(os.path.join(output_directory, latex_output))))

    # Debug print statments used while developing
    #print "*** Doxygen Emiter***"
    #print "*** list of source files ***" 
    #for s in source:
    #     print str(s)
    #print "\n\n"
    #print "*** list of target files ***" 
    #for t in target:
    #     print str(t)
    #print "\n\n"
    return target, source

def generate(env):
    # Add Builders for the Doxygen documentation tool
    import SCons.Builder
    doxygen_scanner = env.Scanner(name = 'doxygen_scanner',
                                  function = _doxygen_scanner)

    doxygen_builder = SCons.Builder.Builder(
        action = '${DOXYGENCOM}',
        emitter = _doxygen_emitter,
        source_scanner = doxygen_scanner,
        single_source = 1
    )

    env.Append(BUILDERS = {
        'Doxygen': doxygen_builder,
    })

    env.AppendUnique(
        DOXYGEN = 'doxygen',
        DOXYGENFLAGS = '',
        DOXYGENCOM = 'cd ${SOURCE.dir} && ${DOXYGEN} ${DOXYGENFLAGS} ${SOURCE.file}'
    )
    
    # SystemDrive environment variable is used by doxygen on some systems to write
    # cach files.  If the OS defines the enviroment variable 'SystemDrive' make
    # sure it is imported into the scons environment.
    if os.environ.has_key('SystemDrive'):
        env.PrependENVPath('SystemDrive', os.path.normpath(os.environ['SystemDrive']))

def exists(env):
    """
    Make sure doxygen exists.
    """
    return env.Detect("doxygen")
