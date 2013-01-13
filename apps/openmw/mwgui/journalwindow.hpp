#ifndef MWGUI_JOURNAL_H
#define MWGUI_JOURNAL_H

namespace MWBase { class WindowManager; }

namespace MWGui
{
    struct IJournalWindow
    {
        /// construct a new instance of the one JournalWindow implementation
        static IJournalWindow * create ();

        /// destroy this instance of the JournalWindow implementation
        virtual ~IJournalWindow () {};

        /// show/hide the journal window
        virtual void setVisible (bool newValue) = 0;
    };
}

#endif
