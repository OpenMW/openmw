#include "loadinfo.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/debug/debuglog.hpp>
#include <components/misc/concepts.hpp>
#include <components/misc/strings/conversion.hpp>

namespace
{
    enum class SelectRuleStatus
    {
        Valid,
        Invalid,
        Ignorable
    };

    SelectRuleStatus isValidSelectRule(std::string_view rule)
    {
        if (rule.size() < 5)
            return SelectRuleStatus::Invalid;
        if (rule[4] < '0' || rule[4] > '5') // Comparison operators
            return SelectRuleStatus::Invalid;
        if (rule[1] == '1') // Function
        {
            int function = Misc::StringUtils::toNumeric<int>(rule.substr(2, 2), -1);
            if (function >= 0 && function <= 73)
                return SelectRuleStatus::Valid;
            return SelectRuleStatus::Invalid;
        }
        if (rule.size() == 5) // Missing ID
            return SelectRuleStatus::Invalid;
        if (rule[3] != 'X')
            return SelectRuleStatus::Ignorable;
        constexpr auto ignorable
            = [](bool valid) { return valid ? SelectRuleStatus::Valid : SelectRuleStatus::Ignorable; };
        switch (rule[1])
        {
            case '2':
            case '3':
            case 'C':
                return ignorable(rule[2] == 's' || rule[2] == 'l' || rule[2] == 'f');
            case '4':
                return ignorable(rule[2] == 'J');
            case '5':
                return ignorable(rule[2] == 'I');
            case '6':
                return ignorable(rule[2] == 'D');
            case '7':
                return ignorable(rule[2] == 'X');
            case '8':
                return ignorable(rule[2] == 'F');
            case '9':
                return ignorable(rule[2] == 'C');
            case 'A':
                return ignorable(rule[2] == 'R');
            case 'B':
                return ignorable(rule[2] == 'L');
            default:
                return SelectRuleStatus::Invalid;
        }
    }
}

namespace ESM
{
    template <Misc::SameAsWithoutCvref<DialInfo::DATAstruct> T>
    void decompose(T&& v, const auto& f)
    {
        char padding = 0;
        f(v.mType, v.mDisposition, v.mRank, v.mGender, v.mPCrank, padding);
    }

    void DialInfo::load(ESMReader& esm, bool& isDeleted)
    {
        mId = esm.getHNRefId("INAM");

        isDeleted = false;

        mQuestStatus = QS_None;
        mFactionLess = false;

        mPrev = esm.getHNRefId("PNAM");
        mNext = esm.getHNRefId("NNAM");

        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case fourCC("DATA"):
                    esm.getSubComposite(mData);
                    break;
                case fourCC("ONAM"):
                    mActor = esm.getRefId();
                    break;
                case fourCC("RNAM"):
                    mRace = esm.getRefId();
                    break;
                case fourCC("CNAM"):
                    mClass = esm.getRefId();
                    break;
                case fourCC("FNAM"):
                {
                    mFaction = esm.getRefId();
                    if (mFaction == "FFFF")
                    {
                        mFactionLess = true;
                    }
                    break;
                }
                case fourCC("ANAM"):
                    mCell = esm.getRefId();
                    break;
                case fourCC("DNAM"):
                    mPcFaction = esm.getRefId();
                    break;
                case fourCC("SNAM"):
                    mSound = esm.getHString();
                    break;
                case SREC_NAME:
                    mResponse = esm.getHString();
                    break;
                case fourCC("SCVR"):
                {
                    SelectStruct ss;
                    ss.mSelectRule = esm.getHString();
                    ss.mValue.read(esm, Variant::Format_Info);
                    auto valid = isValidSelectRule(ss.mSelectRule);
                    if (ss.mValue.getType() != VT_Int && ss.mValue.getType() != VT_Float)
                        valid = SelectRuleStatus::Invalid;
                    if (valid == SelectRuleStatus::Invalid)
                        Log(Debug::Warning) << "Skipping invalid SCVR for INFO " << mId;
                    else
                    {
                        mSelects.push_back(ss);
                        if (valid == SelectRuleStatus::Ignorable)
                            Log(Debug::Info)
                                << "Found malformed SCVR for INFO " << mId << " at index " << ss.mSelectRule[0];
                    }
                    break;
                }
                case fourCC("BNAM"):
                    mResultScript = esm.getHString();
                    break;
                case fourCC("QSTN"):
                    mQuestStatus = QS_Name;
                    esm.skipRecord();
                    break;
                case fourCC("QSTF"):
                    mQuestStatus = QS_Finished;
                    esm.skipRecord();
                    break;
                case fourCC("QSTR"):
                    mQuestStatus = QS_Restart;
                    esm.skipRecord();
                    break;
                case SREC_DELE:
                    esm.skipHSub();
                    isDeleted = true;
                    break;
                default:
                    esm.fail("Unknown subrecord");
                    break;
            }
        }
    }

    void DialInfo::save(ESMWriter& esm, bool isDeleted) const
    {
        esm.writeHNCRefId("INAM", mId);
        esm.writeHNCRefId("PNAM", mPrev);
        esm.writeHNCRefId("NNAM", mNext);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeNamedComposite("DATA", mData);
        esm.writeHNOCRefId("ONAM", mActor);
        esm.writeHNOCRefId("RNAM", mRace);
        esm.writeHNOCRefId("CNAM", mClass);
        esm.writeHNOCRefId("FNAM", mFaction);
        esm.writeHNOCRefId("ANAM", mCell);
        esm.writeHNOCRefId("DNAM", mPcFaction);
        esm.writeHNOCString("SNAM", mSound);
        esm.writeHNOString("NAME", mResponse);

        for (std::vector<SelectStruct>::const_iterator it = mSelects.begin(); it != mSelects.end(); ++it)
        {
            esm.writeHNString("SCVR", it->mSelectRule);
            it->mValue.write(esm, Variant::Format_Info);
        }

        esm.writeHNOString("BNAM", mResultScript);

        switch (mQuestStatus)
        {
            case QS_Name:
                esm.writeHNT("QSTN", '\1');
                break;
            case QS_Finished:
                esm.writeHNT("QSTF", '\1');
                break;
            case QS_Restart:
                esm.writeHNT("QSTR", '\1');
                break;
            default:
                break;
        }
    }

    void DialInfo::blank()
    {
        mData = {};

        mSelects.clear();
        mPrev = ESM::RefId();
        mNext = ESM::RefId();
        mActor = ESM::RefId();
        mRace = ESM::RefId();
        mClass = ESM::RefId();
        mFaction = ESM::RefId();
        mPcFaction = ESM::RefId();
        mCell = ESM::RefId();
        mSound.clear();
        mResponse.clear();
        mResultScript.clear();
        mFactionLess = false;
        mQuestStatus = QS_None;
    }
}
