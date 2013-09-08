#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <fstream>

#include "components/file_finder/search.hpp"

struct SearchTest : public ::testing::Test
{
  protected:
    SearchTest()
      : mTestDir("./search_test_dir/")
    {
    }

    virtual void SetUp()
    {
      boost::filesystem::create_directory(boost::filesystem::path(mTestDir));

      std::ofstream ofs(std::string(mTestDir + "test2.txt").c_str(), std::ofstream::out);
      ofs << std::endl;
      ofs.close();
    }

    virtual void TearDown()
    {
      boost::filesystem::remove_all(boost::filesystem::path(mTestDir));
    }

    std::string mTestDir;
};

TEST_F(SearchTest, file_not_found)
{
  struct Result : public FileFinder::ReturnPath
  {
    Result(const boost::filesystem::path& expectedPath)
      : mExpectedPath(expectedPath)
    {
    }

    void add(const boost::filesystem::path& p)
    {
      ASSERT_FALSE(p == mExpectedPath);
    }

    private:
      boost::filesystem::path mExpectedPath;

  } r(boost::filesystem::path(mTestDir  + "test.txt"));

  FileFinder::find(mTestDir, r, false);
}

TEST_F(SearchTest, file_found)
{
  struct Result : public FileFinder::ReturnPath
  {
    Result(const boost::filesystem::path& expectedPath)
      : mExpectedPath(expectedPath)
    {
    }

    void add(const boost::filesystem::path& p)
    {
      ASSERT_TRUE(p == mExpectedPath);
    }

    private:
      boost::filesystem::path mExpectedPath;

  } r(boost::filesystem::path(mTestDir  + "test2.txt"));

  FileFinder::find(mTestDir, r, false);
}
