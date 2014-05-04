#ifndef GAME_MWBASE_DIALOGUEMANAGER_H
#define GAME_MWBASE_DIALOGUEMANAGER_H

#include <string>

#include <stdint.h>

namespace Loading
{
    class Listener;
}

namespace ESM
{
    class ESMReader;
    class ESMWriter;
}

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

            virtual void clear() = 0;

            virtual ~DialogueManager() {}

            virtual bool isInChoice() const = 0;

            virtual void startDialogue (const MWWorld::Ptr& actor) = 0;

            virtual void addTopic (const std::string& topic) = 0;

            virtual void askQuestion (const std::string& question,int choice) = 0;

            virtual void goodbye() = 0;

            virtual void say(const MWWorld::Ptr &actor, const std::string &topic) const = 0;

            //calbacks for the GUI
            virtual void keywordSelected (const std::string& keyword) = 0;
            virtual void goodbyeSelected() = 0;
            virtual void questionAnswered (int answer) = 0;

            virtual bool checkServiceRefused () = 0;

            virtual void persuade (int type) = 0;
            virtual int getTemporaryDispositionChange () const = 0;
            virtual void applyDispositionChange (int delta) = 0;

            virtual int countSavedGameRecords() const = 0;

            virtual void write (ESM::ESMWriter& writer, Loading::Listener& progress) const = 0;

            virtual void readRecord (ESM::ESMReader& reader, int32_t type) = 0;
    };
}

#endif
