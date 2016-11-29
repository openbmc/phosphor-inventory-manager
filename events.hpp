#pragma once

#include <utility>
#include <memory>
#include <sdbusplus/message.hpp>
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
using FilterBase = holder::CallableBase<
    bool, sdbusplus::message::message&, Manager&>;
using FilterBasePtr = std::shared_ptr<FilterBase>;
template <typename T>
using Filter = holder::CallableHolder<
    T, bool, sdbusplus::message::message&, Manager&>;

/** @struct Event
 *  @brief Event object interface.
 */
struct Event
{
    enum class Type
    {
        DBUS_SIGNAL,
    };

    virtual ~Event() = default;
    Event(const Event&) = default;
    Event& operator=(const Event&) = default;
    Event(Event&&) = default;
    Event& operator=(Event&&) = default;
    explicit Event(Type t) : type(t) {}

    Type type;
};

using EventBasePtr = std::shared_ptr<Event>;

/** @struct DbusSignal
 *  @brief DBus signal event.
 *
 *  DBus signal events are an association of a match signature
 *  and filtering function object.
 */
struct DbusSignal final :
    public Event,
    public std::tuple<const char *, FilterBasePtr>
{
    virtual ~DbusSignal() = default;
    DbusSignal(const DbusSignal&) = default;
    DbusSignal & operator=(const DbusSignal&) = delete;
    DbusSignal(DbusSignal&&) = default;
    DbusSignal& operator=(DbusSignal&&) = default;

    /** @brief Import from signature and filter constructor.
     *
     *  @param[in] sig - The DBus match signature.
     *  @param[in] filter - A DBus signal match callback filtering function.
     */
    DbusSignal(const char *sig, FilterBasePtr filter) :
        Event(Type::DBUS_SIGNAL),
        std::tuple<const char *, FilterBasePtr>(
                sig, std::move(filter)) {}
};

/** @brief make_filter
 *
 *  Adapt a filter function object.
 *
 *  @param[in] filter - The filter being adapted.
 *  @returns - The adapted filter.
 *
 *  @tparam T - The type of the filter being adapted.
 */
template <typename T>
auto make_filter(T&& filter)
{
    return Filter<T>::template make_shared<Filter<T>>(
        std::forward<T>(filter));
}
} // namespace details

namespace filters
{
namespace details
{
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
    PropertyCondition& operator=(PropertyCondition&&) = default;
    PropertyCondition(const char *iface, const char *property, U &&condition) :
        _iface(iface),
        _property(property),
        _condition(std::forward<U>(condition)) { }

    /** @brief Test a property value.
     *
     * Extract the property from the PropertiesChanged
     * message and run the condition test.
     */
    bool operator()(sdbusplus::message::message &msg, Manager &) const
    {
        std::map<
            std::string,
            sdbusplus::message::variant<T>> properties;
        const char *iface = nullptr;

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
    auto condition = [val = std::move(val)](auto arg){return arg == val;};
    using U = decltype(condition);
    return details::property_condition::PropertyCondition<T, U>(
            iface, property, std::move(condition));
}

} // namespace filters
} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
