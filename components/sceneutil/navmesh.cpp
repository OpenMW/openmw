#include "navmesh.hpp"

#include "detourdebugdraw.hpp"

#include <components/detournavigator/settings.hpp>

#include <DetourDebugDraw.h>

#include <osg/Group>

namespace
{
    // Copied from
    // https://github.com/recastnavigation/recastnavigation/blob/c5cbd53024c8a9d8d097a4371215e3342d2fdc87/DebugUtils/Source/DetourDebugDraw.cpp#L26-L38
    float distancePtLine2d(const float* pt, const float* p, const float* q)
    {
        float pqx = q[0] - p[0];
        float pqz = q[2] - p[2];
        float dx = pt[0] - p[0];
        float dz = pt[2] - p[2];
        float d = pqx * pqx + pqz * pqz;
        float t = pqx * dx + pqz * dz;
        if (d != 0)
            t /= d;
        dx = p[0] + t * pqx - pt[0];
        dz = p[2] + t * pqz - pt[2];
        return dx * dx + dz * dz;
    }

    // Copied from
    // https://github.com/recastnavigation/recastnavigation/blob/c5cbd53024c8a9d8d097a4371215e3342d2fdc87/DebugUtils/Source/DetourDebugDraw.cpp#L40-L118
    void drawPolyBoundaries(
        duDebugDraw* dd, const dtMeshTile* tile, const unsigned int col, const float linew, bool inner)
    {
        static const float thr = 0.01f * 0.01f;

        dd->begin(DU_DRAW_LINES, linew);

        for (int i = 0; i < tile->header->polyCount; ++i)
        {
            const dtPoly* p = &tile->polys[i];

            if (p->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
                continue;

            const dtPolyDetail* pd = &tile->detailMeshes[i];

            for (int j = 0, nj = (int)p->vertCount; j < nj; ++j)
            {
                unsigned int c = col;
                if (inner)
                {
                    if (p->neis[j] == 0)
                        continue;
                    if (p->neis[j] & DT_EXT_LINK)
                    {
                        bool con = false;
                        for (unsigned int k = p->firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
                        {
                            if (tile->links[k].edge == j)
                            {
                                con = true;
                                break;
                            }
                        }
                        if (con)
                            c = duRGBA(255, 255, 255, 48);
                        else
                            c = duRGBA(0, 0, 0, 48);
                    }
                    else
                        c = duRGBA(0, 48, 64, 32);
                }
                else
                {
                    if (p->neis[j] != 0)
                        continue;
                }

                const float* v0 = &tile->verts[p->verts[j] * 3];
                const float* v1 = &tile->verts[p->verts[(j + 1) % nj] * 3];

                // Draw detail mesh edges which align with the actual poly edge.
                // This is really slow.
                for (int k = 0; k < pd->triCount; ++k)
                {
                    const unsigned char* t = &tile->detailTris[(pd->triBase + k) * 4];
                    const float* tv[3];
                    for (int m = 0; m < 3; ++m)
                    {
                        if (t[m] < p->vertCount)
                            tv[m] = &tile->verts[p->verts[t[m]] * 3];
                        else
                            tv[m] = &tile->detailVerts[(pd->vertBase + (t[m] - p->vertCount)) * 3];
                    }
                    for (int m = 0, n = 2; m < 3; n = m++)
                    {
                        if ((dtGetDetailTriEdgeFlags(t[3], n) & DT_DETAIL_EDGE_BOUNDARY) == 0)
                            continue;

                        if (distancePtLine2d(tv[n], v0, v1) < thr && distancePtLine2d(tv[m], v0, v1) < thr)
                        {
                            dd->vertex(tv[n], c);
                            dd->vertex(tv[m], c);
                        }
                    }
                }
            }
        }
        dd->end();
    }

    float getHeat(unsigned salt, unsigned minSalt, unsigned maxSalt)
    {
        if (salt < minSalt)
            return 0;
        if (salt > maxSalt)
            return 1;
        if (maxSalt <= minSalt)
            return 0.5;
        return static_cast<float>(salt - minSalt) / static_cast<float>(maxSalt - minSalt);
    }

    int getRgbaComponent(float v, int base)
    {
        return static_cast<int>(std::round(v * base));
    }

    unsigned heatToColor(float heat, int alpha)
    {
        constexpr int min = 100;
        constexpr int max = 200;
        if (heat < 0.25f)
            return duRGBA(min, min + getRgbaComponent(4 * heat, max - min), max, alpha);
        if (heat < 0.5f)
            return duRGBA(min, max, min + getRgbaComponent(1 - 4 * (heat - 0.5f), max - min), alpha);
        if (heat < 0.75f)
            return duRGBA(min + getRgbaComponent(4 * (heat - 0.5f), max - min), max, min, alpha);
        return duRGBA(max, min + getRgbaComponent(1 - 4 * (heat - 0.75f), max - min), min, alpha);
    }

    // Based on
    // https://github.com/recastnavigation/recastnavigation/blob/c5cbd53024c8a9d8d097a4371215e3342d2fdc87/DebugUtils/Source/DetourDebugDraw.cpp#L120-L235
    void drawMeshTile(duDebugDraw* dd, const dtNavMesh& mesh, const dtNavMeshQuery* query, const dtMeshTile* tile,
        unsigned char flags, float heat)
    {
        using namespace SceneUtil;

        dtPolyRef base = mesh.getPolyRefBase(tile);

        int tileNum = mesh.decodePolyIdTile(base);
        const unsigned alpha = tile->header->userId == 0 ? 64 : 128;
        const unsigned int tileNumColor = duIntToCol(tileNum, alpha);

        dd->depthMask(false);

        dd->begin(DU_DRAW_TRIS);
        for (int i = 0; i < tile->header->polyCount; ++i)
        {
            const dtPoly* p = &tile->polys[i];
            if (p->getType() == DT_POLYTYPE_OFFMESH_CONNECTION) // Skip off-mesh links.
                continue;

            const dtPolyDetail* pd = &tile->detailMeshes[i];

            unsigned int col;
            if (query && query->isInClosedList(base | (dtPolyRef)i))
                col = duRGBA(255, 196, 0, alpha);
            else
            {
                if (flags & NavMeshTileDrawFlagsColorTiles)
                    col = duTransCol(tileNumColor, alpha);
                else if (flags & NavMeshTileDrawFlagsHeat)
                    col = heatToColor(heat, alpha);
                else
                    col = duTransCol(dd->areaToCol(p->getArea()), alpha);
            }

            for (int j = 0; j < pd->triCount; ++j)
            {
                const unsigned char* t = &tile->detailTris[(pd->triBase + j) * 4];
                for (int k = 0; k < 3; ++k)
                {
                    if (t[k] < p->vertCount)
                        dd->vertex(&tile->verts[p->verts[t[k]] * 3], col);
                    else
                        dd->vertex(&tile->detailVerts[(pd->vertBase + t[k] - p->vertCount) * 3], col);
                }
            }
        }
        dd->end();

        // Draw inter poly boundaries
        drawPolyBoundaries(dd, tile, duRGBA(0, 48, 64, 32), 1.5f, true);

        // Draw outer poly boundaries
        drawPolyBoundaries(dd, tile, duRGBA(0, 48, 64, 220), 2.5f, false);

        if (flags & NavMeshTileDrawFlagsOffMeshConnections)
        {
            dd->begin(DU_DRAW_LINES, 2.0f);
            for (int i = 0; i < tile->header->polyCount; ++i)
            {
                const dtPoly* p = &tile->polys[i];
                if (p->getType() != DT_POLYTYPE_OFFMESH_CONNECTION) // Skip regular polys.
                    continue;

                unsigned int col, col2;
                if (query && query->isInClosedList(base | (dtPolyRef)i))
                    col = duRGBA(255, 196, 0, 220);
                else
                    col = duDarkenCol(duTransCol(dd->areaToCol(p->getArea()), 220));

                const dtOffMeshConnection* con = &tile->offMeshCons[i - tile->header->offMeshBase];
                const float* va = &tile->verts[p->verts[0] * 3];
                const float* vb = &tile->verts[p->verts[1] * 3];

                // Check to see if start and end end-points have links.
                bool startSet = false;
                bool endSet = false;
                for (unsigned int k = p->firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
                {
                    if (tile->links[k].edge == 0)
                        startSet = true;
                    if (tile->links[k].edge == 1)
                        endSet = true;
                }

                // End points and their on-mesh locations.
                dd->vertex(va[0], va[1], va[2], col);
                dd->vertex(con->pos[0], con->pos[1], con->pos[2], col);
                col2 = startSet ? col : duRGBA(220, 32, 16, 196);
                duAppendCircle(dd, con->pos[0], con->pos[1] + 0.1f, con->pos[2], con->rad, col2);

                dd->vertex(vb[0], vb[1], vb[2], col);
                dd->vertex(con->pos[3], con->pos[4], con->pos[5], col);
                col2 = endSet ? col : duRGBA(220, 32, 16, 196);
                duAppendCircle(dd, con->pos[3], con->pos[4] + 0.1f, con->pos[5], con->rad, col2);

                // End point vertices.
                dd->vertex(con->pos[0], con->pos[1], con->pos[2], duRGBA(0, 48, 64, 196));
                dd->vertex(con->pos[0], con->pos[1] + 0.2f, con->pos[2], duRGBA(0, 48, 64, 196));

                dd->vertex(con->pos[3], con->pos[4], con->pos[5], duRGBA(0, 48, 64, 196));
                dd->vertex(con->pos[3], con->pos[4] + 0.2f, con->pos[5], duRGBA(0, 48, 64, 196));

                // Connection arc.
                duAppendArc(dd, con->pos[0], con->pos[1], con->pos[2], con->pos[3], con->pos[4], con->pos[5], 0.25f,
                    (con->flags & 1) ? 0.6f : 0, 0.6f, col);
            }
            dd->end();
        }

        const unsigned int vcol = duRGBA(0, 0, 0, 196);
        dd->begin(DU_DRAW_POINTS, 3.0f);
        for (int i = 0; i < tile->header->vertCount; ++i)
        {
            const float* v = &tile->verts[i * 3];
            dd->vertex(v[0], v[1], v[2], vcol);
        }
        dd->end();

        dd->depthMask(true);
    }
}

namespace SceneUtil
{
    osg::ref_ptr<osg::Group> createNavMeshTileGroup(const dtNavMesh& navMesh, const dtMeshTile& meshTile,
        const DetourNavigator::Settings& settings, const osg::ref_ptr<osg::StateSet>& debugDrawStateSet,
        unsigned char flags, unsigned minSalt, unsigned maxSalt)
    {
        if (meshTile.header == nullptr)
            return nullptr;

        osg::ref_ptr<osg::Group> group(new osg::Group);
        constexpr float shift = 10.0f;
        DebugDraw debugDraw(
            *group, debugDrawStateSet, osg::Vec3f(0, 0, shift), 1.0f / settings.mRecast.mRecastScaleFactor);
        dtNavMeshQuery navMeshQuery;
        navMeshQuery.init(&navMesh, settings.mDetour.mMaxNavMeshQueryNodes);
        drawMeshTile(&debugDraw, navMesh, &navMeshQuery, &meshTile, flags, getHeat(meshTile.salt, minSalt, maxSalt));

        return group;
    }
}
