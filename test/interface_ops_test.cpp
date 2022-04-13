#include "../interface_ops.hpp"

#include <sdbusplus/test/sdbus_mock.hpp>

#include <gtest/gtest.h>

using namespace phosphor::inventory::manager;
using namespace testing;
using namespace std::string_literals;

struct MockInterface;
struct DummyInterfaceWithProperties;

MockInterface* g_currentMock = nullptr;

using FakeVariantType = int;
using InterfaceVariant = std::map<std::string, FakeVariantType>;

struct MockInterface
{
    MockInterface()
    {
        g_currentMock = this;
    }
    ~MockInterface()
    {
        g_currentMock = nullptr;
    }
    MockInterface(const MockInterface&) = delete;
    MockInterface& operator=(const MockInterface&) = delete;
    // Not supporting move semantics simply because they aren't needed.
    MockInterface(MockInterface&&) = delete;
    MockInterface& operator=(MockInterface&&) = delete;

    // We'll be getting calls proxyed through other objects.
    MOCK_METHOD3(constructWithProperties,
                 void(const char*, const InterfaceVariant& i, bool));
    MOCK_METHOD1(constructWithoutProperties, void(const char*));
    MOCK_METHOD3(setPropertyByName, void(std::string, FakeVariantType, bool));

    MOCK_METHOD2(serializeTwoArgs,
                 void(const std::string&, const std::string&));
    MOCK_METHOD3(serializeThreeArgs,
                 void(const std::string&, const std::string&,
                      const DummyInterfaceWithProperties&));

    MOCK_METHOD0(deserializeNoop, void());
    MOCK_METHOD3(deserializeThreeArgs,
                 void(const std::string&, const std::string&,
                      DummyInterfaceWithProperties&));
};

struct DummyInterfaceWithoutProperties
{
    DummyInterfaceWithoutProperties(sdbusplus::bus::bus&, const char* name)
    {
        g_currentMock->constructWithoutProperties(name);
    }
};

struct DummyInterfaceWithProperties
{
    using PropertiesVariant = FakeVariantType;

    DummyInterfaceWithProperties(sdbusplus::bus::bus&, const char* name,
                                 const InterfaceVariant& i, bool skipSignal)
    {
        g_currentMock->constructWithProperties(name, i, skipSignal);
    }

    void setPropertyByName(std::string name, PropertiesVariant val,
                           bool skipSignal)
    {
        g_currentMock->setPropertyByName(name, val, skipSignal);
    }
};

struct SerialForwarder
{
    static void serialize(const std::string& path, const std::string& iface)
    {
        g_currentMock->serializeTwoArgs(path, iface);
    }

    static void serialize(const std::string& path, const std::string& iface,
                          const DummyInterfaceWithProperties& obj)
    {
        g_currentMock->serializeThreeArgs(path, iface, obj);
    }

    static void deserialize(const std::string& /* path */,
                            const std::string& /* iface */)
    {
        g_currentMock->deserializeNoop();
    }

    static void deserialize(const std::string& path, const std::string& iface,
                            DummyInterfaceWithProperties& obj)
    {
        g_currentMock->deserializeThreeArgs(path, iface, obj);
    }
};

TEST(InterfaceOpsTest, TestHasPropertiesNoProperties)
{
    EXPECT_FALSE(HasProperties<DummyInterfaceWithoutProperties>::value);
}

TEST(InterfaceOpsTest, TestHasPropertiesHasProperties)
{
    EXPECT_TRUE(HasProperties<DummyInterfaceWithProperties>::value);
}

TEST(InterfaceOpsTest, TestMakePropertylessInterfaceWithoutArguments)
{
    MockInterface mock;
    Interface i;
    sdbusplus::SdBusMock interface;

    EXPECT_CALL(mock, constructWithoutProperties("foo")).Times(1);
    EXPECT_CALL(mock, constructWithProperties(_, _, _)).Times(0);

    auto b = sdbusplus::get_mocked_new(&interface);
    auto r =
        MakeInterface<DummyInterfaceWithoutProperties>::op(b, "foo", i, false);

    EXPECT_NO_THROW(
        std::any_cast<std::shared_ptr<DummyInterfaceWithoutProperties>>(r));
}

