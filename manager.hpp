#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <sdbusplus/server.hpp>
#include <xyz/openbmc_project/Inventory/Manager/server.hpp>
#include "filters.hpp"
#include "actions.hpp"

namespace phosphor
{
namespace inventory
{
namespace manager
{
namespace details
{
namespace interface
{
namespace holder
{

/** @struct Base
 *  @brief sdbusplus server interface holder base.
 *
 *  Provides a common type for assembling containers of sdbusplus server
 *  interfaces.  Typically a multiple inheritance scheme (sdbusplus::object)
 *  would be used for this; however, for objects created by PIM the interfaces
 *  are not known at build time.
 */
struct Base
{
    Base() = default;
    virtual ~Base() = default;
    Base(const Base&) = delete;
    Base& operator=(const Base&) = delete;
    Base(Base&&) = delete;
    Base& operator=(Base&&) = delete;
};

/** @struct Holder
 *  @brief sdbusplus server interface holder.
 *
 *  Holds a pointer to an sdbusplus server interface instance.
 *
 *  @tparam T - The sdbusplus server interface type to hold.
 */
template <typename T>
struct Holder final : public Base
{
    Holder() = delete;
    ~Holder() = default;
    Holder(const Holder&) = delete;
    Holder& operator=(const Holder&) = delete;
    Holder& operator=(Holder&&) = delete;
    explicit Holder(auto &&ptr) noexcept : _ptr(std::move(ptr)) {}

    private:
    std::unique_ptr<T> _ptr;
};

/** @struct Make
 *  @brief sdbusplus server interface holder factory method.
 *
 *  @tparam T - The sdbusplus server interface type for which a holder
 *              should be generated.
 */
template <typename T>
struct Make
{
    static auto make(sdbusplus::bus::bus &bus, const char *path)
    {
        return std::unique_ptr<Base>(
                new Holder<T>(
                    std::make_unique<T>(bus, path)));
    }
};

} // namespace holder
} // namespace interface
} // namespace details

/** @class Manager
 *  @brief OpenBMC inventory manager implementation.
 *
 *  A concrete implementation for the xyz.openbmc_project.Inventory.Manager
 *  DBus API.
 */
class Manager final :
    public sdbusplus::server::xyz::openbmc_project::Inventory::Manager
{
    public:
    Manager() = delete;
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = default;
    Manager& operator=(Manager&&) = delete;
    ~Manager() = default;
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
    void signal(sdbusplus::message::message &, auto &);
    void destroyObject(const char *);

    using Event = std::tuple<
        const char *,
        filters::details::Wrapper,
        actions::details::Wrapper>;
    using SigArgs = std::vector<
        std::unique_ptr<
            std::tuple<
                Manager *,
                const Event *>>>;
    using SigArg = SigArgs::value_type::element_type;

    private:
    using Holder = details::interface::holder::Base;
    using HolderPtr = std::unique_ptr<Holder>;
    using InterfaceComposite = std::map<std::string, HolderPtr>;
    using ObjectReferences = std::map<std::string, InterfaceComposite>;
    using Events = std::map<const char *, Event>;
    using MakerType = HolderPtr(*)(
            sdbusplus::bus::bus &, const char *);
    using Makers = std::map<std::string, MakerType>;

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
