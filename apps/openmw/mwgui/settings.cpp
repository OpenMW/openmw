#include "settings.hpp"

#include "components/settings/values.hpp"

namespace MWGui
{
    WindowSettingValues makeAlchemyWindowSettingValues()
    {
        return WindowSettingValues{
            .mRegular = WindowRectSettingValues {
                .mX = Settings::windows().mAlchemyX,
                .mY = Settings::windows().mAlchemyY,
                .mW = Settings::windows().mAlchemyW,
                .mH = Settings::windows().mAlchemyH,
            },
            .mMaximized = WindowRectSettingValues {
                .mX = Settings::windows().mAlchemyMaximizedX,
                .mY = Settings::windows().mAlchemyMaximizedY,
                .mW = Settings::windows().mAlchemyMaximizedW,
                .mH = Settings::windows().mAlchemyMaximizedH,
            },
            .mIsMaximized = Settings::windows().mAlchemyMaximized,
        };
    }

    WindowSettingValues makeBarterWindowSettingValues()
    {
        return WindowSettingValues{
            .mRegular = WindowRectSettingValues {
                .mX = Settings::windows().mBarterX,
                .mY = Settings::windows().mBarterY,
                .mW = Settings::windows().mBarterW,
                .mH = Settings::windows().mBarterH,
            },
            .mMaximized = WindowRectSettingValues {
                .mX = Settings::windows().mBarterMaximizedX,
                .mY = Settings::windows().mBarterMaximizedY,
                .mW = Settings::windows().mBarterMaximizedW,
                .mH = Settings::windows().mBarterMaximizedH,
            },
            .mIsMaximized = Settings::windows().mBarterMaximized,
        };
    }

    WindowSettingValues makeCompanionWindowSettingValues()
    {
        return WindowSettingValues{
            .mRegular = WindowRectSettingValues {
                .mX = Settings::windows().mCompanionX,
                .mY = Settings::windows().mCompanionY,
                .mW = Settings::windows().mCompanionW,
                .mH = Settings::windows().mCompanionH,
            },
            .mMaximized = WindowRectSettingValues {
                .mX = Settings::windows().mCompanionMaximizedX,
                .mY = Settings::windows().mCompanionMaximizedY,
                .mW = Settings::windows().mCompanionMaximizedW,
                .mH = Settings::windows().mCompanionMaximizedH,
            },
            .mIsMaximized = Settings::windows().mCompanionMaximized,
        };
    }

    WindowSettingValues makeConsoleWindowSettingValues()
    {
        return WindowSettingValues{
            .mRegular = WindowRectSettingValues {
                .mX = Settings::windows().mConsoleX,
                .mY = Settings::windows().mConsoleY,
                .mW = Settings::windows().mConsoleW,
                .mH = Settings::windows().mConsoleH,
            },
            .mMaximized = WindowRectSettingValues {
                .mX = Settings::windows().mConsoleMaximizedX,
                .mY = Settings::windows().mConsoleMaximizedY,
                .mW = Settings::windows().mConsoleMaximizedW,
                .mH = Settings::windows().mConsoleMaximizedH,
            },
            .mIsMaximized = Settings::windows().mConsoleMaximized,
        };
    }

    WindowSettingValues makeContainerWindowSettingValues()
    {
        return WindowSettingValues{
            .mRegular = WindowRectSettingValues {
                .mX = Settings::windows().mContainerX,
                .mY = Settings::windows().mContainerY,
                .mW = Settings::windows().mContainerW,
                .mH = Settings::windows().mContainerH,
            },
            .mMaximized = WindowRectSettingValues {
                .mX = Settings::windows().mContainerMaximizedX,
                .mY = Settings::windows().mContainerMaximizedY,
                .mW = Settings::windows().mContainerMaximizedW,
                .mH = Settings::windows().mContainerMaximizedH,
            },
            .mIsMaximized = Settings::windows().mContainerMaximized,
        };
    }

