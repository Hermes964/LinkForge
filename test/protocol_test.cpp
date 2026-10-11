#include "linkforge/protocol.hpp"
#include <cstring>
#include <random>
#include <iostream>
#include <cstdlib>
#include <cassert>
#include <functional>

using namespace linkforge;

std::vector<uint8_t> makeRandomPayload(size_t len)
{
    std::vector<uint8_t> v(len);
    std::mt19937 gen(42);
    std::uniform_int_distribution<> dis(0, 255);
    for (size_t i = 0; i < len; ++i)
        v[i] = dis(gen);
    return v;
}

std::vector<std::string> g_results;

void test(void (*func)(), const std::string& name)
{
    try {
        func();
        g_results.push_back("PASS: " + name);
        std::cout << "PASSED: " << name << std::endl;
    } catch (const std::exception& e) {
        g_results.push_back("FAIL: " + name + " - " + std::string(e.what()));
        std::cout << "FAILED: " << name << " - " << e.what() << std::endl;
    } catch (...) {
        g_results.push_back("FAIL: " + name + " - unknown exception");
        std::cout << "FAILED: " << name << " - unknown exception" << std::endl;
    }
}

void test_assert(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void test_assert_eq(int expected, int actual, const std::string& message) {
    if (expected != actual) throw std::runtime_error(message + " (expected " + std::to_string(expected) + ", got " + std::to_string(actual) + ")");
}

void test_assert_eq(const std::string& expected, const std::string& actual, const std::string& message) {
    if (expected != actual) throw std::runtime_error(message + " (expected '" + expected + "', got '" + actual + "')");
}

void test_assert_eq(const void* expected, const void* actual, size_t len, const std::string& message) {
    if (std::memcmp(expected, actual, len) != 0) throw std::runtime_error(message);
}

void test_assert_throws(std::function<void()> func, const std::string& name) {
    try {
        func();
        g_results.push_back("FAIL: " + name + " - expected exception");
        std::cout << "FAILED: " << name << " - expected exception" << std::endl;
    } catch (const ProtocolError&) {
        g_results.push_back("PASS: " + name);
        std::cout << "PASSED: " << name << std::endl;
    } catch (...) {
        g_results.push_back("FAIL: " + name + " - wrong exception type");
        std::cout << "FAILED: " << name << " - wrong exception type" << std::endl;
    }
}

void test1()
{
    test_assert(isValidProtocolVersion(1), "isValidProtocolVersion(1)");
    test_assert(!isValidProtocolVersion(0), "isValidProtocolVersion(0)");
    test_assert(!isValidProtocolVersion(2), "isValidProtocolVersion(2)");
}

void test2()
{
    test_assert(isValidMessageType(0), "isValidMessageType(0)");
    test_assert(isValidMessageType(1), "isValidMessageType(1)");
    test_assert(isValidMessageType(static_cast<uint8_t>(MessageType::HANDSHAKE_RESPONSE)), "isValidMessageType(HANDSHAKE_RESPONSE)");
    test_assert(!isValidMessageType(128), "isValidMessageType(128)");
    test_assert(!isValidMessageType(255), "isValidMessageType(255)");
}

void test3()
{
    assert(PROTOCOL_MAGIC == 0x4C666F72);
    assert(sizeof(PacketHeader) == 56);
    assert(sizeof(FrameHeader) == 12);
}

void test4()
{
    uint16_t val = 0x1234;
    test_assert_eq(ntohs(htons(val)), val, "htons/ntohs roundtrip");
    test_assert_eq(htons(0x0001), 0x0100, "htons(0x0001)");
    test_assert_eq(htons(0x0100), 0x0001, "htons(0x0100)");
    test_assert_eq(htons(0x1234), 0x3412, "htons(0x1234)");
}

void test5()
{
    uint32_t val = 0x12345678;
    test_assert_eq(ntohl(htonl(val)), val, "htonl/ntohl roundtrip");
    test_assert_eq(htonl(0x00000001), 0x01000000, "htonl(0x00000001)");
    test_assert_eq(htonl(0x01000000), 0x00000001, "htonl(0x01000000)");
    test_assert_eq(htonl(0x12345678), 0x78563412, "htonl(0x12345678)");
}

void test6()
{
    PacketHeader header = {};
    header.magic = PROTOCOL_MAGIC;
    header.protocol_version = PROTOCOL_VERSION;
    header.message_type = static_cast<uint8_t>(MessageType::VIDEO_FRAGMENT);
    header.flags = 0;
    header.payload_length = 100;
    header.sequence_number = 100;
    header.timestamp = 1000;
    header.fragment_index = 0;
    header.fragment_count = 1;
    for (size_t i = 0; i < MAX_SESSION_ID; ++i)
        header.session_id[i] = (i == 0) ? 'A' : 0;

    std::vector<uint8_t> payload = makeRandomPayload(100);
    std::vector<uint8_t> serialized;
    ProtocolSerializer serializer;

    size_t written = serializer.serialize(header, payload, serialized);
    test_assert_eq((int)written, (int)(PacketHeader::PacketHeader::HEADER_SIZE + 100), "serialize length");

    PacketHeader header2;
    std::vector<uint8_t> payload2;
    size_t read = serializer.deserialize(serialized, header2, payload2);

    test_assert_eq((int)read, (int)written, "deserialize length");
    test_assert_eq(header2.magic, header.magic, "magic");
    test_assert_eq(header2.protocol_version, header.protocol_version, "version");
    test_assert_eq(header2.message_type, header.message_type, "message_type");
    test_assert_eq(header2.flags, header.flags, "flags");
    test_assert_eq(header2.payload_length, header.payload_length, "payload_length");
    test_assert_eq(header2.sequence_number, header.sequence_number, "sequence_number");
    test_assert_eq(memcmp(header2.session_id, header.session_id, MAX_SESSION_ID), 0, "session_id");
    test_assert_eq(header2.timestamp, header.timestamp, "timestamp");
    test_assert_eq(header2.fragment_index, header.fragment_index, "fragment_index");
    test_assert_eq(header2.fragment_count, header.fragment_count, "fragment_count");
    test_assert_eq((int)payload2.size(), (int)payload.size(), "payload size");
    test_assert_eq(memcmp(payload2.data(), payload.data(), payload.size()), 0, "payload content");
}

void test7()
{
    PacketHeader header = {};
    header.magic = PROTOCOL_MAGIC;
    header.protocol_version = PROTOCOL_VERSION;
    header.message_type = static_cast<uint8_t>(MessageType::VIDEO_FRAGMENT);
    header.payload_length = 50 + sizeof(FrameHeader);
    header.sequence_number = 200;
    header.timestamp = 2000;
    header.fragment_index = 0;
    header.fragment_count = 1;
    header.session_id[0] = 'B';

    FrameHeader frame = {};
    frame.frame_id = 12345;
    frame.capture_timestamp = 3000;
    frame.fragment_index = 0;
    frame.fragment_count = 1;

    std::vector<uint8_t> payload = makeRandomPayload(50);
    std::vector<uint8_t> serialized;
    ProtocolSerializer serializer;

    size_t written = serializer.serialize(header, frame, payload, serialized);
    test_assert_eq((int)written, (int)(PacketHeader::PacketHeader::HEADER_SIZE + sizeof(FrameHeader) + 50), "serialize with frame length");

    PacketHeader header2;
    FrameHeader frame2;
    std::vector<uint8_t> payload2;
    size_t read = serializer.deserialize(serialized, header2, frame2, payload2);

    test_assert_eq((int)read, (int)written, "deserialize with frame length");
    test_assert_eq(header2.magic, header.magic, "magic");
    test_assert_eq(header2.message_type, header.message_type, "message_type");
    test_assert_eq(header2.payload_length, header.payload_length, "payload_length");
    test_assert_eq(frame2.frame_id, frame.frame_id, "frame_id");
    test_assert_eq(frame2.capture_timestamp, frame.capture_timestamp, "capture_timestamp");
    test_assert_eq(frame2.fragment_index, frame.fragment_index, "fragment_index");
    test_assert_eq(frame2.fragment_count, frame.fragment_count, "fragment_count");
    test_assert_eq((int)payload2.size(), (int)payload.size(), "payload size");
}

void test8()
{
    ProtocolSerializer serializer;
    std::vector<uint8_t> data(20);
    PacketHeader header;
    std::vector<uint8_t> payload;

    test_assert_throws(
        [&serializer, &data, &header, &payload]() { serializer.deserialize(data, header, payload); },
        "Truncated packet should throw ProtocolError"
    );
}

void test9()
{
    ProtocolSerializer serializer;
    PacketHeader header = {};
    header.magic = 0x00000000;
    header.protocol_version = PROTOCOL_VERSION;
    header.message_type = static_cast<uint8_t>(MessageType::VIDEO_FRAGMENT);
    header.session_id[0] = 'A';
    header.payload_length = 10;
    header.sequence_number = 1;
    header.timestamp = 1;
    header.fragment_index = 0;
    header.fragment_count = 1;

    std::vector<uint8_t> data(PacketHeader::HEADER_SIZE + header.payload_length);
    std::memcpy(data.data(), &header, PacketHeader::HEADER_SIZE);
    std::memset(data.data() + PacketHeader::HEADER_SIZE, 0, header.payload_length);

    PacketHeader header2;
    std::vector<uint8_t> payload;

    test_assert_throws(
        [&serializer, &data, &header2, &payload]() { serializer.deserialize(data, header2, payload); },
        "Invalid magic should throw ProtocolError"
    );
}

void test10()
{
    ProtocolSerializer serializer;
    PacketHeader header = {};
    header.magic = PROTOCOL_MAGIC;
    header.protocol_version = 2;
    header.message_type = static_cast<uint8_t>(MessageType::VIDEO_FRAGMENT);
    header.session_id[0] = 'A';
    header.payload_length = 10;
    header.sequence_number = 1;
    header.timestamp = 1;
    header.fragment_index = 0;
    header.fragment_count = 1;

    std::vector<uint8_t> data(PacketHeader::HEADER_SIZE + header.payload_length);
    std::memcpy(data.data(), &header, PacketHeader::HEADER_SIZE);
    std::memset(data.data() + PacketHeader::HEADER_SIZE, 0, header.payload_length);

    PacketHeader header2;
    std::vector<uint8_t> payload;

    test_assert_throws(
        [&serializer, &data, &header2, &payload]() { serializer.deserialize(data, header2, payload); },
        "Invalid version should throw ProtocolError"
    );
}

void test11()
{
    ProtocolSerializer serializer;
    PacketHeader header = {};
    header.magic = PROTOCOL_MAGIC;
    header.protocol_version = PROTOCOL_VERSION;
    header.message_type = 128;
    header.session_id[0] = 'A';
    header.payload_length = 10;
    header.sequence_number = 1;
    header.timestamp = 1;
    header.fragment_index = 0;
    header.fragment_count = 1;

    std::vector<uint8_t> data(PacketHeader::HEADER_SIZE + header.payload_length);
    std::memcpy(data.data(), &header, PacketHeader::HEADER_SIZE);
    std::memset(data.data() + PacketHeader::HEADER_SIZE, 0, header.payload_length);

    PacketHeader header2;
    std::vector<uint8_t> payload;

    test_assert_throws(
        [&serializer, &data, &header2, &payload]() { serializer.deserialize(data, header2, payload); },
        "Invalid message type should throw ProtocolError"
    );
}

void test12()
{
    ProtocolSerializer serializer;
    PacketHeader header = {};
    header.magic = PROTOCOL_MAGIC;
    header.protocol_version = PROTOCOL_VERSION;
    header.message_type = static_cast<uint8_t>(MessageType::VIDEO_FRAGMENT);
    header.session_id[0] = 'A';
    header.payload_length = MAX_PACKET_SIZE + 1;
    header.sequence_number = 1;
    header.timestamp = 1;
    header.fragment_index = 0;
    header.fragment_count = 1;

    std::vector<uint8_t> data(PacketHeader::HEADER_SIZE + header.payload_length);
    std::memcpy(data.data(), &header, PacketHeader::HEADER_SIZE);
    std::memset(data.data() + PacketHeader::HEADER_SIZE, 0, header.payload_length);

    PacketHeader header2;
    std::vector<uint8_t> payload;

    test_assert_throws(
        [&serializer, &data, &header2, &payload]() { serializer.deserialize(data, header2, payload); },
        "Payload too large should throw ProtocolError"
    );
}

void test13()
{
    ProtocolSerializer serializer;
    PacketHeader header = {};
    header.magic = PROTOCOL_MAGIC;
    header.protocol_version = PROTOCOL_VERSION;
    header.message_type = static_cast<uint8_t>(MessageType::VIDEO_FRAGMENT);
    header.session_id[0] = 'A';
    header.payload_length = 10;
    header.sequence_number = 1;
    header.timestamp = 1;
    header.fragment_index = 0;
    header.fragment_count = MAX_FRAGMENT_COUNT + 1;

    std::vector<uint8_t> data(PacketHeader::HEADER_SIZE + header.payload_length);
    std::memcpy(data.data(), &header, PacketHeader::HEADER_SIZE);
    std::memset(data.data() + PacketHeader::HEADER_SIZE, 0, header.payload_length);

    PacketHeader header2;
    std::vector<uint8_t> payload;

    test_assert_throws(
        [&serializer, &data, &header2, &payload]() { serializer.deserialize(data, header2, payload); },
        "Invalid fragment count should throw ProtocolError"
    );
}

void test14()
{
    ProtocolSerializer serializer;
    PacketHeader header = {};
    header.magic = PROTOCOL_MAGIC;
    header.protocol_version = PROTOCOL_VERSION;
    header.message_type = static_cast<uint8_t>(MessageType::VIDEO_FRAGMENT);
    header.session_id[0] = 'A';
    header.payload_length = 10;
    header.sequence_number = 1;
    header.timestamp = 1;
    header.fragment_index = 1;
    header.fragment_count = 0;

    std::vector<uint8_t> data(PacketHeader::HEADER_SIZE + header.payload_length);
    std::memcpy(data.data(), &header, PacketHeader::HEADER_SIZE);
    std::memset(data.data() + PacketHeader::HEADER_SIZE, 0, header.payload_length);

    PacketHeader header2;
    std::vector<uint8_t> payload;

    test_assert_throws(
        [&serializer, &data, &header2, &payload]() { serializer.deserialize(data, header2, payload); },
        "Invalid fragment index should throw ProtocolError"
    );
}

void test15()
{
    ProtocolSerializer serializer;
    test_assert_throws(
        [&serializer]() {
            SessionId id("this_is_a_very_long_session_id_that_exceeds_the_limit");
        },
        "SessionId with too-long string should throw ProtocolError"
    );
}

void test16()
{
    ProtocolSerializer serializer;
    SessionId id("session-42");
    test_assert_eq(id.toString(), "session-42", "SessionId toString");
    test_assert_eq(static_cast<int>(id.length()), 10, "SessionId length");
    test_assert(!id.empty(), "SessionId should not be empty");
}

void test17()
{
    ProtocolSerializer serializer;
    SessionId id;
    id.set("session-42");
    test_assert_eq(id.toString(), "session-42", "SessionId set");
    test_assert_eq(static_cast<int>(id.length()), 10, "SessionId set length");
}

void test18()
{
    ProtocolSerializer serializer;
    PacketHeader header = {};
    header.payload_length = MAX_PACKET_SIZE - PacketHeader::HEADER_SIZE;
    test_assert(serializer.withinLimits(header), "withinLimits should pass for max size");

    header.payload_length = MAX_PACKET_SIZE - PacketHeader::HEADER_SIZE + 1;
    test_assert(!serializer.withinLimits(header), "withinLimits should fail for oversized payload");
}

void test19()
{
    ProtocolSerializer serializer;
    const size_t testPayloadSize = 50;

    PacketHeader header = {};
    header.magic = PROTOCOL_MAGIC;
    header.protocol_version = PROTOCOL_VERSION;
    header.message_type = static_cast<uint8_t>(MessageType::VIDEO_FRAGMENT);
    header.session_id[0] = 'A';
    header.payload_length = testPayloadSize;
    header.sequence_number = 42;
    header.timestamp = 4200;
    header.fragment_index = 0;
    header.fragment_count = 1;

    std::vector<uint8_t> payload = makeRandomPayload(testPayloadSize);
    std::vector<uint8_t> serialized;

    size_t written = serializer.serialize(header, payload, serialized);
    test_assert_eq((int)written, (int)(PacketHeader::PacketHeader::HEADER_SIZE + testPayloadSize), "serialize length");

    PacketHeader header2;
    std::vector<uint8_t> payload2;
    size_t read = serializer.deserialize(serialized, header2, payload2);

    test_assert_eq((int)read, (int)written, "deserialize length");
    test_assert_eq(header2.payload_length, header.payload_length, "payload_length");
    test_assert_eq((int)payload2.size(), (int)payload.size(), "payload size");
    test_assert_eq(memcmp(payload2.data(), payload.data(), payload.size()), 0, "payload content");
}

void test20()
{
    ProtocolSerializer serializer;
    PacketHeader header = {};
    header.magic = PROTOCOL_MAGIC;
    header.protocol_version = PROTOCOL_VERSION;
    header.message_type = static_cast<uint8_t>(MessageType::VIDEO_FRAGMENT);
    header.session_id[0] = 'A';
    header.payload_length = 10;
    header.sequence_number = 1;
    header.timestamp = 1;
    header.fragment_index = 0;
    header.fragment_count = MAX_FRAGMENT_COUNT;

    std::vector<uint8_t> data(PacketHeader::HEADER_SIZE + header.payload_length);
    std::memcpy(data.data(), &header, PacketHeader::HEADER_SIZE);
    std::memset(data.data() + PacketHeader::HEADER_SIZE, 0, header.payload_length);

    PacketHeader header2;
    std::vector<uint8_t> payload;

    serializer.deserialize(data, header2, payload);
}

void runAllTests()
{
    test(test1, "IsValidProtocolVersion");
    test(test2, "IsValidMessageType");
    test(test3, "MagicNumberAndSizes");
    test(test4, "ByteOrder");
    test(test5, "ByteOrderBig");
    test(test6, "RoundTripPacket");
    test(test7, "RoundTripPacketWithFrame");
    test(test8, "TruncatedPacket");
    test(test9, "InvalidMagic");
    test(test10, "InvalidVersion");
    test(test11, "InvalidMessageType");
    test(test12, "PayloadTooLarge");
    test(test13, "InvalidFragmentCount");
    test(test14, "InvalidFragmentIndex");
    test(test15, "SessionIdTooLong");
    test(test16, "SessionIdValidation");
    test(test17, "SessionIdSetter");
    test(test18, "WithinLimits");
    test(test19, "PayloadLengthRoundTrip");
    test(test20, "MaxFragmentCount");
}

int main(int argc, char** argv)
{
    runAllTests();

    std::cout << std::endl << "Results Summary" << std::endl;
    std::cout << "===============" << std::endl;
    for (const auto& r : g_results) {
        std::cout << r << std::endl;
    }
    std::cout << "Tests run: " << g_results.size() << std::endl;

    bool allPassed = true;
    for (const auto& r : g_results) {
        if (r.rfind("PASS:", 0) != 0) {
            allPassed = false;
        }
    }

    if (!allPassed) {
        std::cout << "FAILED: Some tests did not pass" << std::endl;
        return 1;
    }
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
