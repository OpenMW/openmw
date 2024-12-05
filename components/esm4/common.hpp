/*
  Copyright (C) 2015-2020 cc9cii

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  cc9cii cc9c@iinet.net.au

  Much of the information on the data structures are based on the information
  from Tes4Mod:Mod_File_Format and Tes5Mod:File_Formats but also refined by
  trial & error.  See http://en.uesp.net/wiki for details.

*/
#ifndef ESM4_COMMON_H
#define ESM4_COMMON_H

#include "components/esm/fourcc.hpp"

namespace ESM4
{
    using ESM::fourCC;

    enum RecordTypes
    {
        REC_AACT = fourCC("AACT"), // Action
        REC_ACHR = fourCC("ACHR"), // Actor Reference
        REC_ACTI = fourCC("ACTI"), // Activator
        REC_ADDN = fourCC("ADDN"), // Addon Node
        REC_ALCH = fourCC("ALCH"), // Potion
        REC_AMMO = fourCC("AMMO"), // Ammo
        REC_ANIO = fourCC("ANIO"), // Animated Object
        REC_APPA = fourCC("APPA"), // Apparatus (probably unused)
        REC_ARMA = fourCC("ARMA"), // Armature (Model)
        REC_ARMO = fourCC("ARMO"), // Armor
        REC_ARTO = fourCC("ARTO"), // Art Object
        REC_ASPC = fourCC("ASPC"), // Acoustic Space
        REC_ASTP = fourCC("ASTP"), // Association Type
        REC_AVIF = fourCC("AVIF"), // Actor Values/Perk Tree Graphics
        REC_BOOK = fourCC("BOOK"), // Book
        REC_BPTD = fourCC("BPTD"), // Body Part Data
        REC_CAMS = fourCC("CAMS"), // Camera Shot
        REC_CELL = fourCC("CELL"), // Cell
        REC_CLAS = fourCC("CLAS"), // Class
        REC_CLFM = fourCC("CLFM"), // Color
        REC_CLMT = fourCC("CLMT"), // Climate
        REC_CLOT = fourCC("CLOT"), // Clothing
        REC_COBJ = fourCC("COBJ"), // Constructible Object (recipes)
        REC_COLL = fourCC("COLL"), // Collision Layer
        REC_CONT = fourCC("CONT"), // Container
        REC_CPTH = fourCC("CPTH"), // Camera Path
        REC_CREA = fourCC("CREA"), // Creature
        REC_CSTY = fourCC("CSTY"), // Combat Style
        REC_DEBR = fourCC("DEBR"), // Debris
        REC_DIAL = fourCC("DIAL"), // Dialog Topic
        REC_DLBR = fourCC("DLBR"), // Dialog Branch
        REC_DLVW = fourCC("DLVW"), // Dialog View
        REC_DOBJ = fourCC("DOBJ"), // Default Object Manager
        REC_DOOR = fourCC("DOOR"), // Door
        REC_DUAL = fourCC("DUAL"), // Dual Cast Data (possibly unused)
        REC_ECZN = fourCC("ECZN"), // Encounter Zone
        REC_EFSH = fourCC("EFSH"), // Effect Shader
        REC_ENCH = fourCC("ENCH"), // Enchantment
        REC_EQUP = fourCC("EQUP"), // Equip Slot (flag-type values)
        REC_EXPL = fourCC("EXPL"), // Explosion
        REC_EYES = fourCC("EYES"), // Eyes
        REC_FACT = fourCC("FACT"), // Faction
        REC_FLOR = fourCC("FLOR"), // Flora
        REC_FLST = fourCC("FLST"), // Form List (non-levelled list)
        REC_FSTP = fourCC("FSTP"), // Footstep
        REC_FSTS = fourCC("FSTS"), // Footstep Set
        REC_FURN = fourCC("FURN"), // Furniture
        REC_GLOB = fourCC("GLOB"), // Global Variable
        REC_GMST = fourCC("GMST"), // Game Setting
        REC_GRAS = fourCC("GRAS"), // Grass
        REC_GRUP = fourCC("GRUP"), // Form Group
        REC_HAIR = fourCC("HAIR"), // Hair
        REC_HAZD = fourCC("HAZD"), // Hazard
        REC_HDPT = fourCC("HDPT"), // Head Part
        REC_IDLE = fourCC("IDLE"), // Idle Animation
        REC_IDLM = fourCC("IDLM"), // Idle Marker
        REC_IMAD = fourCC("IMAD"), // Image Space Modifier
        REC_IMGS = fourCC("IMGS"), // Image Space
        REC_INFO = fourCC("INFO"), // Dialog Topic Info
        REC_INGR = fourCC("INGR"), // Ingredient
        REC_IPCT = fourCC("IPCT"), // Impact Data
        REC_IPDS = fourCC("IPDS"), // Impact Data Set
        REC_KEYM = fourCC("KEYM"), // Key
        REC_KYWD = fourCC("KYWD"), // Keyword
        REC_LAND = fourCC("LAND"), // Land
        REC_LCRT = fourCC("LCRT"), // Location Reference Type
        REC_LCTN = fourCC("LCTN"), // Location
        REC_LGTM = fourCC("LGTM"), // Lighting Template
        REC_LIGH = fourCC("LIGH"), // Light
        REC_LSCR = fourCC("LSCR"), // Load Screen
        REC_LTEX = fourCC("LTEX"), // Land Texture
        REC_LVLC = fourCC("LVLC"), // Leveled Creature
        REC_LVLI = fourCC("LVLI"), // Leveled Item
        REC_LVLN = fourCC("LVLN"), // Leveled Actor
        REC_LVSP = fourCC("LVSP"), // Leveled Spell
        REC_MATO = fourCC("MATO"), // Material Object
        REC_MATT = fourCC("MATT"), // Material Type
        REC_MESG = fourCC("MESG"), // Message
        REC_MGEF = fourCC("MGEF"), // Magic Effect
        REC_MISC = fourCC("MISC"), // Misc. Object
        REC_MOVT = fourCC("MOVT"), // Movement Type
        REC_MSTT = fourCC("MSTT"), // Movable Static
        REC_MUSC = fourCC("MUSC"), // Music Type
        REC_MUST = fourCC("MUST"), // Music Track
        REC_NAVI = fourCC("NAVI"), // Navigation (master data)
        REC_NAVM = fourCC("NAVM"), // Nav Mesh
        REC_NOTE = fourCC("NOTE"), // Note
        REC_NPC_ = fourCC("NPC_"), // Actor (NPC, Creature)
        REC_OTFT = fourCC("OTFT"), // Outfit
        REC_PACK = fourCC("PACK"), // AI Package
        REC_PERK = fourCC("PERK"), // Perk
        REC_PGRE = fourCC("PGRE"), // Placed grenade
        REC_PHZD = fourCC("PHZD"), // Placed hazard
        REC_PROJ = fourCC("PROJ"), // Projectile
        REC_QUST = fourCC("QUST"), // Quest
        REC_RACE = fourCC("RACE"), // Race / Creature type
        REC_REFR = fourCC("REFR"), // Object Reference
        REC_REGN = fourCC("REGN"), // Region (Audio/Weather)
        REC_RELA = fourCC("RELA"), // Relationship
        REC_REVB = fourCC("REVB"), // Reverb Parameters
        REC_RFCT = fourCC("RFCT"), // Visual Effect
        REC_SBSP = fourCC("SBSP"), // Subspace (TES4 only?)
        REC_SCEN = fourCC("SCEN"), // Scene
        REC_SCPT = fourCC("SCPT"), // Script
        REC_SCRL = fourCC("SCRL"), // Scroll
        REC_SGST = fourCC("SGST"), // Sigil Stone
        REC_SHOU = fourCC("SHOU"), // Shout
        REC_SLGM = fourCC("SLGM"), // Soul Gem
        REC_SMBN = fourCC("SMBN"), // Story Manager Branch Node
        REC_SMEN = fourCC("SMEN"), // Story Manager Event Node
        REC_SMQN = fourCC("SMQN"), // Story Manager Quest Node
        REC_SNCT = fourCC("SNCT"), // Sound Category
        REC_SNDR = fourCC("SNDR"), // Sound Reference
        REC_SOPM = fourCC("SOPM"), // Sound Output Model
        REC_SOUN = fourCC("SOUN"), // Sound
        REC_SPEL = fourCC("SPEL"), // Spell
        REC_SPGD = fourCC("SPGD"), // Shader Particle Geometry
        REC_STAT = fourCC("STAT"), // Static
        REC_TACT = fourCC("TACT"), // Talking Activator
        REC_TERM = fourCC("TERM"), // Terminal
        REC_TES4 = fourCC("TES4"), // Plugin info
        REC_TREE = fourCC("TREE"), // Tree
        REC_TXST = fourCC("TXST"), // Texture Set
        REC_VTYP = fourCC("VTYP"), // Voice Type
        REC_WATR = fourCC("WATR"), // Water Type
        REC_WEAP = fourCC("WEAP"), // Weapon
        REC_WOOP = fourCC("WOOP"), // Word Of Power
        REC_WRLD = fourCC("WRLD"), // World Space
        REC_WTHR = fourCC("WTHR"), // Weather
        REC_ACRE = fourCC("ACRE"), // Placed Creature (TES4 only?)
        REC_PGRD = fourCC("PGRD"), // Pathgrid (TES4 only?)
        REC_ROAD = fourCC("ROAD"), // Road (TES4 only?)
        REC_IMOD = fourCC("IMOD"), // Item Mod
        REC_PWAT = fourCC("PWAT"), // Placeable Water
        REC_SCOL = fourCC("SCOL"), // Static Collection
        REC_CCRD = fourCC("CCRD"), // Caravan Card
        REC_CMNY = fourCC("CMNY"), // Caravan Money
        REC_ALOC = fourCC("ALOC"), // Audio Location Controller
        REC_MSET = fourCC("MSET") // Media Set
    };

