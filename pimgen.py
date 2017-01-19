#!/usr/bin/env python

'''Phosphor Inventory Manager YAML parser and code generator.

The parser workflow is broken down as follows:
  1 - Import YAML files as native python type(s) instance(s).
  2 - Create an instance of the Everything class from the
        native python type instance(s) with the Everything.load
        method.
  3 - The Everything class constructor orchestrates conversion of the
        native python type(s) instances(s) to render helper types.
        Each render helper type constructor imports its attributes
        from the native python type(s) instances(s).
  4 - Present the converted YAML to the command processing method
        requested by the script user.
'''

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
    '''Provide various interface transformations.'''

    def __init__(self, iface):
        super(Interface, self).__init__(iface.split('.'))

    def namespace(self):
        '''Represent as an sdbusplus namespace.'''
        return '::'.join(['sdbusplus'] + self[:-1] + ['server', self[-1]])

    def header(self):
        '''Represent as an sdbusplus server binding header.'''
        return os.sep.join(self + ['server.hpp'])

    def __str__(self):
        return '.'.join(self)


class Argument(sdbusplus.property.Property):
    '''Bridge sdbusplus property typenames to syntatically correct c++.'''

    def __init__(self, **kw):
        self.value = kw.pop('value')
        self.cast = kw.pop('cast', None)
        super(Argument, self).__init__(**kw)

    def cppArg(self):
        '''Transform string types to c++ string constants.'''

        a = self.value
        if self.typeName == 'string':
            a = '"%s"' % a

        if self.cast:
            a = 'static_cast<%s>(%s)' % (self.cast, a)

        return a


class MethodCall(NamedElement, Renderer):
    '''Render syntatically correct c++ method calls.'''

    def __init__(self, **kw):
        self.namespace = kw.pop('namespace', [])
        self.template = kw.pop('template', '')
        self.pointer = kw.pop('pointer', False)
        self.args = \
            [Argument(**x) for x in kw.pop('args', [])]
        super(MethodCall, self).__init__(**kw)

    def bare_method(self):
        '''Provide the method name and encompassing
        namespace without any arguments.'''

        m = '::'.join(self.namespace + [self.name])
        if self.template:
            m += '<%s>' % self.template

        return m


class Filter(MethodCall):
    '''Provide common attributes for any filter.'''

    def __init__(self, **kw):
        kw['namespace'] = ['filters']
        super(Filter, self).__init__(**kw)


class Action(MethodCall):
    '''Provide common attributes for any action.'''

    def __init__(self, **kw):
        kw['namespace'] = ['actions']
        super(Action, self).__init__(**kw)


class DbusSignature(NamedElement, Renderer):
    '''Represent a dbus signal match signature.'''

    def __init__(self, **kw):
        self.sig = {x: y for x, y in kw.iteritems()}
        kw.clear()
        super(DbusSignature, self).__init__(**kw)


class DestroyObject(Action):
    '''Render a destroyObject action.'''

    def __init__(self, **kw):
        mapped = kw.pop('args')
        kw['args'] = [
            {'value': mapped['path'], 'type':'string'},
        ]
        super(DestroyObject, self).__init__(**kw)


class SetProperty(Action):
    '''Render a setProperty action.'''

    def __init__(self, **kw):
        mapped = kw.pop('args')
        member = Interface(mapped['interface']).namespace()
        member = '&%s' % '::'.join(
            member.split('::') + [NamedElement(
                name=mapped['property']).camelCase])

        memberType = Argument(**mapped['value']).cppTypeName

        kw['template'] = Interface(mapped['interface']).namespace()
        kw['args'] = [
            {'value': mapped['path'], 'type':'string'},
            {'value': mapped['interface'], 'type':'string'},
            {'value': member, 'cast': '{0} ({1}::*)({0})'.format(
                memberType,
                Interface(mapped['interface']).namespace())},
            mapped['value'],
        ]
        super(SetProperty, self).__init__(**kw)


class NoopAction(Action):
    '''Render a noop action.'''

    def __init__(self, **kw):
        kw['pointer'] = True
        super(NoopAction, self).__init__(**kw)


class NoopFilter(Filter):
    '''Render a noop filter.'''

    def __init__(self, **kw):
        kw['pointer'] = True
        super(NoopFilter, self).__init__(**kw)


class PropertyChanged(Filter):
    '''Render a propertyChanged filter.'''

    def __init__(self, **kw):
        mapped = kw.pop('args')
        kw['args'] = [
            {'value': mapped['interface'], 'type':'string'},
            {'value': mapped['property'], 'type':'string'},
            mapped['value']
        ]
        super(PropertyChanged, self).__init__(**kw)


class Event(NamedElement, Renderer):
    '''Render an inventory manager event.'''

    action_map = {
        'noop': NoopAction,
        'destroyObject': DestroyObject,
        'setProperty': SetProperty,
    }

    def __init__(self, **kw):
        self.cls = kw.pop('type')
        self.actions = \
            [self.action_map[x['name']](**x)
                for x in kw.pop('actions', [{'name': 'noop'}])]
        super(Event, self).__init__(**kw)


class MatchEvent(Event):
    '''Associate one or more dbus signal match signatures with
    a filter.'''

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
    '''Parse/render entry point.'''

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
        help='%s.' % " | ".join(valid_commands.keys()))

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
