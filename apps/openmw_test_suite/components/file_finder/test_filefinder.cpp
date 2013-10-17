#include <gtest/gtest.h>
#include <fstream>
#include "components/file_finder/file_finder.hpp"

struct FileFinderTest : public ::testing::Test
{
  protected:
    FileFinderTest()
      : mTestDir("./filefinder_test_dir/")
      , mTestFile("test.txt")
      , mTestFileUppercase("TEST.TXT")
      , mTestFileNotFound("foobarbaz.txt")
    {
    }

    virtual void SetUp()
    {
      boost::filesystem::create_directory(boost::filesystem::path(mTestDir));

      std::ofstream ofs(std::string(mTestDir + mTestFile).c_str(), std::ofstream::out);
      ofs << std::endl;
      ofs.close();
    }

    virtual void TearDown()
    {
      boost::filesystem::remove_all(boost::filesystem::path(mTestDir));
    }

    std::string mTestDir;
    std::string mTestFile;
    std::string mTestFileUppercase;
    std::string mTestFileNotFound;
};

TEST_F(FileFinderTest, FileFinder_has_file)
{
  FileFinder::FileFinder fileFinder(mTestDir);
  ASSERT_TRUE(fileFinder.has(mTestFile));
  ASSERT_TRUE(fileFinder.has(mTestFileUppercase));
  ASSERT_TRUE(fileFinder.lookup(mTestFile) == std::string(mTestDir + mTestFile));
  ASSERT_TRUE(fileFinder.lookup(mTestFileUppercase) == std::string(mTestDir + mTestFile));
}

TEST_F(FileFinderTest, FileFinder_does_not_have_file)
{
  FileFinder::FileFinder fileFinder(mTestDir);
  ASSERT_FALSE(fileFinder.has(mTestFileNotFound));
  ASSERT_TRUE(fileFinder.lookup(mTestFileNotFound).empty());
}

TEST_F(FileFinderTest, FileFinderStrict_has_file)
{
  FileFinder::FileFinderStrict fileFinder(mTestDir);
  ASSERT_TRUE(fileFinder.has(mTestFile));
  ASSERT_FALSE(fileFinder.has(mTestFileUppercase));
  ASSERT_TRUE(fileFinder.lookup(mTestFile) == std::string(mTestDir + mTestFile));
  ASSERT_FALSE(fileFinder.lookup(mTestFileUppercase) == std::string(mTestDir + mTestFile));
}

TEST_F(FileFinderTest, FileFinderStrict_does_not_have_file)
{
  FileFinder::FileFinderStrict fileFinder(mTestDir);
  ASSERT_FALSE(fileFinder.has(mTestFileNotFound));
  ASSERT_TRUE(fileFinder.lookup(mTestFileNotFound).empty());
}
