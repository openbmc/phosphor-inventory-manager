#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/message/types.hpp>

#include <cstdint>
#include <functional>
#include <map>
#include <string>

namespace phosphor
{
namespace inventory
{
namespace manager
{

class Manager;

/** @brief Inventory manager supported property types. */
using InterfaceVariantType =
    std::variant<bool, size_t, int64_t, uint16_t, std::string,
                 std::vector<uint8_t>, std::vector<std::string>>;

template <typename T>
using InterfaceType = std::map<std::string, T>;

template <typename T>
using ObjectType = std::map<std::string, InterfaceType<T>>;

using Interface = InterfaceType<InterfaceVariantType>;
using Object = ObjectType<InterfaceVariantType>;

using Action = std::function<void(sdbusplus::bus_t&, Manager&)>;
using Filter =
    std::function<bool(sdbusplus::bus_t&, sdbusplus::message_t&, Manager&)>;
using PathCondition =
    std::function<bool(const std::string&, sdbusplus::bus_t&, Manager&)>;
template <typename T>
using GetProperty = std::function<T(Manager&)>;
} // namespace manager
} // namespace inventory
} // namespace phosphor
