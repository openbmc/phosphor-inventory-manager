#include "association_manager.hpp"

#include <phosphor-logging/lg2.hpp>

#include <filesystem>
#include <fstream>

namespace phosphor
{
namespace inventory
{
namespace manager
{
namespace associations
{
namespace fs = std::filesystem;

Manager::Manager(sdbusplus::bus_t& bus, const std::string& jsonPath) :
    _bus(bus), _jsonFile(jsonPath)
{
    // If there aren't any conditional associations files, look for
    // that default nonconditional one.
    if (!loadConditions())
    {
        if (fs::exists(_jsonFile))
        {
            std::ifstream file{_jsonFile};
            auto json = nlohmann::json::parse(file, nullptr, true);
            load(json);
        }
    }
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
        throw std::invalid_argument("Invalid empty field in JSON");
    }
}

bool Manager::loadConditions()
{
    auto dir = _jsonFile.parent_path();

    for (const auto& dirent : fs::recursive_directory_iterator(dir))
    {
        const auto& path = dirent.path();
        if (path.extension() == ".json")
        {
            std::ifstream file{path};
            auto json = nlohmann::json::parse(file, nullptr, true);

            if (json.is_object() && json.contains("condition"))
            {
                const auto& conditionJSON = json.at("condition");
                if (!conditionJSON.contains("path") ||
                    !conditionJSON.contains("interface") ||
                    !conditionJSON.contains("property") ||
                    !conditionJSON.contains("values"))
                {
                    lg2::error(
                        "Invalid JSON in associations condition entry in {PATH}. Skipping file.",
                        "PATH", path);
                    continue;
                }

                Condition c;
                c.file = path;
                c.path = conditionJSON["path"].get<std::string>();
                if (c.path.front() != '/')
                {
                    c.path = '/' + c.path;
                }
                fprintf(stderr, "found conditions file %s\n", c.file.c_str());
                c.interface = conditionJSON["interface"].get<std::string>();
                c.property = conditionJSON["property"].get<std::string>();

                // The values are in an array, and need to be
                // converted to an InterfaceVariantType.
                for (const auto& value : conditionJSON["values"])
                {
                    if (value.is_array())
                    {
                        std::vector<uint8_t> variantValue;
                        for (const auto& v : value)
                        {
                            variantValue.push_back(v.get<uint8_t>());
                        }
                        c.values.push_back(variantValue);
                        continue;
                    }

                    // Try the remaining types
                    auto s = value.get_ptr<const std::string*>();
                    auto i = value.get_ptr<const int64_t*>();
                    auto b = value.get_ptr<const bool*>();
                    if (s)
                    {
                        c.values.push_back(*s);
                    }
                    else if (i)
                    {
                        c.values.push_back(*i);
                    }
                    else if (b)
                    {
                        c.values.push_back(*b);
                    }
                    else
                    {
                        lg2::error(
                            "Invalid condition property value in {FILE}:",
                            "FILE", c.file);
                        throw std::runtime_error(
                            "Invalid condition property value");
                    }
                }

                _conditions.push_back(std::move(c));
            }
        }
    }

    return !_conditions.empty();
}

bool Manager::conditionMatch(const sdbusplus::message::object_path& objectPath,
                             const Object& object)
{
    fs::path foundPath;
    for (const auto& condition : _conditions)
    {
        if (condition.path != objectPath)
        {
            continue;
        }

        auto interface = std::find_if(object.begin(), object.end(),
                                      [&condition](const auto& i) {
            return i.first == condition.interface;
        });
        if (interface == object.end())
        {
            continue;
        }

        auto property = std::find_if(interface->second.begin(),
                                     interface->second.end(),
                                     [&condition](const auto& p) {
            return condition.property == p.first;
        });
        if (property == interface->second.end())
        {
            continue;
        }

        auto match = std::find(condition.values.begin(), condition.values.end(),
                               property->second);
        if (match != condition.values.end())
        {
            foundPath = condition.file;
            break;
        }
    }

    if (!foundPath.empty())
    {
        std::ifstream file{foundPath};
        auto json = nlohmann::json::parse(file, nullptr, true);
        load(json["associations"]);
        _conditions.clear();
        return true;
    }

    return false;
}

bool Manager::conditionMatch()
{
    fs::path foundPath;

    for (const auto& condition : _conditions)
    {
        // Compare the actualValue field against the values in the
        // values vector to see if there is a condition match.
        auto found = std::find(condition.values.begin(), condition.values.end(),
                               condition.actualValue);
        if (found != condition.values.end())
        {
            foundPath = condition.file;
            break;
        }
    }

    if (!foundPath.empty())
    {
        std::ifstream file{foundPath};
        auto json = nlohmann::json::parse(file, nullptr, true);
        load(json["associations"]);
        _conditions.clear();
        return true;
    }

    return false;
}

void Manager::load(const nlohmann::json& json)
{
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
            Types types{std::move(ftype), std::move(rtype)};

            Paths paths = endpoint.at("paths");
            throwIfZero(paths.size());
            assocEndpoints.emplace_back(std::move(types), std::move(paths));
        }
    }
}

void Manager::createAssociations(const std::string& objectPath,
                                 bool deferSignal)
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
            const auto& forwardType = std::get<forwardTypePos>(types);
            const auto& reverseType = std::get<reverseTypePos>(types);

            createAssociation(objectPath, forwardType, endpointPath,
                              reverseType, deferSignal);
        }
    }
}

void Manager::createAssociation(const std::string& forwardPath,
                                const std::string& forwardType,
                                const std::string& reversePath,
                                const std::string& reverseType,
                                bool deferSignal)
{
    auto object = _associationIfaces.find(forwardPath);
    if (object == _associationIfaces.end())
    {
        auto a = std::make_unique<AssociationObject>(
            _bus, forwardPath.c_str(), AssociationObject::action::defer_emit);

        using AssociationProperty =
            std::vector<std::tuple<std::string, std::string, std::string>>;
        AssociationProperty prop;

        prop.emplace_back(forwardType, reverseType, reversePath);
        a->associations(std::move(prop));
        if (!deferSignal)
        {
            a->emit_object_added();
        }
        _associationIfaces.emplace(forwardPath, std::move(a));
    }
    else
    {
        // Interface exists, just update the property
        auto prop = object->second->associations();
        prop.emplace_back(forwardType, reverseType, reversePath);
        object->second->associations(std::move(prop), deferSignal);
    }
}
} // namespace associations
} // namespace manager
} // namespace inventory
} // namespace phosphor
