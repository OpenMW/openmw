#ifndef GAME_MWBASE_DIALOGUEMANAGER_H
#define GAME_MWBASE_DIALOGUEMANAGER_H

#include <string>

namespace MWWorld
{
    class Ptr;
}

namespace MWBase
{
    /// \brief Interface for dialogue manager (implemented in MWDialogue)
    class DialogueManager
    {
            DialogueManager (const DialogueManager&);
            ///< not implemented

            DialogueManager& operator= (const DialogueManager&);
            ///< not implemented

        public:

            DialogueManager() {}

            virtual ~DialogueManager() {}

            virtual void startDialogue (const MWWorld::Ptr& actor) = 0;

            virtual void addTopic (const std::string& topic) = 0;

            virtual void askQuestion (const std::string& question,int choice) = 0;

            virtual void goodbye() = 0;

            virtual MWWorld::Ptr getActor() const = 0;
            ///< Return the actor the player is currently talking to.

            //calbacks for the GUI
            virtual void keywordSelected (const std::string& keyword) = 0;
            virtual void goodbyeSelected() = 0;
            virtual void questionAnswered (const std::string& answer) = 0;

            virtual bool checkServiceRefused () = 0;

            virtual void persuade (int type) = 0;
            virtual int getTemporaryDispositionChange () const = 0;
            virtual void applyTemporaryDispositionChange (int delta) = 0;
    };
}

#endif
