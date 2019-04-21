#include "importer.hpp"

#include <iostream>
#include <sstream>
#include <components/misc/stringops.hpp>
#include <components/esm/esmreader.hpp>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/version.hpp>

namespace bfs = boost::filesystem;

MwIniImporter::MwIniImporter()
    : mVerbose(false)
    , mEncoding(ToUTF8::WINDOWS_1250)
{
    const char *map[][2] =
    {
        { "no-sound", "General:Disable Audio" },
        { 0, 0 }
    };
    const char *fallback[] = {

        // light
        "LightAttenuation:UseConstant",
        "LightAttenuation:ConstantValue",
        "LightAttenuation:UseLinear",
        "LightAttenuation:LinearMethod",
        "LightAttenuation:LinearValue",
        "LightAttenuation:LinearRadiusMult",
        "LightAttenuation:UseQuadratic",
        "LightAttenuation:QuadraticMethod",
        "LightAttenuation:QuadraticValue",
        "LightAttenuation:QuadraticRadiusMult",
        "LightAttenuation:OutQuadInLin",

        // inventory
        "Inventory:DirectionalDiffuseR",
        "Inventory:DirectionalDiffuseG",
        "Inventory:DirectionalDiffuseB",
        "Inventory:DirectionalAmbientR",
        "Inventory:DirectionalAmbientG",
        "Inventory:DirectionalAmbientB",
        "Inventory:DirectionalRotationX",
        "Inventory:DirectionalRotationY",
        "Inventory:UniformScaling",

        // map
        "Map:Travel Siltstrider Red",
        "Map:Travel Siltstrider Green",
        "Map:Travel Siltstrider Blue",
        "Map:Travel Boat Red",
        "Map:Travel Boat Green",
        "Map:Travel Boat Blue",
        "Map:Travel Magic Red",
        "Map:Travel Magic Green",
        "Map:Travel Magic Blue",
        "Map:Show Travel Lines",

        // water
        "Water:Map Alpha",
        "Water:World Alpha",
        "Water:SurfaceTextureSize",
        "Water:SurfaceTileCount",
        "Water:SurfaceFPS",
        "Water:SurfaceTexture",
        "Water:SurfaceFrameCount",
        "Water:TileTextureDivisor",
        "Water:RippleTexture",
        "Water:RippleFrameCount",
        "Water:RippleLifetime",
        "Water:MaxNumberRipples",
        "Water:RippleScale",
        "Water:RippleRotSpeed",
        "Water:RippleAlphas",
        "Water:PSWaterReflectTerrain",
        "Water:PSWaterReflectUpdate",
        "Water:NearWaterRadius",
        "Water:NearWaterPoints",
        "Water:NearWaterUnderwaterFreq",
        "Water:NearWaterUnderwaterVolume",
        "Water:NearWaterIndoorTolerance",
        "Water:NearWaterOutdoorTolerance",
        "Water:NearWaterIndoorID",
        "Water:NearWaterOutdoorID",
        "Water:UnderwaterSunriseFog",
        "Water:UnderwaterDayFog",
        "Water:UnderwaterSunsetFog",
        "Water:UnderwaterNightFog",
        "Water:UnderwaterIndoorFog",
        "Water:UnderwaterColor",
        "Water:UnderwaterColorWeight",

        // pixelwater
        "PixelWater:SurfaceFPS",
        "PixelWater:TileCount",
        "PixelWater:Resolution",

        // fonts
        "Fonts:Font 0",
        "Fonts:Font 1",
        "Fonts:Font 2",

        // UI colors
        "FontColor:color_normal",
        "FontColor:color_normal_over",
        "FontColor:color_normal_pressed",
        "FontColor:color_active",
        "FontColor:color_active_over",
        "FontColor:color_active_pressed",
        "FontColor:color_disabled",
        "FontColor:color_disabled_over",
        "FontColor:color_disabled_pressed",
        "FontColor:color_link",
        "FontColor:color_link_over",
        "FontColor:color_link_pressed",
        "FontColor:color_journal_link",
        "FontColor:color_journal_link_over",
        "FontColor:color_journal_link_pressed",
        "FontColor:color_journal_topic",
        "FontColor:color_journal_topic_over",
        "FontColor:color_journal_topic_pressed",
        "FontColor:color_answer",
        "FontColor:color_answer_over",
        "FontColor:color_answer_pressed",
        "FontColor:color_header",
        "FontColor:color_notify",
        "FontColor:color_big_normal",
        "FontColor:color_big_normal_over",
        "FontColor:color_big_normal_pressed",
        "FontColor:color_big_link",
        "FontColor:color_big_link_over",
        "FontColor:color_big_link_pressed",
        "FontColor:color_big_answer",
        "FontColor:color_big_answer_over",
        "FontColor:color_big_answer_pressed",
        "FontColor:color_big_header",
        "FontColor:color_big_notify",
        "FontColor:color_background",
        "FontColor:color_focus",
        "FontColor:color_health",
        "FontColor:color_magic",
        "FontColor:color_fatigue",
        "FontColor:color_misc",
        "FontColor:color_weapon_fill",
        "FontColor:color_magic_fill",
        "FontColor:color_positive",
        "FontColor:color_negative",
        "FontColor:color_count",

        // level up messages
        "Level Up:Level2",
        "Level Up:Level3",
        "Level Up:Level4",
        "Level Up:Level5",
        "Level Up:Level6",
        "Level Up:Level7",
        "Level Up:Level8",
        "Level Up:Level9",
        "Level Up:Level10",
        "Level Up:Level11",
        "Level Up:Level12",
        "Level Up:Level13",
        "Level Up:Level14",
        "Level Up:Level15",
        "Level Up:Level16",
        "Level Up:Level17",
        "Level Up:Level18",
        "Level Up:Level19",
        "Level Up:Level20",
        "Level Up:Default",

        // character creation multiple choice test
        "Question 1:Question",
        "Question 1:AnswerOne",
        "Question 1:AnswerTwo",
        "Question 1:AnswerThree",
        "Question 1:Sound",
        "Question 2:Question",
        "Question 2:AnswerOne",
        "Question 2:AnswerTwo",
        "Question 2:AnswerThree",
        "Question 2:Sound",
        "Question 3:Question",
        "Question 3:AnswerOne",
        "Question 3:AnswerTwo",
        "Question 3:AnswerThree",
        "Question 3:Sound",
        "Question 4:Question",
        "Question 4:AnswerOne",
        "Question 4:AnswerTwo",
        "Question 4:AnswerThree",
        "Question 4:Sound",
        "Question 5:Question",
        "Question 5:AnswerOne",
        "Question 5:AnswerTwo",
        "Question 5:AnswerThree",
        "Question 5:Sound",
        "Question 6:Question",
        "Question 6:AnswerOne",
        "Question 6:AnswerTwo",
        "Question 6:AnswerThree",
        "Question 6:Sound",
        "Question 7:Question",
        "Question 7:AnswerOne",
        "Question 7:AnswerTwo",
        "Question 7:AnswerThree",
        "Question 7:Sound",
        "Question 8:Question",
        "Question 8:AnswerOne",
        "Question 8:AnswerTwo",
        "Question 8:AnswerThree",
        "Question 8:Sound",
        "Question 9:Question",
        "Question 9:AnswerOne",
        "Question 9:AnswerTwo",
        "Question 9:AnswerThree",
        "Question 9:Sound",
        "Question 10:Question",
        "Question 10:AnswerOne",
        "Question 10:AnswerTwo",
        "Question 10:AnswerThree",
        "Question 10:Sound",

        // blood textures and models
        "Blood:Model 0",
        "Blood:Model 1",
        "Blood:Model 2",
        "Blood:Texture 0",
        "Blood:Texture 1",
        "Blood:Texture 2",
        "Blood:Texture 3",
        "Blood:Texture 4",
        "Blood:Texture 5",
        "Blood:Texture 6",
        "Blood:Texture 7",
        "Blood:Texture Name 0",
        "Blood:Texture Name 1",
        "Blood:Texture Name 2",
        "Blood:Texture Name 3",
        "Blood:Texture Name 4",
        "Blood:Texture Name 5",
        "Blood:Texture Name 6",
        "Blood:Texture Name 7",

        // movies
        "Movies:Company Logo",
        "Movies:Morrowind Logo",
        "Movies:New Game",
        "Movies:Loading",
        "Movies:Options Menu",

        // weather related values

        "Weather Thunderstorm:Thunder Sound ID 0",
        "Weather Thunderstorm:Thunder Sound ID 1",
        "Weather Thunderstorm:Thunder Sound ID 2",
        "Weather Thunderstorm:Thunder Sound ID 3",
        "Weather:Sunrise Time",
        "Weather:Sunset Time",
        "Weather:Sunrise Duration",
        "Weather:Sunset Duration",
        "Weather:Hours Between Weather Changes", // AKA weather update time
        "Weather Thunderstorm:Thunder Frequency",
        "Weather Thunderstorm:Thunder Threshold",

        "Weather:EnvReduceColor",
        "Weather:LerpCloseColor",
        "Weather:BumpFadeColor",
        "Weather:AlphaReduce",
        "Weather:Minimum Time Between Environmental Sounds",
        "Weather:Maximum Time Between Environmental Sounds",
        "Weather:Sun Glare Fader Max",
        "Weather:Sun Glare Fader Angle Max",
        "Weather:Sun Glare Fader Color",
        "Weather:Timescale Clouds",
        "Weather:Precip Gravity",
        "Weather:Rain Ripples",
        "Weather:Rain Ripple Radius",
        "Weather:Rain Ripples Per Drop",
        "Weather:Rain Ripple Scale",
        "Weather:Rain Ripple Speed",
        "Weather:Fog Depth Change Speed",
        "Weather:Sky Pre-Sunrise Time",
        "Weather:Sky Post-Sunrise Time",
        "Weather:Sky Pre-Sunset Time",
        "Weather:Sky Post-Sunset Time",
        "Weather:Ambient Pre-Sunrise Time",
        "Weather:Ambient Post-Sunrise Time",
        "Weather:Ambient Pre-Sunset Time",
        "Weather:Ambient Post-Sunset Time",
        "Weather:Fog Pre-Sunrise Time",
        "Weather:Fog Post-Sunrise Time",
        "Weather:Fog Pre-Sunset Time",
        "Weather:Fog Post-Sunset Time",
        "Weather:Sun Pre-Sunrise Time",
        "Weather:Sun Post-Sunrise Time",
        "Weather:Sun Pre-Sunset Time",
        "Weather:Sun Post-Sunset Time",
        "Weather:Stars Post-Sunset Start",
        "Weather:Stars Pre-Sunrise Finish",
        "Weather:Stars Fading Duration",
        "Weather:Snow Ripples",
        "Weather:Snow Ripple Radius",
        "Weather:Snow Ripples Per Flake",
        "Weather:Snow Ripple Scale",
        "Weather:Snow Ripple Speed",
        "Weather:Snow Gravity Scale",
        "Weather:Snow High Kill",
        "Weather:Snow Low Kill",

        "Weather Clear:Cloud Texture",
        "Weather Clear:Clouds Maximum Percent",
        "Weather Clear:Transition Delta",
        "Weather Clear:Sky Sunrise Color",
        "Weather Clear:Sky Day Color",
        "Weather Clear:Sky Sunset Color",
        "Weather Clear:Sky Night Color",
        "Weather Clear:Fog Sunrise Color",
        "Weather Clear:Fog Day Color",
        "Weather Clear:Fog Sunset Color",
        "Weather Clear:Fog Night Color",
        "Weather Clear:Ambient Sunrise Color",
        "Weather Clear:Ambient Day Color",
        "Weather Clear:Ambient Sunset Color",
        "Weather Clear:Ambient Night Color",
        "Weather Clear:Sun Sunrise Color",
        "Weather Clear:Sun Day Color",
        "Weather Clear:Sun Sunset Color",
        "Weather Clear:Sun Night Color",
        "Weather Clear:Sun Disc Sunset Color",
        "Weather Clear:Land Fog Day Depth",
        "Weather Clear:Land Fog Night Depth",
        "Weather Clear:Wind Speed",
        "Weather Clear:Cloud Speed",
        "Weather Clear:Glare View",
        "Weather Clear:Ambient Loop Sound ID",

        "Weather Cloudy:Cloud Texture",
        "Weather Cloudy:Clouds Maximum Percent",
        "Weather Cloudy:Transition Delta",
        "Weather Cloudy:Sky Sunrise Color",
        "Weather Cloudy:Sky Day Color",
        "Weather Cloudy:Sky Sunset Color",
        "Weather Cloudy:Sky Night Color",
        "Weather Cloudy:Fog Sunrise Color",
        "Weather Cloudy:Fog Day Color",
        "Weather Cloudy:Fog Sunset Color",
        "Weather Cloudy:Fog Night Color",
        "Weather Cloudy:Ambient Sunrise Color",
        "Weather Cloudy:Ambient Day Color",
        "Weather Cloudy:Ambient Sunset Color",
        "Weather Cloudy:Ambient Night Color",
        "Weather Cloudy:Sun Sunrise Color",
        "Weather Cloudy:Sun Day Color",
        "Weather Cloudy:Sun Sunset Color",
        "Weather Cloudy:Sun Night Color",
        "Weather Cloudy:Sun Disc Sunset Color",
        "Weather Cloudy:Land Fog Day Depth",
        "Weather Cloudy:Land Fog Night Depth",
        "Weather Cloudy:Wind Speed",
        "Weather Cloudy:Cloud Speed",
        "Weather Cloudy:Glare View",
        "Weather Cloudy:Ambient Loop Sound ID",

        "Weather Foggy:Cloud Texture",
        "Weather Foggy:Clouds Maximum Percent",
        "Weather Foggy:Transition Delta",
        "Weather Foggy:Sky Sunrise Color",
        "Weather Foggy:Sky Day Color",
        "Weather Foggy:Sky Sunset Color",
        "Weather Foggy:Sky Night Color",
        "Weather Foggy:Fog Sunrise Color",
        "Weather Foggy:Fog Day Color",
        "Weather Foggy:Fog Sunset Color",
        "Weather Foggy:Fog Night Color",
        "Weather Foggy:Ambient Sunrise Color",
        "Weather Foggy:Ambient Day Color",
        "Weather Foggy:Ambient Sunset Color",
        "Weather Foggy:Ambient Night Color",
        "Weather Foggy:Sun Sunrise Color",
        "Weather Foggy:Sun Day Color",
        "Weather Foggy:Sun Sunset Color",
        "Weather Foggy:Sun Night Color",
        "Weather Foggy:Sun Disc Sunset Color",
        "Weather Foggy:Land Fog Day Depth",
        "Weather Foggy:Land Fog Night Depth",
        "Weather Foggy:Wind Speed",
        "Weather Foggy:Cloud Speed",
        "Weather Foggy:Glare View",
        "Weather Foggy:Ambient Loop Sound ID",

        "Weather Thunderstorm:Cloud Texture",
        "Weather Thunderstorm:Clouds Maximum Percent",
        "Weather Thunderstorm:Transition Delta",
        "Weather Thunderstorm:Sky Sunrise Color",
        "Weather Thunderstorm:Sky Day Color",
        "Weather Thunderstorm:Sky Sunset Color",
        "Weather Thunderstorm:Sky Night Color",
        "Weather Thunderstorm:Fog Sunrise Color",
        "Weather Thunderstorm:Fog Day Color",
        "Weather Thunderstorm:Fog Sunset Color",
        "Weather Thunderstorm:Fog Night Color",
        "Weather Thunderstorm:Ambient Sunrise Color",
        "Weather Thunderstorm:Ambient Day Color",
        "Weather Thunderstorm:Ambient Sunset Color",
        "Weather Thunderstorm:Ambient Night Color",
        "Weather Thunderstorm:Sun Sunrise Color",
        "Weather Thunderstorm:Sun Day Color",
        "Weather Thunderstorm:Sun Sunset Color",
        "Weather Thunderstorm:Sun Night Color",
        "Weather Thunderstorm:Sun Disc Sunset Color",
        "Weather Thunderstorm:Land Fog Day Depth",
        "Weather Thunderstorm:Land Fog Night Depth",
        "Weather Thunderstorm:Wind Speed",
        "Weather Thunderstorm:Cloud Speed",
        "Weather Thunderstorm:Glare View",
        "Weather Thunderstorm:Rain Loop Sound ID",
        "Weather Thunderstorm:Using Precip",
        "Weather Thunderstorm:Rain Diameter",
        "Weather Thunderstorm:Rain Height Min",
        "Weather Thunderstorm:Rain Height Max",
        "Weather Thunderstorm:Rain Threshold",
        "Weather Thunderstorm:Max Raindrops",
        "Weather Thunderstorm:Rain Entrance Speed",
        "Weather Thunderstorm:Ambient Loop Sound ID",
        "Weather Thunderstorm:Flash Decrement",

        "Weather Rain:Cloud Texture",
        "Weather Rain:Clouds Maximum Percent",
        "Weather Rain:Transition Delta",
        "Weather Rain:Sky Sunrise Color",
        "Weather Rain:Sky Day Color",
        "Weather Rain:Sky Sunset Color",
        "Weather Rain:Sky Night Color",
        "Weather Rain:Fog Sunrise Color",
        "Weather Rain:Fog Day Color",
        "Weather Rain:Fog Sunset Color",
        "Weather Rain:Fog Night Color",
        "Weather Rain:Ambient Sunrise Color",
        "Weather Rain:Ambient Day Color",
        "Weather Rain:Ambient Sunset Color",
        "Weather Rain:Ambient Night Color",
        "Weather Rain:Sun Sunrise Color",
        "Weather Rain:Sun Day Color",
        "Weather Rain:Sun Sunset Color",
        "Weather Rain:Sun Night Color",
        "Weather Rain:Sun Disc Sunset Color",
        "Weather Rain:Land Fog Day Depth",
        "Weather Rain:Land Fog Night Depth",
        "Weather Rain:Wind Speed",
        "Weather Rain:Cloud Speed",
        "Weather Rain:Glare View",
        "Weather Rain:Rain Loop Sound ID",
        "Weather Rain:Using Precip",
        "Weather Rain:Rain Diameter",
        "Weather Rain:Rain Height Min",
        "Weather Rain:Rain Height Max",
        "Weather Rain:Rain Threshold",
        "Weather Rain:Rain Entrance Speed",
        "Weather Rain:Ambient Loop Sound ID",
        "Weather Rain:Max Raindrops",

        "Weather Overcast:Cloud Texture",
        "Weather Overcast:Clouds Maximum Percent",
        "Weather Overcast:Transition Delta",
        "Weather Overcast:Sky Sunrise Color",
        "Weather Overcast:Sky Day Color",
        "Weather Overcast:Sky Sunset Color",
        "Weather Overcast:Sky Night Color",
        "Weather Overcast:Fog Sunrise Color",
        "Weather Overcast:Fog Day Color",
        "Weather Overcast:Fog Sunset Color",
        "Weather Overcast:Fog Night Color",
        "Weather Overcast:Ambient Sunrise Color",
        "Weather Overcast:Ambient Day Color",
        "Weather Overcast:Ambient Sunset Color",
        "Weather Overcast:Ambient Night Color",
        "Weather Overcast:Sun Sunrise Color",
        "Weather Overcast:Sun Day Color",
        "Weather Overcast:Sun Sunset Color",
        "Weather Overcast:Sun Night Color",
        "Weather Overcast:Sun Disc Sunset Color",
        "Weather Overcast:Land Fog Day Depth",
        "Weather Overcast:Land Fog Night Depth",
        "Weather Overcast:Wind Speed",
        "Weather Overcast:Cloud Speed",
        "Weather Overcast:Glare View",
        "Weather Overcast:Ambient Loop Sound ID",

        "Weather Ashstorm:Cloud Texture",
        "Weather Ashstorm:Clouds Maximum Percent",
        "Weather Ashstorm:Transition Delta",
        "Weather Ashstorm:Sky Sunrise Color",
        "Weather Ashstorm:Sky Day Color",
        "Weather Ashstorm:Sky Sunset Color",
        "Weather Ashstorm:Sky Night Color",
        "Weather Ashstorm:Fog Sunrise Color",
        "Weather Ashstorm:Fog Day Color",
        "Weather Ashstorm:Fog Sunset Color",
        "Weather Ashstorm:Fog Night Color",
        "Weather Ashstorm:Ambient Sunrise Color",
        "Weather Ashstorm:Ambient Day Color",
        "Weather Ashstorm:Ambient Sunset Color",
        "Weather Ashstorm:Ambient Night Color",
        "Weather Ashstorm:Sun Sunrise Color",
        "Weather Ashstorm:Sun Day Color",
        "Weather Ashstorm:Sun Sunset Color",
        "Weather Ashstorm:Sun Night Color",
        "Weather Ashstorm:Sun Disc Sunset Color",
        "Weather Ashstorm:Land Fog Day Depth",
        "Weather Ashstorm:Land Fog Night Depth",
        "Weather Ashstorm:Wind Speed",
        "Weather Ashstorm:Cloud Speed",
        "Weather Ashstorm:Glare View",
        "Weather Ashstorm:Ambient Loop Sound ID",
        "Weather Ashstorm:Storm Threshold",

        "Weather Blight:Cloud Texture",
        "Weather Blight:Clouds Maximum Percent",
        "Weather Blight:Transition Delta",
        "Weather Blight:Sky Sunrise Color",
        "Weather Blight:Sky Day Color",
        "Weather Blight:Sky Sunset Color",
        "Weather Blight:Sky Night Color",
        "Weather Blight:Fog Sunrise Color",
        "Weather Blight:Fog Day Color",
        "Weather Blight:Fog Sunset Color",
        "Weather Blight:Fog Night Color",
        "Weather Blight:Ambient Sunrise Color",
        "Weather Blight:Ambient Day Color",
        "Weather Blight:Ambient Sunset Color",
        "Weather Blight:Ambient Night Color",
        "Weather Blight:Sun Sunrise Color",
        "Weather Blight:Sun Day Color",
        "Weather Blight:Sun Sunset Color",
        "Weather Blight:Sun Night Color",
        "Weather Blight:Sun Disc Sunset Color",
        "Weather Blight:Land Fog Day Depth",
        "Weather Blight:Land Fog Night Depth",
        "Weather Blight:Wind Speed",
        "Weather Blight:Cloud Speed",
        "Weather Blight:Glare View",
        "Weather Blight:Ambient Loop Sound ID",
        "Weather Blight:Storm Threshold",
        "Weather Blight:Disease Chance",

        // for Bloodmoon
        "Weather Snow:Cloud Texture",
        "Weather Snow:Clouds Maximum Percent",
        "Weather Snow:Transition Delta",
        "Weather Snow:Sky Sunrise Color",
        "Weather Snow:Sky Day Color",
        "Weather Snow:Sky Sunset Color",
        "Weather Snow:Sky Night Color",
        "Weather Snow:Fog Sunrise Color",
        "Weather Snow:Fog Day Color",
        "Weather Snow:Fog Sunset Color",
        "Weather Snow:Fog Night Color",
        "Weather Snow:Ambient Sunrise Color",
        "Weather Snow:Ambient Day Color",
        "Weather Snow:Ambient Sunset Color",
        "Weather Snow:Ambient Night Color",
        "Weather Snow:Sun Sunrise Color",
        "Weather Snow:Sun Day Color",
        "Weather Snow:Sun Sunset Color",
        "Weather Snow:Sun Night Color",
        "Weather Snow:Sun Disc Sunset Color",
        "Weather Snow:Land Fog Day Depth",
        "Weather Snow:Land Fog Night Depth",
        "Weather Snow:Wind Speed",
        "Weather Snow:Cloud Speed",
        "Weather Snow:Glare View",
        "Weather Snow:Snow Diameter",
        "Weather Snow:Snow Height Min",
        "Weather Snow:Snow Height Max",
        "Weather Snow:Snow Entrance Speed",
        "Weather Snow:Max Snowflakes",
        "Weather Snow:Ambient Loop Sound ID",
        "Weather Snow:Snow Threshold",

        // for Bloodmoon
        "Weather Blizzard:Cloud Texture",
        "Weather Blizzard:Clouds Maximum Percent",
        "Weather Blizzard:Transition Delta",
        "Weather Blizzard:Sky Sunrise Color",
        "Weather Blizzard:Sky Day Color",
        "Weather Blizzard:Sky Sunset Color",
        "Weather Blizzard:Sky Night Color",
        "Weather Blizzard:Fog Sunrise Color",
        "Weather Blizzard:Fog Day Color",
        "Weather Blizzard:Fog Sunset Color",
        "Weather Blizzard:Fog Night Color",
        "Weather Blizzard:Ambient Sunrise Color",
        "Weather Blizzard:Ambient Day Color",
        "Weather Blizzard:Ambient Sunset Color",
        "Weather Blizzard:Ambient Night Color",
        "Weather Blizzard:Sun Sunrise Color",
        "Weather Blizzard:Sun Day Color",
        "Weather Blizzard:Sun Sunset Color",
        "Weather Blizzard:Sun Night Color",
        "Weather Blizzard:Sun Disc Sunset Color",
        "Weather Blizzard:Land Fog Day Depth",
        "Weather Blizzard:Land Fog Night Depth",
        "Weather Blizzard:Wind Speed",
        "Weather Blizzard:Cloud Speed",
        "Weather Blizzard:Glare View",
        "Weather Blizzard:Ambient Loop Sound ID",
        "Weather Blizzard:Storm Threshold",

        // moons
        "Moons:Secunda Size",
        "Moons:Secunda Axis Offset",
        "Moons:Secunda Speed",
        "Moons:Secunda Daily Increment",
        "Moons:Secunda Moon Shadow Early Fade Angle",
        "Moons:Secunda Fade Start Angle",
        "Moons:Secunda Fade End Angle",
        "Moons:Secunda Fade In Start",
        "Moons:Secunda Fade In Finish",
        "Moons:Secunda Fade Out Start",
        "Moons:Secunda Fade Out Finish",
        "Moons:Masser Size",
        "Moons:Masser Axis Offset",
        "Moons:Masser Speed",
        "Moons:Masser Daily Increment",
        "Moons:Masser Moon Shadow Early Fade Angle",
        "Moons:Masser Fade Start Angle",
        "Moons:Masser Fade End Angle",
        "Moons:Masser Fade In Start",
        "Moons:Masser Fade In Finish",
        "Moons:Masser Fade Out Start",
        "Moons:Masser Fade Out Finish",
        "Moons:Script Color",

        // werewolf (Bloodmoon)
        "General:Werewolf FOV",

        0
    };

    for(int i=0; map[i][0]; i++) {
        mMergeMap.insert(std::make_pair<std::string, std::string>(map[i][0], map[i][1]));
    }

    for(int i=0; fallback[i]; i++) {
        mMergeFallback.push_back(fallback[i]);
    }
}

