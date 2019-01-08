#pragma once

#include "events.hpp"
#include "functor.hpp"
#include "serialize.hpp"
#include "types.hpp"

#include <any>
#include <map>
#include <memory>
#include <sdbusplus/server.hpp>
#include <string>
#include <vector>
#include <xyz/openbmc_project/Inventory/Manager/server.hpp>

namespace phosphor
{
namespace inventory
{
namespace manager
{

template <typename T>
using ServerObject = T;

using ManagerIface =
    sdbusplus::xyz::openbmc_project::Inventory::server::Manager;

/** @struct PropertiesVariant
 *  @brief Wrapper for sdbusplus PropertiesVariant.
 *
 *  A wrapper is useful since MakeInterface is instantiated with 'int'
 *  to deduce the return type of its methods, which does not depend
 *  on T.
 *
 *  @tparam T - The sdbusplus server binding type.
 */
template <typename T, typename Enable = void>
struct PropertiesVariant
{
};

template <typename T>
struct PropertiesVariant<
    T, typename std::enable_if<std::is_object<T>::value>::type>
{
    using Type = typename T::PropertiesVariant;
};

template <typename T>
using PropertiesVariantType = typename PropertiesVariant<T>::Type;

template <typename T, typename U = int>
struct HasProperties : std::false_type
{
};

template <typename T>
struct HasProperties<
    T, decltype((void)std::declval<typename T::PropertiesVariant>(), 0)>
    : std::true_type
{
};

template <typename T, std::enable_if_t<HasProperties<T>::value, bool> = true>
std::any propMake(sdbusplus::bus::bus& bus, const char* path,
                  const Interface& props)
{
    using InterfaceVariant = std::map<std::string, PropertiesVariantType<T>>;

    InterfaceVariant v;
    for (const auto& p : props)
    {
        v.emplace(p.first, convertVariant<PropertiesVariantType<T>>(p.second));
    }

    return std::any(std::make_shared<T>(bus, path, v));
}

template <typename T, std::enable_if_t<!HasProperties<T>::value, bool> = false>
std::any propMake(sdbusplus::bus::bus& bus, const char* path,
                  const Interface& props)
{
    return std::any(std::make_shared<T>(bus, path));
}

template <typename T, std::enable_if_t<HasProperties<T>::value, bool> = true>
void propAssign(const Interface& props, std::any& holder)
{
    auto& iface = *std::any_cast<std::shared_ptr<T>&>(holder);
    for (const auto& p : props)
    {
        iface.setPropertyByName(
            p.first, convertVariant<PropertiesVariantType<T>>(p.second));
    }
}

template <typename T, std::enable_if_t<!HasProperties<T>::value, bool> = false>
void propAssign(const Interface& props, std::any& holder)
{
}

template <typename T, std::enable_if_t<HasProperties<T>::value, bool> = true>
void propSerialize(const std::string& path, const std::string& iface,
                   const std::any& holder)
{
    const auto& object = *std::any_cast<const std::shared_ptr<T>&>(holder);
    cereal::serialize(path, iface, object);
}

template <typename T, std::enable_if_t<!HasProperties<T>::value, bool> = false>
void propSerialize(const std::string& path, const std::string& iface,
                   const std::any& holder)
{
    cereal::serialize(path, iface);
}

template <typename T, std::enable_if_t<HasProperties<T>::value, bool> = true>
void propDeSerialize(const std::string& path, const std::string& iface,
                     std::any& holder)
{
    auto& object = *std::any_cast<std::shared_ptr<T>&>(holder);
    cereal::deserialize(path, iface, object);
}

template <typename T, std::enable_if_t<!HasProperties<T>::value, bool> = false>
void propDeSerialize(const std::string& path, const std::string& iface,
                     std::any& holder)
{
}

/** @struct MakeInterface
 *  @brief Adapt an sdbusplus interface proxy.
 *
 *  Template instances are builder functions that create
 *  adapted sdbusplus interface proxy interface objects.
 *
 *  @tparam T - The type of the interface being adapted.
 */

template <typename T>
struct MakeInterface
{
    static std::any make(sdbusplus::bus::bus& bus, const char* path,
                         const Interface& props)
    {
        return propMake<T>(bus, path, props);
    }

    static void assign(const Interface& props, std::any& holder)
    {
        propAssign<T>(props, holder);
    }

    static void serialize(const std::string& path, const std::string& iface,
                          const std::any& holder)
    {
        propSerialize<T>(path, iface, holder);
    }

    static void deserialize(const std::string& path, const std::string& iface,
                            std::any& holder)
    {
        propDeSerialize<T>(path, iface, holder);
    }
};

/** @class Manager
 *  @brief OpenBMC inventory manager implementation.
 *
 *  A concrete implementation for the xyz.openbmc_project.Inventory.Manager
 *  DBus API.
 */
class Manager final : public ServerObject<ManagerIface>
{
  public:
    Manager() = delete;
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = default;
    Manager& operator=(Manager&&) = default;
    ~Manager() = default;

    /** @brief Construct an inventory manager.
     *
     *  @param[in] bus - An sdbusplus bus connection.
     *  @param[in] busname - The DBus busname to own.
     *  @param[in] root - The DBus path on which to implement
     *      an inventory manager.
     *  @param[in] iface - The DBus inventory interface to implement.
     */
    Manager(sdbusplus::bus::bus&&, const char*, const char*, const char*);

