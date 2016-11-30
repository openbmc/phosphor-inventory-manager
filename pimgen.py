#!/usr/bin/env python

import sys
import os
import argparse
import subprocess
import yaml
import mako.lookup
import sdbusplus.property
from sdbusplus.namedelement import NamedElement
from sdbusplus.renderer import Renderer


class Interface(list):
    def __init__(self, iface):
        super(Interface, self).__init__(iface.split('.'))

    def namespace(self):
        return '::'.join(['sdbusplus'] + self[:-1] + ['server', self[-1]])

    def header(self):
        return os.sep.join(self + ['server.hpp'])

    def __str__(self):
        return '.'.join(self)


class Argument(sdbusplus.property.Property):
    def __init__(self, **kw):
        self.value = kw.pop('value')
        super(Argument, self).__init__(**kw)

    def cppArg(self):
        if self.typeName == 'string':
            return '"%s"' % self.value

        return self.value


class MethodCall(NamedElement, Renderer):
    def __init__(self, **kw):
        self.namespace = kw.pop('namespace', [])
        self.pointer = kw.pop('pointer', False)
        self.args = \
            [Argument(**x) for x in kw.pop('args', [])]
        super(MethodCall, self).__init__(**kw)

    def bare_method(self):
        return '::'.join(self.namespace + [self.name])


class Filter(MethodCall):
    def __init__(self, **kw):
        kw['namespace'] = ['filters']
        super(Filter, self).__init__(**kw)


class Action(MethodCall):
    def __init__(self, **kw):
        kw['namespace'] = ['actions']
        super(Action, self).__init__(**kw)


class DbusSignature(NamedElement, Renderer):
    def __init__(self, **kw):
        self.sig = {x: y for x, y in kw.iteritems()}
        kw.clear()
        super(DbusSignature, self).__init__(**kw)


class DestroyObject(Action):
    def __init__(self, **kw):
        mapped = kw.pop('args')
        kw['args'] = [
            {'value': mapped['path'], 'type':'string'},
        ]
        super(DestroyObject, self).__init__(**kw)


class NoopAction(Action):
    def __init__(self, **kw):
        kw['pointer'] = True
        super(NoopAction, self).__init__(**kw)


class NoopFilter(Filter):
    def __init__(self, **kw):
        kw['pointer'] = True
        super(NoopFilter, self).__init__(**kw)


class PropertyChanged(Filter):
    def __init__(self, **kw):
        mapped = kw.pop('args')
        kw['args'] = [
            {'value': mapped['interface'], 'type':'string'},
            {'value': mapped['property'], 'type':'string'},
            mapped['value']
        ]
        super(PropertyChanged, self).__init__(**kw)


class Event(NamedElement, Renderer):
    action_map = {
        'noop': NoopAction,
        'destroyObject': DestroyObject,
    }

    def __init__(self, **kw):
        self.cls = kw.pop('type')
        self.actions = \
            [self.action_map[x['name']](**x)
                for x in kw.pop('actions', [{'name': 'noop'}])]
        super(Event, self).__init__(**kw)


class MatchEvent(Event):
    filter_map = {
        'none': NoopFilter,
        'propertyChangedTo': PropertyChanged,
    }

    def __init__(self, **kw):
        self.signatures = \
            [DbusSignature(**x) for x in kw.pop('signatures', [])]
        self.filters = \
            [self.filter_map[x['name']](**x)
                for x in kw.pop('filters', [{'name': 'none'}])]
        super(MatchEvent, self).__init__(**kw)


class Everything(Renderer):
    class_map = {
        'match': MatchEvent,
    }

    @staticmethod
    def load(args):
        # Invoke sdbus++ to generate any extra interface bindings for
        # extra interfaces that aren't defined externally.
        yaml_files = []
        extra_ifaces_dir = os.path.join(args.inputdir, 'extra_interfaces.d')
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

        # Aggregate all the event YAML in the events.d directory
        # into a single list of events.

        events_dir = os.path.join(args.inputdir, 'events.d')
        yaml_files = filter(
            lambda x: x.endswith('.yaml'),
            os.listdir(events_dir))

        events = []
        for x in yaml_files:
            with open(os.path.join(events_dir, x), 'r') as fd:
                for e in yaml.load(fd.read()).get('events', {}):
                    events.append(e)

        return Everything(
            *events,
            interfaces=Everything.get_interfaces(args))

    @staticmethod
    def get_interfaces(args):
        '''Aggregate all the interface YAML in the interfaces.d
        directory into a single list of interfaces.'''

        interfaces_dir = os.path.join(args.inputdir, 'interfaces.d')
        yaml_files = filter(
            lambda x: x.endswith('.yaml'),
            os.listdir(interfaces_dir))

        interfaces = []
        for x in yaml_files:
            with open(os.path.join(interfaces_dir, x), 'r') as fd:
                for i in yaml.load(fd.read()):
                    interfaces.append(i)

        return interfaces

    def __init__(self, *a, **kw):
        self.interfaces = \
            [Interface(x) for x in kw.pop('interfaces', [])]
        self.events = [
            self.class_map[x['type']](**x) for x in a]
        super(Everything, self).__init__(**kw)

    def list_interfaces(self, *a):
        print ' '.join([str(i) for i in self.interfaces])

    def generate_cpp(self, loader):
        '''Render the template with the provided events and interfaces.'''
        with open(os.path.join(
                args.outputdir,
                'generated.cpp'), 'w') as fd:
            fd.write(
                self.render(
                    loader,
                    'generated.mako.cpp',
                    events=self.events,
                    interfaces=self.interfaces))


if __name__ == '__main__':
    script_dir = os.path.dirname(os.path.realpath(__file__))
    valid_commands = {
        'generate-cpp': 'generate_cpp',
        'list-interfaces': 'list_interfaces'
    }

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
    parser.add_argument(
        'command', metavar='COMMAND', type=str,
        choices=valid_commands.keys(),
        help='Command to run.')

    args = parser.parse_args()

    if sys.version_info < (3, 0):
        lookup = mako.lookup.TemplateLookup(
            directories=[script_dir],
            disable_unicode=True)
    else:
        lookup = mako.lookup.TemplateLookup(
            directories=[script_dir])

    function = getattr(
        Everything.load(args),
        valid_commands[args.command])
    function(lookup)


# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
