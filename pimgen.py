#!/usr/bin/env python

import sys
import os
import re
import argparse
import yaml
import subprocess
from mako.template import Template

valid_c_name_pattern = re.compile('[\W_]+')


def parse_event(e):
    e['name'] = valid_c_name_pattern.sub('_', e['name']).lower()
    if e.get('filter') is None:
        e.setdefault('filter', {}).setdefault('type', 'none')
    if e.get('action') is None:
        e.setdefault('action', {}).setdefault('type', 'noop')
    return e


if __name__ == '__main__':
    script_dir = os.path.dirname(os.path.realpath(__file__))

    parser = argparse.ArgumentParser(
        description='Phosphor Inventory Manager (PIM) YAML '
        'scanner and code generator.')
    parser.add_argument(
        '-o', '--output-dir', dest='outputdir',
        default='.', help='Output directory.')
    parser.add_argument(
        '-d', '--dir', dest='inputdir',
        default=os.path.join(script_dir, 'example'),
        help='Location of files to process.')

    args = parser.parse_args()

    events_dir = os.path.join(args.inputdir, 'events')
    yaml_files = filter(
        lambda x: x.endswith('.yaml'),
        os.listdir(events_dir))

    events = []
    for x in yaml_files:
        with open(os.path.join(events_dir, x), 'r') as fd:
            for e in yaml.load(fd.read()).get('events', {}):
                events.append(parse_event(e))

    template = os.path.join(script_dir, 'generated.mako.cpp')
    t = Template(filename=template)
    with open(os.path.join(args.inputdir, 'interfaces.yaml'), 'r') as fd:
        interfaces = yaml.load(fd.read())

    with open(os.path.join(args.outputdir, 'generated.cpp'), 'w') as fd:
        fd.write(
            t.render(
                interfaces=interfaces,
                events=events))

    yaml_files = []
    extra_ifaces_dir = os.path.join(args.inputdir, 'interfaces')
    if os.path.exists(extra_ifaces_dir):
        for directory, _, files in os.walk(extra_ifaces_dir):
            if not files:
                continue

            yaml_files += map(
                lambda f: os.path.relpath(
                    os.path.join(directory, f),
                    extra_ifaces_dir),
                filter(lambda f: f.endswith('.interface.yaml'), files))

    genfiles = {
        'server-cpp': lambda x: '%s.cpp' % (
            x.replace(os.sep, '.')),
        'server-header': lambda x: os.path.join(
            os.path.join(
                *x.split('.')), 'server.hpp')
    }

    for i in yaml_files:
        iface = i.replace('.interface.yaml', '').replace(os.sep, '.')
        for process, f in genfiles.iteritems():

            dest = os.path.join(args.outputdir, f(iface))
            parent = os.path.dirname(dest)
            if parent and not os.path.exists(parent):
                os.makedirs(parent)

            with open(dest, 'w') as fd:
                subprocess.call([
                    'sdbus++',
                    '-r',
                    extra_ifaces_dir,
                    'interface',
                    process,
                    iface],
                    stdout=fd)

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
