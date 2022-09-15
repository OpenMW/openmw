#include "tes4.hpp"
#include "arguments.hpp"
#include "labels.hpp"

#include <iostream>
#include <type_traits>

#include <components/esm/esmcommon.hpp>
#include <components/esm4/reader.hpp>
#include <components/esm4/records.hpp>

namespace EsmTool
{
    namespace
    {
        struct Params
        {
            const bool mQuite;

            explicit Params(const Arguments& info)
                : mQuite(info.quiet_given || info.mode == "clone")
            {}
        };

        std::string toString(ESM4::GroupType type)
        {
            switch (type)
            {
                case ESM4::Grp_RecordType: return "RecordType";
                case ESM4::Grp_WorldChild: return "WorldChild";
                case ESM4::Grp_InteriorCell: return "InteriorCell";
                case ESM4::Grp_InteriorSubCell: return "InteriorSubCell";
                case ESM4::Grp_ExteriorCell: return "ExteriorCell";
                case ESM4::Grp_ExteriorSubCell: return "ExteriorSubCell";
                case ESM4::Grp_CellChild: return "CellChild";
                case ESM4::Grp_TopicChild: return "TopicChild";
                case ESM4::Grp_CellPersistentChild: return "CellPersistentChild";
                case ESM4::Grp_CellTemporaryChild: return "CellTemporaryChild";
                case ESM4::Grp_CellVisibleDistChild: return "CellVisibleDistChild";
            }

            return "Unknown (" + std::to_string(type) + ")";
        }

        template <class T, class = std::void_t<>>
        struct HasFormId : std::false_type {};

        template <class T>
        struct HasFormId<T, std::void_t<decltype(T::mFormId)>> : std::true_type {};

        template <class T>
        constexpr bool hasFormId = HasFormId<T>::value;

        template <class T, class = std::void_t<>>
        struct HasFlags : std::false_type {};

        template <class T>
        struct HasFlags<T, std::void_t<decltype(T::mFlags)>> : std::true_type {};

        template <class T>
        constexpr bool hasFlags = HasFlags<T>::value;

        template <class T>
        void readTypedRecord(const Params& params, ESM4::Reader& reader)
        {
            reader.getRecordData();

            T value;
            value.load(reader);

            if (params.mQuite)
                return;

            std::cout << "\n  Record: " << ESM::NAME(reader.hdr().record.typeId).toStringView();
            if constexpr (hasFormId<T>)
                std::cout << ' ' << value.mFormId;
            if constexpr (hasFlags<T>)
                std::cout << "\n  Record flags: " << recordFlags(value.mFlags);
            std::cout << '\n';
        }

