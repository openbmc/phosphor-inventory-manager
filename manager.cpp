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

auto _signal(sd_bus_message *m, void *data, sd_bus_error *e) noexcept
{
    try {
        auto msg = sdbusplus::message::message(m);
        auto &args = *static_cast<Manager::SigArg*>(data);
        sd_bus_message_ref(m);
        auto &mgr = *std::get<0>(args);
        mgr.signal(msg, *std::get<1>(args));
    }
    catch (std::exception &e) {
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
    sdbusplus::server::xyz::openbmc_project::Inventory::Manager(bus, root),
    _shutdown(false),
    _root(root),
    _bus(std::move(bus)),
    _manager(sdbusplus::server::manager::manager(_bus, root))
{
    for (auto &x: _events) {
        _sigargs.emplace_back(std::make_tuple(
                    this,
                    &x.second,
                    sdbusplus::server::match::match(
                        _bus,
                        x.second.signature,
                        details::_signal,
                        &_sigargs.back())));
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
        catch (std::exception &e) {
            std::cerr << e.what() << std::endl;
        }
    }
}

void Manager::notify(std::string path, Object object)
{
    try {
        using MakeType = std::unique_ptr<details::Interface>(*)(
                sdbusplus::bus::bus &, const char *);
        using Makers = std::map<std::string, MakeType>;

        if(object.cbegin() == object.cend())
            throw std::runtime_error(
                    "No interfaces in " + path);

        static const Makers makers{
            // TODO - Add mappings here.
        };

        path.insert(0, _root);

        auto obj = _refs.find(path);
        if(obj != _refs.end())
            throw std::runtime_error(
                    obj->first + " already exists");

        InterfaceComposite ref;

        for (auto &x: object) {
            auto maker = makers.find(x.first.c_str());

            if(maker == makers.end())
                throw std::runtime_error(
                        "Unimplemented interface: " + x.first);

            ref.emplace(
                    std::make_pair(
                        x.first,
                        (maker->second)(_bus, path.c_str())));
        }

        _refs.emplace(
                std::make_pair(
                    path, std::move(ref)));
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}

void Manager::signal(sdbusplus::message::message &msg, auto &args)
{
    auto &filter = args.filter;

    if(filter(msg)) {
        args.action();
    }
}

#include "generated.hpp"

} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