TEST(InterfaceOpsTest, TestMakePropertylessInterfaceWithOneArgument)
{
    MockInterface mock;
    Interface i{{"foo"s, static_cast<int64_t>(1ll)}};
    sdbusplus::SdBusMock interface;

    EXPECT_CALL(mock, constructWithoutProperties("foo")).Times(1);
    EXPECT_CALL(mock, constructWithProperties(_, _, _)).Times(0);

    auto b = sdbusplus::get_mocked_new(&interface);
    auto r =
        MakeInterface<DummyInterfaceWithoutProperties>::op(b, "foo", i, false);

    EXPECT_NO_THROW(
        std::any_cast<std::shared_ptr<DummyInterfaceWithoutProperties>>(r));
}

TEST(InterfaceOpsTest, TestMakeInterfaceWithWithoutArguments)
{
    MockInterface mock;
    Interface i;
    sdbusplus::SdBusMock interface;

    EXPECT_CALL(mock, constructWithoutProperties(_)).Times(0);
    EXPECT_CALL(mock, constructWithProperties("bar", _, _)).Times(1);

    auto b = sdbusplus::get_mocked_new(&interface);
    auto r =
        MakeInterface<DummyInterfaceWithProperties>::op(b, "bar", i, false);

    EXPECT_NO_THROW(
        std::any_cast<std::shared_ptr<DummyInterfaceWithProperties>>(r));
}

TEST(InterfaceOpsTest, TestMakeInterfaceWithOneArgument)
{
    MockInterface mock;
    Interface i{{"foo"s, static_cast<int64_t>(1ll)}};
    sdbusplus::SdBusMock interface;

    EXPECT_CALL(mock, constructWithoutProperties(_)).Times(0);
    EXPECT_CALL(mock, constructWithProperties("foo", _, _)).Times(1);

    auto b = sdbusplus::get_mocked_new(&interface);
    auto r =
        MakeInterface<DummyInterfaceWithProperties>::op(b, "foo", i, false);

    EXPECT_NO_THROW(
        std::any_cast<std::shared_ptr<DummyInterfaceWithProperties>>(r));
}

TEST(InterfaceOpsTest, TestAssignPropertylessInterfaceWithoutArguments)
{
    MockInterface mock;
    Interface i;
    sdbusplus::SdBusMock interface;

    EXPECT_CALL(mock, setPropertyByName(_, _, _)).Times(0);

    auto b = sdbusplus::get_mocked_new(&interface);
    auto r =
        MakeInterface<DummyInterfaceWithoutProperties>::op(b, "foo", i, false);

    AssignInterface<DummyInterfaceWithoutProperties>::op(i, r, false);
}

TEST(InterfaceOpsTest, TestAssignPropertylessInterfaceWithOneArgument)
{
    MockInterface mock;
    Interface i{{"foo"s, static_cast<int64_t>(1ll)}};
    sdbusplus::SdBusMock interface;

    EXPECT_CALL(mock, setPropertyByName(_, _, _)).Times(0);

    auto b = sdbusplus::get_mocked_new(&interface);
    auto r =
        MakeInterface<DummyInterfaceWithoutProperties>::op(b, "foo", i, false);

    AssignInterface<DummyInterfaceWithoutProperties>::op(i, r, false);
}

TEST(InterfaceOpsTest, TestAssignInterfaceWithoutArguments)
{
    MockInterface mock;
    Interface i;
    sdbusplus::SdBusMock interface;

    EXPECT_CALL(mock, setPropertyByName(_, _, _)).Times(0);

    auto b = sdbusplus::get_mocked_new(&interface);
    auto r =
        MakeInterface<DummyInterfaceWithProperties>::op(b, "foo", i, false);

    AssignInterface<DummyInterfaceWithProperties>::op(i, r, false);
}

TEST(InterfaceOpsTest, TestAssignInterfaceWithOneArgument)
{
    MockInterface mock;
    Interface i{{"foo"s, static_cast<int64_t>(1ll)}};
    sdbusplus::SdBusMock interface;

    EXPECT_CALL(mock, setPropertyByName("foo"s, 1ll, _)).Times(1);

    auto b = sdbusplus::get_mocked_new(&interface);
    auto r =
        MakeInterface<DummyInterfaceWithProperties>::op(b, "bar", i, false);

    AssignInterface<DummyInterfaceWithProperties>::op(i, r, false);
}

TEST(InterfaceOpsTest, TestSerializePropertylessInterfaceWithoutArguments)
{
    MockInterface mock;
    Interface i;
    sdbusplus::SdBusMock interface;

    auto b = sdbusplus::get_mocked_new(&interface);
    auto r =
        MakeInterface<DummyInterfaceWithoutProperties>::op(b, "foo", i, false);

    EXPECT_CALL(mock, serializeTwoArgs("/foo"s, "bar"s)).Times(1);

    SerializeInterface<DummyInterfaceWithoutProperties, SerialForwarder>::op(
        "/foo"s, "bar"s, r);
}

