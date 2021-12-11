#include "navmeshdb.hpp"

#include <components/debug/debuglog.hpp>
#include <components/misc/compression.hpp>
#include <components/sqlite3/db.hpp>
#include <components/sqlite3/request.hpp>

#include <DetourAlloc.h>

#include <sqlite3.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace DetourNavigator
{
    namespace
    {
        constexpr const char schema[] = R"(
            BEGIN TRANSACTION;

            CREATE TABLE IF NOT EXISTS tiles (
                tile_id INTEGER PRIMARY KEY,
                revision INTEGER NOT NULL DEFAULT 1,
                worldspace TEXT NOT NULL,
                tile_position_x INTEGER NOT NULL,
                tile_position_y INTEGER NOT NULL,
                version INTEGER NOT NULL,
                input BLOB,
                data BLOB
            );

            CREATE UNIQUE INDEX IF NOT EXISTS index_unique_tiles_by_worldspace_and_tile_position_and_input
                ON tiles (worldspace, tile_position_x, tile_position_y, input);

            CREATE TABLE IF NOT EXISTS shapes (
                shape_id INTEGER PRIMARY KEY,
                name TEXT NOT NULL,
                type INTEGER NOT NULL,
                hash BLOB NOT NULL
            );

            CREATE UNIQUE INDEX IF NOT EXISTS index_unique_shapes_by_name_and_type_and_hash
                ON shapes (name, type, hash);

            COMMIT;
        )";

        constexpr std::string_view getMaxTileIdQuery = R"(
            SELECT max(tile_id) FROM tiles
        )";

        constexpr std::string_view findTileQuery = R"(
            SELECT tile_id, version
              FROM tiles
             WHERE worldspace = :worldspace
               AND tile_position_x = :tile_position_x
               AND tile_position_y = :tile_position_y
               AND input = :input
        )";

        constexpr std::string_view getTileDataQuery = R"(
            SELECT tile_id, version, data
              FROM tiles
             WHERE worldspace = :worldspace
               AND tile_position_x = :tile_position_x
               AND tile_position_y = :tile_position_y
               AND input = :input
        )";

        constexpr std::string_view insertTileQuery = R"(
            INSERT INTO tiles ( tile_id,  worldspace,  version,  tile_position_x,  tile_position_y,  input,  data)
                   VALUES     (:tile_id, :worldspace, :version, :tile_position_x, :tile_position_y, :input, :data)
        )";

        constexpr std::string_view updateTileQuery = R"(
            UPDATE tiles
               SET version = :version,
                   data = :data,
                   revision = revision + 1
             WHERE tile_id = :tile_id
        )";

        constexpr std::string_view getMaxShapeIdQuery = R"(
            SELECT max(shape_id) FROM shapes
        )";

        constexpr std::string_view findShapeIdQuery = R"(
            SELECT shape_id
              FROM shapes
             WHERE name = :name
               AND type = :type
               AND hash = :hash
        )";

        constexpr std::string_view insertShapeQuery = R"(
            INSERT INTO shapes ( shape_id,  name,  type,  hash)
                   VALUES      (:shape_id, :name, :type, :hash)
        )";
    }

    std::ostream& operator<<(std::ostream& stream, ShapeType value)
    {
        switch (value)
        {
            case ShapeType::Collision: return stream << "collision";
            case ShapeType::Avoid: return stream << "avoid";
        }
        return stream << "unknown shape type (" << static_cast<std::underlying_type_t<ShapeType>>(value) << ")";
    }

    NavMeshDb::NavMeshDb(std::string_view path)
        : mDb(Sqlite3::makeDb(path, schema))
        , mGetMaxTileId(*mDb, DbQueries::GetMaxTileId {})
        , mFindTile(*mDb, DbQueries::FindTile {})
        , mGetTileData(*mDb, DbQueries::GetTileData {})
        , mInsertTile(*mDb, DbQueries::InsertTile {})
        , mUpdateTile(*mDb, DbQueries::UpdateTile {})
        , mGetMaxShapeId(*mDb, DbQueries::GetMaxShapeId {})
        , mFindShapeId(*mDb, DbQueries::FindShapeId {})
        , mInsertShape(*mDb, DbQueries::InsertShape {})
    {
    }

    Sqlite3::Transaction NavMeshDb::startTransaction()
    {
        return Sqlite3::Transaction(*mDb);
    }

    TileId NavMeshDb::getMaxTileId()
    {
        TileId tileId {0};
        request(*mDb, mGetMaxTileId, &tileId, 1);
        return tileId;
    }

    std::optional<Tile> NavMeshDb::findTile(const std::string& worldspace,
        const TilePosition& tilePosition, const std::vector<std::byte>& input)
    {
        Tile result;
        auto row = std::tie(result.mTileId, result.mVersion);
        const std::vector<std::byte> compressedInput = Misc::compress(input);
        if (&row == request(*mDb, mFindTile, &row, 1, worldspace, tilePosition, compressedInput))
            return {};
        return result;
    }

    std::optional<TileData> NavMeshDb::getTileData(const std::string& worldspace,
        const TilePosition& tilePosition, const std::vector<std::byte>& input)
    {
        TileData result;
        auto row = std::tie(result.mTileId, result.mVersion, result.mData);
        const std::vector<std::byte> compressedInput = Misc::compress(input);
        if (&row == request(*mDb, mGetTileData, &row, 1, worldspace, tilePosition, compressedInput))
            return {};
        result.mData = Misc::decompress(result.mData);
        return result;
    }

    int NavMeshDb::insertTile(TileId tileId, const std::string& worldspace, const TilePosition& tilePosition,
        TileVersion version, const std::vector<std::byte>& input, const std::vector<std::byte>& data)
    {
        const std::vector<std::byte> compressedInput = Misc::compress(input);
        const std::vector<std::byte> compressedData = Misc::compress(data);
        return execute(*mDb, mInsertTile, tileId, worldspace, tilePosition, version, compressedInput, compressedData);
    }

    int NavMeshDb::updateTile(TileId tileId, TileVersion version, const std::vector<std::byte>& data)
    {
        const std::vector<std::byte> compressedData = Misc::compress(data);
        return execute(*mDb, mUpdateTile, tileId, version, compressedData);
    }

    ShapeId NavMeshDb::getMaxShapeId()
    {
        ShapeId shapeId {0};
        request(*mDb, mGetMaxShapeId, &shapeId, 1);
        return shapeId;
    }

    std::optional<ShapeId> NavMeshDb::findShapeId(const std::string& name, ShapeType type,
        const Sqlite3::ConstBlob& hash)
    {
        ShapeId shapeId;
        if (&shapeId == request(*mDb, mFindShapeId, &shapeId, 1, name, type, hash))
            return {};
        return shapeId;
    }

    int NavMeshDb::insertShape(ShapeId shapeId, const std::string& name, ShapeType type,
        const Sqlite3::ConstBlob& hash)
    {
        return execute(*mDb, mInsertShape, shapeId, name, type, hash);
    }

    namespace DbQueries
    {
        std::string_view GetMaxTileId::text() noexcept
        {
            return getMaxTileIdQuery;
        }

        std::string_view FindTile::text() noexcept
        {
            return findTileQuery;
        }

        void FindTile::bind(sqlite3& db, sqlite3_stmt& statement, const std::string& worldspace,
            const TilePosition& tilePosition, const std::vector<std::byte>& input)
        {
            Sqlite3::bindParameter(db, statement, ":worldspace", worldspace);
            Sqlite3::bindParameter(db, statement, ":tile_position_x", tilePosition.x());
            Sqlite3::bindParameter(db, statement, ":tile_position_y", tilePosition.y());
            Sqlite3::bindParameter(db, statement, ":input", input);
        }

        std::string_view GetTileData::text() noexcept
        {
            return getTileDataQuery;
        }

        void GetTileData::bind(sqlite3& db, sqlite3_stmt& statement, const std::string& worldspace,
            const TilePosition& tilePosition, const std::vector<std::byte>& input)
        {
            Sqlite3::bindParameter(db, statement, ":worldspace", worldspace);
            Sqlite3::bindParameter(db, statement, ":tile_position_x", tilePosition.x());
            Sqlite3::bindParameter(db, statement, ":tile_position_y", tilePosition.y());
            Sqlite3::bindParameter(db, statement, ":input", input);
        }

        std::string_view InsertTile::text() noexcept
        {
            return insertTileQuery;
        }

        void InsertTile::bind(sqlite3& db, sqlite3_stmt& statement, TileId tileId, const std::string& worldspace,
            const TilePosition& tilePosition, TileVersion version, const std::vector<std::byte>& input,
            const std::vector<std::byte>& data)
        {
            Sqlite3::bindParameter(db, statement, ":tile_id", tileId);
            Sqlite3::bindParameter(db, statement, ":worldspace", worldspace);
            Sqlite3::bindParameter(db, statement, ":tile_position_x", tilePosition.x());
            Sqlite3::bindParameter(db, statement, ":tile_position_y", tilePosition.y());
            Sqlite3::bindParameter(db, statement, ":version", version);
            Sqlite3::bindParameter(db, statement, ":input", input);
            Sqlite3::bindParameter(db, statement, ":data", data);
        }

        std::string_view UpdateTile::text() noexcept
        {
            return updateTileQuery;
        }

        void UpdateTile::bind(sqlite3& db, sqlite3_stmt& statement, TileId tileId, TileVersion version,
            const std::vector<std::byte>& data)
        {
            Sqlite3::bindParameter(db, statement, ":tile_id", tileId);
            Sqlite3::bindParameter(db, statement, ":version", version);
            Sqlite3::bindParameter(db, statement, ":data", data);
        }

        std::string_view GetMaxShapeId::text() noexcept
        {
            return getMaxShapeIdQuery;
        }

        std::string_view FindShapeId::text() noexcept
        {
            return findShapeIdQuery;
        }

        void FindShapeId::bind(sqlite3& db, sqlite3_stmt& statement, const std::string& name,
            ShapeType type, const Sqlite3::ConstBlob& hash)
        {
            Sqlite3::bindParameter(db, statement, ":name", name);
            Sqlite3::bindParameter(db, statement, ":type", static_cast<int>(type));
            Sqlite3::bindParameter(db, statement, ":hash", hash);
        }

        std::string_view InsertShape::text() noexcept
        {
            return insertShapeQuery;
        }

        void InsertShape::bind(sqlite3& db, sqlite3_stmt& statement, ShapeId shapeId, const std::string& name,
            ShapeType type, const Sqlite3::ConstBlob& hash)
        {
            Sqlite3::bindParameter(db, statement, ":shape_id", shapeId);
            Sqlite3::bindParameter(db, statement, ":name", name);
            Sqlite3::bindParameter(db, statement, ":type", static_cast<int>(type));
            Sqlite3::bindParameter(db, statement, ":hash", hash);
        }
    }
}
