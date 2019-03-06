#include "association_manager.hpp"

#include <filesystem>
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
    _bus(bus), _jsonFile(jsonPath)
{
    load();
}

/**
 * @brief Throws an exception if 'num' is zero. Used for JSON
 *        sanity checking.
 *
 * @param[in] num - the number to check
 */
void throwIfZero(int num)
{
    if (!num)
    {
        throw std::runtime_error("Invalid empty field in JSON");
    }
}

void Manager::load()
{
    try
    {
        // Load the contents of _jsonFile into _associations.

        if (!std::filesystem::exists(_jsonFile))
        {
            using namespace phosphor::logging;
            log<level::ERR>("Could not find associations file",
                            entry("FILE=%s", _jsonFile.c_str()));
            return;
        }

        std::ifstream file{_jsonFile};

        auto json = nlohmann::json::parse(file, nullptr, true);

        const std::string root{INVENTORY_ROOT};

        for (const auto& jsonAssoc : json)
        {
            // Only add the slash if necessary
            std::string path = jsonAssoc.at("path");
            throwIfZero(path.size());
            if (path.front() != '/')
            {
                path = root + "/" + path;
            }
            else
            {
                path = root + path;
            }

            auto& assocEndpoints = _associations[path];

            for (const auto& endpoint : jsonAssoc.at("endpoints"))
            {
                std::string ftype = endpoint.at("types").at("fType");
                std::string rtype = endpoint.at("types").at("rType");
                throwIfZero(ftype.size());
                throwIfZero(rtype.size());
                Types types{ftype, rtype};

                Paths paths = endpoint.at("paths");
                throwIfZero(paths.size());
                assocEndpoints.emplace_back(std::move(types), std::move(paths));
            }
        }
    }
    catch (std::exception& e)
    {
        log<level::ERR>(
            "Failed loading associations json file. Not creating associations",
            entry("FILE=%s", _jsonFile.c_str()), entry("ERROR=%s", e.what()));
        _associations.clear();
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
