#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHDB_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHDB_H

#include "tileposition.hpp"
#include "tilespositionsrange.hpp"

#include <components/esm/refid.hpp>
#include <components/misc/strongtypedef.hpp>
#include <components/sqlite3/db.hpp>
#include <components/sqlite3/statement.hpp>
#include <components/sqlite3/transaction.hpp>
#include <components/sqlite3/types.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

struct sqlite3;
struct sqlite3_stmt;

namespace DetourNavigator
{
    using TileId = Misc::StrongTypedef<std::int64_t, struct TiledIdTag>;
    using TileRevision = Misc::StrongTypedef<std::int64_t, struct TileRevisionTag>;
    using TileVersion = Misc::StrongTypedef<std::int64_t, struct TileVersionTag>;
    using ShapeId = Misc::StrongTypedef<std::int64_t, struct ShapeIdTag>;

    struct Tile
    {
        TileId mTileId;
        TileVersion mVersion;
    };

    struct TileData
    {
        TileId mTileId;
        TileVersion mVersion;
        std::vector<std::byte> mData;
    };

    enum class ShapeType
    {
        Collision = 1,
        Avoid = 2,
    };

    std::ostream& operator<<(std::ostream& stream, ShapeType value);

    namespace DbQueries
    {
        struct GetMaxTileId
        {
            static std::string_view text() noexcept;
            static void bind(sqlite3&, sqlite3_stmt&) {}
        };

        struct FindTile
        {
            static std::string_view text() noexcept;
            static void bind(sqlite3& db, sqlite3_stmt& statement, std::string_view worldspace,
                const TilePosition& tilePosition, const std::vector<std::byte>& input);
        };

        struct GetTileData
        {
            static std::string_view text() noexcept;
            static void bind(sqlite3& db, sqlite3_stmt& statement, std::string_view worldspace,
                const TilePosition& tilePosition, const std::vector<std::byte>& input);
        };

        struct InsertTile
        {
            static std::string_view text() noexcept;
            static void bind(sqlite3& db, sqlite3_stmt& statement, TileId tileId, std::string_view worldspace,
                const TilePosition& tilePosition, TileVersion version, const std::vector<std::byte>& input,
                const std::vector<std::byte>& data);
        };

        struct UpdateTile
        {
            static std::string_view text() noexcept;
            static void bind(sqlite3& db, sqlite3_stmt& statement, TileId tileId, TileVersion version,
                const std::vector<std::byte>& data);
        };

        struct DeleteTilesAt
        {
            static std::string_view text() noexcept;
            static void bind(
                sqlite3& db, sqlite3_stmt& statement, std::string_view worldspace, const TilePosition& tilePosition);
        };

        struct DeleteTilesAtExcept
        {
            static std::string_view text() noexcept;
            static void bind(sqlite3& db, sqlite3_stmt& statement, std::string_view worldspace,
                const TilePosition& tilePosition, TileId excludeTileId);
        };

        struct DeleteTilesOutsideRange
        {
            static std::string_view text() noexcept;
            static void bind(
                sqlite3& db, sqlite3_stmt& statement, std::string_view worldspace, const TilesPositionsRange& range);
        };

        struct GetMaxShapeId
        {
            static std::string_view text() noexcept;
            static void bind(sqlite3&, sqlite3_stmt&) {}
        };

        struct FindShapeId
        {
            static std::string_view text() noexcept;
            static void bind(sqlite3& db, sqlite3_stmt& statement, std::string_view name, ShapeType type,
                const Sqlite3::ConstBlob& hash);
        };

        struct InsertShape
        {
            static std::string_view text() noexcept;
            static void bind(sqlite3& db, sqlite3_stmt& statement, ShapeId shapeId, std::string_view name,
                ShapeType type, const Sqlite3::ConstBlob& hash);
        };

        struct Vacuum
        {
            static std::string_view text() noexcept;
            static void bind(sqlite3&, sqlite3_stmt&) {}
        };
    }

    class NavMeshDb
    {
    public:
        explicit NavMeshDb(std::string_view path, std::uint64_t maxFileSize);

        Sqlite3::Transaction startTransaction(Sqlite3::TransactionMode mode = Sqlite3::TransactionMode::Default);

        TileId getMaxTileId();

        std::optional<Tile> findTile(
            ESM::RefId worldspace, const TilePosition& tilePosition, const std::vector<std::byte>& input);

        std::optional<TileData> getTileData(
            ESM::RefId worldspace, const TilePosition& tilePosition, const std::vector<std::byte>& input);

        int insertTile(TileId tileId, ESM::RefId worldspace, const TilePosition& tilePosition, TileVersion version,
            const std::vector<std::byte>& input, const std::vector<std::byte>& data);

        int updateTile(TileId tileId, TileVersion version, const std::vector<std::byte>& data);

        int deleteTilesAt(ESM::RefId worldspace, const TilePosition& tilePosition);

        int deleteTilesAtExcept(ESM::RefId worldspace, const TilePosition& tilePosition, TileId excludeTileId);

        int deleteTilesOutsideRange(ESM::RefId worldspace, const TilesPositionsRange& range);

        ShapeId getMaxShapeId();

        std::optional<ShapeId> findShapeId(std::string_view name, ShapeType type, const Sqlite3::ConstBlob& hash);

        int insertShape(ShapeId shapeId, std::string_view name, ShapeType type, const Sqlite3::ConstBlob& hash);

        void vacuum();

    private:
        Sqlite3::Db mDb;
        Sqlite3::Statement<DbQueries::GetMaxTileId> mGetMaxTileId;
        Sqlite3::Statement<DbQueries::FindTile> mFindTile;
        Sqlite3::Statement<DbQueries::GetTileData> mGetTileData;
        Sqlite3::Statement<DbQueries::InsertTile> mInsertTile;
        Sqlite3::Statement<DbQueries::UpdateTile> mUpdateTile;
        Sqlite3::Statement<DbQueries::DeleteTilesAt> mDeleteTilesAt;
        Sqlite3::Statement<DbQueries::DeleteTilesAtExcept> mDeleteTilesAtExcept;
        Sqlite3::Statement<DbQueries::DeleteTilesOutsideRange> mDeleteTilesOutsideRange;
        Sqlite3::Statement<DbQueries::GetMaxShapeId> mGetMaxShapeId;
        Sqlite3::Statement<DbQueries::FindShapeId> mFindShapeId;
        Sqlite3::Statement<DbQueries::InsertShape> mInsertShape;
        Sqlite3::Statement<DbQueries::Vacuum> mVacuum;
    };
}

#endif
