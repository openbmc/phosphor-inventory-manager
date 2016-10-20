#pragma once

#include <utility>
#include <memory>

namespace phosphor
{
namespace inventory
{
namespace manager
{
class Manager;

namespace actions
{
namespace details
{
namespace holder
{

/** @struct Base
 *  @brief Event action functor holder base.
 *
 *  Provides an un-templated holder for actionsof any type with the correct
 *  function call signature.
 */
struct Base
{
    Base() = default;
    virtual ~Base() = default;
    Base(const Base&) = delete;
    Base& operator=(const Base&) = delete;
    Base(Base&&) = delete;
    Base& operator=(Base&&) = delete;

    virtual void operator()(Manager &mgr) const = 0;
    virtual void operator()(Manager &mgr)
    {
        const_cast<const Base &>(*this)(mgr);
    }
};

/** @struct Holder
 *  @brief Event action functor holder.
 *
 *  Adapts a functor of any type (with the correct function call
 *  signature) to a non-templated type usable by the manager for
 *  actions.
 *
 *  @tparam T - The functor type.
 */
template <typename T>
struct Holder final : public Base
{
    Holder() = delete;
    ~Holder() = default;
    Holder(const Holder&) = delete;
    Holder & operator=(const Holder&) = delete;
    Holder(Holder&&) = delete;
    Holder& operator=(Holder&&) = delete;
    explicit Holder(T &&func) : _func(std::forward<T>(func)) {}

    virtual void operator()(Manager &mgr) const override
    {
        _func(mgr);
    }

    virtual void operator()(Manager &mgr) override
    {
        _func(mgr);
    }

    private:
    T _func;
};

} // namespace holder

/** @struct Wrapper
 *  @brief Provides implicit type conversion from action functors.
 *
 *  Converts action functors to ptr-to-holder.
 */
struct Wrapper
{
    template <typename T>
    Wrapper(T &&func) :
        _ptr(std::shared_ptr<holder::Base>(
                    new holder::Holder<T>(std::forward<T>(func)))) { }

    ~Wrapper() = default;
    Wrapper(const Wrapper&) = default;
    Wrapper& operator=(const Wrapper&) = delete;
    Wrapper(Wrapper&&) = default;
    Wrapper& operator=(Wrapper&&) = delete;

    void operator()(Manager &mgr)
    {
        (*_ptr)(mgr);
    }
    void operator()(Manager &mgr) const
    {
        (*_ptr)(mgr);
    }

    private:
    std::shared_ptr<holder::Base> _ptr;
};

} // namespace details

/** @brief The default action.  */
inline void noop(Manager &mgr) noexcept { }

} // namespace actions
} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
