#include <sdbusplus/server.hpp>
#include <xyz/openbmc_project/Inventory/Manager/server.hpp>

namespace sdbusplus
{
namespace server
{
namespace xyz
{
namespace openbmc_project
{
namespace Inventory
{

Manager::Manager(bus::bus& bus, const char* path)
        : _xyz_openbmc_project_Inventory_Manager_interface(
                bus, path, _interface, _vtable, this)
{
}


int Manager::_callback_Notify(
        sd_bus_message* msg, void* context, sd_bus_error* error)
{
    auto m = message::message(sd_bus_message_ref(msg));

    std::string path{};
    std::map<std::string, std::map<std::string, sdbusplus::message::variant<std::string>>> object{};

    m.read(path, object);

    auto o = static_cast<Manager*>(context);
    o->notify(path, object);

    auto reply = m.new_method_return();
    // No data to append on reply.

    reply.method_return();

    return 0;
}

namespace details
{
namespace Manager
{
static const auto _param_Notify =
        utility::tuple_to_array(message::types::type_id<
                std::string, std::map<std::string, std::map<std::string, sdbusplus::message::variant<std::string>>>>());
}
}



const vtable::vtable_t Manager::_vtable[] = {
    vtable::start(),

    vtable::method("Notify",
                   details::Manager::_param_Notify
                        .data(),
                   nullptr,
                   _callback_Notify),
    vtable::end()
};

} // namespace Inventory
} // namespace openbmc_project
} // namespace xyz
} // namespace server
} // namespace sdbusplus

