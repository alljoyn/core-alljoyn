#!/usr/bin/python

# Copyright (c) 2010 - 2012, 2014 AllSeen Alliance. All rights reserved.
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

import sys
import os
import getopt
import copy
from xml.dom import minidom

if sys.version_info[:3] < (2,4,0):
    from sets import Set as set

includeSet = set()

def openFile(name, type):
    try:
        return open(name, type)
    except IOError, e:
        errno, errStr = e
        print "I/O Operation on %s failed" % name
        print "I/O Error(%d): %s" % (errno, errStr)
        raise e


def main(argv=None):
    """
    make_status --header <header_file> --code <code_file> --prefix <prefix> --base <base_dir> 
                [--commenCode<comment_code_file>] [--deps <dep_file>] [--help]
    Where:
      <header_file>       - Output "C" header file
      <code_file>         - Output "C" code
      <comment_code_file> - Output code that maps status to comment
      <prefix>            - Prefix which is unique across all projects (see Makefile XXX_DIR)
      <base_dir>          - Root directory for xi:include directives
      <dep_file>          - Ouput makefile dependency file

    """
    global headerOut
    global codeOut
    global depOut
    global isFirst
    global fileArgs
    global baseDir
    global prefix
    global CommentCodeOut

    headerOut = None
    codeOut = None
    depOut = None
    baseDir = ""
    prefix = ""
    CommentCodeOut = None

    if argv is None:
        argv = sys.argv[1:]

    try:
        opts, fileArgs = getopt.getopt(argv, "h", ["help", "header=", "code=", "dep=", "base=", "prefix=", "commentCode="])
        for o, a in opts:
            if o in ("-h", "--help"):
                print __doc__
                return 0
            if o in ("--header"):
                headerOut = openFile(a, 'w')
            if o in ("--code"):
                codeOut = openFile(a, 'w')
            if o in ("--dep"):
                depOut = openFile(a, 'w')
            if o in ("--base"):
                baseDir = a
            if o in ("--prefix"):
                prefix = a
            if o in ("--commentCode"):
                CommentCodeOut = openFile(a, 'w')

        if None == headerOut or None == codeOut:
            raise Error("Must specify both --header and --code")
            
        isFirst = True
        includeSet.clear()

        writeHeaders()

        for arg in fileArgs:
            ret = parseAndWriteDocument(arg)

        writeFooters()
                                
        if None != headerOut:
            headerOut.close()
        if None != codeOut:
            codeOut.close()
        if None != CommentCodeOut:
            CommentCodeOut.close()
        if None != depOut:
            depOut.close()
    except getopt.error, msg:
        print msg
        print "for help use --help"
        return 1
    except Exception, e:
        print "ERROR: %s" % e
        if None != headerOut:
            os.unlink(headerOut.name)
        if None != codeOut:
            os.unlink(codeOut.name)
        if None != CommentCodeOut:
            os.unlink(CommentCodeOut.name)
        if None != depOut:
            os.unlink(depOut.name)
        return 1
    
    return 0

def writeHeaders():
    global headerOut
    global codeOut
    global depOut
    global fileArgs
    
    if None != depOut:
        depOut.write("%s %s %s:" % (depOut.name, codeOut.name, headerOut.name))
        for arg in fileArgs:
            depOut.write(" \\\n %s" % arg)
    if None != headerOut:
        headerOut.write("""
/**
 * @file
 * This file contains an enumerated list values that QStatus can return
 *
 * Note: This file is generated during the make process.
 */
/******************************************************************************
 * Copyright (c) 2009-2014, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/ 
#ifndef _STATUS_H
#define _STATUS_H
/**
 * This @#define allows for setting of visibility support on relevant platforms
 */
#ifndef AJ_API
#  if defined(QCC_OS_GROUP_POSIX)
#    define AJ_API __attribute__((visibility("default")))
#  else
#    define AJ_API
#  endif
#endif

/** This @#define allows for calling convention redefinition on relevant platforms */
#ifndef AJ_CALL
#  if defined(QCC_OS_GROUP_WINDOWS)
#    define AJ_CALL __stdcall
#  else
#    define AJ_CALL
#  endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enumerated list of values QStatus can return
 */
typedef enum {""")

    if None != codeOut:
        codeOut.write("""
/**
 * @file
 * This file contains an enumerated list values that QStatus can return
 *
 * Note: This file is generated during the make process.
 */

/******************************************************************************
 * Copyright (c) 2009-2014, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include <stdio.h>
#include <Status.h>

#define CASE(_status) case _status: return #_status 
    
""")
        codeOut.write("AJ_API const char* AJ_CALL QCC_%sStatusText(QStatus status)" % prefix)
        codeOut.write("""
{
    switch (status) {
""")

def writeFooters():
    global headerOut
    global codeOut
    global depOut

    if None != depOut:
        depOut.write("\n")
    if None != headerOut:
        headerOut.write("""
} QStatus;

/**
 * Convert a status code to a C string.
 *
 * @c %QCC_StatusText(ER_OK) returns the C string @c "ER_OK"
 *
 * @param status    Status code to be converted.
 *
 * @return  C string representation of the status code.
 */
""")
        headerOut.write("extern AJ_API const char* AJ_CALL QCC_%sStatusText(QStatus status);" % prefix)
        headerOut.write("""

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif
""")
    if None != codeOut:
        codeOut.write("""    default:
        static char code[20];
#ifdef _WIN32
        _snprintf(code, sizeof(code), "<unknown>: 0x%04x", status);
#else
        snprintf(code, sizeof(code), "<unknown>: 0x%04x", status);
#endif
        return code;
    }
}
""")
    
def parseAndWriteDocument(fileName):
    dom = minidom.parse(fileName)
    for child in dom.childNodes:
        if child.localName == 'status_block':
            parseAndWriteStatusBlock(child)
        elif child.localName == 'include' and child.namespaceURI == 'http://www.w3.org/2001/XInclude':
            parseAndWriteInclude(child)
    dom.unlink()

def parseAndWriteStatusBlock(blockNode):
    global headerOut
    global codeOut
    global isFirst
    offset = 0

    for node in blockNode.childNodes:
        if node.localName == 'offset':
            offset = int(node.firstChild.data, 0)
        elif node.localName == 'status':
            if isFirst:
                if None != headerOut:
                    headerOut.write("\n    %s = %s /**< %s */" % (node.getAttribute('name'), node.getAttribute('value'), node.getAttribute('comment')))
                isFirst = False
            else:
                if None != headerOut:
                    headerOut.write(",\n    %s = %s /**< %s */" % (node.getAttribute('name'), node.getAttribute('value'), node.getAttribute('comment')))
            if None != codeOut:
                codeOut.write("        CASE(%s);\n" % (node.getAttribute('name')))
            offset += 1
        elif node.localName == 'include' and node.namespaceURI == 'http://www.w3.org/2001/XInclude':
            parseAndWriteInclude(node)


def parseAndWriteInclude(includeNode):
    global baseDir
    global includeSet

    href = os.path.join(baseDir, includeNode.attributes['href'].nodeValue)
    if href not in includeSet:
        includeSet.add(href)
        if None != depOut:
            depOut.write(" \\\n %s" % href)
        parseAndWriteDocument(href)


if __name__ == "__main__":
    sys.exit(main())

#end
