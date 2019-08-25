/*
  Copyright (C) 2015-2018, 2019 cc9cii

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

  MKTAG macro was adapated from ScummVM.

*/
#ifndef ESM4_COMMON_H
#define ESM4_COMMON_H

#include <cstdint>
#include <string>

// From ScummVM's endianness.h but for little endian
#define MKTAG(a0,a1,a2,a3) ((std::uint32_t)((a0) | ((a1) << 8) | ((a2) << 16) | ((a3) << 24)))

namespace ESM4
{
    enum ESMVersions
    {
        VER_080 = 0x3f4ccccd, // TES4
        VER_100 = 0x3f800000, // TES4
        VER_132 = 0x3fa8f5c3, // FONV Courier's Stash, DeadMoney
        VER_133 = 0x3faa3d71, // FONV HonestHearts
        VER_134 = 0x3fab851f, // FONV, GunRunnersArsenal, LonesomeRoad, OldWorldBlues
        VER_094 = 0x3f70a3d7, // TES5/FO3
        VER_170 = 0x3fd9999a  // TES5
    };

    // Based on http://www.uesp.net/wiki/Tes5Mod:Mod_File_Format
    enum RecordTypes
    {
        REC_AACT = MKTAG('A','A','C','T'), // Action
        REC_ACHR = MKTAG('A','C','H','R'), // Actor Reference
        REC_ACTI = MKTAG('A','C','T','I'), // Activator
        REC_ADDN = MKTAG('A','D','D','N'), // Addon Node
        REC_ALCH = MKTAG('A','L','C','H'), // Potion
        REC_AMMO = MKTAG('A','M','M','O'), // Ammo
        REC_ANIO = MKTAG('A','N','I','O'), // Animated Object
        REC_APPA = MKTAG('A','P','P','A'), // Apparatus (probably unused)
        REC_ARMA = MKTAG('A','R','M','A'), // Armature (Model)
        REC_ARMO = MKTAG('A','R','M','O'), // Armor
        REC_ARTO = MKTAG('A','R','T','O'), // Art Object
        REC_ASPC = MKTAG('A','S','P','C'), // Acoustic Space
        REC_ASTP = MKTAG('A','S','T','P'), // Association Type
        REC_AVIF = MKTAG('A','V','I','F'), // Actor Values/Perk Tree Graphics
        REC_BOOK = MKTAG('B','O','O','K'), // Book
        REC_BPTD = MKTAG('B','P','T','D'), // Body Part Data
        REC_CAMS = MKTAG('C','A','M','S'), // Camera Shot
        REC_CELL = MKTAG('C','E','L','L'), // Cell
        REC_CLAS = MKTAG('C','L','A','S'), // Class
        REC_CLFM = MKTAG('C','L','F','M'), // Color
        REC_CLMT = MKTAG('C','L','M','T'), // Climate
        REC_CLOT = MKTAG('C','L','O','T'), // Clothing
        REC_COBJ = MKTAG('C','O','B','J'), // Constructible Object (recipes)
        REC_COLL = MKTAG('C','O','L','L'), // Collision Layer
        REC_CONT = MKTAG('C','O','N','T'), // Container
        REC_CPTH = MKTAG('C','P','T','H'), // Camera Path
        REC_CREA = MKTAG('C','R','E','A'), // Creature
        REC_CSTY = MKTAG('C','S','T','Y'), // Combat Style
        REC_DEBR = MKTAG('D','E','B','R'), // Debris
        REC_DIAL = MKTAG('D','I','A','L'), // Dialog Topic
        REC_DLBR = MKTAG('D','L','B','R'), // Dialog Branch
        REC_DLVW = MKTAG('D','L','V','W'), // Dialog View
        REC_DOBJ = MKTAG('D','O','B','J'), // Default Object Manager
        REC_DOOR = MKTAG('D','O','O','R'), // Door
        REC_DUAL = MKTAG('D','U','A','L'), // Dual Cast Data (possibly unused)
      //REC_ECZN = MKTAG('E','C','Z','N'), // Encounter Zone
        REC_EFSH = MKTAG('E','F','S','H'), // Effect Shader
        REC_ENCH = MKTAG('E','N','C','H'), // Enchantment
        REC_EQUP = MKTAG('E','Q','U','P'), // Equip Slot (flag-type values)
        REC_EXPL = MKTAG('E','X','P','L'), // Explosion
        REC_EYES = MKTAG('E','Y','E','S'), // Eyes
        REC_FACT = MKTAG('F','A','C','T'), // Faction
        REC_FLOR = MKTAG('F','L','O','R'), // Flora
        REC_FLST = MKTAG('F','L','S','T'), // Form List (non-leveled list)
        REC_FSTP = MKTAG('F','S','T','P'), // Footstep
        REC_FSTS = MKTAG('F','S','T','S'), // Footstep Set
        REC_FURN = MKTAG('F','U','R','N'), // Furniture
        REC_GLOB = MKTAG('G','L','O','B'), // Global Variable
        REC_GMST = MKTAG('G','M','S','T'), // Game Setting
        REC_GRAS = MKTAG('G','R','A','S'), // Grass
        REC_GRUP = MKTAG('G','R','U','P'), // Form Group
        REC_HAIR = MKTAG('H','A','I','R'), // Hair
      //REC_HAZD = MKTAG('H','A','Z','D'), // Hazard
        REC_HDPT = MKTAG('H','D','P','T'), // Head Part
        REC_IDLE = MKTAG('I','D','L','E'), // Idle Animation
        REC_IDLM = MKTAG('I','D','L','M'), // Idle Marker
        REC_IMAD = MKTAG('I','M','A','D'), // Image Space Modifier
        REC_IMGS = MKTAG('I','M','G','S'), // Image Space
        REC_INFO = MKTAG('I','N','F','O'), // Dialog Topic Info
        REC_INGR = MKTAG('I','N','G','R'), // Ingredient
        REC_IPCT = MKTAG('I','P','C','T'), // Impact Data
        REC_IPDS = MKTAG('I','P','D','S'), // Impact Data Set
        REC_KEYM = MKTAG('K','E','Y','M'), // Key
        REC_KYWD = MKTAG('K','Y','W','D'), // Keyword
        REC_LAND = MKTAG('L','A','N','D'), // Land
        REC_LCRT = MKTAG('L','C','R','T'), // Location Reference Type
        REC_LCTN = MKTAG('L','C','T','N'), // Location
        REC_LGTM = MKTAG('L','G','T','M'), // Lighting Template
        REC_LIGH = MKTAG('L','I','G','H'), // Light
        REC_LSCR = MKTAG('L','S','C','R'), // Load Screen
        REC_LTEX = MKTAG('L','T','E','X'), // Land Texture
        REC_LVLC = MKTAG('L','V','L','C'), // Leveled Creature
        REC_LVLI = MKTAG('L','V','L','I'), // Leveled Item
        REC_LVLN = MKTAG('L','V','L','N'), // Leveled Actor
        REC_LVSP = MKTAG('L','V','S','P'), // Leveled Spell
        REC_MATO = MKTAG('M','A','T','O'), // Material Object
        REC_MATT = MKTAG('M','A','T','T'), // Material Type
        REC_MESG = MKTAG('M','E','S','G'), // Message
        REC_MGEF = MKTAG('M','G','E','F'), // Magic Effect
        REC_MISC = MKTAG('M','I','S','C'), // Misc. Object
        REC_MOVT = MKTAG('M','O','V','T'), // Movement Type
        REC_MSTT = MKTAG('M','S','T','T'), // Movable Static
        REC_MUSC = MKTAG('M','U','S','C'), // Music Type
        REC_MUST = MKTAG('M','U','S','T'), // Music Track
        REC_NAVI = MKTAG('N','A','V','I'), // Navigation (master data)
        REC_NAVM = MKTAG('N','A','V','M'), // Nav Mesh
        REC_NOTE = MKTAG('N','O','T','E'), // Note
        REC_NPC_ = MKTAG('N','P','C','_'), // Actor (NPC, Creature)
        REC_OTFT = MKTAG('O','T','F','T'), // Outfit
        REC_PACK = MKTAG('P','A','C','K'), // AI Package
        REC_PERK = MKTAG('P','E','R','K'), // Perk
        REC_PGRE = MKTAG('P','G','R','E'), // Placed grenade
        REC_PHZD = MKTAG('P','H','Z','D'), // Placed hazard
        REC_PROJ = MKTAG('P','R','O','J'), // Projectile
        REC_QUST = MKTAG('Q','U','S','T'), // Quest
        REC_RACE = MKTAG('R','A','C','E'), // Race / Creature type
        REC_REFR = MKTAG('R','E','F','R'), // Object Reference
        REC_REGN = MKTAG('R','E','G','N'), // Region (Audio/Weather)
        REC_RELA = MKTAG('R','E','L','A'), // Relationship
        REC_REVB = MKTAG('R','E','V','B'), // Reverb Parameters
        REC_RFCT = MKTAG('R','F','C','T'), // Visual Effect
        REC_SBSP = MKTAG('S','B','S','P'), // Subspace (TES4 only?)
        REC_SCEN = MKTAG('S','C','E','N'), // Scene
        REC_SCRL = MKTAG('S','C','R','L'), // Scroll
        REC_SGST = MKTAG('S','G','S','T'), // Sigil Stone
        REC_SHOU = MKTAG('S','H','O','U'), // Shout
        REC_SLGM = MKTAG('S','L','G','M'), // Soul Gem
        REC_SMBN = MKTAG('S','M','B','N'), // Story Manager Branch Node
        REC_SMEN = MKTAG('S','M','E','N'), // Story Manager Event Node
        REC_SMQN = MKTAG('S','M','Q','N'), // Story Manager Quest Node
        REC_SNCT = MKTAG('S','N','C','T'), // Sound Category
        REC_SNDR = MKTAG('S','N','D','R'), // Sound Reference
        REC_SOPM = MKTAG('S','O','P','M'), // Sound Output Model
        REC_SOUN = MKTAG('S','O','U','N'), // Sound
        REC_SPEL = MKTAG('S','P','E','L'), // Spell
        REC_SPGD = MKTAG('S','P','G','D'), // Shader Particle Geometry
        REC_STAT = MKTAG('S','T','A','T'), // Static
        REC_TACT = MKTAG('T','A','C','T'), // Talking Activator
        REC_TERM = MKTAG('T','E','R','M'), // Terminal
        REC_TES4 = MKTAG('T','E','S','4'), // Plugin info
        REC_TREE = MKTAG('T','R','E','E'), // Tree
        REC_TXST = MKTAG('T','X','S','T'), // Texture Set
        REC_VTYP = MKTAG('V','T','Y','P'), // Voice Type
        REC_WATR = MKTAG('W','A','T','R'), // Water Type
        REC_WEAP = MKTAG('W','E','A','P'), // Weapon
        REC_WOOP = MKTAG('W','O','O','P'), // Word Of Power
        REC_WRLD = MKTAG('W','R','L','D'), // World Space
        REC_WTHR = MKTAG('W','T','H','R'), // Weather
        REC_ACRE = MKTAG('A','C','R','E'), // Placed Creature (TES4 only?)
        REC_PGRD = MKTAG('P','G','R','D'), // Pathgrid (TES4 only?)
        REC_ROAD = MKTAG('R','O','A','D')  // Road (TES4 only?)
    };

