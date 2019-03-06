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
    _bus(bus), jsonFile(jsonPath)
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
        // Load the contents of jsonFile into _associations.

        if (!std::filesystem::exists(jsonFile))
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
            entry("FILE=%s", jsonFile.c_str()), entry("ERROR=%s", e.what()));
        _associations.clear();
    }
}

bool Manager::exists(const std::string& objectPath)
{
    try
    {
        auto method = _bus.new_method_call("xyz.openbmc_project.ObjectMapper",
                                           "/xyz/openbmc_project/object_mapper",
                                           "xyz.openbmc_project.ObjectMapper",
                                           "GetObject");
        method.append(objectPath, std::vector<std::string>{});
        auto response = _bus.call(method);

        std::map<std::string, std::vector<std::string>> object;
        response.read(object);

        if (!object.empty())
        {
            return true;
        }
    }
    catch (SdBusError& e)
    {
    }

    return false;
}

void Manager::createAssociations(const std::string& objectPath)
{
    auto endpoints = _associations.find(objectPath);
    if (endpoints == _associations.end())
    {
        return;
    }

    if (std::find(_handled.begin(), _handled.end(), objectPath) !=
        _handled.end())
    {
        return;
    }

    _handled.push_back(objectPath);

    for (const auto& endpoint : endpoints->second)
    {
        const auto& types = std::get<typesPos>(endpoint);
        const auto& paths = std::get<pathsPos>(endpoint);

        for (const auto& endpointPath : paths)
        {
            if (exists(endpointPath))
            {
                const auto& forwardType = std::get<forwardTypePos>(types);
                const auto& reverseType = std::get<reverseTypePos>(types);

                createAssociation(objectPath, forwardType, endpointPath,
                                  reverseType);
            }
            else
            {
                ifacesAddedWatch(endpointPath);
            }
        }
    }
}

void Manager::createAssociation(const std::string& forwardPath,
                                const std::string& forwardType,
                                const std::string& reversePath,
                                const std::string& reverseType)
{
    auto object = _associationIfaces.find(forwardPath);
    if (object == _associationIfaces.end())
    {
        auto a = std::make_unique<AssociationObject>(_bus, forwardPath.c_str(),
                                                     true);

        using AssociationProperty =
            std::vector<std::tuple<std::string, std::string, std::string>>;
        AssociationProperty prop;

        prop.emplace_back(forwardType, reverseType, reversePath);
        a->associations(std::move(prop));
        a->emit_object_added();
        _associationIfaces.emplace(forwardPath, std::move(a));
    }
    else
    {
        // Interface exists, just update the property
        auto prop = object->second->associations();
        prop.emplace_back(forwardType, reverseType, reversePath);
        object->second->associations(std::move(prop));
    }
}

void Manager::ifacesAddedWatch(const std::string& objectPath)
{
    if (_addMatches.find(objectPath) != _addMatches.end())
    {
        return;
    }

    auto match = std::make_unique<sdbusplus::bus::match_t>(
        _bus,
        sdbusplus::bus::match::rules::interfacesAdded() +
            sdbusplus::bus::match::rules::argNpath(0, objectPath),
        std::bind(std::mem_fn(&Manager::interfaceAdded), this,
                  std::placeholders::_1));

    _addMatches.emplace(objectPath, std::move(match));
}

void Manager::interfaceAdded(sdbusplus::message::message& msg)
{
    sdbusplus::message::object_path objectPath;
    msg.read(objectPath);

    // The path that was just added will be in an endpoints list.
    // Find it and create the association object.

    for (const auto& [invPath, endpoints] : _associations)
    {
        for (const auto& endpoint : endpoints)
        {
            const auto& paths = std::get<pathsPos>(endpoint);
            auto path = std::find(paths.begin(), paths.end(), objectPath.str);

            if (path != paths.end())
            {
                const auto& types = std::get<typesPos>(endpoint);
                createAssociation(invPath, std::get<forwardTypePos>(types),
                                  *path, std::get<reverseTypePos>(types));
            }
        }
    }

    _addMatches.erase(objectPath);
}

} // namespace associations
} // namespace manager
} // namespace inventory
} // namespace phosphor
