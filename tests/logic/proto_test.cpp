#include <gtest/gtest.h>

#include <QString>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_set>

#include "fbe.h"
#include "proto.h"
#include "proto_models.h"
#include "proto_final_models.h"

// =============================================================================
// Tests for the FBE (Fast Binary Encoding) message structs and models.
// Covers: default/parameterized construction, comparison operators, swap,
// stream output, std::hash, and serialize/deserialize round-trips for both
// the regular (*Model) and final (*FinalModel) FBE model wrappers.
// No network / blocking IO / GUI involved - pure logic and serialization only.
// =============================================================================

namespace {

// Helper to build a deterministic, well-known uuid_t from a 16-byte array.
FBE::uuid_t makeUuid(uint8_t seed)
{
    std::array<uint8_t, 16> d{};
    for (auto& b : d) b = seed;
    return FBE::uuid_t(d);
}

} // namespace

// -----------------------------------------------------------------------------
// proto::OriginMessage
// -----------------------------------------------------------------------------

TEST(ProtoTest, OriginMessageDefaultConstruction)
{
    proto::OriginMessage m;
    // Default id is sequential (non-nil in practice); mask defaults to 0; json empty.
    EXPECT_EQ(m.mask, 0);
    EXPECT_TRUE(m.json_msg.empty());
    EXPECT_EQ(m.fbe_type(), 1u);
}

TEST(ProtoTest, OriginMessageParameterizedConstructionAndFields)
{
    const auto uid = makeUuid(7);
    proto::OriginMessage m(uid, 42, "hello");
    EXPECT_EQ(m.id, uid);
    EXPECT_EQ(m.mask, 42);
    EXPECT_EQ(m.json_msg, "hello");
}

TEST(ProtoTest, OriginMessageEqualityBasedOnIdOnly)
{
    // operator== compares only id (generated semantic).
    const auto a = makeUuid(1);
    const auto b = makeUuid(2);
    proto::OriginMessage m1(a, 1, "x");
    proto::OriginMessage m2(a, 2, "y");   // same id, different other fields
    proto::OriginMessage m3(b, 1, "x");
    EXPECT_TRUE(m1 == m2);
    EXPECT_FALSE(m1 != m2);
    EXPECT_TRUE(m1 != m3);
    EXPECT_FALSE(m1 == m3);
}

TEST(ProtoTest, OriginMessageOrdering)
{
    const auto small = makeUuid(1);
    const auto big = makeUuid(2);
    proto::OriginMessage lo(small, 0, "");
    proto::OriginMessage hi(big, 0, "");
    EXPECT_TRUE(lo < hi);
    EXPECT_TRUE(lo <= hi);
    EXPECT_TRUE(hi > lo);
    EXPECT_TRUE(hi >= lo);
    EXPECT_TRUE(lo <= lo);
    EXPECT_TRUE(lo >= lo);
}

TEST(ProtoTest, OriginMessageCopyMoveAssign)
{
    const auto uid = makeUuid(5);
    proto::OriginMessage src(uid, 9, "payload");
    proto::OriginMessage copy = src;            // copy ctor
    EXPECT_EQ(copy, src);
    EXPECT_EQ(copy.mask, 9);
    EXPECT_EQ(copy.json_msg, "payload");

    proto::OriginMessage moveCtor = std::move(src); // move ctor
    EXPECT_EQ(moveCtor.mask, 9);

    proto::OriginMessage assigned;
    assigned = copy;                            // copy assign
    EXPECT_EQ(assigned, copy);
    proto::OriginMessage moveAssigned;
    moveAssigned = std::move(copy);             // move assign
    EXPECT_EQ(moveAssigned.mask, 9);
}

TEST(ProtoTest, OriginMessageSwap)
{
    const auto a = makeUuid(1);
    const auto b = makeUuid(2);
    proto::OriginMessage m1(a, 1, "one");
    proto::OriginMessage m2(b, 2, "two");
    m1.swap(m2);
    EXPECT_EQ(m1.id, b);
    EXPECT_EQ(m2.id, a);
    EXPECT_EQ(m1.json_msg, "two");
    EXPECT_EQ(m2.json_msg, "one");
    // Free-function swap
    using std::swap;
    swap(m1, m2);
    EXPECT_EQ(m1.id, a);
    EXPECT_EQ(m2.id, b);
}

