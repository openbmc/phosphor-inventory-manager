#pragma once
#include <tuple>
#include <systemd/sd-bus.h>
#include <sdbusplus/server.hpp>

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

class Manager
{
    public:
        /* Define all of the basic class operations:
         *     Not allowed:
         *         - Default constructor to avoid nullptrs.
         *         - Copy operations due to internal unique_ptr.
         *     Allowed:
         *         - Move operations.
         *         - Destructor.
         */
        Manager() = delete;
        Manager(const Manager&) = delete;
        Manager& operator=(const Manager&) = delete;
        Manager(Manager&&) = default;
        Manager& operator=(Manager&&) = default;
        virtual ~Manager() = default;

        /** @brief Constructor to put object onto bus at a dbus path.
         *  @param[in] bus - Bus to attach to.
         *  @param[in] path - Path to attach at.
         */
        Manager(bus::bus& bus, const char* path);


        /** @brief Implementation for Notify
         *  Signal the implementing service that an item is ready to have its state managed.
         *
         *  @param[in] path - The path of the item to be managed, relative to the inventory namespace root.
         *  @param[in] object - The fully enumerated item to be managed.
         */
        virtual void notify(
            std::string path,
            std::map<std::string, std::map<std::string, sdbusplus::message::variant<std::string>>> object) = 0;



    private:

        /** @brief sd-bus callback for Notify
         */
        static int _callback_Notify(
            sd_bus_message*, void*, sd_bus_error*);


        static constexpr auto _interface = "xyz.openbmc_project.Inventory.Manager";
        static const vtable::vtable_t _vtable[];
        interface::interface _xyz_openbmc_project_Inventory_Manager_interface;


};

} // namespace Inventory
} // namespace openbmc_project
} // namespace xyz
} // namespace server
} // namespace sdbusplus

