Phosphor Inventory Manager (PIM) is an implementation of the
xyz.openbmc_project.Inventory.Manager DBus interface, and supporting tools.
PIM uses a combination of build-time YAML files and run-time calls to the
Notify method of the Manager interface to provide a generalized inventory
state management solution.

## YAML
PIM includes a YAML parser (pimgen.py).  For PIM to do anything useful, a
set of YAML files must be provided externally that tell it what to do.
An example can be found in the examples directory.

The following top level YAML tags are supported:

* description - An optional description of the file.
* events - One or more events that PIM should monitor.

----
**events**
Supported event tags are:

* name - A globally unique event name.
* type - The event type.  Supported types are: *match*.

Subsequent tags are defined by the event type.

----
**match**
Supported match tags are:

* signature - A DBus match specification.
* filter - A filter to apply when a match occurs.

----
**filter**
Supported filter tags are:

* name - The name of the filter.
* args - An optional list of arguments to pass to the filter.
* value - The argument value.
* type - The argument type (defaults to string if unspecified).

The available filters provided by PIM are:

* none - A non-filter.

----

## Building
After running pimgen.py, build PIM using the following steps:

```
    ./bootstrap.sh
    ./configure ${CONFIGURE_FLAGS}
    make
```

To clean the repository run:

```
 ./bootstrap.sh clean
```
