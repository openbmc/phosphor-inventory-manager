#!/usr/bin/env python

import os
import sys
import yaml
import subprocess


if __name__ == '__main__':
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
                subprocess.call([
                    'sdbus++',
                    '-r',
                    os.path.join('example', 'interfaces'),
                    'interface',
                    process,
                    i],
                    stdout=fd)

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
