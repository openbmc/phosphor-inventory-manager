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

class Manager;
namespace filters
{
namespace details
{
namespace holder
{

/** @struct Base
 *  @brief Match event filter functor holder base.
 *
 *  Provides an un-templated holder for filters of any type with the correct
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

    virtual bool operator()(sdbusplus::message::message &, Manager &) const = 0;
    virtual bool operator()(sdbusplus::message::message &msg, Manager &mgr)
    {
        return const_cast<const Base &>(*this)(msg, mgr);
    }
};

/** @struct Holder
 *  @brief Match event filter functor holder.
 *
 *  Adapts a functor of any type (with the correct function call
 *  signature) to a non-templated type usable by the manager for
 *  filtering.
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
    explicit Holder(T &&func) : _func(std::forward<T>(func)) { }

    virtual bool operator()(
            sdbusplus::message::message &msg, Manager &mgr) const override
    {
        return _func(msg, mgr);
    }

    virtual bool operator()(
            sdbusplus::message::message &msg, Manager &mgr) override
    {
        return _func(msg, mgr);
    }

    private:
    T _func;
};

} // namespace holder

/** @struct Wrapper
 *  @brief Provides implicit type conversion from filter functors.
 *
 *  Converts filter functors to ptr-to-holder.
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

    bool operator()(sdbusplus::message::message &msg, Manager &mgr)
    {
        return (*_ptr)(msg, mgr);
    }
    bool operator()(sdbusplus::message::message &msg, Manager &mgr) const
    {
        return (*_ptr)(msg, mgr);
    }

    private:
    std::shared_ptr<holder::Base> _ptr;
};

namespace property_changed
{

/** @struct Base
 *  @brief Common code for all PropertyChangedTo filter functors.
 */
struct Base
{
    Base() = delete;
    virtual ~Base() = default;
    Base(const Base&) = default;
    Base & operator=(const Base&) = delete;
    Base(Base&&) = default;
    Base& operator=(Base&&) = delete;

    Base(const char *iface, const char *property) :
        _iface(iface), _property(property) {}

    virtual bool checkValue(
            sdbusplus::message::message &_msg) const = 0;
    bool operator()(sdbusplus::message::message &_msg, Manager &) const;

    private:
    const char *_iface;
    const char *_property;
};

/** @struct PropertyChangedTo
 *  @brief Match filter functor that tests a property value.
 *
 *  @tparam T - The property type.
 */
template <typename T>
struct PropertyChanged final : public Base
{
    PropertyChanged() = delete;
    ~PropertyChanged() = default;
    PropertyChanged(const PropertyChanged&) = default;
    PropertyChanged & operator=(const PropertyChanged&) = delete;
    PropertyChanged(PropertyChanged&&) = default;
    PropertyChanged& operator=(PropertyChanged&&) = delete;
    PropertyChanged(const char *iface, const char *property, T val) :
        Base(iface, property),
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

} // namespace property_changed
} // namespace details

/** @brief The default filter.  */
inline bool none(sdbusplus::message::message &, Manager &) noexcept
{
    return true;
}

/** @brief Implicit type deduction for constructing PropertyChanged.  */
template <typename T>
details::property_changed::PropertyChanged<T> propertyChangedTo(
        const char *iface,
        const char *property,
        T val)
{
    return details::property_changed::PropertyChanged<T>(iface, property, val);
}

} // namespace filters
} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
