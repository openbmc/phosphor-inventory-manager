/**
 * Copyright © 2016 IBM Corporation
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
#include <exception>
#include <cstring>
#include "filters.hpp"

namespace phosphor
{
namespace inventory
{
namespace manager
{
namespace filters
{
namespace details
{
namespace property_changed
{

bool Base::operator()(sdbusplus::message::message &_msg, Manager &) const
{
    const char *property;
    const char *iface;
    auto m = _msg.release();
    sdbusplus::message::message msg{m};

    // Look for the property under test in the message.  When
    // found, forward to the template instance to do the actual
    // comparison.
    msg.read(iface);

    if(strcmp(iface, _iface))
        return false;

    auto r = sd_bus_message_enter_container(m, 0, nullptr);
    if(r < 0)
        throw std::runtime_error(strerror(-r));

    while(!sd_bus_message_at_end(m, false)) {
        r = sd_bus_message_enter_container(m, 0, nullptr);
        if(r < 0)
            throw std::runtime_error(strerror(-r));

        msg.read(property);

        if(!strcmp(property, _property))
            return checkValue(msg);

        r = sd_bus_message_skip(m, "v");
        if(r < 0)
            throw std::runtime_error(strerror(-r));

        r = sd_bus_message_exit_container(m);
        if(r < 0)
            throw std::runtime_error(strerror(-r));
    }

    return false;
}

} // namespace property_changed
} // namespace details
} // namespace filters
} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
