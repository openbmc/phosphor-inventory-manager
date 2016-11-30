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
#include "manager.hpp"

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
auto _signal(sd_bus_message *m, void *data, sd_bus_error *e) noexcept
{
    try {
        auto msg = sdbusplus::message::message(m);
        auto &args = *static_cast<Manager::SigArg*>(data);
        sd_bus_message_ref(m);
        auto &mgr = *std::get<0>(args);
        mgr.signal(
                msg,
                static_cast<const details::DbusSignal &>(
                    *std::get<1>(args)),
                *std::get<2>(args));
    }
    catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}

} // namespace details

Manager::Manager(
        sdbusplus::bus::bus &&bus,
        const char *busname,
        const char *root,
        const char *iface) :
    details::ServerObject<details::ManagerIface>(bus, root),
    _shutdown(false),
    _root(root),
    _bus(std::move(bus)),
    _manager(sdbusplus::server::manager::manager(_bus, root))
{
    for (auto &group: _events)
    {
        for (auto pEvent: std::get<0>(group))
        {
            if (pEvent->type !=
                    details::Event::Type::DBUS_SIGNAL)
                continue;

            // Create a callback context for this event group.
            auto dbusEvent = static_cast<details::DbusSignal *>(
                    pEvent.get());

            // Go ahead and store an iterator pointing at
            // the event data to avoid lookups later since
            // additional signal callbacks aren't added
            // after the manager is constructed.
            _sigargs.emplace_back(
                    std::make_unique<SigArg>(
                        std::make_tuple(
                            this,
                            dbusEvent,
                            &group)));

            // Register our callback and the context for
            // each signal event.
            _matches.emplace_back(
                    sdbusplus::server::match::match(
                        _bus,
                        std::get<0>(*dbusEvent),
                        details::_signal,
                        _sigargs.back().get()));
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
    while(!_shutdown) {
        try {
            _bus.process_discard();
            _bus.wait(5000000);
        }
        catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
        }
    }
}

void Manager::notify(std::string path, Object object)
{
    try {
        if(object.cbegin() == object.cend())
            throw std::runtime_error(
                    "No interfaces in " + path);

        path.insert(0, _root);

        auto obj = _refs.find(path);
        if(obj != _refs.end())
            throw std::runtime_error(
                    obj->first + " already exists");

        // Create an interface holder for each interface
        // provided by the client and group them into
        // a container.
        InterfaceComposite ref;

        for (auto &x: object) {
            auto maker = _makers.find(x.first.c_str());

            if(maker == _makers.end())
                throw std::runtime_error(
                        "Unimplemented interface: " + x.first);

            ref.emplace(
                    std::make_pair(
                        x.first,
                        (maker->second)(_bus, path.c_str())));
        }

        // Hang on to a reference to the object (interfaces)
        // so it stays on the bus, and so we can make calls
        // to it if needed.
        _refs.emplace(
                std::make_pair(
                    path, std::move(ref)));
    }
    catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}

void Manager::signal(
        sdbusplus::message::message &msg,
        const details::DbusSignal &event,
        const EventInfo &info)
{
    auto &filter = *std::get<1>(event);
    auto &actions = std::get<1>(info);

    if(filter(msg, *this)) {
        for (auto &action: actions)
            (*action)(*this);
    }
}

void Manager::destroyObject(const char *path)
{
    std::string p{path};
    _refs.erase(_root + p);
}

details::holder::Base& Manager::getInterfaceHolder(
        const char *path, const char *interface)
{
    return const_cast<const Manager *>(
            this)->getInterfaceHolder(path, interface);
}

details::holder::Base& Manager::getInterfaceHolder(
        const char *path, const char *interface) const
{
    std::string p{path};
    auto oit = _refs.find(_root + p);
    if(oit == _refs.end())
        throw std::runtime_error(
                _root + p + " was not found");

    auto &obj = oit->second;
    auto iit = obj.find(interface);
    if(iit == obj.end())
        throw std::runtime_error(
                "interface was not found");

    return *iit->second;
}

} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