void MwIniImporter::setVerbose(bool verbose) {
    mVerbose = verbose;
}

MwIniImporter::multistrmap MwIniImporter::loadIniFile(const boost::filesystem::path&  filename) const {
    std::cout << "load ini file: " << filename << std::endl;

    std::string section("");
    MwIniImporter::multistrmap map;
    bfs::ifstream file((bfs::path(filename)));
    ToUTF8::Utf8Encoder encoder(mEncoding);

    std::string line;
    while (std::getline(file, line)) {

        line = encoder.getUtf8(line);

        // unify Unix-style and Windows file ending
        if (!(line.empty()) && (line[line.length()-1]) == '\r') {
            line = line.substr(0, line.length()-1);
        }

        if(line.empty()) {
            continue;
        }

        if(line[0] == '[') {
            int pos = line.find(']');
            if(pos < 2) {
                std::cout << "Warning: ini file wrongly formatted (" << line << "). Line ignored." << std::endl;
                continue;
            }

            section = line.substr(1, line.find(']')-1);
            continue;
        }

        int comment_pos = line.find(";");
        if(comment_pos > 0) {
            line = line.substr(0,comment_pos);
        }

        int pos = line.find("=");
        if(pos < 1) {
            continue;
        }

        std::string key(section + ":" + line.substr(0,pos));
        std::string value(line.substr(pos+1));
        if(value.empty()) {
            std::cout << "Warning: ignored empty value for key '" << key << "'." << std::endl;
            continue;
        }

        if(map.find(key) == map.end()) {
            map.insert( std::make_pair (key, std::vector<std::string>() ) );
        }
        map[key].push_back(value);
    }

    return map;
}