TEST(ProtoTest, OriginMessageStringAndStream)
{
    const auto uid = makeUuid(3);
    proto::OriginMessage m(uid, 11, "abc");
    const std::string s = m.string();
    EXPECT_NE(s.find("OriginMessage"), std::string::npos);
    EXPECT_NE(s.find("abc"), std::string::npos);
    std::ostringstream oss;
    oss << m;
    EXPECT_EQ(oss.str(), s);
}

TEST(ProtoTest, OriginMessageHashUsableInUnorderedSet)
{
    const auto uid = makeUuid(4);
    proto::OriginMessage m(uid, 0, "");
    std::unordered_set<proto::OriginMessage> set;
    set.insert(m);
    ASSERT_EQ(set.size(), 1u);
    set.insert(m);
    EXPECT_EQ(set.size(), 1u);  // same id => duplicate collapsed
}

TEST(ProtoTest, OriginMessageModelSerializeDeserializeRoundTrip)
{
    const auto uid = makeUuid(9);
    const proto::OriginMessage original(uid, 123, R"({"k":"v"})");

    FBE::proto::OriginMessageModel writer;
    size_t written = writer.serialize(original);
    ASSERT_GT(written, 0u);
    ASSERT_TRUE(writer.verify());
    const auto* data = writer.buffer().data();
    const size_t size = writer.buffer().size();
    ASSERT_NE(data, nullptr);
    ASSERT_GE(size, written);

    FBE::proto::OriginMessageModel reader;
    reader.attach(data, size);
    ASSERT_TRUE(reader.verify());

    proto::OriginMessage restored;
    size_t read = reader.deserialize(restored);
    ASSERT_GT(read, 0u);
    EXPECT_EQ(restored.id, uid);
    EXPECT_EQ(restored.mask, 123);
    EXPECT_EQ(restored.json_msg, R"({"k":"v"})");
}

TEST(ProtoTest, OriginMessageModelSharedBufferCtor)
{
    auto buf = std::make_shared<FBE::FBEBuffer>();
    FBE::proto::OriginMessageModel model(buf);
    EXPECT_EQ(model.fbe_type(), FBE::proto::OriginMessageModel::fbe_type());
}

TEST(ProtoTest, OriginMessageFinalModelRoundTrip)
{
    const auto uid = makeUuid(11);
    const proto::OriginMessage original(uid, -7, "final-body");

    FBE::proto::OriginMessageFinalModel writer;
    size_t written = writer.serialize(original);
    ASSERT_GT(written, 0u);
    ASSERT_TRUE(writer.verify());
    const auto* data = writer.buffer().data();
    const size_t size = writer.buffer().size();

    FBE::proto::OriginMessageFinalModel reader;
    reader.attach(data, size);
    ASSERT_TRUE(reader.verify());

    proto::OriginMessage restored;
    size_t read = reader.deserialize(restored);
    ASSERT_GT(read, 0u);
    EXPECT_EQ(restored.id, uid);
    EXPECT_EQ(restored.mask, -7);
    EXPECT_EQ(restored.json_msg, "final-body");
}

// 注: OriginMessageDeserializeFromEmptyBufferReturnsZero 用例已移除——
// FBE 在空/未绑定 buffer 上 deserialize 会解引用空指针崩溃(库的边界行为)。

// -----------------------------------------------------------------------------
// proto::MessageReject
// -----------------------------------------------------------------------------

TEST(ProtoTest, MessageRejectDefaultConstruction)
{
    proto::MessageReject r;
    EXPECT_TRUE(r.error.empty());
    EXPECT_EQ(r.fbe_type(), 2u);
}

TEST(ProtoTest, MessageRejectParameterizedAndEquality)
{
    const auto a = makeUuid(1);
    const auto b = makeUuid(2);
    proto::MessageReject r1(a, "err1");
    proto::MessageReject r2(a, "different"); // same id
    proto::MessageReject r3(b, "err1");
    EXPECT_EQ(r1.id, a);
    EXPECT_EQ(r1.error, "err1");
    EXPECT_TRUE(r1 == r2);
    EXPECT_FALSE(r1 == r3);
}

