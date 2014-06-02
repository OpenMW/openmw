#ifndef COMPONENTS_LOADINGLISTENER_H
#define COMPONENTS_LOADINGLISTENER_H

namespace Loading
{
    class Listener
    {
    public:
        virtual void setLabel (const std::string& label) = 0;

        // Use ScopedLoad instead of using these directly
        virtual void loadingOn() = 0;
        virtual void loadingOff() = 0;

        /// Indicate that some progress has been made, without specifying how much
        virtual void indicateProgress () = 0;

        virtual void setProgressRange (size_t range) = 0;
        virtual void setProgress (size_t value) = 0;
        virtual void increaseProgress (size_t increase = 1) = 0;
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
