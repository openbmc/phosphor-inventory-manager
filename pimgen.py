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


def cppTypeName(yaml_type):
    ''' Convert yaml types to cpp types.'''
    return sdbusplus.property.Property(type=yaml_type).cppTypeName


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


class Indent(object):
    '''Help templates be depth agnostic.'''

    def __init__(self, depth=0):
        self.depth = depth

    def __add__(self, depth):
        return Indent(self.depth + depth)

    def __call__(self, depth):
        '''Render an indent at the current depth plus depth.'''
        return 4*' '*(depth + self.depth)


class Template(NamedElement):
    '''Associate a template name with its namespace.'''

    def __init__(self, **kw):
        self.namespace = kw.pop('namespace')
        super(Template, self).__init__(**kw)

    def qualified(self):
        return '::'.join(self.namespace + [self.name])


class Quote(object):
    '''Decorate an argument by quoting it.'''

    def __call__(self, arg):
        return '"{0}"'.format(arg)


class Cast(object):
    '''Decorate an argument by casting it.'''

    def __init__(self, cast, target):
        '''cast is the cast type (static, const, etc...).
           target is the cast target type.'''
        self.cast = cast
        self.target = target

    def __call__(self, arg):
        return '{0}_cast<{1}>({2})'.format(self.cast, self.target, arg)


class Literal(object):
    '''Decorate an argument with a literal operator.'''

    literals = {
        'string': 's',
        'int64': 'll',
        'uint64': 'ull'
    }

    def __init__(self, type):
        self.type = type

    def __call__(self, arg):
        literal = self.literals.get(self.type)

        if literal:
            return '{0}{1}'.format(arg, literal)

        return arg


class Argument(NamedElement, Renderer):
    '''Define argument type inteface.'''

    def __init__(self, **kw):
        self.type = kw.pop('type', None)
        super(Argument, self).__init__(**kw)

    def argument(self, loader, indent):
        raise NotImplementedError


class TrivialArgument(Argument):
    '''Non-array type arguments.'''

    def __init__(self, **kw):
        self.value = kw.pop('value')
        self.decorators = kw.pop('decorators', [])
        if kw.get('type', None) == 'string':
            self.decorators.insert(0, Quote())

        super(TrivialArgument, self).__init__(**kw)

    def argument(self, loader, indent):
        a = str(self.value)
        for d in self.decorators:
            a = d(a)

        return a


class InitializerList(Argument):
    '''Initializer list arguments.'''

    def __init__(self, **kw):
        self.values = kw.pop('values')
        super(InitializerList, self).__init__(**kw)

    def argument(self, loader, indent):
        return self.render(
            loader,
            'argument.mako.cpp',
            arg=self,
            indent=indent)


class DbusSignature(Argument):
    '''DBus signature arguments.'''

    def __init__(self, **kw):
        self.sig = {x: y for x, y in kw.iteritems()}
        kw.clear()
        super(DbusSignature, self).__init__(**kw)

    def argument(self, loader, indent):
        return self.render(
            loader,
            'signature.mako.cpp',
            signature=self,
            indent=indent)


class MethodCall(Argument):
    '''Render syntatically correct c++ method calls.'''

    def __init__(self, **kw):
        self.namespace = kw.pop('namespace', [])
        self.templates = kw.pop('templates', [])
        self.args = kw.pop('args', [])
        super(MethodCall, self).__init__(**kw)

    def call(self, loader, indent):
        return self.render(
            loader,
            'method.mako.cpp',
            method=self,
            indent=indent)

    def argument(self, loader, indent):
        return self.call(loader, indent)


class Vector(MethodCall):
    '''Convenience type for vectors.'''

    def __init__(self, **kw):
        kw['name'] = 'vector'
        kw['namespace'] = ['std']
        kw['args'] = [InitializerList(values=kw.pop('args'))]
        super(Vector, self).__init__(**kw)


class Wrapper(MethodCall):
    '''Convenience type for functions that wrap other functions.'''

    def __init__(self, **kw):
        m = MethodCall(
            name=kw.pop('name'),
            namespace=kw.pop('namespace', []),
            templates=kw.pop('templates', []),
            args=kw.pop('args', []))

        kw['name'] = kw.pop('wrapper_name')
        kw['namespace'] = kw.pop('wrapper_namespace', [])
        kw['args'] = [m]
        super(Wrapper, self).__init__(**kw)