    enum SubRecordTypes
    {
        SUB_HEDR = MKTAG('H','E','D','R'),
        SUB_CNAM = MKTAG('C','N','A','M'),
        SUB_SNAM = MKTAG('S','N','A','M'), // TES4 only?
        SUB_MAST = MKTAG('M','A','S','T'),
        SUB_DATA = MKTAG('D','A','T','A'),
        SUB_ONAM = MKTAG('O','N','A','M'),
        SUB_INTV = MKTAG('I','N','T','V'),
        SUB_INCC = MKTAG('I','N','C','C'),
        SUB_OFST = MKTAG('O','F','S','T'), // TES4 only?
        SUB_DELE = MKTAG('D','E','L','E'), // TES4 only?

        SUB_DNAM = MKTAG('D','N','A','M'),
        SUB_EDID = MKTAG('E','D','I','D'),
        SUB_FULL = MKTAG('F','U','L','L'),
        SUB_LTMP = MKTAG('L','T','M','P'),
        SUB_MHDT = MKTAG('M','H','D','T'),
        SUB_MNAM = MKTAG('M','N','A','M'),
        SUB_MODL = MKTAG('M','O','D','L'),
        SUB_NAM0 = MKTAG('N','A','M','0'),
        SUB_NAM2 = MKTAG('N','A','M','2'),
        SUB_NAM3 = MKTAG('N','A','M','3'),
        SUB_NAM4 = MKTAG('N','A','M','4'),
        SUB_NAM9 = MKTAG('N','A','M','9'),
        SUB_NAMA = MKTAG('N','A','M','A'),
        SUB_PNAM = MKTAG('P','N','A','M'),
        SUB_RNAM = MKTAG('R','N','A','M'),
        SUB_TNAM = MKTAG('T','N','A','M'),
        SUB_UNAM = MKTAG('U','N','A','M'),
        SUB_WCTR = MKTAG('W','C','T','R'),
        SUB_WNAM = MKTAG('W','N','A','M'),
        SUB_XEZN = MKTAG('X','E','Z','N'),
        SUB_XLCN = MKTAG('X','L','C','N'),
        SUB_XXXX = MKTAG('X','X','X','X'),
        SUB_ZNAM = MKTAG('Z','N','A','M'),
        SUB_MODT = MKTAG('M','O','D','T'),
        SUB_ICON = MKTAG('I','C','O','N'), // TES4 only?

        SUB_NVER = MKTAG('N','V','E','R'),
        SUB_NVMI = MKTAG('N','V','M','I'),
        SUB_NVPP = MKTAG('N','V','P','P'),
        SUB_NVSI = MKTAG('N','V','S','I'),

        SUB_NVNM = MKTAG('N','V','N','M'),
        SUB_NNAM = MKTAG('N','N','A','M'),

