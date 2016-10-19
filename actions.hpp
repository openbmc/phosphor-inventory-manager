#pragma once

#include <utility>
#include <memory>

namespace phosphor
{
namespace inventory
{
namespace manager
{
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
    Base(Base&&) = default;
    Base& operator=(Base&&) = default;

    virtual void operator()() const = 0;
    virtual void operator()()
    {
        const_cast<const Base &>(*this)();
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
    Holder(Holder&&) = default;
    Holder& operator=(Holder&&) = default;
    explicit Holder(T &&func) : _func(std::forward<T>(func)) {}

    virtual void operator()() const override
    {
        _func();
    }

    virtual void operator()() override
    {
        _func();
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
        _ptr(static_cast<std::shared_ptr<holder::Base>>(
                    std::make_shared<holder::Holder<T>>(
                        std::forward<T>(func)))) { }

    ~Wrapper() = default;
    Wrapper(const Wrapper&) = default;
    Wrapper& operator=(const Wrapper&) = delete;
    Wrapper(Wrapper&&) = default;
    Wrapper& operator=(Wrapper&&) = default;

    void operator()()
    {
        (*_ptr)();
    }
    void operator()() const
    {
        (*_ptr)();
    }

    private:
    std::shared_ptr<holder::Base> _ptr;
};

} // namespace details

/** @brief The default action.  */
inline void noop() noexcept { }

} // namespace actions
} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
