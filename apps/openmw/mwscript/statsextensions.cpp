
#include "statsextensions.hpp"

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwworld/class.hpp"

#include "../mwmechanics/creaturestats.hpp"

#include "interpretercontext.hpp"

namespace MWScript
{
    namespace Stats
    {
        class OpGetAttribute : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpGetAttribute (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    MWWorld::Ptr ptr = context.getReference();

                    Interpreter::Type_Integer value =
                        MWWorld::Class::get (ptr).getCreatureStats (ptr).mAttributes[mIndex].
                        getModified();

                    runtime.push (value);
                }
        };

        class OpGetAttributeExplicit : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpGetAttributeExplicit (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getWorld().getPtr (id, false);

                    Interpreter::Type_Integer value =
                        MWWorld::Class::get (ptr).getCreatureStats (ptr).mAttributes[mIndex].
                        getModified();

                    runtime.push (value);
                }
        };

        class OpSetAttribute : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpSetAttribute (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getReference();

                    MWWorld::Class::get (ptr).getCreatureStats (ptr).mAttributes[mIndex].
                        setModified (value, 0);
                }
        };

        class OpSetAttributeExplicit : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpSetAttributeExplicit (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getWorld().getPtr (id, false);

                    MWWorld::Class::get (ptr).getCreatureStats (ptr).mAttributes[mIndex].
                        setModified (value, 0);
                }
        };

        class OpModAttribute : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpModAttribute (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getReference();

                    value += MWWorld::Class::get (ptr).getCreatureStats (ptr).mAttributes[mIndex].
                        getModified();

                    MWWorld::Class::get (ptr).getCreatureStats (ptr).mAttributes[mIndex].
                        setModified (value, 0, 100);
                }
        };

        class OpModAttributeExplicit : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpModAttributeExplicit (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getWorld().getPtr (id, false);

                    value +=
                        MWWorld::Class::get (ptr).getCreatureStats (ptr).mAttributes[mIndex].
                        getModified();

                    MWWorld::Class::get (ptr).getCreatureStats (ptr).mAttributes[mIndex].
                        setModified (value, 0, 100);
                }
        };

        class OpGetDynamic : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpGetDynamic (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    MWWorld::Ptr ptr = context.getReference();

                    if (mIndex==0 && MWWorld::Class::get (ptr).hasItemHealth (ptr))
                    {
                        // health is a special case
                        Interpreter::Type_Integer value =
                            MWWorld::Class::get (ptr).getItemMaxHealth (ptr);
                        runtime.push (value);

                        return;
                    }

                    Interpreter::Type_Integer value =
                        MWWorld::Class::get (ptr).getCreatureStats (ptr).mDynamic[mIndex].
                        getCurrent();

                    runtime.push (value);
                }
        };

        class OpGetDynamicExplicit : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpGetDynamicExplicit (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getWorld().getPtr (id, false);

                    if (mIndex==0 && MWWorld::Class::get (ptr).hasItemHealth (ptr))
                    {
                        // health is a special case
                        Interpreter::Type_Integer value =
                            MWWorld::Class::get (ptr).getItemMaxHealth (ptr);
                        runtime.push (value);

                        return;
                    }

                    Interpreter::Type_Integer value =
                        MWWorld::Class::get (ptr).getCreatureStats (ptr).mDynamic[mIndex].
                        getCurrent();

                    runtime.push (value);
                }
        };

        class OpSetDynamic : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpSetDynamic (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getReference();

                    MWWorld::Class::get (ptr).getCreatureStats (ptr).mDynamic[mIndex].
                        setModified (value, 0);
                }
        };

        class OpSetDynamicExplicit : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpSetDynamicExplicit (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getWorld().getPtr (id, false);

                    MWWorld::Class::get (ptr).getCreatureStats (ptr).mDynamic[mIndex].
                        setModified (value, 0);
                }
        };

        class OpModDynamic : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpModDynamic (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    Interpreter::Type_Integer diff = runtime[0].mInteger;
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getReference();

                    MWMechanics::CreatureStats& stats = MWWorld::Class::get (ptr).getCreatureStats (ptr);

                    Interpreter::Type_Integer current = stats.mDynamic[mIndex].getCurrent();

                    stats.mDynamic[mIndex].setModified (
                        diff + stats.mDynamic[mIndex].getModified(), 0);

                    stats.mDynamic[mIndex].setCurrent (diff + current);
                }
        };

        class OpModDynamicExplicit : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpModDynamicExplicit (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer diff = runtime[0].mInteger;
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getWorld().getPtr (id, false);

                    MWMechanics::CreatureStats& stats =
                        MWWorld::Class::get (ptr).getCreatureStats (ptr);

                    Interpreter::Type_Integer current = stats.mDynamic[mIndex].getCurrent();

                    stats.mDynamic[mIndex].setModified (
                        diff + stats.mDynamic[mIndex].getModified(), 0);

                    stats.mDynamic[mIndex].setCurrent (diff + current);
                }
        };


        class OpModCurrentDynamic : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpModCurrentDynamic (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    Interpreter::Type_Integer diff = runtime[0].mInteger;
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getReference();

                    MWMechanics::CreatureStats& stats = MWWorld::Class::get (ptr).getCreatureStats (ptr);

                    Interpreter::Type_Integer current = stats.mDynamic[mIndex].getCurrent();

                    stats.mDynamic[mIndex].setCurrent (diff + current);
                }
        };

        class OpModCurrentDynamicExplicit : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpModCurrentDynamicExplicit (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer diff = runtime[0].mInteger;
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getWorld().getPtr (id, false);

                    MWMechanics::CreatureStats& stats =
                        MWWorld::Class::get (ptr).getCreatureStats (ptr);

                    Interpreter::Type_Integer current = stats.mDynamic[mIndex].getCurrent();

                    stats.mDynamic[mIndex].setCurrent (diff + current);
                }
        };

        class OpGetDynamicGetRatio : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpGetDynamicGetRatio (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    MWWorld::Ptr ptr = context.getReference();

                    MWMechanics::CreatureStats& stats = MWWorld::Class::get (ptr).getCreatureStats (ptr);

                    Interpreter::Type_Float value = 0;

                    Interpreter::Type_Float max = stats.mDynamic[mIndex].getModified();

                    if (max>0)
                        value = stats.mDynamic[mIndex].getCurrent() / max;

                    runtime.push (value);
                }
        };

        class OpGetDynamicGetRatioExplicit : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpGetDynamicGetRatioExplicit (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getWorld().getPtr (id, false);

                    MWMechanics::CreatureStats& stats =
                        MWWorld::Class::get (ptr).getCreatureStats (ptr);

                    Interpreter::Type_Float value = 0;

                    Interpreter::Type_Float max = stats.mDynamic[mIndex].getModified();

                    if (max>0)
                        value = stats.mDynamic[mIndex].getCurrent() / max;

                    runtime.push (value);
                }
        };

        class OpGetSkill : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpGetSkill (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    MWWorld::Ptr ptr = context.getReference();

                    Interpreter::Type_Integer value =
                        MWWorld::Class::get (ptr).getNpcStats (ptr).mSkill[mIndex].
                        getModified();

                    runtime.push (value);
                }
        };

        class OpGetSkillExplicit : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpGetSkillExplicit (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getWorld().getPtr (id, false);

                    Interpreter::Type_Integer value =
                        MWWorld::Class::get (ptr).getNpcStats (ptr).mSkill[mIndex].
                        getModified();

                    runtime.push (value);
                }
        };

        class OpSetSkill : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpSetSkill (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getReference();

                    MWWorld::Class::get (ptr).getNpcStats (ptr).mSkill[mIndex].
                        setModified (value, 0);
                }
        };

        class OpSetSkillExplicit : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpSetSkillExplicit (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getWorld().getPtr (id, false);

                    MWWorld::Class::get (ptr).getNpcStats (ptr).mSkill[mIndex].
                        setModified (value, 0);
                }
        };

        class OpModSkill : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpModSkill (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getReference();

                    value += MWWorld::Class::get (ptr).getNpcStats (ptr).mSkill[mIndex].
                        getModified();

                    MWWorld::Class::get (ptr).getNpcStats (ptr).mSkill[mIndex].
                        setModified (value, 0, 100);
                }
        };

        class OpModSkillExplicit : public Interpreter::Opcode0
        {
                int mIndex;

            public:

                OpModSkillExplicit (int index) : mIndex (index) {}

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer value = runtime[0].mInteger;
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getWorld().getPtr (id, false);

                    value +=
                        MWWorld::Class::get (ptr).getNpcStats (ptr).mSkill[mIndex].
                        getModified();

                    MWWorld::Class::get (ptr).getNpcStats (ptr).mSkill[mIndex].
                        setModified (value, 0, 100);
                }
        };

        const int numberOfAttributes = 8;

        const int opcodeGetAttribute = 0x2000027;
        const int opcodeGetAttributeExplicit = 0x200002f;
        const int opcodeSetAttribute = 0x2000037;
        const int opcodeSetAttributeExplicit = 0x200003f;
        const int opcodeModAttribute = 0x2000047;
        const int opcodeModAttributeExplicit = 0x200004f;

        const int numberOfDynamics = 3;

        const int opcodeGetDynamic = 0x2000057;
        const int opcodeGetDynamicExplicit = 0x200005a;
        const int opcodeSetDynamic = 0x200005d;
        const int opcodeSetDynamicExplicit = 0x2000060;
        const int opcodeModDynamic = 0x2000063;
        const int opcodeModDynamicExplicit = 0x2000066;
        const int opcodeModCurrentDynamic = 0x2000069;
        const int opcodeModCurrentDynamicExplicit = 0x200006c;
        const int opcodeGetDynamicGetRatio = 0x200006f;
        const int opcodeGetDynamicGetRatioExplicit = 0x2000072;

        const int numberOfSkills = 27;

        const int opcodeGetSkill = 0x200008e;
        const int opcodeGetSkillExplicit = 0x20000a9;
        const int opcodeSetSkill = 0x20000c4;
        const int opcodeSetSkillExplicit = 0x20000df;
        const int opcodeModSkill = 0x20000fa;
        const int opcodeModSkillExplicit = 0x2000115;

        void registerExtensions (Compiler::Extensions& extensions)
        {
            static const char *attributes[numberOfAttributes] =
            {
                "strength", "intelligence", "willpower", "agility", "speed", "endurance",
                "personality", "luck"
            };

            static const char *dynamics[numberOfDynamics] =
            {
                "health", "magicka", "fatigue"
            };

            static const char *skills[numberOfSkills] =
            {
                "block", "armorer", "mediumarmor", "heavyarmor", "bluntweapon",
                "longblade", "axe", "spear", "athletics", "enchant", "destruction",
                "alteration", "illusion", "conjuration", "mysticism",
                "restoration", "alchemy", "unarmored", "security", "sneak",
                "acrobatics", "lightarmor", "shortblade", "marksman",
                "merchantile", "speechcraft", "handtohand"
            };

            std::string get ("get");
            std::string set ("set");
            std::string mod ("mod");
            std::string modCurrent ("modcurrent");
            std::string getRatio ("getratio");

            for (int i=0; i<numberOfAttributes; ++i)
            {
                extensions.registerFunction (get + attributes[i], 'l', "",
                    opcodeGetAttribute+i, opcodeGetAttributeExplicit+i);

                extensions.registerInstruction (set + attributes[i], "l",
                    opcodeSetAttribute+i, opcodeSetAttributeExplicit+i);

                extensions.registerInstruction (mod + attributes[i], "l",
                    opcodeModAttribute+i, opcodeModAttributeExplicit+i);
            }

            for (int i=0; i<numberOfDynamics; ++i)
            {
                extensions.registerFunction (get + dynamics[i], 'l', "",
                    opcodeGetDynamic+i, opcodeGetDynamicExplicit+i);

                extensions.registerInstruction (set + dynamics[i], "l",
                    opcodeSetDynamic+i, opcodeSetDynamicExplicit+i);

                extensions.registerInstruction (mod + dynamics[i], "l",
                    opcodeModDynamic+i, opcodeModDynamicExplicit+i);

                extensions.registerInstruction (modCurrent + dynamics[i], "l",
                    opcodeModCurrentDynamic+i, opcodeModCurrentDynamicExplicit+i);

                extensions.registerFunction (get + dynamics[i] + getRatio, 'f', "",
                    opcodeGetDynamicGetRatio+i, opcodeGetDynamicGetRatioExplicit+i);
            }

            for (int i=0; i<numberOfSkills; ++i)
            {
                extensions.registerFunction (get + skills[i], 'l', "",
                    opcodeGetSkill+i, opcodeGetSkillExplicit+i);

                extensions.registerInstruction (set + skills[i], "l",
                    opcodeSetSkill+i, opcodeSetSkillExplicit+i);

                extensions.registerInstruction (mod + skills[i], "l",
                    opcodeModSkill+i, opcodeModSkillExplicit+i);
            }
        }

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            for (int i=0; i<numberOfAttributes; ++i)
            {
                interpreter.installSegment5 (opcodeGetAttribute+i, new OpGetAttribute (i));
                interpreter.installSegment5 (opcodeGetAttributeExplicit+i,
                    new OpGetAttributeExplicit (i));

                interpreter.installSegment5 (opcodeSetAttribute+i, new OpSetAttribute (i));
                interpreter.installSegment5 (opcodeSetAttributeExplicit+i,
                    new OpSetAttributeExplicit (i));

                interpreter.installSegment5 (opcodeModAttribute+i, new OpModAttribute (i));
                interpreter.installSegment5 (opcodeModAttributeExplicit+i,
                    new OpModAttributeExplicit (i));
            }

            for (int i=0; i<numberOfDynamics; ++i)
            {
                interpreter.installSegment5 (opcodeGetDynamic+i, new OpGetDynamic (i));
                interpreter.installSegment5 (opcodeGetDynamicExplicit+i,
                    new OpGetDynamicExplicit (i));

                interpreter.installSegment5 (opcodeSetDynamic+i, new OpSetDynamic (i));
                interpreter.installSegment5 (opcodeSetDynamicExplicit+i,
                    new OpSetDynamicExplicit (i));

                interpreter.installSegment5 (opcodeModDynamic+i, new OpModDynamic (i));
                interpreter.installSegment5 (opcodeModDynamicExplicit+i,
                    new OpModDynamicExplicit (i));

                interpreter.installSegment5 (opcodeModCurrentDynamic+i, new OpModCurrentDynamic (i));
                interpreter.installSegment5 (opcodeModCurrentDynamicExplicit+i,
                    new OpModCurrentDynamicExplicit (i));

                interpreter.installSegment5 (opcodeGetDynamicGetRatio+i,
                    new OpGetDynamicGetRatio (i));
                interpreter.installSegment5 (opcodeGetDynamicGetRatioExplicit+i,
                    new OpGetDynamicGetRatioExplicit (i));
            }

            for (int i=0; i<numberOfSkills; ++i)
            {
                interpreter.installSegment5 (opcodeGetSkill+i, new OpGetSkill (i));
                interpreter.installSegment5 (opcodeGetSkillExplicit+i, new OpGetSkillExplicit (i));

                interpreter.installSegment5 (opcodeSetSkill+i, new OpSetSkill (i));
                interpreter.installSegment5 (opcodeSetSkillExplicit+i, new OpSetSkillExplicit (i));

                interpreter.installSegment5 (opcodeModSkill+i, new OpModSkill (i));
                interpreter.installSegment5 (opcodeModSkillExplicit+i, new OpModSkillExplicit (i));
            }
        }
    }
}
