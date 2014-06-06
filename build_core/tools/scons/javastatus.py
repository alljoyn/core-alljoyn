#!/usr/bin/python

# Copyright (c) 2010-2011,2014 AllSeen Alliance. All rights reserved.
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
from xml.sax.saxutils import escape

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
    <code_file>   - Output "Java" code
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
        codeOut.write("""/* This file is auto-generated.  Do not modify. */
/*
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
 */

package org.alljoyn.bus;

/**
 * Standard function return codes for this package.
 */
public enum Status {
""")

def writeFooters():
    global codeOut
    global depOut

    if None != depOut:
        depOut.write("\n")
    if None != codeOut:
        codeOut.write(""";

    /** Error Code */
    private int errorCode;

    /** Constructor */
    private Status(int errorCode) {
        this.errorCode = errorCode;
    }   

    /** Static constructor */
    private static Status create(int errorCode) {
        for (Status s : Status.values()) {
            if (s.getErrorCode() == errorCode) {
                return s;
            }
        }
        return NONE;
    }

    /** 
     * Gets the numeric error code. 
     *
     * @return the numeric error code
     */
    public int getErrorCode() { return errorCode; }
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
            if isFirst:
                if None != codeOut:
                    codeOut.write("\n    /** <b><tt>%s</tt></b> %s. */" % (escape(node.getAttribute('value')), escape(node.getAttribute('comment'))))
                    codeOut.write("\n    %s(%s)" % (node.getAttribute('name')[3:], node.getAttribute('value')))
                isFirst = False
            else:
                if None != codeOut:
                    codeOut.write(",\n    /** <b><tt>%s</tt></b> %s. */" % (escape(node.getAttribute('value')), escape(node.getAttribute('comment'))))
                    codeOut.write("\n    %s(%s)" % (node.getAttribute('name')[3:], node.getAttribute('value')))
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

def JavaStatus(source):
    return main(['--base=%s' % os.path.abspath('..'),
                 '--code=%s.java' % source,
                 '%s.xml' % source])

if __name__ == "__main__":
    sys.exit(main())
