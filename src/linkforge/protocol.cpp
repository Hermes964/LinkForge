#include "protocol.hpp"
#include <stdexcept>
#include <algorithm>
#include <cstring>

namespace linkforge
{
    void SessionId::set(const std::string& id)
    {
        if (id.length() > MAX_SESSION_ID)
            throw ProtocolError(ErrorCode::INVALID_SESSION_ID, "Session ID too long");
        std::memcpy(data_, id.data(), id.length());
        length_ = id.length();
    }

    void SessionId::set(const uint8_t* id, size_t len)
    {
        if (len > MAX_SESSION_ID)
            throw ProtocolError(ErrorCode::INVALID_SESSION_ID, "Session ID too long");
        std::memcpy(data_, id, len);
        length_ = len;
    }

    std::string SessionId::toString() const
    {
        return std::string(reinterpret_cast<const char*>(data_), length_);
    }

    size_t ProtocolSerializer::serialize(const PacketHeader& header, const std::vector<uint8_t>& payload, std::vector<uint8_t>& out)
    {
        out.clear();
        out.resize(PacketHeader::HEADER_SIZE + payload.size());
        std::memcpy(out.data(), &header, PacketHeader::HEADER_SIZE);
        std::memcpy(out.data() + PacketHeader::HEADER_SIZE, payload.data(), payload.size());
        return out.size();
    }

    size_t ProtocolSerializer::serialize(const PacketHeader& header, const FrameHeader& frame, const std::vector<uint8_t>& payload, std::vector<uint8_t>& out)
    {
        out.clear();
        out.resize(PacketHeader::HEADER_SIZE + sizeof(FrameHeader) + payload.size());
        std::memcpy(out.data(), &header, PacketHeader::HEADER_SIZE);
        std::memcpy(out.data() + PacketHeader::HEADER_SIZE, &frame, sizeof(FrameHeader));
        std::memcpy(out.data() + PacketHeader::HEADER_SIZE + sizeof(FrameHeader), payload.data(), payload.size());
        return out.size();
    }

    size_t ProtocolSerializer::deserialize(const std::vector<uint8_t>& data, PacketHeader& header, std::vector<uint8_t>& payload)
    {
        if (data.size() < PacketHeader::HEADER_SIZE)
            throw ProtocolError(ErrorCode::TRUNCATED_PACKET, "Packet truncated");

        std::memcpy(&header, data.data(), PacketHeader::HEADER_SIZE);

        if (header.magic != PROTOCOL_MAGIC)
            throw ProtocolError(ErrorCode::INVALID_MAGIC, "Invalid magic number");

        if (!isValidProtocolVersion(header.protocol_version))
            throw ProtocolError(ErrorCode::INVALID_VERSION, "Invalid protocol version");

        if (!isValidMessageType(header.message_type))
            throw ProtocolError(ErrorCode::INVALID_MESSAGE_TYPE, "Invalid message type");

        if (header.payload_length > MAX_PACKET_SIZE - PacketHeader::HEADER_SIZE)
            throw ProtocolError(ErrorCode::PAYLOAD_TOO_LARGE, "Payload too large");

        validate(header);

        payload.clear();
        payload.resize(header.payload_length);

        std::memcpy(payload.data(), data.data() + PacketHeader::HEADER_SIZE, header.payload_length);

        return header.payload_length + PacketHeader::HEADER_SIZE;
    }

    size_t ProtocolSerializer::deserialize(const std::vector<uint8_t>& data, PacketHeader& header, FrameHeader& frame, std::vector<uint8_t>& payload)
    {
        if (data.size() < PacketHeader::HEADER_SIZE)
            throw ProtocolError(ErrorCode::TRUNCATED_PACKET, "Packet truncated");

        std::memcpy(&header, data.data(), PacketHeader::HEADER_SIZE);

        if (header.magic != PROTOCOL_MAGIC)
            throw ProtocolError(ErrorCode::INVALID_MAGIC, "Invalid magic number");

        if (!isValidProtocolVersion(header.protocol_version))
            throw ProtocolError(ErrorCode::INVALID_VERSION, "Invalid protocol version");

        if (!isValidMessageType(header.message_type))
            throw ProtocolError(ErrorCode::INVALID_MESSAGE_TYPE, "Invalid message type");

        if (header.payload_length > MAX_PACKET_SIZE - PacketHeader::HEADER_SIZE - sizeof(FrameHeader))
            throw ProtocolError(ErrorCode::PAYLOAD_TOO_LARGE, "Payload too large");

        validate(header);

        if (data.size() < PacketHeader::HEADER_SIZE + sizeof(FrameHeader))
            throw ProtocolError(ErrorCode::TRUNCATED_PACKET, "Frame header truncated");

        std::memcpy(&frame, data.data() + PacketHeader::HEADER_SIZE, sizeof(FrameHeader));

        if (frame.fragment_index >= frame.fragment_count)
            throw ProtocolError(ErrorCode::INVALID_FRAGMENT_INDEX, "Invalid fragment index");

        if (frame.fragment_count > MAX_FRAGMENT_COUNT)
            throw ProtocolError(ErrorCode::INVALID_FRAGMENT_COUNT, "Fragment count too large");

        payload.clear();
        payload.resize(header.payload_length - sizeof(FrameHeader));

        if (data.size() < PacketHeader::HEADER_SIZE + sizeof(FrameHeader) + payload.size())
            throw ProtocolError(ErrorCode::TRUNCATED_PACKET, "Payload truncated");

        std::memcpy(payload.data(), data.data() + PacketHeader::HEADER_SIZE + sizeof(FrameHeader), payload.size());

        return payload.size() + PacketHeader::HEADER_SIZE + sizeof(FrameHeader);
    }

    void ProtocolSerializer::validate(const PacketHeader& header) const
    {
        bool isEmpty = true;
        for (size_t i = 0; i < MAX_SESSION_ID; ++i)
        {
            if (header.session_id[i] != 0)
            {
                isEmpty = false;
                break;
            }
        }

        if (!isEmpty)
        {
            for (size_t i = 0; i < MAX_SESSION_ID; ++i)
            {
                if (header.session_id[i] == 0 && i < header.session_id[0])
                    throw ProtocolError(ErrorCode::INVALID_SESSION_ID, "Session ID contains null byte");
            }
        }

        if (header.sequence_number == 0)
            throw ProtocolError(ErrorCode::INVALID_PACKET, "Sequence number must be non-zero");

        if (header.fragment_index >= header.fragment_count)
        {
            if (header.fragment_index != 0 || header.fragment_count != 1)
                throw ProtocolError(ErrorCode::INVALID_FRAGMENT_INDEX, "Invalid fragment index");
        }

        if (header.fragment_count > MAX_FRAGMENT_COUNT)
            throw ProtocolError(ErrorCode::INVALID_FRAGMENT_COUNT, "Fragment count exceeds maximum");

        if (header.timestamp > 0x7FFFFFFFFFFFFFFFULL)
            throw ProtocolError(ErrorCode::INVALID_ARGUMENT, "Invalid timestamp");
    }

    bool ProtocolSerializer::withinLimits(const PacketHeader& header) const
    {
        return header.payload_length <= MAX_PACKET_SIZE - PacketHeader::HEADER_SIZE;
    }
} // namespace linkforge
