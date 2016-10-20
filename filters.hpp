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

namespace property_condition
{

/** @struct PropertyCondition
 *  @brief Match filter functor that tests a property value.
 *
 *  @tparam T - The type of the property being tested.
 *  @tparam U - The type of the condition checking functor.
 */
template <typename T, typename U>
struct PropertyCondition
{
    PropertyCondition() = delete;
    ~PropertyCondition() = default;
    PropertyCondition(const PropertyCondition&) = default;
    PropertyCondition & operator=(const PropertyCondition&) = delete;
    PropertyCondition(PropertyCondition&&) = default;
    PropertyCondition& operator=(PropertyCondition&&) = delete;
    PropertyCondition(const char *iface, const char *property, U &&condition) :
        _iface(iface),
        _property(property),
        _condition(std::forward<U>(condition)) { }

    bool operator()(sdbusplus::message::message &msg, Manager &) const
    {
        std::map<
            std::string,
            sdbusplus::message::variant<T>> properties;
        const char *iface;

        msg.read(iface);
        if(strcmp(iface, _iface))
                return false;

        msg.read(properties);
        auto it = properties.find(_property);
        if(it == properties.cend())
            return false;

        return _condition(it->second);
    }

    private:
    const char *_iface;
    const char *_property;
    U _condition;
};

} // namespace property_condition
} // namespace details

/** @brief The default filter.  */
inline bool none(sdbusplus::message::message &, Manager &) noexcept
{
    return true;
}

/** @brief Implicit type deduction for constructing PropertyCondition.  */
template <typename T>
auto propertyChangedTo(
        const char *iface,
        const char *property,
        T val)
{
    auto condition = [val](auto arg){return arg == val;};
    using U = decltype(condition);
    return details::property_condition::PropertyCondition<T, U>(
            iface, property, std::move(condition));
}

} // namespace filters
} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
