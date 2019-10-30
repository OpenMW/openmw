#include "fogstate.hpp"

#include <osgDB/ReadFile>

#include <components/debug/debuglog.hpp>
#include <components/files/memorystream.hpp>

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "savedgame.hpp"

void ESM::FogState::load (ESMReader &esm)
{
    esm.getHNOT(mBounds, "BOUN");
    esm.getHNOT(mNorthMarkerAngle, "ANGL");
    while (esm.isNextSub("FTEX"))
    {
        esm.getSubHeader();
        FogTexture tex;

        esm.getT(tex.mX);
        esm.getT(tex.mY);

        size_t imageSize = esm.getSubSize()-sizeof(int)*2;
        tex.mImageData.resize(imageSize);
        esm.getExact(&tex.mImageData[0], imageSize);

        if (esm.getFormat() < 6)
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

osg::Image* ESM::FogState::initFogOfWar()
{
    osg::Image* fogOfWarImage = new osg::Image;
    // Assign a PixelBufferObject for asynchronous transfer of data to the GPU
    fogOfWarImage->setPixelBufferObject(new osg::PixelBufferObject);
    fogOfWarImage->allocateImage(FogTexture::sFogOfWarResolution, FogTexture::sFogOfWarResolution, 1, GL_ALPHA, GL_UNSIGNED_BYTE);
    assert(fogOfWarImage->isDataContiguous());
    std::fill(fogOfWarImage->data(), fogOfWarImage->data() + FogTexture::sFogOfWarResolution*FogTexture::sFogOfWarResolution, 0xff);

    return fogOfWarImage;
}

void ESM::FogState::saveFogOfWar(osg::Image* fogImage, ESM::FogTexture &fog)
{
    if (!fogImage)
        return;

    assert (fogImage->isDataContiguous());
    fog.mImageData.assign(fogImage->data(), fogImage->data() + FogTexture::sFogOfWarResolution*FogTexture::sFogOfWarResolution);
}

osg::Image* ESM::FogState::loadFogOfWar(const ESM::FogTexture &esm)
{
    osg::Image* fogImage = initFogOfWar();
    const std::vector<char>& data = esm.mImageData;
    if (data.empty())
    {
        return fogImage;
    }

    std::copy(data.begin(), data.end(), fogImage->data());
    fogImage->dirty();

    return fogImage;
}

void ESM::FogState::convertFogOfWar(std::vector<char>& imageData)
{
    if (imageData.empty())
    {
        return;
    }

    osgDB::ReaderWriter* readerwriter = osgDB::Registry::instance()->getReaderWriterForExtension("tga");
    if (!readerwriter)
    {
        Log(Debug::Error) << "Error: Unable to load fog, can't find a TGA ReaderWriter";
        return;
    }

    Files::IMemStream in(&imageData[0], imageData.size());

    osgDB::ReaderWriter::ReadResult result = readerwriter->readImage(in);
    if (!result.success())
    {
        Log(Debug::Error) << "Error: Failed to read fog: " << result.message() << " code " << result.status();
        return;
    }
    std::ostringstream ostream;

    osg::Image* resultImage = result.getImage();
    resultImage->flipVertical();

    unsigned char* oldFogData = resultImage->data();
    for (int i=0;i<FogTexture::sFogOfWarResolution*FogTexture::sFogOfWarResolution;++i)
        ostream << oldFogData[4*i+3];

    std::string str = ostream.str();
    imageData = std::vector<char>(str.begin(), str.end());
}