        void readRecord(const Params& params, ESM4::Reader& reader)
        {
            switch (static_cast<ESM4::RecordTypes>(reader.hdr().record.typeId))
            {
                case ESM4::REC_AACT: break;
                case ESM4::REC_ACHR: return readTypedRecord<ESM4::ActorCharacter>(params, reader);
                case ESM4::REC_ACRE: return readTypedRecord<ESM4::ActorCreature>(params, reader);
                case ESM4::REC_ACTI: return readTypedRecord<ESM4::Activator>(params, reader);
                case ESM4::REC_ADDN: break;
                case ESM4::REC_ALCH: return readTypedRecord<ESM4::Potion>(params, reader);
                case ESM4::REC_ALOC: return readTypedRecord<ESM4::MediaLocationController>(params, reader);
                case ESM4::REC_AMMO: return readTypedRecord<ESM4::Ammunition>(params, reader);
                case ESM4::REC_ANIO: return readTypedRecord<ESM4::AnimObject>(params, reader);
                case ESM4::REC_APPA: return readTypedRecord<ESM4::Apparatus>(params, reader);
                case ESM4::REC_ARMA: return readTypedRecord<ESM4::ArmorAddon>(params, reader);
                case ESM4::REC_ARMO: return readTypedRecord<ESM4::Armor>(params, reader);
                case ESM4::REC_ARTO: break;
                case ESM4::REC_ASPC: return readTypedRecord<ESM4::AcousticSpace>(params, reader);
                case ESM4::REC_ASTP: break;
                case ESM4::REC_AVIF: break;
                case ESM4::REC_BOOK: return readTypedRecord<ESM4::Book>(params, reader);
                case ESM4::REC_BPTD: return readTypedRecord<ESM4::BodyPartData>(params, reader);
                case ESM4::REC_CAMS: break;
                case ESM4::REC_CCRD: break;
                case ESM4::REC_CELL: return readTypedRecord<ESM4::Cell>(params, reader);
                case ESM4::REC_CLAS: return readTypedRecord<ESM4::Class>(params, reader);
                case ESM4::REC_CLFM: return readTypedRecord<ESM4::Colour>(params, reader);
                case ESM4::REC_CLMT: break;
                case ESM4::REC_CLOT: return readTypedRecord<ESM4::Clothing>(params, reader);
                case ESM4::REC_CMNY: break;
                case ESM4::REC_COBJ: break;
                case ESM4::REC_COLL: break;
                case ESM4::REC_CONT: return readTypedRecord<ESM4::Container>(params, reader);
                case ESM4::REC_CPTH: break;
                case ESM4::REC_CREA: return readTypedRecord<ESM4::Creature>(params, reader);
                case ESM4::REC_CSTY: break;
                case ESM4::REC_DEBR: break;
                case ESM4::REC_DIAL: return readTypedRecord<ESM4::Dialogue>(params, reader);
                case ESM4::REC_DLBR: break;
                case ESM4::REC_DLVW: break;
                case ESM4::REC_DOBJ: return readTypedRecord<ESM4::DefaultObj>(params, reader);
                case ESM4::REC_DOOR: return readTypedRecord<ESM4::Door>(params, reader);
                case ESM4::REC_DUAL: break;
                case ESM4::REC_ECZN: break;
                case ESM4::REC_EFSH: break;
                case ESM4::REC_ENCH: break;
                case ESM4::REC_EQUP: break;
                case ESM4::REC_EXPL: break;
                case ESM4::REC_EYES: return readTypedRecord<ESM4::Eyes>(params, reader);
                case ESM4::REC_FACT: break;
                case ESM4::REC_FLOR: return readTypedRecord<ESM4::Flora>(params, reader);
                case ESM4::REC_FLST: return readTypedRecord<ESM4::FormIdList>(params, reader);
                case ESM4::REC_FSTP: break;
                case ESM4::REC_FSTS: break;
                case ESM4::REC_FURN: return readTypedRecord<ESM4::Furniture>(params, reader);
                case ESM4::REC_GLOB: return readTypedRecord<ESM4::GlobalVariable>(params, reader);
                case ESM4::REC_GMST: break;
                case ESM4::REC_GRAS: return readTypedRecord<ESM4::Grass>(params, reader);
                case ESM4::REC_GRUP: break;
                case ESM4::REC_HAIR: return readTypedRecord<ESM4::Hair>(params, reader);
                case ESM4::REC_HAZD: break;
                case ESM4::REC_HDPT: return readTypedRecord<ESM4::HeadPart>(params, reader);
                case ESM4::REC_IDLE:
                    // FIXME: ESM4::IdleAnimation::load does not work with Oblivion.esm
                    // return readTypedRecord<ESM4::IdleAnimation>(params, reader);
                    break;
                case ESM4::REC_IDLM: return readTypedRecord<ESM4::IdleMarker>(params, reader);
                case ESM4::REC_IMAD: break;
                case ESM4::REC_IMGS: break;
                case ESM4::REC_IMOD: return readTypedRecord<ESM4::ItemMod>(params, reader);
                case ESM4::REC_INFO: return readTypedRecord<ESM4::DialogInfo>(params, reader);
                case ESM4::REC_INGR: return readTypedRecord<ESM4::Ingredient>(params, reader);
                case ESM4::REC_IPCT: break;
                case ESM4::REC_IPDS: break;
                case ESM4::REC_KEYM: return readTypedRecord<ESM4::Key>(params, reader);
                case ESM4::REC_KYWD: break;
                case ESM4::REC_LAND: return readTypedRecord<ESM4::Land>(params, reader);
                case ESM4::REC_LCRT: break;
                case ESM4::REC_LCTN: break;
                case ESM4::REC_LGTM: return readTypedRecord<ESM4::LightingTemplate>(params, reader);
                case ESM4::REC_LIGH: return readTypedRecord<ESM4::Light>(params, reader);
                case ESM4::REC_LSCR: break;
                case ESM4::REC_LTEX: return readTypedRecord<ESM4::LandTexture>(params, reader);
                case ESM4::REC_LVLC: return readTypedRecord<ESM4::LevelledCreature>(params, reader);
                case ESM4::REC_LVLI: return readTypedRecord<ESM4::LevelledItem>(params, reader);
                case ESM4::REC_LVLN: return readTypedRecord<ESM4::LevelledNpc>(params, reader);
                case ESM4::REC_LVSP: break;
                case ESM4::REC_MATO: return readTypedRecord<ESM4::Material>(params, reader);
                case ESM4::REC_MATT: break;
                case ESM4::REC_MESG: break;
                case ESM4::REC_MGEF: break;
                case ESM4::REC_MISC: return readTypedRecord<ESM4::MiscItem>(params, reader);
                case ESM4::REC_MOVT: break;
                case ESM4::REC_MSET: return readTypedRecord<ESM4::MediaSet>(params, reader);
                case ESM4::REC_MSTT: return readTypedRecord<ESM4::MovableStatic>(params, reader);
                case ESM4::REC_MUSC: return readTypedRecord<ESM4::Music>(params, reader);
                case ESM4::REC_MUST: break;
                case ESM4::REC_NAVI: return readTypedRecord<ESM4::Navigation>(params, reader);
                case ESM4::REC_NAVM: return readTypedRecord<ESM4::NavMesh>(params, reader);
                case ESM4::REC_NOTE: return readTypedRecord<ESM4::Note>(params, reader);
                case ESM4::REC_NPC_: return readTypedRecord<ESM4::Npc>(params, reader);
                case ESM4::REC_OTFT: return readTypedRecord<ESM4::Outfit>(params, reader);
                case ESM4::REC_PACK: return readTypedRecord<ESM4::AIPackage>(params, reader);
                case ESM4::REC_PERK: break;
                case ESM4::REC_PGRD: return readTypedRecord<ESM4::Pathgrid>(params, reader);
                case ESM4::REC_PGRE: return readTypedRecord<ESM4::PlacedGrenade>(params, reader);
                case ESM4::REC_PHZD: break;
                case ESM4::REC_PROJ: break;
                case ESM4::REC_PWAT: return readTypedRecord<ESM4::PlaceableWater>(params, reader);
                case ESM4::REC_QUST: return readTypedRecord<ESM4::Quest>(params, reader);
                case ESM4::REC_RACE: return readTypedRecord<ESM4::Race>(params, reader);
                case ESM4::REC_REFR: return readTypedRecord<ESM4::Reference>(params, reader);
                case ESM4::REC_REGN: return readTypedRecord<ESM4::Region>(params, reader);
                case ESM4::REC_RELA: break;
                case ESM4::REC_REVB: break;
                case ESM4::REC_RFCT: break;
                case ESM4::REC_ROAD: return readTypedRecord<ESM4::Road>(params, reader);
                case ESM4::REC_SBSP: return readTypedRecord<ESM4::SubSpace>(params, reader);
                case ESM4::REC_SCEN: break;
                case ESM4::REC_SCOL: return readTypedRecord<ESM4::StaticCollection>(params, reader);
                case ESM4::REC_SCPT: return readTypedRecord<ESM4::Script>(params, reader);
                case ESM4::REC_SCRL: return readTypedRecord<ESM4::Scroll>(params, reader);
                case ESM4::REC_SGST: return readTypedRecord<ESM4::SigilStone>(params, reader);
                case ESM4::REC_SHOU: break;
                case ESM4::REC_SLGM: return readTypedRecord<ESM4::SoulGem>(params, reader);
                case ESM4::REC_SMBN: break;
                case ESM4::REC_SMEN: break;
                case ESM4::REC_SMQN: break;
                case ESM4::REC_SNCT: break;
                case ESM4::REC_SNDR: return readTypedRecord<ESM4::SoundReference>(params, reader);
                case ESM4::REC_SOPM: break;
                case ESM4::REC_SOUN: return readTypedRecord<ESM4::Sound>(params, reader);
                case ESM4::REC_SPEL: break;
                case ESM4::REC_SPGD: break;
                case ESM4::REC_STAT: return readTypedRecord<ESM4::Static>(params, reader);
                case ESM4::REC_TACT: return readTypedRecord<ESM4::TalkingActivator>(params, reader);
                case ESM4::REC_TERM: return readTypedRecord<ESM4::Terminal>(params, reader);
                case ESM4::REC_TES4: return readTypedRecord<ESM4::Header>(params, reader);
                case ESM4::REC_TREE: return readTypedRecord<ESM4::Tree>(params, reader);
                case ESM4::REC_TXST: return readTypedRecord<ESM4::TextureSet>(params, reader);
                case ESM4::REC_VTYP: break;
                case ESM4::REC_WATR: break;
                case ESM4::REC_WEAP: return readTypedRecord<ESM4::Weapon>(params, reader);
                case ESM4::REC_WOOP: break;
                case ESM4::REC_WRLD: return readTypedRecord<ESM4::World>(params, reader);
                case ESM4::REC_WTHR: break;
            }

            if (!params.mQuite)
                std::cout << "\n  Unsupported record: " << ESM::NAME(reader.hdr().record.typeId).toStringView() << '\n';

            reader.skipRecordData();
        }

