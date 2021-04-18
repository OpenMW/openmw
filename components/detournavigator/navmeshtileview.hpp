#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHTILEVIEW_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHTILEVIEW_H

struct dtMeshHeader;
struct dtPoly;
struct dtPolyDetail;
struct dtBVNode;
struct dtOffMeshConnection;
struct dtMeshTile;

namespace DetourNavigator
{
    struct NavMeshTileConstView
    {
        const dtMeshHeader* mHeader;
        const dtPoly* mPolys;
        const float* mVerts;
        const dtPolyDetail* mDetailMeshes;
        const float* mDetailVerts;
        const unsigned char* mDetailTris;
        const dtBVNode* mBvTree;
        const dtOffMeshConnection* mOffMeshCons;

        friend bool operator==(const NavMeshTileConstView& lhs, const NavMeshTileConstView& rhs);
    };

    NavMeshTileConstView asNavMeshTileConstView(const unsigned char* data);
    NavMeshTileConstView asNavMeshTileConstView(const dtMeshTile& tile);
}

#endif