MwIniImporter::multistrmap MwIniImporter::loadCfgFile(const boost::filesystem::path& filename) {
    std::cout << "load cfg file: " << filename << std::endl;

    MwIniImporter::multistrmap map;
    bfs::ifstream file((bfs::path(filename)));

    std::string line;
    while (std::getline(file, line)) {

        // we cant say comment by only looking at first char anymore
        int comment_pos = line.find("#");
        if(comment_pos > 0) {
            line = line.substr(0,comment_pos);
        }

        if(line.empty()) {
            continue;
        }

        int pos = line.find("=");
        if(pos < 1) {
            continue;
        }

        std::string key(line.substr(0,pos));
        std::string value(line.substr(pos+1));

        if(map.find(key) == map.end()) {
            map.insert( std::make_pair (key, std::vector<std::string>() ) );
        }
        map[key].push_back(value);
    }

    return map;
}

void MwIniImporter::merge(multistrmap &cfg, const multistrmap &ini) const {
    multistrmap::const_iterator iniIt;
    for(strmap::const_iterator it=mMergeMap.begin(); it!=mMergeMap.end(); ++it) {
        if((iniIt = ini.find(it->second)) != ini.end()) {
            for(std::vector<std::string>::const_iterator vc = iniIt->second.begin(); vc != iniIt->second.end(); ++vc) {
                cfg.erase(it->first);
                insertMultistrmap(cfg, it->first, *vc);
            }
        }
    }
}

