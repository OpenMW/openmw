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

        char brace;
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
                level.mSegmentsCrossSection = stream.readInt();
                return;
            case 6009:
                level.mSegmentsLength = stream.readInt();
                return;
            case 6010:
                level.mGenerationFirst = stream.readFloat();
                return;
            case 6011:
                level.mGenerationLast = stream.readFloat();
                return;
            case 6012:
                level.mGenerationFrequency = stream.readFloat();
                return;
            case 6013:
                level.mTextureTilingU = stream.readFloat();
                return;
            case 6014:
                level.mTextureTilingV = stream.readFloat();
                return;
            case 6015:
                level.mTextureTilingUAbs = stream.readByte();
                return;
            case 6016:
                level.mTextureTilingVAbs = stream.readByte();
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
                    tex.mMakeBlossom = stream.readByte();
                    break;
                case 4001:
                    stream.readVec3f(tex.mColor);
                    break;
                case 4002:
                    tex.mOrientationVariance = stream.readFloat();
                    break;
                case 4003:
                    tex.mFilename = stream.readIntString();
                    break;
                case 4004:
                    stream.readVec3f(tex.mOrigin);
                    break;
                case 4005:
                    stream.readVec3f(tex.mSize);
                    break;
                case 4006:
                    stream.readVec3f(tex.mUnknown4006);
                    break;
                case 4007:
                    tex.mUnknown4007 = stream.readFloat();
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
                    mBlossomDistance = stream.readFloat();
                    break;
                case 3001:
                    mBlossomDepth = stream.readInt();
                    break;
                case 3002:
                    mBlossomWeight = stream.readFloat();
                    break;
                case 3003:
                    mUnknown3003 = stream.readByte();
                    break;
                case 3004:
                    mUnknown3004 = stream.readFloat();
                    break;
                case 3009:
                    mLightingDimmingToggle = stream.readByte();
                    break;
                case 3010:
                    mLightingDimming = stream.readFloat();
                    break;
                case 3005:
                    mUnknown3005 = stream.readFloat();
                    break;
                case 3006:
                    mUnknown3006 = stream.readByte();
                    break;
                case 3007:
                    mPlacementTolerance = stream.readFloat();
                    break;
                case 3008:
                    mPlacementCollision = stream.readInt();
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
                                mNumLeafTextures = stream.readInt();
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
                    mUnknown14003 = stream.readFloat();
                    break;
                case 14004:
                    mSizeFactor = stream.readFloat();
                    break;
                case 14005:
                    mMinOffsetAngle = stream.readFloat();
                    break;
                case 14006:
                    mMaxOffsetAngle = stream.readFloat();
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
                    mGenerationLevel = stream.readInt();
                    break;
                case 13003:
                    mBladesOrExtruded = stream.readInt();
                    break;
                case 13004:
                    mBladeCount = stream.readInt();
                    break;
                case 13005:
                    mExtrudedProfile = stream.readIntString();
                    break;
                case 13006:
                    mExtrudedSegments = stream.readInt();
                    break;
                case 13007:
                    mEnabled = stream.readByte();
                    break;
                case 13008:
                    mNumTextureMaps = stream.readInt();
                    break;
                case 13009:
                    mNumLods = stream.readInt();
                    break;
                case 13010:
                    mMaxArea = stream.readFloat();
                    break;
                case 13011:
                    mMinArea = stream.readFloat();
                    break;
                case 13012:
                    mFuzziness = stream.readFloat();
                    break;
                case 13013:
                    mRetention = stream.readFloat();
                    break;
                case 14007:
                    mMinExtrusionSegLength = stream.readInt();
                    break;
                case 14008:
                    mMinExtrusionSegProfile = stream.readInt();
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