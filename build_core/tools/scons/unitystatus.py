#!/usr/bin/python

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

import sys
import os
import getopt
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
    make_status --code <code_file> --base <base_dir> [--deps <dep_file>] [--help]
    Where:
    <code_file>   - Output "C#" code
    <base_dir>    - Root directory for xi:include directives
    <dep_file>    - Ouput makefile dependency file

    """
    global codeOut
    global depOut
    global isFirst
    global fileArgs
    global baseDir

    codeOut = None
    depOut = None
    isFirst = True
    baseDir = ""

    if argv is None:
        argv = sys.argv[1:]

    try:
        opts, fileArgs = getopt.getopt(argv, "h", ["help", "code=", "dep=", "base="])
        for o, a in opts:
            if o in ("-h", "--help"):
                print __doc__
                return 0
            if o in ("--code"):
                codeOut = openFile(a, 'w')
            if o in ("--dep"):
                depOut = openFile(a, 'w')
            if o in ("--base"):
                baseDir = a

        if None == codeOut:
            raise Error("Must specify --code")
        
        writeHeaders()

        for arg in fileArgs:
            ret = parseDocument(arg)
            
        writeFooters()

        if None != codeOut:
            codeOut.close()
        if None != depOut:
            depOut.close()
    except getopt.error, msg:
        print msg
        print "for help use --help"
        return 1
    except Exception, e:
        print "ERROR: %s" % e
        if None != codeOut:
            os.unlink(codeOut.name)
        if None != depOut:
            os.unlink(depOut.name)
        return 1
    
    return 0

def writeHeaders():
    global codeOut
    global depOut
    global fileArgs

    if None != depOut:
        depOut.write("%s %s %s:" % (depOut.name, codeOut.name))
        for arg in fileArgs:
            depOut.write(" \\\n %s" % arg)
    if None != codeOut:
        codeOut.write("""