        SUB_XCLC = MKTAG('X','C','L','C'),
        SUB_XCLL = MKTAG('X','C','L','L'),
        SUB_TVDT = MKTAG('T','V','D','T'),
        SUB_XCGD = MKTAG('X','C','G','D'),
        SUB_LNAM = MKTAG('L','N','A','M'),
        SUB_XCLW = MKTAG('X','C','L','W'),
        SUB_XNAM = MKTAG('X','N','A','M'),
        SUB_XCLR = MKTAG('X','C','L','R'),
        SUB_XWCS = MKTAG('X','W','C','S'),
        SUB_XWCN = MKTAG('X','W','C','N'),
        SUB_XWCU = MKTAG('X','W','C','U'),
        SUB_XCWT = MKTAG('X','C','W','T'),
        SUB_XOWN = MKTAG('X','O','W','N'),
        SUB_XILL = MKTAG('X','I','L','L'),
        SUB_XWEM = MKTAG('X','W','E','M'),
        SUB_XCCM = MKTAG('X','C','C','M'),
        SUB_XCAS = MKTAG('X','C','A','S'),
        SUB_XCMO = MKTAG('X','C','M','O'),
        SUB_XCIM = MKTAG('X','C','I','M'),
        SUB_XCMT = MKTAG('X','C','M','T'), // TES4 only?
        SUB_XRNK = MKTAG('X','R','N','K'), // TES4 only?
        SUB_XGLB = MKTAG('X','G','L','B'), // TES4 only?

        SUB_VNML = MKTAG('V','N','M','L'),
        SUB_VHGT = MKTAG('V','H','G','T'),
        SUB_VCLR = MKTAG('V','C','L','R'),
        SUA_BTXT = MKTAG('B','T','X','T'),
        SUB_ATXT = MKTAG('A','T','X','T'),
        SUB_VTXT = MKTAG('V','T','X','T'),
        SUB_VTEX = MKTAG('V','T','E','X'),

        SUB_HNAM = MKTAG('H','N','A','M'),
        SUB_GNAM = MKTAG('G','N','A','M'),

        SUB_RCLR = MKTAG('R','C','L','R'),
        SUB_RPLI = MKTAG('R','P','L','I'),
        SUB_RPLD = MKTAG('R','P','L','D'),
        SUB_RDAT = MKTAG('R','D','A','T'),
        SUB_RDMD = MKTAG('R','D','M','D'), // TES4 only?
        SUB_RDSD = MKTAG('R','D','S','D'), // TES4 only?
        SUB_RDGS = MKTAG('R','D','G','S'), // TES4 only?
        SUB_RDMO = MKTAG('R','D','M','O'),
        SUB_RDSA = MKTAG('R','D','S','A'),
        SUB_RDWT = MKTAG('R','D','W','T'),
        SUB_RDOT = MKTAG('R','D','O','T'),
        SUB_RDMP = MKTAG('R','D','M','P'),

        SUB_MODB = MKTAG('M','O','D','B'),
        SUB_OBND = MKTAG('O','B','N','D'),
        SUB_MODS = MKTAG('M','O','D','S'),

        SUB_NAME = MKTAG('N','A','M','E'),
        SUB_XMRK = MKTAG('X','M','R','K'),
        SUB_FNAM = MKTAG('F','N','A','M'),
        SUB_XSCL = MKTAG('X','S','C','L'),
        SUB_XTEL = MKTAG('X','T','E','L'),
        SUB_XTRG = MKTAG('X','T','R','G'),
        SUB_XSED = MKTAG('X','S','E','D'),
        SUB_XLOD = MKTAG('X','L','O','D'),
        SUB_XPCI = MKTAG('X','P','C','I'),
        SUB_XLOC = MKTAG('X','L','O','C'),
        SUB_XESP = MKTAG('X','E','S','P'),
        SUB_XLCM = MKTAG('X','L','C','M'),
        SUB_XRTM = MKTAG('X','R','T','M'),
        SUB_XACT = MKTAG('X','A','C','T'),
        SUB_XCNT = MKTAG('X','C','N','T'),
        SUB_VMAD = MKTAG('V','M','A','D'),
        SUB_XPRM = MKTAG('X','P','R','M'),
        SUB_XMBO = MKTAG('X','M','B','O'),
        SUB_XPOD = MKTAG('X','P','O','D'),
        SUB_XRMR = MKTAG('X','R','M','R'),
        SUB_INAM = MKTAG('I','N','A','M'),
        SUB_SCHR = MKTAG('S','C','H','R'),
        SUB_XLRM = MKTAG('X','L','R','M'),
        SUB_XRGD = MKTAG('X','R','G','D'),
        SUB_XRDS = MKTAG('X','R','D','S'),
        SUB_XEMI = MKTAG('X','E','M','I'),
        SUB_XLIG = MKTAG('X','L','I','G'),
        SUB_XALP = MKTAG('X','A','L','P'),
        SUB_XNDP = MKTAG('X','N','D','P'),
        SUB_XAPD = MKTAG('X','A','P','D'),
        SUB_XAPR = MKTAG('X','A','P','R'),
        SUB_XLIB = MKTAG('X','L','I','B'),
        SUB_XLKR = MKTAG('X','L','K','R'),
        SUB_XLRT = MKTAG('X','L','R','T'),
        SUB_XCVL = MKTAG('X','C','V','L'),
        SUB_XCVR = MKTAG('X','C','V','R'),
        SUB_XCZA = MKTAG('X','C','Z','A'),
        SUB_XCZC = MKTAG('X','C','Z','C'),
        SUB_XFVC = MKTAG('X','F','V','C'),
        SUB_XHTW = MKTAG('X','H','T','W'),
        SUB_XIS2 = MKTAG('X','I','S','2'),
        SUB_XMBR = MKTAG('X','M','B','R'),
        SUB_XCCP = MKTAG('X','C','C','P'),
        SUB_XPWR = MKTAG('X','P','W','R'),
        SUB_XTRI = MKTAG('X','T','R','I'),
        SUB_XATR = MKTAG('X','A','T','R'),
        SUB_XPRD = MKTAG('X','P','R','D'),
        SUB_XPPA = MKTAG('X','P','P','A'),
        SUB_PDTO = MKTAG('P','D','T','O'),
        SUB_XLRL = MKTAG('X','L','R','L'),

        SUB_QNAM = MKTAG('Q','N','A','M'),
        SUB_COCT = MKTAG('C','O','C','T'),
        SUB_COED = MKTAG('C','O','E','D'),
        SUB_CNTO = MKTAG('C','N','T','O'),
        SUB_SCRI = MKTAG('S','C','R','I'),

        SUB_BNAM = MKTAG('B','N','A','M'),