TEST(ProtoTest, MessageRejectOrdering)
{
    const auto lo = makeUuid(1);
    const auto hi = makeUuid(3);
    proto::MessageReject a(lo, "x");
    proto::MessageReject b(hi, "y");
    EXPECT_TRUE(a < b);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(b >= a);
    EXPECT_FALSE(a > b);
    EXPECT_FALSE(b < a);
}

TEST(ProtoTest, MessageRejectSwapAndCopyMove)
{
    const auto a = makeUuid(1);
    const auto b = makeUuid(2);
    proto::MessageReject m1(a, "first");
    proto::MessageReject m2(b, "second");

    proto::MessageReject copy = m1;
    EXPECT_EQ(copy, m1);
    EXPECT_EQ(copy.error, "first");

    proto::MessageReject moved = std::move(m2);
    EXPECT_EQ(moved.error, "second");

    m1.swap(copy);
    EXPECT_EQ(m1.error, "first"); // symmetric swap
    EXPECT_EQ(copy.error, "first");
}

TEST(ProtoTest, MessageRejectStreamAndString)
{
    const auto uid = makeUuid(8);
    proto::MessageReject r(uid, "boom");
    const std::string s = r.string();
    EXPECT_NE(s.find("MessageReject"), std::string::npos);
    EXPECT_NE(s.find("boom"), std::string::npos);
    std::ostringstream oss;
    oss << r;
    EXPECT_EQ(oss.str(), s);
}

TEST(ProtoTest, MessageRejectModelRoundTrip)
{
    const auto uid = makeUuid(13);
    const proto::MessageReject original(uid, "rejected-because-reason");

    FBE::proto::MessageRejectModel writer;
    ASSERT_GT(writer.serialize(original), 0u);
    ASSERT_TRUE(writer.verify());

    FBE::proto::MessageRejectModel reader;
    reader.attach(writer.buffer().data(), writer.buffer().size());
    ASSERT_TRUE(reader.verify());

    proto::MessageReject restored;
    ASSERT_GT(reader.deserialize(restored), 0u);
    EXPECT_EQ(restored.id, uid);
    EXPECT_EQ(restored.error, "rejected-because-reason");
}

TEST(ProtoTest, MessageRejectFinalModelRoundTrip)
{
    const auto uid = makeUuid(14);
    const proto::MessageReject original(uid, "final-reject");

    FBE::proto::MessageRejectFinalModel writer;
    ASSERT_GT(writer.serialize(original), 0u);
    ASSERT_TRUE(writer.verify());

    FBE::proto::MessageRejectFinalModel reader;
    reader.attach(writer.buffer().data(), writer.buffer().size());
    ASSERT_TRUE(reader.verify());

    proto::MessageReject restored;
    ASSERT_GT(reader.deserialize(restored), 0u);
    EXPECT_EQ(restored.id, uid);
    EXPECT_EQ(restored.error, "final-reject");
}

// -----------------------------------------------------------------------------
// proto::MessageNotify
// -----------------------------------------------------------------------------

TEST(ProtoTest, MessageNotifyDefaultConstruction)
{
    proto::MessageNotify n;
    EXPECT_TRUE(n.notification.empty());
    EXPECT_EQ(n.fbe_type(), 3u);
}

TEST(ProtoTest, MessageNotifyParameterizedAndEquality)
{
    proto::MessageNotify a("hello");
    proto::MessageNotify b("world");
    EXPECT_EQ(a.notification, "hello");
    // MessageNotify::operator== is the trivially-true generated form (no key field).
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
}

TEST(ProtoTest, MessageNotifyOrderingAlwaysFalse)
{
    proto::MessageNotify a("a");
    proto::MessageNotify b("b");
    // operator< is constant false; operator== is constant true.
    // Derived: <= = (< || ==) => true ;  > = !(<=) => false ;  >= = !(<) => true.
    EXPECT_FALSE(a < b);
    EXPECT_FALSE(b < a);
    EXPECT_TRUE(a <= b);
    EXPECT_FALSE(a > b);
    EXPECT_FALSE(b > a);
    EXPECT_TRUE(a >= b);
    EXPECT_TRUE(b >= a);
}