void MwIniImporter::mergeFallback(multistrmap &cfg, const multistrmap &ini) const {
    cfg.erase("fallback");

    multistrmap::const_iterator iniIt;
    for(std::vector<std::string>::const_iterator it=mMergeFallback.begin(); it!=mMergeFallback.end(); ++it) {
        if((iniIt = ini.find(*it)) != ini.end()) {
            for(std::vector<std::string>::const_iterator vc = iniIt->second.begin(); vc != iniIt->second.end(); ++vc) {
                std::string value(*it);
                std::replace( value.begin(), value.end(), ' ', '_' );
                std::replace( value.begin(), value.end(), ':', '_' );
                value.append(",").append(vc->substr(0,vc->length()));
                insertMultistrmap(cfg, "fallback", value);
            }
        }
    }
}

void MwIniImporter::insertMultistrmap(multistrmap &cfg, const std::string& key, const std::string& value) {
    const multistrmap::const_iterator it = cfg.find(key);
    if(it == cfg.end()) {
        cfg.insert(std::make_pair (key, std::vector<std::string>() ));
    }
    cfg[key].push_back(value);
}

void MwIniImporter::importArchives(multistrmap &cfg, const multistrmap &ini) const {
    std::vector<std::string> archives;
    std::string baseArchive("Archives:Archive ");
    std::string archive;

    // Search archives listed in ini file
    multistrmap::const_iterator it = ini.begin();
    for(int i=0; it != ini.end(); i++) {
        archive = baseArchive;
        archive.append(std::to_string(i));

        it = ini.find(archive);
        if(it == ini.end()) {
            break;
        }

        for(std::vector<std::string>::const_iterator entry = it->second.begin(); entry!=it->second.end(); ++entry) {
            archives.push_back(*entry);
        }
    }

    cfg.erase("fallback-archive");
    cfg.insert( std::make_pair<std::string, std::vector<std::string> > ("fallback-archive", std::vector<std::string>()));

    // Add Morrowind.bsa by default, since Vanilla loads this archive even if it
    // does not appears in the ini file
    cfg["fallback-archive"].push_back("Morrowind.bsa");

    for(std::vector<std::string>::const_iterator iter=archives.begin(); iter!=archives.end(); ++iter) {
        cfg["fallback-archive"].push_back(*iter);
    }
}