        SUB_BMDT = MKTAG('B','M','D','T'),
        SUB_MOD2 = MKTAG('M','O','D','2'),
        SUB_MOD3 = MKTAG('M','O','D','3'),
        SUB_MOD4 = MKTAG('M','O','D','4'),
        SUB_MO2B = MKTAG('M','O','2','B'),
        SUB_MO3B = MKTAG('M','O','3','B'),
        SUB_MO4B = MKTAG('M','O','4','B'),
        SUB_MO2T = MKTAG('M','O','2','T'),
        SUB_MO3T = MKTAG('M','O','3','T'),
        SUB_MO4T = MKTAG('M','O','4','T'),
        SUB_ANAM = MKTAG('A','N','A','M'),
        SUB_ENAM = MKTAG('E','N','A','M'),
        SUB_ICO2 = MKTAG('I','C','O','2'),

        SUB_ACBS = MKTAG('A','C','B','S'),
        SUB_SPLO = MKTAG('S','P','L','O'),
        SUB_AIDT = MKTAG('A','I','D','T'),
        SUB_PKID = MKTAG('P','K','I','D'),
        SUB_HCLR = MKTAG('H','C','L','R'),
        SUB_FGGS = MKTAG('F','G','G','S'),
        SUB_FGGA = MKTAG('F','G','G','A'),
        SUB_FGTS = MKTAG('F','G','T','S'),
        SUB_KFFZ = MKTAG('K','F','F','Z'),

        SUB_PFIG = MKTAG('P','F','I','G'),
        SUB_PFPC = MKTAG('P','F','P','C'),

        SUB_XHRS = MKTAG('X','H','R','S'),
        SUB_XMRC = MKTAG('X','M','R','C'),

        SUB_SNDD = MKTAG('S','N','D','D'),
        SUB_SNDX = MKTAG('S','N','D','X'),

        SUB_DESC = MKTAG('D','E','S','C'),

        SUB_ENIT = MKTAG('E','N','I','T'),
        SUB_EFID = MKTAG('E','F','I','D'),
        SUB_EFIT = MKTAG('E','F','I','T'),
        SUB_SCIT = MKTAG('S','C','I','T'),

        SUB_SOUL = MKTAG('S','O','U','L'),
        SUB_SLCP = MKTAG('S','L','C','P'),

        SUB_CSCR = MKTAG('C','S','C','R'),
        SUB_CSDI = MKTAG('C','S','D','I'),
        SUB_CSDC = MKTAG('C','S','D','C'),
        SUB_NIFZ = MKTAG('N','I','F','Z'),
        SUB_CSDT = MKTAG('C','S','D','T'),
        SUB_NAM1 = MKTAG('N','A','M','1'),
        SUB_NIFT = MKTAG('N','I','F','T'),

        SUB_LVLD = MKTAG('L','V','L','D'),
        SUB_LVLF = MKTAG('L','V','L','F'),
        SUB_LVLO = MKTAG('L','V','L','O'),

        SUB_BODT = MKTAG('B','O','D','T'),
        SUB_YNAM = MKTAG('Y','N','A','M'),
        SUB_DEST = MKTAG('D','E','S','T'),
        SUB_DMDL = MKTAG('D','M','D','L'),
        SUB_DMDS = MKTAG('D','M','D','S'),
        SUB_DMDT = MKTAG('D','M','D','T'),
        SUB_DSTD = MKTAG('D','S','T','D'),
        SUB_DSTF = MKTAG('D','S','T','F'),
        SUB_KNAM = MKTAG('K','N','A','M'),
        SUB_KSIZ = MKTAG('K','S','I','Z'),
        SUB_KWDA = MKTAG('K','W','D','A'),
        SUB_VNAM = MKTAG('V','N','A','M'),
        SUB_SDSC = MKTAG('S','D','S','C'),
        SUB_MO2S = MKTAG('M','O','2','S'),
        SUB_MO4S = MKTAG('M','O','4','S'),
        SUB_BOD2 = MKTAG('B','O','D','2'),
        SUB_BAMT = MKTAG('B','A','M','T'),
        SUB_BIDS = MKTAG('B','I','D','S'),
        SUB_ETYP = MKTAG('E','T','Y','P'),
        SUB_BMCT = MKTAG('B','M','C','T'),
        SUB_MICO = MKTAG('M','I','C','O'),
        SUB_MIC2 = MKTAG('M','I','C','2'),
        SUB_EAMT = MKTAG('E','A','M','T'),
        SUB_EITM = MKTAG('E','I','T','M'),

        SUB_SCTX = MKTAG('S','C','T','X'),
        SUB_XLTW = MKTAG('X','L','T','W'),
        SUB_XMBP = MKTAG('X','M','B','P'),
        SUB_XOCP = MKTAG('X','O','C','P'),
        SUB_XRGB = MKTAG('X','R','G','B'),
        SUB_XSPC = MKTAG('X','S','P','C'),
        SUB_XTNM = MKTAG('X','T','N','M'),
        SUB_ATKR = MKTAG('A','T','K','R'),
        SUB_CRIF = MKTAG('C','R','I','F'),
        SUB_DOFT = MKTAG('D','O','F','T'),
        SUB_DPLT = MKTAG('D','P','L','T'),
        SUB_ECOR = MKTAG('E','C','O','R'),
        SUB_ATKD = MKTAG('A','T','K','D'),
        SUB_ATKE = MKTAG('A','T','K','E'),
        SUB_FTST = MKTAG('F','T','S','T'),
        SUB_HCLF = MKTAG('H','C','L','F'),
        SUB_NAM5 = MKTAG('N','A','M','5'),
        SUB_NAM6 = MKTAG('N','A','M','6'),
        SUB_NAM7 = MKTAG('N','A','M','7'),
        SUB_NAM8 = MKTAG('N','A','M','8'),
        SUB_PRKR = MKTAG('P','R','K','R'),
        SUB_PRKZ = MKTAG('P','R','K','Z'),
        SUB_SOFT = MKTAG('S','O','F','T'),
        SUB_SPCT = MKTAG('S','P','C','T'),
        SUB_TINC = MKTAG('T','I','N','C'),
        SUB_TIAS = MKTAG('T','I','A','S'),
        SUB_TINI = MKTAG('T','I','N','I'),
        SUB_TINV = MKTAG('T','I','N','V'),
        SUB_TPLT = MKTAG('T','P','L','T'),
        SUB_VTCK = MKTAG('V','T','C','K'),
        SUB_SHRT = MKTAG('S','H','R','T'),
        SUB_SPOR = MKTAG('S','P','O','R'),
        SUB_XHOR = MKTAG('X','H','O','R'),
        SUB_CTDA = MKTAG('C','T','D','A'),
        SUB_CRDT = MKTAG('C','R','D','T'),
        SUB_FNMK = MKTAG('F','N','M','K'),
        SUB_FNPR = MKTAG('F','N','P','R'),
        SUB_WBDT = MKTAG('W','B','D','T'),
        SUB_QUAL = MKTAG('Q','U','A','L'),
        SUB_INDX = MKTAG('I','N','D','X'),
        SUB_ATTR = MKTAG('A','T','T','R'),
        SUB_MTNM = MKTAG('M','T','N','M'),
        SUB_UNES = MKTAG('U','N','E','S'),
        SUB_TIND = MKTAG('T','I','N','D'),
        SUB_TINL = MKTAG('T','I','N','L'),
        SUB_TINP = MKTAG('T','I','N','P'),
        SUB_TINT = MKTAG('T','I','N','T'),
        SUB_TIRS = MKTAG('T','I','R','S'),
        SUB_PHWT = MKTAG('P','H','W','T'),
        SUB_AHCF = MKTAG('A','H','C','F'),
        SUB_AHCM = MKTAG('A','H','C','M'),
        SUB_HEAD = MKTAG('H','E','A','D'),
        SUB_MPAI = MKTAG('M','P','A','I'),
        SUB_MPAV = MKTAG('M','P','A','V'),
        SUB_DFTF = MKTAG('D','F','T','F'),
        SUB_DFTM = MKTAG('D','F','T','M'),
        SUB_FLMV = MKTAG('F','L','M','V'),
        SUB_FTSF = MKTAG('F','T','S','F'),
        SUB_FTSM = MKTAG('F','T','S','M'),
        SUB_MTYP = MKTAG('M','T','Y','P'),
        SUB_PHTN = MKTAG('P','H','T','N'),
        SUB_RNMV = MKTAG('R','N','M','V'),
        SUB_RPRF = MKTAG('R','P','R','F'),
        SUB_RPRM = MKTAG('R','P','R','M'),
        SUB_SNMV = MKTAG('S','N','M','V'),
        SUB_SPED = MKTAG('S','P','E','D'),
        SUB_SWMV = MKTAG('S','W','M','V'),
        SUB_WKMV = MKTAG('W','K','M','V'),
        SUB_LLCT = MKTAG('L','L','C','T'),
        SUB_IDLF = MKTAG('I','D','L','F'),
        SUB_IDLA = MKTAG('I','D','L','A'),
        SUB_IDLC = MKTAG('I','D','L','C'),
        SUB_IDLT = MKTAG('I','D','L','T'),
        SUB_DODT = MKTAG('D','O','D','T'),
        SUB_TX00 = MKTAG('T','X','0','0'),
        SUB_TX01 = MKTAG('T','X','0','1'),
        SUB_TX02 = MKTAG('T','X','0','2'),
        SUB_TX03 = MKTAG('T','X','0','3'),
        SUB_TX04 = MKTAG('T','X','0','4'),
        SUB_TX05 = MKTAG('T','X','0','5'),
        SUB_TX06 = MKTAG('T','X','0','6'),
        SUB_TX07 = MKTAG('T','X','0','7'),
        SUB_BPND = MKTAG('B','P','N','D'),
        SUB_BPTN = MKTAG('B','P','T','N'),
        SUB_BPNN = MKTAG('B','P','N','N'),
        SUB_BPNT = MKTAG('B','P','N','T'),
        SUB_BPNI = MKTAG('B','P','N','I'),
        SUB_RAGA = MKTAG('R','A','G','A'),

