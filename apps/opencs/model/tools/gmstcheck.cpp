#include "gmstcheck.hpp"

CSMTools::GMSTCheckStage::GMSTCheckStage(const CSMWorld::IdCollection<ESM::GameSetting>& gameSettings)
: mGameSettings(gameSettings)
{}

int CSMTools::GMSTCheckStage::setup()
{
    return mGameSettings.getSize();
}

void CSMTools::GMSTCheckStage::perform(int stage, CSMDoc::Messages& messages)
{
}
