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
    cereal::BinaryOutputArchive oarchive(os);
    oarchive(object);
}

} // namespace cereal
