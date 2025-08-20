#include "tes4.hpp"
#include "arguments.hpp"
#include "labels.hpp"

#include <array>
#include <fstream>
#include <iostream>
#include <type_traits>

#include <components/debug/writeflags.hpp>
#include <components/esm/esmcommon.hpp>
#include <components/esm/refid.hpp>
#include <components/esm/typetraits.hpp>
#include <components/esm4/reader.hpp>
#include <components/esm4/readerutils.hpp>
#include <components/esm4/records.hpp>
#include <components/esm4/typetraits.hpp>
#include <components/toutf8/toutf8.hpp>

namespace EsmTool
{
    namespace
    {
        struct Params
        {
            const bool mQuite;

            explicit Params(const Arguments& info)
                : mQuite(info.quiet_given || info.mode == "clone")
            {
            }
        };

        std::string toString(ESM4::GroupType type)
        {
            switch (type)
            {
                case ESM4::Grp_RecordType:
                    return "RecordType";
                case ESM4::Grp_WorldChild:
                    return "WorldChild";
                case ESM4::Grp_InteriorCell:
                    return "InteriorCell";
                case ESM4::Grp_InteriorSubCell:
                    return "InteriorSubCell";
                case ESM4::Grp_ExteriorCell:
                    return "ExteriorCell";
                case ESM4::Grp_ExteriorSubCell:
                    return "ExteriorSubCell";
                case ESM4::Grp_CellChild:
                    return "CellChild";
                case ESM4::Grp_TopicChild:
                    return "TopicChild";
                case ESM4::Grp_CellPersistentChild:
                    return "CellPersistentChild";
                case ESM4::Grp_CellTemporaryChild:
                    return "CellTemporaryChild";
                case ESM4::Grp_CellVisibleDistChild:
                    return "CellVisibleDistChild";
            }

            return "Unknown (" + std::to_string(type) + ")";
        }

        template <class T>
        struct WriteArray
        {
            std::string_view mPrefix;
            const T& mValue;

            explicit WriteArray(std::string_view prefix, const T& value)
                : mPrefix(prefix)
                , mValue(value)
            {
            }
        };

        template <class T>
        struct WriteData
        {
            const T& mValue;

            explicit WriteData(const T& value)
                : mValue(value)
            {
            }
        };

        template <class T>
        std::ostream& operator<<(std::ostream& stream, const WriteArray<T>& write)
        {
            for (const auto& value : write.mValue)
                stream << write.mPrefix << value;
            return stream;
        }

        template <class T>
        std::ostream& operator<<(std::ostream& stream, const WriteData<T>& /*write*/)
        {
            return stream << " ?";
        }

        std::ostream& operator<<(std::ostream& stream, const std::monostate&)
        {
            return stream << "[none]";
        }

        std::ostream& operator<<(std::ostream& stream, const WriteData<ESM4::GameSetting::Data>& write)
        {
            std::visit([&](const auto& v) { stream << v; }, write.mValue);
            return stream;
        }

        struct WriteCellFlags
        {
            std::uint16_t mValue;
        };

        using CellFlagString = Debug::FlagString<std::uint16_t>;

        constexpr std::array cellFlags{
            CellFlagString{ ESM4::CELL_Interior, "Interior" },
            CellFlagString{ ESM4::CELL_HasWater, "HasWater" },
            CellFlagString{ ESM4::CELL_NoTravel, "NoTravel" },
            CellFlagString{ ESM4::CELL_HideLand, "HideLand" },
            CellFlagString{ ESM4::CELL_Public, "Public" },
            CellFlagString{ ESM4::CELL_HandChgd, "HandChgd" },
            CellFlagString{ ESM4::CELL_QuasiExt, "QuasiExt" },
            CellFlagString{ ESM4::CELL_SkyLight, "SkyLight" },
        };

        std::ostream& operator<<(std::ostream& stream, const WriteCellFlags& write)
        {
            return Debug::writeFlags(stream, write.mValue, cellFlags);
        }

