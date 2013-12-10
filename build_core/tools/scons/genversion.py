# Copyright (c) 2010 - 2013, AllSeen Alliance. All rights reserved.
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

import getpass
import platform
import os
import re
from subprocess import *

ver_re = re.compile('int\s+(?P<REL>architecture|apiLevel|release)\s*=\s*(?P<VAL>\d+)\s*;\s*$')
prod_re = re.compile('char(\s+const)?\s*(\sproduct\[\]|\*\s*product)\s*=\s*"(?P<STR>.*)"\s*;\s*$')

def GetBuildInfo(env, source):
    branches = []
    tags = []
    if env.has_key('GIT'):
        try:
            branches = Popen([env['GIT'], 'branch'], stdout = PIPE, stderr = PIPE, cwd = source).communicate()[0].splitlines()
            tags = Popen([env['GIT'], 'describe', '--always', '--long', '--abbrev=40'], stdout = PIPE, stderr = PIPE, cwd = source).communicate()[0].splitlines()
        except WindowsError as e:
            if e[0] == 2:
                try:
                    branches = Popen([env['GIT'] + '.cmd', 'branch'], stdout = PIPE, stderr = PIPE, cwd = source).communicate()[0].splitlines()
                    tags = Popen([env['GIT'] + '.cmd', 'describe', '--always', '--long', '--abbrev=40'], stdout = PIPE, stderr = PIPE, cwd = source).communicate()[0].splitlines()
                except:
                    pass
        except:
            pass

    branch = None
    for b in branches:
        if b[0] == '*':
            branch = b[2:]
            break

    tag = None
    commit_delta = None
    commit_hash = None
    if tags:
        if tags[0].find('-') >= 0:
            tag, commit_delta, commit_hash = tuple(tags[0].rsplit('-',2))
            commit_hash = commit_hash[1:]  # lop off the "g"
        else:
            tag = '<none>'
            commit_delta = 0;
            commit_hash = tags[0]

    if branch or commit_hash:
        bld_string = 'Git'
    else:
        bld_string = ''
    if branch:
        bld_string += " branch: '%s'" % branch
    if commit_hash:
        bld_string += " tag: '%s'" % tag
        if commit_delta:
            bld_string += ' (+%s changes)' % commit_delta
        if commit_delta or tag == '<none>':
            bld_string += ' commit ref: %s' % commit_hash

    return bld_string


def ParseSource(source):
    prod = '<none>'
    arch = 0
    api = 0
    rel = 0
    f = open(source, 'r')
    lines = f.readlines()
    f.close();

    for l in lines:
        m = ver_re.search(l)
        if m:
            d = m.groupdict()
            if d['REL'] == 'architecture':
                arch = int(d['VAL'])
            elif d['REL'] == 'apiLevel':
                api = int(d['VAL'])
            elif d['REL'] == 'release':
                rel = int(d['VAL'])
        else:
            m = prod_re.search(l)
            if m:
                prod = m.groupdict()['STR']
    return (prod, arch, api, rel, lines)


def GenVersionAction(source, target, env):
    import time
    product, architecture, api_level, release, lines = ParseSource(str(source[0]))
    fpath = os.path.abspath(os.path.dirname(str(source[0])))
    bld_info = GetBuildInfo(env, fpath)
    date = time.strftime('%a %b %d %H:%M:%S UTC %Y', time.gmtime())
    version_str = 'v%(arch)d.%(api)d.%(rel)d' % ({ 'arch': architecture,
                                                   'api': api_level,
                                                   'rel': release })
    build_str = '%(prod)s %(ver)s (Built %(date)s by %(user)s%(bld)s)' % ({
        'prod': product,
        'ver': version_str,
        'date': date,
        'user': getpass.getuser(),
        'bld': (bld_info and ' - ' + bld_info) or '' })

    f = open(str(target[0]), 'w')
    f.write('/* This file is auto-generated.  Do not modify. */\n')
    for l in lines:
        if l.find('##VERSION_STRING##') >= 0:
            f.write(l.replace('##VERSION_STRING##', version_str))
        elif l.find('##BUILD_STRING##') >= 0:
            f.write(l.replace('##BUILD_STRING##', build_str))
        else:
            f.write(l)
    f.close()


def generate(env):
    import SCons.Builder
    builders = [ SCons.Builder.Builder(action = GenVersionAction,
                                       suffix = '.cc',
                                       src_suffix = '.cc.in'),
                 SCons.Builder.Builder(action = GenVersionAction,
                                       suffix = '.c',
                                       src_suffix = '.c.in') ]
    env.Append(BUILDERS = { 'GenVersion' : builders[0] })
    if env.Detect('git'):
        env.AppendUnique(GIT = 'git')

def exists(env):
    return true
