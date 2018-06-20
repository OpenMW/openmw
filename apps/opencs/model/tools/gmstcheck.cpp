#include "gmstcheck.hpp"

#include <sstream>

#include "../prefs/state.hpp"

#include "../world/defaultgmsts.hpp"

CSMTools::GmstCheckStage::GmstCheckStage(const CSMWorld::IdCollection<ESM::GameSetting>& gameSettings)
    : mGameSettings(gameSettings)
{
    mIgnoreBaseRecords = false;
}

int CSMTools::GmstCheckStage::setup()
{
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mGameSettings.getSize();
}

void CSMTools::GmstCheckStage::perform(int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<ESM::GameSetting>& record = mGameSettings.getRecord (stage);
    
    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && record.mState == CSMWorld::RecordBase::State_BaseOnly) || record.isDeleted())
        return;
    
    const ESM::GameSetting& gmst = record.get();
    
    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Gmst, gmst.mId);
    
    // Test for empty string
    if (gmst.mValue.getType() == ESM::VT_String && gmst.mValue.getString().empty())
        messages.add(id, gmst.mId + " is an empty string", "", CSMDoc::Message::Severity_Warning);
    
    // Checking type and limits
    // optimization - compare it to lists based on naming convention (f-float,i-int,s-string)
    if (gmst.mId[0] == 'f')
    {
        for (size_t i = 0; i < CSMWorld::DefaultGmsts::FloatCount; ++i)
        {
            if (gmst.mId == CSMWorld::DefaultGmsts::Floats[i])
            {
                if (gmst.mValue.getType() != ESM::VT_Float)
                {
                    std::ostringstream stream;
                    stream << "Expected float type for " << gmst.mId << " but found "
                        << varTypeToString(gmst.mValue.getType()) << " type";
                    
                    messages.add(id, stream.str(), "", CSMDoc::Message::Severity_Error);
                }
                
                if (gmst.mValue.getFloat() < CSMWorld::DefaultGmsts::FloatLimits[i*2])
                    messages.add(id, gmst.mId + " is less than the suggested range", "",
                                 CSMDoc::Message::Severity_Warning);
                
                if (gmst.mValue.getFloat() > CSMWorld::DefaultGmsts::FloatLimits[i*2+1])
                    messages.add(id, gmst.mId + " is more than the suggested range", "",
                                 CSMDoc::Message::Severity_Warning);
                
                break; // for loop
            }
        }
    }
    else if (gmst.mId[0] == 'i')
    {
        for (size_t i = 0; i < CSMWorld::DefaultGmsts::IntCount; ++i)
        {   
            if (gmst.mId == CSMWorld::DefaultGmsts::Ints[i])
            {
                if (gmst.mValue.getType() != ESM::VT_Int)
                {
                    std::ostringstream stream;
                    stream << "Expected int type for " << gmst.mId << " but found "
                        << varTypeToString(gmst.mValue.getType()) << " type";
                    
                    messages.add(id, stream.str(), "", CSMDoc::Message::Severity_Error);
                }
                
                if (gmst.mValue.getInteger() < CSMWorld::DefaultGmsts::IntLimits[i*2])
                    messages.add(id, gmst.mId + " is less than the suggested range", "",
                                 CSMDoc::Message::Severity_Warning);
                
                if (gmst.mValue.getInteger() > CSMWorld::DefaultGmsts::IntLimits[i*2+1])
                    messages.add(id, gmst.mId + " is more than the suggested range", "",
                                 CSMDoc::Message::Severity_Warning);
                
                break; // for loop
            }
        }
    }
    else if (gmst.mId[0] == 's')
    {
        for (size_t i = 0; i < CSMWorld::DefaultGmsts::StringCount; ++i)
        {
            if (gmst.mId == CSMWorld::DefaultGmsts::Strings[i])
            {
                ESM::VarType type = gmst.mValue.getType();
                
                if (type != ESM::VT_String && type != ESM::VT_None)
                {
                    std::ostringstream stream;
                    stream << "Expected string or none type for " << gmst.mId << " but found "
                        << varTypeToString(gmst.mValue.getType()) << " type";
                    
                    messages.add(id, stream.str(), "", CSMDoc::Message::Severity_Error);
                }
                
                break; // for loop
            }
        }
    }
}

std::string CSMTools::GmstCheckStage::varTypeToString(ESM::VarType type)
{
    switch (type)
    {
        case ESM::VT_Unknown: return "unknown";
        case ESM::VT_None: return "none";
        case ESM::VT_Short: return "short";
        case ESM::VT_Int: return "int";
        case ESM::VT_Long: return "long";
        case ESM::VT_Float: return "float";
        case ESM::VT_String: return "string";
        default: return "unhandled";
    }
}
