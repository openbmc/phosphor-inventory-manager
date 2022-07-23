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

#include "errors.hpp"

#include <algorithm>
#include <chrono>
#include <exception>
#include <filesystem>
#include <iostream>

using namespace std::literals::chrono_literals;

namespace phosphor
{
namespace inventory
{
namespace manager
{
/** @brief Fowrarding signal callback.
 *
 *  Extracts per-signal specific context and forwards the call to the manager
 *  instance.
 */
auto _signal(sd_bus_message* m, void* data, sd_bus_error* /* e */) noexcept
{
    try
    {
        auto msg = sdbusplus::message_t(m);
        auto& args = *static_cast<Manager::SigArg*>(data);
        sd_bus_message_ref(m);
        auto& mgr = *std::get<0>(args);
        mgr.handleEvent(msg, static_cast<const DbusSignal&>(*std::get<1>(args)),
                        *std::get<2>(args));
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}

Manager::Manager(sdbusplus::bus_t&& bus, const char* root) :
    ServerObject<ManagerIface>(bus, root), _root(root), _bus(std::move(bus)),
    _manager(_bus, root),
#ifdef CREATE_ASSOCIATIONS
    _associations(_bus),
#endif
    _status(ManagerStatus::STARTING)
{
    for (auto& group : _events)
    {
        for (auto pEvent : std::get<std::vector<EventBasePtr>>(group))
        {
            if (pEvent->type != Event::Type::DBUS_SIGNAL)
            {
                continue;
            }

            // Create a callback context for this event group.
            auto dbusEvent = static_cast<DbusSignal*>(pEvent.get());

            // Go ahead and store an iterator pointing at
            // the event data to avoid lookups later since
            // additional signal callbacks aren't added
            // after the manager is constructed.
            _sigargs.emplace_back(
                std::make_unique<SigArg>(this, dbusEvent, &group));

            // Register our callback and the context for
            // each signal event.
            _matches.emplace_back(_bus, dbusEvent->signature, _signal,
                                  _sigargs.back().get());
        }
    }

    // Restore any persistent inventory
    restore();
}

void Manager::shutdown() noexcept
{
    _status = ManagerStatus::STOPPING;
}

void Manager::run(const char* busname)
{
    sdbusplus::message_t unusedMsg{nullptr};

    // Run startup events.
    for (auto& group : _events)
    {
        for (auto pEvent : std::get<std::vector<EventBasePtr>>(group))
        {
            if (pEvent->type == Event::Type::STARTUP)
            {
                handleEvent(unusedMsg, *pEvent, group);
            }
        }
    }

    _status = ManagerStatus::RUNNING;
    _bus.request_name(busname);

    while (_status != ManagerStatus::STOPPING)
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

void Manager::updateInterfaces(const sdbusplus::message::object_path& path,
                               const Object& interfaces,
                               ObjectReferences::iterator pos, bool newObject,
                               bool restoreFromCache)
{
    auto& refaces = pos->second;
    auto ifaceit = interfaces.cbegin();
    auto opsit = _makers.cbegin();
    auto refaceit = refaces.begin();
    std::vector<std::string> signals;

    while (ifaceit != interfaces.cend())
    {
        try
        {
            // Find the binding ops for this interface.
            opsit = std::lower_bound(opsit, _makers.cend(), ifaceit->first,
                                     compareFirst(_makers.key_comp()));

            if (opsit == _makers.cend() || opsit->first != ifaceit->first)
            {
                // This interface is not supported.
                throw InterfaceError("Encountered unsupported interface.",
                                     ifaceit->first);
            }

            // Find the binding insertion point or the binding to update.
            refaceit = std::lower_bound(refaceit, refaces.end(), ifaceit->first,
                                        compareFirst(refaces.key_comp()));

            if (refaceit == refaces.end() || refaceit->first != ifaceit->first)
            {
                // Add the new interface.
                auto& ctor = std::get<MakeInterfaceType>(opsit->second);
                // skipSignal = true here to avoid getting PropertiesChanged
                // signals while the interface is constructed.  We'll emit an
                // ObjectManager signal for this interface below.
                refaceit = refaces.insert(
                    refaceit, std::make_pair(ifaceit->first,
                                             ctor(_bus, path.str.c_str(),
                                                  ifaceit->second, true)));
                signals.push_back(ifaceit->first);
            }
            else
            {
                // Set the new property values.
                auto& assign = std::get<AssignInterfaceType>(opsit->second);
                assign(ifaceit->second, refaceit->second,
                       _status != ManagerStatus::RUNNING);
            }
            if (!restoreFromCache)
            {
                auto& serialize =
                    std::get<SerializeInterfaceType<SerialOps>>(opsit->second);
                serialize(path, ifaceit->first, refaceit->second);
            }
            else
            {
                auto& deserialize =
                    std::get<DeserializeInterfaceType<SerialOps>>(
                        opsit->second);
                deserialize(path, ifaceit->first, refaceit->second);
            }
        }
        catch (const InterfaceError& e)
        {
            // Reset the binding ops iterator since we are
            // at the end.
            opsit = _makers.cbegin();
            e.log();
        }

        ++ifaceit;
    }

    if (_status == ManagerStatus::RUNNING)
    {
        if (newObject)
        {
            _bus.emit_object_added(path.str.c_str());
        }
        else if (!signals.empty())
        {
            _bus.emit_interfaces_added(path.str.c_str(), signals);
        }
    }
}

void Manager::updateObjects(
    const std::map<sdbusplus::message::object_path, Object>& objs,
    bool restoreFromCache)
{
    auto objit = objs.cbegin();
    auto refit = _refs.begin();
    std::string absPath;
    bool newObj;

    while (objit != objs.cend())
    {
        // Find the insertion point or the object to update.
        refit = std::lower_bound(refit, _refs.end(), objit->first,
                                 compareFirst(RelPathCompare(_root)));

        absPath.assign(_root);
        absPath.append(objit->first);

        newObj = false;
        if (refit == _refs.end() || refit->first != absPath)
        {
            refit = _refs.insert(
                refit, std::make_pair(absPath, decltype(_refs)::mapped_type()));
            newObj = true;
        }

        updateInterfaces(absPath, objit->second, refit, newObj,
                         restoreFromCache);
#ifdef CREATE_ASSOCIATIONS
        if (!_associations.pendingCondition() && newObj)
        {
            _associations.createAssociations(absPath,
                                             _status != ManagerStatus::RUNNING);
        }
        else if (!restoreFromCache &&
                 _associations.conditionMatch(objit->first, objit->second))
        {
            // The objit path/interface/property matched a pending condition.
            // Now the associations are valid so attempt to create them against
            // all existing objects.  If this was the restoreFromCache path,
            // objit doesn't contain property values so don't bother checking.
            std::for_each(_refs.begin(), _refs.end(), [this](const auto& ref) {
                _associations.createAssociations(
                    ref.first, _status != ManagerStatus::RUNNING);
            });
        }
#endif
        ++objit;
    }
}

void Manager::notify(std::map<sdbusplus::message::object_path, Object> objs)
{
    updateObjects(objs);
}

void Manager::handleEvent(sdbusplus::message_t& msg, const Event& event,
                          const EventInfo& info)
{
    auto& actions = std::get<1>(info);

    for (auto& f : event)
    {
        if (!f(_bus, msg, *this))
        {
            return;
        }
    }
    for (auto& action : actions)
    {
        action(_bus, *this);
    }
}

void Manager::destroyObjects(const std::vector<const char*>& paths)
{
    std::string p;

    for (const auto& path : paths)
    {
        p.assign(_root);
        p.append(path);
        _bus.emit_object_removed(p.c_str());
        _refs.erase(p);
    }
}

void Manager::createObjects(
    const std::map<sdbusplus::message::object_path, Object>& objs)
{
    updateObjects(objs);
}

std::any& Manager::getInterfaceHolder(const char* path, const char* interface)
{
    return const_cast<std::any&>(
        const_cast<const Manager*>(this)->getInterfaceHolder(path, interface));
}

const std::any& Manager::getInterfaceHolder(const char* path,
                                            const char* interface) const
{
    std::string p{path};
    auto oit = _refs.find(_root + p);
    if (oit == _refs.end())
        throw std::runtime_error(_root + p + " was not found");

    auto& obj = oit->second;
    auto iit = obj.find(interface);
    if (iit == obj.end())
        throw std::runtime_error("interface was not found");

    return iit->second;
}

void Manager::restore()
{
    namespace fs = std::filesystem;

    if (!fs::exists(fs::path(PIM_PERSIST_PATH)))
    {
        return;
    }

    static const std::string remove =
        std::string(PIM_PERSIST_PATH) + INVENTORY_ROOT;

    std::map<sdbusplus::message::object_path, Object> objects;
    for (const auto& dirent :
         fs::recursive_directory_iterator(PIM_PERSIST_PATH))
    {
        const auto& path = dirent.path();
        if (fs::is_regular_file(path))
        {
            auto ifaceName = path.filename().string();
            auto objPath = path.parent_path().string();
            objPath.erase(0, remove.length());
            auto objit = objects.find(objPath);
            Interface propertyMap{};
            if (objects.end() != objit)
            {
                auto& object = objit->second;
                object.emplace(std::move(ifaceName), std::move(propertyMap));
            }
            else
            {
                Object object;
                object.emplace(std::move(ifaceName), std::move(propertyMap));
                objects.emplace(std::move(objPath), std::move(object));
            }
        }
    }
    if (!objects.empty())
    {
        auto restoreFromCache = true;
        updateObjects(objects, restoreFromCache);

#ifdef CREATE_ASSOCIATIONS
        // There may be conditional associations waiting to be loaded
        // based on certain path/interface/property values.  Now that
        // _refs contains all objects with their property values, check
        // which property values the conditions need and set them in the
        // condition structure entries, using the actualValue field.  Then
        // the associations manager can check if the conditions are met.
        if (_associations.pendingCondition())
        {
            ObjectReferences::iterator refIt;
            InterfaceComposite::iterator ifaceIt;

            auto& conditions = _associations.getConditions();
            for (auto& condition : conditions)
            {
                refIt = _refs.find(_root + condition.path);
                if (refIt != _refs.end())
                {
                    ifaceIt = refIt->second.find(condition.interface);
                }

                if ((refIt != _refs.end()) && (ifaceIt != refIt->second.end()))
                {
                    const auto& maker = _makers.find(condition.interface);
                    if (maker != _makers.end())
                    {
                        auto& getProperty =
                            std::get<GetPropertyValueType>(maker->second);

                        condition.actualValue =
                            getProperty(condition.property, ifaceIt->second);
                    }
                }
            }

            // Check if a property value in a condition matches an
            // actual property value just saved.  If one did, now the
            // associations file is valid so create its associations.
            if (_associations.conditionMatch())
            {
                std::for_each(
                    _refs.begin(), _refs.end(), [this](const auto& ref) {
                        _associations.createAssociations(
                            ref.first, _status != ManagerStatus::RUNNING);
                    });
            }
        }
#endif
    }
}

} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
