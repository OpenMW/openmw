#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/esm3/loaddial.hpp>

#include <gtest/gtest.h>

namespace ESM
{
    namespace
    {
        TEST(Esm3CStringIdTest, dialNameShouldBeNullTerminated)
        {
            std::unique_ptr<std::istream> stream;

            {
                auto ostream = std::make_unique<std::stringstream>();

                ESMWriter writer;
                writer.setFormatVersion(DefaultFormatVersion);
                writer.save(*ostream);

                Dialogue record;
                record.blank();
                record.mStringId = "topic name";
                record.mId = RefId::stringRefId(record.mStringId);
                record.mType = Dialogue::Topic;
                writer.startRecord(Dialogue::sRecordId);
                record.save(writer);
                writer.endRecord(Dialogue::sRecordId);

                stream = std::move(ostream);
            }

            ESMReader reader;
            reader.open(std::move(stream), "stream");
            ASSERT_TRUE(reader.hasMoreRecs());
            ASSERT_EQ(reader.getRecName(), Dialogue::sRecordId);
            reader.getRecHeader();
            while (reader.hasMoreSubs())
            {
                reader.getSubName();
                if (reader.retSubName().toInt() == SREC_NAME)
                {
                    reader.getSubHeader();
                    auto size = reader.getSubSize();
                    std::string buffer(size, '1');
                    reader.getExact(buffer.data(), size);
                    ASSERT_EQ(buffer[size - 1], '\0');
                    return;
                }
                else
                    reader.skipHSub();
            }
            ASSERT_FALSE(true);
        }
    }
}
