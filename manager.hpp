#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <sdbusplus/server.hpp>
#include <xyz/openbmc_project/Inventory/Manager/server.hpp>

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

struct Base
{
    Base() = default;
    virtual ~Base() = default;
    Base(const Base&) = delete;
    Base& operator=(const Base&) = delete;
    Base(Base&&) = delete;
    Base& operator=(Base&&) = delete;
};

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

struct Event
{
    const char *signature;
};

} // namespace details

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

    void run() noexcept;
    void shutdown() noexcept;
    void notify(std::string path, Object) override;
    void signal(sdbusplus::message::message &, auto &);

    using SigArgs = std::vector<
        std::unique_ptr<
            std::tuple<
                Manager *,
                const details::Event *>>>;
    using SigArg = SigArgs::value_type::element_type;

    private:
    using Holder = details::interface::holder::Base;
    using HolderPtr = std::unique_ptr<Holder>;
    using InterfaceComposite = std::map<std::string, HolderPtr>;
    using ObjectReferences = std::map<std::string, InterfaceComposite>;
    using Events = std::map<const char *, details::Event>;

    bool _shutdown;
    const char * _root;
    ObjectReferences _refs;
    SigArgs _sigargs;
    std::vector<sdbusplus::server::match::match> _matches;
    sdbusplus::bus::bus _bus;
    sdbusplus::server::manager::manager _manager;
    static const Events _events;
};

} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