void MwIniImporter::dependencySortStep(std::string& element, MwIniImporter::dependencyList& source, std::vector<std::string>& result)
{
    auto iter = std::find_if(
        source.begin(),
        source.end(),
        [&element](std::pair< std::string, std::vector<std::string> >& sourceElement)
        {
            return sourceElement.first == element;
        }
    );
    if (iter != source.end())
    {
        auto foundElement = std::move(*iter);
        source.erase(iter);
        for (auto name : foundElement.second)
        {
            MwIniImporter::dependencySortStep(name, source, result);
        }
        result.push_back(std::move(foundElement.first));
    }
}

std::vector<std::string> MwIniImporter::dependencySort(MwIniImporter::dependencyList source)
{
    std::vector<std::string> result;
    while (!source.empty())
    {
        MwIniImporter::dependencySortStep(source.begin()->first, source, result);
    }
    return result;
}

std::vector<std::string>::iterator MwIniImporter::findString(std::vector<std::string>& source, const std::string& string)
{
    return std::find_if(source.begin(), source.end(), [&string](const std::string& sourceString)
    {
        return Misc::StringUtils::ciEqual(sourceString, string);
    });
}

void MwIniImporter::addPaths(std::vector<boost::filesystem::path>& output, std::vector<std::string> input) {
    for (auto& path : input)
    {
        if (path.front() == '"')
        {
            // Drop first and last characters - quotation marks
            path = path.substr(1, path.size() - 2);
        }
        output.emplace_back(path);
    }
}

