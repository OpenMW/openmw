#ifndef INTERPRETER_CONTEXT_H_INCLUDED
#define INTERPRETER_CONTEXT_H_INCLUDED

#include <string>
#include <string_view>
#include <vector>

namespace Interpreter
{
    class Context
    {
        public:

            virtual ~Context() {}

            virtual std::string getTarget() const = 0;

            virtual int getLocalShort (int index) const = 0;

            virtual int getLocalLong (int index) const = 0;

            virtual float getLocalFloat (int index) const = 0;

            virtual void setLocalShort (int index, int value) = 0;

            virtual void setLocalLong (int index, int value) = 0;

            virtual void setLocalFloat (int index, float value) = 0;

            virtual void messageBox (const std::string& message,
                const std::vector<std::string>& buttons) = 0;

            void messageBox (const std::string& message)
            {
                std::vector<std::string> empty;
                messageBox (message, empty);
            }

            virtual void report (const std::string& message) = 0;

            virtual int getGlobalShort(std::string_view name) const = 0;

            virtual int getGlobalLong(std::string_view name) const = 0;

            virtual float getGlobalFloat(std::string_view name) const = 0;

            virtual void setGlobalShort(std::string_view name, int value) = 0;

            virtual void setGlobalLong(std::string_view name, int value) = 0;

            virtual void setGlobalFloat(std::string_view name, float value) = 0;

            virtual std::vector<std::string> getGlobals () const = 0;

            virtual char getGlobalType(std::string_view name) const = 0;

            virtual std::string getActionBinding(std::string_view action) const = 0;

            virtual std::string getActorName() const = 0;

            virtual std::string getNPCRace() const = 0;

            virtual std::string getNPCClass() const = 0;

            virtual std::string getNPCFaction() const = 0;

            virtual std::string getNPCRank() const = 0;

            virtual std::string getPCName() const = 0;

            virtual std::string getPCRace() const = 0;

            virtual std::string getPCClass() const = 0;

            virtual std::string getPCRank() const = 0;

            virtual std::string getPCNextRank() const = 0;

            virtual int getPCBounty() const = 0;

            virtual std::string getCurrentCellName() const = 0;

            virtual int getMemberShort(std::string_view id, std::string_view name, bool global) const = 0;

            virtual int getMemberLong(std::string_view id, std::string_view name, bool global) const = 0;

            virtual float getMemberFloat(std::string_view id, std::string_view name, bool global) const = 0;

            virtual void setMemberShort(std::string_view id, std::string_view name, int value, bool global) = 0;

            virtual void setMemberLong(std::string_view id, std::string_view name, int value, bool global) = 0;

            virtual void setMemberFloat(std::string_view id, std::string_view name, float value, bool global)
                = 0;
    };
}

#endif