        template <class T>
        void readTypedRecord(const Params& params, ESM4::Reader& reader)
        {
            reader.getRecordData();

            T value;
            value.load(reader);

            if (params.mQuite)
                return;

            std::cout << "\n  Record: " << ESM::NAME(reader.hdr().record.typeId).toStringView();
            if constexpr (ESM::hasId<T>)
                std::cout << "\n  Id: " << value.mId;
            if constexpr (ESM4::hasFlags<T>)
                std::cout << "\n  Record flags: " << recordFlags(value.mFlags);
            if constexpr (ESM4::hasParent<T>)
                std::cout << "\n  Parent: " << value.mParent;
            if constexpr (ESM4::hasEditorId<T>)
                std::cout << "\n  EditorId: " << value.mEditorId;
            if constexpr (ESM4::hasFullName<T>)
                std::cout << "\n  FullName: " << value.mFullName;
            if constexpr (ESM4::hasCellFlags<T>)
                std::cout << "\n  CellFlags: " << WriteCellFlags{ value.mCellFlags };
            if constexpr (ESM4::hasX<T>)
                std::cout << "\n  X: " << value.mX;
            if constexpr (ESM4::hasY<T>)
                std::cout << "\n  Y: " << value.mY;
            if constexpr (ESM::hasModel<T>)
                std::cout << "\n  Model: " << value.mModel;
            if constexpr (ESM4::hasNif<T>)
                std::cout << "\n  Nif:" << WriteArray("\n  - ", value.mNif);
            if constexpr (ESM4::hasKf<T>)
                std::cout << "\n  Kf:" << WriteArray("\n  - ", value.mKf);
            if constexpr (ESM4::hasType<T>)
                std::cout << "\n  Type: " << value.mType;
            if constexpr (ESM4::hasValue<T>)
                std::cout << "\n  Value: " << value.mValue;
            if constexpr (ESM4::hasData<T>)
                std::cout << "\n  Data: " << WriteData(value.mData);
            std::cout << '\n';
        }

