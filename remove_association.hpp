#pragma once

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>

#include <map>
#include <string>
#include <variant>
#include <vector>

namespace phosphor
{
namespace inventory
{
namespace manager
{

using namespace phosphor::logging;
using DBusValue = std::variant<
    std::string, bool, std::vector<uint8_t>, std::vector<std::string>,
    std::vector<std::tuple<std::string, std::string, std::string>>>;
using DBusInterface = std::string;
using DBusService = std::string;
using DBusInterfaceList = std::vector<DBusInterface>;
using DBusObjectPath = std::string;
using DBusSubtree =
    std::map<DBusObjectPath, std::map<DBusService, std::vector<DBusInterface>>>;

/**
 * @brief Wrapper for the 'Get' properties method call
 *
 * @param[in] bus - The sdbusplus bus object for making D-Bus calls
 * @param[in] service - The D-Bus service to call it on
 * @param[in] objectPath - The D-Bus object path
 * @param[in] interface - The interface to get the property on
 * @param[in] property - The property name
 * @param[out] value - Filled in with the property value.
 */
void getProperty(sdbusplus::bus::bus& bus, const std::string& service,
                 const std::string& objectPath, const std::string& interface,
                 const std::string& property, DBusValue& value);

/**
 * @brief Get subtree from the object mapper to get inventory associations
 *
 * @param[in] bus - The sdbusplus bus object for making D-Bus calls
 * @param[in] path - The root of the tree to search.
 * @param[in] depth - The number of path elements to descend.
 */
DBusSubtree getInventoryAssociations(sdbusplus::bus::bus& bus,
                                     const std::string& path, int32_t depth);

/**
 * @brief Removes the critical association on the D-Bus object.
 *
 * @param[in] bus - The sdbusplus bus object for making D-Bus calls
 * @param[in] objectPath - The D-Bus object path
 * @param[in] service - The D-Bus service to call it on
 */
void removeCriticalAssociation(sdbusplus::bus::bus& bus,
                               const std::string& objectPath,
                               const std::string& service);

/** @brief get Dbus service
 *
 * @param[in] path - The D-Bus object path
 * @param[in] interface - The DBus interface.
 */
const std::string getService(const std::string& path,
                             const std::string& interface);

} // namespace manager
} // namespace inventory
} // namespace phosphor
