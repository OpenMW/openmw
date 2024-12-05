#include "dialoguecondition.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "variant.hpp"

#include <components/debug/debuglog.hpp>
#include <components/misc/concepts.hpp>
#include <components/misc/strings/conversion.hpp>

namespace ESM
{
    std::optional<DialogueCondition> DialogueCondition::load(ESMReader& esm, ESM::RefId context)
    {
        std::string rule = esm.getHString();
        ESM::Variant variant;
        variant.read(esm, Variant::Format_Info);
        if (rule.size() < 5)
        {
            Log(Debug::Warning) << "Found invalid SCVR rule of size " << rule.size() << " in INFO " << context;
            return {};
        }
        if (rule[4] < '0' || rule[4] > '5')
        {
            Log(Debug::Warning) << "Found invalid SCVR comparison operator " << static_cast<int>(rule[4]) << " in INFO "
                                << context;
            return {};
        }
        DialogueCondition condition;
        if (rule[0] >= '0' && rule[0] <= '9')
            condition.mIndex = rule[0] - '0';
        else
        {
            Log(Debug::Info) << "Found invalid SCVR index " << static_cast<int>(rule[0]) << " in INFO " << context;
            condition.mIndex = 0;
        }
        if (rule[1] == '1')
        {
            int function = Misc::StringUtils::toNumeric<int>(std::string_view{ rule }.substr(2, 2), -1);
            if (function >= Function_FacReactionLowest && function <= Function_PcWerewolfKills)
                condition.mFunction = static_cast<Function>(function);
            else
            {
                Log(Debug::Warning) << "Encountered invalid SCVR function index " << function << " in INFO " << context;
                return {};
            }
        }
        else if ((rule[1] > '1' && rule[1] <= '9') || (rule[1] >= 'A' && rule[1] <= 'C'))
        {
            if (rule.size() == 5)
            {
                Log(Debug::Warning) << "Missing variable for SCVR of type " << rule[1] << " in INFO " << context;
                return {};
            }
            bool malformed = rule[3] != 'X';
            if (rule[1] == '2')
            {
                condition.mFunction = Function_Global;
                malformed |= rule[2] != 'f' && rule[2] != 'l' && rule[2] != 's';
            }
            else if (rule[1] == '3')
            {
                condition.mFunction = Function_Local;
                malformed |= rule[2] != 'f' && rule[2] != 'l' && rule[2] != 's';
            }
            else if (rule[1] == '4')
            {
                condition.mFunction = Function_Journal;
                malformed |= rule[2] != 'J';
            }
            else if (rule[1] == '5')
            {
                condition.mFunction = Function_Item;
                malformed |= rule[2] != 'I';
            }
            else if (rule[1] == '6')
            {
                condition.mFunction = Function_Dead;
                malformed |= rule[2] != 'D';
            }
            else if (rule[1] == '7')
            {
                condition.mFunction = Function_NotId;
                malformed |= rule[2] != 'X';
            }
            else if (rule[1] == '8')
            {
                condition.mFunction = Function_NotFaction;
                malformed |= rule[2] != 'F';
            }
            else if (rule[1] == '9')
            {
                condition.mFunction = Function_NotClass;
                malformed |= rule[2] != 'C';
            }
            else if (rule[1] == 'A')
            {
                condition.mFunction = Function_NotRace;
                malformed |= rule[2] != 'R';
            }
            else if (rule[1] == 'B')
            {
                condition.mFunction = Function_NotCell;
                malformed |= rule[2] != 'L';
            }
            else if (rule[1] == 'C')
            {
                condition.mFunction = Function_NotLocal;
                malformed |= rule[2] != 'f' && rule[2] != 'l' && rule[2] != 's';
            }
            if (malformed)
                Log(Debug::Info) << "Found malformed SCVR rule in INFO " << context;
        }
        else
        {
            Log(Debug::Warning) << "Found invalid SCVR function " << static_cast<int>(rule[1]) << " in INFO "
                                << context;
            return {};
        }
        condition.mComparison = static_cast<Comparison>(rule[4]);
        condition.mVariable = rule.substr(5);
        if (variant.getType() == VT_Int)
            condition.mValue = variant.getInteger();
        else if (variant.getType() == VT_Float)
            condition.mValue = variant.getFloat();
        else
        {
            Log(Debug::Warning) << "Found invalid SCVR variant " << variant.getType() << " in INFO " << context;
            return {};
        }
        return condition;
    }

    void DialogueCondition::save(ESMWriter& esm) const
    {
        auto variant = std::visit([](auto value) { return ESM::Variant(value); }, mValue);
        if (variant.getType() != VT_Float)
            variant.setType(VT_Int);
        std::string rule;
        rule.reserve(5 + mVariable.size());
        rule += static_cast<char>(mIndex + '0');
        const auto appendVariableType = [&]() {
            if (variant.getType() == VT_Float)
                rule += "fX";
            else
            {
                int32_t value = variant.getInteger();
                if (static_cast<int16_t>(value) == value)
                    rule += "sX";
                else
                    rule += "lX";
            }
        };
        if (mFunction == Function_Global)
        {
            rule += '2';
            appendVariableType();
        }
        else if (mFunction == Function_Local)
        {
            rule += '3';
            appendVariableType();
        }
        else if (mFunction == Function_Journal)
            rule += "4JX";
        else if (mFunction == Function_Item)
            rule += "5IX";
        else if (mFunction == Function_Dead)
            rule += "6DX";
        else if (mFunction == Function_NotId)
            rule += "7XX";
        else if (mFunction == Function_NotFaction)
            rule += "8FX";
        else if (mFunction == Function_NotClass)
            rule += "9CX";
        else if (mFunction == Function_NotRace)
            rule += "ARX";
        else if (mFunction == Function_NotCell)
            rule += "BLX";
        else if (mFunction == Function_NotLocal)
        {
            rule += 'C';
            appendVariableType();
        }
        else
        {
            rule += "100";
            char* start = rule.data() + rule.size();
            char* end = start;
            if (mFunction < Function_PcStrength)
                start--;
            else
                start -= 2;
            auto result = std::to_chars(start, end, static_cast<int>(mFunction));
            if (result.ec != std::errc())
            {
                Log(Debug::Error) << "Failed to save SCVR rule";
                return;
            }
        }
        rule += static_cast<char>(mComparison);
        rule += mVariable;
        esm.writeHNString("SCVR", rule);
        variant.write(esm, Variant::Format_Info);
    }
}
