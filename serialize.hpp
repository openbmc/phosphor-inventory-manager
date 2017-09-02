#pragma once

#include <cereal/archives/json.hpp>
#include <experimental/filesystem>
#include <fstream>
#include "config.h"

namespace cereal
{

namespace fs = std::experimental::filesystem;

using Path = std::string;
using Interface = std::string;

/** @brief Serialize inventory item
 *
 *  @param[in] path - DBus object path
 *  @param[in] iface - Inventory interface name
 *  @param[in] object - Object to be serialized
 */
template <typename T>
inline void serialize(const Path& path, const Interface& iface, const T& object)
{
    fs::path p(PIM_PERSIST_PATH);
    p /= path;
    fs::create_directories(p);
    p /= iface;
    std::ofstream os(p, std::ios::binary);
    cereal::JSONOutputArchive oarchive(os);
    oarchive(object);
}

/** @brief Serialize inventory item path
 *  Serializing only path for an empty interface to be consistent
 *  interfaces.
 *  @param[in] path - DBus object path
 *  @param[in] iface - Inventory interface name
 */
inline void serialize(const Path& path, const Interface& iface)
{
    fs::path p(PIM_PERSIST_PATH);
    p /= path;
    fs::create_directories(p);
    p /= iface;
    std::ofstream os(p, std::ios::binary);
}

/** @brief Deserialize inventory item
 *
 *  @param[in] path - DBus object path
 *  @param[in] iface - Inventory interface name
 *  @param[in] object - Object to be deserialized
 */
template <typename T>
inline void deserialize(
    const Path& path, const Interface& iface, T& object)
{
    fs::path p(PIM_PERSIST_PATH);
    p /= path;
    p /= iface;
    if (fs::exists(p))
    {
        std::ifstream is(p, std::ios::in | std::ios::binary);
        cereal::JSONInputArchive iarchive(is);
        iarchive(object);
    }
}
} // namespace cereal
