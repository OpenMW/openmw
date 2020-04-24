#ifndef MWGUI_JOURNALVIEWMODEL_HPP
#define MWGUI_JOURNALVIEWMODEL_HPP

#include <string>
#include <memory>
#include <functional>
#include <stdint.h>

#include <components/misc/utf8stream.hpp>

namespace MWGui
{
    /// View-Model for the journal GUI
    ///
    /// This interface defines an abstract data model suited
    ///  specifically to the needs of the journal GUI. It isolates
    /// the journal GUI from the implementation details of the
    /// game data store.
    struct JournalViewModel
    {
        typedef std::shared_ptr <JournalViewModel> Ptr;

        typedef intptr_t QuestId;
        typedef intptr_t TopicId;
        typedef uint8_t const * Utf8Point;
        typedef std::pair <Utf8Point, Utf8Point> Utf8Span;

        /// The base interface for both journal entries and topics.
        struct Entry
        {
            /// returns the body text for the journal entry
            ///
            /// This function returns a borrowed reference to the body of the
            /// journal entry. The returned reference becomes invalid when the
            /// entry is destroyed.
            virtual Utf8Span body () const = 0;

            /// Visits each subset of text in the body, delivering the beginning
            /// and end of the span relative to the body, and a valid topic ID if
            /// the span represents a keyword, or zero if not.
            virtual void visitSpans (std::function <void (TopicId, size_t, size_t)> visitor) const = 0;

            virtual ~Entry() = default;
        };

        /// An interface to topic data.
        struct TopicEntry : Entry
        {
            /// Returns a pre-formatted span of UTF8 encoded text representing
            /// the name of the NPC this portion of dialog was heard from.
            virtual Utf8Span source () const = 0;

            virtual ~TopicEntry() = default;
        };

        /// An interface to journal data.
        struct JournalEntry : Entry
        {
            /// Returns a pre-formatted span of UTF8 encoded text representing
            /// the in-game date this entry was added to the journal.
            virtual Utf8Span timestamp () const = 0;

            virtual ~JournalEntry() = default;
        };

        /// called prior to journal opening
        virtual void load () = 0;

        /// called prior to journal closing
        virtual void unload () = 0;

        /// returns true if their are no journal entries to display
        virtual bool isEmpty () const = 0;

        /// walks the active and optionally completed, quests providing the name and completed status
        virtual void visitQuestNames (bool active_only, std::function <void (const std::string&, bool)> visitor) const = 0;

        /// walks over the journal entries related to all quests with the given name
        /// If \a questName is empty, simply visits all journal entries
        virtual void visitJournalEntries (const std::string& questName, std::function <void (JournalEntry const &)> visitor) const = 0;

        /// provides the name of the topic specified by its id
        virtual void visitTopicName (TopicId topicId, std::function <void (Utf8Span)> visitor) const = 0;

        /// walks over the topics whose names start with the character
        virtual void visitTopicNamesStartingWith (Utf8Stream::UnicodeChar character, std::function < void (const std::string&) > visitor) const = 0;

        /// walks over the topic entries for the topic specified by its identifier
        virtual void visitTopicEntries (TopicId topicId, std::function <void (TopicEntry const &)> visitor) const = 0;

        // create an instance of the default journal view model implementation
        static Ptr create ();

        virtual ~JournalViewModel() = default;
    };
}

#endif // MWGUI_JOURNALVIEWMODEL_HPP