void MwIniImporter::importGameFiles(multistrmap &cfg, const multistrmap &ini, const boost::filesystem::path& iniFilename) const
{
    std::vector<std::pair<std::time_t, boost::filesystem::path>> contentFiles;
    std::string baseGameFile("Game Files:GameFile");
    std::time_t defaultTime = 0;
    ToUTF8::Utf8Encoder encoder(mEncoding);

    std::vector<boost::filesystem::path> dataPaths;
    if (cfg.count("data"))
        addPaths(dataPaths, cfg["data"]);

    if (cfg.count("data-local"))
        addPaths(dataPaths, cfg["data-local"]);

    dataPaths.push_back(iniFilename.parent_path() /= "Data Files");

    multistrmap::const_iterator it = ini.begin();
    for (int i=0; it != ini.end(); i++)
    {
        std::string gameFile = baseGameFile;
        gameFile.append(std::to_string(i));

        it = ini.find(gameFile);
        if(it == ini.end())
            break;

        for(std::vector<std::string>::const_iterator entry = it->second.begin(); entry!=it->second.end(); ++entry)
        {
            std::string filetype(entry->substr(entry->length()-3));
            Misc::StringUtils::lowerCaseInPlace(filetype);

            if(filetype.compare("esm") == 0 || filetype.compare("esp") == 0)
            {
                bool found = false;
                for (auto & dataPath : dataPaths)
                {
                    boost::filesystem::path path = dataPath / *entry;
                    std::time_t time = lastWriteTime(path, defaultTime);
                    if (time != defaultTime)
                    {
                        contentFiles.push_back({time, path});
                        found = true;
                        break;
                    }
                }
                if (!found)
                    std::cout << "Warning: " << *entry << " not found, ignoring" << std::endl;
            }
        }
    }

    cfg.erase("content");
    cfg.insert( std::make_pair("content", std::vector<std::string>() ) );

    // sort by timestamp
    sort(contentFiles.begin(), contentFiles.end());

    MwIniImporter::dependencyList unsortedFiles;

    ESM::ESMReader reader;
    reader.setEncoder(&encoder);
    for (auto& file : contentFiles)
    {
        reader.open(file.second.string());
        std::vector<std::string> dependencies;
        for (auto& gameFile : reader.getGameFiles())
        {
            dependencies.push_back(gameFile.name);
        }
        unsortedFiles.emplace_back(boost::filesystem::path(reader.getName()).filename().string(), dependencies);
        reader.close();
    }

    auto sortedFiles = dependencySort(unsortedFiles);

    // hard-coded dependency Morrowind - Tribunal - Bloodmoon
    if(findString(sortedFiles, "Morrowind.esm") != sortedFiles.end())
    {
        auto tribunalIter  = findString(sortedFiles, "Tribunal.esm");
        auto bloodmoonIter = findString(sortedFiles, "Bloodmoon.esm");

        if (bloodmoonIter != sortedFiles.end() && tribunalIter != sortedFiles.end())
        {
            size_t bloodmoonIndex = std::distance(sortedFiles.begin(), bloodmoonIter);
            size_t tribunalIndex  = std::distance(sortedFiles.begin(), tribunalIter);
            if (bloodmoonIndex < tribunalIndex)
                tribunalIndex++;
            sortedFiles.insert(bloodmoonIter, *tribunalIter);
            sortedFiles.erase(sortedFiles.begin() + tribunalIndex);
        }
    }

    for (auto& file : sortedFiles)
        cfg["content"].push_back(file);
}

