#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# UI Policy consistancy
#
# Author: Martin Owens <doctormo@geek-2.com>
# Licensed under GPL version 2 or any later version, read the file "COPYING" for more information.

import fnmatch
import os
import sys

from glob import glob
from copy import deepcopy
from collections import defaultdict

from lxml import etree

UI_PATH = os.path.join(os.path.dirname(__file__), '..', 'share', 'ui')

class Glade:
    """Open and parse a glade/ui file"""
    def __init__(self, filename):
        self.filename = filename
        self.fn = os.path.basename(filename)
        self.objects = []
        self.chain = []

    def parse(self):
        """Parse the document into checkable concerns"""
        self.parse_child(etree.parse(self.filename).getroot())

    def parse_child(self, elem):
        template = {'class': None, 'properties': {}, 'id': "NOID"}
        if elem.tag == "object":
            name = elem.attrib['class']
            self.chain.append(deepcopy(template))
            self.chain[-1]['class'] = elem.attrib['class']
            self.chain[-1]['id'] = elem.attrib.get('id', None)
            self.objects.append(self.chain[-1])
        elif elem.tag == "property" and self.chain and self.chain[-1]:
            name = elem.attrib['name']
            if name in self.chain[-1]['properties']:
                self.chain[-1]['error'] = f"Duplicate property '{name}'"
            name = name.replace('-', '_')
            self.chain[-1]['properties'][name] = elem.text
            self.chain.append(None)
        else:
            self.chain.append(None)

        for child in elem.getchildren():
            self.parse_child(child)

        self.chain.pop()


class PolicyChecker:
    name = None
    search = None
    ignore = []
    errors = {
        'parse': ("Parser Error", "Found something unusual in the XML"),
    }

    def __init__(self):
        self._e = defaultdict(list)
        self.policies = [f for n, f in type(self).__dict__.items() if n.startswith("policy_")]

    def check(self):
        for file in glob(os.path.join(UI_PATH, self.search)):
            ui = Glade(file)
            if ui.fn in self.ignore:
                continue
            ui.parse()

            for obj in ui.objects:
                if 'error' in obj:
                    self.report(ui, 'parse', obj, obj['error'])
                self.repair_id(obj)
                for f in self.policies:
                    for err in f(self, obj['class'], **obj['properties']):
                        self.violation(ui, err, obj, f.__code__.co_varnames)
        return self.print_report()

    def repair_id(self, obj):
        if not obj['id']:
            obj['id'] = obj['properties'].get('action-name', None)
            if obj['id'] and 'action-target' in obj['properties']:
                obj['id'] = f"{obj['id']}{{{obj['properties']['action-target']}}}"

    def print_report(self):
        ret = 0
        sys.stderr.write(f"\n\n==== CHECKING {self.name} FILES ====\n\n")
        for code, instances in self._e.items():
            name, desc = self.errors[code]
            sys.stderr.write(f"\n == {name} ==\n\n  {desc}\n\n")
            for _id, props in instances:
                sys.stderr.write(f" * {_id}: {props}\n")
                ret += 1
        return ret

    def report(self, ui, key, obj, msg):
        self._e[key].append((f"{ui.fn}: {obj['class']}:{obj['id']}", msg))

    def violation(self, ui, key, obj, props):
        self.report(ui, key, obj, " ".join([p + "=" + obj['properties'].get(p, 'N/A') for p in props if p not in ('self', 'cls', 'props')]))
        

class PolicyCheckerToolbars(PolicyChecker):
    name = "TOOLBAR"
    search = "toolbar-*.ui"
    ignore = ['toolbar-tool-prefs.ui']
    errors = {
        'button-focus1': ("Button Takes Focus", "A toolbar button can have focus and will take that focus when clicked. Add focus-on-click=False to fix this."),
        'button-focus2': ("Button Refuses Focus", "A toolbar button is refusing focus, which makes it inaccessable to keyboard navigation. Remove focusable=False"),
        'entry-focus': ("Entry Refuses Focus", "A toolbar entry doesn't allow itself to be in focus, stopping text from being entered. Change focusable to True and focus-on-click to True (or remove them)"),
}

    def policy_01_entries(self, cls, focusable="True", focus_on_click="True", **props):
        # Policy 1. All Buttons should have focusable: False
        if cls in ("GtkButton", "GtkMenuButton", "GtkToggleButton", "GtkRadioButton"):
            if focusable == "False":
                yield 'button-focus2'
            elif focus_on_click != "False":
                yield 'button-focus1'

    def policy_02_entries(self, cls, focusable="True", focus_on_click="True", **props):
        # Policy 2. All Entries, SpinButtons should have focusable: True
        if cls in ("GtkEntry", "GtkSpinButton", "GtkComboBoxText"):
            if focus_on_click == "False" or focusable == "False":
                yield 'entry-focus'



if __name__ == '__main__':
    errors = 0
    errors += PolicyCheckerToolbars().check()

    if errors:
        sys.exit(-5)
    else:
        sys.stderr.write("COMPLETE, NO PROBLEMS FOUND\n")

# vi:sw=4:expandtab:
