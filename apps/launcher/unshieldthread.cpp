#include "unshieldthread.hpp"

#include <fstream>
#include <components/misc/stringops.hpp>
   
namespace bfs = boost::filesystem;

namespace 
{        
    static bool make_sure_directory_exists(bfs::path directory)
    {

        if(!bfs::exists(directory))
        {
            bfs::create_directories(directory);
        }

        return bfs::exists(directory);
    }

    void fill_path(bfs::path& path, const std::string& name)
    {
        size_t start = 0;
        
        size_t i;
        for(i = 0; i < name.length(); i++)
        {
            switch(name[i])
            {
                case '\\':
                    path /= name.substr(start, i-start);
                    start = i+1;
                    break;
            }
        }

        path /= name.substr(start, i-start);
    }

    std::string get_setting(const std::string& category, const std::string& setting, const std::string& inx)
    {
        size_t start = inx.find(category);
        start = inx.find(setting, start) + setting.length() + 3;

        size_t end = inx.find("!", start);

        return inx.substr(start, end-start);
    }

    std::string read_to_string(const bfs::path& path)
    {
        std::ifstream strstream(path.c_str(), std::ios::in | std::ios::binary);
        std::string str;

        strstream.seekg(0, std::ios::end);
        str.resize(strstream.tellg());
        strstream.seekg(0, std::ios::beg);
        strstream.read(&str[0], str.size());
        strstream.close();

        return str;
    }

    void add_setting(const std::string& category, const std::string& setting, const std::string& val, std::string& ini)
    {                                                                                       
        size_t loc;
        loc = ini.find("[" + category + "]");
        
        // If category is not found, create it 
        if(loc == std::string::npos)
        {
            loc = ini.size() + 2;
            ini += ("\r\n[" + category + "]\r\n");
        }
          
        loc += category.length() +2 +2;
        ini.insert(loc, setting + "=" + val + "\r\n");
    }

    void bloodmoon_fix_ini(std::string& ini, const bfs::path inxPath)
    {
        std::string inx = read_to_string(inxPath);

        // Remove this one setting (the only one actually changed by bloodmoon, as opposed to just adding new ones)
        size_t start = ini.find("[Weather Blight]");
        start = ini.find("Ambient Loop Sound ID", start);
        size_t end = ini.find("\r\n", start) +2;
        ini.erase(start, end-start);

        std::string category;
        std::string setting;

        category = "General";
        {
            setting = "Werewolf FOV"; add_setting(category, setting, get_setting(category, setting, inx), ini);
        }
        category = "Moons";
        {
            setting = "Script Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
        }
        category = "Weather";
        {
            setting = "Snow Ripples"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Snow Ripple Radius"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Snow Ripples Per Flake"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Snow Ripple Scale"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Snow Ripple Speed"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Snow Gravity Scale"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Snow High Kill"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Snow Low Kill"; add_setting(category, setting, get_setting(category, setting, inx), ini);
        }
        category = "Weather Blight";
        {
            setting = "Ambient Loop Sound ID"; add_setting(category, setting, get_setting(category, setting, inx), ini);
        }
        category = "Weather Snow";
        {
            setting = "Sky Sunrise Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Sky Day Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Sky Sunset Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Sky Night Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Fog Sunrise Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Fog Day Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Fog Sunset Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Fog Night Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Ambient Sunrise Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Ambient Day Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Ambient Sunset Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Ambient Night Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Sun Sunrise Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Sun Day Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Sun Sunset Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Sun Night Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Sun Disc Sunset Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Transition Delta"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Land Fog Day Depth"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Land Fog Night Depth"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Clouds Maximum Percent"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Wind Speed"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Cloud Speed"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Glare View"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Cloud Texture"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Ambient Loop Sound ID"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Snow Threshold"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Snow Diameter"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Snow Height Min"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Snow Height Max"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Snow Entrance Speed"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Max Snowflakes"; add_setting(category, setting, get_setting(category, setting, inx), ini);
        }
        category = "Weather Blizzard";
        {
            setting = "Sky Sunrise Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Sky Day Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Sky Sunset Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Sky Night Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Fog Sunrise Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Fog Day Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Fog Sunset Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Fog Night Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Ambient Sunrise Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Ambient Day Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Ambient Sunset Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Ambient Night Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Sun Sunrise Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Sun Day Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Sun Sunset Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Sun Night Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Sun Disc Sunset Color"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Transition Delta"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Land Fog Day Depth"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Land Fog Night Depth"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Clouds Maximum Percent"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Wind Speed"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Cloud Speed"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Glare View"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Cloud Texture"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Ambient Loop Sound ID"; add_setting(category, setting, get_setting(category, setting, inx), ini);
            setting = "Storm Threshold"; add_setting(category, setting, get_setting(category, setting, inx), ini);
        }
    }


    void fix_ini(const bfs::path& output_dir, bfs::path cdPath, bool tribunal, bool bloodmoon)
    {
        bfs::path ini_path = output_dir;
        ini_path /= "Morrowind.ini";

        std::string ini = read_to_string(ini_path.string());

        if(tribunal)
        {
            add_setting("Game Files", "GameFile1", "Tribunal.esm", ini);
            add_setting("Archives", "Archive 0", "Tribunal.bsa", ini);
        }
        if(bloodmoon)
        {
            bloodmoon_fix_ini(ini, cdPath / "setup.inx");
            add_setting("Game Files", "GameFile2", "Bloodmoon.esm", ini);
            add_setting("Archives", "Archive 1", "Bloodmoon.bsa", ini);
        }

        std::ofstream inistream(ini_path.c_str());
        inistream << ini;
        inistream.close();
    }

    void installToPath(const bfs::path& from, const bfs::path& to, bool copy = false)
    {
        make_sure_directory_exists(to);

        for ( bfs::directory_iterator end, dir(from); dir != end; ++dir )
        {
            if(bfs::is_directory(dir->path()))
                installToPath(dir->path(), to / dir->path().filename(), copy);
            else
            {
                if(copy)
                {
                    bfs::path dest = to / dir->path().filename();
                    if(bfs::exists(dest))
                        bfs::remove_all(dest);
                    bfs::copy_file(dir->path(), dest);
                }
                else
                    bfs::rename(dir->path(), to / dir->path().filename());
            }
        }
    }

    bfs::path findFile(const bfs::path& in, std::string filename, bool recursive = true)
    {
        if(recursive)
        {
            for ( bfs::recursive_directory_iterator end, dir(in); dir != end; ++dir )
            {
                if(Misc::StringUtils::lowerCase(dir->path().filename().string()) == filename)
                    return dir->path();
            }
        }
        else
        {
            for ( bfs::directory_iterator end, dir(in); dir != end; ++dir )
            {
                if(Misc::StringUtils::lowerCase(dir->path().filename().string()) == filename)
                    return dir->path();
            }
        }

        return "";
    }

    bool contains(const bfs::path& in, std::string filename)
    {
        for(bfs::directory_iterator end, dir(in); dir != end; ++dir)
        {
            if(Misc::StringUtils::lowerCase(dir->path().filename().string()) == filename)
                return true;
        }

        return false;
    }

    time_t getTime(const char* time)
    {
        struct tm tms;
        memset(&tms, 0, sizeof(struct tm));
        strptime(time, "%d %B %Y", &tms);
        return mktime(&tms);
    }
}

bool UnshieldThread::SetMorrowindPath(const std::string& path)
{
    mMorrowindPath = path;
    return true;
}

bool UnshieldThread::SetTribunalPath(const std::string& path)
{
    mTribunalPath = path;
    return true;
}

bool UnshieldThread::SetBloodmoonPath(const std::string& path)
{
    mBloodmoonPath = path;
    return true;
}

void UnshieldThread::SetOutputPath(const std::string& path)
{
    mOutputPath = path;
}

bool UnshieldThread::extract_file(Unshield* unshield, bfs::path output_dir, const char* prefix, int index)
{
    bool success;
    bfs::path dirname;
    bfs::path filename;
    int directory = unshield_file_directory(unshield, index);

    dirname = output_dir;

    if (prefix && prefix[0])
        dirname /= prefix;

    if (directory >= 0)
    {
        const char* tmp = unshield_directory_name(unshield, directory);
        if (tmp && tmp[0])
            fill_path(dirname, tmp);
    }

    make_sure_directory_exists(dirname);

    filename = dirname;
    filename /= unshield_file_name(unshield, index);
  
    emit signalGUI(QString("Extracting: ") + QString(filename.c_str()));
  
    success = unshield_file_save(unshield, index, filename.c_str());

    if (!success)
        bfs::remove(filename);

    return success;
}

void UnshieldThread::extract_cab(const bfs::path& cab, const bfs::path& output_dir, bool extract_ini)
{
    Unshield * unshield;
    unshield = unshield_open(cab.c_str());

    int i;
    for (i = 0; i < unshield_file_group_count(unshield); i++)
    {
        UnshieldFileGroup* file_group = unshield_file_group_get(unshield, i);

        for (size_t j = file_group->first_file; j <= file_group->last_file; j++)
        {
            if (unshield_file_is_valid(unshield, j)) 
                    extract_file(unshield, output_dir, file_group->name, j);
        }
    }
    unshield_close(unshield);
}


