#pragma once

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <array>
#include <vector>
#include <algorithm>

// LinkForge protocol namespace
namespace linkforge
{
    // Protocol version
    inline constexpr uint8_t PROTOCOL_VERSION = 1;

    // Protocol magic number for identification
    inline constexpr uint32_t PROTOCOL_MAGIC = 0x4C666F72; // 'LfOr'

    // Message types (0-127 valid, 128-255 reserved)
    enum class MessageType : uint8_t
    {
        INVALID = 0,
        HANDSHAKE_REQUEST = 1,
        HANDSHAKE_RESPONSE = 2,
        VIDEO_FRAGMENT = 3,
        STATUS_UPDATE = 4,
        CONTROL_COMMAND = 5,
        ERROR = 6,
        HEARTBEAT = 7,
        MAX_MESSAGE_TYPE = 7
    };

    // Session state
    enum class SessionState : uint8_t
    {
        DISCONNECTED,
        PAIRING,
        CONNECTING,
        ACTIVE,
        ERROR_STATE,
        MAX_SESSION_STATE = 5
    };

    // Packet limits
    inline constexpr size_t MAX_PACKET_SIZE = 1200;
    inline constexpr size_t MAX_FRAGMENT_COUNT = 64;
    inline constexpr size_t MAX_FRAME_PAYLOAD = 1024;
    inline constexpr size_t MAX_SESSION_ID = 32;
    inline constexpr size_t MAX_MESSAGE_TYPE = 128;

    // Byte order conversion functions (network byte order = big endian)
    inline uint16_t htons(uint16_t v) { return static_cast<uint16_t>((v << 8) | (v >> 8)); }
    inline uint16_t ntohs(uint16_t v) { return htons(v); }
    inline uint32_t htonl(uint32_t v) { return ((v >> 24) & 0x000000FF) | ((v >> 8) & 0x0000FF00) | ((v << 8) & 0x00FF0000) | ((v << 24) & 0xFF000000); }
    inline uint32_t ntohl(uint32_t v) { return htonl(v); }

    // Validate protocol version
    inline bool isValidProtocolVersion(uint8_t version) noexcept
    {
        return version == PROTOCOL_VERSION;
    }

    // Validate message type (0-127 valid per protocol spec)
    inline bool isValidMessageType(uint8_t type) noexcept
    {
        return type < MAX_MESSAGE_TYPE;
    }

    // Protocol packet structure (56 bytes after alignment)
    struct PacketHeader
    {
        uint32_t magic;
        uint8_t  protocol_version;
        uint8_t  message_type;
        uint8_t  flags;
        uint16_t payload_length;
        uint32_t sequence_number;
        uint8_t  session_id[MAX_SESSION_ID];
        uint32_t timestamp;
        uint16_t fragment_index;
        uint16_t fragment_count;

        static constexpr size_t HEADER_SIZE = 56;
    };

    struct FrameHeader
    {
        uint32_t frame_id;
        uint32_t capture_timestamp;
        uint16_t fragment_index;
        uint16_t fragment_count;
    };

    enum class ErrorCode : uint8_t
    {
        SUCCESS = 0,
        INVALID_PACKET = 1,
        INVALID_MAGIC = 2,
        INVALID_VERSION = 3,
        INVALID_MESSAGE_TYPE = 4,
        INVALID_PAYLOAD_LENGTH = 5,
        INVALID_SESSION_ID = 6,
        INVALID_FRAME_ID = 7,
        INVALID_FRAGMENT_INDEX = 8,
        INVALID_FRAGMENT_COUNT = 9,
        PAYLOAD_TOO_LARGE = 10,
        TRUNCATED_PACKET = 11,
        INVALID_FRAGMENT_LENGTH = 12,
        SESSION_EXPIRED = 13,
        AUTHENTICATION_REQUIRED = 14,
        INVALID_ARGUMENT = 15,
        MAX_ERROR_CODE = 15
    };

    class ProtocolError : public std::runtime_error
    {
    public:
        explicit ProtocolError(ErrorCode code, const std::string& message)
            : std::runtime_error(message), code_(code) {}
        ErrorCode code() const { return code_; }
    private:
        ErrorCode code_;
    };

    class ProtocolSerializer
    {
    public:
        ProtocolSerializer() = default;

        size_t serialize(const PacketHeader& header, const std::vector<uint8_t>& payload, std::vector<uint8_t>& out);
        size_t serialize(const PacketHeader& header, const FrameHeader& frame, const std::vector<uint8_t>& payload, std::vector<uint8_t>& out);
        size_t deserialize(const std::vector<uint8_t>& data, PacketHeader& header, std::vector<uint8_t>& payload);
        size_t deserialize(const std::vector<uint8_t>& data, PacketHeader& header, FrameHeader& frame, std::vector<uint8_t>& payload);
        void validate(const PacketHeader& header) const;
        bool withinLimits(const PacketHeader& header) const;
        static constexpr size_t maxPacketSize() { return MAX_PACKET_SIZE; }
        static constexpr size_t maxFramePayload() { return MAX_FRAME_PAYLOAD; }
        static constexpr uint16_t maxFragmentCount() { return MAX_FRAGMENT_COUNT; }
        static constexpr uint8_t messageTypeCount() { return static_cast<uint8_t>(MessageType::MAX_MESSAGE_TYPE) + 1; }
    };

    class SessionId
    {
    public:
        SessionId() { std::memset(data_, 0, sizeof(data_)); length_ = 0; }
        SessionId(const std::string& id) { set(id); }
        SessionId(const uint8_t* id, size_t len) { set(id, len); }
        void set(const std::string& id);
        void set(const uint8_t* id, size_t len);
        std::string toString() const;
        bool empty() const { return length_ == 0; }
        size_t length() const { return length_; }
    private:
        uint8_t data_[MAX_SESSION_ID];
        size_t length_;
    };
} // namespace linkforge