        SUB_XHLT = MKTAG('X','H','L','T'), // Unofficial Oblivion Patch
        SUB_XCHG = MKTAG('X','C','H','G'), // thievery.exp

        SUB_ITXT = MKTAG('I','T','X','T'),
        SUB_MO5T = MKTAG('M','O','5','T'),
        SUB_MOD5 = MKTAG('M','O','D','5'),
        SUB_MDOB = MKTAG('M','D','O','B'),
        SUB_SPIT = MKTAG('S','P','I','T'),
        SUB_XIBS = MKTAG('X','I','B','S'), // FO3
        SUB_REPL = MKTAG('R','E','P','L'), // FO3
        SUB_BIPL = MKTAG('B','I','P','L'), // FO3
        SUB_MODD = MKTAG('M','O','D','D'), // FO3
        SUB_MOSD = MKTAG('M','O','S','D'), // FO3
        SUB_MO3S = MKTAG('M','O','3','S'), // FO3
        SUB_XCET = MKTAG('X','C','E','T'), // FO3
        SUB_LVLG = MKTAG('L','V','L','G'), // FO3
        SUB_NVCI = MKTAG('N','V','C','I'), // FO3
        SUB_NVVX = MKTAG('N','V','V','X'), // FO3
        SUB_NVTR = MKTAG('N','V','T','R'), // FO3
        SUB_NVCA = MKTAG('N','V','C','A'), // FO3
        SUB_NVDP = MKTAG('N','V','D','P'), // FO3
        SUB_NVGD = MKTAG('N','V','G','D'), // FO3
        SUB_NVEX = MKTAG('N','V','E','X'), // FO3
        SUB_XHLP = MKTAG('X','H','L','P'), // FO3
        SUB_XRDO = MKTAG('X','R','D','O'), // FO3
        SUB_XAMT = MKTAG('X','A','M','T'), // FO3
        SUB_XAMC = MKTAG('X','A','M','C'), // FO3
        SUB_XRAD = MKTAG('X','R','A','D'), // FO3
        SUB_XORD = MKTAG('X','O','R','D'), // FO3
        SUB_XCLP = MKTAG('X','C','L','P'), // FO3
        SUB_SCDA = MKTAG('S','C','D','A'), // FO3
        SUB_SCRO = MKTAG('S','C','R','O'), // FO3
        SUB_IMPS = MKTAG('I','M','P','S'), // FO3 Anchorage
        SUB_IMPF = MKTAG('I','M','P','F'), // FO3 Anchorage

        SUB_XATO = MKTAG('X','A','T','O'), // FONV
        SUB_DAT2 = MKTAG('D','A','T','2'), // FONV
        SUB_RCIL = MKTAG('R','C','I','L'), // FONV
        SUB_MMRK = MKTAG('M','M','R','K'), // FONV
        SUB_SCRV = MKTAG('S','C','R','V'), // FONV
        SUB_SCVR = MKTAG('S','C','V','R'), // FONV
        SUB_SLSD = MKTAG('S','L','S','D'), // FONV
        SUB_XSRF = MKTAG('X','S','R','F'), // FONV
        SUB_XSRD = MKTAG('X','S','R','D'), // FONV
        SUB_WMI1 = MKTAG('W','M','I','1'), // FONV
        SUB_RDID = MKTAG('R','D','I','D'), // FONV
        SUB_RDSB = MKTAG('R','D','S','B'), // FONV
        SUB_RDSI = MKTAG('R','D','S','I'), // FONV
        SUB_BRUS = MKTAG('B','R','U','S'), // FONV
        SUB_VATS = MKTAG('V','A','T','S'), // FONV
        SUB_VANM = MKTAG('V','A','N','M'), // FONV
        SUB_MWD1 = MKTAG('M','W','D','1'), // FONV
        SUB_MWD2 = MKTAG('M','W','D','2'), // FONV
        SUB_MWD3 = MKTAG('M','W','D','3'), // FONV
        SUB_MWD4 = MKTAG('M','W','D','4'), // FONV
        SUB_MWD5 = MKTAG('M','W','D','5'), // FONV
        SUB_MWD6 = MKTAG('M','W','D','6'), // FONV
        SUB_MWD7 = MKTAG('M','W','D','7'), // FONV
        SUB_WMI2 = MKTAG('W','M','I','2'), // FONV
        SUB_WMI3 = MKTAG('W','M','I','3'), // FONV
        SUB_WMS1 = MKTAG('W','M','S','1'), // FONV
        SUB_WMS2 = MKTAG('W','M','S','2'), // FONV
        SUB_WNM1 = MKTAG('W','N','M','1'), // FONV
        SUB_WNM2 = MKTAG('W','N','M','2'), // FONV
        SUB_WNM3 = MKTAG('W','N','M','3'), // FONV
        SUB_WNM4 = MKTAG('W','N','M','4'), // FONV
        SUB_WNM5 = MKTAG('W','N','M','5'), // FONV
        SUB_WNM6 = MKTAG('W','N','M','6'), // FONV
        SUB_WNM7 = MKTAG('W','N','M','7'), // FONV
        SUB_EFSD = MKTAG('E','F','S','D'), // FONV DeadMoney
    };