void MwIniImporter::writeToFile(std::ostream &out, const multistrmap &cfg) {

    for(multistrmap::const_iterator it=cfg.begin(); it != cfg.end(); ++it) {
        for(std::vector<std::string>::const_iterator entry=it->second.begin(); entry != it->second.end(); ++entry) {
            out << (it->first) << "=" << (*entry) << std::endl;
        }
    }
}

void MwIniImporter::setInputEncoding(const ToUTF8::FromType &encoding)
{
  mEncoding = encoding;
}

std::time_t MwIniImporter::lastWriteTime(const boost::filesystem::path& filename, std::time_t defaultTime)
{
    std::time_t writeTime(defaultTime);
    if (boost::filesystem::exists(filename))
    {
        // FixMe: remove #if when Boost dependency for Linux builds updated
        // This allows Linux to build until then
#if (BOOST_VERSION >= 104800)
        // need to resolve any symlinks so that we get time of file, not symlink
        boost::filesystem::path resolved = boost::filesystem::canonical(filename);
#else
        boost::filesystem::path resolved = filename;
#endif
        writeTime = boost::filesystem::last_write_time(resolved);

        // print timestamp
        const int size=1024;
        char timeStrBuffer[size];
        if (std::strftime(timeStrBuffer, size, "%x %X", localtime(&writeTime)) > 0)
            std::cout << "content file: " << resolved << " timestamp = (" << writeTime <<
                ") " << timeStrBuffer << std::endl;
    }
    return writeTime;
}
