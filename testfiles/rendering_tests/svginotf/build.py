# SPDX-License-Identifier: GPL-2.0-or-later

#  Requirements and build instructions
# setup venv (consult with python docs for non Linux OS):
#
# python -m venv venv
# source ./venv/bin/activate
# pip install fonttools defcon ufo2ft ufoLib2

# glyph indexes/order in config and ids in SVG files are important

import defcon
import fontTools
import ufo2ft
import fontTools.svgLib.path
from fontTools.ttLib import newTable
from pathlib import Path
from fontTools.ttLib.tables.S_V_G_ import SVGDocument

OUTPUT = "svginotf_testfont1.otf"
EM_SIZE = 1000

ufo_font = defcon.Font()
ufo_font.info.unitPerEm = EM_SIZE
ufo_font.info.familyName = "SVGinOTF testfont1"
ufo_font.info.xHeight = 500
ufo_font.info.capHeight = 800
ufo_font.info.ascender = 1000
ufo_font.info.descender = -200

DEFAULT_WIDTH = 500

FALLBACK_BOX = 'M 0,0 L 0,800 800,800 800,0 z'
glyphs = [
    # somewhat normal
    {'unicode': 0x61, 'name': 'a', 'svg': 'glyphs/a.svg'},
    # tall
    {'unicode': 0x62, 'name': 'b',
     'fallback_shape': 'M 0,-1400 L 0,2331 500,2331 500,-1400 z',
     'width': 502,
     'svg': 'glyphs/b.svg'},
    # wide
    {'unicode': 0x63, 'name': 'c',
     'fallback_shape': 'M -1276,0 L -1276,920 1750,920 500,0 z',
     'svg': 'glyphs/c.svg'},
    # viewport offset
    {'unicode': 0x64, 'name': 'd',
     'fallback_shape': 'M 0,0 L 0,800 500,800 500,0 z',
     'svg': 'glyphs/d.svg'},
]

for info in glyphs:
    glyph = ufo_font.newGlyph(info['name'])
    glyph.unicodes = [info['unicode']]
    glyph.width = info.get('width', DEFAULT_WIDTH)
    pen = glyph.getPen()
    non_color_shape = info.get('fallback_shape', FALLBACK_BOX)
    fontTools.svgLib.parse_path(non_color_shape, pen)

otf = ufo2ft.compileOTF(ufo_font)
svg_table = newTable("SVG ")
svg_table.docList = []
glyph_order = otf.getGlyphOrder()
print(glyph_order)
index_map = {name: i for i, name in enumerate(glyph_order)}
for info in glyphs:
    svg_file_path = info.get('svg', None)
    if not svg_file_path:
        continue
    group_range = info.get('group', None)
    svg_text = Path(svg_file_path).read_text()
    gid = index_map[info['name']]
    gid_min = gid
    gid_max = gid

    if group_range:
        gid_min = group_range[0]
        gid_max = group_range[1]

    svg_table.docList.append(SVGDocument(svg_text, gid_min, gid_max, info.get('compressed', True)))
svg_table.docList.sort(key=lambda x: x.startGlyphID)
otf[svg_table.tableTag] = svg_table
otf.save(OUTPUT)
