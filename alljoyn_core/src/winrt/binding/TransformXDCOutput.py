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

#
# This script massages the XDCMake output xml into something the Visual Studio object browser can view. 
#
import sys, codecs, os;
import re;


# dictionary of strings to replace 
d = { 'Platform.String': 'System.String',
      'Platform.Object' : 'System.Object',
      '#ER_': 'QStatus::ER_',
    }

p = re.compile("|".join(re.escape(k) for k in d))
def repl(m):
    return d[m.group(0)]

filename = sys.argv[1]
filenameOut = sys.argv[2]
inFile = codecs.open(filename, "r", "utf-8")
outFile = codecs.open(filenameOut, "w", "utf-8")
for line in inFile:
    newline = p.sub(repl, line)
    
    #Replace some of the more complicated expressions directly
    newline = re.sub('Platform\.Array\&lt\;(.*)\&gt\;', '\\1[]', newline)
    newline = re.sub('Platform\.WriteOnlyArray\&lt\;(.*)\&gt\;', '\\1[]', newline)
    newline = re.sub('\!System\.Runtime\.CompilerServices\.IsConst', '', newline)
    outFile.write(newline)
print sys.argv[0] + ": closing " + filename
inFile.close()

print sys.argv[0] + ": closing " + filenameOut
outFile.close()
