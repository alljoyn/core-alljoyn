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
#    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
#    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
#    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
#    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
#    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
#    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
#    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
#    PERFORMANCE OF THIS SOFTWARE.
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