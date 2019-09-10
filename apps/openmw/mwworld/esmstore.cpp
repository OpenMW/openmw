#include "esmstore.hpp"

#include <set>

#include <boost/filesystem/operations.hpp>

#include <components/debug/debuglog.hpp>
#include <components/loadinglistener/loadinglistener.hpp>
#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>
#include <components/esm/esm4reader.hpp>


namespace MWWorld
{

static bool isCacheableRecord(int id)
{
    if (id == ESM::REC_ACTI || id == ESM::REC_ALCH || id == ESM::REC_APPA || id == ESM::REC_ARMO ||
        id == ESM::REC_BOOK || id == ESM::REC_CLOT || id == ESM::REC_CONT || id == ESM::REC_CREA ||
        id == ESM::REC_DOOR || id == ESM::REC_INGR || id == ESM::REC_LEVC || id == ESM::REC_LEVI ||
        id == ESM::REC_LIGH || id == ESM::REC_LOCK || id == ESM::REC_MISC || id == ESM::REC_NPC_ ||
        id == ESM::REC_PROB || id == ESM::REC_REPA || id == ESM::REC_STAT || id == ESM::REC_WEAP ||
        id == ESM::REC_BODY)
    {
        return true;
    }
    return false;
}

void ESMStore::load(ESM::ESMReader &esm, Loading::Listener* listener)
{
    listener->setProgressRange(1000);

    ESM::Dialogue *dialogue = 0;

    // Land texture loading needs to use a separate internal store for each plugin.
    // We set the number of plugins here to avoid continual resizes during loading,
    // and so we can properly verify if valid plugin indices are being passed to the
    // LandTexture Store retrieval methods.
    mLandTextures.resize(esm.getGlobalReaderList()->size());
    int esmVer = esm.getVer();
    bool isTes4 = esmVer == ESM::VER_080 || esmVer == ESM::VER_100;
    bool isTes5 = esmVer == ESM::VER_094 || esmVer == ESM::VER_17;
    bool isFONV = esmVer == ESM::VER_132 || esmVer == ESM::VER_133 || esmVer == ESM::VER_134;

    // FIXME: for TES4/TES5 whether a dependent file is loaded is already checked in
    // ESM4::Reader::updateModIndicies() which is called in EsmLoader::load() before this
    if (!(isTes4 || isTes5 || isFONV)) // MW only
    {
        /// \todo Move this to somewhere else. ESMReader?
    /// \todo Move this to somewhere else. ESMReader?
    // Cache parent esX files by tracking their indices in the global list of
    //  all files/readers used by the engine. This will greaty accelerate
    //  refnumber mangling, as required for handling moved references.
    const std::vector<ESM::Header::MasterData> &masters = esm.getGameFiles();
    std::vector<ESM::ESMReader*> *allPlugins = esm.getGlobalReaderList();
    for (size_t j = 0; j < masters.size(); j++) {
        ESM::Header::MasterData &mast = const_cast<ESM::Header::MasterData&>(masters[j]);
        std::string fname = mast.name;
        int index = ~0;
        for (int i = 0; i < esm.getIndex(); i++) {
            const std::string &candidate = allPlugins->at(i)->getContext().filename;
            std::string fnamecandidate = boost::filesystem::path(candidate).filename().string();
            if (Misc::StringUtils::ciEqual(fname, fnamecandidate)) {
                index = i;
                break;
            }
        }
        if (index == (int)~0) {
            // Tried to load a parent file that has not been loaded yet. This is bad,
            //  the launcher should have taken care of this.
            std::string fstring = "File " + esm.getName() + " asks for parent file " + masters[j].name
                + ", but it has not been loaded yet. Please check your load order.";
            esm.fail(fstring);
        }
        mast.index = index;
    }
}
    // Loop through all records
    while(esm.hasMoreRecs())
    {
        if (isTes4 || isTes5 || isFONV)
        {
            ESM4::Reader& reader = dynamic_cast<ESM::ESM4Reader*>(&esm)->reader();
            //if(reader.getFileOffset()>= reader.getFileSize())break;
            reader.checkGroupStatus();

            loadTes4Group(esm);
            listener->setProgress(static_cast<size_t>(reader.getFileOffset() / (float)reader.getFileSize() * 1000));
            continue;
        }
        ESM::NAME n = esm.getRecName();
        esm.getRecHeader();

        // Look up the record type.
        std::map<int, StoreBase *>::iterator it = mStores.find(n.intval);

        if (it == mStores.end())
        {
            if (n.intval == ESM::REC_INFO) {
                if (dialogue)
                {
                    dialogue->readInfo(esm, esm.getIndex() != 0);
                }
                else
                {
                    Log(Debug::Error) << "Error: info record without dialog";
                    esm.skipRecord();
                }
            } else if (n.intval == ESM::REC_MGEF) {
                mMagicEffects.load (esm);
            } else if (n.intval == ESM::REC_SKIL) {
                mSkills.load (esm);
            }
            else if (n.intval==ESM::REC_FILT || n.intval == ESM::REC_DBGP)
            {
                // ignore project file only records
                esm.skipRecord();
            }
            else {
                std::stringstream error;
                error << "Unknown record: " << n.toString();
                throw std::runtime_error(error.str());
            }
        } else {
            RecordId id = it->second->load(esm);
            if (id.mIsDeleted)
            {
                it->second->eraseStatic(id.mId);
                continue;
            }

            if (n.intval==ESM::REC_DIAL) {
                dialogue = const_cast<ESM::Dialogue*>(mDialogs.find(id.mId));
            } else {
                dialogue = 0;
            }
        }
        listener->setProgress(static_cast<size_t>(esm.getFileOffset() / (float)esm.getFileSize() * 1000));
    }
}

// Can't use ESM4::Reader& as the parameter here because we need esm.hasMoreRecs() for
// checking an empty group followed by EOF
void ESMStore::loadTes4Group (ESM::ESMReader &esm)
{
    ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();

    reader.getRecordHeader();
    const ESM4::RecordHeader& hdr = reader.hdr();

    if (hdr.record.typeId != ESM4::REC_GRUP)
    {
        /*if(hdr.record.typeId==ESM4::REC_DIAL)
        {
            ESM4::Dialog * dialog;
            std::string id = ESM4::formIdToString(hdr.record.id);
            loadTes4Record(esm);//load Dialog

            dialog = const_cast<ESM4::Dialog*>(mDialogs2.find(id));

            reader.getRecordHeader();
            if(hdr.record.typeId == ESM4::REC_GRUP||hdr.record.typeId == ESM4::REC_INFO){
                if(hdr.record.typeId == ESM4::REC_GRUP){
                    reader.saveGroupStatus();

                    reader.getRecordHeader();
                }

                while(hdr.record.typeId == ESM4::REC_INFO)
                {
                    reader.getRecordData();
                    dialog->loadInfo(reader);
                    reader.getRecordHeader();
                }
            }else
                Log(Debug::Warning)<<"NO INFO DIALOG?"<<ESM4::printName(hdr.record.typeId );
            if(hdr.record.typeId != ESM4::REC_GRUP)
                return loadTes4Record(esm);
        }
        else */return loadTes4Record(esm);
    }
    switch (hdr.group.type)
    {
        case ESM4::Grp_RecordType:
        {
            // FIXME: rewrite to workaround reliability issue
            if (hdr.group.label.value == ESM4::REC_NAVI || hdr.group.label.value == ESM4::REC_WRLD ||
                hdr.group.label.value == ESM4::REC_REGN || hdr.group.label.value == ESM4::REC_STAT ||
                hdr.group.label.value == ESM4::REC_ANIO || hdr.group.label.value == ESM4::REC_CONT ||
                hdr.group.label.value == ESM4::REC_MISC || hdr.group.label.value == ESM4::REC_ACTI ||
                hdr.group.label.value == ESM4::REC_ARMO || hdr.group.label.value == ESM4::REC_NPC_ ||
                hdr.group.label.value == ESM4::REC_FLOR || hdr.group.label.value == ESM4::REC_GRAS ||
                hdr.group.label.value == ESM4::REC_TREE || hdr.group.label.value == ESM4::REC_LIGH ||
                hdr.group.label.value == ESM4::REC_BOOK || hdr.group.label.value == ESM4::REC_FURN ||
                hdr.group.label.value == ESM4::REC_SOUN || hdr.group.label.value == ESM4::REC_WEAP ||
                hdr.group.label.value == ESM4::REC_DOOR || hdr.group.label.value == ESM4::REC_AMMO ||
                hdr.group.label.value == ESM4::REC_CLOT || hdr.group.label.value == ESM4::REC_ALCH ||
                hdr.group.label.value == ESM4::REC_APPA || hdr.group.label.value == ESM4::REC_INGR ||
                hdr.group.label.value == ESM4::REC_SGST || hdr.group.label.value == ESM4::REC_SLGM ||
                hdr.group.label.value == ESM4::REC_KEYM || hdr.group.label.value == ESM4::REC_HAIR ||
                hdr.group.label.value == ESM4::REC_EYES || hdr.group.label.value == ESM4::REC_CELL ||
                hdr.group.label.value == ESM4::REC_CREA || hdr.group.label.value == ESM4::REC_LVLC ||
                hdr.group.label.value == ESM4::REC_LVLI || hdr.group.label.value == ESM4::REC_MATO ||
                hdr.group.label.value == ESM4::REC_IDLE || hdr.group.label.value == ESM4::REC_LTEX ||
                hdr.group.label.value == ESM4::REC_RACE || hdr.group.label.value == ESM4::REC_SBSP

                    || hdr.group.label.value == ESM4::REC_BSGN|| hdr.group.label.value == ESM4::REC_DIAL|| hdr.group.label.value == ESM4::REC_QUST
                    || hdr.group.label.value == ESM4::REC_FACT|| hdr.group.label.value == ESM4::REC_CLAS|| hdr.group.label.value == ESM4::REC_GLOB
                    || hdr.group.label.value == ESM4::REC_SCPT
                )
            {
                reader.saveGroupStatus();
                loadTes4Group(esm);
            }
            else
            {
                // Skip groups that are of no interest (for now).
                //  GMST  SKIL MGEF  ENCH SPEL  WTHR CLMT
                //  QUST PACK CSTY LSCR LVSP WATR EFSH

                // FIXME: The label field of a group is not reliable, so we will need to check here as well
                std::cout << "skipping group... " << ESM4::printLabel(hdr.group.label, hdr.group.type) << std::endl;
                reader.saveGroupStatus();
                loadTes4Group(esm);//reader.skipGroup();
                return;
            }

            break;
        }
        case ESM4::Grp_CellChild:
        case ESM4::Grp_WorldChild:
        case ESM4::Grp_TopicChild:
        case ESM4::Grp_CellPersistentChild:
        {
            reader.adjustGRUPFormId();  // not needed or even shouldn't be done? (only labels anyway)
            reader.saveGroupStatus();
//#if 0
            // Below test shows that Oblivion.esm does not have any persistent cell child
            // groups under exterior world sub-block group.  Haven't checked other files yet.
             if (reader.grp(0).type == ESM4::Grp_CellPersistentChild &&
                 reader.grp(1).type == ESM4::Grp_CellChild &&
                 !(reader.grp(2).type == ESM4::Grp_WorldChild || reader.grp(2).type == ESM4::Grp_InteriorSubCell))
                 std::cout << "Unexpected persistent child group in exterior subcell" << std::endl;
//#endif
            if (!esm.hasMoreRecs())
                return; // may have been an empty group followed by EOF

            loadTes4Group(esm);

            break;
        }
        case ESM4::Grp_CellTemporaryChild:
        case ESM4::Grp_CellVisibleDistChild:
        {
            // NOTE: preload strategy and persistent records
            //
            // Current strategy defers loading of "temporary" or "visible when distant"
            // references and other records (land and pathgrid) until they are needed.
            //
            // The "persistent" records need to be loaded up front, however.  This is to allow,
            // for example, doors to work.  A door reference will have a FormId of the
            // destination door FormId.  But we have no way of knowing to which cell the
            // destination FormId belongs until that cell and that reference is loaded.
            //
            // For worldspaces the persistent records are usully (always?) stored in a dummy
            // cell under a "world child" group.  It may be possible to skip the whole "cell
            // child" group without scanning for persistent records.  See above short test.
            reader.skipGroup();
            break;
        }
        case ESM4::Grp_ExteriorCell:
        case ESM4::Grp_ExteriorSubCell:
        case ESM4::Grp_InteriorCell:
        case ESM4::Grp_InteriorSubCell:
        {
            reader.saveGroupStatus();
            loadTes4Group(esm);

            break;
        }
        default:
            reader.skipGroup();
            break;
    }

    return;
}

void ESMStore::loadTes4Record (ESM::ESMReader& esm)
{
    // Assumes that the reader has just read the record header only.
    ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();
    const ESM4::RecordHeader& hdr = reader.hdr();

    switch (hdr.record.typeId)
    {

        // FIXME: removed for now
    case ESM4::REC_SOUN : // Sound
        case ESM4::REC_STAT : // Static
    case ESM4::REC_WEAP :// weapons

    case ESM4::REC_RACE : // Race / Creature type
    case ESM4::REC_REGN : // Region (Audio/Weather)
    case ESM4::REC_LIGH : // Light
    case ESM4::REC_LTEX :// Land Texture
    case ESM4::REC_LAND :// Land
    case ESM4::REC_INGR : // Ingredient

    case ESM4::REC_DOOR : // Door
    case ESM4::REC_CREA: // Creature
    case ESM4::REC_CONT : // Container
    case ESM4::REC_CLOT : // Clothing
    case ESM4::REC_CLAS : // Class
    case ESM4::REC_CELL : // Cell
    case ESM4::REC_ARMO : // Armor
    case ESM4::REC_APPA : // Apparatus (probably unused)
    case ESM4::REC_ALCH :// Potion
    case ESM4::REC_ACTI :// Activator
    case ESM4::REC_BOOK :// Book
    case ESM4::REC_BSGN : // Birth Sign
    case ESM4::REC_DIAL : // Dialog Topic
   case ESM4::REC_DLBR : // Dialog Branch
   case ESM4::REC_HAIR : // Hair
    case ESM4::REC_QUST :// Quest
       case ESM4::REC_INFO :// Dialog Topic Info
  case ESM4::REC_ACHR :// Actor Reference
   case ESM4::REC_ACRE :// Placed Creature (TES4 only?)
    case ESM4::REC_NPC_ :  // Actor (NPC, Creature) TOFIX there seems to be subrecordd larger than the record itself !:/

 case ESM4::REC_HDPT : // Head Part
        case ESM4::REC_FLOR : // Flora
        case ESM4::REC_AMMO : // Ammo
    case ESM4::REC_EYES : // Eyes
    case ESM4::REC_NOTE :  // Note
    case ESM4::REC_GRAS :  // Grass
    case ESM4::REC_GLOB : // Global Variable
    case ESM4::REC_ANIO :// Animated Object

    case ESM4::REC_ARMA: // Armature (Model) ArmorAddon
         case ESM4::REC_FACT :// Faction
        case ESM4::REC_FURN : // Furniture

    case ESM4::REC_SCPT: // script
         case ESM4::REC_REFR : // Object Reference (only persistent refs)
    {
        //  ESM4::ReaderContext ctx=reader.getContext();
        reader.getRecordData();
        mESM4Stores[hdr.record.typeId]->load(esm);
       // reader.restoreContext(ctx);
    }
        break;
     //   case ESM4::REC_NPC_ :  // Actor (NPC, Creature) TOFIX there seems to be subrecordd larger than the record itself !:/
   //   reader.getRecordData();        while(reader.getSubRecordHeader())            reader.skipSubRecordData();
          //reader.skipRecordData();
        break;
    case ESM4::REC_AACT :reader.skipRecordData();break; // Action
    //case ESM4::REC_ACHR :reader.skipRecordData();break; // Actor Reference
    case ESM4::REC_ADDN :reader.skipRecordData();break; // Addon Node
    //case ESM4::REC_AMMO :reader.skipRecordData();break; // Ammo
    //case ESM4::REC_ANIO :reader.skipRecordData();break;// Animated Object
    //case ESM4::REC_ARMA:reader.skipRecordData();break;// Armature (Model)
    case ESM4::REC_ARTO :reader.skipRecordData();break; // Art Object
    case ESM4::REC_ASPC :reader.skipRecordData();break; // Acoustic Space
    case ESM4::REC_ASTP:reader.skipRecordData();break; // Association Type
    case ESM4::REC_AVIF :reader.skipRecordData();break; // Actor Values/Perk Tree Graphics
    case ESM4::REC_BPTD :reader.skipRecordData();break; // Body Part Data
    case ESM4::REC_CAMS :reader.skipRecordData();break; // Camera Shot
    case ESM4::REC_CLFM :reader.skipRecordData();break; // Color
    case ESM4::REC_CLMT :reader.skipRecordData();break; // Climate
    case ESM4::REC_COBJ :reader.skipRecordData();break;// Constructible Object (recipes)
    case ESM4::REC_COLL :reader.skipRecordData();break; // Collision Layer
    case ESM4::REC_CPTH :reader.skipRecordData();break; // Camera Path
    case ESM4::REC_CSTY :reader.skipRecordData();break; // Combat Style
    case ESM4::REC_DEBR :reader.skipRecordData();break; // Debris
//    case ESM4::REC_DIAL :reader.skipRecordData();break; // Dialog Topic
    //case ESM4::REC_DLBR :reader.skipRecordData();break; // Dialog Branch
    case ESM4::REC_DLVW :reader.skipRecordData();break; // Dialog View
    case ESM4::REC_DOBJ :reader.skipRecordData();break; // Default Object Manager
    case ESM4::REC_DUAL :reader.skipRecordData();break; // Dual Cast Data (possibly unused)
  //REC_ECZN = MKTAG('E','C','Z','N'), // Encounter Zone
  case ESM4::  REC_EFSH :reader.skipRecordData();break; // Effect Shader
    case ESM4::REC_ENCH :reader.skipRecordData();break; // Enchantment
    case ESM4::REC_EQUP :reader.skipRecordData();break; // Equip Slot (flag-type values)
    case ESM4::REC_EXPL :reader.skipRecordData();break; // Explosion
  //  case ESM4::REC_EYES :reader.skipRecordData();break; // Eyes
   // case ESM4::REC_FACT :reader.skipRecordData();break; // Faction
   // case ESM4::REC_FLOR :reader.skipRecordData();break; // Flora
    case ESM4::REC_FLST:reader.skipRecordData();break; // Form List (non-leveled list)
    case ESM4::REC_FSTP :reader.skipRecordData();break; // Footstep
    case ESM4::REC_FSTS :reader.skipRecordData();break; // Footstep Set
    //case ESM4::REC_FURN :reader.skipRecordData();break; // Furniture
  //  case ESM4::REC_GLOB :reader.skipRecordData();break; // Global Variable
    case ESM4::REC_GMST :reader.skipRecordData();break; // Game Setting
//    case ESM4::REC_GRAS :reader.skipRecordData();break; // Grass
    //case ESM4::REC_HAIR :reader.skipRecordData();break; // Hair
  //REC_HAZD = MKTAG('H','A','Z','D'), // Hazard
    //case ESM4::REC_HDPT :reader.skipRecordData();break; // Head Part
    case ESM4::REC_IDLE :reader.skipRecordData();break; // Idle Animation
    case ESM4::REC_IDLM :reader.skipRecordData();break; // Idle Marker
    case ESM4::REC_IMAD :reader.skipRecordData();break;// Image Space Modifier
    case ESM4::REC_IMGS :reader.skipRecordData();break; // Image Space
    //case ESM4::REC_INFO :reader.skipRecordData();break; // Dialog Topic Info
    case ESM4::REC_IPCT :reader.skipRecordData();break; // Impact Data
    case ESM4::REC_IPDS :reader.skipRecordData();break; // Impact Data Set
    case ESM4::REC_KEYM :reader.skipRecordData();break; // Key
    case ESM4::REC_KYWD :reader.skipRecordData();break; // Keyword
    case ESM4::REC_LCRT :reader.skipRecordData();break; // Location Reference Type
    case ESM4::REC_LCTN :reader.skipRecordData();break; // Location
    case ESM4::REC_LGTM :reader.skipRecordData();break; // Lighting Template
    case ESM4::REC_LSCR :reader.skipRecordData();break; // Load Screen
    case ESM4::REC_LVLC:reader.skipRecordData();break; // Leveled Creature
    case ESM4::REC_LVLI :reader.skipRecordData();break; // Leveled Item
    case ESM4::REC_LVLN :reader.skipRecordData();break; // Leveled Actor
    case ESM4::REC_LVSP :reader.skipRecordData();break; // Leveled Spell
    case ESM4::REC_MATO :reader.skipRecordData();break; // Material Object
    case ESM4::REC_MATT :reader.skipRecordData();break; // Material Type
    case ESM4::REC_MESG :reader.skipRecordData();break; // Message
    case ESM4::REC_MGEF :reader.skipRecordData();break; // Magic Effect
    case ESM4::REC_MISC :reader.skipRecordData();break; // Misc. Object
    case ESM4::REC_MOVT :reader.skipRecordData();break; // Movement Type
    case ESM4::REC_MSTT :reader.skipRecordData();break; // Movable Static
    case ESM4::REC_MUSC :reader.skipRecordData();break; // Music Type
    case ESM4::REC_MUST :reader.skipRecordData();break; // Music Track
    case ESM4::REC_NAVI :reader.skipRecordData();break; // Navigation (master data)
    case ESM4::REC_NAVM :reader.skipRecordData();break; // Nav Mesh
    //case ESM4::REC_NOTE :reader.skipRecordData();break; // Note
    //case ESM4::REC_NPC_ :reader.skipRecordData();break; // Actor (NPC, Creature)
    case ESM4::REC_OTFT :reader.skipRecordData();break;// Outfit
    case ESM4::REC_PACK :reader.skipRecordData();break; // AI Package
    case ESM4::REC_PERK :reader.skipRecordData();break; // Perk
    case ESM4::REC_PGRE :reader.skipRecordData();break; // Placed grenade
    case ESM4::REC_PHZD :reader.skipRecordData();break; // Placed hazard
    case ESM4::REC_PROJ :reader.skipRecordData();break; // Projectile
  //  case ESM4::REC_QUST :reader.skipRecordData();break; // Quest
   // case ESM4::REC_REFR :reader.skipRecordData();break; // Object Reference
    case ESM4::REC_RELA :reader.skipRecordData();break; // Relationship
    case ESM4::REC_REVB :reader.skipRecordData();break; // Reverb Parameters
    case ESM4::REC_RFCT :reader.skipRecordData();break; // Visual Effect
    case ESM4::REC_SBSP :reader.skipRecordData();break; // Subspace (TES4 only?)
    case ESM4::REC_SCEN :reader.skipRecordData();break;// Scene
    case ESM4::REC_SCRL :reader.skipRecordData();break; // Scroll
    case ESM4::REC_SGST :reader.skipRecordData();break; // Sigil Stone
    case ESM4::REC_SHOU :reader.skipRecordData();break; // Shout
    case ESM4::REC_SLGM :reader.skipRecordData();break; // Soul Gem
    case ESM4::REC_SMBN :reader.skipRecordData();break; // Story Manager Branch Node
    case ESM4::REC_SMEN :reader.skipRecordData();break; // Story Manager Event Node
    case ESM4::REC_SMQN :reader.skipRecordData();break; // Story Manager Quest Node
    case ESM4::REC_SNCT:reader.skipRecordData();break;// Sound Category
    case ESM4::REC_SNDR :reader.skipRecordData();break; // Sound Reference
    case ESM4::REC_SOPM :reader.skipRecordData();break; // Sound Output Model
    case ESM4::REC_SPEL :reader.skipRecordData();break; // Spell
    case ESM4::REC_SPGD :reader.skipRecordData();break; // Shader Particle Geometry

    case ESM4::REC_TACT :reader.skipRecordData();break; // Talking Activator
    case ESM4::REC_TERM :reader.skipRecordData();break; // Terminal
    case ESM4::REC_TES4 :reader.skipRecordData();break; // Plugin info
    case ESM4::REC_TREE :reader.skipRecordData();break; // Tree
    case ESM4::REC_TXST :reader.skipRecordData();break; // Texture Set
    case ESM4::REC_VTYP :reader.skipRecordData();break; // Voice Type
    case ESM4::REC_WATR :reader.skipRecordData();break; // Water Type
    case ESM4::REC_WOOP :reader.skipRecordData();break; // Word Of Power
    case ESM4::REC_WRLD :reader.skipRecordData();break;// World Space
    case ESM4::REC_WTHR :reader.skipRecordData();break; // Weather
   //case ESM4::REC_ACRE:reader.skipRecordData();break; // Placed Creature (TES4 only?)
    case ESM4::REC_PGRD :reader.skipRecordData();break;// Pathgrid (TES4 only?)
    case ESM4::REC_ROAD :reader.skipRecordData();break;  // Road (TES4 only?)
        default:
    {
        if(false)//hdr.record.typeId==ESM4::REC_NPC_)//REC_SSCR)
        {
            reader.getRecordData();
            mESM4Stores[hdr.record.typeId]->load(esm);
        }
        else{
            std::cerr<<"skipping record" <<ESM4::printName (hdr.record.typeId)<<std::endl;
            reader.skipRecordData();
        }
}
    }

    return;
}

void ESMStore::setUp(bool validateRecords)
{
    mIds.clear();
{
    std::map<int, StoreBase *>::iterator storeIt = mStores.begin();
    for (; storeIt != mStores.end(); ++storeIt) {
        storeIt->second->setUp();

        if (isCacheableRecord(storeIt->first))
        {
            std::vector<std::string> identifiers;
            storeIt->second->listIdentifier(identifiers);

            for (std::vector<std::string>::const_iterator record = identifiers.begin(); record != identifiers.end(); ++record)
                mIds[*record] = storeIt->first;
        }
    }}
    std::map<int, StoreBase *>::iterator storeIt = mESM4Stores.begin();
    for (; storeIt != mESM4Stores.end(); ++storeIt) {
        storeIt->second->setUp();

        if (isCacheableRecord(storeIt->first))
        {
            std::vector<std::string> identifiers;
            storeIt->second->listIdentifier(identifiers);

            for (std::vector<std::string>::const_iterator record = identifiers.begin(); record != identifiers.end(); ++record)
                mIds[*record] = storeIt->first;
        }
    }
    mSkills.setUp();
    mMagicEffects.setUp();
    mAttributes.setUp();
    mDialogs.setUp();

    if (validateRecords)
        validate();
}

void ESMStore::validate()
{
    // Cache first class from store - we will use it if current class is not found
    std::string defaultCls = "";
    Store<ESM::Class>::iterator it = mClasses.begin();
    if (it != mClasses.end())
        defaultCls = it->mId;
    else
        throw std::runtime_error("List of NPC classes is empty!");

    // Validate NPCs for non-existing class and faction.
    // We will replace invalid entries by fixed ones
    std::vector<ESM::NPC> npcsToReplace;
    for (ESM::NPC npc : mNpcs)
    {
        bool changed = false;

        const std::string npcFaction = npc.mFaction;
        if (!npcFaction.empty())
        {
            const ESM::Faction *fact = mFactions.search(npcFaction);
            if (!fact)
            {
                Log(Debug::Verbose) << "NPC '" << npc.mId << "' (" << npc.mName << ") has nonexistent faction '" << npc.mFaction << "', ignoring it.";
                npc.mFaction.clear();
                npc.mNpdt.mRank = 0;
                changed = true;
            }
        }

        std::string npcClass = npc.mClass;
        if (!npcClass.empty())
        {
            const ESM::Class *cls = mClasses.search(npcClass);
            if (!cls)
            {
                Log(Debug::Verbose) << "NPC '" << npc.mId << "' (" << npc.mName << ") has nonexistent class '" << npc.mClass << "', using '" << defaultCls << "' class as replacement.";
                npc.mClass = defaultCls;
                changed = true;
            }
        }

        if (changed)
            npcsToReplace.push_back(npc);
    }

    for (const ESM::NPC &npc : npcsToReplace)
    {
        mNpcs.eraseStatic(npc.mId);
        mNpcs.insertStatic(npc);
    }

    // Validate spell effects for invalid arguments
    std::vector<ESM::Spell> spellsToReplace;
    for (ESM::Spell spell : mSpells)
    {
        if (spell.mEffects.mList.empty())
            continue;

        bool changed = false;
        auto iter = spell.mEffects.mList.begin();
        while (iter != spell.mEffects.mList.end())
        {
            const ESM::MagicEffect* mgef = mMagicEffects.search(iter->mEffectID);
            if (!mgef)
            {
                Log(Debug::Verbose) << "Spell '" << spell.mId << "' has an an invalid effect (index " << iter->mEffectID << ") present, dropping it.";
                iter = spell.mEffects.mList.erase(iter);
                changed = true;
                continue;
            }

            if (mgef->mData.mFlags & ESM::MagicEffect::TargetSkill)
            {
                if (iter->mAttribute != -1)
                {
                    iter->mAttribute = -1;
                    Log(Debug::Verbose) << ESM::MagicEffect::effectIdToString(iter->mEffectID) <<
                        " effect of spell '" << spell.mId << "'  has an attribute argument present, dropping it.";
                    changed = true;
                }
            }
            else if (mgef->mData.mFlags & ESM::MagicEffect::TargetAttribute)
            {
                if (iter->mSkill != -1)
                {
                    iter->mSkill = -1;
                    Log(Debug::Verbose) << ESM::MagicEffect::effectIdToString(iter->mEffectID) <<
                        " effect of spell '" << spell.mId << "' has a skill argument present, dropping it.";
                    changed = true;
                }
            }
            else if (iter->mSkill != -1 || iter->mAttribute != -1)
            {
                iter->mSkill = -1;
                iter->mAttribute = -1;
                Log(Debug::Verbose) << ESM::MagicEffect::effectIdToString(iter->mEffectID) <<
                    " effect of spell '" << spell.mId << "' has argument(s) present, dropping them.";
                changed = true;
            }

            ++iter;
        }

        if (changed)
            spellsToReplace.emplace_back(spell);
    }

    for (const ESM::Spell &spell : spellsToReplace)
    {
        mSpells.eraseStatic(spell.mId);
        mSpells.insertStatic(spell);
    }
}