    // Based on http://www.uesp.net/wiki/Tes5Mod:Mod_File_Format#Records
    enum RecordFlag
    {
        Rec_ESM = 0x00000001, // (TES4 record only) Master (ESM) file.
        Rec_Deleted = 0x00000020, // Deleted
        Rec_Constant = 0x00000040, // Constant
        Rec_HiddenLMap = 0x00000040, // (REFR) Hidden From Local Map (Needs Confirmation: Related to shields)
        Rec_Localized = 0x00000080, // (TES4 record only) Is localized. This will make Skyrim load the
                                    //   .STRINGS, .DLSTRINGS, and .ILSTRINGS files associated with the mod.
                                    //   If this flag is not set, lstrings are treated as zstrings.
        Rec_FireOff = 0x00000080, // (PHZD) Turn off fire
        Rec_UpdateAnim = 0x00000100, // Must Update Anims
        Rec_NoAccess = 0x00000100, // (REFR) Inaccessible
        Rec_Hidden = 0x00000200, // (REFR) Hidden from local map
        Rec_StartDead = 0x00000200, // (ACHR) Starts dead /(REFR) MotionBlurCastsShadows
        Rec_Persistent = 0x00000400, // Quest item / Persistent reference
        Rec_DispMenu = 0x00000400, // (LSCR) Displays in Main Menu
        Rec_Disabled = 0x00000800, // Initially disabled
        Rec_Ignored = 0x00001000, // Ignored
        Rec_VisDistant = 0x00008000, // Visible when distant
        Rec_RandAnim = 0x00010000, // (ACTI) Random Animation Start
        Rec_Danger = 0x00020000, // (ACTI) Dangerous / Off limits (Interior cell)
                                 //   Dangerous Can't be set withough Ignore Object Interaction
        Rec_Compressed = 0x00040000, // Data is compressed
        Rec_CanNotWait = 0x00080000, // Can't wait
        Rec_IgnoreObj = 0x00100000, // (ACTI) Ignore Object Interaction
                                    //   Ignore Object Interaction Sets Dangerous Automatically
        Rec_Marker = 0x00800000, // Is Marker
        Rec_Obstacle = 0x02000000, // (ACTI) Obstacle / (REFR) No AI Acquire
        Rec_NavMFilter = 0x04000000, // NavMesh Gen - Filter
        Rec_NavMBBox = 0x08000000, // NavMesh Gen - Bounding Box
        Rec_ExitToTalk = 0x10000000, // (FURN) Must Exit to Talk
        Rec_Refected = 0x10000000, // (REFR) Reflected By Auto Water
        Rec_ChildUse = 0x20000000, // (FURN/IDLM) Child Can Use
        Rec_NoHavok = 0x20000000, // (REFR) Don't Havok Settle
        Rec_NavMGround = 0x40000000, // NavMesh Gen - Ground
        Rec_NoRespawn = 0x40000000, // (REFR) NoRespawn
        Rec_MultiBound = 0x80000000 // (REFR) MultiBound
    };
}

#endif // ESM4_COMMON_H
