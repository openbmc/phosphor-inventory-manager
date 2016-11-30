#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <sdbusplus/server.hpp>
#include "xyz/openbmc_project/Inventory/Manager/server.hpp"
#include "events.hpp"
#include "actions.hpp"

namespace phosphor
{
namespace inventory
{
namespace manager
{
namespace details
{

template <typename T>
using ServerObject = typename sdbusplus::server::object::object<T>;

using ManagerIface =
    sdbusplus::server::xyz::openbmc_project::Inventory::Manager;

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
    static decltype(auto) make(sdbusplus::bus::bus &bus, const char *path)
    {
        using HolderType = holder::Holder<std::unique_ptr<T>>;
        return static_cast<std::unique_ptr<holder::Base>>(
            HolderType::template make_unique<HolderType>(
                std::forward<std::unique_ptr<T>>(
                    std::make_unique<T>(
                        std::forward<decltype(bus)>(bus),
                        std::forward<decltype(path)>(path)))));
    }
};
} // namespace details

/** @class Manager
 *  @brief OpenBMC inventory manager implementation.
 *
 *  A concrete implementation for the xyz.openbmc_project.Inventory.Manager
 *  DBus API.
 */
class Manager final :
    public details::ServerObject<details::ManagerIface>
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
    Manager(sdbusplus::bus::bus &&, const char *, const char*, const char*);

    using Object = std::map<
        std::string, std::map<
            std::string, sdbusplus::message::variant<std::string>>>;

    /** @brief Start processing DBus messages. */
    void run() noexcept;

    /** @brief Provided for testing only. */
    void shutdown() noexcept;

    /** @brief sd_bus Notify method implementation callback. */
    void notify(std::string path, Object) override;

    /** @brief sd_bus signal callback. */
    void signal(sdbusplus::message::message &, auto &, auto &);

    /** @brief Drop an object from DBus. */
    void destroyObject(const char *);

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
     */
    template<typename T, typename U, typename ...Args>
    decltype(auto) invokeMethod(const char *path, const char *interface,
            U&& member, Args&&...args)
    {
        auto &holder = getInterface<std::unique_ptr<T>>(path, interface);
        auto &iface = *holder.get();
        return (iface.*member)(std::forward<Args>(args)...);
    }

    using EventInfo = std::tuple<
        std::vector<details::EventBasePtr>,
        std::vector<details::ActionBasePtr>>;
    using SigArgs = std::vector<
        std::unique_ptr<
            std::tuple<
                Manager *,
                const details::DbusSignal *,
                const EventInfo *>>>;
    using SigArg = SigArgs::value_type::element_type;

    private:
    using HolderPtr = std::unique_ptr<details::holder::Base>;
    using InterfaceComposite = std::map<std::string, HolderPtr>;
    using ObjectReferences = std::map<std::string, InterfaceComposite>;
    using Events = std::vector<EventInfo>;
    using MakerType = HolderPtr(*)(
            sdbusplus::bus::bus &, const char *);
    using Makers = std::map<std::string, MakerType>;

    /** @brief Provides weak references to interface holders.
     *
     *  @param[in] path - The DBus path for which the interface
     *      holder instance should be provided.
     *  @param[in] interface - The DBus interface for which the
     *      holder instance should be provided.
     *
     *  @returns A weak reference to the holder instance.
     */
    details::holder::Base& getInterfaceHolder(
            const char *, const char *) const;
    details::holder::Base& getInterfaceHolder(
            const char *, const char *);

    /** @brief Provides weak references to interface holder.
     *
     *  @tparam T - The sdbusplus server binding interface type.
     *
     *  @param[in] path - The DBus path for which the interface
     *      should be provided.
     *  @param[in] interface - The DBus interface to obtain.
     *
     *  @returns A weak reference to the interface.
     */
    template<typename T>
    decltype(auto) getInterface(const char *path, const char *interface)
    {
        auto &holder = getInterfaceHolder(path, interface);
        return static_cast<
            details::holder::Holder<T> &>(holder);
    }
    template<typename T>
    decltype(auto) getInterface(const char *path, const char *interface) const
    {
        auto &holder = getInterfaceHolder(path, interface);
        return static_cast<
            const details::holder::Holder<T> &>(holder);
    }

    /** @brief Provided for testing only. */
    bool _shutdown;

    /** @brief Path prefix applied to any relative paths. */
    const char * _root;

    /** @brief A container of sdbusplus server interface references. */
    ObjectReferences _refs;

    /** @brief A container contexts for signal callbacks. */
    SigArgs _sigargs;

    /** @brief A container of sdbusplus signal matches.  */
    std::vector<sdbusplus::server::match::match> _matches;

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
