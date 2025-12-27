#ifndef OPENMW_COMPONENTS_NAVMESHTOOL_PROTOCOL_H
#define OPENMW_COMPONENTS_NAVMESHTOOL_PROTOCOL_H

#include <cstddef>
#include <cstdint>
#include <variant>
#include <vector>

namespace NavMeshTool
{
    inline constexpr char messageMagic[] = { 'n', 'v', 't', 'm' };

    enum class MessageType : std::uint64_t
    {
        ExpectedCells = 1,
        ProcessedCells = 2,
        ExpectedTiles = 3,
        GeneratedTiles = 4,
    };

    struct Message
    {
        std::uint64_t mType = 0;
        std::uint64_t mSize = 0;
        const std::byte* mData = nullptr;
    };

    struct ExpectedCells
    {
        static constexpr MessageType sMessageType = MessageType::ExpectedCells;
        std::uint64_t mCount = 0;
    };

    struct ProcessedCells
    {
        static constexpr MessageType sMessageType = MessageType::ProcessedCells;
        std::uint64_t mCount = 0;
    };

    struct ExpectedTiles
    {
        static constexpr MessageType sMessageType = MessageType::ExpectedTiles;
        std::uint64_t mCount = 0;
    };

    struct GeneratedTiles
    {
        static constexpr MessageType sMessageType = MessageType::GeneratedTiles;
        std::uint64_t mCount = 0;
    };

    using TypedMessage = std::variant<ExpectedCells, ProcessedCells, ExpectedTiles, GeneratedTiles>;

    std::vector<std::byte> serialize(const ExpectedCells& value);

    std::vector<std::byte> serialize(const ProcessedCells& value);

    std::vector<std::byte> serialize(const ExpectedTiles& value);

    std::vector<std::byte> serialize(const GeneratedTiles& value);

    const std::byte* deserialize(const std::byte* begin, const std::byte* end, Message& message);

    TypedMessage decode(const Message& message);
}

#endif
