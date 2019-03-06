#include "config.h"

#include "association_manager.hpp"

namespace phosphor
{
namespace inventory
{
namespace manager
{
namespace associations
{

Manager::Manager(sdbusplus::bus::bus& bus, const std::string& jsonPath) :
    _bus(bus), jsonFile(jsonPath)
{
}

void Manager::createAssociations(const std::string& objectPath)
{
    // TODO
}
} // namespace associations
} // namespace manager
} // namespace inventory
} // namespace phosphor
