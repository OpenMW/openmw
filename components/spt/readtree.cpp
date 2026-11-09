// SPDX-License-Identifier: GPL-3.0-only
#include "file.hpp"

#include <algorithm>
#include <format>
#include <sstream>
#include <stdexcept>
#include <string>

#include "stream.hpp"

namespace Spt
{
    Curve parseCurve(SPTStream& stream)
    {
        std::string raw = stream.readIntString();
        std::istringstream ss(raw);
        std::string magic;
        ss >> magic;
        if (magic != "BezierSpline")
            throw std::runtime_error("Invalid curve data: missing BezierSpline header");

        Curve curve;
        ss >> curve.mMin >> curve.mMax >> curve.mVariance;

        char brace{};
        ss >> brace;
        if (brace != '{')
            throw std::runtime_error("Invalid curve data: missing opening brace");

        int32_t numPoints{};
        ss >> numPoints;
        for (int32_t i = 0; i < numPoints; ++i)
        {
            ControlPoint cp;
            ss >> cp.mPosX >> cp.mPosY >> cp.mTanX >> cp.mTanY >> cp.mTanLen;
            curve.mPoints.push_back(std::move(cp));
        }

        ss >> brace;
        if (brace != '}')
            throw std::runtime_error("Invalid curve data: missing closing brace");

        return curve;
    }

    namespace
    {
        // Cubic Bezier scalar at parameter s in [0,1].
        float cubicBezier(float a, float b, float c, float d, float s)
        {
            float u = 1.0f - s;
            return u * u * u * a + 3.0f * u * u * s * b + 3.0f * u * s * s * c + s * s * s * d;
        }
    }

    // Control points are a piecewise 2D cubic Bezier (posX,posY + tangent dir*len).
    // Query is by X (progress): solve bezierX(s)=t, return bezierY(s). The old code
    // used only the Y tangent (Hermite-in-X), which flattened curves toward linear.
    float Curve::evaluate(float t) const
    {
        if (mPoints.empty())
            return t * (mMax - mMin) + mMin;

        auto remap = [&](float y) { return mMin + std::clamp(y, 0.0f, 1.0f) * (mMax - mMin); };

        if (t <= mPoints.front().mPosX)
            return remap(mPoints.front().mPosY);
        if (t >= mPoints.back().mPosX)
            return remap(mPoints.back().mPosY);

        size_t i = 0;
        for (; i + 1 < mPoints.size(); ++i)
            if (t <= mPoints[i + 1].mPosX)
                break;

        const auto& a = mPoints[i];
        const auto& b = mPoints[i + 1];
        float p0x = a.mPosX, p0y = a.mPosY;
        float p3x = b.mPosX, p3y = b.mPosY;
        float p1x = p0x + a.mTanX * a.mTanLen, p1y = p0y + a.mTanY * a.mTanLen;
        float p2x = p3x - b.mTanX * b.mTanLen, p2y = p3y - b.mTanY * b.mTanLen;

        // Bisect for the s where bezierX(s) == t (x assumed monotonic in segment).
        float lo = 0.0f, hi = 1.0f, s = 0.5f;
        for (int it = 0; it < 24; ++it)
        {
            s = 0.5f * (lo + hi);
            if (cubicBezier(p0x, p1x, p2x, p3x, s) < t)
                lo = s;
            else
                hi = s;
        }
        return remap(cubicBezier(p0y, p1y, p2y, p3y, s));
    }

    void readBezierSection(SPTLevelData& level, SPTStream& stream, int32_t section)
    {
        switch (section)
        {
            case 6000:
                level.mDisturbance = parseCurve(stream);
                return;
            case 6001:
                level.mGravity = parseCurve(stream);
                return;
            case 6002:
                level.mFlexibility = parseCurve(stream);
                return;
            case 6003:
                level.mFlexibilityProfile = parseCurve(stream);
                return;
            case 6004:
                level.mLength = parseCurve(stream);
                return;
            case 6005:
                level.mRadius = parseCurve(stream);
                return;
            case 6006:
                level.mRadiusProfile = parseCurve(stream);
                return;
            case 6007:
                level.mStartAngle = parseCurve(stream);
                return;
            case 6017:
                level.mGravityProfile = parseCurve(stream);
                return;
            case 6008:
                stream.read(level.mSegmentsCrossSection);
                return;
            case 6009:
                stream.read(level.mSegmentsLength);
                return;
            case 6010:
                stream.read(level.mGenerationFirst);
                return;
            case 6011:
                stream.read(level.mGenerationLast);
                return;
            case 6012:
                stream.read(level.mGenerationFrequency);
                return;
            case 6013:
                stream.read(level.mTextureTilingU);
                return;
            case 6014:
                stream.read(level.mTextureTilingV);
                return;
            case 6015:
                stream.read(level.mTextureTilingUAbs);
                return;
            case 6016:
                stream.read(level.mTextureTilingVAbs);
                return;
            default:
                throw std::runtime_error(std::format("Unknown bezier section {} in SPT file", section));
        }
    }

