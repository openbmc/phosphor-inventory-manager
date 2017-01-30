#pragma once

#include <map>
#include <sdbusplus/message.hpp>
#include "types.hpp"

namespace phosphor
{
namespace inventory
{
namespace manager
{
class Manager;
namespace functor
{
namespace details
{

template <typename T, typename ...Args>
auto callPtrArrayWithStatus(T&& container, Args&& ...args)
{
    for (auto f : container)
    {
        if (!(*f)(std::forward<Args>(args)...))
        {
            return false;
        }
    }
    return true;
}

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
            _path(path ? path : std::string()),
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

        /** @brief Test a property value.
         *
         * Make a DBus call and test the value of any property.
         */
        bool operator()(
            const std::string&,
            sdbusplus::bus::bus&,
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
} // namespace details

/** @brief Destroy objects action.  */
inline auto destroyObjects(
    std::vector<const char*> paths,
    std::vector<PathCondition::Shared> conditions)
{
    return [paths = std::move(paths), conditions = std::move(conditions)](auto & b,
            auto & m)
    {
        for (const auto& p : paths)
        {
            if (details::callPtrArrayWithStatus(conditions, p, b, m))
            {
                m.destroyObjects({p});
            }
        }
    };
}

/** @brief Create objects action.  */
inline auto createObjects(
    std::map<sdbusplus::message::object_path, Object> objs)
{
    return [objs = std::move(objs)](auto&, auto & m)
    {
        m.createObjects(std::move(objs));
    };
}

/** @brief Set a property action.
 *
 *  Invoke the requested method with a reference to the requested
 *  sdbusplus server binding interface as a parameter.
 *
 *  @tparam T - The sdbusplus server binding interface type.
 *  @tparam U - The type of the sdbusplus server binding member
 *      function that sets the property.
 *  @tparam V - The property value type.
 *
 *  @param[in] paths - The DBus paths on which the property should
 *      be set.
 *  @param[in] iface - The DBus interface hosting the property.
 *  @param[in] member - Pointer to sdbusplus server binding member.
 *  @param[in] value - The value the property should be set to.
 *
 *  @returns - A function object that sets the requested property
 *      to the requested value.
 */
template <typename T, typename U, typename V>
auto setProperty(
    std::vector<const char*> paths,
    std::vector<PathCondition::Shared> conditions,
    const char* iface,
    U&& member,
    V&& value)
{
    // The manager is the only parameter passed to actions.
    // Bind the path, interface, interface member function pointer,
    // and value to a lambda.  When it is called, forward the
    // path, interface and value on to the manager member function.
    return [paths = std::move(paths), conditions = std::move(conditions), iface,
                  member,
                  value = std::forward<V>(value)](auto & b, auto & m)
    {
        for (auto p : paths)
        {
            if (details::callPtrArrayWithStatus(conditions, p, b, m))
            {
                m.template invokeMethod<T>(
                    p, iface, member, value);
            }
        }
    };
}

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
    return details::PropertyChangedCondition<T, U>(
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
    return details::PropertyCondition<T, U>(
               path, iface, property, std::move(condition), service);
}
} // namespace functor
} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
