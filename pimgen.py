#!/usr/bin/env python

import sys
import os
import re
import argparse
import yaml

valid_c_name_pattern = re.compile('[\W_]+')
ignore_list = ['description']
all_names = []


def get_parser(x, fmt, lmbda=lambda x: x.capitalize()):
    try:
        return getattr(
            sys.modules[__name__],
            '%s' % (fmt.format(lmbda(x))))
    except AttributeError:
        raise NotImplementedError("Don't know how to parse '%s'" % x)


class RenderList(list):
    def __init__(self, renderers):
        self.extend(renderers)

    def __call__(self, fd):
        for x in self:
            x(fd)


class ParseList(list):
    def __init__(self, parsers):
        self.extend(parsers)

    def __call__(self):
        return RenderList([x() for x in self])


class MatchRender(object):
    def __init__(self, name, signature, fltr, action):
        self.name = valid_c_name_pattern.sub('_', name).lower()
        self.signature = signature
        self.fltr = fltr
        self.action = action

        if self.name in all_names:
            raise RuntimeError('The name "%s" is not unique.' % name)
        else:
            all_names.append(self.name)

    def __call__(self, fd):
        sig = ['%s=\'%s\'' % (k, v) for k, v in self.signature.iteritems()]
        sig = ['%s,' % x for x in sig[:-1]] + [sig[-1]]
        sig = ['"%s"' % x for x in sig]
        sig = ['%s\n' % x for x in sig[:-1]] + [sig[-1]]

        fd.write('    {\n')
        fd.write('        "%s",\n' % self.name)
        fd.write('        std::make_tuple(\n')
        for s in sig:
            fd.write('            %s' % s)
        fd.write(',\n')
        self.fltr(fd)
        fd.write(',\n')
        self.action(fd)
        fd.write('\n')
        fd.write('        ),\n')
        fd.write('    },\n')


class FilterRender(object):
    namespace = 'filters'
    default = 'none'

    def __init__(self, fltr):
        self.args = None
        if fltr is None:
            self.name = self.default
        else:
            self.name = fltr.get('type')
            self.args = fltr.get('args')

    def __call__(self, fd):
        def fmt(x):
            if x.get('type') is None:
                return '"%s"' % x['value']
            return str(x['value'])

        fd.write('            %s::%s' % (self.namespace, self.name))
        if self.args:
            fd.write('(')
            buf = ','.join(([fmt(x) for x in self.args]))
            fd.write(buf)
            fd.write(')')


class ActionRender(FilterRender):
    namespace = 'actions'
    default = 'noop'

    def __init__(self, action):
        FilterRender.__init__(self, action)


class MatchEventParse(object):
    def __init__(self, match):
        self.name = match['name']
        self.signature = match['signature']
        self.fltr = match.get('filter')
        self.action = match.get('action')

    def __call__(self):
        return MatchRender(
            self.name,
            self.signature,
            FilterRender(self.fltr),
            ActionRender(self.action))


class EventsParse(object):
    def __init__(self, event):
        self.delegate = None
        cls = event['type']
        if cls not in ignore_list:
            fmt = '{0}EventParse'
            self.delegate = get_parser(cls, fmt)(event)

    def __call__(self):
        if self.delegate:
            return self.delegate()
        return lambda x: None


class DictParse(ParseList):
    def __init__(self, data):
        fmt = '{0}Parse'
        parse = set(data.iterkeys()).difference(ignore_list)
        ParseList.__init__(
            self, [get_parser(x, fmt)(*data[x]) for x in parse])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Phosphor Inventory Manager (PIM) YAML '
        'scanner and code generator.')
    parser.add_argument(
        '-o', '--output', dest='output',
        default='generated.hpp', help='Output file name.')
    parser.add_argument(
        '-d', '--dir', dest='inputdir',
        default='examples', help='Location of files to process.')

    args = parser.parse_args()

    yaml_files = filter(
        lambda x: x.endswith('.yaml'),
        os.listdir(args.inputdir))

    def get_parsers(x):
        with open(os.path.join(args.inputdir, x), 'r') as fd:
            return DictParse(yaml.load(fd.read()))

    head = """// This file was auto generated.  Do not edit.

#pragma once

const Manager::Events Manager::_events{
"""

    tail = """};
"""

    r = ParseList([get_parsers(x) for x in yaml_files])()
    r.insert(0, lambda x: x.write(head))
    r.append(lambda x: x.write(tail))

    with open(args.output, 'w') as fd:
        r(fd)

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
