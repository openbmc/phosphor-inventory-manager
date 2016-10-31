#!/usr/bin/env python

import os
import sys
import yaml
import subprocess


class SDBUSPlus(object):
    def __init__(self, path):
        self.path = path

    def __call__(self, *a, **kw):
        args = [
            os.path.join(self.path,  'sdbus++'),
            '-t',
            os.path.join(self.path, 'templates')
        ]

        subprocess.call(args + list(a), **kw)


if __name__ == '__main__':
    sdbusplus = None
    for p in os.environ.get('PATH', "").split(os.pathsep):
        if os.path.exists(os.path.join(p, 'sdbus++')):
            sdbusplus = SDBUSPlus(p)
            break

    if sdbusplus is None:
        sys.stderr.write('Cannot find sdbus++\n')
        sys.exit(1)

    genfiles = {
        'server-cpp': lambda x: '%s.cpp' % x,
        'server-header': lambda x: os.path.join(
            os.path.join(*x.split('.')), 'server.hpp')
    }
    with open(os.path.join('example', 'interfaces.yaml'), 'r') as fd:
        interfaces = yaml.load(fd.read())

    for i in interfaces:
        for process, f in genfiles.iteritems():

            dest = f(i)
            parent = os.path.dirname(dest)
            if parent and not os.path.exists(parent):
                os.makedirs(parent)

            with open(dest, 'w') as fd:
                sdbusplus(
                    '-r',
                    os.path.join('example', 'interfaces'),
                    'interface',
                    process,
                    i,
                    stdout=fd)

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
