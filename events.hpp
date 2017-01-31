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
using FilterBase = holder::CallableBase <
                   bool, sdbusplus::bus::bus&, sdbusplus::message::message&, Manager& >;
using FilterBasePtr = std::shared_ptr<FilterBase>;
template <typename T>
using Filter = holder::CallableHolder <
               T, bool, sdbusplus::bus::bus&, sdbusplus::message::message&, Manager& >;

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
    public std::tuple<const char*, std::vector<FilterBasePtr>>
{
    virtual ~DbusSignal() = default;
    DbusSignal(const DbusSignal&) = default;
    DbusSignal& operator=(const DbusSignal&) = delete;
    DbusSignal(DbusSignal&&) = default;
    DbusSignal& operator=(DbusSignal&&) = default;

    /** @brief Import from signature and filter constructor.
     *
     *  @param[in] sig - The DBus match signature.
     *  @param[in] filter - A DBus signal match callback filtering function.
     */
    DbusSignal(const char* sig, const std::vector<FilterBasePtr>& filters) :
        Event(Type::DBUS_SIGNAL),
        std::tuple<const char*, std::vector<FilterBasePtr>>(
                    sig, std::move(filters)) {}
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

/** @struct PropertyChangedCondition
 *  @brief Match filter functor that tests a property value.
 *
 *  @tparam T - The type of the property being tested.
 *  @tparam U - The type of the condition checking functor.
 */
template <typename T, typename U>
struct PropertyChangedCondition
{
        PropertyChangedCondition() = delete;
        ~PropertyChangedCondition() = default;
        PropertyChangedCondition(const PropertyChangedCondition&) = default;
        PropertyChangedCondition& operator=(const PropertyChangedCondition&) = delete;
        PropertyChangedCondition(PropertyChangedCondition&&) = default;
        PropertyChangedCondition& operator=(PropertyChangedCondition&&) = default;
        PropertyChangedCondition(const char* iface, const char* property,
                                 U&& condition) :
            _iface(iface),
            _property(property),
            _condition(std::forward<U>(condition)) { }

        /** @brief Test a property value.
         *
         * Extract the property from the PropertiesChanged
         * message and run the condition test.
         */
        bool operator()(
            sdbusplus::bus::bus&,
            sdbusplus::message::message& msg,
            Manager&) const
        {
            std::map <
            std::string,
                sdbusplus::message::variant<T >> properties;
            const char* iface = nullptr;

            msg.read(iface);
            if (!iface || strcmp(iface, _iface))
            {
                return false;
            }

            msg.read(properties);
            auto it = properties.find(_property);
            if (it == properties.cend())
            {
                return false;
            }

            return _condition(
                       std::forward<T>(it->second.template get<T>()));
        }

    private:
        const char* _iface;
        const char* _property;
        U _condition;
};

/** @struct PropertyConditionBase
 *  @brief Match filter functor that tests a property value.
 *
 *  Base class for PropertyCondition - factored out code that
 *  doesn't need to be templated.
 */
struct PropertyConditionBase
{
        PropertyConditionBase() = delete;
        virtual ~PropertyConditionBase() = default;
        PropertyConditionBase(const PropertyConditionBase&) = delete;
        PropertyConditionBase& operator=(const PropertyConditionBase&) = delete;
        PropertyConditionBase(PropertyConditionBase&&) = default;
        PropertyConditionBase& operator=(PropertyConditionBase&&) = default;

        /** @brief Constructor
         *
         *  The service argument can be nullptr.  If something
         *  else is provided the function will call the the
         *  service directly.  If omitted, the function will
         *  look up the service in the ObjectMapper.
         *
         *  @param path - The path of the object containing
         *     the property to be tested.
         *  @param iface - The interface hosting the property
         *     to be tested.
         *  @param property - The property to be tested.
         *  @param service - The DBus service hosting the object.
         */
        PropertyConditionBase(
            const char* path,
            const char* iface,
            const char* property,
            const char* service) :
            _path(path),
            _iface(iface),
            _property(property),
            _service(service) {}

        /** @brief Forward comparison to type specific implementation. */
        virtual bool eval(sdbusplus::message::message&) const = 0;

        /** @brief Test a property value.
         *
         * Make a DBus call and test the value of any property.
         */
        bool operator()(
            sdbusplus::bus::bus&,
            sdbusplus::message::message&,
            Manager&) const;

    private:
        std::string _path;
        std::string _iface;
        std::string _property;
        const char* _service;
};

/** @struct PropertyCondition
 *  @brief Match filter functor that tests a property value.
 *
 *  @tparam T - The type of the property being tested.
 *  @tparam U - The type of the condition checking functor.
 */
template <typename T, typename U>
struct PropertyCondition final : public PropertyConditionBase
{
        PropertyCondition() = delete;
        ~PropertyCondition() = default;
        PropertyCondition(const PropertyCondition&) = delete;
        PropertyCondition& operator=(const PropertyCondition&) = delete;
        PropertyCondition(PropertyCondition&&) = default;
        PropertyCondition& operator=(PropertyCondition&&) = default;

        /** @brief Constructor
         *
         *  The service argument can be nullptr.  If something
         *  else is provided the function will call the the
         *  service directly.  If omitted, the function will
         *  look up the service in the ObjectMapper.
         *
         *  @param path - The path of the object containing
         *     the property to be tested.
         *  @param iface - The interface hosting the property
         *     to be tested.
         *  @param property - The property to be tested.
         *  @param condition - The test to run on the property.
         *  @param service - The DBus service hosting the object.
         */
        PropertyCondition(
            const char* path,
            const char* iface,
            const char* property,
            U&& condition,
            const char* service) :
            PropertyConditionBase(path, iface, property, service),
            _condition(std::forward<decltype(condition)>(condition)) {}

        /** @brief Test a property value.
         *
         * Make a DBus call and test the value of any property.
         */
        bool eval(sdbusplus::message::message& msg) const override
        {
            sdbusplus::message::variant<T> value;
            msg.read(value);
            return _condition(
                       std::forward<T>(value.template get<T>()));
        }

    private:
        U _condition;
};

} // namespace property_condition
} // namespace details

/** @brief Implicit type deduction for constructing PropertyChangedCondition.  */
template <typename T>
auto propertyChangedTo(
    const char* iface,
    const char* property,
    T&& val)
{
    auto condition = [val = std::forward<T>(val)](T && arg)
    {
        return arg == val;
    };
    using U = decltype(condition);
    return details::property_condition::PropertyChangedCondition<T, U>(
               iface, property, std::move(condition));
}

/** @brief Implicit type deduction for constructing PropertyCondition.  */
template <typename T>
auto propertyIs(
    const char* path,
    const char* iface,
    const char* property,
    T&& val,
    const char* service = nullptr)
{
    auto condition = [val = std::forward<T>(val)](T && arg)
    {
        return arg == val;
    };
    using U = decltype(condition);
    return details::property_condition::PropertyCondition<T, U>(
               path, iface, property, std::move(condition), service);
}

} // namespace filters
} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
