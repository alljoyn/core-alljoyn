#!/usr/bin/python

# Copyright (c) 2010 - 2012,2014 AllSeen Alliance. All rights reserved.
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
    make_status --header <header_file> --code <code_file> --prefix <prefix> --base <base_dir> [--cpp0xnamespace <cpp0x_namespace>] 
                [--cpp0xcode <cpp0x_code_file>] [--cpp0xheader <cpp0x_header_file>] [--commenCode<comment_code_file>] [--deps <dep_file>] [--help]
    Where:
      <header_file>       - Output "C" header file
      <code_file>         - Output "C" code
      <cpp0x_namespace>   - C++x0 namespace
      <cpp0x_header_file> - Output C++x0 header file
      <cpp0x_code_file>   - Output C++x0 code
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
    global CPP0xHeaderOut
    global CPP0xNamespace
    global CPP0xCodeOut
    global CommentCodeOut

    headerOut = None
    codeOut = None
    depOut = None
    baseDir = ""
    prefix = ""
    CPP0xHeaderOut = None
    CPP0xNamespace = None
    CPP0xCodeOut = None
    CommentCodeOut = None

    if argv is None:
        argv = sys.argv[1:]

    try:
        opts, fileArgs = getopt.getopt(argv, "h", ["help", "header=", "code=", "dep=", "base=", "prefix=", "cpp0xnamespace=", "cpp0xcode=", "cpp0xheader=", "commentCode="])
        for o, a in opts:
            if o in ("-h", "--help"):
                print __doc__
                return 0
            if o in ("--header"):
                headerOut = openFile(a, 'w')
            if o in ("--cpp0xheader"):
                CPP0xHeaderOut = openFile(a, 'w')
            if o in ("--code"):
                codeOut = openFile(a, 'w')
            if o in ("--cpp0xcode"):
                CPP0xCodeOut = openFile(a, 'w')
            if o in ("--dep"):
                depOut = openFile(a, 'w')
            if o in ("--base"):
                baseDir = a
            if o in ("--prefix"):
                prefix = a
            if o in ("--cpp0xnamespace"):
                CPP0xNamespace = a
            if o in ("--commentCode"):
                CommentCodeOut = openFile(a, 'w')

        if None == headerOut or None == codeOut:
            raise Error("Must specify both --header and --code")
            
        if (None != CPP0xCodeOut and None == CPP0xHeaderOut) or (None == CPP0xCodeOut and None != CPP0xHeaderOut):
            raise Error("Must specify both --cpp0xheader and --cpp0xcode")
            
        if None != CPP0xCodeOut and None == CPP0xNamespace:
            raise Error("Emiting CPP0x status requires --cpp0xnamespace to be specified")

        isFirst = True
        includeSet.clear()

        writeHeaders()

        for arg in fileArgs:
            ret = parseAndWriteDocument(arg)

        writeFooters()
                                
        if CPP0xHeaderOut != None:
            isFirst = True
            includeSet.clear()

            writeCPP0xHeaders();

            for arg in fileArgs:
                ret = parseAndWriteCPP0xDocument(arg)

            writeCPP0xFooters();
                
        if None != headerOut:
            headerOut.close()
        if None != codeOut:
            codeOut.close()
        if None != CPP0xHeaderOut:
            CPP0xHeaderOut.close()
        if None != CPP0xCodeOut:
            CPP0xCodeOut.close()
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
        if None != CPP0xCodeOut:
            os.unlink(CPP0xCodeOut.name)
        if None != CPP0xHeaderOut:
            os.unlink(CPP0xHeaderOut.name)
        if None != CommentCodeOut:
            os.unlink(CommentCodeOut.name)
        if None != depOut:
            os.unlink(depOut.name)
        return 1
    
    return 0

def writeCPP0xHeaders():
    global CPP0xHeaderOut
    global CPP0xNamespace
    global CPP0xCodeOut
    global fileArgs
    
    if None != CPP0xHeaderOut:
        CPP0xHeaderOut.write("""
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
 
#pragma once

""")

        CPP0xHeaderOut.write("namespace %s {" % CPP0xNamespace)

        CPP0xHeaderOut.write("""

/**
 * Enumerated list of values QStatus can return
 */
public enum class QStatus {""")

    if None != CPP0xCodeOut:
        CPP0xCodeOut.write("""
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

#include <Status.h>
""")

        CPP0xCodeOut.write("""
#define CASE(_status) case _status: return #_status 
    
""")
        CPP0xCodeOut.write("namespace %s {" % CPP0xNamespace)
        CPP0xCodeOut.write("""
    
const char* QCC_%sStatusText(QStatus status)""" % prefix)
    CPP0xCodeOut.write("""
{
    switch (status) {
""")

    if None != CommentCodeOut:
        CommentCodeOut.write("""
/**
 * @file
 * This file maps a QStatus code to the detailed comment information
 *
 *
 * Note: This file is generated during the make process.
 */

/******************************************************************************
 * Copyright (c) 2012-2014, AllSeen Alliance. All rights reserved.
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

#include <unordered_map>
#include <qcc/Mutex.h>
#include <qcc/platform.h>
#include <Status.h>

#if defined(QCC_OS_GROUP_WINRT)
namespace qcc {

static std::unordered_map<uint32_t, const char *> statusMap;
static bool statusMapInited = false;
static qcc::Mutex statusMapLock;

#define TO_TEXT(_status) #_status
#define TO_TEXT_AND_CONCAT(_status, comment) #_status##" : "##comment
void InitStatusMap()
{
""")

