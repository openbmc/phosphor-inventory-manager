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
     * If there is an association that needs to be created for this path,
     * it will check if the other end of it exists on D-Bus, and then create
     * the association.  If the other end doesn't exist, it will add a watch
     * for interfacesAdded on it.
     *
     * @param[in] objectPath - the D-Bus object path to check for associations
     */
    void createAssociations(const std::string& objectPath);

  private:
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
