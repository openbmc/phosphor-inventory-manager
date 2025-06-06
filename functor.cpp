/**
 * Copyright © 2017 IBM Corporation
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

#include "functor.hpp"

#include "manager.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>

namespace phosphor
{
namespace inventory
{
namespace manager
{
namespace functor
{
bool PropertyConditionBase::operator()(
    sdbusplus::bus_t& bus, sdbusplus::message_t&, Manager& mgr) const
{
    std::string path(_path);
    return (*this)(path, bus, mgr);
}

bool PropertyConditionBase::operator()(
    const std::string& path, sdbusplus::bus_t& bus, Manager& mgr) const
{
    std::string host;

    if (_service)
    {
        host.assign(_service);
    }
    else
    {
        auto mapperCall = bus.new_method_call(
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetObject");
        mapperCall.append(path);
        mapperCall.append(std::vector<std::string>({_iface}));

        std::map<std::string, std::vector<std::string>> mapperResponse;
        try
        {
            auto mapperResponseMsg = bus.call(mapperCall);
            mapperResponseMsg.read(mapperResponse);
        }
        catch (const std::exception& e)
        {
            lg2::error("Failed to execute GetObject method: {ERROR}", "ERROR",
                       e);
            return false;
        }

        if (mapperResponse.empty())
        {
            return false;
        }

        host = mapperResponse.begin()->first;
    }

    // When the host service name is inventory manager, eval using
    // a given `getProperty` function to retrieve a property value from
    // an interface hosted by inventory manager.
    if (host == BUSNAME)
    {
        try
        {
            return eval(mgr);
        }
        catch (const std::exception& e)
        {
            // Unable to find property on inventory manager,
            // default condition to false.
            return false;
        }
    }

    auto hostCall = bus.new_method_call(
        host.c_str(), path.c_str(), "org.freedesktop.DBus.Properties", "Get");
    hostCall.append(_iface);
    hostCall.append(_property);

    try
    {
        auto hostResponseMsg = bus.call(hostCall);
        return eval(hostResponseMsg);
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed to execute Get method: {ERROR}", "ERROR", e);
        return false;
    }
}

} // namespace functor
} // namespace manager
} // namespace inventory
} // namespace phosphor
