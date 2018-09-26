#pragma once

#include <experimental/any>
#include <functional>
#include <map>
#include <sdbusplus/message.hpp>
#include <string>

namespace phosphor
{
namespace inventory
{
namespace manager
{

class Manager;
namespace any_ns = std::experimental;

/** @brief Inventory manager supported property types. */
using InterfaceVariantType =
    sdbusplus::message::variant<bool, int64_t, std::string>;

template <typename T>
using InterfaceType = std::map<std::string, T>;

template <typename T>
using ObjectType = std::map<std::string, InterfaceType<T>>;

using Interface = InterfaceType<InterfaceVariantType>;
using Object = ObjectType<InterfaceVariantType>;

using Action = std::function<void(sdbusplus::bus::bus&, Manager&)>;
using Filter = std::function<bool(sdbusplus::bus::bus&,
                                  sdbusplus::message::message&, Manager&)>;
using PathCondition =
    std::function<bool(const std::string&, sdbusplus::bus::bus&, Manager&)>;
template <typename T> using GetProperty = std::function<T(Manager&)>;
} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
