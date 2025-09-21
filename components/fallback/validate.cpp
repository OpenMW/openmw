#include "validate.hpp"

#include <boost/any.hpp>
#include <boost/program_options/errors.hpp>
#include <set>
#include <string>

static const std::set<std::string_view> allowedKeysInt
    = { "LightAttenuation_LinearMethod", "LightAttenuation_OutQuadInLin", "LightAttenuation_QuadraticMethod",
          "LightAttenuation_UseConstant", "LightAttenuation_UseLinear", "LightAttenuation_UseQuadratic",
          "Water_MaxNumberRipples", "Water_NearWaterRadius", "Water_NearWaterPoints", "Water_RippleFrameCount",
          "Water_SurfaceTileCount", "Water_SurfaceFrameCount", "Weather_Ashstorm_Max_Raindrops",
          "Weather_Blight_Max_Raindrops", "Weather_Blizzard_Max_Raindrops", "Weather_Clear_Max_Raindrops",
          "Weather_Cloudy_Max_Raindrops", "Weather_Foggy_Max_Raindrops", "Weather_Overcast_Max_Raindrops",
          "Weather_Rain_Max_Raindrops", "Weather_Snow_Max_Raindrops", "Weather_Thunderstorm_Max_Raindrops",
          "Weather_Clear_Using_Precip", "Weather_Cloudy_Using_Precip", "Weather_Foggy_Using_Precip",
          "Weather_Overcast_Using_Precip", "Weather_Rain_Using_Precip", "Weather_Thunderstorm_Using_Precip",
          "Weather_Ashstorm_Using_Precip", "Weather_Blight_Using_Precip", "Weather_Snow_Using_Precip",
          "Weather_Blizzard_Using_Precip", "Weather_Rain_Ripples", "Weather_Snow_Ripples", "Weather_Timescale_Clouds" };

