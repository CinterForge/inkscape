#! /bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later

description = 'Generate android app icons from an SVG.'

import os, subprocess, argparse

data = (
    ('m', 48),
    ('h', 72),
    ('xh', 96),
    ('xxh', 144),
    ('xxxh', 192)
)

def generate(svg, outputdir, name):
    for sizecode, _ in data:
        try:
            os.mkdir(os.path.join(outputdir, f'mipmap-{sizecode}dpi'))
        except FileExistsError:
            pass

    subprocess.run([
        'inkscape',
        svg,
        '--batch-process',
        '--actions=' + '; '.join([f'export-filename:{os.path.join(outputdir, f'mipmap-{sizecode}dpi', f'{name}.png')}; export-width:{size}; export-do' for sizecode, size in data])
    ], check = True)

def main():
    parser = argparse.ArgumentParser(prog='genicons.py', description=description, formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-o', '--outputdir', default='.', metavar='DIR', help='Directory to place the output')
    parser.add_argument('-n', '--name', default='ic_launcher', metavar='STRING', help='What to call the icon')
    parser.add_argument('svg', help='The input SVG file')
    args = parser.parse_args()
    
    generate(svg=args.svg, outputdir=args.outputdir, name=args.name)

if __name__ == "__main__":
    main()
