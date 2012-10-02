
#include "containerextensions.hpp"

#include <stdexcept>

#include <components/compiler/extensions.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>

#include "../mwbase/environment.hpp"

#include "../mwworld/manualref.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"

#include "interpretercontext.hpp"
#include "ref.hpp"

namespace
{
    std::string toLower (const std::string& name)
    {
        std::string lowerCase;

        std::transform (name.begin(), name.end(), std::back_inserter (lowerCase),
            (int(*)(int)) std::tolower);

        return lowerCase;
    }
}

namespace MWScript
{
    namespace Container
    {
        template<class R>
        class OpAddItem : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string item = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer count = runtime[0].mInteger;
                    runtime.pop();

                    if (count<0)
                        throw std::runtime_error ("second argument for AddItem must be non-negative");

                    MWWorld::ManualRef ref (MWBase::Environment::get().getWorld()->getStore(), item);

                    ref.getPtr().getRefData().setCount (count);

                    MWWorld::Class::get (ptr).getContainerStore (ptr).add (ref.getPtr());
                }
        };

        template<class R>
        class OpGetItemCount : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string item = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWWorld::ContainerStore& store = MWWorld::Class::get (ptr).getContainerStore (ptr);

                    Interpreter::Type_Integer sum = 0;

                    for (MWWorld::ContainerStoreIterator iter (store.begin()); iter!=store.end(); ++iter)
                        if (toLower(iter->getCellRef().mRefID) == toLower(item))
                            sum += iter->getRefData().getCount();

                    runtime.push (sum);
                }
        };

        template<class R>
        class OpRemoveItem : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string item = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer count = runtime[0].mInteger;
                    runtime.pop();

                    if (count<0)
                        throw std::runtime_error ("second argument for RemoveItem must be non-negative");

                    MWWorld::ContainerStore& store = MWWorld::Class::get (ptr).getContainerStore (ptr);

                    for (MWWorld::ContainerStoreIterator iter (store.begin()); iter!=store.end() && count;
                        ++iter)
                    {
                        if (toLower(iter->getCellRef().mRefID) == toLower(item))
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
             interpreter.installSegment5 (opcodeAddItem, new OpAddItem<ImplicitRef>);
             interpreter.installSegment5 (opcodeAddItemExplicit, new OpAddItem<ExplicitRef>);
             interpreter.installSegment5 (opcodeGetItemCount, new OpGetItemCount<ImplicitRef>);
             interpreter.installSegment5 (opcodeGetItemCountExplicit, new OpGetItemCount<ExplicitRef>);
             interpreter.installSegment5 (opcodeRemoveItem, new OpRemoveItem<ImplicitRef>);
             interpreter.installSegment5 (opcodeRemoveItemExplicit, new OpRemoveItem<ExplicitRef>);
        }
    }
}
