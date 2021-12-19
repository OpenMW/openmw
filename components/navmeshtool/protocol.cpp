#include "protocol.hpp"

#include <components/serialization/format.hpp>
#include <components/serialization/sizeaccumulator.hpp>
#include <components/serialization/binarywriter.hpp>
#include <components/serialization/binaryreader.hpp>

#include <stdexcept>
#include <string>

namespace NavMeshTool
{
    namespace
    {
        template <Serialization::Mode mode>
        struct Format : Serialization::Format<mode, Format<mode>>
        {
            using Serialization::Format<mode, Format<mode>>::operator();

            template <class Visitor, class T>
            auto operator()(Visitor&& visitor, T& value) const
                -> std::enable_if_t<std::is_same_v<std::decay_t<T>, Message>>
            {
                if constexpr (mode == Serialization::Mode::Write)
                    visitor(*this, messageMagic);
                else
                {
                    static_assert(mode == Serialization::Mode::Read);
                    char magic[std::size(messageMagic)];
                    visitor(*this, magic);
                    if (std::memcmp(magic, messageMagic, sizeof(magic)) != 0)
                        throw BadMessageMagic();
                }
                visitor(*this, value.mType);
                visitor(*this, value.mSize);
                if constexpr (mode == Serialization::Mode::Write)
                    visitor(*this, value.mData, value.mSize);
                else
                    visitor(*this, value.mData);
            }

            template <class Visitor, class T>
            auto operator()(Visitor&& visitor, T& value) const
                -> std::enable_if_t<std::is_same_v<std::decay_t<T>, ExpectedCells>>
            {
                visitor(*this, value.mCount);
            }

            template <class Visitor, class T>
            auto operator()(Visitor&& visitor, T& value) const
                -> std::enable_if_t<std::is_same_v<std::decay_t<T>, ProcessedCells>>
            {
                visitor(*this, value.mCount);
            }

            template <class Visitor, class T>
            auto operator()(Visitor&& visitor, T& value) const
                -> std::enable_if_t<std::is_same_v<std::decay_t<T>, ExpectedTiles>>
            {
                visitor(*this, value.mCount);
            }

            template <class Visitor, class T>
            auto operator()(Visitor&& visitor, T& value) const
                -> std::enable_if_t<std::is_same_v<std::decay_t<T>, GeneratedTiles>>
            {
                visitor(*this, value.mCount);
            }
        };

        template <class T>
        std::vector<std::byte> serializeToVector(const T& value)
        {
            constexpr Format<Serialization::Mode::Write> format;
            Serialization::SizeAccumulator sizeAccumulator;
            format(sizeAccumulator, value);
            std::vector<std::byte> buffer(sizeAccumulator.value());
            format(Serialization::BinaryWriter(buffer.data(), buffer.data() + buffer.size()), value);
            return buffer;
        }

        template <class T>
        std::vector<std::byte> serializeImpl(const T& value)
        {
            const auto data = serializeToVector(value);
            const Message message {static_cast<std::uint64_t>(T::sMessageType), static_cast<std::uint64_t>(data.size()), data.data()};
            return serializeToVector(message);
        }
    }

    std::vector<std::byte> serialize(const ExpectedCells& value)
    {
        return serializeImpl(value);
    }

    std::vector<std::byte> serialize(const ProcessedCells& value)
    {
        return serializeImpl(value);
    }

    std::vector<std::byte> serialize(const ExpectedTiles& value)
    {
        return serializeImpl(value);
    }

    std::vector<std::byte> serialize(const GeneratedTiles& value)
    {
        return serializeImpl(value);
    }

    const std::byte* deserialize(const std::byte* begin, const std::byte* end, Message& message)
    {
        try
        {
            constexpr Format<Serialization::Mode::Read> format;
            Serialization::BinaryReader reader(begin, end);
            format(reader, message);
            return message.mData + message.mSize;
        }
        catch (const Serialization::NotEnoughData&)
        {
            return begin;
        }
    }

    TypedMessage decode(const Message& message)
    {
        constexpr Format<Serialization::Mode::Read> format;
        Serialization::BinaryReader reader(message.mData, message.mData + message.mSize);
        switch (static_cast<MessageType>(message.mType))
        {
            case MessageType::ExpectedCells:
            {
                ExpectedCells value;
                format(reader, value);
                return value;
            }
            case MessageType::ProcessedCells:
            {
                ProcessedCells value;
                format(reader, value);
                return value;
            }
            case MessageType::ExpectedTiles:
            {
                ExpectedTiles value;
                format(reader, value);
                return value;
            }
            case MessageType::GeneratedTiles:
            {
                GeneratedTiles value;
                format(reader, value);
                return value;
            }
        }
        throw std::logic_error("Unsupported message type: " + std::to_string(message.mType));
    }
}
