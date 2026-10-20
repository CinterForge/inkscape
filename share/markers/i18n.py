#!/usr/bin/env python3

from xml.dom import minidom
import sys

doc = minidom.parse(sys.argv[1])
markers = doc.getElementsByTagName('marker')

stockids = []
for m in markers:
    stockids.append("N_(\"" + m.getAttribute('inkscape:stockid') + "\")")

sys.stdout.write("const char **stringlst = {\n    " + ",\n    ".join(stockids) + "\n};\n")

