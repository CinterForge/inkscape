from lxml import etree

def d_cmp(orig, new):
    """ Compares the original d attribute to the new one. """
    orig_list = orig.split()
    new_list = new.split()

    if len(orig_list) != len(new_list):
        return False

    # Normalize the final 'z' to uppercase:
    orig_list[-1] = orig_list[-1].upper()
    new_list[-1] = new_list[-1].upper()

    for (o, n) in zip(orig_list, new_list):
        if o == n:
            continue
        numeric = "{:.0f}".format(float(n))
        if o != numeric:
            return False

    return True

document = etree.parse("regression-1364_output.svg")
layer = document.find('{http://www.w3.org/2000/svg}g[@id="layer1"]')
boolop_result = layer.find('{http://www.w3.org/2000/svg}path[@id="small"]')

assert boolop_result.attrib.get("transform") == "scale(2)"

result_d = boolop_result.attrib.get("d")
assert d_cmp("M 0 0 L 0 50 A 50 50 0 0 0 50 0 L 0 0 z", result_d), result_d