    using EventInfo =
        std::tuple<std::vector<EventBasePtr>, std::vector<Action>>;

    /** @brief Start processing DBus messages. */
    void run() noexcept;

    /** @brief Provided for testing only. */
    void shutdown() noexcept;

    /** @brief sd_bus Notify method implementation callback. */
    void
        notify(std::map<sdbusplus::message::object_path, Object> objs) override;

    /** @brief Event processing entry point. */
    void handleEvent(sdbusplus::message::message&, const Event& event,
                     const EventInfo& info);

    /** @brief Drop one or more objects from DBus. */
    void destroyObjects(const std::vector<const char*>& paths);

    /** @brief Add objects to DBus. */
    void createObjects(
        const std::map<sdbusplus::message::object_path, Object>& objs);

    /** @brief Add or update objects on DBus. */
    void updateObjects(
        const std::map<sdbusplus::message::object_path, Object>& objs,
        bool restoreFromCache = false);

    /** @brief Restore persistent inventory items */
    void restore();

    /** @brief Invoke an sdbusplus server binding method.
     *
     *  Invoke the requested method with a reference to the requested
     *  sdbusplus server binding interface as a parameter.
     *
     *  @tparam T - The sdbusplus server binding interface type.
     *  @tparam U - The type of the sdbusplus server binding member.
     *  @tparam Args - Argument types of the binding member.
     *
     *  @param[in] path - The DBus path on which the method should
     *      be invoked.
     *  @param[in] interface - The DBus interface hosting the method.
     *  @param[in] member - Pointer to sdbusplus server binding member.
     *  @param[in] args - Arguments to forward to the binding member.
     *
     *  @returns - The return/value type of the binding method being
     *      called.
     */
    template <typename T, typename U, typename... Args>
    decltype(auto) invokeMethod(const char* path, const char* interface,
                                U&& member, Args&&... args)
    {
        auto& iface = getInterface<T>(path, interface);
        return (iface.*member)(std::forward<Args>(args)...);
    }

    using SigArgs = std::vector<std::unique_ptr<
        std::tuple<Manager*, const DbusSignal*, const EventInfo*>>>;
    using SigArg = SigArgs::value_type::element_type;

  private:
    using InterfaceComposite = std::map<std::string, std::any>;
    using ObjectReferences = std::map<std::string, InterfaceComposite>;
    using Events = std::vector<EventInfo>;

    // The int instantiations are safe since the signature of these
    // functions don't change from one instantiation to the next.
    using MakerType = std::add_pointer_t<decltype(MakeInterface<int>::make)>;
    using AssignerType =
        std::add_pointer_t<decltype(MakeInterface<int>::assign)>;
    using SerializerType =
        std::add_pointer_t<decltype(MakeInterface<int>::serialize)>;
    using DeserializerType =
        std::add_pointer_t<decltype(MakeInterface<int>::deserialize)>;
    using Makers =
        std::map<std::string, std::tuple<MakerType, AssignerType,
                                         SerializerType, DeserializerType>>;

    /** @brief Provides weak references to interface holders.
     *
     *  Common code for all types for the templated getInterface
     *  methods.
     *
     *  @param[in] path - The DBus path for which the interface
     *      holder instance should be provided.
     *  @param[in] interface - The DBus interface for which the
     *      holder instance should be provided.
     *
     *  @returns A weak reference to the holder instance.
     */
    const std::any& getInterfaceHolder(const char*, const char*) const;
    std::any& getInterfaceHolder(const char*, const char*);

    /** @brief Provides weak references to interface holders.
     *
     *  @tparam T - The sdbusplus server binding interface type.
     *
     *  @param[in] path - The DBus path for which the interface
     *      should be provided.
     *  @param[in] interface - The DBus interface to obtain.
     *
     *  @returns A weak reference to the interface holder.
     */
    template <typename T>
    auto& getInterface(const char* path, const char* interface)
    {
        auto& holder = getInterfaceHolder(path, interface);
        return *std::any_cast<std::shared_ptr<T>&>(holder);
    }
    template <typename T>
    auto& getInterface(const char* path, const char* interface) const
    {
        auto& holder = getInterfaceHolder(path, interface);
        return *std::any_cast<T>(holder);
    }

    /** @brief Add or update interfaces on DBus. */
    void updateInterfaces(const sdbusplus::message::object_path& path,
                          const Object& interfaces,
                          ObjectReferences::iterator pos,
                          bool emitSignals = true,
                          bool restoreFromCache = false);

    /** @brief Provided for testing only. */
    volatile bool _shutdown;

    /** @brief Path prefix applied to any relative paths. */
    const char* _root;

    /** @brief A container of sdbusplus server interface references. */
    ObjectReferences _refs;

    /** @brief A container contexts for signal callbacks. */
    SigArgs _sigargs;

    /** @brief A container of sdbusplus signal matches.  */
    std::vector<sdbusplus::bus::match_t> _matches;

    /** @brief Persistent sdbusplus DBus bus connection. */
    sdbusplus::bus::bus _bus;

    /** @brief sdbusplus org.freedesktop.DBus.ObjectManager reference. */
    sdbusplus::server::manager::manager _manager;

    /** @brief A container of pimgen generated events and responses.  */
    static const Events _events;

    /** @brief A container of pimgen generated factory methods.  */
    static const Makers _makers;
};

} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