    void SPTLevelData::read(SPTStream& stream)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            if (section == 1017)
                return;
            readBezierSection(*this, stream, section);
        }
    }

    void readLeafTextureMap(SPTLeafTextureMap& tex, SPTStream& stream)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            if (section == 1008)
                return;
            switch (section)
            {
                case 4000:
                    stream.read(tex.mMakeBlossom);
                    break;
                case 4001:
                    stream.read(tex.mColor);
                    break;
                case 4002:
                    stream.read(tex.mOrientationVariance);
                    break;
                case 4003:
                    tex.mFilename = stream.readIntString();
                    break;
                case 4004:
                    stream.read(tex.mOrigin);
                    break;
                case 4005:
                    stream.read(tex.mSize);
                    break;
                case 4006:
                    stream.read(tex.mUnknown4006);
                    break;
                case 4007:
                    stream.read(tex.mUnknown4007);
                    break;
                default:
                    throw std::runtime_error(std::format("Unknown section {} in leaf texture map block", section));
            }
        }
    }

    void SPTLeafData::read(SPTStream& stream)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            if (section == 1005)
                return;
            switch (section)
            {
                case 3000:
                    stream.read(mBlossomDistance);
                    break;
                case 3001:
                    stream.read(mBlossomDepth);
                    break;
                case 3002:
                    stream.read(mBlossomWeight);
                    break;
                case 3003:
                    stream.read(mUnknown3003);
                    break;
                case 3004:
                    stream.read(mUnknown3004);
                    break;
                case 3009:
                    stream.read(mLightingDimmingToggle);
                    break;
                case 3010:
                    stream.read(mLightingDimming);
                    break;
                case 3005:
                    stream.read(mUnknown3005);
                    break;
                case 3006:
                    stream.read(mUnknown3006);
                    break;
                case 3007:
                    stream.read(mPlacementTolerance);
                    break;
                case 3008:
                    stream.read(mPlacementCollision);
                    break;
                case 1009:
                    while (true)
                    {
                        int32_t inner = stream.readSection();
                        if (inner == 1010)
                            break;
                        switch (inner)
                        {
                            case 1006:
                                stream.read(mNumLeafTextures);
                                break;
                            case 1007:
                            {
                                SPTLeafTextureMap tex;
                                readLeafTextureMap(tex, stream);
                                mLeafTextures.push_back(std::move(tex));
                                break;
                            }
                            default:
                                throw std::runtime_error(std::format("Unknown section {} in leaf 1009 block", inner));
                        }
                    }
                    break;
                default:
                    throw std::runtime_error(std::format("Unknown section {} in leaf properties block", section));
            }
        }
    }

    void SPTFrondTextureMap::read(SPTStream& stream)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            if (section == 14001)
                return;
            switch (section)
            {
                case 14002:
                    mFilename = stream.readIntString();
                    break;
                case 14003:
                    stream.read(mUnknown14003);
                    break;
                case 14004:
                    stream.read(mSizeFactor);
                    break;
                case 14005:
                    stream.read(mMinOffsetAngle);
                    break;
                case 14006:
                    stream.read(mMaxOffsetAngle);
                    break;
                default:
                    throw std::runtime_error(std::format("Unknown section {} in frond texture map block", section));
            }
        }
    }

    void SPTFrondData::read(SPTStream& stream)
    {
        while (true)
        {
            int32_t section = stream.readSection();
            if (section == 13001)
                return;
            switch (section)
            {
                case 13002:
                    stream.read(mGenerationLevel);
                    break;
                case 13003:
                    stream.read(mBladesOrExtruded);
                    break;
                case 13004:
                    stream.read(mBladeCount);
                    break;
                case 13005:
                    mExtrudedProfile = stream.readIntString();
                    break;
                case 13006:
                    stream.read(mExtrudedSegments);
                    break;
                case 13007:
                    stream.read(mEnabled);
                    break;
                case 13008:
                    stream.read(mNumTextureMaps);
                    break;
                case 13009:
                    stream.read(mNumLods);
                    break;
                case 13010:
                    stream.read(mMaxArea);
                    break;
                case 13011:
                    stream.read(mMinArea);
                    break;
                case 13012:
                    stream.read(mFuzziness);
                    break;
                case 13013:
                    stream.read(mRetention);
                    break;
                case 14007:
                    stream.read(mMinExtrusionSegLength);
                    break;
                case 14008:
                    stream.read(mMinExtrusionSegProfile);
                    break;
                case 14000:
                {
                    SPTFrondTextureMap tex;
                    tex.read(stream);
                    mTextureMaps.push_back(std::move(tex));
                    break;
                }
                default:
                    throw std::runtime_error(std::format("Unknown section {} in frond block", section));
            }
        }
    }
}