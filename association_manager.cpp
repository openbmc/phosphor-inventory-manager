#include "config.h"

#include "association_manager.hpp"

#include <experimental/filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace inventory
{
namespace manager
{
namespace associations
{
using namespace phosphor::logging;
using sdbusplus::exception::SdBusError;

Manager::Manager(sdbusplus::bus::bus& bus, const std::string& jsonPath) :
    _bus(bus), jsonFile(jsonPath)
{
    load();
}

void Manager::load()
{
    try
    {
        // Load the contents of jsonFile into _associations.

        if (!std::experimental::filesystem::exists(jsonFile))
        {
            using namespace phosphor::logging;
            log<level::ERR>("Could not find associations file",
                            entry("FILE=%s", jsonFile.c_str()));
            return;
        }

        std::ifstream file{jsonFile};

        auto json = nlohmann::json::parse(file, nullptr, true);

        const std::string root{INVENTORY_ROOT};

        for (const auto& jsonAssoc : json)
        {
            // Only add the slash if necessary
            std::string path = jsonAssoc["path"];
            if (path.front() != '/')
            {
                path = root + "/" + path;
            }
            else
            {
                path = root + path;
            }

            auto& assocEndpoints = _associations[path];

            for (const auto& endpoint : jsonAssoc["endpoints"])
            {
                Types types{endpoint["types"]["fType"],
                            endpoint["types"]["rType"]};

                Paths paths = endpoint["paths"];
                assocEndpoints.emplace_back(std::move(types), std::move(paths));
            }
        }
    }
    catch (std::exception& e)
    {
        log<level::ERR>(
            "Failed loading associations json file. Not creating associations",
            entry("FILE=%s", jsonFile.c_str()), entry("ERROR=%s", e.what()));
    }
}

void Manager::createAssociations(const std::string& objectPath)
{
    // TODO
}
} // namespace associations
} // namespace manager
} // namespace inventory
} // namespace phosphor
