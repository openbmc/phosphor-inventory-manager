#pragma once

#include <utility>
#include <memory>
#include "utils.hpp"

namespace phosphor
{
namespace inventory
{
namespace manager
{

class Manager;
namespace details
{
using ActionBase = holder::CallableBase<void, Manager&>;
using ActionBasePtr = std::shared_ptr<ActionBase>;
template <typename T>
using Action = holder::CallableHolder<T, void, Manager&>;

/** @brief make_action
 *
 *  Adapt an action function object.
 *
 *  @param[in] action - The action being adapted.
 *  @returns - The adapted action.
 *
 *  @tparam T - The type of the action being adapted.
 */
template <typename T>
auto make_action(T&& action)
{
    return Action<T>::template make_shared<Action<T>>(
        std::forward<T>(action));
}
} // namespace details

namespace actions
{

/** @brief The default action.  */
inline void noop(Manager &mgr) noexcept { }

/** @brief Destroy an object action.  */
inline auto destroyObject(const char *path)
{
    return [path](auto &m){m.destroyObject(path);};
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
 *  @param[in] path - The DBus path on which the property should
 *      be set.
 *  @param[in] interface - The DBus interface hosting the property.
 *  @param[in] member - Pointer to sdbusplus server binding member.
 *  @param[in] value - The value the property should be set to.
 */
template <typename T, typename U, typename V>
decltype(auto) setProperty(
        const char *path, const char *iface,
        U &&member, V &&value)
{
    return [path, iface, member,
        value = std::forward<V>(value)](auto &m)
    {
        m.template invokeMethod<T>(
            path, iface, member, value);
    };
}

} // namespace actions
} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
