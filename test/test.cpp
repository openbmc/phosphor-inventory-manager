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
#include <thread>

constexpr auto SERVICE = "phosphor.inventory.test";
constexpr auto INTERFACE = IFACE;
constexpr auto ROOT = "/testing/inventory";

/** @class SignalQueue
 *  @brief Store DBus signals in a queue.
 */
class SignalQueue
{
    public:
        ~SignalQueue() = default;
        SignalQueue() = delete;
        SignalQueue(const SignalQueue&) = delete;
        SignalQueue(SignalQueue&&) = default;
        SignalQueue& operator=(const SignalQueue&) = delete;
        SignalQueue& operator=(SignalQueue&&) = default;
        explicit SignalQueue(const std::string& match) :
            _bus(sdbusplus::bus::new_default()),
            _match(_bus, match.c_str(), &callback, this),
            _next(nullptr)
        {
        }

        auto&& pop(unsigned timeout = 1000000)
        {
            while (timeout > 0 && !_next)
            {
                _bus.process_discard();
                _bus.wait(50000);
                timeout -= 50000;
            }
            return std::move(_next);
        }

    private:
        static int callback(sd_bus_message* m, void* context, sd_bus_error*)
        {
            auto* me = static_cast<SignalQueue*>(context);
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
using Object = std::map <
               std::string,
               std::map <
               std::string,
               sdbusplus::message::variant<T... >>>;

using ObjectPath = std::string;

/**@brief Find a subset of interfaces and properties in an object. */
template <typename ...T>
auto hasProperties(const Object<T...>& l, const Object<T...>& r)
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

/**@brief Check an object for one or more interfaces. */
template <typename ...T>
auto hasInterfaces(const std::vector<std::string>& l, const Object<T...>& r)
{
    std::vector<std::string> stripped, interfaces;
    std::transform(
        r.cbegin(),
        r.cend(),
        std::back_inserter(stripped),
        [](auto & p)
    {
        return p.first;
    });
    std::set_difference(
        stripped.cbegin(),
        stripped.cend(),
        l.cbegin(),
        l.cend(),
        std::back_inserter(interfaces));
    return interfaces.empty();
}

void runTests(phosphor::inventory::manager::Manager& mgr)
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
    auto set = [&](const std::string & path)
    {
        return b.new_method_call(
                   SERVICE,
                   path.c_str(),
                   "org.freedesktop.DBus.Properties",
                   "Set");
    };

    Object<std::string> obj
    {
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

    // Make sure DBus signals are handled.
    {
        ObjectPath relDeleteMeOne{"/deleteme1"};
        ObjectPath relDeleteMeTwo{"/deleteme2"};
        ObjectPath relTriggerOne{"/trigger1"};
        ObjectPath deleteMeOne{root + relDeleteMeOne};
        ObjectPath deleteMeTwo{root + relDeleteMeTwo};
        ObjectPath triggerOne{root + relTriggerOne};

        // Create some objects to be deleted by an action.
        {
            auto m = notify();
            m.append(relDeleteMeOne);
            m.append(obj);
            b.call(m);
        }
        {
            auto m = notify();
            m.append(relDeleteMeTwo);
            m.append(obj);
            b.call(m);
        }

        // Create the triggering object.
        {
            auto m = notify();
            m.append(relTriggerOne);
            m.append(obj);
            b.call(m);
        }

        // Set a property that should not trigger due to a filter.
        {
            SignalQueue queue(
                "path='" + root + "',member='InterfacesRemoved'");
            auto m = set(triggerOne);
            m.append("xyz.openbmc_project.Example.Iface2");
            m.append("ExampleProperty2");
            m.append(sdbusplus::message::variant<std::string>("abc123"));
            b.call(m);
            auto sig{queue.pop()};
            assert(!sig);
        }

        // Set a property that should trigger.
        {
            SignalQueue queue(
                "path='" + root + "',member='InterfacesRemoved'");

            auto m = set(triggerOne);
            m.append("xyz.openbmc_project.Example.Iface2");
            m.append("ExampleProperty2");
            m.append(sdbusplus::message::variant<std::string>("xxxyyy"));
            b.call(m);

            ObjectPath sigpath;
            std::vector<std::string> interfaces;
            {
                std::vector<std::string> interfaces;
                auto sig{queue.pop()};
                assert(sig);
                sig.read(sigpath);
                assert(sigpath == deleteMeOne);
                sig.read(interfaces);
                std::sort(interfaces.begin(), interfaces.end());
                assert(hasInterfaces(interfaces, obj));
            }
            {
                std::vector<std::string> interfaces;
                auto sig{queue.pop()};
                assert(sig);
                sig.read(sigpath);
                assert(sigpath == deleteMeTwo);
                sig.read(interfaces);
                std::sort(interfaces.begin(), interfaces.end());
                assert(hasInterfaces(interfaces, obj));
            }
            {
                // Make sure there were only two signals.
                auto sig{queue.pop()};
                assert(!sig);
            }
        }
    }

    // Validate the set property action.
    {
        ObjectPath relChangeMe{"/changeme"};
        ObjectPath relTriggerTwo{"/trigger2"};
        ObjectPath changeMe{root + relChangeMe};
        ObjectPath triggerTwo{root + relTriggerTwo};

        // Create an object to be updated by the set property action.
        {
            auto m = notify();
            m.append(relChangeMe);
            m.append(obj);
            b.call(m);
        }

        // Create the triggering object.
        {
            auto m = notify();
            m.append(relTriggerTwo);
            m.append(obj);
            b.call(m);
        }

        // Trigger and validate the change.
        {
            SignalQueue queue(
                "path='" + changeMe + "',member='PropertiesChanged'");
            auto m = set(triggerTwo);
            m.append("xyz.openbmc_project.Example.Iface2");
            m.append("ExampleProperty2");
            m.append(sdbusplus::message::variant<std::string>("yyyxxx"));
            b.call(m);

            std::string sigInterface;
            std::map <
            std::string,
                sdbusplus::message::variant<std::string >> sigProperties;
            {
                std::vector<std::string> interfaces;
                auto sig{queue.pop()};
                sig.read(sigInterface);
                assert(sigInterface == "xyz.openbmc_project.Example.Iface1");
                sig.read(sigProperties);
                assert(sigProperties["ExampleProperty1"] == "changed");
            }
        }
    }

    mgr.shutdown();
    std::cout << "Success!" << std::endl;
}

int main()
{
    phosphor::inventory::manager::Manager mgr(
        sdbusplus::bus::new_default(),
        SERVICE, ROOT, INTERFACE);

    auto f = [](auto mgr)
    {
        mgr->run();
    };
    auto t = std::thread(f, &mgr);
    runTests(mgr);

    // Wait for server thread to exit.
    t.join();

    return 0;
}

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
