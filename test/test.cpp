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
#include "../manager.hpp"
#include "../config.h"
#include <cassert>
#include <iostream>
#include <algorithm>

constexpr auto SERVICE = "phosphor.inventory.test";
constexpr auto INTERFACE = IFACE;
constexpr auto ROOT = "/testing/inventory";

auto server_thread(void *data)
{
    auto mgr = static_cast<phosphor::inventory::manager::Manager*>(data);

    mgr->run();

    return static_cast<void *>(nullptr);
}

/** @class SignalQueue
 *  @brief Store DBus signals in a queue.
 */
class SignalQueue
{
    public:
    ~SignalQueue() = default;
    SignalQueue() = delete;
    SignalQueue(const SignalQueue &) = delete;
    SignalQueue(SignalQueue &&) = default;
    SignalQueue& operator=(const SignalQueue &) = delete;
    SignalQueue& operator=(SignalQueue &&) = default;
    explicit SignalQueue(const std::string &match) :
        _bus(sdbusplus::bus::new_default()),
        _match(_bus, match.c_str(), &callback, this),
        _next(nullptr)
    {
    }

    auto &&pop(unsigned timeout=1000000)
    {
        while(timeout > 0 && !_next)
        {
            _bus.process_discard();
            _bus.wait(50000);
            timeout -= 50000;
        }
        return std::move(_next);
    }

    private:
    static int callback(sd_bus_message *m, void *context, sd_bus_error *)
    {
        auto *me = static_cast<SignalQueue *>(context);
        sd_bus_message_ref(m);
        sdbusplus::message::message msg{m};
        me->_next = std::move(msg);
        return 0;
    }

    sdbusplus::bus::bus _bus;
    sdbusplus::server::match::match _match;
    sdbusplus::message::message _next;
};

template <typename ...T>
using Object = std::map<
        std::string,
        std::map<
            std::string,
            sdbusplus::message::variant<T...>>>;

using ObjectPath = std::string;

/**@brief Find a subset of interfaces and properties in an object. */
template <typename ...T>
auto hasProperties(const Object<T...> &l, const Object<T...> &r)
{
    Object<T...> result;
    std::set_difference(
            r.cbegin(),
            r.cend(),
            l.cbegin(),
            l.cend(),
            std::inserter(result, result.end()));
    return result.empty();
}

void runTests(phosphor::inventory::manager::Manager &mgr)
{
    const std::string root{ROOT};
    auto b = sdbusplus::bus::new_default();
    auto notify = [&]()
    {
        return b.new_method_call(
                SERVICE,
                ROOT,
                INTERFACE,
                "Notify");
    };

    Object<std::string> obj{
        {
            "xyz.openbmc_project.Example.Iface1",
            {{"ExampleProperty1", "test1"}}
        },
        {
            "xyz.openbmc_project.Example.Iface2",
            {{"ExampleProperty2", "test2"}}
        },
    };

    // Make sure the notify method works.
    {
        ObjectPath relPath{"/foo"};
        ObjectPath path{root + relPath};

        SignalQueue queue(
                "path='" + root + "',member='InterfacesAdded'");

        auto m = notify();
        m.append(relPath);
        m.append(obj);
        b.call(m);

        auto sig{queue.pop()};
        assert(sig);
        ObjectPath signalPath;
        Object<std::string> signalObject;
        sig.read(signalPath);
        assert(path == signalPath);
        sig.read(signalObject);
        assert(hasProperties(signalObject, obj));
        auto moreSignals{queue.pop()};
        assert(!moreSignals);
    }

    mgr.shutdown();
    std::cout << "Success!" << std::endl;
}

int main()
{
    auto mgr = phosphor::inventory::manager::Manager(
            sdbusplus::bus::new_default(),
            SERVICE, ROOT, INTERFACE);

    pthread_t t;
    {
        pthread_create(&t, nullptr, server_thread, &mgr);
    }

    runTests(mgr);

    // Wait for server thread to exit.
    pthread_join(t, nullptr);

    return 0;
}

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
