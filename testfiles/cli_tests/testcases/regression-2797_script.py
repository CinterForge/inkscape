from lxml import etree

document = etree.parse("regression-2797_output.svg")

parent = document.find('{http://www.w3.org/2000/svg}text[@id="parent"]')
tspan1, = parent.findall('{http://www.w3.org/2000/svg}tspan')
tspan2s = tspan1.findall('{http://www.w3.org/2000/svg}tspan')

# Expect outer tspan added as SVG 1.1 fallback with x/y position.
# Expect no inner tspan with incorrect "font-size:medium".

assert len(tspan2s) == 0

assert parent.attrib.get("style") == "font-size:4px;shape-inside:url(#theshape)"
assert tspan1.attrib.get("style") is None
assert tspan1.attrib.get("x") == "2"
assert tspan1.attrib.get("y") is not None
