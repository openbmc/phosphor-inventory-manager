#pragma once

#include "config.h"

#include <cereal/archives/json.hpp>
#include <experimental/filesystem>
#include <fstream>
#include <phosphor-logging/log.hpp>

namespace phosphor
{
namespace inventory
{
namespace manager
{

namespace fs = std::experimental::filesystem;

struct SerialOps
{
    /** @brief Serialize inventory item path
     *  Serializing only path for an empty interface to be consistent
     *  interfaces.
     *  @param[in] path - DBus object path
     *  @param[in] iface - Inventory interface name
     */
    static void serialize(const std::string& path, const std::string& iface)
    {
        fs::path p(PIM_PERSIST_PATH);
        p /= path;
        fs::create_directories(p);
        p /= iface;
        std::ofstream os(p, std::ios::binary);
    }

    /** @brief Serialize inventory item
     *
     *  @param[in] path - DBus object path
     *  @param[in] iface - Inventory interface name
     *  @param[in] object - Object to be serialized
     */
    template <typename T>
    static void serialize(const std::string& path, const std::string& iface,
                          const T& object)
    {
        fs::path p(PIM_PERSIST_PATH);
        p /= path;
        fs::create_directories(p);
        p /= iface;
        std::ofstream os(p, std::ios::binary);
        cereal::JSONOutputArchive oarchive(os);
        oarchive(object);
    }

    static void deserialize(const std::string&, const std::string&)
    {
        // This is intentionally a noop.
    }

    /** @brief Deserialize inventory item
     *
     *  @param[in] path - DBus object path
     *  @param[in] iface - Inventory interface name
     *  @param[in] object - Object to be serialized
     */
    template <typename T>
    static void deserialize(const std::string& path, const std::string& iface,
                            T& object)
    {
        fs::path p(PIM_PERSIST_PATH);
        p /= path;
        p /= iface;
        try
        {
            if (fs::exists(p))
            {
                std::ifstream is(p, std::ios::in | std::ios::binary);
                cereal::JSONInputArchive iarchive(is);
                iarchive(object);
            }
        }
        catch (cereal::Exception& e)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(e.what());
            fs::remove(p);
        }
    }
};
} // namespace manager
} // namespace inventory
} // namespace phosphor
