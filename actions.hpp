#pragma once

#include <utility>
#include <memory>
#include "utils.hpp"
#include "types.hpp"

namespace phosphor
{
namespace inventory
{
namespace manager
{

class Manager;
namespace details
{
using Action = holder::Adapted <
               void,
               sdbusplus::bus::bus&,
               Manager& >;
} // namespace details

namespace actions
{

/** @brief Destroy objects action.  */
inline auto destroyObjects(std::vector<const char*> paths)
{
    return [paths = std::move(paths)](auto&, auto & m)
    {
        m.destroyObjects(paths);
    };
}

/** @brief Create objects action.  */
inline auto createObjects(
    const std::map<sdbusplus::message::object_path, Object>& objs)
{
    return [&objs](auto&, auto & m)
    {
        m.createObjects(objs);
    };
}

/** @brief Set a property action.
 *
 *  Invoke the requested method with a reference to the requested
 *  sdbusplus server binding interface as a parameter.
 *
 *  @tparam T - The sdbusplus server binding interface type.
 *  @tparam U - The type of the sdbusplus server binding member
 *      function that sets the property.
 *  @tparam V - The property value type.
 *
 *  @param[in] paths - The DBus paths on which the property should
 *      be set.
 *  @param[in] iface - The DBus interface hosting the property.
 *  @param[in] member - Pointer to sdbusplus server binding member.
 *  @param[in] value - The value the property should be set to.
 *
 *  @returns - A function object that sets the requested property
 *      to the requested value.
 */
template <typename T, typename U, typename V>
auto setProperty(
    std::vector<const char*> paths, const char* iface,
    U&& member, V&& value)
{
    // The manager is the only parameter passed to actions.
    // Bind the path, interface, interface member function pointer,
    // and value to a lambda.  When it is called, forward the
    // path, interface and value on to the manager member function.
    return [paths = std::move(paths), iface, member,
                  value = std::forward<V>(value)](auto&, auto & m)
    {
        for (auto p : paths)
        {
            m.template invokeMethod<T>(
                p, iface, member, value);
        }
    };
}

} // namespace actions
} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