static const std::set<std::string_view> allowedKeysFloat = { "General_Werewolf_FOV", "Inventory_DirectionalAmbientB",
    "Inventory_DirectionalAmbientG", "Inventory_DirectionalAmbientR", "Inventory_DirectionalDiffuseB",
    "Inventory_DirectionalDiffuseG", "Inventory_DirectionalDiffuseR", "Inventory_DirectionalRotationX",
    "Inventory_DirectionalRotationY", "LightAttenuation_ConstantValue", "LightAttenuation_LinearValue",
    "LightAttenuation_LinearRadiusMult", "LightAttenuation_QuadraticValue", "LightAttenuation_QuadraticRadiusMult",
    "Moons_Masser_Axis_Offset", "Moons_Masser_Daily_Increment", "Moons_Masser_Fade_End_Angle",
    "Moons_Masser_Fade_In_Finish", "Moons_Masser_Fade_In_Start", "Moons_Masser_Fade_Out_Finish",
    "Moons_Masser_Fade_Out_Start", "Moons_Masser_Fade_Start_Angle", "Moons_Masser_Moon_Shadow_Early_Fade_Angle",
    "Moons_Masser_Size", "Moons_Masser_Speed", "Moons_Secunda_Axis_Offset", "Moons_Secunda_Daily_Increment",
    "Moons_Secunda_Fade_End_Angle", "Moons_Secunda_Fade_In_Finish", "Moons_Secunda_Fade_In_Start",
    "Moons_Secunda_Fade_Out_Finish", "Moons_Secunda_Fade_Out_Start", "Moons_Secunda_Fade_Start_Angle",
    "Moons_Secunda_Moon_Shadow_Early_Fade_Angle", "Moons_Secunda_Size", "Moons_Secunda_Speed", "Water_World_Alpha",
    "Water_Map_Alpha", "Water_NearWaterIndoorTolerance", "Water_NearWaterOutdoorTolerance", "Water_RippleLifetime",
    "Water_RippleRotSpeed", "Water_SurfaceFPS", "Water_UnderwaterColorWeight", "Water_UnderwaterDayFog",
    "Water_UnderwaterIndoorFog", "Water_UnderwaterNightFog", "Water_UnderwaterSunriseFog", "Water_UnderwaterSunsetFog",
    "Weather_Ambient_Post-Sunrise_Time", "Weather_Ambient_Post-Sunset_Time", "Weather_Ambient_Pre-Sunrise_Time",
    "Weather_Ambient_Pre-Sunset_Time", "Weather_Ashstorm_Clouds_Maximum_Percent", "Weather_Ashstorm_Cloud_Speed",
    "Weather_Ashstorm_Flash_Decrement", "Weather_Ashstorm_Glare_View", "Weather_Ashstorm_Land_Fog_Day_Depth",
    "Weather_Ashstorm_Land_Fog_Night_Depth", "Weather_Ashstorm_Rain_Diameter", "Weather_Ashstorm_Rain_Entrance_Speed",
    "Weather_Ashstorm_Rain_Height_Max", "Weather_Ashstorm_Rain_Height_Min", "Weather_Ashstorm_Rain_Threshold",
    "Weather_Ashstorm_Thunder_Frequency", "Weather_Ashstorm_Thunder_Threshold", "Weather_Ashstorm_Transition_Delta",
    "Weather_Ashstorm_Wind_Speed", "Weather_Blight_Clouds_Maximum_Percent", "Weather_Blight_Cloud_Speed",
    "Weather_Blight_Flash_Decrement", "Weather_Blight_Glare_View", "Weather_Blight_Land_Fog_Day_Depth",
    "Weather_Blight_Land_Fog_Night_Depth", "Weather_Blight_Rain_Diameter", "Weather_Blight_Rain_Entrance_Speed",
    "Weather_Blight_Rain_Height_Max", "Weather_Blight_Rain_Height_Min", "Weather_Blight_Rain_Threshold",
    "Weather_Blight_Thunder_Frequency", "Weather_Blight_Thunder_Threshold", "Weather_Blight_Transition_Delta",
    "Weather_Blight_Wind_Speed", "Weather_Blizzard_Clouds_Maximum_Percent", "Weather_Blizzard_Cloud_Speed",
    "Weather_Blizzard_Flash_Decrement", "Weather_Blizzard_Glare_View", "Weather_Blizzard_Land_Fog_Day_Depth",
    "Weather_Blizzard_Land_Fog_Night_Depth", "Weather_Blizzard_Rain_Diameter", "Weather_Blizzard_Rain_Entrance_Speed",
    "Weather_Blizzard_Rain_Height_Max", "Weather_Blizzard_Rain_Height_Min", "Weather_Blizzard_Rain_Threshold",
    "Weather_Blizzard_Thunder_Frequency", "Weather_Blizzard_Thunder_Threshold", "Weather_Blizzard_Transition_Delta",
    "Weather_Blizzard_Wind_Speed", "Weather_Clear_Clouds_Maximum_Percent", "Weather_Clear_Cloud_Speed",
    "Weather_Clear_Flash_Decrement", "Weather_Clear_Glare_View", "Weather_Clear_Land_Fog_Day_Depth",
    "Weather_Clear_Land_Fog_Night_Depth", "Weather_Clear_Rain_Diameter", "Weather_Clear_Rain_Entrance_Speed",
    "Weather_Clear_Rain_Height_Max", "Weather_Clear_Rain_Height_Min", "Weather_Clear_Rain_Threshold",
    "Weather_Clear_Thunder_Frequency", "Weather_Clear_Thunder_Threshold", "Weather_Clear_Transition_Delta",
    "Weather_Clear_Wind_Speed", "Weather_Cloudy_Clouds_Maximum_Percent", "Weather_Cloudy_Cloud_Speed",
    "Weather_Cloudy_Flash_Decrement", "Weather_Cloudy_Glare_View", "Weather_Cloudy_Land_Fog_Day_Depth",
    "Weather_Cloudy_Land_Fog_Night_Depth", "Weather_Cloudy_Rain_Diameter", "Weather_Cloudy_Rain_Entrance_Speed",
    "Weather_Cloudy_Rain_Height_Max", "Weather_Cloudy_Rain_Height_Min", "Weather_Cloudy_Rain_Threshold",
    "Weather_Cloudy_Thunder_Frequency", "Weather_Cloudy_Thunder_Threshold", "Weather_Cloudy_Transition_Delta",
    "Weather_Cloudy_Wind_Speed", "Weather_Foggy_Clouds_Maximum_Percent", "Weather_Foggy_Cloud_Speed",
    "Weather_Foggy_Flash_Decrement", "Weather_Foggy_Glare_View", "Weather_Foggy_Land_Fog_Day_Depth",
    "Weather_Foggy_Land_Fog_Night_Depth", "Weather_Foggy_Rain_Diameter", "Weather_Foggy_Rain_Entrance_Speed",
    "Weather_Foggy_Rain_Height_Max", "Weather_Foggy_Rain_Height_Min", "Weather_Foggy_Rain_Threshold",
    "Weather_Foggy_Thunder_Frequency", "Weather_Foggy_Thunder_Threshold", "Weather_Foggy_Transition_Delta",
    "Weather_Foggy_Wind_Speed", "Weather_Fog_Post-Sunrise_Time", "Weather_Fog_Post-Sunset_Time",
    "Weather_Fog_Pre-Sunrise_Time", "Weather_Fog_Pre-Sunset_Time", "Weather_Hours_Between_Weather_Changes",
    "Weather_Maximum_Time_Between_Environmental_Sounds", "Weather_Minimum_Time_Between_Environmental_Sounds",
    "Weather_Overcast_Clouds_Maximum_Percent", "Weather_Overcast_Cloud_Speed", "Weather_Overcast_Flash_Decrement",
    "Weather_Overcast_Glare_View", "Weather_Overcast_Land_Fog_Day_Depth", "Weather_Overcast_Land_Fog_Night_Depth",
    "Weather_Overcast_Rain_Diameter", "Weather_Overcast_Rain_Entrance_Speed", "Weather_Overcast_Rain_Height_Max",
    "Weather_Overcast_Rain_Height_Min", "Weather_Overcast_Rain_Threshold", "Weather_Overcast_Thunder_Frequency",
    "Weather_Overcast_Thunder_Threshold", "Weather_Overcast_Transition_Delta", "Weather_Overcast_Wind_Speed",
    "Weather_Precip_Gravity", "Weather_Rain_Clouds_Maximum_Percent", "Weather_Rain_Cloud_Speed",
    "Weather_Rain_Flash_Decrement", "Weather_Rain_Glare_View", "Weather_Rain_Land_Fog_Day_Depth",
    "Weather_Rain_Land_Fog_Night_Depth", "Weather_Rain_Rain_Diameter", "Weather_Rain_Rain_Entrance_Speed",
    "Weather_Rain_Rain_Height_Max", "Weather_Rain_Rain_Height_Min", "Weather_Rain_Rain_Threshold",
    "Weather_Rain_Thunder_Frequency", "Weather_Rain_Thunder_Threshold", "Weather_Rain_Transition_Delta",
    "Weather_Rain_Wind_Speed", "Weather_Sky_Post-Sunrise_Time", "Weather_Sky_Post-Sunset_Time",
    "Weather_Sky_Pre-Sunrise_Time", "Weather_Sky_Pre-Sunset_Time", "Weather_Snow_Clouds_Maximum_Percent",
    "Weather_Snow_Cloud_Speed", "Weather_Snow_Flash_Decrement", "Weather_Snow_Glare_View",
    "Weather_Snow_Land_Fog_Day_Depth", "Weather_Snow_Land_Fog_Night_Depth", "Weather_Snow_Rain_Diameter",
    "Weather_Snow_Rain_Entrance_Speed", "Weather_Snow_Rain_Height_Max", "Weather_Snow_Rain_Height_Min",
    "Weather_Snow_Rain_Threshold", "Weather_Snow_Thunder_Frequency", "Weather_Snow_Thunder_Threshold",
    "Weather_Snow_Transition_Delta", "Weather_Snow_Wind_Speed", "Weather_Stars_Fading_Duration",
    "Weather_Stars_Post-Sunset_Start", "Weather_Stars_Pre-Sunrise_Finish", "Weather_Sun_Glare_Fader_Angle_Max",
    "Weather_Sun_Glare_Fader_Max", "Weather_Sun_Post-Sunrise_Time", "Weather_Sun_Post-Sunset_Time",
    "Weather_Sun_Pre-Sunrise_Time", "Weather_Sun_Pre-Sunset_Time", "Weather_Sunrise_Duration", "Weather_Sunrise_Time",
    "Weather_Sunset_Duration", "Weather_Sunset_Time", "Weather_Thunderstorm_Clouds_Maximum_Percent",
    "Weather_Thunderstorm_Cloud_Speed", "Weather_Thunderstorm_Flash_Decrement", "Weather_Thunderstorm_Glare_View",
    "Weather_Thunderstorm_Land_Fog_Day_Depth", "Weather_Thunderstorm_Land_Fog_Night_Depth",
    "Weather_Thunderstorm_Rain_Diameter", "Weather_Thunderstorm_Rain_Entrance_Speed",
    "Weather_Thunderstorm_Rain_Height_Max", "Weather_Thunderstorm_Rain_Height_Min",
    "Weather_Thunderstorm_Rain_Threshold", "Weather_Thunderstorm_Thunder_Frequency",
    "Weather_Thunderstorm_Thunder_Threshold", "Weather_Thunderstorm_Transition_Delta",
    "Weather_Thunderstorm_Wind_Speed" };

