# Phosphor Inventory Manager (PIM)

Phosphor Inventory Manager (PIM) is an implementation of the
`xyz.openbmc_project.Inventory.Manager` DBus interface, and supporting tools.
PIM uses a combination of build-time YAML files, run-time calls to the Notify
method of the Manager interface, and association definition JSON files to
provide a generalized inventory state management solution.

## YAML

PIM includes a YAML parser (pimgen.py). For PIM to do anything useful, a set of
YAML files must be provided externally that tell it what to do. Examples can be
found in the examples directory.

The following top level YAML tags are supported:

- description - An optional description of the file.
- events - One or more events that PIM should monitor.

### events

Supported event tags are:

- name - A globally unique event name.
- description - An optional description of the event.
- type - The event type. Supported types are: _match_ and _startup_.
- actions - The responses to the event.

Subsequent tags are defined by the event type.

### match

Supported match tags are:

- signatures - A DBus match specification.
- filters - Filters to apply when a match occurs.

### startup

Supported startup tags are:

- filters - Filters to apply at startup.

### filters

Supported filter tags are:

- name - The filter to use.

Subsequent tags are defined by the filter type.

The available filters provided by PIM are:

- propertyChangedTo - Only match events when the specified property has the
  specified value.
- propertyIs - Only match events when the specified property has the specified
  value.

### propertyChangedTo

The property under test is obtained from an sdbus message generated from an
org.freedesktop.DBus.Properties.PropertiesChanged signal payload.

Supported arguments for the propertyChangedTo filter are:

- interface - The interface hosting the property to be checked.
- property - The property to check.
- value - The value to check.

### propertyIs

The property under test is obtained by invoking org.freedesktop.Properties.Get
on the specified interface.

Supported arguments for the propertyIs filter are:

- path - The object hosting the property to be checked.
- interface - The interface hosting the property to be checked.
- property - The property to check.
- value - The value to check.
- service - An optional DBus service name.

The service argument is optional. If provided that service will be called
explicitly. If omitted, the service will be obtained with an
`xyz.openbmc_project.ObjectMapper` lookup.

propertyIs can be used in an action condition context when the action operates
on a dbus object path.

### actions

Supported action tags are:

- name - The action to perform.

Subsequent tags are defined by the action type.

The available actions provided by PIM are:

- destroyObject - Destroy the specified DBus object.
- setProperty - Set the specified property on the specified DBus object.

### destroyObject

Supported arguments for the destroyObject action are:

- paths - The paths of the objects to remove from DBus.
- conditions - An array of conditions.

Conditions are tested and logically ANDed. If the conditions do not pass, the
object is not destroyed. Any condition that accepts a path parameter is
supported.

### setProperty

Supported arguments for the setProperty action are:

- interface - The interface hosting the property to be set.
- property - The property to set.
- paths - The objects hosting the property to be set.
- value - The value to set.
- conditions - An array of conditions.

Conditions are tested and logically ANDed. If the conditions do not pass, the
property is not set. Any condition that accepts a path parameter is supported.

### createObjects

Supported arguments for the createObjects action are:

- objs - A dictionary of objects to create.

## Creating Associations

PIM can create [associations][1] between inventory items and other D-Bus
objects.

This functionality is optional and is controlled with the
`--enable-associations` configure option. It defaults to disabled.

To use this, the associations to create should be defined in a JSON file which
is specified by the `ASSOCIATIONS_FILE_PATH` configure variable, which defaults
to `/usr/share/phosphor-inventory-manager/associations.json`. This file is
processed at runtime.

An example of this JSON is:

```json
[
  {
    "path": "system/chassis/motherboard/cpu0/core1",
    "endpoints": [
      {
        "types": {
          "fType": "sensors",
          "rType": "inventory"
        },
        "paths": ["/xyz/openbmc_project/sensors/temperature/p0_core0_temp"]
      }
    ]
  }
]
```

Then, when/if PIM creates the
`xyz/openbmc_project/system/chassis/motherboard/cpu0/core1` inventory object, it
will add an `xyz.openbmc_project.Association.Definitions` interface on it such
that the object mapper creates the 2 association objects:

```text
    /xyz/openbmc_project/inventory/system/chassis/motherboard/cpu0/core1/sensors
       endpoints property:
       ['/xyz/openbmc_project/sensors/temperature/p0_core0_temp']

    /xyz/openbmc_project/sensors/temperature/p0_core0_temp/inventory
       endpoints property:
       ['/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu0/core1']
```

The JSON description is:

```json
[
    {
        "path": "The relative path of the inventory object to create the
                 xyz.openbmc_project.Association.Definitions interface on."
        "endpoints":
        [
            {
                "types":
                {
                    "fType": "The forward association type."
                    "rType": "The reverse association type."
                },
                "paths":
                [
                    "The list of association endpoints for this inventory path
                     and association type."
                ]
            }
        ]
    }
]
```

In the case where different systems that require different associations reside
in the same flash image, multiple JSON files must be used. These files must be
in the same directory as the default associations file would go, but it does not
matter what they are named as long as the name ends in '.json'. Each file then
contains a 'condition' entry that specifies an inventory path, interface,
property, and list of values. If the actual value of that property is in the
list of values, then the condition is met and those associations are activated.

If a file with a conditions section is found, then the default associations file
is ignored. The end result is that associations are only ever loaded from one
file, either the default file if there aren't any files with conditions in them,
or the first file that had a condition that matched.

An example is:

```json
{
  "condition": {
    "path": "system/chassis/motherboard",
    "interface": "xyz.openbmc_project.Inventory.Decorator.Asset",
    "property": "Model",
    "values": ["ModelA", "ModelB"]
  },
  "associations": [
    {
      "path": "system/chassis/motherboard/cpu0/core1",
      "endpoints": [
        {
          "types": { "fType": "sensors", "rType": "inventory" },
          "paths": ["/xyz/openbmc_project/sensors/temperature/p0_core0_temp"]
        }
      ]
    }
  ]
}
```

This states that these associations are valid if the system/chassis/motherboard
inventory object has a Model property with a value of either ModelA or ModelB.

The values field supports the same types as in the inventory, so either a `bool`
(true/false), `int64_t`, `std::string`, or `std::vector<uint8_t>`([1, 2]).

## Building

After running pimgen.py, build PIM using the following steps:

```sh
meson setup builddir
ninja -C builddir
```

[1]:
  https://github.com/openbmc/docs/blob/master/architecture/object-mapper.md#associations
