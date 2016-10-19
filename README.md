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