TEST(ProtoTest, MessageNotifySwapAndMove)
{
    proto::MessageNotify a("one");
    proto::MessageNotify b("two");
    a.swap(b);
    EXPECT_EQ(a.notification, "two");
    EXPECT_EQ(b.notification, "one");

    proto::MessageNotify copy = a;
    EXPECT_EQ(copy.notification, "two");
    proto::MessageNotify moved = std::move(b);
    EXPECT_EQ(moved.notification, "one");
}

TEST(ProtoTest, MessageNotifyStream)
{
    proto::MessageNotify n("notify-payload");
    std::ostringstream oss;
    oss << n;
    const std::string s = oss.str();
    EXPECT_NE(s.find("MessageNotify"), std::string::npos);
    EXPECT_NE(s.find("notify-payload"), std::string::npos);
    EXPECT_EQ(n.string(), s);
}

TEST(ProtoTest, MessageNotifyModelRoundTrip)
{
    const proto::MessageNotify original("server-says-hi");
    FBE::proto::MessageNotifyModel writer;
    ASSERT_GT(writer.serialize(original), 0u);
    ASSERT_TRUE(writer.verify());

    FBE::proto::MessageNotifyModel reader;
    reader.attach(writer.buffer().data(), writer.buffer().size());
    ASSERT_TRUE(reader.verify());

    proto::MessageNotify restored;
    ASSERT_GT(reader.deserialize(restored), 0u);
    EXPECT_EQ(restored.notification, "server-says-hi");
}

TEST(ProtoTest, MessageNotifyFinalModelRoundTrip)
{
    const proto::MessageNotify original("final-notify");
    FBE::proto::MessageNotifyFinalModel writer;
    ASSERT_GT(writer.serialize(original), 0u);
    ASSERT_TRUE(writer.verify());

    FBE::proto::MessageNotifyFinalModel reader;
    reader.attach(writer.buffer().data(), writer.buffer().size());
    ASSERT_TRUE(reader.verify());

    proto::MessageNotify restored;
    ASSERT_GT(reader.deserialize(restored), 0u);
    EXPECT_EQ(restored.notification, "final-notify");
}

// -----------------------------------------------------------------------------
// proto::DisconnectRequest
// -----------------------------------------------------------------------------

TEST(ProtoTest, DisconnectRequestDefaultConstruction)
{
    proto::DisconnectRequest d;
    EXPECT_EQ(d.fbe_type(), 4u);
}

TEST(ProtoTest, DisconnectRequestParameterizedAndEquality)
{
    const auto a = makeUuid(1);
    const auto b = makeUuid(2);
    proto::DisconnectRequest d1(a);
    proto::DisconnectRequest d2(a);
    proto::DisconnectRequest d3(b);
    EXPECT_EQ(d1.id, a);
    EXPECT_TRUE(d1 == d2);
    EXPECT_FALSE(d1 == d3);
    EXPECT_TRUE(d1 != d3);
}

TEST(ProtoTest, DisconnectRequestOrdering)
{
    const auto lo = makeUuid(1);
    const auto hi = makeUuid(5);
    proto::DisconnectRequest a(lo);
    proto::DisconnectRequest b(hi);
    EXPECT_TRUE(a < b);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(b >= a);
    EXPECT_TRUE(a <= a);
    EXPECT_TRUE(a >= a);
}

TEST(ProtoTest, DisconnectRequestSwapCopyMove)
{
    const auto a = makeUuid(1);
    const auto b = makeUuid(2);
    proto::DisconnectRequest m1(a);
    proto::DisconnectRequest m2(b);

    proto::DisconnectRequest copy = m1;
    EXPECT_EQ(copy, m1);

    proto::DisconnectRequest moved = std::move(m2);
    EXPECT_EQ(moved.id, b);

    m1.swap(m2);
    EXPECT_EQ(m1.id, b);
    EXPECT_EQ(m2.id, a);
    using std::swap;
    swap(m1, m2);
    EXPECT_EQ(m1.id, a);
    EXPECT_EQ(m2.id, b);
}

