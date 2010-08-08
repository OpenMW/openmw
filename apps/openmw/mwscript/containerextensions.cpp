
#include "containerextensions.hpp"

#include <stdexcept>

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwworld/manualref.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/containerutil.hpp"

#include "interpretercontext.hpp"

namespace MWScript
{
    namespace Container
    {
        class OpAddItem : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string item = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer count = runtime[0].mInteger;
                    runtime.pop();

                    if (count<0)
                        throw std::runtime_error ("second argument for AddItem must be non-negative");

                    MWWorld::Ptr ptr = context.getReference();

                    MWWorld::ManualRef ref (context.getWorld().getStore(), item);

                    ref.getPtr().getRefData().setCount (count);

                    MWWorld::Class::get (ref.getPtr()).insertIntoContainer (ref.getPtr(),
                        MWWorld::Class::get (ptr).getContainerStore (ptr));
                }
        };

        class OpAddItemExplicit : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    std::string item = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer count = runtime[0].mInteger;
                    runtime.pop();

                    if (count<0)
                        throw std::runtime_error ("second argument for AddItem must be non-negative");

                    MWWorld::Ptr ptr = context.getWorld().getPtr (id, false);

                    MWWorld::ManualRef ref (context.getWorld().getStore(), item);

                    ref.getPtr().getRefData().setCount (count);

                    MWWorld::Class::get (ref.getPtr()).insertIntoContainer (ref.getPtr(),
                        MWWorld::Class::get (ptr).getContainerStore (ptr));
                }
        };

        class OpGetItemCount : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string item = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getReference();

                    std::vector<MWWorld::Ptr> list;

                    MWWorld::listItemsInContainer (item,
                        MWWorld::Class::get (ptr).getContainerStore (ptr),
                        context.getWorld().getStore(), list);

                    Interpreter::Type_Integer sum = 0;

                    for (std::vector<MWWorld::Ptr>::iterator iter (list.begin()); iter!=list.end();
                        ++iter)
                    {
                        sum += iter->getRefData().getCount();
                    }

                    runtime.push (sum);
                }
        };

        class OpGetItemCountExplicit : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    std::string item = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWWorld::Ptr ptr = context.getWorld().getPtr (id, false);

                    std::vector<MWWorld::Ptr> list;

                    MWWorld::listItemsInContainer (item,
                        MWWorld::Class::get (ptr).getContainerStore (ptr),
                        context.getWorld().getStore(), list);

                    Interpreter::Type_Integer sum = 0;

                    for (std::vector<MWWorld::Ptr>::iterator iter (list.begin()); iter!=list.end();
                        ++iter)
                    {
                        sum += iter->getRefData().getCount();
                    }

                    runtime.push (sum);
                }
        };

        class OpRemoveItem : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string item = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer count = runtime[0].mInteger;
                    runtime.pop();

                    if (count<0)
                        throw std::runtime_error ("second argument for RemoveItem must be non-negative");

                    MWWorld::Ptr ptr = context.getReference();

                    std::vector<MWWorld::Ptr> list;

                    MWWorld::listItemsInContainer (item,
                        MWWorld::Class::get (ptr).getContainerStore (ptr),
                        context.getWorld().getStore(), list);

                    for (std::vector<MWWorld::Ptr>::iterator iter (list.begin());
                        iter!=list.end() && count;
                        ++iter)
                    {
                        if (iter->getRefData().getCount()<=count)
                        {
                            count -= iter->getRefData().getCount();
                            iter->getRefData().setCount (0);
                        }
                        else
                        {
                            iter->getRefData().setCount (iter->getRefData().getCount()-count);
                            count = 0;
                        }
                    }

                    // To be fully compatible with original Morrowind, we would need to check if
                    // count is >= 0 here and throw an exception. But let's be tollerant instead.
                }
        };

        class OpRemoveItemExplicit : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWScript::InterpreterContext& context
                        = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

                    std::string id = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    std::string item = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer count = runtime[0].mInteger;
                    runtime.pop();

                    if (count<0)
                        throw std::runtime_error ("second argument for RemoveItem must be non-negative");

                    MWWorld::Ptr ptr = context.getWorld().getPtr (id, false);

                    std::vector<MWWorld::Ptr> list;

                    MWWorld::listItemsInContainer (item,
                        MWWorld::Class::get (ptr).getContainerStore (ptr),
                        context.getWorld().getStore(), list);

                    for (std::vector<MWWorld::Ptr>::iterator iter (list.begin());
                        iter!=list.end() && count;
                        ++iter)
                    {
                        if (iter->getRefData().getCount()<=count)
                        {
                            count -= iter->getRefData().getCount();
                            iter->getRefData().setCount (0);
                        }
                        else
                        {
                            iter->getRefData().setCount (iter->getRefData().getCount()-count);
                            count = 0;
                        }
                    }

                    // To be fully compatible with original Morrowind, we would need to check if
                    // count is >= 0 here and throw an exception. But let's be tollerant instead.
                }
        };

        const int opcodeAddItem = 0x2000076;
        const int opcodeAddItemExplicit = 0x2000077;
        const int opcodeGetItemCount = 0x2000078;
        const int opcodeGetItemCountExplicit = 0x2000079;
        const int opcodeRemoveItem = 0x200007a;
        const int opcodeRemoveItemExplicit = 0x200007b;

        void registerExtensions (Compiler::Extensions& extensions)
        {
            extensions.registerInstruction ("additem", "cl", opcodeAddItem, opcodeAddItemExplicit);
            extensions.registerFunction ("getitemcount", 'l', "c", opcodeGetItemCount,
                opcodeGetItemCountExplicit);
            extensions.registerInstruction ("removeitem", "cl", opcodeRemoveItem,
                opcodeRemoveItemExplicit);
        }

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
             interpreter.installSegment5 (opcodeAddItem, new OpAddItem);
             interpreter.installSegment5 (opcodeAddItemExplicit, new OpAddItemExplicit);
             interpreter.installSegment5 (opcodeGetItemCount, new OpGetItemCount);
             interpreter.installSegment5 (opcodeGetItemCountExplicit, new OpGetItemCountExplicit);
             interpreter.installSegment5 (opcodeRemoveItem, new OpRemoveItem);
             interpreter.installSegment5 (opcodeRemoveItemExplicit, new OpRemoveItemExplicit);
        }
    }
}