        bool readItem(const Params& params, ESM4::Reader& reader);

        bool readGroup(const Params& params, ESM4::Reader& reader)
        {
            const ESM4::RecordHeader& header = reader.hdr();

            if (!params.mQuite)
                std::cout << "\nGroup: " << toString(static_cast<ESM4::GroupType>(header.group.type))
                    << " " << ESM::NAME(header.group.typeId).toStringView() << '\n';

            switch (static_cast<ESM4::GroupType>(header.group.type))
            {
                case ESM4::Grp_RecordType:
                case ESM4::Grp_InteriorCell:
                case ESM4::Grp_InteriorSubCell:
                case ESM4::Grp_ExteriorCell:
                case ESM4::Grp_ExteriorSubCell:
                    reader.enterGroup();
                    return readItem(params, reader);
                case ESM4::Grp_WorldChild:
                case ESM4::Grp_CellChild:
                case ESM4::Grp_TopicChild:
                case ESM4::Grp_CellPersistentChild:
                case ESM4::Grp_CellTemporaryChild:
                case ESM4::Grp_CellVisibleDistChild:
                    reader.adjustGRUPFormId();
                    reader.enterGroup();
                    if (!reader.hasMoreRecs())
                        return false;
                    return readItem(params, reader);
            }

            reader.skipGroup();

            return true;
        }

