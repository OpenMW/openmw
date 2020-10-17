#ifndef INTERPRETER_MISCOPCODES_H_INCLUDED
#define INTERPRETER_MISCOPCODES_H_INCLUDED

#include <stdexcept>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

#include "opcodes.hpp"
#include "runtime.hpp"
#include "defines.hpp"

#include <components/misc/messageformatparser.hpp>

namespace Interpreter
{
    class RuntimeMessageFormatter : public Misc::MessageFormatParser
    {
        private:
            std::string mFormattedMessage;
            Runtime& mRuntime;

        protected:
            void visitedPlaceholder(Placeholder placeholder, char padding, int width, int precision, Notation notation) override
            {
                std::ostringstream out;
                out.fill(padding);
                if (width != -1)
                    out.width(width);
                if (precision != -1)
                    out.precision(precision);

                switch (placeholder)
                {
                    case StringPlaceholder:
                        {
                            int index = mRuntime[0].mInteger;
                            mRuntime.pop();

                            out << mRuntime.getStringLiteral(index);
                            mFormattedMessage += out.str();
                        }
                        break;
                    case IntegerPlaceholder:
                        {
                            Type_Integer value = mRuntime[0].mInteger;
                            mRuntime.pop();

                            out << value;
                            mFormattedMessage += out.str();
                        }
                        break;
                    case FloatPlaceholder:
                        {
                            float value = mRuntime[0].mFloat;
                            mRuntime.pop();

                            if (notation == FixedNotation)
                            {
                                out << std::fixed << value;
                                mFormattedMessage += out.str();
                            }
                            else if (notation == ShortestNotation)
                            {
                                out << value;
                                std::string standard = out.str();

                                out.str(std::string());
                                out.clear();

                                out << std::scientific << value;
                                std::string scientific = out.str();

                                mFormattedMessage += standard.length() < scientific.length() ? standard : scientific;
                            }
                            else
                            {
                                out << std::scientific << value;
                                mFormattedMessage += out.str();
                            }
                        }
                        break;
                    default:
                        break;
                }
            }

            void visitedCharacter(char c) override
            {
                mFormattedMessage += c;
            }

        public:
            RuntimeMessageFormatter(Runtime& runtime)
                : mRuntime(runtime)
            {
            }

            void process(const std::string& message) override
            {
                mFormattedMessage.clear();
                MessageFormatParser::process(message);
            }

            std::string getFormattedMessage() const
            {
                return mFormattedMessage;
            }
    };

    inline std::string formatMessage (const std::string& message, Runtime& runtime)
    {
        RuntimeMessageFormatter formatter(runtime);
        formatter.process(message);

        std::string formattedMessage = formatter.getFormattedMessage();
        formattedMessage = fixDefinesMsgBox(formattedMessage, runtime.getContext());
        return formattedMessage;
    }

    class OpMessageBox : public Opcode1
    {
        public:

            void execute (Runtime& runtime, unsigned int arg0) override
            {
                // message
                int index = runtime[0].mInteger;
                runtime.pop();
                std::string message = runtime.getStringLiteral (index);

                // buttons
                std::vector<std::string> buttons;

                for (std::size_t i=0; i<arg0; ++i)
                {
                    index = runtime[0].mInteger;
                    runtime.pop();
                    buttons.push_back (runtime.getStringLiteral (index));
                }

                std::reverse (buttons.begin(), buttons.end());

                // handle additional parameters
                std::string formattedMessage = formatMessage (message, runtime);

                runtime.getContext().messageBox (formattedMessage, buttons);
            }
    };

    class OpReport : public Opcode0
    {
        public:

            void execute (Runtime& runtime) override
            {
                // message
                int index = runtime[0].mInteger;
                runtime.pop();
                std::string message = runtime.getStringLiteral (index);

                // handle additional parameters
                std::string formattedMessage = formatMessage (message, runtime);

                runtime.getContext().report (formattedMessage);
            }
    };

}

#endif
