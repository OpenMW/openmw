#ifndef INTERPRETER_LOCALOPCODES_H_INCLUDED
#define INTERPRETER_LOCALOPCODES_H_INCLUDED

#include "opcodes.hpp"
#include "runtime.hpp"
#include "context.hpp"

namespace Interpreter
{
    class OpStoreLocalShort : public Opcode0
    {
        public:

            void execute (Runtime& runtime) override
            {
                Type_Integer data = runtime[0].mInteger;
                int index = runtime[1].mInteger;

                runtime.getContext().setLocalShort (index, data);

                runtime.pop();
                runtime.pop();
            }
    };

    class OpStoreLocalLong : public Opcode0
    {
        public:

            void execute (Runtime& runtime) override
            {
                Type_Integer data = runtime[0].mInteger;
                int index = runtime[1].mInteger;

                runtime.getContext().setLocalLong (index, data);

                runtime.pop();
                runtime.pop();
            }
    };

    class OpStoreLocalFloat : public Opcode0
    {
        public:

            void execute (Runtime& runtime) override
            {
                Type_Float data = runtime[0].mFloat;
                int index = runtime[1].mInteger;

                runtime.getContext().setLocalFloat (index, data);

                runtime.pop();
                runtime.pop();
            }
    };

    class OpFetchIntLiteral : public Opcode0
    {
        public:

            void execute (Runtime& runtime) override
            {
                Type_Integer intValue = runtime.getIntegerLiteral (runtime[0].mInteger);
                runtime[0].mInteger = intValue;
            }
    };

    class OpFetchFloatLiteral : public Opcode0
    {
        public:

            void execute (Runtime& runtime) override
            {
                Type_Float floatValue = runtime.getFloatLiteral (runtime[0].mInteger);
                runtime[0].mFloat = floatValue;
            }
    };

    class OpFetchLocalShort : public Opcode0
    {
        public:

            void execute (Runtime& runtime) override
            {
                int index = runtime[0].mInteger;
                int value = runtime.getContext().getLocalShort (index);
                runtime[0].mInteger = value;
            }
    };

    class OpFetchLocalLong : public Opcode0
    {
        public:

            void execute (Runtime& runtime) override
            {
                int index = runtime[0].mInteger;
                int value = runtime.getContext().getLocalLong (index);
                runtime[0].mInteger = value;
            }
    };

    class OpFetchLocalFloat : public Opcode0
    {
        public:

            void execute (Runtime& runtime) override
            {
                int index = runtime[0].mInteger;
                float value = runtime.getContext().getLocalFloat (index);
                runtime[0].mFloat = value;
            }
    };

    class OpStoreGlobalShort : public Opcode0
    {
        public:

            void execute (Runtime& runtime) override
            {
                Type_Integer data = runtime[0].mInteger;
                int index = runtime[1].mInteger;

                std::string name = runtime.getStringLiteral (index);

                runtime.getContext().setGlobalShort (name, data);

                runtime.pop();
                runtime.pop();
            }
    };

    class OpStoreGlobalLong : public Opcode0
    {
        public:

            void execute (Runtime& runtime) override
            {
                Type_Integer data = runtime[0].mInteger;
                int index = runtime[1].mInteger;

                std::string name = runtime.getStringLiteral (index);

                runtime.getContext().setGlobalLong (name, data);

                runtime.pop();
                runtime.pop();
            }
    };

    class OpStoreGlobalFloat : public Opcode0
    {
        public:

            void execute (Runtime& runtime) override
            {
                Type_Float data = runtime[0].mFloat;
                int index = runtime[1].mInteger;

                std::string name = runtime.getStringLiteral (index);

                runtime.getContext().setGlobalFloat (name, data);

                runtime.pop();
                runtime.pop();
            }
    };

    class OpFetchGlobalShort : public Opcode0
    {
        public:

            void execute (Runtime& runtime) override
            {
                int index = runtime[0].mInteger;
                std::string name = runtime.getStringLiteral (index);
                Type_Integer value = runtime.getContext().getGlobalShort (name);
                runtime[0].mInteger = value;
            }
    };

    class OpFetchGlobalLong : public Opcode0
    {
        public:

