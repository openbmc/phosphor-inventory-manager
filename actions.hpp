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

inline void noop() noexcept { }

namespace details
{

struct WrapperBase
{
    WrapperBase() = default;
    virtual ~WrapperBase() = default;
    WrapperBase(const WrapperBase&) = delete;
    WrapperBase& operator=(const WrapperBase&) = delete;
    WrapperBase(WrapperBase&&) = delete;
    WrapperBase& operator=(WrapperBase&&) = delete;

    virtual void operator()() const = 0;
    virtual void operator()()
    {
        const_cast<const WrapperBase &>(*this)();
    }
};

template <typename T>
struct Wrapper final : public WrapperBase
{
    Wrapper() = delete;
    ~Wrapper() = default;
    Wrapper(const Wrapper&) = delete;
    Wrapper & operator=(const Wrapper&) = delete;
    Wrapper(Wrapper&&) = delete;
    Wrapper& operator=(Wrapper&&) = delete;
    explicit Wrapper(T &&func) : _func(std::forward<T>(func)) {}

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

struct Holder
{
    template <typename T>
    Holder(T &&func) :
        _ptr(std::shared_ptr<WrapperBase>(
                    new Wrapper<T>(std::forward<T>(func)))) { }

    ~Holder() = default;
    Holder(const Holder&) = default;
    Holder& operator=(const Holder&) = delete;
    Holder(Holder&&) = delete;
    Holder& operator=(Holder&&) = delete;

    void operator()()
    {
        (*_ptr)();
    }
    void operator()() const
    {
        (*_ptr)();
    }

    private:
    std::shared_ptr<WrapperBase> _ptr;
};

} // namespace details
} // namespace actions
} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
