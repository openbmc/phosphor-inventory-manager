#pragma once

#include <map>
#include <string>
#include <sdbusplus/message.hpp>
#include "utils.hpp"

namespace phosphor
{
namespace inventory
{
namespace manager
{
class Manager;

/** @brief Inventory manager supported property types. */
using InterfaceVariantType =
    sdbusplus::message::variant<bool, int64_t, std::string>;

template <typename T>
using InterfaceType = std::map<std::string, T>;

template <typename T>
using ObjectType = std::map<std::string, InterfaceType<T>>;

using Interface = InterfaceType<InterfaceVariantType>;
using Object = ObjectType<InterfaceVariantType>;

using Action = details::holder::Adapted <
               void,
               sdbusplus::bus::bus&,
               Manager& >;

using Filter = details::holder::Adapted <
               bool,
               sdbusplus::bus::bus&,
               sdbusplus::message::message&,
               Manager& >;

using PathCondition = details::holder::Adapted <
                      bool,
                      const std::string&,
                      sdbusplus::bus::bus&,
                      Manager& >;

} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