            void execute (Runtime& runtime) override
            {
                int index = runtime[0].mInteger;
                std::string name = runtime.getStringLiteral (index);
                Type_Integer value = runtime.getContext().getGlobalLong (name);
                runtime[0].mInteger = value;
            }
    };

    class OpFetchGlobalFloat : public Opcode0
    {
        public:

            void execute (Runtime& runtime) override
            {
                int index = runtime[0].mInteger;
                std::string name = runtime.getStringLiteral (index);
                Type_Float value = runtime.getContext().getGlobalFloat (name);
                runtime[0].mFloat = value;
            }
    };

    class OpStoreMemberShort : public Opcode0
    {
            bool mGlobal;

        public:

            OpStoreMemberShort (bool global) : mGlobal (global) {}

            void execute (Runtime& runtime) override
            {
                Type_Integer data = runtime[0].mInteger;
                Type_Integer index = runtime[1].mInteger;
                std::string id = runtime.getStringLiteral (index);
                index = runtime[2].mInteger;
                std::string variable = runtime.getStringLiteral (index);

                runtime.getContext().setMemberShort (id, variable, data, mGlobal);

                runtime.pop();
                runtime.pop();
                runtime.pop();
            }
    };

    class OpStoreMemberLong : public Opcode0
    {
            bool mGlobal;

        public:

            OpStoreMemberLong (bool global) : mGlobal (global) {}

            void execute (Runtime& runtime) override
            {
                Type_Integer data = runtime[0].mInteger;
                Type_Integer index = runtime[1].mInteger;
                std::string id = runtime.getStringLiteral (index);
                index = runtime[2].mInteger;
                std::string variable = runtime.getStringLiteral (index);

                runtime.getContext().setMemberLong (id, variable, data, mGlobal);

                runtime.pop();
                runtime.pop();
                runtime.pop();
            }
    };

    class OpStoreMemberFloat : public Opcode0
    {
            bool mGlobal;

        public:

            OpStoreMemberFloat (bool global) : mGlobal (global) {}

            void execute (Runtime& runtime) override
            {
                Type_Float data = runtime[0].mFloat;
                Type_Integer index = runtime[1].mInteger;
                std::string id = runtime.getStringLiteral (index);
                index = runtime[2].mInteger;
                std::string variable = runtime.getStringLiteral (index);

                runtime.getContext().setMemberFloat (id, variable, data, mGlobal);

                runtime.pop();
                runtime.pop();
                runtime.pop();
            }
    };

    class OpFetchMemberShort : public Opcode0
    {
            bool mGlobal;

        public:

            OpFetchMemberShort (bool global) : mGlobal (global) {}

            void execute (Runtime& runtime) override
            {
                Type_Integer index = runtime[0].mInteger;
                std::string id = runtime.getStringLiteral (index);
                index = runtime[1].mInteger;
                std::string variable = runtime.getStringLiteral (index);
                runtime.pop();

                int value = runtime.getContext().getMemberShort (id, variable, mGlobal);
                runtime[0].mInteger = value;
            }
    };

    class OpFetchMemberLong : public Opcode0
    {
            bool mGlobal;

        public:

            OpFetchMemberLong (bool global) : mGlobal (global) {}

            void execute (Runtime& runtime) override
            {
                Type_Integer index = runtime[0].mInteger;
                std::string id = runtime.getStringLiteral (index);
                index = runtime[1].mInteger;
                std::string variable = runtime.getStringLiteral (index);
                runtime.pop();

                int value = runtime.getContext().getMemberLong (id, variable, mGlobal);
                runtime[0].mInteger = value;
            }
    };

    class OpFetchMemberFloat : public Opcode0
    {
            bool mGlobal;

        public:

            OpFetchMemberFloat (bool global) : mGlobal (global) {}

            void execute (Runtime& runtime) override
            {
                Type_Integer index = runtime[0].mInteger;
                std::string id = runtime.getStringLiteral (index);
                index = runtime[1].mInteger;
                std::string variable = runtime.getStringLiteral (index);
                runtime.pop();

                float value = runtime.getContext().getMemberFloat (id, variable, mGlobal);
                runtime[0].mFloat = value;
            }
    };
}

#endif
