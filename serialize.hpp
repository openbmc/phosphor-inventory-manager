#pragma once

#include <cereal/archives/binary.hpp>
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
 *  @param[in] - DBus object path
 *  @iface[in] - Inventory interface name
 *  @object[in] - Object to be serialized
 */
template <typename T>
inline void serialize(const Path& path, const Interface& iface, const T& object)
{
    fs::path p(PIM_PERSIST_PATH);
    p /= path;
    fs::create_directories(p);
    p /= iface;
    std::ofstream os(p, std::ios::binary);
    cereal::BinaryOutputArchive oarchive(os);
    oarchive(object);
}

/** @brief Deserialize inventory item
 *
 *  @param[in] - DBus object path
 *  @iface[in] - Inventory interface name
 *  @object[in] - Object to be deserialized
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
        cereal::BinaryInputArchive iarchive(is);
        iarchive(object);
    }
}

} // namespace cereal