    enum MagicEffectID
    {
        // Alteration
        EFI_BRDN = MKTAG('B','R','D','N'),
        EFI_FTHR = MKTAG('F','T','H','R'),
        EFI_FISH = MKTAG('F','I','S','H'),
        EFI_FRSH = MKTAG('F','R','S','H'),
        EFI_OPEN = MKTAG('O','P','N','N'),
        EFI_SHLD = MKTAG('S','H','L','D'),
        EFI_LISH = MKTAG('L','I','S','H'),
        EFI_WABR = MKTAG('W','A','B','R'),
        EFI_WAWA = MKTAG('W','A','W','A'),

        // Conjuration
        EFI_BABO = MKTAG('B','A','B','O'), // Bound Boots
        EFI_BACU = MKTAG('B','A','C','U'), // Bound Cuirass
        EFI_BAGA = MKTAG('B','A','G','A'), // Bound Gauntlets
        EFI_BAGR = MKTAG('B','A','G','R'), // Bound Greaves
        EFI_BAHE = MKTAG('B','A','H','E'), // Bound Helmet
        EFI_BASH = MKTAG('B','A','S','H'), // Bound Shield
        EFI_BWAX = MKTAG('B','W','A','X'), // Bound Axe
        EFI_BWBO = MKTAG('B','W','B','O'), // Bound Bow
        EFI_BWDA = MKTAG('B','W','D','A'), // Bound Dagger
        EFI_BWMA = MKTAG('B','W','M','A'), // Bound Mace
        EFI_BWSW = MKTAG('B','W','S','W'), // Bound Sword
        EFI_Z001 = MKTAG('Z','0','0','1'), // Summon Rufio's Ghost
        EFI_Z002 = MKTAG('Z','0','0','2'), // Summon Ancestor Guardian
        EFI_Z003 = MKTAG('Z','0','0','3'), // Summon Spiderling
        EFI_Z005 = MKTAG('Z','0','0','5'), // Summon Bear
        EFI_ZCLA = MKTAG('Z','C','L','A'), // Summon Clannfear
        EFI_ZDAE = MKTAG('Z','D','A','E'), // Summon Daedroth
        EFI_ZDRE = MKTAG('Z','D','R','E'), // Summon Dremora
        EFI_ZDRL = MKTAG('Z','D','R','L'), // Summon Dremora Lord
        EFI_ZFIA = MKTAG('Z','F','I','A'), // Summon Flame Atronach
        EFI_ZFRA = MKTAG('Z','F','R','A'), // Summon Frost Atronach
        EFI_ZGHO = MKTAG('Z','G','H','O'), // Summon Ghost
        EFI_ZHDZ = MKTAG('Z','H','D','Z'), // Summon Headless Zombie
        EFI_ZLIC = MKTAG('Z','L','I','C'), // Summon Lich
        EFI_ZSCA = MKTAG('Z','S','C','A'), // Summon Scamp
        EFI_ZSKE = MKTAG('Z','S','K','E'), // Summon Skeleton
        EFI_ZSKA = MKTAG('Z','S','K','A'), // Summon Skeleton Guardian
        EFI_ZSKH = MKTAG('Z','S','K','H'), // Summon Skeleton Hero
        EFI_ZSKC = MKTAG('Z','S','K','C'), // Summon Skeleton Champion
        EFI_ZSPD = MKTAG('Z','S','P','D'), // Summon Spider Daedra
        EFI_ZSTA = MKTAG('Z','S','T','A'), // Summon Storm Atronach
        EFI_ZWRA = MKTAG('Z','W','R','A'), // Summon Faded Wraith
        EFI_ZWRL = MKTAG('Z','W','R','L'), // Summon Gloom Wraith
        EFI_ZXIV = MKTAG('Z','X','I','V'), // Summon Xivilai
        EFI_ZZOM = MKTAG('Z','Z','O','M'), // Summon Zombie
        EFI_TURN = MKTAG('T','U','R','N'), // Turn Undead

        // Destruction
        EFI_DGAT = MKTAG('D','G','A','T'), // Damage Attribute
        EFI_DGFA = MKTAG('D','G','F','A'), // Damage Fatigue
        EFI_DGHE = MKTAG('D','G','H','E'), // Damage Health
        EFI_DGSP = MKTAG('D','G','S','P'), // Damage Magicka
        EFI_DIAR = MKTAG('D','I','A','R'), // Disintegrate Armor
        EFI_DIWE = MKTAG('D','I','W','E'), // Disintegrate Weapon
        EFI_DRAT = MKTAG('D','R','A','T'), // Drain Attribute
        EFI_DRFA = MKTAG('D','R','F','A'), // Drain Fatigue
        EFI_DRHE = MKTAG('D','R','H','E'), // Drain Health
        EFI_DRSP = MKTAG('D','R','S','P'), // Drain Magicka
        EFI_DRSK = MKTAG('D','R','S','K'), // Drain Skill
        EFI_FIDG = MKTAG('F','I','D','G'), // Fire Damage
        EFI_FRDG = MKTAG('F','R','D','G'), // Frost Damage
        EFI_SHDG = MKTAG('S','H','D','G'), // Shock Damage
        EFI_WKDI = MKTAG('W','K','D','I'), // Weakness to Disease
        EFI_WKFI = MKTAG('W','K','F','I'), // Weakness to Fire
        EFI_WKFR = MKTAG('W','K','F','R'), // Weakness to Frost
        EFI_WKMA = MKTAG('W','K','M','A'), // Weakness to Magic
        EFI_WKNW = MKTAG('W','K','N','W'), // Weakness to Normal Weapons
        EFI_WKPO = MKTAG('W','K','P','O'), // Weakness to Poison
        EFI_WKSH = MKTAG('W','K','S','H'), // Weakness to Shock

        // Illusion
        EFI_CALM = MKTAG('C','A','L','M'), // Calm
        EFI_CHML = MKTAG('C','H','M','L'), // Chameleon
        EFI_CHRM = MKTAG('C','H','R','M'), // Charm
        EFI_COCR = MKTAG('C','O','C','R'), // Command Creature
        EFI_COHU = MKTAG('C','O','H','U'), // Command Humanoid
        EFI_DEMO = MKTAG('D','E','M','O'), // Demoralize
        EFI_FRNZ = MKTAG('F','R','N','Z'), // Frenzy
        EFI_INVI = MKTAG('I','N','V','I'), // Invisibility
        EFI_LGHT = MKTAG('L','G','H','T'), // Light
        EFI_NEYE = MKTAG('N','E','Y','E'), // Night-Eye
        EFI_PARA = MKTAG('P','A','R','A'), // Paralyze
        EFI_RALY = MKTAG('R','A','L','Y'), // Rally
        EFI_SLNC = MKTAG('S','L','N','C'), // Silence