static const std::set<std::string_view> allowedKeysNonNumeric = { "Blood_Model_0", "Blood_Model_1", "Blood_Model_2",
    "FontColor_color_active", "FontColor_color_active_over", "FontColor_color_active_pressed", "FontColor_color_answer",
    "FontColor_color_answer_over", "FontColor_color_answer_pressed", "FontColor_color_background",
    "FontColor_color_big_answer", "FontColor_color_big_answer_over", "FontColor_color_big_answer_pressed",
    "FontColor_color_big_header", "FontColor_color_big_link", "FontColor_color_big_link_over",
    "FontColor_color_big_link_pressed", "FontColor_color_big_normal", "FontColor_color_big_normal_over",
    "FontColor_color_big_normal_pressed", "FontColor_color_big_notify", "FontColor_color_count",
    "FontColor_color_disabled", "FontColor_color_disabled_over", "FontColor_color_disabled_pressed",
    "FontColor_color_fatigue", "FontColor_color_focus", "FontColor_color_header", "FontColor_color_health",
    "FontColor_color_journal_link", "FontColor_color_journal_link_over", "FontColor_color_journal_link_pressed",
    "FontColor_color_journal_topic", "FontColor_color_journal_topic_over", "FontColor_color_journal_topic_pressed",
    "FontColor_color_link", "FontColor_color_link_over", "FontColor_color_link_pressed", "FontColor_color_magic",
    "FontColor_color_magic_fill", "FontColor_color_misc", "FontColor_color_negative", "FontColor_color_normal",
    "FontColor_color_normal_over", "FontColor_color_normal_pressed", "FontColor_color_notify",
    "FontColor_color_positive", "FontColor_color_weapon_fill", "Fonts_Font_0", "Fonts_Font_1", "Fonts_Font_2",
    "Level_Up_Default", "Moons_Script_Color", "Movies_Company_Logo", "Movies_Morrowind_Logo", "Movies_New_Game",
    "Question_10_AnswerOne", "Question_10_AnswerThree", "Question_10_AnswerTwo", "Question_10_Question",
    "Question_10_Sound", "Question_1_AnswerOne", "Question_1_AnswerThree", "Question_1_AnswerTwo",
    "Question_1_Question", "Question_1_Sound", "Question_2_AnswerOne", "Question_2_AnswerThree", "Question_2_AnswerTwo",
    "Question_2_Question", "Question_2_Sound", "Question_3_AnswerOne", "Question_3_AnswerThree", "Question_3_AnswerTwo",
    "Question_3_Question", "Question_3_Sound", "Question_4_AnswerOne", "Question_4_AnswerThree", "Question_4_AnswerTwo",
    "Question_4_Question", "Question_4_Sound", "Question_5_AnswerOne", "Question_5_AnswerThree", "Question_5_AnswerTwo",
    "Question_5_Question", "Question_5_Sound", "Question_6_AnswerOne", "Question_6_AnswerThree", "Question_6_AnswerTwo",
    "Question_6_Question", "Question_6_Sound", "Question_7_AnswerOne", "Question_7_AnswerThree", "Question_7_AnswerTwo",
    "Question_7_Question", "Question_7_Sound", "Question_8_AnswerOne", "Question_8_AnswerThree", "Question_8_AnswerTwo",
    "Question_8_Question", "Question_8_Sound", "Question_9_AnswerOne", "Question_9_AnswerThree", "Question_9_AnswerTwo",
    "Question_9_Question", "Question_9_Sound", "Water_NearWaterIndoorID", "Water_NearWaterOutdoorID",
    "Water_RippleTexture", "Water_SurfaceTexture", "Water_UnderwaterColor", "Weather_Blizzard_Ambient_Day_Color",
    "Weather_Blizzard_Ambient_Loop_Sound_ID", "Weather_Blizzard_Ambient_Night_Color",
    "Weather_Blizzard_Ambient_Sunrise_Color", "Weather_Blizzard_Ambient_Sunset_Color", "Weather_Blizzard_Cloud_Texture",
    "Weather_Blizzard_Fog_Day_Color", "Weather_Blizzard_Fog_Night_Color", "Weather_Blizzard_Fog_Sunrise_Color",
    "Weather_Blizzard_Fog_Sunset_Color", "Weather_Blizzard_Sky_Day_Color", "Weather_Blizzard_Sky_Night_Color",
    "Weather_Blizzard_Sky_Sunrise_Color", "Weather_Blizzard_Sky_Sunset_Color", "Weather_Blizzard_Storm_Threshold",
    "Weather_Blizzard_Sun_Day_Color", "Weather_Blizzard_Sun_Disc_Sunset_Color", "Weather_Blizzard_Sun_Night_Color",
    "Weather_Blizzard_Sun_Sunrise_Color", "Weather_Blizzard_Sun_Sunset_Color", "Weather_BumpFadeColor",
    "Weather_Clear_Ambient_Day_Color", "Weather_Clear_Ambient_Loop_Sound_ID", "Weather_Clear_Ambient_Night_Color",
    "Weather_Clear_Ambient_Sunrise_Color", "Weather_Clear_Ambient_Sunset_Color", "Weather_Clear_Cloud_Texture",
    "Weather_Clear_Fog_Day_Color", "Weather_Clear_Fog_Night_Color", "Weather_Clear_Fog_Sunrise_Color",
    "Weather_Clear_Fog_Sunset_Color", "Weather_Clear_Sky_Day_Color", "Weather_Clear_Sky_Night_Color",
    "Weather_Clear_Sky_Sunrise_Color", "Weather_Clear_Sky_Sunset_Color", "Weather_Clear_Sun_Day_Color",
    "Weather_Clear_Sun_Disc_Sunset_Color", "Weather_Clear_Sun_Night_Color", "Weather_Clear_Sun_Sunrise_Color",
    "Weather_Clear_Sun_Sunset_Color", "Weather_Cloudy_Ambient_Day_Color", "Weather_Cloudy_Ambient_Loop_Sound_ID",
    "Weather_Cloudy_Ambient_Night_Color", "Weather_Cloudy_Ambient_Sunrise_Color", "Weather_Cloudy_Ambient_Sunset_Color",
    "Weather_Cloudy_Cloud_Texture", "Weather_Cloudy_Fog_Day_Color", "Weather_Cloudy_Fog_Night_Color",
    "Weather_Cloudy_Fog_Sunrise_Color", "Weather_Cloudy_Fog_Sunset_Color", "Weather_Cloudy_Sky_Day_Color",
    "Weather_Cloudy_Sky_Night_Color", "Weather_Cloudy_Sky_Sunrise_Color", "Weather_Cloudy_Sky_Sunset_Color",
    "Weather_Cloudy_Sun_Day_Color", "Weather_Cloudy_Sun_Disc_Sunset_Color", "Weather_Cloudy_Sun_Night_Color",
    "Weather_Cloudy_Sun_Sunrise_Color", "Weather_Cloudy_Sun_Sunset_Color", "Weather_EnvReduceColor",
    "Weather_Fog_Depth_Change_Speed", "Weather_Foggy_Ambient_Day_Color", "Weather_Foggy_Ambient_Loop_Sound_ID",
    "Weather_Foggy_Ambient_Night_Color", "Weather_Foggy_Ambient_Sunrise_Color", "Weather_Foggy_Ambient_Sunset_Color",
    "Weather_Foggy_Cloud_Texture", "Weather_Foggy_Fog_Day_Color", "Weather_Foggy_Fog_Night_Color",
    "Weather_Foggy_Fog_Sunrise_Color", "Weather_Foggy_Fog_Sunset_Color", "Weather_Foggy_Sky_Day_Color",
    "Weather_Foggy_Sky_Night_Color", "Weather_Foggy_Sky_Sunrise_Color", "Weather_Foggy_Sky_Sunset_Color",
    "Weather_Foggy_Sun_Day_Color", "Weather_Foggy_Sun_Disc_Sunset_Color", "Weather_Foggy_Sun_Night_Color",
    "Weather_Foggy_Sun_Sunrise_Color", "Weather_Foggy_Sun_Sunset_Color", "Weather_LerpCloseColor",
    "Weather_Overcast_Ambient_Day_Color", "Weather_Overcast_Ambient_Loop_Sound_ID",
    "Weather_Overcast_Ambient_Night_Color", "Weather_Overcast_Ambient_Sunrise_Color",
    "Weather_Overcast_Ambient_Sunset_Color", "Weather_Overcast_Cloud_Texture", "Weather_Overcast_Fog_Day_Color",
    "Weather_Overcast_Fog_Night_Color", "Weather_Overcast_Fog_Sunrise_Color", "Weather_Overcast_Fog_Sunset_Color",
    "Weather_Overcast_Sky_Day_Color", "Weather_Overcast_Sky_Night_Color", "Weather_Overcast_Sky_Sunrise_Color",
    "Weather_Overcast_Sky_Sunset_Color", "Weather_Overcast_Sun_Day_Color", "Weather_Overcast_Sun_Disc_Sunset_Color",
    "Weather_Overcast_Sun_Night_Color", "Weather_Overcast_Sun_Sunrise_Color", "Weather_Overcast_Sun_Sunset_Color",
    "Weather_Rain_Ambient_Day_Color", "Weather_Rain_Ambient_Loop_Sound_ID", "Weather_Rain_Ambient_Night_Color",
    "Weather_Rain_Ambient_Sunrise_Color", "Weather_Rain_Ambient_Sunset_Color", "Weather_Rain_Cloud_Texture",
    "Weather_Rain_Fog_Day_Color", "Weather_Rain_Fog_Night_Color", "Weather_Rain_Fog_Sunrise_Color",
    "Weather_Rain_Fog_Sunset_Color", "Weather_Rain_Rain_Loop_Sound_ID", "Weather_Rain_Ripple_Radius",
    "Weather_Rain_Ripple_Scale", "Weather_Rain_Ripple_Speed", "Weather_Rain_Ripples_Per_Drop",
    "Weather_Rain_Sky_Day_Color", "Weather_Rain_Sky_Night_Color", "Weather_Rain_Sky_Sunrise_Color",
    "Weather_Rain_Sky_Sunset_Color", "Weather_Rain_Sun_Day_Color", "Weather_Rain_Sun_Disc_Sunset_Color",
    "Weather_Rain_Sun_Night_Color", "Weather_Rain_Sun_Sunrise_Color", "Weather_Rain_Sun_Sunset_Color",
    "Weather_Snow_Ambient_Day_Color", "Weather_Snow_Ambient_Loop_Sound_ID", "Weather_Snow_Ambient_Night_Color",
    "Weather_Snow_Ambient_Sunrise_Color", "Weather_Snow_Ambient_Sunset_Color", "Weather_Snow_Cloud_Texture",
    "Weather_Snow_Fog_Day_Color", "Weather_Snow_Fog_Night_Color", "Weather_Snow_Fog_Sunrise_Color",
    "Weather_Snow_Fog_Sunset_Color", "Weather_Snow_Gravity_Scale", "Weather_Snow_High_Kill", "Weather_Snow_Low_Kill",
    "Weather_Snow_Max_Snowflakes", "Weather_Snow_Ripple_Radius", "Weather_Snow_Ripple_Scale",
    "Weather_Snow_Ripple_Speed", "Weather_Snow_Ripples_Per_Flake", "Weather_Snow_Sky_Day_Color",
    "Weather_Snow_Sky_Night_Color", "Weather_Snow_Sky_Sunrise_Color", "Weather_Snow_Sky_Sunset_Color",
    "Weather_Snow_Snow_Diameter", "Weather_Snow_Snow_Entrance_Speed", "Weather_Snow_Snow_Height_Max",
    "Weather_Snow_Snow_Height_Min", "Weather_Snow_Snow_Threshold", "Weather_Snow_Sun_Day_Color",
    "Weather_Snow_Sun_Disc_Sunset_Color", "Weather_Snow_Sun_Night_Color", "Weather_Snow_Sun_Sunrise_Color",
    "Weather_Snow_Sun_Sunset_Color", "Weather_Sun_Glare_Fader_Color", "Weather_Thunderstorm_Ambient_Day_Color",
    "Weather_Thunderstorm_Ambient_Loop_Sound_ID", "Weather_Thunderstorm_Ambient_Night_Color",
    "Weather_Thunderstorm_Ambient_Sunrise_Color", "Weather_Thunderstorm_Ambient_Sunset_Color",
    "Weather_Thunderstorm_Cloud_Texture", "Weather_Thunderstorm_Fog_Day_Color", "Weather_Thunderstorm_Fog_Night_Color",
    "Weather_Thunderstorm_Fog_Sunrise_Color", "Weather_Thunderstorm_Fog_Sunset_Color",
    "Weather_Thunderstorm_Rain_Loop_Sound_ID", "Weather_Thunderstorm_Sky_Day_Color",
    "Weather_Thunderstorm_Sky_Night_Color", "Weather_Thunderstorm_Sky_Sunrise_Color",
    "Weather_Thunderstorm_Sky_Sunset_Color", "Weather_Thunderstorm_Sun_Day_Color",
    "Weather_Thunderstorm_Sun_Disc_Sunset_Color", "Weather_Thunderstorm_Sun_Night_Color",
    "Weather_Thunderstorm_Sun_Sunrise_Color", "Weather_Thunderstorm_Sun_Sunset_Color",
    "Weather_Thunderstorm_Thunder_Sound_ID_0", "Weather_Thunderstorm_Thunder_Sound_ID_1",
    "Weather_Thunderstorm_Thunder_Sound_ID_2", "Weather_Thunderstorm_Thunder_Sound_ID_3",
    "Weather_Clear_Thunder_Sound_ID_0", "Weather_Clear_Thunder_Sound_ID_1", "Weather_Clear_Thunder_Sound_ID_2",
    "Weather_Clear_Thunder_Sound_ID_3", "Weather_Cloudy_Thunder_Sound_ID_0", "Weather_Cloudy_Thunder_Sound_ID_1",
    "Weather_Cloudy_Thunder_Sound_ID_2", "Weather_Cloudy_Thunder_Sound_ID_3", "Weather_Foggy_Thunder_Sound_ID_0",
    "Weather_Foggy_Thunder_Sound_ID_1", "Weather_Foggy_Thunder_Sound_ID_2", "Weather_Foggy_Thunder_Sound_ID_3",
    "Weather_Overcast_Thunder_Sound_ID_0", "Weather_Overcast_Thunder_Sound_ID_1", "Weather_Overcast_Thunder_Sound_ID_2",
    "Weather_Overcast_Thunder_Sound_ID_3", "Weather_Rain_Thunder_Sound_ID_0", "Weather_Rain_Thunder_Sound_ID_1",
    "Weather_Rain_Thunder_Sound_ID_2", "Weather_Rain_Thunder_Sound_ID_3", "Weather_Ashstorm_Cloud_Texture",
    "Weather_Ashstorm_Sky_Sunrise_Color", "Weather_Ashstorm_Sky_Day_Color", "Weather_Ashstorm_Sky_Sunset_Color",
    "Weather_Ashstorm_Sky_Night_Color", "Weather_Ashstorm_Fog_Sunrise_Color", "Weather_Ashstorm_Fog_Day_Color",
    "Weather_Ashstorm_Fog_Sunset_Color", "Weather_Ashstorm_Fog_Night_Color", "Weather_Ashstorm_Ambient_Sunrise_Color",
    "Weather_Ashstorm_Ambient_Day_Color", "Weather_Ashstorm_Ambient_Sunset_Color",
    "Weather_Ashstorm_Ambient_Night_Color", "Weather_Ashstorm_Sun_Sunrise_Color", "Weather_Ashstorm_Sun_Day_Color",
    "Weather_Ashstorm_Sun_Sunset_Color", "Weather_Ashstorm_Sun_Night_Color", "Weather_Ashstorm_Sun_Disc_Sunset_Color",
    "Weather_Ashstorm_Thunder_Sound_ID_0", "Weather_Ashstorm_Thunder_Sound_ID_1", "Weather_Ashstorm_Thunder_Sound_ID_2",
    "Weather_Ashstorm_Thunder_Sound_ID_3", "Weather_Ashstorm_Ambient_Loop_Sound_ID", "Weather_Blight_Cloud_Texture",
    "Weather_Blight_Sky_Sunrise_Color", "Weather_Blight_Sky_Day_Color", "Weather_Blight_Sky_Sunset_Color",
    "Weather_Blight_Sky_Night_Color", "Weather_Blight_Fog_Sunrise_Color", "Weather_Blight_Fog_Day_Color",
    "Weather_Blight_Fog_Sunset_Color", "Weather_Blight_Fog_Night_Color", "Weather_Blight_Ambient_Sunrise_Color",
    "Weather_Blight_Ambient_Day_Color", "Weather_Blight_Ambient_Sunset_Color", "Weather_Blight_Ambient_Night_Color",
    "Weather_Blight_Sun_Sunrise_Color", "Weather_Blight_Sun_Day_Color", "Weather_Blight_Sun_Sunset_Color",
    "Weather_Blight_Sun_Night_Color", "Weather_Blight_Sun_Disc_Sunset_Color", "Weather_Blight_Thunder_Sound_ID_0",
    "Weather_Blight_Thunder_Sound_ID_1", "Weather_Blight_Thunder_Sound_ID_2", "Weather_Blight_Thunder_Sound_ID_3",
    "Weather_Blight_Ambient_Loop_Sound_ID", "Weather_Snow_Thunder_Sound_ID_0", "Weather_Snow_Thunder_Sound_ID_1",
    "Weather_Snow_Thunder_Sound_ID_2", "Weather_Snow_Thunder_Sound_ID_3", "Weather_Blizzard_Thunder_Sound_ID_0",
    "Weather_Blizzard_Thunder_Sound_ID_1", "Weather_Blizzard_Thunder_Sound_ID_2",
    "Weather_Blizzard_Thunder_Sound_ID_3" };

