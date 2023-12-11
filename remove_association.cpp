/**
 * Copyright © 2021 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "config.h"

#include "remove_association.hpp"

#include <cstdlib>
#include <iostream>

namespace phosphor
{
namespace inventory
{
namespace manager
{

constexpr auto mapperBus = "xyz.openbmc_project.ObjectMapper";
constexpr auto mapperObj = "/xyz/openbmc_project/object_mapper";
constexpr auto mapperIntf = "xyz.openbmc_project.ObjectMapper";
constexpr auto propIntf = "org.freedesktop.DBus.Properties";

using AssociationTuple = std::tuple<std::string, std::string, std::string>;
using AssociationsProperty = std::vector<AssociationTuple>;

DBusSubtree getInventoryAssociations(sdbusplus::bus::bus& bus,
                                     const std::string& objectPath,
                                     int32_t depth)
{
    auto mapperCall =
        bus.new_method_call(mapperBus, mapperObj, mapperIntf,
                            "GetSubTree");

    mapperCall.append(objectPath, depth,
                      std::vector<std::string>(
                          {"xyz.openbmc_project.Association.Definitions"}));

    auto reply = bus.call(mapperCall);
    DBusSubtree response;
    try
    {
        reply.read(response);
    }
    catch (const sdbusplus::exception::exception& e)
    {
        log<level::ERR>("Failed to parse existing callouts subtree message",
                        entry("ERROR=%s", e.what()),
                        entry("REPLY_SIG=%s", reply.get_signature()));
    }

    return response;
}

void getProperty(sdbusplus::bus::bus& bus, const std::string& service,
                 const std::string& objectPath, const std::string& interface,
                 const std::string& property, DBusValue& value)
{
    auto method = bus.new_method_call(service.c_str(), objectPath.c_str(),
                                      propIntf, "Get");
    method.append(interface, property);
    auto reply = bus.call(method);

    reply.read(value);
}

/** @brief get Dbus service
 *
 */
const std::string getService(const std::string& path,
                             const std::string& interface)
{
    constexpr auto mapper_busname = "xyz.openbmc_project.objectmapper";
    constexpr auto mapper_obj_path = "/xyz/openbmc_project/object_mapper";
    constexpr auto mapper_iface = "xyz.openbmc_project.objectmapper";
    using interfacelist = std::vector<std::string>;
    std::map<std::string, std::vector<std::string>> mapperresponse;

    static auto bus = sdbusplus::bus::new_default();
    auto mapper = bus.new_method_call(mapper_busname, mapper_obj_path,
                                      mapper_iface, "getobject");
    mapper.append(path, interfacelist({interface}));

    auto mapperresponsemsg = bus.call(mapper);
    mapperresponsemsg.read(mapperresponse);

    // the value here will be the service name
    return mapperresponse.cbegin()->first;
}

void removeCriticalAssociation(sdbusplus::bus::bus& bus,
                               const std::string& objectPath,
                               const std::string& service)
{
    DBusValue getAssociationValue;

    getProperty(bus, service, objectPath,
                "xyz.openbmc_project.Association.Definitions", "Associations",
                getAssociationValue);

    auto association = std::get<AssociationsProperty>(getAssociationValue);

    AssociationTuple critAssociation{
        "health_rollup", "critical",
        "/xyz/openbmc_project/inventory/system/chassis"};

    auto it = std::find(association.begin(), association.end(),
                        critAssociation);
    if (it != association.end())
    {
        association.erase(it);
        DBusValue setAssociationValue = association;

        auto method = bus.new_method_call(service.c_str(), objectPath.c_str(),
                                          propIntf, "Set");

        method.append("xyz.openbmc_project.Association.Definitions",
                      "Associations", setAssociationValue);
        bus.call(method);
    }
}
} // namespace manager
} // namespace inventory
} // namespace phosphor