        bool readItem(const Params& params, ESM4::Reader& reader)
        {
            if (!reader.getRecordHeader() || !reader.hasMoreRecs())
                return false;

            const ESM4::RecordHeader& header = reader.hdr();

            if (header.record.typeId == ESM4::REC_GRUP)
                return readGroup(params, reader);

            readRecord(params, reader);
            return true;
        }
    }

    int loadTes4(const Arguments& info, std::unique_ptr<boost::filesystem::ifstream>&& stream)
    {
        std::cout << "Loading TES4 file: " << info.filename << '\n';

        try
        {
            const ToUTF8::StatelessUtf8Encoder encoder(ToUTF8::calculateEncoding(info.encoding));
            ESM4::Reader reader(std::move(stream), info.filename);
            reader.setEncoder(&encoder);
            const Params params(info);

            if (!params.mQuite)
            {
                std::cout << "Author: " << reader.getAuthor() << '\n'
                    << "Description: " << reader.getDesc() << '\n'
                    << "File format version: " << reader.esmVersion() << '\n';

                if (const std::vector<ESM::MasterData>& masterData = reader.getGameFiles(); !masterData.empty())
                {
                    std::cout << "Masters:" << '\n';
                    for (const auto& master : masterData)
                        std::cout << "  " << master.name << ", " << master.size << " bytes\n";
                }
            }

            while (reader.hasMoreRecs())
            {
                reader.exitGroupCheck();
                if (!readItem(params, reader))
                    break;
            }
        }
        catch (const std::exception& e)
        {
            std::cout << "\nERROR:\n\n  " << e.what() << std::endl;
            return -1;
        }

        return 0;
    }
}
