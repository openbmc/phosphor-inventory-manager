// This file was auto generated.  Do not edit.

#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <xyz/openbmc_project/Inventory/Item/server.hpp>
#include <xyz/openbmc_project/Inventory/Decorator/Asset/server.hpp>
#include <xyz/openbmc_project/Inventory/Decorator/Revision/server.hpp>
#include <xyz/openbmc_project/Inventory/Item/NetworkInterface/server.hpp>
#include <xyz/openbmc_project/Example/Iface2/server.hpp>
#include <xyz/openbmc_project/Example/Iface1/server.hpp>

namespace cereal
{

template<class Archive>
void save(Archive& a,
          const sdbusplus::xyz::openbmc_project::Inventory::server::Item& object)
{
    a(object.prettyName(), object.present());
}


template<class Archive>
void load(Archive& a,
          sdbusplus::xyz::openbmc_project::Inventory::server::Item& object)
{
    decltype(object.prettyName()) PrettyName{};
    decltype(object.present()) Present{};
    a(PrettyName, Present);
    object.prettyName(PrettyName);
    object.present(Present);
}

template<class Archive>
void save(Archive& a,
          const sdbusplus::xyz::openbmc_project::Inventory::Decorator::server::Asset& object)
{
    a(object.partNumber(), object.serialNumber(), object.manufacturer(), object.buildDate(), object.model());
}


template<class Archive>
void load(Archive& a,
          sdbusplus::xyz::openbmc_project::Inventory::Decorator::server::Asset& object)
{
    decltype(object.partNumber()) PartNumber{};
    decltype(object.serialNumber()) SerialNumber{};
    decltype(object.manufacturer()) Manufacturer{};
    decltype(object.buildDate()) BuildDate{};
    decltype(object.model()) Model{};
    a(PartNumber, SerialNumber, Manufacturer, BuildDate, Model);
    object.partNumber(PartNumber);
    object.serialNumber(SerialNumber);
    object.manufacturer(Manufacturer);
    object.buildDate(BuildDate);
    object.model(Model);
}

template<class Archive>
void save(Archive& a,
          const sdbusplus::xyz::openbmc_project::Inventory::Decorator::server::Revision& object)
{
    a(object.version());
}


template<class Archive>
void load(Archive& a,
          sdbusplus::xyz::openbmc_project::Inventory::Decorator::server::Revision& object)
{
    decltype(object.version()) Version{};
    a(Version);
    object.version(Version);
}

template<class Archive>
void save(Archive& a,
          const sdbusplus::xyz::openbmc_project::Inventory::Item::server::NetworkInterface& object)
{
    a(object.mACAddress());
}


template<class Archive>
void load(Archive& a,
          sdbusplus::xyz::openbmc_project::Inventory::Item::server::NetworkInterface& object)
{
    decltype(object.mACAddress()) MACAddress{};
    a(MACAddress);
    object.mACAddress(MACAddress);
}

template<class Archive>
void save(Archive& a,
          const sdbusplus::xyz::openbmc_project::Example::server::Iface2& object)
{
    a(object.exampleProperty2(), object.exampleProperty3());
}


template<class Archive>
void load(Archive& a,
          sdbusplus::xyz::openbmc_project::Example::server::Iface2& object)
{
    decltype(object.exampleProperty2()) ExampleProperty2{};
    decltype(object.exampleProperty3()) ExampleProperty3{};
    a(ExampleProperty2, ExampleProperty3);
    object.exampleProperty2(ExampleProperty2);
    object.exampleProperty3(ExampleProperty3);
}

template<class Archive>
void save(Archive& a,
          const sdbusplus::xyz::openbmc_project::Example::server::Iface1& object)
{
    a(object.exampleProperty1());
}


template<class Archive>
void load(Archive& a,
          sdbusplus::xyz::openbmc_project::Example::server::Iface1& object)
{
    decltype(object.exampleProperty1()) ExampleProperty1{};
    a(ExampleProperty1);
    object.exampleProperty1(ExampleProperty1);
}

} // namespace cereal
