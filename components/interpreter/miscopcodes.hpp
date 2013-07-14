#ifndef INTERPRETER_MISCOPCODES_H_INCLUDED
#define INTERPRETER_MISCOPCODES_H_INCLUDED

#include <cstdlib>
#include <stdexcept>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

#include "opcodes.hpp"
#include "runtime.hpp"
#include "defines.hpp"

namespace Interpreter
{
    inline std::string formatMessage (const std::string& message, Runtime& runtime)
    {
        std::string formattedMessage;

        for (std::size_t i=0; i<message.size(); ++i)
        {
            char c = message[i];

            if (c!='%')
                formattedMessage += c;
            else
            {
                ++i;
                if (i<message.size())
                {
                    c = message[i];

                    if (c=='S' || c=='s')
                    {
                        int index = runtime[0].mInteger;
                        runtime.pop();
                        formattedMessage += runtime.getStringLiteral (index);
                    }
                    else if (c=='g' || c=='G')
                    {
                        Type_Integer value = runtime[0].mInteger;
                        runtime.pop();

                        std::ostringstream out;
                        out << value;
                        formattedMessage += out.str();
                    }
                    else if (c=='f' || c=='F' || c=='.')
                    {
                        while (c!='f' && i<message.size())
                        {
                            ++i;
                        }

                        float value = runtime[0].mFloat;
                        runtime.pop();

                        std::ostringstream out;
                        out << value;
                        formattedMessage += out.str();
                    }
                    else if (c=='%')
                        formattedMessage += "%";
                    else
                    {
                        formattedMessage += "%";
                        formattedMessage += c;
                    }
                }
            }
        }
        
        formattedMessage = fixDefinesMsgBox(formattedMessage, runtime.getContext());
        return formattedMessage;
    }

    class OpMessageBox : public Opcode1
    {
        public:

            virtual void execute (Runtime& runtime, unsigned int arg0)
            {
                // message
                int index = runtime[0].mInteger;
                runtime.pop();
                std::string message = runtime.getStringLiteral (index);

                // buttons
                std::vector<std::string> buttons;

                for (std::size_t i=0; i<arg0; ++i)
                {
                    int index = runtime[0].mInteger;
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

            virtual void execute (Runtime& runtime)
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

    class OpMenuMode : public Opcode0
    {
        public:

            virtual void execute (Runtime& runtime)
            {
                runtime.push (runtime.getContext().menuMode());
            }
    };

    class OpRandom : public Opcode0
    {
        public:

            virtual void execute (Runtime& runtime)
            {
                double r = static_cast<double> (std::rand()) / RAND_MAX; // [0, 1)

                Type_Integer limit = runtime[0].mInteger;

                if (limit<0)
                    throw std::runtime_error (
                        "random: argument out of range (Don't be so negative!)");

                Type_Integer value = static_cast<Type_Integer> (r*limit); // [o, limit)

                runtime[0].mInteger = value;
            }
    };

    class OpGetSecondsPassed : public Opcode0
    {
        public:

            virtual void execute (Runtime& runtime)
            {
                Type_Float duration = runtime.getContext().getSecondsPassed();

                runtime.push (duration);
            }
    };

    class OpEnable : public Opcode0
    {
        public:

            virtual void execute (Runtime& runtime)
            {
                runtime.getContext().enable();
            }
    };

    class OpDisable : public Opcode0
    {
        public:

            virtual void execute (Runtime& runtime)
            {
                runtime.getContext().disable();
            }
    };

    class OpGetDisabled : public Opcode0
    {
        public:

            virtual void execute (Runtime& runtime)
            {
                runtime.push (runtime.getContext().isDisabled());
            }
    };

    class OpEnableExplicit : public Opcode0
    {
        public:

            virtual void execute (Runtime& runtime)
            {
                int index = runtime[0].mInteger;
                runtime.pop();
                std::string id = runtime.getStringLiteral (index);

                runtime.getContext().enable (id);
            }
    };

    class OpDisableExplicit : public Opcode0
    {
        public:

            virtual void execute (Runtime& runtime)
            {
                int index = runtime[0].mInteger;
                runtime.pop();
                std::string id = runtime.getStringLiteral (index);

                runtime.getContext().disable (id);
            }
    };

    class OpGetDisabledExplicit : public Opcode0
    {
        public:

            virtual void execute (Runtime& runtime)
            {
                int index = runtime[0].mInteger;
                runtime.pop();
                std::string id = runtime.getStringLiteral (index);

                runtime.push (runtime.getContext().isDisabled (id));
            }
    };

}

#endif
