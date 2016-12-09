#!/usr/bin/python

# Copyright (c) 2016 Open Connectivity Foundation (OCF) and AllJoyn Open
#    Source Project (AJOSP) Contributors and others.
#
#    SPDX-License-Identifier: Apache-2.0
#
#    All rights reserved. This program and the accompanying materials are
#    made available under the terms of the Apache License, Version 2.0
#    which accompanies this distribution, and is available at
#    http://www.apache.org/licenses/LICENSE-2.0
#
#    Copyright 2016 Open Connectivity Foundation and Contributors to
#    AllSeen Alliance. All rights reserved.
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
        static char code[22];
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