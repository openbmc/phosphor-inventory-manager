#pragma once

#include <utility>
#include <memory>
#include <sdbusplus/message.hpp>

namespace phosphor
{
namespace inventory
{
namespace manager
{
namespace filters
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

    virtual bool operator()(sdbusplus::message::message &) const = 0;
    virtual bool operator()(sdbusplus::message::message &msg)
    {
        return const_cast<const Base &>(*this)(msg);
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
    explicit Holder(T &&func) : _func(std::forward<T>(func)) { }

    virtual bool operator()(sdbusplus::message::message &msg) const override
    {
        return _func(msg);
    }

    virtual bool operator()(sdbusplus::message::message &msg) override
    {
        return _func(msg);
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

    bool operator()(sdbusplus::message::message &msg)
    {
        return (*_ptr)(msg);
    }
    bool operator()(sdbusplus::message::message &msg) const
    {
        return (*_ptr)(msg);
    }

    private:
    std::shared_ptr<holder::Base> _ptr;
};

} // namespace details

inline bool none(sdbusplus::message::message &) noexcept
{
    return true;
}

} // namespace filters
} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