        // Mysticism
        EFI_DTCT = MKTAG('D','T','C','T'), // Detect Life
        EFI_DSPL = MKTAG('D','S','P','L'), // Dispel
        EFI_REDG = MKTAG('R','E','D','G'), // Reflect Damage
        EFI_RFLC = MKTAG('R','F','L','C'), // Reflect Spell
        EFI_STRP = MKTAG('S','T','R','P'), // Soul Trap
        EFI_SABS = MKTAG('S','A','B','S'), // Spell Absorption
        EFI_TELE = MKTAG('T','E','L','E'), // Telekinesis

        // Restoration
        EFI_ABAT = MKTAG('A','B','A','T'), // Absorb Attribute
        EFI_ABFA = MKTAG('A','B','F','A'), // Absorb Fatigue
        EFI_ABHe = MKTAG('A','B','H','e'), // Absorb Health
        EFI_ABSP = MKTAG('A','B','S','P'), // Absorb Magicka
        EFI_ABSK = MKTAG('A','B','S','K'), // Absorb Skill
        EFI_1400 = MKTAG('1','4','0','0'), // Cure Disease
        EFI_CUPA = MKTAG('C','U','P','A'), // Cure Paralysis
        EFI_CUPO = MKTAG('C','U','P','O'), // Cure Poison
        EFI_FOAT = MKTAG('F','O','A','T'), // Fortify Attribute
        EFI_FOFA = MKTAG('F','O','F','A'), // Fortify Fatigue
        EFI_FOHE = MKTAG('F','O','H','E'), // Fortify Health
        EFI_FOSP = MKTAG('F','O','S','P'), // Fortify Magicka
        EFI_FOSK = MKTAG('F','O','S','K'), // Fortify Skill
        EFI_RSDI = MKTAG('R','S','D','I'), // Resist Disease
        EFI_RSFI = MKTAG('R','S','F','I'), // Resist Fire
        EFI_RSFR = MKTAG('R','S','F','R'), // Resist Frost
        EFI_RSMA = MKTAG('R','S','M','A'), // Resist Magic
        EFI_RSNW = MKTAG('R','S','N','W'), // Resist Normal Weapons
        EFI_RSPA = MKTAG('R','S','P','A'), // Resist Paralysis
        EFI_RSPO = MKTAG('R','S','P','O'), // Resist Poison
        EFI_RSSH = MKTAG('R','S','S','H'), // Resist Shock
        EFI_REAT = MKTAG('R','E','A','T'), // Restore Attribute
        EFI_REFA = MKTAG('R','E','F','A'), // Restore Fatigue
        EFI_REHE = MKTAG('R','E','H','E'), // Restore Health
        EFI_RESP = MKTAG('R','E','S','P'), // Restore Magicka

        // Effects
        EFI_LOCK = MKTAG('L','O','C','K'), // Lock Lock
        EFI_SEFF = MKTAG('S','E','F','F'), // Script Effect
        EFI_Z020 = MKTAG('Z','0','2','0'), // Summon 20 Extra
        EFI_MYHL = MKTAG('M','Y','H','L'), // Summon Mythic Dawn Helmet
        EFI_MYTH = MKTAG('M','Y','T','H'), // Summon Mythic Dawn Armor
        EFI_REAN = MKTAG('R','E','A','N'), // Reanimate
        EFI_DISE = MKTAG('D','I','S','E'), // Disease Info
        EFI_POSN = MKTAG('P','O','S','N'), // Poison Info
        EFI_DUMY = MKTAG('D','U','M','Y'), // Mehrunes Dagon Custom Effect
        EFI_STMA = MKTAG('S','T','M','A'), // Stunted Magicka
        EFI_SUDG = MKTAG('S','U','D','G'), // Sun Damage
        EFI_VAMP = MKTAG('V','A','M','P'), // Vampirism
        EFI_DARK = MKTAG('D','A','R','K'), // Darkness
        EFI_RSWD = MKTAG('R','S','W','D')  // Resist Water Damage
    };

    // Based on http://www.uesp.net/wiki/Tes5Mod:Mod_File_Format#Groups
    enum GroupType
    {
        Grp_RecordType           = 0,
        Grp_WorldChild           = 1,
        Grp_InteriorCell         = 2,
        Grp_InteriorSubCell      = 3,
        Grp_ExteriorCell         = 4,
        Grp_ExteriorSubCell      = 5,
        Grp_CellChild            = 6,
        Grp_TopicChild           = 7,
        Grp_CellPersistentChild  = 8,
        Grp_CellTemporaryChild   = 9,
        Grp_CellVisibleDistChild = 10
    };

    // Based on http://www.uesp.net/wiki/Tes5Mod:Mod_File_Format#Records
    enum RecordFlag
    {
        Rec_ESM        = 0x00000001, // (TES4 record only) Master (ESM) file.
        Rec_Deleted    = 0x00000020, // Deleted
        Rec_Constant   = 0x00000040, // Constant
        Rec_HiddenLMap = 0x00000040, // (REFR) Hidden From Local Map (Needs Confirmation: Related to shields)
        Rec_Localized  = 0x00000080, // (TES4 record only) Is localized. This will make Skyrim load the
                                     //   .STRINGS, .DLSTRINGS, and .ILSTRINGS files associated with the mod.
                                     //   If this flag is not set, lstrings are treated as zstrings.
        Rec_FireOff    = 0x00000080, // (PHZD) Turn off fire
        Rec_UpdateAnim = 0x00000100, // Must Update Anims
        Rec_NoAccess   = 0x00000100, // (REFR) Inaccessible
        Rec_Hidden     = 0x00000200, // (REFR) Hidden from local map
        Rec_StartDead  = 0x00000200, // (ACHR) Starts dead /(REFR) MotionBlurCastsShadows
        Rec_Persistent = 0x00000400, // Quest item / Persistent reference
        Rec_DispMenu   = 0x00000400, // (LSCR) Displays in Main Menu
        Rec_Disabled   = 0x00000800, // Initially disabled
        Rec_Ignored    = 0x00001000, // Ignored
        Rec_DistVis    = 0x00008000, // Visible when distant
        Rec_RandAnim   = 0x00010000, // (ACTI) Random Animation Start
        Rec_Danger     = 0x00020000, // (ACTI) Dangerous / Off limits (Interior cell)
                                     //   Dangerous Can't be set withough Ignore Object Interaction
        Rec_Compressed = 0x00040000, // Data is compressed
        Rec_CanNotWait = 0x00080000, // Can't wait
        Rec_IgnoreObj  = 0x00100000, // (ACTI) Ignore Object Interaction
                                     //   Ignore Object Interaction Sets Dangerous Automatically
        Rec_Marker     = 0x00800000, // Is Marker
        Rec_Obstacle   = 0x02000000, // (ACTI) Obstacle / (REFR) No AI Acquire
        Rec_NavMFilter = 0x04000000, // NavMesh Gen - Filter
        Rec_NavMBBox   = 0x08000000, // NavMesh Gen - Bounding Box
        Rec_ExitToTalk = 0x10000000, // (FURN) Must Exit to Talk
        Rec_Refected   = 0x10000000, // (REFR) Reflected By Auto Water
        Rec_ChildUse   = 0x20000000, // (FURN/IDLM) Child Can Use
        Rec_NoHavok    = 0x20000000, // (REFR) Don't Havok Settle
        Rec_NavMGround = 0x40000000, // NavMesh Gen - Ground
        Rec_NoRespawn  = 0x40000000, // (REFR) NoRespawn
        Rec_MultiBound = 0x80000000  // (REFR) MultiBound
    };