    WindowSettingValues makeDebugWindowSettingValues()
    {
        return WindowSettingValues{
            .mRegular = WindowRectSettingValues {
                .mX = Settings::windows().mDebugX,
                .mY = Settings::windows().mDebugY,
                .mW = Settings::windows().mDebugW,
                .mH = Settings::windows().mDebugH,
            },
            .mMaximized = WindowRectSettingValues {
                .mX = Settings::windows().mDebugMaximizedX,
                .mY = Settings::windows().mDebugMaximizedY,
                .mW = Settings::windows().mDebugMaximizedW,
                .mH = Settings::windows().mDebugMaximizedH,
            },
            .mIsMaximized = Settings::windows().mDebugMaximized,
        };
    }

    WindowSettingValues makeDialogueWindowSettingValues()
    {
        return WindowSettingValues{
            .mRegular = WindowRectSettingValues {
                .mX = Settings::windows().mDialogueX,
                .mY = Settings::windows().mDialogueY,
                .mW = Settings::windows().mDialogueW,
                .mH = Settings::windows().mDialogueH,
            },
            .mMaximized = WindowRectSettingValues {
                .mX = Settings::windows().mDialogueMaximizedX,
                .mY = Settings::windows().mDialogueMaximizedY,
                .mW = Settings::windows().mDialogueMaximizedW,
                .mH = Settings::windows().mDialogueMaximizedH,
            },
            .mIsMaximized = Settings::windows().mDialogueMaximized,
        };
    }

    WindowSettingValues makeInventoryWindowSettingValues()
    {
        return WindowSettingValues{
            .mRegular = WindowRectSettingValues {
                .mX = Settings::windows().mInventoryX,
                .mY = Settings::windows().mInventoryY,
                .mW = Settings::windows().mInventoryW,
                .mH = Settings::windows().mInventoryH,
            },
            .mMaximized = WindowRectSettingValues {
                .mX = Settings::windows().mInventoryMaximizedX,
                .mY = Settings::windows().mInventoryMaximizedY,
                .mW = Settings::windows().mInventoryMaximizedW,
                .mH = Settings::windows().mInventoryMaximizedH,
            },
            .mIsMaximized = Settings::windows().mInventoryMaximized,
        };
    }

    WindowSettingValues makeInventoryBarterWindowSettingValues()
    {
        return WindowSettingValues{
            .mRegular = WindowRectSettingValues {
                .mX = Settings::windows().mInventoryBarterX,
                .mY = Settings::windows().mInventoryBarterY,
                .mW = Settings::windows().mInventoryBarterW,
                .mH = Settings::windows().mInventoryBarterH,
            },
            .mMaximized = WindowRectSettingValues {
                .mX = Settings::windows().mInventoryBarterMaximizedX,
                .mY = Settings::windows().mInventoryBarterMaximizedY,
                .mW = Settings::windows().mInventoryBarterMaximizedW,
                .mH = Settings::windows().mInventoryBarterMaximizedH,
            },
            .mIsMaximized = Settings::windows().mInventoryBarterMaximized,
        };
    }

    WindowSettingValues makeInventoryCompanionWindowSettingValues()
    {
        return WindowSettingValues{
            .mRegular = WindowRectSettingValues {
                .mX = Settings::windows().mInventoryCompanionX,
                .mY = Settings::windows().mInventoryCompanionY,
                .mW = Settings::windows().mInventoryCompanionW,
                .mH = Settings::windows().mInventoryCompanionH,
            },
            .mMaximized = WindowRectSettingValues {
                .mX = Settings::windows().mInventoryCompanionMaximizedX,
                .mY = Settings::windows().mInventoryCompanionMaximizedY,
                .mW = Settings::windows().mInventoryCompanionMaximizedW,
                .mH = Settings::windows().mInventoryCompanionMaximizedH,
            },
            .mIsMaximized = Settings::windows().mInventoryCompanionMaximized,
        };
    }

    WindowSettingValues makeInventoryContainerWindowSettingValues()
    {
        return WindowSettingValues{
            .mRegular = WindowRectSettingValues {
                .mX = Settings::windows().mInventoryContainerX,
                .mY = Settings::windows().mInventoryContainerY,
                .mW = Settings::windows().mInventoryContainerW,
                .mH = Settings::windows().mInventoryContainerH,
            },
            .mMaximized = WindowRectSettingValues {
                .mX = Settings::windows().mInventoryContainerMaximizedX,
                .mY = Settings::windows().mInventoryContainerMaximizedY,
                .mW = Settings::windows().mInventoryContainerMaximizedW,
                .mH = Settings::windows().mInventoryContainerMaximizedH,
            },
            .mIsMaximized = Settings::windows().mInventoryContainerMaximized,
        };
    }

