#pragma once

#include <map>
#include <string>
#include <sdbusplus/message.hpp>
#include <experimental/any>

namespace phosphor
{
namespace inventory
{
namespace manager
{

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

} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
