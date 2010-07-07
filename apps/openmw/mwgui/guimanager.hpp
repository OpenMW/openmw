#ifndef GAME_SOUND_GUIMANAGER_H
#define GAME_SOUND_GUIMANAGER_H

namespace MWGui
{
    class GuiManager
    {
        public:
        
            void showBirthDialogue();
            ///< Birthsign (part of the character generation process)
            
            void showClassDialogue();
            ///< Class selection (part of the character generation process)

            void showNameDialogue();
            ///< Enter character name (part of the character generation process)
    
            void showRaceDialogue();
            ///< Race selection (part of the character generation process)
            
            void showReviewDialogue();
            ///< Character generation review (final part of the character generation process) 
    
            void enableInventoryWindow();
            ///< Initially disabled.
            
            void enableMagicWindow();
            ///< Initially disabled.

            void enableMapWindow();
            ///< Initially disabled.
    
            void enableStatsMenu();
            ///< Initially disabled.            
            
            void enableLevelUpDialogue();
            ///< Rest/Level-up. Initially disabled.
            
            void showRestDialogue();
            ///< Rest dialogue: ask player how many hours he wants to sleep.
            
            bool isGuiActive() const;
            ///< Any non-HUD GUI element active (dialogues and windows)?
    };
}

#endif
