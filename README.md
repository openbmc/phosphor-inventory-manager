Phosphor Inventory Manager (PIM) is an implementation of the
xyz.openbmc_project.Inventory.Manager DBus interface, and supporting tools.
PIM uses a combination of build-time YAML files and run-time calls to the
Notify method of the Manager interface to provide a generalized inventory
state management solution.

## YAML
PIM includes a YAML parser (pimgen.py).  For PIM to do anything useful, a
set of YAML files must be provided externally that tell it what to do.
Examples can be found in the examples directory.

The following top level YAML tags are supported:

* description - An optional description of the file.
* events - One or more events that PIM should monitor.

----
**events**

Supported event tags are:

* name - A globally unique event name.
* description - An optional description of the event.
* type - The event type.  Supported types are: *match*.
* actions - The responses to the event.

Subsequent tags are defined by the event type.

-----
**match**

Supported match tags are:

* signatures - A DBus match specification.
* filters - Filters to apply when a match occurs.

----
**filters**

Supported filter tags are:

* name - The filter to use.
* args - An optional list of arguments to pass to the filter.

The available filters provided by PIM are:

* none - A non-filter.
* propertyChangedTo - Only match events when the specified property has
the specified value.

----
**propertyChangedTo**

Supported arguments for the propertyChangedTo filter are:
* interface - The interface hosting the property to be checked.
* property - The property to check.
* value - The value to check.

---
**actions**

Supported action tags are:

* name - The action to perform.
* args - An optional list of arguments to pass to the action.

The available actions provided by PIM are:

* noop - A non-action.
* destroyObject - Destroy the specified DBus object.
* setProperty - Set the specified property on the specified DBus object.

----
**destroyObject**

Supported arguments for the destroyObject action are:
* path - The path of the object to remove from DBus.

----
**setProperty**

Supported arguments for the setProperty action are:
* interface - The interface hosting the property to be set.
* property - The property to set.
* path - The object hosting the property to be set.
* value - The value to set.


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
