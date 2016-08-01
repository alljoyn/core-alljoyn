#!/usr/bin/python

#    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
#    Source Project (AJOSP) Contributors and others.
#
#    SPDX-License-Identifier: Apache-2.0
#
#    All rights reserved. This program and the accompanying materials are
#    made available under the terms of the Apache License, Version 2.0
#    which accompanies this distribution, and is available at
#    http://www.apache.org/licenses/LICENSE-2.0
#
#    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
#    Alliance. All rights reserved.
#
#    Permission to use, copy, modify, and/or distribute this software for
#    any purpose with or without fee is hereby granted, provided that the
#    above copyright notice and this permission notice appear in all
#    copies.
#
#     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
#     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
#     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
#     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
#     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
#     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
#     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
#     PERFORMANCE OF THIS SOFTWARE.
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