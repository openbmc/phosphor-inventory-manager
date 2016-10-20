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

struct Interface
{
    Interface() = default;
    virtual ~Interface() = default;
    Interface(const Interface&) = delete;
    Interface& operator=(const Interface&) = delete;
    Interface(Interface&&) = delete;
    Interface& operator=(Interface&&) = delete;
};

template <typename T>
struct _Interface final : public Interface
{
    _Interface() = delete;
    ~_Interface() = default;
    _Interface(const _Interface&) = delete;
    _Interface& operator=(const _Interface&) = delete;
    _Interface& operator=(_Interface&&) = delete;
    explicit _Interface(auto &&ptr) noexcept : _ptr(std::move(ptr)) {}

    private:
    std::unique_ptr<T> _ptr;
};

template <typename T>
struct MakeInterface
{
    static auto make(sdbusplus::bus::bus &bus, const char *path)
    {
        return std::unique_ptr<Interface>(
                new _Interface<T>(
                    std::make_unique<T>(bus, path)));
    }
};

struct Event
{
    const char *signature;
    filters::details::Holder filter;
    actions::details::Holder action;
};

} // namespace details

struct Manager final :
    public sdbusplus::server::xyz::openbmc_project::Inventory::Manager
{
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
    void destroyObject(const char *);

    using SigArgs = std::vector<
        std::unique_ptr<
            std::tuple<
                Manager *,
                const details::Event *>>>;
    using SigArg = SigArgs::value_type::element_type;

    private:
    using InterfaceComposite = std::map<
        std::string,
        std::unique_ptr<details::Interface>>;
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
