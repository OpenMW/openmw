#ifndef COMPONENTS_LOADINGLISTENER_H
#define COMPONENTS_LOADINGLISTENER_H

namespace Loading
{
    class Listener
    {
    public:
        virtual void setLabel (const std::string& label) {}

        // Use ScopedLoad instead of using these directly
        virtual void loadingOn() {}
        virtual void loadingOff() {}

        /// Indicate that some progress has been made, without specifying how much
        virtual void indicateProgress () {}

        virtual void setProgressRange (size_t range) {}
        virtual void setProgress (size_t value) {}
        virtual void increaseProgress (size_t increase = 1) {}
    };

    // Used for stopping a loading sequence when the object goes out of scope
    struct ScopedLoad
    {
        ScopedLoad(Listener* l) : mListener(l) { mListener->loadingOn(); }
        ~ScopedLoad() { mListener->loadingOff(); }
        Listener* mListener;
    };
}

#endif