/******************************************************************************
 * @file
 * This file contains an enumerated list values that QStatus can return
 *
 * Note: This file is generated during the make process.
 *
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

using System;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.Threading;

namespace AllJoynUnity
{
\tpublic partial class AllJoyn
\t{
\t\t/**
\t\t * Enumerated list of values QStatus can return
\t\t */ 
\t\tpublic class QStatus
\t\t{
\t\t\tprivate QStatus(int x)
\t\t\t{
\t\t\t\tvalue = x;
\t\t\t}

\t\t\t/** 
\t\t\t * Static constructor
\t\t\t * @param x status to set for QStatus object
\t\t\t *
\t\t\t * @return a new QStatus object
\t\t\t */
\t\t\tpublic static implicit operator QStatus(int x)
\t\t\t{
\t\t\t\treturn new QStatus(x);
\t\t\t}

\t\t\t/** 
\t\t\t * Gets the int value of the QStatus object  
\t\t\t *
\t\t\t * @param x QStatus object to check status
\t\t\t * @return the int value of the QStatus object  
\t\t\t */
\t\t\tpublic static implicit operator int(QStatus x)
\t\t\t{
\t\t\t\treturn x.value;
\t\t\t}

\t\t\t/** 
\t\t\t * Shortcut to determine if a QStatus is an OK status
\t\t\t *
\t\t\t * @param x QStatus object to check status
\t\t\t * @return true if the status == OK
\t\t\t */
\t\t\tpublic static bool operator true(QStatus x)
\t\t\t{
\t\t\t\treturn (x == OK);
\t\t\t}

\t\t\t/** 
\t\t\t * Shortcut to determine if a QStatus is not an OK status
\t\t\t *
\t\t\t * @param x QStatus object to check status
\t\t\t * @return true if the status != OK
\t\t\t */
\t\t\tpublic static bool operator false(QStatus x)
\t\t\t{
\t\t\t\treturn (x != OK);
\t\t\t}

\t\t\t/** 
\t\t\t * Compares the status value of two QStatus objects
\t\t\t *
\t\t\t * @param x QStatus object to compare with
\t\t\t * @param y QStatus object to compare against
\t\t\t * @return true if the status values are equal
\t\t\t */
\t\t\tpublic static bool operator ==(QStatus x, QStatus y)
\t\t\t{
\t\t\t\treturn x.value == y.value;
\t\t\t}

\t\t\t/** 
\t\t\t * Compares two QStatus objects
\t\t\t *
\t\t\t * @param o object to compare against this QStatus
\t\t\t * @return true if two QStatus objects are equals
\t\t\t */
\t\t\tpublic override bool Equals(object o) 
\t\t\t{
\t\t\t\ttry
\t\t\t\t{
\t\t\t\t\treturn (this == (QStatus)o);
\t\t\t\t}
\t\t\t\tcatch
\t\t\t\t{
\t\t\t\t\treturn false;
\t\t\t\t}
\t\t\t}

\t\t\t/** 
\t\t\t * Gets the numeric error code
\t\t\t *
\t\t\t * @return the numeric error code
\t\t\t */
\t\t\tpublic override int GetHashCode()
\t\t\t{
\t\t\t\treturn value;
\t\t\t}

\t\t\t/** 
\t\t\t * Gets a string representing the QStatus value
\t\t\t *
\t\t\t * @return a string representing the QStatus value
\t\t\t */
\t\t\tpublic override string ToString()
\t\t\t{
\t\t\t
\t\t\t\treturn Marshal.PtrToStringAnsi(QCC_StatusText(value));
\t\t\t}

\t\t\t/** 
\t\t\t * Gets the string representation of the QStatus value
\t\t\t *
\t\t\t * @param x QStatus object to get value from 
\t\t\t * @return the string representation of the QStatus value
\t\t\t */
\t\t\tpublic static implicit operator string(QStatus x)
\t\t\t{
\t\t\t\treturn x.value.ToString();
\t\t\t}

\t\t\t/** 
\t\t\t * Checks if two QStatus objects are not equal
\t\t\t *
\t\t\t * @param x QStatus object to compare with
\t\t\t * @param y QStatus object to compare against
\t\t\t * @return true if two QStatus objects are not equal
\t\t\t */
\t\t\tpublic static bool operator !=(QStatus x, QStatus y)
\t\t\t{
\t\t\t\treturn x.value != y.value;
\t\t\t}

\t\t\t/** 
\t\t\t * checks if the QStatus object does not equal OK
\t\t\t * 
\t\t\t * @param x QStatus object to compare against
\t\t\t * @return true if the QStatus object does not equal OK
\t\t\t */
\t\t\tpublic static bool operator !(QStatus x)
\t\t\t{
\t\t\t\treturn (x != OK);
\t\t\t}

\t\t\tinternal int value;
""")

def writeFooters():
    global codeOut
    global depOut

    if None != depOut:
        depOut.write("\n")
    if None != codeOut:
        codeOut.write("""
\t\t}
\t\t#region DLL Imports
\t\t[DllImport(DLL_IMPORT_TARGET)]
\t\tprivate extern static IntPtr QCC_StatusText(int status);

\t\t#endregion
\t}
}
""")
    
def parseDocument(fileName):
    dom = minidom.parse(fileName)
    for child in dom.childNodes:
        if child.localName == 'status_block':
            parseStatusBlock(child)
        elif child.localName == 'include' and child.namespaceURI == 'http://www.w3.org/2001/XInclude':
            parseInclude(child)
    dom.unlink()

def parseStatusBlock(blockNode):
    global codeOut
    global isFirst
    offset = 0

    for node in blockNode.childNodes:
        if node.localName == 'offset':
            offset = int(node.firstChild.data, 0)
        elif node.localName == 'status':
            if None != codeOut:
                codeOut.write("\n\t\t\t/// %s" % node.getAttribute('comment'))
                codeOut.write("\n\t\t\tpublic static readonly QStatus %s = new QStatus(%s);" % (node.getAttribute('name')[3:], node.getAttribute('value')))
            offset += 1
        elif node.localName == 'include' and node.namespaceURI == 'http://www.w3.org/2001/XInclude':
            parseInclude(node)


def parseInclude(includeNode):
    global baseDir
    global includeSet

    href = os.path.join(baseDir, includeNode.attributes['href'].nodeValue)
    if href not in includeSet:
        includeSet.add(href)
        if None != depOut:
            depOut.write(" \\\n %s" % href)
        parseDocument(href)

def UnityStatus(source, output, basedir):
    return main(['--base=%s' % basedir,
                 '--code=%s' % output,
                 '%s' % source])

if __name__ == "__main__":
    sys.exit(main())
