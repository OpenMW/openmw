#include "navmeshdb.hpp"

#include <components/debug/debuglog.hpp>
#include <components/misc/compression.hpp>
#include <components/sqlite3/db.hpp>
#include <components/sqlite3/request.hpp>

#include <DetourAlloc.h>

#include <sqlite3.h>

#include <cstddef>
#include <format>
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

            CREATE INDEX IF NOT EXISTS index_tiles_by_worldspace_and_tile_position
                ON tiles (worldspace, tile_position_x, tile_position_y);

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

        constexpr std::string_view deleteTilesAtQuery = R"(
            DELETE FROM tiles
             WHERE worldspace = :worldspace
               AND tile_position_x = :tile_position_x
               AND tile_position_y = :tile_position_y
        )";

        constexpr std::string_view deleteTilesAtExceptQuery = R"(
            DELETE FROM tiles
             WHERE worldspace = :worldspace
               AND tile_position_x = :tile_position_x
               AND tile_position_y = :tile_position_y
               AND tile_id != :exclude_tile_id
        )";

        constexpr std::string_view deleteTilesOutsideRangeQuery = R"(
            DELETE FROM tiles
             WHERE worldspace = :worldspace
               AND (   tile_position_x < :begin_tile_position_x
                    OR tile_position_y < :begin_tile_position_y
                    OR tile_position_x >= :end_tile_position_x
                    OR tile_position_y >= :end_tile_position_y
                   )
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

        constexpr std::string_view vacuumQuery = R"(
            VACUUM;
        )";

        struct GetPageSize
        {
            static std::string_view text() noexcept { return "pragma page_size;"; }
            static void bind(sqlite3&, sqlite3_stmt&) {}
        };

        std::uint64_t getPageSize(sqlite3& db)
        {
            Sqlite3::Statement<GetPageSize> statement(db);
            std::uint64_t value = 0;
            request(db, statement, &value, 1);
            return value;
        }

        void setMaxPageCount(sqlite3& db, std::uint64_t value)
        {
            const auto query = std::format("pragma max_page_count = {};", value);
            if (const int ec = sqlite3_exec(&db, query.c_str(), nullptr, nullptr, nullptr); ec != SQLITE_OK)
                throw std::runtime_error("Failed set max page count: " + std::string(sqlite3_errmsg(&db)));
        }
    }

    std::ostream& operator<<(std::ostream& stream, ShapeType value)
    {
        switch (value)
        {
            case ShapeType::Collision:
                return stream << "collision";
            case ShapeType::Avoid:
                return stream << "avoid";
        }
        return stream << "unknown shape type (" << static_cast<std::underlying_type_t<ShapeType>>(value) << ")";
    }

    NavMeshDb::NavMeshDb(std::string_view path, std::uint64_t maxFileSize)
        : mDb(Sqlite3::makeDb(path, schema))
        , mGetMaxTileId(*mDb, DbQueries::GetMaxTileId{})
        , mFindTile(*mDb, DbQueries::FindTile{})
        , mGetTileData(*mDb, DbQueries::GetTileData{})
        , mInsertTile(*mDb, DbQueries::InsertTile{})
        , mUpdateTile(*mDb, DbQueries::UpdateTile{})
        , mDeleteTilesAt(*mDb, DbQueries::DeleteTilesAt{})
        , mDeleteTilesAtExcept(*mDb, DbQueries::DeleteTilesAtExcept{})
        , mDeleteTilesOutsideRange(*mDb, DbQueries::DeleteTilesOutsideRange{})
        , mGetMaxShapeId(*mDb, DbQueries::GetMaxShapeId{})
        , mFindShapeId(*mDb, DbQueries::FindShapeId{})
        , mInsertShape(*mDb, DbQueries::InsertShape{})
        , mVacuum(*mDb, DbQueries::Vacuum{})
    {
        const std::uint64_t dbPageSize = getPageSize(*mDb);
        if (dbPageSize == 0)
            throw std::runtime_error("NavMeshDb page size is zero");
        setMaxPageCount(*mDb, maxFileSize / dbPageSize + static_cast<std::uint64_t>((maxFileSize % dbPageSize) != 0));
    }

    Sqlite3::Transaction NavMeshDb::startTransaction(Sqlite3::TransactionMode mode)
    {
        return Sqlite3::Transaction(*mDb, mode);
    }

    TileId NavMeshDb::getMaxTileId()
    {
        TileId tileId{ 0 };
        request(*mDb, mGetMaxTileId, &tileId.mValue, 1);
        return tileId;
    }

    std::optional<Tile> NavMeshDb::findTile(
        ESM::RefId worldspace, const TilePosition& tilePosition, const std::vector<std::byte>& input)
    {
        Tile result;
        auto row = std::tie(result.mTileId.mValue, result.mVersion.mValue);
        const std::vector<std::byte> compressedInput = Misc::compress(input);
        if (&row == request(*mDb, mFindTile, &row, 1, worldspace.serializeText(), tilePosition, compressedInput))
            return {};
        return result;
    }

    std::optional<TileData> NavMeshDb::getTileData(
        ESM::RefId worldspace, const TilePosition& tilePosition, const std::vector<std::byte>& input)
    {
        TileData result;
        auto row = std::tie(result.mTileId.mValue, result.mVersion.mValue, result.mData);
        const std::vector<std::byte> compressedInput = Misc::compress(input);
        if (&row == request(*mDb, mGetTileData, &row, 1, worldspace.serializeText(), tilePosition, compressedInput))
            return {};
        result.mData = Misc::decompress(result.mData);
        return result;
    }

    int NavMeshDb::insertTile(TileId tileId, ESM::RefId worldspace, const TilePosition& tilePosition,
        TileVersion version, const std::vector<std::byte>& input, const std::vector<std::byte>& data)
    {
        const std::vector<std::byte> compressedInput = Misc::compress(input);
        const std::vector<std::byte> compressedData = Misc::compress(data);
        return execute(*mDb, mInsertTile, tileId, worldspace.serializeText(), tilePosition, version, compressedInput,
            compressedData);
    }

    int NavMeshDb::updateTile(TileId tileId, TileVersion version, const std::vector<std::byte>& data)
    {
        const std::vector<std::byte> compressedData = Misc::compress(data);
        return execute(*mDb, mUpdateTile, tileId, version, compressedData);
    }

    int NavMeshDb::deleteTilesAt(ESM::RefId worldspace, const TilePosition& tilePosition)
    {
        return execute(*mDb, mDeleteTilesAt, worldspace.serializeText(), tilePosition);
    }

    int NavMeshDb::deleteTilesAtExcept(ESM::RefId worldspace, const TilePosition& tilePosition, TileId excludeTileId)
    {
        return execute(*mDb, mDeleteTilesAtExcept, worldspace.serializeText(), tilePosition, excludeTileId);
    }

    int NavMeshDb::deleteTilesOutsideRange(ESM::RefId worldspace, const TilesPositionsRange& range)
    {
        return execute(*mDb, mDeleteTilesOutsideRange, worldspace.serializeText(), range);
    }

    ShapeId NavMeshDb::getMaxShapeId()
    {
        ShapeId shapeId{ 0 };
        request(*mDb, mGetMaxShapeId, &shapeId.mValue, 1);
        return shapeId;
    }

    std::optional<ShapeId> NavMeshDb::findShapeId(std::string_view name, ShapeType type, const Sqlite3::ConstBlob& hash)
    {
        ShapeId shapeId;
        if (&shapeId.mValue == request(*mDb, mFindShapeId, &shapeId.mValue, 1, name, type, hash))
            return {};
        return shapeId;
    }

    int NavMeshDb::insertShape(ShapeId shapeId, std::string_view name, ShapeType type, const Sqlite3::ConstBlob& hash)
    {
        return execute(*mDb, mInsertShape, shapeId, name, type, hash);
    }

    void NavMeshDb::vacuum()
    {
        execute(*mDb, mVacuum);
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

        void FindTile::bind(sqlite3& db, sqlite3_stmt& statement, std::string_view worldspace,
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

        void GetTileData::bind(sqlite3& db, sqlite3_stmt& statement, std::string_view worldspace,
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

        void InsertTile::bind(sqlite3& db, sqlite3_stmt& statement, TileId tileId, std::string_view worldspace,
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

        std::string_view DeleteTilesAt::text() noexcept
        {
            return deleteTilesAtQuery;
        }

        void DeleteTilesAt::bind(
            sqlite3& db, sqlite3_stmt& statement, std::string_view worldspace, const TilePosition& tilePosition)
        {
            Sqlite3::bindParameter(db, statement, ":worldspace", worldspace);
            Sqlite3::bindParameter(db, statement, ":tile_position_x", tilePosition.x());
            Sqlite3::bindParameter(db, statement, ":tile_position_y", tilePosition.y());
        }

        std::string_view DeleteTilesAtExcept::text() noexcept
        {
            return deleteTilesAtExceptQuery;
        }

        void DeleteTilesAtExcept::bind(sqlite3& db, sqlite3_stmt& statement, std::string_view worldspace,
            const TilePosition& tilePosition, TileId excludeTileId)
        {
            Sqlite3::bindParameter(db, statement, ":worldspace", worldspace);
            Sqlite3::bindParameter(db, statement, ":tile_position_x", tilePosition.x());
            Sqlite3::bindParameter(db, statement, ":tile_position_y", tilePosition.y());
            Sqlite3::bindParameter(db, statement, ":exclude_tile_id", excludeTileId);
        }

        std::string_view DeleteTilesOutsideRange::text() noexcept
        {
            return deleteTilesOutsideRangeQuery;
        }

        void DeleteTilesOutsideRange::bind(
            sqlite3& db, sqlite3_stmt& statement, std::string_view worldspace, const TilesPositionsRange& range)
        {
            Sqlite3::bindParameter(db, statement, ":worldspace", worldspace);
            Sqlite3::bindParameter(db, statement, ":begin_tile_position_x", range.mBegin.x());
            Sqlite3::bindParameter(db, statement, ":begin_tile_position_y", range.mBegin.y());
            Sqlite3::bindParameter(db, statement, ":end_tile_position_x", range.mEnd.x());
            Sqlite3::bindParameter(db, statement, ":end_tile_position_y", range.mEnd.y());
        }

        std::string_view GetMaxShapeId::text() noexcept
        {
            return getMaxShapeIdQuery;
        }

        std::string_view FindShapeId::text() noexcept
        {
            return findShapeIdQuery;
        }

        void FindShapeId::bind(
            sqlite3& db, sqlite3_stmt& statement, std::string_view name, ShapeType type, const Sqlite3::ConstBlob& hash)
        {
            Sqlite3::bindParameter(db, statement, ":name", name);
            Sqlite3::bindParameter(db, statement, ":type", static_cast<int>(type));
            Sqlite3::bindParameter(db, statement, ":hash", hash);
        }

        std::string_view InsertShape::text() noexcept
        {
            return insertShapeQuery;
        }

        void InsertShape::bind(sqlite3& db, sqlite3_stmt& statement, ShapeId shapeId, std::string_view name,
            ShapeType type, const Sqlite3::ConstBlob& hash)
        {
            Sqlite3::bindParameter(db, statement, ":shape_id", shapeId);
            Sqlite3::bindParameter(db, statement, ":name", name);
            Sqlite3::bindParameter(db, statement, ":type", static_cast<int>(type));
            Sqlite3::bindParameter(db, statement, ":hash", hash);
        }

        std::string_view Vacuum::text() noexcept
        {
            return vacuumQuery;
        }
    }
}
