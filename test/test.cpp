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
#include "manager.hpp"
#include "../config.h"
#include <cassert>

constexpr auto SERVICE = "phosphor.inventory.test";
constexpr auto INTERFACE = IFACE;
constexpr auto ROOT = "/testing/inventory";

auto server_thread(void *data)
{
    auto mgr = static_cast<phosphor::inventory::manager::Manager*>(data);

    mgr->run();

    return static_cast<void *>(nullptr);
}

void runTests(phosphor::inventory::manager::Manager &mgr)
{
    auto b = sdbusplus::bus::new_default();

    // make sure the notify method works
    {
        auto m = b.new_method_call(SERVICE, ROOT, INTERFACE, "Notify");
        m.append("/foo");

        using var = sdbusplus::message::variant<std::string>;
        using inner = std::map<std::string, var>;
        using outer = std::map<std::string, inner>;

        inner i = {{"ExampleProperty1", "test"}};
        outer o = {{"xyz.openbmc_project.Example.Iface1", i}};

        m.append(o);
        auto reply = b.call(m);
        auto cleanup = sdbusplus::message::message(reply);
        assert(sd_bus_message_get_errno(reply) == 0);
    }

    mgr.shutdown();
}

int main()
{
    auto mgr = phosphor::inventory::manager::Manager(
            sdbusplus::bus::new_system(),
            SERVICE, ROOT, INTERFACE);

    pthread_t t;
    {
        pthread_create(&t, NULL, server_thread, &mgr);
    }

    runTests(mgr);

    // Wait for server thread to exit.
    pthread_join(t, NULL);

    return 0;
}

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
