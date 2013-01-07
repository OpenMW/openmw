#include "constrainedfiledatastream.hpp"
#include "lowlevelfile.hpp"

#include <stdexcept>
#include <cassert>
#ifndef __clang__
#include <cstdint>
#else
#include <tr1/cstdint>
#endif

namespace {

class ConstrainedDataStream : public Ogre::DataStream {
public:

	static const size_t sBufferSize = 4096; // somewhat arbitrary though 64KB buffers didn't seem to improve performance any
	static const size_t sBufferThreshold = 1024; // reads larger than this bypass buffering as cost of memcpy outweighs cost of system call

    ConstrainedDataStream(const Ogre::String &fname, size_t start, size_t length)
	{
		mFile.open (fname.c_str ());
		mSize  = length != 0xFFFFFFFF ? length : mFile.size () - start;

		mPos    = 0;
		mOrigin = start;
		mExtent = start + mSize;

		mBufferOrigin = 0;
		mBufferExtent = 0;
	}
		

	size_t read(void* buf, size_t count)
	{
		assert (mPos <= mSize);

		uint8_t * out = reinterpret_cast <uint8_t *> (buf);

		size_t posBeg = mOrigin + mPos;
		size_t posEnd = posBeg + count;

		if (posEnd > mExtent)
			posEnd = mExtent;

		size_t posCur = posBeg;

		while (posCur != posEnd)
		{
			size_t readLeft = posEnd - posCur;

			if (posCur < mBufferOrigin || posCur >= mBufferExtent)
			{
				if (readLeft >= sBufferThreshold || (posCur == mOrigin && posEnd == mExtent))
				{
					assert (mFile.tell () == mBufferExtent);

					if (posCur != mBufferExtent)
						mFile.seek (posCur);

					posCur += mFile.read (out, readLeft);

					mBufferOrigin = mBufferExtent = posCur;

					mPos = posCur - mOrigin;

					return posCur - posBeg;
				}
				else
				{
					size_t newBufferOrigin;

					if ((posCur < mBufferOrigin) && (mBufferOrigin - posCur < sBufferSize))
						newBufferOrigin = std::max (mOrigin, mBufferOrigin > sBufferSize ? mBufferOrigin - sBufferSize : 0);
					else
						newBufferOrigin = posCur;

					fill (newBufferOrigin);
				}
			}

			size_t xfer = std::min (readLeft, mBufferExtent - posCur);

			memcpy (out, mBuffer + (posCur - mBufferOrigin), xfer);

			posCur += xfer;
			out += xfer;
		}

		count = posEnd - posBeg;
		mPos += count;
		return count;
	}

    void skip(long count)
    {
		assert (mPos <= mSize);

        if((count >= 0 && (size_t)count <= mSize-mPos) ||
           (count < 0 && (size_t)-count <= mPos))
			mPos += count;
    }

    void seek(size_t pos)
    {
		assert (mPos <= mSize);

        if (pos < mSize)
			mPos = pos;
    }

    virtual size_t tell() const
    {
		assert (mPos <= mSize);

		return mPos;
	}

    virtual bool eof() const
    {
		assert (mPos <= mSize);

		return mPos == mSize;
	}

    virtual void close()
    {
		mFile.close();
	}

private:

	void fill (size_t newOrigin)
	{
		assert (mFile.tell () == mBufferExtent);

		size_t newExtent = newOrigin + sBufferSize;

		if (newExtent > mExtent)
			newExtent = mExtent;

		size_t oldExtent = mBufferExtent;

		if (newOrigin != oldExtent)
			mFile.seek (newOrigin);

		mBufferOrigin = mBufferExtent = newOrigin;

		size_t amountRequested = newExtent - newOrigin;

		size_t amountRead = mFile.read (mBuffer, amountRequested);

		if (amountRead != amountRequested)
			throw std::runtime_error ("An unexpected condition occurred while reading from a file.");

		mBufferExtent = newExtent;
	}

	LowLevelFile mFile;

	size_t mOrigin;
	size_t mExtent;
	size_t mPos;

	uint8_t mBuffer [sBufferSize];
	size_t mBufferOrigin;
	size_t mBufferExtent;
};

} // end of unnamed namespace

Ogre::DataStreamPtr openConstrainedFileDataStream (char const * filename, size_t offset, size_t length)
{
	return Ogre::DataStreamPtr(new ConstrainedDataStream(filename, offset, length));
}