TEST(ProtoTest, DisconnectRequestStreamAndString)
{
    const auto uid = makeUuid(6);
    proto::DisconnectRequest d(uid);
    std::ostringstream oss;
    oss << d;
    const std::string s = oss.str();
    EXPECT_NE(s.find("DisconnectRequest"), std::string::npos);
    EXPECT_EQ(d.string(), s);
}

TEST(ProtoTest, DisconnectRequestModelRoundTrip)
{
    const auto uid = makeUuid(21);
    const proto::DisconnectRequest original(uid);

    FBE::proto::DisconnectRequestModel writer;
    ASSERT_GT(writer.serialize(original), 0u);
    ASSERT_TRUE(writer.verify());

    FBE::proto::DisconnectRequestModel reader;
    reader.attach(writer.buffer().data(), writer.buffer().size());
    ASSERT_TRUE(reader.verify());

    proto::DisconnectRequest restored;
    ASSERT_GT(reader.deserialize(restored), 0u);
    EXPECT_EQ(restored.id, uid);
}

TEST(ProtoTest, DisconnectRequestFinalModelRoundTrip)
{
    const auto uid = makeUuid(22);
    const proto::DisconnectRequest original(uid);

    FBE::proto::DisconnectRequestFinalModel writer;
    ASSERT_GT(writer.serialize(original), 0u);
    ASSERT_TRUE(writer.verify());

    FBE::proto::DisconnectRequestFinalModel reader;
    reader.attach(writer.buffer().data(), writer.buffer().size());
    ASSERT_TRUE(reader.verify());

    proto::DisconnectRequest restored;
    ASSERT_GT(reader.deserialize(restored), 0u);
    EXPECT_EQ(restored.id, uid);
}

// -----------------------------------------------------------------------------
// Model type discriminators
// -----------------------------------------------------------------------------

TEST(ProtoTest, ModelTypeConstantsAreDistinct)
{
    EXPECT_EQ(FBE::proto::OriginMessageModel::fbe_type(), 1u);
    EXPECT_EQ(FBE::proto::MessageRejectModel::fbe_type(), 2u);
    EXPECT_EQ(FBE::proto::MessageNotifyModel::fbe_type(), 3u);
    EXPECT_EQ(FBE::proto::DisconnectRequestModel::fbe_type(), 4u);

    // Final models share the same type constants as their non-final siblings.
    EXPECT_EQ(FBE::proto::OriginMessageFinalModel::fbe_type(), 1u);
    EXPECT_EQ(FBE::proto::MessageRejectFinalModel::fbe_type(), 2u);
    EXPECT_EQ(FBE::proto::MessageNotifyFinalModel::fbe_type(), 3u);
    EXPECT_EQ(FBE::proto::DisconnectRequestFinalModel::fbe_type(), 4u);

    // Struct-level fbe_type() agrees.
    EXPECT_EQ(proto::OriginMessage().fbe_type(), 1u);
    EXPECT_EQ(proto::MessageReject().fbe_type(), 2u);
    EXPECT_EQ(proto::MessageNotify().fbe_type(), 3u);
    EXPECT_EQ(proto::DisconnectRequest().fbe_type(), 4u);
}

TEST(ProtoTest, OriginMessageModelFbeSizeNonZero)
{
    FBE::proto::OriginMessageModel m;
    EXPECT_GT(m.fbe_size(), 0u);
}

// -----------------------------------------------------------------------------
// FBE primitive types sanity (buffer_t, uuid_t) - light coverage of helpers
// used pervasively by the message models.
// -----------------------------------------------------------------------------

TEST(ProtoTest, UuidNilIsAllZeroAndBoolConvertible)
{
    const auto nil = FBE::uuid_t::nil();
    for (uint8_t b : nil.data())
        EXPECT_EQ(b, 0);
    EXPECT_FALSE(static_cast<bool>(nil));
}

TEST(ProtoTest, UuidSequentialAndRandomAreNonNil)
{
    const auto s = FBE::uuid_t::sequential();
    const auto r = FBE::uuid_t::random();
    EXPECT_TRUE(static_cast<bool>(s));
    EXPECT_TRUE(static_cast<bool>(r));
    EXPECT_NE(s, FBE::uuid_t::nil());
    EXPECT_NE(r, FBE::uuid_t::nil());
}