        bool readRecord(const Params& params, ESM4::Reader& reader)
        {
            switch (static_cast<ESM4::RecordTypes>(reader.hdr().record.typeId))
            {
                case ESM4::REC_AACT:
                    break;
                case ESM4::REC_ACHR:
                    readTypedRecord<ESM4::ActorCharacter>(params, reader);
                    return true;
                case ESM4::REC_ACRE:
                    readTypedRecord<ESM4::ActorCreature>(params, reader);
                    return true;
                case ESM4::REC_ACTI:
                    readTypedRecord<ESM4::Activator>(params, reader);
                    return true;
                case ESM4::REC_ADDN:
                    break;
                case ESM4::REC_ALCH:
                    readTypedRecord<ESM4::Potion>(params, reader);
                    return true;
                case ESM4::REC_ALOC:
                    readTypedRecord<ESM4::MediaLocationController>(params, reader);
                    return true;
                case ESM4::REC_AMMO:
                    readTypedRecord<ESM4::Ammunition>(params, reader);
                    return true;
                case ESM4::REC_ANIO:
                    readTypedRecord<ESM4::AnimObject>(params, reader);
                    return true;
                case ESM4::REC_APPA:
                    readTypedRecord<ESM4::Apparatus>(params, reader);
                    return true;
                case ESM4::REC_ARMA:
                    readTypedRecord<ESM4::ArmorAddon>(params, reader);
                    return true;
                case ESM4::REC_ARMO:
                    readTypedRecord<ESM4::Armor>(params, reader);
                    return true;
                case ESM4::REC_ARTO:
                    break;
                case ESM4::REC_ASPC:
                    readTypedRecord<ESM4::AcousticSpace>(params, reader);
                    return true;
                case ESM4::REC_ASTP:
                    break;
                case ESM4::REC_AVIF:
                    break;
                case ESM4::REC_BOOK:
                    readTypedRecord<ESM4::Book>(params, reader);
                    return true;
                case ESM4::REC_BPTD:
                    readTypedRecord<ESM4::BodyPartData>(params, reader);
                    return true;
                case ESM4::REC_CAMS:
                    break;
                case ESM4::REC_CCRD:
                    break;
                case ESM4::REC_CELL:
                    readTypedRecord<ESM4::Cell>(params, reader);
                    return true;
                case ESM4::REC_CLAS:
                    readTypedRecord<ESM4::Class>(params, reader);
                    return true;
                case ESM4::REC_CLFM:
                    readTypedRecord<ESM4::Colour>(params, reader);
                    return true;
                case ESM4::REC_CLMT:
                    break;
                case ESM4::REC_CLOT:
                    readTypedRecord<ESM4::Clothing>(params, reader);
                    return true;
                case ESM4::REC_CMNY:
                    break;
                case ESM4::REC_COBJ:
                    break;
                case ESM4::REC_COLL:
                    break;
                case ESM4::REC_CONT:
                    readTypedRecord<ESM4::Container>(params, reader);
                    return true;
                case ESM4::REC_CPTH:
                    break;
                case ESM4::REC_CREA:
                    readTypedRecord<ESM4::Creature>(params, reader);
                    return true;
                case ESM4::REC_CSTY:
                    break;
                case ESM4::REC_DEBR:
                    break;
                case ESM4::REC_DIAL:
                    readTypedRecord<ESM4::Dialogue>(params, reader);
                    return true;
                case ESM4::REC_DLBR:
                    break;
                case ESM4::REC_DLVW:
                    break;
                case ESM4::REC_DOBJ:
                    readTypedRecord<ESM4::DefaultObj>(params, reader);
                    return true;
                case ESM4::REC_DOOR:
                    readTypedRecord<ESM4::Door>(params, reader);
                    return true;
                case ESM4::REC_DUAL:
                    break;
                case ESM4::REC_ECZN:
                    break;
                case ESM4::REC_EFSH:
                    break;
                case ESM4::REC_ENCH:
                    break;
                case ESM4::REC_EQUP:
                    break;
                case ESM4::REC_EXPL:
                    break;
                case ESM4::REC_EYES:
                    readTypedRecord<ESM4::Eyes>(params, reader);
                    return true;
                case ESM4::REC_FACT:
                    break;
                case ESM4::REC_FLOR:
                    readTypedRecord<ESM4::Flora>(params, reader);
                    return true;
                case ESM4::REC_FLST:
                    readTypedRecord<ESM4::FormIdList>(params, reader);
                    return true;
                case ESM4::REC_FSTP:
                    break;
                case ESM4::REC_FSTS:
                    break;
                case ESM4::REC_FURN:
                    readTypedRecord<ESM4::Furniture>(params, reader);
                    return true;
                case ESM4::REC_GLOB:
                    readTypedRecord<ESM4::GlobalVariable>(params, reader);
                    return true;
                case ESM4::REC_GMST:
                    readTypedRecord<ESM4::GameSetting>(params, reader);
                    return true;
                case ESM4::REC_GRAS:
                    readTypedRecord<ESM4::Grass>(params, reader);
                    return true;
                case ESM4::REC_GRUP:
                    break;
                case ESM4::REC_HAIR:
                    readTypedRecord<ESM4::Hair>(params, reader);
                    return true;
                case ESM4::REC_HAZD:
                    break;
                case ESM4::REC_HDPT:
                    readTypedRecord<ESM4::HeadPart>(params, reader);
                    return true;
                case ESM4::REC_IDLE:
                    readTypedRecord<ESM4::IdleAnimation>(params, reader);
                    return true;
                    break;
                case ESM4::REC_IDLM:
                    readTypedRecord<ESM4::IdleMarker>(params, reader);
                    return true;
                case ESM4::REC_IMAD:
                    break;
                case ESM4::REC_IMGS:
                    break;
                case ESM4::REC_IMOD:
                    readTypedRecord<ESM4::ItemMod>(params, reader);
                    return true;
                case ESM4::REC_INFO:
                    readTypedRecord<ESM4::DialogInfo>(params, reader);
                    return true;
                case ESM4::REC_INGR:
                    readTypedRecord<ESM4::Ingredient>(params, reader);
                    return true;
                case ESM4::REC_IPCT:
                    break;
                case ESM4::REC_IPDS:
                    break;
                case ESM4::REC_KEYM:
                    readTypedRecord<ESM4::Key>(params, reader);
                    return true;
                case ESM4::REC_KYWD:
                    break;
                case ESM4::REC_LAND:
                    readTypedRecord<ESM4::Land>(params, reader);
                    return true;
                case ESM4::REC_LCRT:
                    break;
                case ESM4::REC_LCTN:
                    break;
                case ESM4::REC_LGTM:
                    readTypedRecord<ESM4::LightingTemplate>(params, reader);
                    return true;
                case ESM4::REC_LIGH:
                    readTypedRecord<ESM4::Light>(params, reader);
                    return true;
                case ESM4::REC_LSCR:
                    break;
                case ESM4::REC_LTEX:
                    readTypedRecord<ESM4::LandTexture>(params, reader);
                    return true;
                case ESM4::REC_LVLC:
                    readTypedRecord<ESM4::LevelledCreature>(params, reader);
                    return true;
                case ESM4::REC_LVLI:
                    readTypedRecord<ESM4::LevelledItem>(params, reader);
                    return true;
                case ESM4::REC_LVLN:
                    readTypedRecord<ESM4::LevelledNpc>(params, reader);
                    return true;
                case ESM4::REC_LVSP:
                    break;
                case ESM4::REC_MATO:
                    readTypedRecord<ESM4::Material>(params, reader);
                    return true;
                case ESM4::REC_MATT:
                    break;
                case ESM4::REC_MESG:
                    break;
                case ESM4::REC_MGEF:
                    break;
                case ESM4::REC_MISC:
                    readTypedRecord<ESM4::MiscItem>(params, reader);
                    return true;
                case ESM4::REC_MOVT:
                    break;
                case ESM4::REC_MSET:
                    readTypedRecord<ESM4::MediaSet>(params, reader);
                    return true;
                case ESM4::REC_MSTT:
                    readTypedRecord<ESM4::MovableStatic>(params, reader);
                    return true;
                case ESM4::REC_MUSC:
                    readTypedRecord<ESM4::Music>(params, reader);
                    return true;
                case ESM4::REC_MUST:
                    break;
                case ESM4::REC_NAVI:
                    readTypedRecord<ESM4::Navigation>(params, reader);
                    return true;
                case ESM4::REC_NAVM:
                    readTypedRecord<ESM4::NavMesh>(params, reader);
                    return true;
                case ESM4::REC_NOTE:
                    readTypedRecord<ESM4::Note>(params, reader);
                    return true;
                case ESM4::REC_NPC_:
                    readTypedRecord<ESM4::Npc>(params, reader);
                    return true;
                case ESM4::REC_OTFT:
                    readTypedRecord<ESM4::Outfit>(params, reader);
                    return true;
                case ESM4::REC_PACK:
                    readTypedRecord<ESM4::AIPackage>(params, reader);
                    return true;
                case ESM4::REC_PERK:
                    break;
                case ESM4::REC_PGRD:
                    readTypedRecord<ESM4::Pathgrid>(params, reader);
                    return true;
                case ESM4::REC_PGRE:
                    readTypedRecord<ESM4::PlacedGrenade>(params, reader);
                    return true;
                case ESM4::REC_PHZD:
                    break;
                case ESM4::REC_PROJ:
                    break;
                case ESM4::REC_PWAT:
                    readTypedRecord<ESM4::PlaceableWater>(params, reader);
                    return true;
                case ESM4::REC_QUST:
                    readTypedRecord<ESM4::Quest>(params, reader);
                    return true;
                case ESM4::REC_RACE:
                    readTypedRecord<ESM4::Race>(params, reader);
                    return true;
                case ESM4::REC_REFR:
                    readTypedRecord<ESM4::Reference>(params, reader);
                    return true;
                case ESM4::REC_REGN:
                    readTypedRecord<ESM4::Region>(params, reader);
                    return true;
                case ESM4::REC_RELA:
                    break;
                case ESM4::REC_REVB:
                    break;
                case ESM4::REC_RFCT:
                    break;
                case ESM4::REC_ROAD:
                    readTypedRecord<ESM4::Road>(params, reader);
                    return true;
                case ESM4::REC_SBSP:
                    readTypedRecord<ESM4::SubSpace>(params, reader);
                    return true;
                case ESM4::REC_SCEN:
                    break;
                case ESM4::REC_SCOL:
                    readTypedRecord<ESM4::StaticCollection>(params, reader);
                    return true;
                case ESM4::REC_SCPT:
                    readTypedRecord<ESM4::Script>(params, reader);
                    return true;
                case ESM4::REC_SCRL:
                    readTypedRecord<ESM4::Scroll>(params, reader);
                    return true;
                case ESM4::REC_SGST:
                    readTypedRecord<ESM4::SigilStone>(params, reader);
                    return true;
                case ESM4::REC_SHOU:
                    break;
                case ESM4::REC_SLGM:
                    readTypedRecord<ESM4::SoulGem>(params, reader);
                    return true;
                case ESM4::REC_SMBN:
                    break;
                case ESM4::REC_SMEN:
                    break;
                case ESM4::REC_SMQN:
                    break;
                case ESM4::REC_SNCT:
                    break;
                case ESM4::REC_SNDR:
                    readTypedRecord<ESM4::SoundReference>(params, reader);
                    return true;
                case ESM4::REC_SOPM:
                    break;
                case ESM4::REC_SOUN:
                    readTypedRecord<ESM4::Sound>(params, reader);
                    return true;
                case ESM4::REC_SPEL:
                    break;
                case ESM4::REC_SPGD:
                    break;
                case ESM4::REC_STAT:
                    readTypedRecord<ESM4::Static>(params, reader);
                    return true;
                case ESM4::REC_TACT:
                    readTypedRecord<ESM4::TalkingActivator>(params, reader);
                    return true;
                case ESM4::REC_TERM:
                    readTypedRecord<ESM4::Terminal>(params, reader);
                    return true;
                case ESM4::REC_TES4:
                    readTypedRecord<ESM4::Header>(params, reader);
                    return true;
                case ESM4::REC_TREE:
                    readTypedRecord<ESM4::Tree>(params, reader);
                    return true;
                case ESM4::REC_TXST:
                    readTypedRecord<ESM4::TextureSet>(params, reader);
                    return true;
                case ESM4::REC_VTYP:
                    break;
                case ESM4::REC_WATR:
                    break;
                case ESM4::REC_WEAP:
                    readTypedRecord<ESM4::Weapon>(params, reader);
                    return true;
                case ESM4::REC_WOOP:
                    break;
                case ESM4::REC_WRLD:
                    readTypedRecord<ESM4::World>(params, reader);
                    return true;
                case ESM4::REC_WTHR:
                    break;
            }

            if (!params.mQuite)
                std::cout << "\n  Unsupported record: " << ESM::NAME(reader.hdr().record.typeId).toStringView() << '\n';
            return false;
        }

    }

