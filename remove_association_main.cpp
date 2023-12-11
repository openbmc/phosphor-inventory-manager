#include "remove_association.hpp"

int main(int, char*[])
{
    using namespace phosphor::inventory::manager;
    auto bus = sdbusplus::bus::new_default();
    DBusSubtree objTree =
        getInventoryAssociations(bus, "/xyz/openbmc_project/inventory", 0);
    for (const auto& object : objTree)
    {
        const auto& objPath = object.first;
        const auto& service = object.second.begin()->first;
        if (!service.compare("xyz.openbmc_project.Inventory.Manager"))
        {
            removeCriticalAssociation(bus, objPath, service);
        }
    }
    return 0;
}