    WindowSettingValues makeMapWindowSettingValues()
    {
        return WindowSettingValues{
            .mRegular = WindowRectSettingValues {
                .mX = Settings::windows().mMapX,
                .mY = Settings::windows().mMapY,
                .mW = Settings::windows().mMapW,
                .mH = Settings::windows().mMapH,
            },
            .mMaximized = WindowRectSettingValues {
                .mX = Settings::windows().mMapMaximizedX,
                .mY = Settings::windows().mMapMaximizedY,
                .mW = Settings::windows().mMapMaximizedW,
                .mH = Settings::windows().mMapMaximizedH,
            },
            .mIsMaximized = Settings::windows().mMapMaximized,
        };
    }

    WindowSettingValues makePostprocessorWindowSettingValues()
    {
        return WindowSettingValues{
            .mRegular = WindowRectSettingValues {
                .mX = Settings::windows().mPostprocessorX,
                .mY = Settings::windows().mPostprocessorY,
                .mW = Settings::windows().mPostprocessorW,
                .mH = Settings::windows().mPostprocessorH,
            },
            .mMaximized = WindowRectSettingValues {
                .mX = Settings::windows().mPostprocessorMaximizedX,
                .mY = Settings::windows().mPostprocessorMaximizedY,
                .mW = Settings::windows().mPostprocessorMaximizedW,
                .mH = Settings::windows().mPostprocessorMaximizedH,
            },
            .mIsMaximized = Settings::windows().mPostprocessorMaximized,
        };
    }

    WindowSettingValues makeSettingsWindowSettingValues()
    {
        return WindowSettingValues{
            .mRegular = WindowRectSettingValues {
                .mX = Settings::windows().mSettingsX,
                .mY = Settings::windows().mSettingsY,
                .mW = Settings::windows().mSettingsW,
                .mH = Settings::windows().mSettingsH,
            },
            .mMaximized = WindowRectSettingValues {
                .mX = Settings::windows().mSettingsMaximizedX,
                .mY = Settings::windows().mSettingsMaximizedY,
                .mW = Settings::windows().mSettingsMaximizedW,
                .mH = Settings::windows().mSettingsMaximizedH,
            },
            .mIsMaximized = Settings::windows().mSettingsMaximized,
        };
    }

    WindowSettingValues makeSpellsWindowSettingValues()
    {
        return WindowSettingValues{
            .mRegular = WindowRectSettingValues {
                .mX = Settings::windows().mSpellsX,
                .mY = Settings::windows().mSpellsY,
                .mW = Settings::windows().mSpellsW,
                .mH = Settings::windows().mSpellsH,
            },
            .mMaximized = WindowRectSettingValues {
                .mX = Settings::windows().mSpellsMaximizedX,
                .mY = Settings::windows().mSpellsMaximizedY,
                .mW = Settings::windows().mSpellsMaximizedW,
                .mH = Settings::windows().mSpellsMaximizedH,
            },
            .mIsMaximized = Settings::windows().mSpellsMaximized,
        };
    }

    WindowSettingValues makeStatsWindowSettingValues()
    {
        return WindowSettingValues{
            .mRegular = WindowRectSettingValues {
                .mX = Settings::windows().mStatsX,
                .mY = Settings::windows().mStatsY,
                .mW = Settings::windows().mStatsW,
                .mH = Settings::windows().mStatsH,
            },
            .mMaximized = WindowRectSettingValues {
                .mX = Settings::windows().mStatsMaximizedX,
                .mY = Settings::windows().mStatsMaximizedY,
                .mW = Settings::windows().mStatsMaximizedW,
                .mH = Settings::windows().mStatsMaximizedH,
            },
            .mIsMaximized = Settings::windows().mStatsMaximized,
        };
    }

}
