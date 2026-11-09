// SPDX-License-Identifier: GPL-3.0-only
#include "file.hpp"

#include <format>
#include <stdexcept>
#include <vector>

#include "stream.hpp"

namespace Spt
{
    namespace
    {
        std::vector<SPTCompositeMapData::Quad> readQuads(SPTStream& stream)
        {
            uint32_t n;
            stream.read(n);
            std::vector<SPTCompositeMapData::Quad> quads;
            quads.reserve(n);
            for (uint32_t i = 0; i < n; ++i)
            {
                SPTCompositeMapData::Quad quad;
                for (auto& corner : quad.mCorners)
                    stream.read(corner);
                quads.push_back(quad);
            }
            return quads;
        }
    }

    void SPTCompositeMapData::read(SPTStream& stream)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            if (section == 10001)
                return;
            switch (section)
            {
                case 10002:
                    mLeafQuads = readQuads(stream);
                    break;
                case 10003:
                    mBillboardQuads = readQuads(stream);
                    break;
                case 10004:
                    mFrondQuads = readQuads(stream);
                    break;
                default:
                    throw std::runtime_error(std::format("Unknown section {} in composite map block", section));
            }
        }
    }

    void SPTGlobalCompositeMapData::read(SPTStream& stream)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            if (section == 20001)
                return;
            switch (section)
            {
                case 20002:
                    mFilename = stream.readIntString();
                    break;
                case 20003:
                    stream.read(mUnknown20003);
                    break;
                case 20004:
                    stream.read(mUnknown20004);
                    break;
                case 20005:
                    for (auto& tc : mTexCoords)
                        stream.read(tc);
                    break;
                default:
                    throw std::runtime_error(std::format("Unknown section {} in global composite map block", section));
            }
        }
    }
}