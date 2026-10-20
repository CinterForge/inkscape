from lxml import etree

document = etree.parse("regression-2602_output.svg")

parent = document.find('{http://www.w3.org/2000/svg}g[@id="parent"]')
paths = parent.findall('{http://www.w3.org/2000/svg}path')

assert parent.attrib.get("style") == "fill:#0000ff"
assert paths[0].attrib.get("style") == "fill:#ff0000"
assert paths[1].attrib.get("style") is None
