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
using ActionBase = holder::CallableBase<void, Manager &>;
using ActionBasePtr = std::shared_ptr<ActionBase>;
template <typename T>
using Action = holder::CallableHolder<T, void, Manager &>;

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
decltype(auto) make_action(T && action)
{
    return Action<T>::template make_shared<Action<T>>(
        std::forward<decltype(action)>(action));
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
} // namespace actions
} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
