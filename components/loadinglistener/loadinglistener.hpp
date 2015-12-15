#ifndef COMPONENTS_LOADINGLISTENER_H
#define COMPONENTS_LOADINGLISTENER_H

#include <string>

namespace Loading
{
    class Listener
    {
    public:
        /// Set a text label to show on the loading screen.
        virtual void setLabel (const std::string& label) {}

        /// Start a loading sequence. Must call loadingOff() when done.
        /// @note To get the loading screen to actually update, you must call setProgress / increaseProgress periodically.
        /// @note It is best to use the ScopedLoad object instead of using loadingOn()/loadingOff() directly,
        ///  so that the loading is exception safe.
        virtual void loadingOn() {}
        virtual void loadingOff() {}

        /// Set the total range of progress (e.g. the number of objects to load).
        virtual void setProgressRange (size_t range) {}
        /// Set current progress. Valid range is [0, progressRange)
        virtual void setProgress (size_t value) {}
        /// Increase current progress, default by 1.
        virtual void increaseProgress (size_t increase = 1) {}
    };

    /// @brief Used for stopping a loading sequence when the object goes out of scope
    struct ScopedLoad
    {
        ScopedLoad(Listener* l) : mListener(l) { mListener->loadingOn(); }
        ~ScopedLoad() { mListener->loadingOff(); }
        Listener* mListener;
    };
}

#endif
