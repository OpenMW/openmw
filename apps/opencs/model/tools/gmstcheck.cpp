#include "gmstcheck.hpp"

#include "../world/defaultgmsts.hpp"

CSMTools::GMSTCheckStage::GMSTCheckStage(const CSMWorld::IdCollection<ESM::GameSetting>& gameSettings)
    : mGameSettings(gameSettings)
{}

int CSMTools::GMSTCheckStage::setup()
{
    return mGameSettings.getSize();
}

void CSMTools::GMSTCheckStage::perform(int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::GameSetting>& record = mGameSettings.getRecord (stage);
    
    if (record.isDeleted())
        return;
    
    const ESM::GameSetting& gmst = record.get();
    
    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Gmst, gmst.mId);
    
    // Test for empty string
    if (gmst.mValue.getType() == ESM::VT_String && gmst.mValue.getString().empty())
        messages.add(id, gmst.mId + " is an empty string", "", CSMDoc::Message::Severity_Warning);
    
    // Checking type and limits
    // optimization - compare it to lists based on naming convention (f-float,i-int,s-string)
    if (gmst.mId.empty())
        messages.add(id, "Game Setting has no name", "", CSMDoc::Message::Severity_SeriousError);
    else if (gmst.mId[0] == 'f')
    {
        size_t floatCount = CSMWorld::DefaultGMSTs::getFloatCount();
        for (size_t i = 0; i < floatCount; ++i)
        {
            if (gmst.mId == CSMWorld::DefaultGMSTs::getFloatName(i))
            {
                if (gmst.mValue.getType() != ESM::VT_Float)
                    messages.add(id, gmst.mId + " has an invalid type; it should be a float", 
                                 "Change the GMST type to a float", CSMDoc::Message::Severity_Error);
                
                if (gmst.mValue.getFloat() < CSMWorld::DefaultGMSTs::getFloatLowerLimit(i))
                    messages.add(id, gmst.mId + " is less than the suggested range", "Change the value",
                                 CSMDoc::Message::Severity_Warning);
                
                if (gmst.mValue.getFloat() > CSMWorld::DefaultGMSTs::getFloatUpperLimit(i))
                    messages.add(id, gmst.mId + " is more than the suggested range", "Change the value",
                                 CSMDoc::Message::Severity_Warning);
                
                break; // for loop
            }
        }
    }
    else if (gmst.mId[0] == 'i')
    {
        size_t intCount = CSMWorld::DefaultGMSTs::getIntCount();
        for (size_t i = 0; i < intCount; ++i)
        {   
            if (gmst.mId == CSMWorld::DefaultGMSTs::getIntName(i))
            {
                if (gmst.mValue.getType() != ESM::VT_Int)
                    messages.add(id, gmst.mId + " has an invalid type; it should be an integer",
                                 "Change the GMST type to an int", CSMDoc::Message::Severity_Error);
                
                if (gmst.mValue.getInteger() < CSMWorld::DefaultGMSTs::getIntLowerLimit(i))
                    messages.add(id, gmst.mId + " is less than the suggested range", "Change the value",
                                 CSMDoc::Message::Severity_Warning);
                
                if (gmst.mValue.getInteger() > CSMWorld::DefaultGMSTs::getIntUpperLimit(i))
                    messages.add(id, gmst.mId + " is more than the suggested range", "Change the value",
                                 CSMDoc::Message::Severity_Warning);
                
                break; // for loop
            }
        }
    }
    else if (gmst.mId[0] == 's')
    {
        size_t stringCount = CSMWorld::DefaultGMSTs::getStringCount();
        for (size_t i = 0; i < stringCount; ++i)
        {
            if (gmst.mId == CSMWorld::DefaultGMSTs::getStringName(i))
            {
                ESM::VarType type = gmst.mValue.getType();
                
                if (type != ESM::VT_String && type != ESM::VT_None)
                    messages.add(id, gmst.mId + " has an invalid type; it should be a string", 
                                "Change the GMST type to a string", CSMDoc::Message::Severity_Error);
                
                break; // for loop
            }
        }
    }
}
