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

            ///get the faction of the actor you are talking with
            virtual std::string getFaction() const = 0;

            //calbacks for the GUI
            virtual void keywordSelected (const std::string& keyword) = 0;
            virtual void goodbyeSelected() = 0;
            virtual void questionAnswered (const std::string& answer) = 0;
    };
}

#endif