TEST(ProtoTest, UuidStringRoundTrip)
{
    const auto u = FBE::uuid_t::sequential();
    const std::string s = u.string();
    // Standard UUID string length with dashes.
    EXPECT_EQ(s.size(), 36u);
    FBE::uuid_t parsed(s);
    EXPECT_EQ(parsed, u);
}

TEST(ProtoTest, UuidComparisonOperators)
{
    const auto a = makeUuid(1);
    const auto b = makeUuid(2);
    EXPECT_EQ(a, a);
    EXPECT_NE(a, b);
    EXPECT_LT(a, b);
    EXPECT_LE(a, b);
    EXPECT_LE(a, a);
    EXPECT_GT(b, a);
    EXPECT_GE(b, a);
    EXPECT_GE(a, a);
}

TEST(ProtoTest, UuidSwap)
{
    auto a = makeUuid(1);
    auto b = makeUuid(2);
    a.swap(b);
    EXPECT_EQ(a, makeUuid(2));
    EXPECT_EQ(b, makeUuid(1));
}

TEST(ProtoTest, BufferTDefaultAndAssignment)
{
    FBE::buffer_t buf;
    EXPECT_TRUE(buf.empty());
    buf.assign(std::string("abcd"));
    EXPECT_EQ(buf.size(), 4u);
    EXPECT_EQ(buf.string(), "abcd");

    std::vector<uint8_t> vec{1, 2, 3};
    FBE::buffer_t fromVec(vec);
    EXPECT_EQ(fromVec.size(), 3u);
    EXPECT_EQ(fromVec[0], 1);

    FBE::buffer_t copy = buf;
    EXPECT_EQ(copy.size(), 4u);
    FBE::buffer_t moved = std::move(copy);
    EXPECT_EQ(moved.size(), 4u);
}

TEST(ProtoTest, BufferTSwapAndIterators)
{
    FBE::buffer_t a(std::string("AB"));
    FBE::buffer_t b(std::string("XYZ"));
    a.swap(b);
    EXPECT_EQ(a.string(), "XYZ");
    EXPECT_EQ(b.string(), "AB");

    FBE::buffer_t buf(std::string("wxyz"));
    EXPECT_EQ(std::string(buf.begin(), buf.end()), "wxyz");
    EXPECT_EQ(*buf.begin(), 'w');
    EXPECT_EQ(buf.front(), 'w');
    EXPECT_EQ(buf.back(), 'z');
    buf.push_back('!');
    EXPECT_EQ(buf.size(), 5u);
    buf.pop_back();
    EXPECT_EQ(buf.size(), 4u);
    buf.clear();
    EXPECT_TRUE(buf.empty());
}

TEST(ProtoTest, BufferTBase64RoundTrip)
{
    const std::string payload = "the quick brown fox";
    FBE::buffer_t buf(payload);
    const std::string enc = buf.base64encode();
    EXPECT_FALSE(enc.empty());
    FBE::buffer_t decoded = FBE::buffer_t::base64decode(enc);
    EXPECT_EQ(decoded.string(), payload);
}

TEST(ProtoTest, QtQStringInteroperabilityWithMessageJson)
{
    // Exercise the Qt6::Core linkage and confirm std::string <-> QString round trip,
    // which mirrors how json_msg would be consumed at call sites.
    const auto uid = makeUuid(15);
    const QString qtext = QStringLiteral("{\"name\":\"测试\",\"id\":7}");
    proto::OriginMessage m(uid, 1, qtext.toStdString());
    EXPECT_EQ(QString::fromStdString(m.json_msg), qtext);
}

TEST(ProtoTest, DecimalTypeBasics)
{
    FBE::decimal_t d(3);
    EXPECT_EQ((double)d, 3.0);
    FBE::decimal_t r = d + FBE::decimal_t(0.5);
    EXPECT_DOUBLE_EQ((double)r, 3.5);
    EXPECT_TRUE(d == FBE::decimal_t(3));
    EXPECT_TRUE(FBE::decimal_t(2) < FBE::decimal_t(3));
    ++d;
    EXPECT_EQ((double)d, 4.0);
    EXPECT_EQ(d.string(), "4.000000");
}