    typedef std::uint32_t FormId;

#pragma pack(push, 1)
    // NOTE: the label field of a group is not reliable (http://www.uesp.net/wiki/Tes4Mod:Mod_File_Format)
    union GroupLabel
    {
        std::uint32_t value;     // formId, blockNo or raw int representation of type
        char recordType[4];      // record type in ascii
        std::int16_t grid[2];    // grid y, x (note the reverse order)
    };

    union TypeId
    {
        std::uint32_t value;
        char name[4];            // record type in ascii
    };

    struct GroupTypeHeader
    {
        std::uint32_t typeId;
        std::uint32_t groupSize; // includes the 24 bytes (20 for TES4) of header (i.e. this struct)
        GroupLabel    label;     // format based on type
        std::int32_t  type;
        std::uint16_t stamp;     // & 0xff for day, & 0xff00 for months since Dec 2002 (i.e. 1 = Jan 2003)
        std::uint16_t unknown;
        std::uint16_t version;   // not in TES4
        std::uint16_t unknown2;  // not in TES4
    };

    struct RecordTypeHeader
    {
        std::uint32_t typeId;
        std::uint32_t dataSize;  // does *not* include 24 bytes (20 for TES4) of header
        std::uint32_t flags;
        FormId        id;
        std::uint32_t revision;
        std::uint16_t version;  // not in TES4
        std::uint16_t unknown;  // not in TES4
    };

    union RecordHeader
    {
        struct GroupTypeHeader  group;
        struct RecordTypeHeader record;
    };

    struct SubRecordHeader
    {
        std::uint32_t typeId;
        std::uint16_t dataSize;
    };

    // Grid, CellGrid and Vertex are shared by NVMI(NAVI) and NVNM(NAVM)

    struct Grid
    {
        std::int16_t x;
        std::int16_t y;
    };

    union CellGrid
    {
        FormId cellId;
        Grid   grid;
    };

    struct Vector3
    {
        float x;
        float y;
        float z;
    };

    typedef Vector3 Vertex;

    // REFR, ACHR, ACRE
    struct Position
    {
        Vector3 pos;
        Vector3 rot; // angles are in radian, rz applied first and rx applied last
    };

    // REFR, ACHR, ACRE
    struct EnableParent
    {
        FormId        parent;
        std::uint32_t flags; //0x0001 = Set Enable State Opposite Parent, 0x0002 = Pop In
    };

    // LVLC, LVLI
    struct LVLO
    {
        std::int16_t  level;
        std::uint16_t unknown;  // sometimes missing
        FormId        item;
        std::int16_t  count;
        std::uint16_t unknown2; // sometimes missing
    };

    struct InventoryItem // NPC_, CREA, CONT
    {
        FormId        item;
        std::uint32_t count;
    };

    struct AIData        // NPC_, CREA
    {
        std::uint8_t  aggression;
        std::uint8_t  confidence;
        std::uint8_t  energyLevel;
        std::uint8_t  responsibility;
        std::uint32_t aiFlags;
        std::uint8_t  trainSkill;
        std::uint8_t  trainLevel;
        std::uint16_t unknown;
    };

    struct AttributeValues
    {
        std::uint8_t  strength;
        std::uint8_t  intelligence;
        std::uint8_t  willpower;
        std::uint8_t  agility;
        std::uint8_t  speed;
        std::uint8_t  endurance;
        std::uint8_t  personality;
        std::uint8_t  luck;
    };

    struct ActorBaseConfig
    {
#if 0
        enum ACBS_NPC
        {
            ACBS_Female               = 0x000001,
            ACBS_Essential            = 0x000002,
            ACBS_Respawn              = 0x000008,
            ACBS_Autocalcstats        = 0x000010,
            ACBS_PCLevelOffset        = 0x000080,
            ACBS_NoLowLevelProcessing = 0x000200,
            ACBS_NoRumors             = 0x002000,
            ACBS_Summonable           = 0x004000,
            ACBS_NoPersuasion         = 0x008000, // different meaning to crea
            ACBS_CanCorpseCheck       = 0x100000  // opposite of crea
        };

        enum ACBS_CREA
        {
            ACBS_Essential            = 0x000002,
            ACBS_WeapAndShield        = 0x000004,
            ACBS_Respawn              = 0x000008,
            ACBS_PCLevelOffset        = 0x000080,
            ACBS_NoLowLevelProcessing = 0x000200,
            ACBS_NoHead               = 0x008000, // different meaning to npc_
            ACBS_NoRightArm           = 0x010000,
            ACBS_NoLeftArm            = 0x020000,
            ACBS_NoCombatWater        = 0x040000,
            ACBS_NoShadow             = 0x080000,
            ACBS_NoCorpseCheck        = 0x100000  // opposite of npc_
        };
#endif
        std::uint32_t flags;
        std::uint16_t baseSpell;  // Base spell points
        std::uint16_t fatigue;    // Fatigue
        std::uint16_t barterGold; // Barter gold
        std::int16_t  level;      // Level/Offset level
        std::uint16_t calcMin;    // Calc Min
        std::uint16_t calcMax;    // Calc Max
    };

    struct ActorFaction
    {
        FormId       faction;
        std::int8_t  rank;
        std::uint8_t unknown1;
        std::uint8_t unknown2;
        std::uint8_t unknown3;
    };

    union EFI_Label
    {
        std::uint32_t value;
        char effect[4];
    };

    struct ScriptEffect
    {
        FormId       formId;       // Script effect (Magic effect must be SEFF)
        std::int32_t school;       // Magic school. See Magic schools for more information.
        EFI_Label    visualEffect; // Visual effect name or 0x00000000 if None
        std::uint8_t flags;        // 0x01 = Hostile
        std::uint8_t unknown1;
        std::uint8_t unknown2;
        std::uint8_t unknown3;
    };
#pragma pack(pop)

    // For pretty printing GroupHeader labels
    std::string printLabel(const GroupLabel& label, const std::uint32_t type);

    std::string printName(const std::uint32_t typeId);

    void gridToString(std::int16_t x, std::int16_t y, std::string& str);
}

#endif // ESM4_COMMON_H