def writeCPP0xFooters():
    global CPP0xHeaderOut
    global CPP0xNamespace
    global CPP0xCodeOut

    if None != CPP0xHeaderOut:
        CPP0xHeaderOut.write("""
};
        
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
        CPP0xHeaderOut.write("extern const char* QCC_%sStatusText(QStatus status);" % prefix)
        CPP0xHeaderOut.write("""
        
}
""")
    if None != CPP0xCodeOut:
        CPP0xCodeOut.write("""    default:
        static char code[20];
#ifdef _WIN32
        _snprintf(code, sizeof(code), "<unknown>: 0x%04x", status);
#else
        snprintf(code, sizeof(code), "<unknown>: 0x%04x", status);
#endif
        return code;
    }
}

}
""")
    if None != CommentCodeOut:
        CommentCodeOut.write("""
}

const char* QCC_StatusMessage(uint32_t status)
{
    statusMapLock.Lock();
    if (!statusMapInited) {
        InitStatusMap();
        statusMapInited = true;
    }
    statusMapLock.Unlock();
    return statusMap[status];
}

}
#endif
""")

def parseAndWriteCPP0xDocument(fileName):
    dom = minidom.parse(fileName)
    for child in dom.childNodes:
        if child.localName == 'status_block':
            parseAndWriteCPP0xStatusBlock(child)
        elif child.localName == 'include' and child.namespaceURI == 'http://www.w3.org/2001/XInclude':
            parseAndWriteCPP0xInclude(child)
    dom.unlink()

def parseAndWriteCPP0xStatusBlock(blockNode):
    global CPP0xHeaderOut
    global CPP0xNamespace
    global CPP0xCodeOut
    global isFirst
    offset = 0

    for node in blockNode.childNodes:
        if node.localName == 'offset':
            offset = int(node.firstChild.data, 0)
        elif node.localName == 'status':
            if isFirst:
                if None != CPP0xHeaderOut:
                    CPP0xHeaderOut.write("\n    %s = %s /**< %s */" % (node.getAttribute('name'), node.getAttribute('value'), node.getAttribute('comment')))
                isFirst = False
            else:
                if None != CPP0xHeaderOut:
                    CPP0xHeaderOut.write(",\n    %s = %s /**< %s */" % (node.getAttribute('name'), node.getAttribute('value'), node.getAttribute('comment')))
            if None != CPP0xCodeOut:
                CPP0xCodeOut.write("        CASE(QStatus::%s);\n" % (node.getAttribute('name')))
            if None != CommentCodeOut:
                CommentCodeOut.write("#ifdef NDEBUG\n")
                CommentCodeOut.write("    statusMap[(uint32_t)%s] = TO_TEXT(%s);\n" % (node.getAttribute('name'), node.getAttribute('name')))
                CommentCodeOut.write("#else\n")
                CommentCodeOut.write("    statusMap[(uint32_t)%s] = TO_TEXT_AND_CONCAT(%s, \"%s\");\n" % (node.getAttribute('name'), node.getAttribute('name'), node.getAttribute('comment')))
                CommentCodeOut.write("#endif\n")
            offset += 1
        elif node.localName == 'include' and node.namespaceURI == 'http://www.w3.org/2001/XInclude':
            parseAndWriteCPP0xInclude(node)


def parseAndWriteCPP0xInclude(includeNode):
    global baseDir
    global includeSet

    href = os.path.join(baseDir, includeNode.attributes['href'].nodeValue)
    if href not in includeSet:
        includeSet.add(href)
        parseAndWriteCPP0xDocument(href)
    
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

#ifndef ALLJOYN_DLLExport /* Used for extern C functions. Add __declspec(dllexport) when using MSVC */
#  if defined(_MSC_VER) /* MSVC compiler*/
#    define ALLJOYN_DLLExport __declspec(dllexport)
#  else /* compiler other than MSVC */
#    define ALLJOYN_DLLExport
#  endif /* Compiler type */
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
        codeOut.write("const char* QCC_%sStatusText(QStatus status)" % prefix)
        codeOut.write("""
{
#if defined(NDEBUG)
    static char code[8];
#ifdef _WIN32
    _snprintf(code, sizeof(code), "0x%04x", status);
#else
    snprintf(code, sizeof(code), "0x%04x", status);
#endif
    return code;
#else
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
        headerOut.write("extern ALLJOYN_DLLExport const char* QCC_%sStatusText(QStatus status);" % prefix)
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
#endif
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