    int loadTes4(const Arguments& info, std::unique_ptr<std::ifstream>&& stream)
    {
        std::cout << "Loading TES4 file: " << info.filename << '\n';

        try
        {
            const ToUTF8::StatelessUtf8Encoder encoder(ToUTF8::calculateEncoding(info.encoding));
            ESM4::Reader reader(std::move(stream), info.filename, nullptr, &encoder, true);
            const Params params(info);

            if (!params.mQuite)
            {
                std::cout << "Author: " << reader.getAuthor() << '\n'
                          << "Description: " << reader.getDesc() << '\n'
                          << "File format version: " << reader.esmVersionF() << '\n';

                if (const std::vector<ESM::MasterData>& masterData = reader.getGameFiles(); !masterData.empty())
                {
                    std::cout << "Masters:" << '\n';
                    for (const auto& master : masterData)
                        std::cout << "  " << master.name << ", " << master.size << " bytes\n";
                }
            }

            auto visitorRec = [&params](ESM4::Reader& r) { return readRecord(params, r); };
            auto visitorGroup = [&params](ESM4::Reader& r) {
                if (params.mQuite)
                    return;
                auto groupType = static_cast<ESM4::GroupType>(r.hdr().group.type);
                std::cout << "\nGroup: " << toString(groupType) << " " << ESM::NAME(r.hdr().group.typeId).toStringView()
                          << '\n';
            };
            ESM4::ReaderUtils::readAll(reader, visitorRec, visitorGroup);
        }
        catch (const std::exception& e)
        {
            std::cout << "\nERROR:\n\n  " << e.what() << std::endl;
            return -1;
        }

        return 0;
    }
}
