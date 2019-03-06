#pragma once

#include "config.h"

#include <sdbusplus/bus.hpp>

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
    {
    }

    /**
     * @brief Creates any association D-Bus interfaces required based on
     *        the JSON associations definition for the object path passed
     *        in.
     *
     * Called after PIM creates a new inventory D-Bus interface on objectPath.
     *
     * @param[in] objectPath - the D-Bus object path to check for associations
     */
    void createAssociations(const std::string& objectPath);

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

  private:
    /**
     *  @brief Loads the association YAML into the _associations data
     *         structure.  This file is optional, so if it doesn't exist
     *         it will just not load anything.
     */
    void load();

    /**
     * @brief The map of association data that is loaded from its
     *        JSON definition.  Association D-Bus objects will be
     *        created from this data.
     */
    AssociationMap _associations;

    /**
     * @brief The sdbusplus bus object.
     */
    sdbusplus::bus::bus& _bus;

    /**
     * @brief The path to the associations JSON File.
     */
    const std::string _jsonFile;
};

} // namespace associations
} // namespace manager
} // namespace inventory
} // namespace phosphor