bool UnshieldThread::extract()
{
    bfs::path outputDataFilesDir = mOutputPath;
    outputDataFilesDir /= "Data Files";
    bfs::path extractPath = mOutputPath;
    extractPath /= "extract-temp";

    if(!mMorrowindDone && mMorrowindPath.string().length() > 0)
    {
        mMorrowindDone = true;

        bfs::path mwExtractPath = extractPath / "morrowind";
        extract_cab(mMorrowindPath, mwExtractPath, true);
        
        bfs::path dFilesDir = findFile(mwExtractPath, "morrowind.esm").parent_path();
        
        installToPath(dFilesDir, outputDataFilesDir); 
        
        // Videos are often kept uncompressed on the cd
        bfs::path videosPath = findFile(mMorrowindPath.parent_path(), "video", false);
        if(videosPath.string() != "")
        {
            emit signalGUI(QString("Installing Videos..."));
            installToPath(videosPath, outputDataFilesDir / "Video", true);
        }

        bfs::path cdDFiles = findFile(mMorrowindPath.parent_path(), "data files", false);
        if(cdDFiles.string() != "")
        {
            emit signalGUI(QString("Installing Uncompressed Data files from CD..."));
            installToPath(cdDFiles, outputDataFilesDir, true);
        }


        bfs::rename(findFile(mwExtractPath, "morrowind.ini"), outputDataFilesDir / "Morrowind.ini");

        mTribunalDone = contains(outputDataFilesDir, "tribunal.esm");
        mBloodmoonDone = contains(outputDataFilesDir, "bloodmoon.esm");

    }

    else if(!mTribunalDone && mTribunalPath.string().length() > 0)
    {
        mTribunalDone = true;

        bfs::path tbExtractPath = extractPath / "tribunal";
        extract_cab(mTribunalPath, tbExtractPath, true);
        
        bfs::path dFilesDir = findFile(tbExtractPath, "tribunal.esm").parent_path();
        
        installToPath(dFilesDir, outputDataFilesDir); 
        
        // Mt GOTY CD has Sounds in a seperate folder from the rest of the data files
        bfs::path soundsPath = findFile(tbExtractPath, "sounds", false);
        if(soundsPath.string() != "")
            installToPath(soundsPath, outputDataFilesDir / "Sounds");

        bfs::path cdDFiles = findFile(mTribunalPath.parent_path(), "data files", false);
        if(cdDFiles.string() != "")
        {
            emit signalGUI(QString("Installing Uncompressed Data files from CD..."));
            installToPath(cdDFiles, outputDataFilesDir, true);
        }

        mBloodmoonDone = contains(outputDataFilesDir, "bloodmoon.esm");

        fix_ini(outputDataFilesDir, bfs::path(mTribunalPath).parent_path(), mTribunalDone, mBloodmoonDone);
    }

    else if(!mBloodmoonDone && mBloodmoonPath.string().length() > 0)
    {
        mBloodmoonDone = true;

        bfs::path bmExtractPath = extractPath / "bloodmoon";
        extract_cab(mBloodmoonPath, bmExtractPath, true);

        bfs::path dFilesDir = findFile(bmExtractPath, "bloodmoon.esm").parent_path();
        
        installToPath(dFilesDir, outputDataFilesDir); 

        // My GOTY CD contains a folder within cab files called Tribunal patch,
        // which contains Tribunal.esm
        bfs::path tbPatchPath = findFile(bmExtractPath, "tribunal.esm");
        if(tbPatchPath.string() != "")
            bfs::rename(tbPatchPath, outputDataFilesDir / "Tribunal.esm");

        bfs::path cdDFiles = findFile(mBloodmoonPath.parent_path(), "data files", false);
        if(cdDFiles.string() != "")
        {
            emit signalGUI(QString("Installing Uncompressed Data files from CD..."));
            installToPath(cdDFiles, outputDataFilesDir, true);
        }
        
        fix_ini(outputDataFilesDir, bfs::path(mBloodmoonPath).parent_path(), false, mBloodmoonDone);
    }

 
    return true;
}

void UnshieldThread::Done()
{
    // Get rid of unnecessary files
    bfs::remove_all(mOutputPath / "extract-temp");

    // Set modified time to release dates, to preserve load order
    if(mMorrowindDone)
        bfs::last_write_time(findFile(mOutputPath, "morrowind.esm"), getTime("1 May 2002"));

    if(mTribunalDone)
        bfs::last_write_time(findFile(mOutputPath, "tribunal.esm"), getTime("6 November 2002"));

    if(mBloodmoonDone)
        bfs::last_write_time(findFile(mOutputPath, "bloodmoon.esm"), getTime("3 June 2003"));
}

std::string UnshieldThread::GetMWEsmPath()
{
    return findFile(mOutputPath / "Data Files", "morrowind.esm").string();
}

bool UnshieldThread::TribunalDone()
{
    return mTribunalDone;
}

bool UnshieldThread::BloodmoonDone()
{
    return mBloodmoonDone;
}

void UnshieldThread::run()
{
    extract();
    emit close();
}

UnshieldThread::UnshieldThread()
{
    unshield_set_log_level(0);
    mMorrowindDone = false;
    mTribunalDone = false;
    mBloodmoonDone = false;
}
