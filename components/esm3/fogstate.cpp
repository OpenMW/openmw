#include "fogstate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <osgDB/ReadFile>

#include <components/debug/debuglog.hpp>
#include <components/files/memorystream.hpp>

#include "savedgame.hpp"

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

    Files::IMemStream in(&imageData[0], imageData.size());

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

void ESM::FogState::load (ESMReader &esm)
{
    esm.getHNOT(mBounds, "BOUN");
    esm.getHNOT(mNorthMarkerAngle, "ANGL");
    int dataFormat = esm.getFormat();
    while (esm.isNextSub("FTEX"))
    {
        esm.getSubHeader();
        FogTexture tex;

        esm.getT(tex.mX);
        esm.getT(tex.mY);

        size_t imageSize = esm.getSubSize()-sizeof(int)*2;
        tex.mImageData.resize(imageSize);
        esm.getExact(&tex.mImageData[0], imageSize);

        if (dataFormat < 7)
            convertFogOfWar(tex.mImageData);

        mFogTextures.push_back(tex);
    }
}

void ESM::FogState::save (ESMWriter &esm, bool interiorCell) const
{
    if (interiorCell)
    {
        esm.writeHNT("BOUN", mBounds);
        esm.writeHNT("ANGL", mNorthMarkerAngle);
    }
    for (std::vector<FogTexture>::const_iterator it = mFogTextures.begin(); it != mFogTextures.end(); ++it)
    {
        esm.startSubRecord("FTEX");
        esm.writeT(it->mX);
        esm.writeT(it->mY);
        esm.write(&it->mImageData[0], it->mImageData.size());
        esm.endRecord("FTEX");
    }
}