class Filter(Wrapper):
    '''Convenience type for filters'''

    def __init__(self, **kw):
        kw['wrapper_name'] = 'make_filter'
        kw['wrapper_namespace'] = ['details']
        kw['namespace'] = ['filters']
        super(Filter, self).__init__(**kw)


class Action(Wrapper):
    '''Convenience type for actions'''

    def __init__(self, **kw):
        kw['wrapper_name'] = 'make_action'
        kw['wrapper_namespace'] = ['details']
        kw['namespace'] = ['actions']
        super(Action, self).__init__(**kw)


class DestroyObjects(Action):
    '''Assemble a destroyObject action.'''

    def __init__(self, **kw):
        values = [{'value': x, 'type': 'string'} for x in kw.pop('paths')]
        args = [InitializerList(
            values=[TrivialArgument(**x) for x in values])]
        kw['args'] = args
        super(DestroyObjects, self).__init__(**kw)


class SetProperty(Action):
    '''Assemble a setProperty action.'''

    def __init__(self, **kw):
        args = []

        value = kw.pop('value')
        prop = kw.pop('property')
        iface = kw.pop('interface')
        iface = Interface(iface)
        namespace = iface.namespace().split('::')[:-1]
        name = iface[-1]
        t = Template(namespace=namespace, name=iface[-1])

        member = '&%s' % '::'.join(
            namespace + [name, NamedElement(name=prop).camelCase])
        member_type = cppTypeName(value['type'])
        member_cast = '{0} ({1}::*)({0})'.format(member_type, t.qualified())

        args.append(TrivialArgument(value=kw.pop('path'), type='string'))
        args.append(TrivialArgument(value=str(iface), type='string'))
        args.append(TrivialArgument(
            value=member, decorators=[Cast('static', member_cast)]))
        args.append(TrivialArgument(**value))

        kw['templates'] = [Template(name=name, namespace=namespace)]
        kw['args'] = args
        super(SetProperty, self).__init__(**kw)


class PropertyChanged(Filter):
    '''Assemble a propertyChanged filter.'''

    def __init__(self, **kw):
        args = []
        args.append(TrivialArgument(value=kw.pop('interface'), type='string'))
        args.append(TrivialArgument(value=kw.pop('property'), type='string'))
        args.append(TrivialArgument(
            decorators=[
                Literal(kw['value'].get('type', None))], **kw.pop('value')))
        kw['args'] = args
        super(PropertyChanged, self).__init__(**kw)


class Event(MethodCall):
    '''Assemble an inventory manager event.'''

    action_map = {
        'destroyObjects': DestroyObjects,
        'setProperty': SetProperty,
    }

    filter_map = {
        'propertyChangedTo': PropertyChanged,
    }

    def __init__(self, **kw):
        self.summary = kw.pop('name')

        filters = [
            self.filter_map[x['name']](**x) for x in kw.pop('filters', [])]

        event = MethodCall(
            name='make_shared',
            namespace=['std'],
            templates=[Template(
                name=kw.pop('event'),
                namespace=kw.pop('event_namespace', []))],
            args=kw.pop('event_args', []) + [filters[0]])

        events = Vector(
            templates=[Template(name='EventBasePtr', namespace=['details'])],
            args=[event])

        action_type = Template(name='ActionBasePtr', namespace=['details'])
        action_args = [
            self.action_map[x['name']](**x) for x in kw.pop('actions', [])]
        actions = Vector(
            templates=[action_type],
            args=action_args)

        kw['name'] = 'make_tuple'
        kw['namespace'] = ['std']
        kw['args'] = [events, actions]
        super(Event, self).__init__(**kw)


class MatchEvent(Event):
    '''Associate one or more dbus signal match signatures with
    a filter.'''

    def __init__(self, **kw):
        kw['event'] = 'DbusSignal'
        kw['event_namespace'] = ['details']
        kw['event_args'] = [
            DbusSignature(**x) for x in kw.pop('signatures', [])]

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
                for e in yaml.safe_load(fd.read()).get('events', {}):
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
                for i in yaml.safe_load(fd.read()):
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
                    interfaces=self.interfaces,
                    indent=Indent()))


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