    int ESMStore::countSavedGameRecords() const
    {
        return 1 // DYNA (dynamic name counter)
            +mPotions.getDynamicSize()
            +mArmors.getDynamicSize()
            +mBooks.getDynamicSize()
            +mClasses.getDynamicSize()
            +mClothes.getDynamicSize()
            +mEnchants.getDynamicSize()
            +mNpcs.getDynamicSize()
            +mSpells.getDynamicSize()
            +mWeapons.getDynamicSize()
            +mCreatureLists.getDynamicSize()
            +mItemLists.getDynamicSize();
    }

    void ESMStore::write (ESM::ESMWriter& writer, Loading::Listener& progress) const
    {
        writer.startRecord(ESM::REC_DYNA);
        writer.startSubRecord("COUN");
        writer.writeT(mDynamicCount);
        writer.endRecord("COUN");
        writer.endRecord(ESM::REC_DYNA);

        mPotions.write (writer, progress);
        mArmors.write (writer, progress);
        mBooks.write (writer, progress);
        mClasses.write (writer, progress);
        mClothes.write (writer, progress);
        mEnchants.write (writer, progress);
        mSpells.write (writer, progress);
        mWeapons.write (writer, progress);
        mNpcs.write (writer, progress);
        mItemLists.write (writer, progress);
        mCreatureLists.write (writer, progress);
    }

    bool ESMStore::readRecord (ESM::ESMReader& reader, uint32_t type)
    {
        switch (type)
        {
            case ESM::REC_ALCH:
            case ESM::REC_ARMO:
            case ESM::REC_BOOK:
            case ESM::REC_CLAS:
            case ESM::REC_CLOT:
            case ESM::REC_ENCH:
            case ESM::REC_SPEL:
            case ESM::REC_WEAP:
            case ESM::REC_NPC_:
            case ESM::REC_LEVI:
            case ESM::REC_LEVC:

                {
                    mStores[type]->read (reader);
                }

                if (type==ESM::REC_NPC_)
                {
                    // NPC record will always be last and we know that there can be only one
                    // dynamic NPC record (player) -> We are done here with dynamic record loading
                    setUp();

                    const ESM::NPC *player = mNpcs.find ("player");

                    if (!mRaces.find (player->mRace) ||
                        !mClasses.find (player->mClass))
                        throw std::runtime_error ("Invalid player record (race or class unavailable");
                }

                return true;

            case ESM::REC_DYNA:
                reader.getSubNameIs("COUN");
                reader.getHT(mDynamicCount);
                return true;

            default:

                return false;
        }
    }

} // end namespace
