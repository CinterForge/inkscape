#!/usr/bin/env python3

from xml.dom import minidom
import sys


sys.stdout.write("char * stringlst = [")

for rawdoc in sys.argv[1:]:
    doc = minidom.parse(rawdoc)

    for title in doc.getElementsByTagName('title'):
        ret = ""
        for child in title.childNodes:
            if child.nodeType == child.TEXT_NODE:
                ret += child.data
        if ret:
            ret = ret.replace("\n", "").replace("\"", "'")
            sys.stdout.write("N_(\"" + ret + "\"),")

    for filter in doc.getElementsByTagName('pattern'):
        stockid = filter.getAttribute('inkscape:stockid')
        if stockid == "":
            stockid = filter.getAttribute('inkscape:label')
        if stockid != "":
            sys.stdout.write("N_(\"" + stockid + "\"),")

sys.stdout.write("];")
