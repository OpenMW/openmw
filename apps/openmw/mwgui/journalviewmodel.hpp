#ifndef MWGUI_JOURNALVIEWMODEL_HPP
#define MWGUI_JOURNALVIEWMODEL_HPP

#include <string>
#include <memory>
#include <functional>
#include <platform/stdint.h>

namespace MWGui
{
    /// View-Model for the journal GUI
    ///
    /// This interface defines an abstract data model suited
    //  specifically to the needs of the journal GUI. It isolates
    /// the journal GUI from the implementation details of the
    /// game data store.
    struct IJournalViewModel
    {
        typedef std::shared_ptr <IJournalViewModel> ptr;

        typedef intptr_t quest_id;
        typedef intptr_t topic_id;
        typedef uint8_t const * utf8_point;
        typedef std::pair <utf8_point, utf8_point> utf8_span;

        /// The base interface for both journal entries and topics.
        struct IEntry
        {
            /// returns the body text for the journal entry
            ///
            /// This function returns a borrowed reference to the body of the
            /// journal entry. The returned reference becomes invalid when the
            /// entry is destroyed.
            virtual utf8_span body () const = 0;

            /// Visits each subset of text in the body, delivering the beginning
            /// and end of the span relative to the body, and a valid topic ID if
            /// the span represents a keyword, or zero if not.
            virtual void visit_spans (std::function <void (topic_id, size_t, size_t)> visitor) const = 0;
        };

        /// An interface to topic data.
        struct ITopicEntry : IEntry
        {
            /// Returns a pre-formatted span of UTF8 encoded text representing
            /// the name of the NPC this portion of dialog was heard from.
            virtual utf8_span source () const = 0;
        };

        /// An interface to journal data.
        struct IJournalEntry : IEntry
        {
            /// Returns a pre-formatted span of UTF8 encoded text representing
            /// the in-game date this entry was added to the journal.
            virtual utf8_span timestamp () const = 0;
        };


        /// called prior to journal opening
        virtual void load () = 0;

        /// called prior to journal closing
        virtual void unload () = 0;

        /// returns true if their are no journal entries to display
        virtual bool is_empty () const = 0;

        /// provides access to the name of the quest with the specified identifier
        virtual void visit_quest_name (topic_id topicId, std::function <void (utf8_span)> visitor) const = 0;

        /// walks the active and optionally completed, quests providing the quest id and name
        virtual void visit_quest_names (bool active_only, std::function <void (quest_id, utf8_span)> visitor) const = 0;

        /// walks over the journal entries related to the specified quest identified by its id
        virtual void visit_journal_entries (quest_id questId, std::function <void (IJournalEntry const &)> visitor) const = 0;

        /// provides the name of the topic specified by its id
        virtual void visit_topic_name (topic_id topicId, std::function <void (utf8_span)> visitor) const = 0;

        /// walks over the topics whose names start with the specified character providing the topics id and name
        virtual void visit_topic_names_starting_with (int character, std::function < void (topic_id , utf8_span) > visitor) const = 0;

        /// walks over the topic entries for the topic specified by its identifier
        virtual void visit_topic_entries (topic_id topicId, std::function <void (ITopicEntry const &)> visitor) const = 0;

        // create an instance of the default journal view model implementation
        static ptr create ();
    };
}

#endif // MWGUI_JOURNALVIEWMODEL_HPP
