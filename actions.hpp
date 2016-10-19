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

struct Base
{
    Base() = default;
    virtual ~Base() = default;
    Base(const Base&) = delete;
    Base& operator=(const Base&) = delete;
    Base(Base&&) = delete;
    Base& operator=(Base&&) = delete;

    virtual void operator()() const = 0;
    virtual void operator()()
    {
        const_cast<const Base &>(*this)();
    }
};

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

struct Wrapper
{
    template <typename T>
    Wrapper(T &&func) :
        _ptr(std::shared_ptr<holder::Base>(
                    new holder::Holder<T>(std::forward<T>(func)))) { }

    ~Wrapper() = default;
    Wrapper(const Wrapper&) = default;
    Wrapper& operator=(const Wrapper&) = delete;
    Wrapper(Wrapper&&) = delete;
    Wrapper& operator=(Wrapper&&) = delete;

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

inline void noop() noexcept { }

} // namespace actions
} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
