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
#include <iostream>
#include <exception>
#include <chrono>
#include "manager.hpp"

using namespace std::literals::chrono_literals;

namespace phosphor
{
namespace inventory
{
namespace manager
{
namespace details
{

/** @brief Fowrarding signal callback.
 *
 *  Extracts per-signal specific context and forwards the call to the manager
 *  instance.
 */
auto _signal(sd_bus_message* m, void* data, sd_bus_error* e) noexcept
{
    try
    {
        auto msg = sdbusplus::message::message(m);
        auto& args = *static_cast<Manager::SigArg*>(data);
        sd_bus_message_ref(m);
        auto& mgr = *std::get<0>(args);
        mgr.handleEvent(
            msg,
            static_cast<const details::DbusSignal&>(
                *std::get<1>(args)),
            *std::get<2>(args));
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}

} // namespace details

Manager::Manager(
    sdbusplus::bus::bus&& bus,
    const char* busname,
    const char* root,
    const char* iface) :
    details::ServerObject<details::ManagerIface>(bus, root),
    _shutdown(false),
    _root(root),
    _bus(std::move(bus)),
    _manager(sdbusplus::server::manager::manager(_bus, root))
{
    for (auto& group : _events)
    {
        for (auto pEvent : std::get<std::vector<details::EventBasePtr>>(
                 group))
        {
            if (pEvent->type !=
                details::Event::Type::DBUS_SIGNAL)
            {
                continue;
            }

            // Create a callback context for this event group.
            auto dbusEvent = static_cast<details::DbusSignal*>(
                                 pEvent.get());

            // Go ahead and store an iterator pointing at
            // the event data to avoid lookups later since
            // additional signal callbacks aren't added
            // after the manager is constructed.
            _sigargs.emplace_back(
                std::make_unique<SigArg>(
                    this,
                    dbusEvent,
                    &group));

            // Register our callback and the context for
            // each signal event.
            _matches.emplace_back(
                _bus,
                dbusEvent->signature,
                details::_signal,
                _sigargs.back().get());
        }
    }

    _bus.request_name(busname);
}

void Manager::shutdown() noexcept
{
    _shutdown = true;
}

void Manager::run() noexcept
{
    sdbusplus::message::message unusedMsg{nullptr};

    // Run startup events.
    for (auto& group : _events)
    {
        for (auto pEvent : std::get<std::vector<details::EventBasePtr>>(
                 group))
        {
            if (pEvent->type ==
                details::Event::Type::STARTUP)
            {
                handleEvent(unusedMsg, *pEvent, group);
            }
        }
    }

    while (!_shutdown)
    {
        try
        {
            _bus.process_discard();
            _bus.wait((5000000us).count());
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
    }
}

void Manager::notify(sdbusplus::message::object_path path, Object object)
{
    try
    {
        createObjects({std::make_pair(path, object)});
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}

void Manager::handleEvent(
    sdbusplus::message::message& msg,
    const details::Event& event,
    const EventInfo& info)
{
    auto& actions = std::get<1>(info);

    for (auto& f : event)
    {
        if (!(*f)(_bus, msg, *this))
        {
            return;
        }
    }
    for (auto& action : actions)
    {
        (*action)(_bus, *this);
    }
}

void Manager::destroyObjects(
    const std::vector<const char*>& paths)
{
    for (const auto& path : paths)
    {
        std::string p{path};
        _refs.erase(_root + p);
    }
}

void Manager::createObjects(
    const std::map<sdbusplus::message::object_path, Object>& objs)
{
    std::string absPath;

    for (auto& o : objs)
    {
        auto& relPath = o.first;
        auto& ifaces = o.second;

        absPath.assign(_root);
        absPath.append(relPath);

        auto obj = _refs.find(absPath);
        if (obj != _refs.end())
        {
            // This object already exists...skip.
            continue;
        }

        // Create an interface holder for each interface
        // provided by the client and group them into
        // a container.
        InterfaceComposite ref;

        auto i = ifaces.size();
        for (auto& iface : ifaces)
        {
            auto& props = iface.second;

            // Defer sending any signals until the last interface.
            auto deferSignals = --i != 0;
            auto pMakers = _makers.find(iface.first.c_str());

            if (pMakers == _makers.end())
            {
                // This interface is not known.
                continue;
            }

            auto& maker = std::get<MakerType>(pMakers->second);

            ref.emplace(
                iface.first,
                maker(
                    _bus,
                    absPath.c_str(),
                    props,
                    deferSignals));
        }

        if (!ref.empty())
        {
            // Hang on to a reference to the object (interfaces)
            // so it stays on the bus, and so we can make calls
            // to it if needed.
            _refs.emplace(
                absPath, std::move(ref));
        }
    }
}

details::holder::Base& Manager::getInterfaceHolder(
    const char* path, const char* interface)
{
    return const_cast<const Manager*>(
               this)->getInterfaceHolder(path, interface);
}

details::holder::Base& Manager::getInterfaceHolder(
    const char* path, const char* interface) const
{
    std::string p{path};
    auto oit = _refs.find(_root + p);
    if (oit == _refs.end())
        throw std::runtime_error(
            _root + p + " was not found");

    auto& obj = oit->second;
    auto iit = obj.find(interface);
    if (iit == obj.end())
        throw std::runtime_error(
            "interface was not found");

    return *iit->second;
}

} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
