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
#include "functor.hpp"

#include <sdbusplus/bus.hpp>

namespace phosphor
{
namespace inventory
{
namespace manager
{
namespace functor
{
bool PropertyConditionBase::operator()(sdbusplus::bus::bus& bus,
                                       sdbusplus::message::message&,
                                       Manager& mgr) const
{
    std::string path(_path);
    return (*this)(path, bus, mgr);
}

bool PropertyConditionBase::operator()(const std::string& path,
                                       sdbusplus::bus::bus& bus, Manager&) const
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

        auto mapperResponseMsg = bus.call(mapperCall);
        if (mapperResponseMsg.is_method_error())
        {
            return false;
        }

        std::map<std::string, std::vector<std::string>> mapperResponse;
        mapperResponseMsg.read(mapperResponse);

        if (mapperResponse.empty())
        {
            return false;
        }

        host = mapperResponse.begin()->first;

        if (host == bus.get_unique_name())
        {
            // TODO I can't call myself here.
            return false;
        }
    }
    auto hostCall = bus.new_method_call(
        host.c_str(), path.c_str(), "org.freedesktop.DBus.Properties", "Get");
    hostCall.append(_iface);
    hostCall.append(_property);

    auto hostResponseMsg = bus.call(hostCall);
    if (hostResponseMsg.is_method_error())
    {
        return false;
    }

    return eval(hostResponseMsg);
}

} // namespace functor
} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
