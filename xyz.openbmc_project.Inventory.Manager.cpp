#include <algorithm>
#include <sdbusplus/server.hpp>
#include <sdbusplus/exception.hpp>
#include <xyz/openbmc_project/Inventory/Manager/server.hpp>


namespace sdbusplus
{
namespace xyz
{
namespace openbmc_project
{
namespace Inventory
{
namespace server
{

Manager::Manager(bus::bus& bus, const char* path)
        : _xyz_openbmc_project_Inventory_Manager_interface(
                bus, path, _interface, _vtable, this)
{
}


int Manager::_callback_Notify(
        sd_bus_message* msg, void* context, sd_bus_error* error)
{
    using sdbusplus::server::binding::details::convertForMessage;

    try
    {
        auto m = message::message(msg);

        sdbusplus::message::object_path path{};
    std::map<std::string, std::map<std::string, sdbusplus::message::variant<bool, int64_t, std::string>>> object{};

        m.read(path, object);

        auto o = static_cast<Manager*>(context);
        o->notify(path, object);

        auto reply = m.new_method_return();
        // No data to append on reply.

        reply.method_return();
    }
    catch(sdbusplus::internal_exception_t& e)
    {
        sd_bus_error_set_const(error, e.name(), e.description());
        return -EINVAL;
    }

    return true;
}

namespace details
{
namespace Manager
{
static const auto _param_Notify =
        utility::tuple_to_array(message::types::type_id<
                sdbusplus::message::object_path, std::map<std::string, std::map<std::string, sdbusplus::message::variant<bool, int64_t, std::string>>>>());
static const auto _return_Notify =
        utility::tuple_to_array(std::make_tuple('\0'));
}
}




const vtable::vtable_t Manager::_vtable[] = {
    vtable::start(),

    vtable::method("Notify",
                   details::Manager::_param_Notify
                        .data(),
                   details::Manager::_return_Notify
                        .data(),
                   _callback_Notify),
    vtable::end()
};

} // namespace server
} // namespace Inventory
} // namespace openbmc_project
} // namespace xyz
} // namespace sdbusplus

