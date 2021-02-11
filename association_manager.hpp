#pragma once

#include "config.h"

#include "types.hpp"

#include <nlohmann/json.hpp>
#include <xyz/openbmc_project/Association/Definitions/server.hpp>

#include <any>
#include <filesystem>

namespace phosphor
{
namespace inventory
{
namespace manager
{
namespace associations
{

static constexpr auto forwardTypePos = 0;
static constexpr auto reverseTypePos = 1;
using Types = std::tuple<std::string, std::string>;
using Paths = std::vector<std::string>;

static constexpr auto typesPos = 0;
static constexpr auto pathsPos = 1;
using EndpointsEntry = std::vector<std::tuple<Types, Paths>>;

using AssociationMap = std::map<std::string, EndpointsEntry>;

using AssociationObject = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Association::server::Definitions>;

using AssociationIfaceMap =
    std::map<std::string, std::unique_ptr<AssociationObject>>;

/**
 * @class Manager
 *
 * @brief This class provides the ability to add org.openbmc.Associations
 *        interfaces on inventory D-Bus objects, based on a definition in a
 *        JSON file.
 *
 *        The purpose for this is to be able to associate other D-Bus paths
 *        with the inventory items they relate to.
 *
 *        For example, a card temperature sensor D-Bus object can be associated
 *        with the D-Bus object for that card's inventory entry so that some
 *        code can tie them together.
 */
class Manager
{
  public:
    struct Condition
    {
        std::string path;
        std::string interface;
        std::string property;
        std::vector<InterfaceVariantType> values;
        std::filesystem::path file;
        InterfaceVariantType actualValue;
    };

    Manager() = delete;
    ~Manager() = default;
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;

    /**
     * @brief Constructor
     *
     * @param[in] bus - sdbusplus object
     * @param[in] jsonPath - path to the JSON File that contains associations
     */
    Manager(sdbusplus::bus::bus& bus, const std::string& jsonPath);

    /**
     * @brief Constructor
     *
     * @param[in] bus - sdbusplus object
     */
    explicit Manager(sdbusplus::bus::bus& bus) :
        Manager(bus, ASSOCIATIONS_FILE_PATH)
    {}

    /**
     * @brief Creates any association D-Bus interfaces required based on
     *        the JSON associations definition for the object path passed
     *        in.
     *
     * Called after PIM creates a new inventory D-Bus interface on objectPath.
     *
     * @param[in] objectPath - the D-Bus object path to check for associations
     * @param[in] deferSignal - whether or not to send a Properties or
     *                          ObjectManager signal
     */
    void createAssociations(const std::string& objectPath, bool deferSignal);

    /**
     * @brief Returned the association configuration.
     *        Used for testing.
     *
     * @return AssociationMap& - the association config
     */
    const AssociationMap& getAssociationsConfig()
    {
        return _associations;
    }

    /**
     * @brief Returns the list of conditions
     *
     * @return vector<Condition>& - The conditions
     */
    std::vector<Condition>& getConditions()
    {
        return _conditions;
    }

    /**
     * @brief Says if there are conditions that need to be met
     *        before an associations file is valid.
     *
     * @return bool - If there are pending conditions
     */
    bool pendingCondition() const
    {
        return !_conditions.empty();
    }

    /**
     * @brief Checks if a pending condition is satisfied based on the
     *        path, interface, and property value of the object passed
     *        in.
     *
     *        If it is valid, it will load the associations pointed to
     *        by that condition and erase the _conditions vector as
     *        there are no longer any pending conditions.
     *
     * @param[in] objectPath - The D-Bus path of the object to check
     * @param[in] in object - The interface and properties of the object
     *
     * @return bool - If the object matched a condition
     */
    bool conditionMatch(const sdbusplus::message::object_path& objectPath,
                        const Object& object);

    /**
     * @brief Checks if a pending condition is satisfied based on if the
     *        actualValue field in the condition matches one of the values
     *        in the values field.
     *
     *        The actualValue field was previously set by code based on the
     *        real property value of the specified interface on the specified
     *        path.
     *
     *        If it is valid, it will load the associations pointed to
     *        by that condition and erase the _conditions vector as
     *        there are no longer any pending conditions.
     *
     * @return bool - If a condition was met
     */
    bool conditionMatch();

  private:
    /**
     *  @brief Loads the association JSON into the _associations data
     *         structure.
     *
     *  @param[in] json - The associations JSON
     */
    void load(const nlohmann::json& json);

    /**
     * @brief Creates an instance of an org.openbmc.Associations
     *        interface using the passed in properties.
     *
     * @param[in] forwardPath - the path of the forward association
     * @param[in] forwardType - the type of the forward association
     * @param[in] reversePath - the path of the reverse association
     * @param[in] reverseType - the type of the reverse association
     * @param[in] deferSignal - whether or not to send a Properties or
     *                          ObjectManager signal
     */
    void createAssociation(const std::string& forwardPath,
                           const std::string& forwardType,
                           const std::string& reversePath,
                           const std::string& reverseType, bool deferSignal);

    /**
     * @brief Looks for all JSON files in the associations directory that
     *        contain a valid association condition, and loads the
     *        conditions into the _conditions vector.
     */
    bool loadConditions();

    /**
     * @brief The map of association data that is loaded from its
     *        JSON definition.  Association D-Bus objects will be
     *        created from this data.
     */
    AssociationMap _associations;

    /**
     * @brief The map of org.openbmc_project.Associations D-Bus
     *        interfaces objects based on their object path.
     */
    AssociationIfaceMap _associationIfaces;

    /**
     * @brief The sdbusplus bus object.
     */
    sdbusplus::bus::bus& _bus;

    /**
     * @brief The path to the associations JSON File.
     */
    const std::filesystem::path _jsonFile;

    /**
     * A list of the inventory association paths that have already been handled.
     */
    std::vector<std::string> _handled;

    /**
     * @brief Conditions that specify when an associations file is valid.
     */
    std::vector<Condition> _conditions;
};

} // namespace associations
} // namespace manager
} // namespace inventory
} // namespace phosphor