static const std::set<std::string_view> allowedKeysUnused = { "Inventory_UniformScaling", "Map_Show_Travel_Lines",
    "Map_Travel_Boat_Blue", "Map_Travel_Boat_Green", "Map_Travel_Boat_Red", "Map_Travel_Magic_Blue",
    "Map_Travel_Magic_Green", "Map_Travel_Magic_Red", "Map_Travel_Siltstrider_Blue", "Map_Travel_Siltstrider_Green",
    "Map_Travel_Siltstrider_Red", "Movies_Game_Logo", "PixelWater_Resolution", "PixelWater_SurfaceFPS",
    "PixelWater_TileCount", "Movies_Loading", "Movies_Options_Menu", "Movies_Project_Logo",
    "Water_NearWaterUnderwaterFreq", "Water_NearWaterUnderwaterVolume", "Water_PSWaterReflectTerrain",
    "Water_PSWaterReflectUpdate", "Water_RippleAlphas", "Water_RippleScale", "Water_SurfaceTextureSize",
    "Water_TileTextureDivisor", "Weather_AlphaReduce", "Weather_Ashstorm_Storm_Threshold",
    "Weather_Blight_Disease_Chance", "Weather_Blight_Storm_Threshold" };

bool Fallback::isAllowedIntFallbackKey(std::string_view key)
{
    return allowedKeysInt.contains(key);
}

bool Fallback::isAllowedFloatFallbackKey(std::string_view key)
{
    return allowedKeysFloat.contains(key);
}

bool Fallback::isAllowedNonNumericFallbackKey(std::string_view key)
{
    return allowedKeysNonNumeric.contains(key) || key.starts_with("Blood_Texture_")
        || key.starts_with("Level_Up_Level");
}

bool Fallback::isAllowedUnusedFallbackKey(std::string_view key)
{
    return allowedKeysUnused.contains(key);
}

void Fallback::validate(boost::any& v, std::vector<std::string> const& tokens, FallbackMap*, int)
{
    if (v.empty())
    {
        v = boost::any(FallbackMap());
    }

    FallbackMap* map = boost::any_cast<FallbackMap>(&v);

    for (const auto& token : tokens)
    {
        size_t sep = token.find(',');
        if (sep < 1 || sep == token.length() - 1 || sep == std::string::npos)
            throw boost::program_options::validation_error(
                boost::program_options::validation_error::invalid_option_value);

        std::string key(token.substr(0, sep));
        std::string value(token.substr(sep + 1));

        map->mMap[key] = std::move(value);
    }
}
