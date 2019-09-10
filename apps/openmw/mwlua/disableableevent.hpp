#pragma once

namespace mwse
{
    namespace lua
    {
        namespace event
        {
            template <typename T>
            class DisableableEvent {
            public:
                // FIXME: seems to does not work with ISO C++
                static bool getEventEnabled() { return true; } //{ return m_EventEnabled; }
                static void setEventEnabled(bool enabled) {} //{ m_EventEnabled = enabled; }

            //protected:
            //  static bool m_EventEnabled;
            };
        }
    }
}
