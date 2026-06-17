#include "fogstate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <osgDB/ReadFile>

#include <components/debug/debuglog.hpp>
#include <components/files/memorystream.hpp>

namespace ESM
{
    namespace
    {
        void convertFogOfWar(std::vector<char>& imageData)
        {
            if (imageData.empty())
            {
                return;
            }

            osgDB::ReaderWriter* tgaReader = osgDB::Registry::instance()->getReaderWriterForExtension("tga");
            if (!tgaReader)
            {
                Log(Debug::Error) << "Error: Unable to load fog, can't find a tga ReaderWriter";
                return;
            }

            Files::IMemStream in(imageData.data(), imageData.size());

            osgDB::ReaderWriter::ReadResult result = tgaReader->readImage(in);
            if (!result.success())
            {
                Log(Debug::Error) << "Error: Failed to read fog: " << result.message() << " code " << result.status();
                return;
            }

            osgDB::ReaderWriter* pngWriter = osgDB::Registry::instance()->getReaderWriterForExtension("png");
            if (!pngWriter)
            {
                Log(Debug::Error) << "Error: Unable to write fog, can't find a png ReaderWriter";
                return;
            }

            std::ostringstream ostream;
            osgDB::ReaderWriter::WriteResult png = pngWriter->writeImage(*result.getImage(), ostream);
            if (!png.success())
            {
                Log(Debug::Error) << "Error: Unable to write fog: " << png.message() << " code " << png.status();
                return;
            }

            std::string str = ostream.str();
            imageData = std::vector<char>(str.begin(), str.end());
        }

    }

    void FogState::load(ESMReader& esm)
    {
        if (esm.isNextSub("BOUN"))
            esm.getHT(mBounds.mMinX, mBounds.mMinY, mBounds.mMaxX, mBounds.mMaxY);
        esm.getHNOT(mNorthMarkerAngle, "ANGL");
        if (!esm.getHNOT("CNTR", mCenterX, mCenterY))
        {
            mCenterX = (mBounds.mMinX + mBounds.mMaxX) / 2;
            mCenterY = (mBounds.mMinY + mBounds.mMaxY) / 2;
        }
        const FormatVersion dataFormat = esm.getFormatVersion();
        while (esm.isNextSub("FTEX"))
        {
            esm.getSubHeader();
            FogTexture tex;

            esm.getT(tex.mX);
            esm.getT(tex.mY);

            const std::size_t imageSize = esm.getSubSize() - sizeof(int32_t) * 2;
            tex.mImageData.resize(imageSize);
            esm.getExact(tex.mImageData.data(), imageSize);

            if (dataFormat <= MaxOldFogOfWarFormatVersion)
                convertFogOfWar(tex.mImageData);

            mFogTextures.push_back(std::move(tex));
        }
    }

    void FogState::save(ESMWriter& esm, bool interiorCell) const
    {
        if (interiorCell)
        {
            esm.writeHNT("BOUN", mBounds);
            esm.writeHNT("ANGL", mNorthMarkerAngle);
            esm.startSubRecord("CNTR");
            esm.writeT(mCenterX);
            esm.writeT(mCenterY);
            esm.endRecord("CNTR");
        }
        for (const FogTexture& texture : mFogTextures)
        {
            esm.startSubRecord("FTEX");
            esm.writeT(texture.mX);
            esm.writeT(texture.mY);
            esm.write(texture.mImageData.data(), texture.mImageData.size());
            esm.endRecord("FTEX");
        }
    }

}
