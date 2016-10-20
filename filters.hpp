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

inline bool none(sdbusplus::message::message &) noexcept
{
    return true;
}

namespace details
{

struct PropertyChangedTo
{
    PropertyChangedTo() = delete;
    virtual ~PropertyChangedTo() = default;
    PropertyChangedTo(const PropertyChangedTo&) = default;
    PropertyChangedTo & operator=(const PropertyChangedTo&) = delete;
    PropertyChangedTo(PropertyChangedTo&&) = default;
    PropertyChangedTo& operator=(PropertyChangedTo&&) = delete;

    PropertyChangedTo(const char *iface, const char *property) :
        _iface(iface), _property(property) {}

    virtual bool checkValue(sdbusplus::message::message &_msg) const = 0;
    bool operator()(sdbusplus::message::message &_msg) const;

    private:
    const char *_iface;
    const char *_property;
};

template <typename T>
struct _PropertyChangedTo final : public PropertyChangedTo
{
    _PropertyChangedTo() = delete;
    ~_PropertyChangedTo() = default;
    _PropertyChangedTo(const _PropertyChangedTo&) = default;
    _PropertyChangedTo & operator=(const _PropertyChangedTo&) = delete;
    _PropertyChangedTo(_PropertyChangedTo&&) = default;
    _PropertyChangedTo& operator=(_PropertyChangedTo&&) = delete;
    _PropertyChangedTo(const char *iface, const char *property, T val) :
        PropertyChangedTo(iface, property),
        _val(val) {}

    bool checkValue(sdbusplus::message::message &msg) const override
    {
        sdbusplus::message::variant<T> value;
        msg.read(value);

        return value == _val;
    }

    private:
    T _val;
};

} // namespace details

template <typename T>
details::_PropertyChangedTo<T> propertyChangedTo(
        const char *iface,
        const char *property,
        T val)
{
    return details::_PropertyChangedTo<T>(iface, property, val);
}

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

    virtual bool operator()(sdbusplus::message::message &) const = 0;
    virtual bool operator()(sdbusplus::message::message &msg)
    {
        return const_cast<const WrapperBase &>(*this)(msg);
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

    bool operator()(sdbusplus::message::message &msg)
    {
        return (*_ptr)(msg);
    }
    bool operator()(sdbusplus::message::message &msg) const
    {
        return (*_ptr)(msg);
    }

    private:
    std::shared_ptr<WrapperBase> _ptr;
};

} // namespace details
} // namespace filters
} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