TEST(InterfaceOpsTest, TestSerializePropertylessInterfaceWithOneArgument)
{
    MockInterface mock;
    Interface i{{"foo"s, static_cast<int64_t>(1ll)}};
    sdbusplus::SdBusMock interface;

    auto b = sdbusplus::get_mocked_new(&interface);
    auto r =
        MakeInterface<DummyInterfaceWithoutProperties>::op(b, "foo", i, false);

    EXPECT_CALL(mock, serializeTwoArgs("/foo"s, "bar"s)).Times(1);

    SerializeInterface<DummyInterfaceWithoutProperties, SerialForwarder>::op(
        "/foo"s, "bar"s, r);
}

TEST(InterfaceOpsTest, TestSerializeInterfaceWithNoArguments)
{
    MockInterface mock;
    Interface i;
    sdbusplus::SdBusMock interface;

    auto b = sdbusplus::get_mocked_new(&interface);
    auto r =
        MakeInterface<DummyInterfaceWithProperties>::op(b, "foo", i, false);

    EXPECT_CALL(mock, serializeThreeArgs("/foo"s, "bar"s, _)).Times(1);

    SerializeInterface<DummyInterfaceWithProperties, SerialForwarder>::op(
        "/foo"s, "bar"s, r);
}

TEST(InterfaceOpsTest, TestSerializeInterfaceWithOneArgument)
{
    MockInterface mock;
    Interface i{{"foo"s, static_cast<int64_t>(1ll)}};
    sdbusplus::SdBusMock interface;

    auto b = sdbusplus::get_mocked_new(&interface);
    auto r =
        MakeInterface<DummyInterfaceWithProperties>::op(b, "foo", i, false);

    EXPECT_CALL(mock, serializeThreeArgs("/foo"s, "bar"s, _)).Times(1);

    SerializeInterface<DummyInterfaceWithProperties, SerialForwarder>::op(
        "/foo"s, "bar"s, r);
}

TEST(InterfaceOpsTest, TestDeserializePropertylessInterfaceWithoutArguments)
{
    MockInterface mock;
    Interface i;
    sdbusplus::SdBusMock interface;

    auto b = sdbusplus::get_mocked_new(&interface);
    auto r =
        MakeInterface<DummyInterfaceWithoutProperties>::op(b, "foo", i, false);

    EXPECT_CALL(mock, deserializeNoop()).Times(1);

    DeserializeInterface<DummyInterfaceWithoutProperties, SerialForwarder>::op(
        "/foo"s, "bar"s, r);
}

TEST(InterfaceOpsTest, TestDeserializePropertylessInterfaceWithOneArgument)
{
    MockInterface mock;
    Interface i{{"foo"s, static_cast<int64_t>(1ll)}};
    sdbusplus::SdBusMock interface;

    auto b = sdbusplus::get_mocked_new(&interface);
    auto r =
        MakeInterface<DummyInterfaceWithoutProperties>::op(b, "foo", i, false);

    EXPECT_CALL(mock, deserializeNoop()).Times(1);

    DeserializeInterface<DummyInterfaceWithoutProperties, SerialForwarder>::op(
        "/foo"s, "bar"s, r);
}

TEST(InterfaceOpsTest, TestDeserializeInterfaceWithNoArguments)
{
    MockInterface mock;
    Interface i;
    sdbusplus::SdBusMock interface;

    auto b = sdbusplus::get_mocked_new(&interface);
    auto r =
        MakeInterface<DummyInterfaceWithProperties>::op(b, "foo", i, false);

    EXPECT_CALL(mock, deserializeThreeArgs("/foo"s, "bar"s, _)).Times(1);

    DeserializeInterface<DummyInterfaceWithProperties, SerialForwarder>::op(
        "/foo"s, "bar"s, r);
}

TEST(InterfaceOpsTest, TestDeserializeInterfaceWithOneArgument)
{
    MockInterface mock;
    Interface i{{"foo"s, static_cast<int64_t>(1ll)}};
    sdbusplus::SdBusMock interface;

    auto b = sdbusplus::get_mocked_new(&interface);
    auto r =
        MakeInterface<DummyInterfaceWithProperties>::op(b, "foo", i, false);

    EXPECT_CALL(mock, deserializeThreeArgs("/foo"s, "bar"s, _)).Times(1);

    DeserializeInterface<DummyInterfaceWithProperties, SerialForwarder>::op(
        "/foo"s, "bar"s, r);
}
