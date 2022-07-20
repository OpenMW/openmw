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

#include <cstdint>
#include <string>

#include <components/esm/defs.hpp>

#include "formid.hpp"

namespace ESM4
{
    using ESM::fourCC;

    // Based on http://www.uesp.net/wiki/Tes5Mod:Mod_File_Format
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
        REC_MSET = fourCC("MSET")  // Media Set
    };

    enum SubRecordTypes
    {
        SUB_HEDR = fourCC("HEDR"),
        SUB_CNAM = fourCC("CNAM"),
        SUB_SNAM = fourCC("SNAM"), // TES4 only?
        SUB_MAST = fourCC("MAST"),
        SUB_DATA = fourCC("DATA"),
        SUB_ONAM = fourCC("ONAM"),
        SUB_INTV = fourCC("INTV"),
        SUB_INCC = fourCC("INCC"),
        SUB_OFST = fourCC("OFST"), // TES4 only?
        SUB_DELE = fourCC("DELE"), // TES4 only?

        SUB_DNAM = fourCC("DNAM"),
        SUB_EDID = fourCC("EDID"),
        SUB_FULL = fourCC("FULL"),
        SUB_LTMP = fourCC("LTMP"),
        SUB_MHDT = fourCC("MHDT"),
        SUB_MNAM = fourCC("MNAM"),
        SUB_MODL = fourCC("MODL"),
        SUB_NAM0 = fourCC("NAM0"),
        SUB_NAM2 = fourCC("NAM2"),
        SUB_NAM3 = fourCC("NAM3"),
        SUB_NAM4 = fourCC("NAM4"),
        SUB_NAM9 = fourCC("NAM9"),
        SUB_NAMA = fourCC("NAMA"),
        SUB_PNAM = fourCC("PNAM"),
        SUB_RNAM = fourCC("RNAM"),
        SUB_TNAM = fourCC("TNAM"),
        SUB_UNAM = fourCC("UNAM"),
        SUB_WCTR = fourCC("WCTR"),
        SUB_WNAM = fourCC("WNAM"),
        SUB_XEZN = fourCC("XEZN"),
        SUB_XLCN = fourCC("XLCN"),
        SUB_XXXX = fourCC("XXXX"),
        SUB_ZNAM = fourCC("ZNAM"),
        SUB_MODT = fourCC("MODT"),
        SUB_ICON = fourCC("ICON"), // TES4 only?

        SUB_NVER = fourCC("NVER"),
        SUB_NVMI = fourCC("NVMI"),
        SUB_NVPP = fourCC("NVPP"),
        SUB_NVSI = fourCC("NVSI"),

        SUB_NVNM = fourCC("NVNM"),
        SUB_NNAM = fourCC("NNAM"),

        SUB_XCLC = fourCC("XCLC"),
        SUB_XCLL = fourCC("XCLL"),
        SUB_TVDT = fourCC("TVDT"),
        SUB_XCGD = fourCC("XCGD"),
        SUB_LNAM = fourCC("LNAM"),
        SUB_XCLW = fourCC("XCLW"),
        SUB_XNAM = fourCC("XNAM"),
        SUB_XCLR = fourCC("XCLR"),
        SUB_XWCS = fourCC("XWCS"),
        SUB_XWCN = fourCC("XWCN"),
        SUB_XWCU = fourCC("XWCU"),
        SUB_XCWT = fourCC("XCWT"),
        SUB_XOWN = fourCC("XOWN"),
        SUB_XILL = fourCC("XILL"),
        SUB_XWEM = fourCC("XWEM"),
        SUB_XCCM = fourCC("XCCM"),
        SUB_XCAS = fourCC("XCAS"),
        SUB_XCMO = fourCC("XCMO"),
        SUB_XCIM = fourCC("XCIM"),
        SUB_XCMT = fourCC("XCMT"), // TES4 only?
        SUB_XRNK = fourCC("XRNK"), // TES4 only?
        SUB_XGLB = fourCC("XGLB"), // TES4 only?

        SUB_VNML = fourCC("VNML"),
        SUB_VHGT = fourCC("VHGT"),
        SUB_VCLR = fourCC("VCLR"),
        SUA_BTXT = fourCC("BTXT"),
        SUB_ATXT = fourCC("ATXT"),
        SUB_VTXT = fourCC("VTXT"),
        SUB_VTEX = fourCC("VTEX"),

        SUB_HNAM = fourCC("HNAM"),
        SUB_GNAM = fourCC("GNAM"),

        SUB_RCLR = fourCC("RCLR"),
        SUB_RPLI = fourCC("RPLI"),
        SUB_RPLD = fourCC("RPLD"),
        SUB_RDAT = fourCC("RDAT"),
        SUB_RDMD = fourCC("RDMD"), // TES4 only?
        SUB_RDSD = fourCC("RDSD"), // TES4 only?
        SUB_RDGS = fourCC("RDGS"), // TES4 only?
        SUB_RDMO = fourCC("RDMO"),
        SUB_RDSA = fourCC("RDSA"),
        SUB_RDWT = fourCC("RDWT"),
        SUB_RDOT = fourCC("RDOT"),
        SUB_RDMP = fourCC("RDMP"),

        SUB_MODB = fourCC("MODB"),
        SUB_OBND = fourCC("OBND"),
        SUB_MODS = fourCC("MODS"),

        SUB_NAME = fourCC("NAME"),
        SUB_XMRK = fourCC("XMRK"),
        SUB_FNAM = fourCC("FNAM"),
        SUB_XSCL = fourCC("XSCL"),
        SUB_XTEL = fourCC("XTEL"),
        SUB_XTRG = fourCC("XTRG"),
        SUB_XSED = fourCC("XSED"),
        SUB_XLOD = fourCC("XLOD"),
        SUB_XPCI = fourCC("XPCI"),
        SUB_XLOC = fourCC("XLOC"),
        SUB_XESP = fourCC("XESP"),
        SUB_XLCM = fourCC("XLCM"),
        SUB_XRTM = fourCC("XRTM"),
        SUB_XACT = fourCC("XACT"),
        SUB_XCNT = fourCC("XCNT"),
        SUB_VMAD = fourCC("VMAD"),
        SUB_XPRM = fourCC("XPRM"),
        SUB_XMBO = fourCC("XMBO"),
        SUB_XPOD = fourCC("XPOD"),
        SUB_XRMR = fourCC("XRMR"),
        SUB_INAM = fourCC("INAM"),
        SUB_SCHR = fourCC("SCHR"),
        SUB_XLRM = fourCC("XLRM"),
        SUB_XRGD = fourCC("XRGD"),
        SUB_XRDS = fourCC("XRDS"),
        SUB_XEMI = fourCC("XEMI"),
        SUB_XLIG = fourCC("XLIG"),
        SUB_XALP = fourCC("XALP"),
        SUB_XNDP = fourCC("XNDP"),
        SUB_XAPD = fourCC("XAPD"),
        SUB_XAPR = fourCC("XAPR"),
        SUB_XLIB = fourCC("XLIB"),
        SUB_XLKR = fourCC("XLKR"),
        SUB_XLRT = fourCC("XLRT"),
        SUB_XCVL = fourCC("XCVL"),
        SUB_XCVR = fourCC("XCVR"),
        SUB_XCZA = fourCC("XCZA"),
        SUB_XCZC = fourCC("XCZC"),
        SUB_XFVC = fourCC("XFVC"),
        SUB_XHTW = fourCC("XHTW"),
        SUB_XIS2 = fourCC("XIS2"),
        SUB_XMBR = fourCC("XMBR"),
        SUB_XCCP = fourCC("XCCP"),
        SUB_XPWR = fourCC("XPWR"),
        SUB_XTRI = fourCC("XTRI"),
        SUB_XATR = fourCC("XATR"),
        SUB_XPRD = fourCC("XPRD"),
        SUB_XPPA = fourCC("XPPA"),
        SUB_PDTO = fourCC("PDTO"),
        SUB_XLRL = fourCC("XLRL"),

        SUB_QNAM = fourCC("QNAM"),
        SUB_COCT = fourCC("COCT"),
        SUB_COED = fourCC("COED"),
        SUB_CNTO = fourCC("CNTO"),
        SUB_SCRI = fourCC("SCRI"),

        SUB_BNAM = fourCC("BNAM"),

        SUB_BMDT = fourCC("BMDT"),
        SUB_MOD2 = fourCC("MOD2"),
        SUB_MOD3 = fourCC("MOD3"),
        SUB_MOD4 = fourCC("MOD4"),
        SUB_MO2B = fourCC("MO2B"),
        SUB_MO3B = fourCC("MO3B"),
        SUB_MO4B = fourCC("MO4B"),
        SUB_MO2T = fourCC("MO2T"),
        SUB_MO3T = fourCC("MO3T"),
        SUB_MO4T = fourCC("MO4T"),
        SUB_ANAM = fourCC("ANAM"),
        SUB_ENAM = fourCC("ENAM"),
        SUB_ICO2 = fourCC("ICO2"),

        SUB_ACBS = fourCC("ACBS"),
        SUB_SPLO = fourCC("SPLO"),
        SUB_AIDT = fourCC("AIDT"),
        SUB_PKID = fourCC("PKID"),
        SUB_HCLR = fourCC("HCLR"),
        SUB_FGGS = fourCC("FGGS"),
        SUB_FGGA = fourCC("FGGA"),
        SUB_FGTS = fourCC("FGTS"),
        SUB_KFFZ = fourCC("KFFZ"),

        SUB_PFIG = fourCC("PFIG"),
        SUB_PFPC = fourCC("PFPC"),

        SUB_XHRS = fourCC("XHRS"),
        SUB_XMRC = fourCC("XMRC"),

        SUB_SNDD = fourCC("SNDD"),
        SUB_SNDX = fourCC("SNDX"),

        SUB_DESC = fourCC("DESC"),

        SUB_ENIT = fourCC("ENIT"),
        SUB_EFID = fourCC("EFID"),
        SUB_EFIT = fourCC("EFIT"),
        SUB_SCIT = fourCC("SCIT"),

        SUB_SOUL = fourCC("SOUL"),
        SUB_SLCP = fourCC("SLCP"),

        SUB_CSCR = fourCC("CSCR"),
        SUB_CSDI = fourCC("CSDI"),
        SUB_CSDC = fourCC("CSDC"),
        SUB_NIFZ = fourCC("NIFZ"),
        SUB_CSDT = fourCC("CSDT"),
        SUB_NAM1 = fourCC("NAM1"),
        SUB_NIFT = fourCC("NIFT"),

        SUB_LVLD = fourCC("LVLD"),
        SUB_LVLF = fourCC("LVLF"),
        SUB_LVLO = fourCC("LVLO"),

        SUB_BODT = fourCC("BODT"),
        SUB_YNAM = fourCC("YNAM"),
        SUB_DEST = fourCC("DEST"),
        SUB_DMDL = fourCC("DMDL"),
        SUB_DMDS = fourCC("DMDS"),
        SUB_DMDT = fourCC("DMDT"),
        SUB_DSTD = fourCC("DSTD"),
        SUB_DSTF = fourCC("DSTF"),
        SUB_KNAM = fourCC("KNAM"),
        SUB_KSIZ = fourCC("KSIZ"),
        SUB_KWDA = fourCC("KWDA"),
        SUB_VNAM = fourCC("VNAM"),
        SUB_SDSC = fourCC("SDSC"),
        SUB_MO2S = fourCC("MO2S"),
        SUB_MO4S = fourCC("MO4S"),
        SUB_BOD2 = fourCC("BOD2"),
        SUB_BAMT = fourCC("BAMT"),
        SUB_BIDS = fourCC("BIDS"),
        SUB_ETYP = fourCC("ETYP"),
        SUB_BMCT = fourCC("BMCT"),
        SUB_MICO = fourCC("MICO"),
        SUB_MIC2 = fourCC("MIC2"),
        SUB_EAMT = fourCC("EAMT"),
        SUB_EITM = fourCC("EITM"),

        SUB_SCTX = fourCC("SCTX"),
        SUB_XLTW = fourCC("XLTW"),
        SUB_XMBP = fourCC("XMBP"),
        SUB_XOCP = fourCC("XOCP"),
        SUB_XRGB = fourCC("XRGB"),
        SUB_XSPC = fourCC("XSPC"),
        SUB_XTNM = fourCC("XTNM"),
        SUB_ATKR = fourCC("ATKR"),
        SUB_CRIF = fourCC("CRIF"),
        SUB_DOFT = fourCC("DOFT"),
        SUB_DPLT = fourCC("DPLT"),
        SUB_ECOR = fourCC("ECOR"),
        SUB_ATKD = fourCC("ATKD"),
        SUB_ATKE = fourCC("ATKE"),
        SUB_FTST = fourCC("FTST"),
        SUB_HCLF = fourCC("HCLF"),
        SUB_NAM5 = fourCC("NAM5"),
        SUB_NAM6 = fourCC("NAM6"),
        SUB_NAM7 = fourCC("NAM7"),
        SUB_NAM8 = fourCC("NAM8"),
        SUB_PRKR = fourCC("PRKR"),
        SUB_PRKZ = fourCC("PRKZ"),
        SUB_SOFT = fourCC("SOFT"),
        SUB_SPCT = fourCC("SPCT"),
        SUB_TINC = fourCC("TINC"),
        SUB_TIAS = fourCC("TIAS"),
        SUB_TINI = fourCC("TINI"),
        SUB_TINV = fourCC("TINV"),
        SUB_TPLT = fourCC("TPLT"),
        SUB_VTCK = fourCC("VTCK"),
        SUB_SHRT = fourCC("SHRT"),
        SUB_SPOR = fourCC("SPOR"),
        SUB_XHOR = fourCC("XHOR"),
        SUB_CTDA = fourCC("CTDA"),
        SUB_CRDT = fourCC("CRDT"),
        SUB_FNMK = fourCC("FNMK"),
        SUB_FNPR = fourCC("FNPR"),
        SUB_WBDT = fourCC("WBDT"),
        SUB_QUAL = fourCC("QUAL"),
        SUB_INDX = fourCC("INDX"),
        SUB_ATTR = fourCC("ATTR"),
        SUB_MTNM = fourCC("MTNM"),
        SUB_UNES = fourCC("UNES"),
        SUB_TIND = fourCC("TIND"),
        SUB_TINL = fourCC("TINL"),
        SUB_TINP = fourCC("TINP"),
        SUB_TINT = fourCC("TINT"),
        SUB_TIRS = fourCC("TIRS"),
        SUB_PHWT = fourCC("PHWT"),
        SUB_AHCF = fourCC("AHCF"),
        SUB_AHCM = fourCC("AHCM"),
        SUB_HEAD = fourCC("HEAD"),
        SUB_MPAI = fourCC("MPAI"),
        SUB_MPAV = fourCC("MPAV"),
        SUB_DFTF = fourCC("DFTF"),
        SUB_DFTM = fourCC("DFTM"),
        SUB_FLMV = fourCC("FLMV"),
        SUB_FTSF = fourCC("FTSF"),
        SUB_FTSM = fourCC("FTSM"),
        SUB_MTYP = fourCC("MTYP"),
        SUB_PHTN = fourCC("PHTN"),
        SUB_RNMV = fourCC("RNMV"),
        SUB_RPRF = fourCC("RPRF"),
        SUB_RPRM = fourCC("RPRM"),
        SUB_SNMV = fourCC("SNMV"),
        SUB_SPED = fourCC("SPED"),
        SUB_SWMV = fourCC("SWMV"),
        SUB_WKMV = fourCC("WKMV"),
        SUB_LLCT = fourCC("LLCT"),
        SUB_IDLF = fourCC("IDLF"),
        SUB_IDLA = fourCC("IDLA"),
        SUB_IDLC = fourCC("IDLC"),
        SUB_IDLT = fourCC("IDLT"),
        SUB_DODT = fourCC("DODT"),
        SUB_TX00 = fourCC("TX00"),
        SUB_TX01 = fourCC("TX01"),
        SUB_TX02 = fourCC("TX02"),
        SUB_TX03 = fourCC("TX03"),
        SUB_TX04 = fourCC("TX04"),
        SUB_TX05 = fourCC("TX05"),
        SUB_TX06 = fourCC("TX06"),
        SUB_TX07 = fourCC("TX07"),
        SUB_BPND = fourCC("BPND"),
        SUB_BPTN = fourCC("BPTN"),
        SUB_BPNN = fourCC("BPNN"),
        SUB_BPNT = fourCC("BPNT"),
        SUB_BPNI = fourCC("BPNI"),
        SUB_RAGA = fourCC("RAGA"),

        SUB_QSTI = fourCC("QSTI"),
        SUB_QSTR = fourCC("QSTR"),
        SUB_QSDT = fourCC("QSDT"),
        SUB_SCDA = fourCC("SCDA"),
        SUB_SCRO = fourCC("SCRO"),
        SUB_QSTA = fourCC("QSTA"),
        SUB_CTDT = fourCC("CTDT"),
        SUB_SCHD = fourCC("SCHD"),
        SUB_TCLF = fourCC("TCLF"),
        SUB_TCLT = fourCC("TCLT"),
        SUB_TRDT = fourCC("TRDT"),
        SUB_TPIC = fourCC("TPIC"),

        SUB_PKDT = fourCC("PKDT"),
        SUB_PSDT = fourCC("PSDT"),
        SUB_PLDT = fourCC("PLDT"),
        SUB_PTDT = fourCC("PTDT"),
        SUB_PGRP = fourCC("PGRP"),
        SUB_PGRR = fourCC("PGRR"),
        SUB_PGRI = fourCC("PGRI"),
        SUB_PGRL = fourCC("PGRL"),
        SUB_PGAG = fourCC("PGAG"),
        SUB_FLTV = fourCC("FLTV"),

        SUB_XHLT = fourCC("XHLT"), // Unofficial Oblivion Patch
        SUB_XCHG = fourCC("XCHG"), // thievery.exp

        SUB_ITXT = fourCC("ITXT"),
        SUB_MO5T = fourCC("MO5T"),
        SUB_MOD5 = fourCC("MOD5"),
        SUB_MDOB = fourCC("MDOB"),
        SUB_SPIT = fourCC("SPIT"),
        SUB_PTDA = fourCC("PTDA"), // TES5
        SUB_PFOR = fourCC("PFOR"), // TES5
        SUB_PFO2 = fourCC("PFO2"), // TES5
        SUB_PRCB = fourCC("PRCB"), // TES5
        SUB_PKCU = fourCC("PKCU"), // TES5
        SUB_PKC2 = fourCC("PKC2"), // TES5
        SUB_CITC = fourCC("CITC"), // TES5
        SUB_CIS1 = fourCC("CIS1"), // TES5
        SUB_CIS2 = fourCC("CIS2"), // TES5
        SUB_TIFC = fourCC("TIFC"), // TES5
        SUB_ALCA = fourCC("ALCA"), // TES5
        SUB_ALCL = fourCC("ALCL"), // TES5
        SUB_ALCO = fourCC("ALCO"), // TES5
        SUB_ALDN = fourCC("ALDN"), // TES5
        SUB_ALEA = fourCC("ALEA"), // TES5
        SUB_ALED = fourCC("ALED"), // TES5
        SUB_ALEQ = fourCC("ALEQ"), // TES5
        SUB_ALFA = fourCC("ALFA"), // TES5
        SUB_ALFC = fourCC("ALFC"), // TES5
        SUB_ALFD = fourCC("ALFD"), // TES5
        SUB_ALFE = fourCC("ALFE"), // TES5
        SUB_ALFI = fourCC("ALFI"), // TES5
        SUB_ALFL = fourCC("ALFL"), // TES5
        SUB_ALFR = fourCC("ALFR"), // TES5
        SUB_ALID = fourCC("ALID"), // TES5
        SUB_ALLS = fourCC("ALLS"), // TES5
        SUB_ALNA = fourCC("ALNA"), // TES5
        SUB_ALNT = fourCC("ALNT"), // TES5
        SUB_ALPC = fourCC("ALPC"), // TES5
        SUB_ALRT = fourCC("ALRT"), // TES5
        SUB_ALSP = fourCC("ALSP"), // TES5
        SUB_ALST = fourCC("ALST"), // TES5
        SUB_ALUA = fourCC("ALUA"), // TES5
        SUB_FLTR = fourCC("FLTR"), // TES5
        SUB_QTGL = fourCC("QTGL"), // TES5
        SUB_TWAT = fourCC("TWAT"), // TES5
        SUB_XIBS = fourCC("XIBS"), // FO3
        SUB_REPL = fourCC("REPL"), // FO3
        SUB_BIPL = fourCC("BIPL"), // FO3
        SUB_MODD = fourCC("MODD"), // FO3
        SUB_MOSD = fourCC("MOSD"), // FO3
        SUB_MO3S = fourCC("MO3S"), // FO3
        SUB_XCET = fourCC("XCET"), // FO3
        SUB_LVLG = fourCC("LVLG"), // FO3
        SUB_NVCI = fourCC("NVCI"), // FO3
        SUB_NVVX = fourCC("NVVX"), // FO3
        SUB_NVTR = fourCC("NVTR"), // FO3
        SUB_NVCA = fourCC("NVCA"), // FO3
        SUB_NVDP = fourCC("NVDP"), // FO3
        SUB_NVGD = fourCC("NVGD"), // FO3
        SUB_NVEX = fourCC("NVEX"), // FO3
        SUB_XHLP = fourCC("XHLP"), // FO3
        SUB_XRDO = fourCC("XRDO"), // FO3
        SUB_XAMT = fourCC("XAMT"), // FO3
        SUB_XAMC = fourCC("XAMC"), // FO3
        SUB_XRAD = fourCC("XRAD"), // FO3
        SUB_XORD = fourCC("XORD"), // FO3
        SUB_XCLP = fourCC("XCLP"), // FO3
        SUB_NEXT = fourCC("NEXT"), // FO3
        SUB_QOBJ = fourCC("QOBJ"), // FO3
        SUB_POBA = fourCC("POBA"), // FO3
        SUB_POCA = fourCC("POCA"), // FO3
        SUB_POEA = fourCC("POEA"), // FO3
        SUB_PKDD = fourCC("PKDD"), // FO3
        SUB_PKD2 = fourCC("PKD2"), // FO3
        SUB_PKPT = fourCC("PKPT"), // FO3
        SUB_PKED = fourCC("PKED"), // FO3
        SUB_PKE2 = fourCC("PKE2"), // FO3
        SUB_PKAM = fourCC("PKAM"), // FO3
        SUB_PUID = fourCC("PUID"), // FO3
        SUB_PKW3 = fourCC("PKW3"), // FO3
        SUB_PTD2 = fourCC("PTD2"), // FO3
        SUB_PLD2 = fourCC("PLD2"), // FO3
        SUB_PKFD = fourCC("PKFD"), // FO3
        SUB_IDLB = fourCC("IDLB"), // FO3
        SUB_XDCR = fourCC("XDCR"), // FO3
        SUB_DALC = fourCC("DALC"), // FO3
        SUB_IMPS = fourCC("IMPS"), // FO3 Anchorage
        SUB_IMPF = fourCC("IMPF"), // FO3 Anchorage

        SUB_XATO = fourCC("XATO"), // FONV
        SUB_INFC = fourCC("INFC"), // FONV
        SUB_INFX = fourCC("INFX"), // FONV
        SUB_TDUM = fourCC("TDUM"), // FONV
        SUB_TCFU = fourCC("TCFU"), // FONV
        SUB_DAT2 = fourCC("DAT2"), // FONV
        SUB_RCIL = fourCC("RCIL"), // FONV
        SUB_MMRK = fourCC("MMRK"), // FONV
        SUB_SCRV = fourCC("SCRV"), // FONV
        SUB_SCVR = fourCC("SCVR"), // FONV
        SUB_SLSD = fourCC("SLSD"), // FONV
        SUB_XSRF = fourCC("XSRF"), // FONV
        SUB_XSRD = fourCC("XSRD"), // FONV
        SUB_WMI1 = fourCC("WMI1"), // FONV
        SUB_RDID = fourCC("RDID"), // FONV
        SUB_RDSB = fourCC("RDSB"), // FONV
        SUB_RDSI = fourCC("RDSI"), // FONV
        SUB_BRUS = fourCC("BRUS"), // FONV
        SUB_VATS = fourCC("VATS"), // FONV
        SUB_VANM = fourCC("VANM"), // FONV
        SUB_MWD1 = fourCC("MWD1"), // FONV
        SUB_MWD2 = fourCC("MWD2"), // FONV
        SUB_MWD3 = fourCC("MWD3"), // FONV
        SUB_MWD4 = fourCC("MWD4"), // FONV
        SUB_MWD5 = fourCC("MWD5"), // FONV
        SUB_MWD6 = fourCC("MWD6"), // FONV
        SUB_MWD7 = fourCC("MWD7"), // FONV
        SUB_WMI2 = fourCC("WMI2"), // FONV
        SUB_WMI3 = fourCC("WMI3"), // FONV
        SUB_WMS1 = fourCC("WMS1"), // FONV
        SUB_WMS2 = fourCC("WMS2"), // FONV
        SUB_WNM1 = fourCC("WNM1"), // FONV
        SUB_WNM2 = fourCC("WNM2"), // FONV
        SUB_WNM3 = fourCC("WNM3"), // FONV
        SUB_WNM4 = fourCC("WNM4"), // FONV
        SUB_WNM5 = fourCC("WNM5"), // FONV
        SUB_WNM6 = fourCC("WNM6"), // FONV
        SUB_WNM7 = fourCC("WNM7"), // FONV
        SUB_JNAM = fourCC("JNAM"), // FONV
        SUB_EFSD = fourCC("EFSD"), // FONV DeadMoney
    };

    enum MagicEffectID
    {
        // Alteration
        EFI_BRDN = fourCC("BRDN"),
        EFI_FTHR = fourCC("FTHR"),
        EFI_FISH = fourCC("FISH"),
        EFI_FRSH = fourCC("FRSH"),
        EFI_OPEN = fourCC("OPNN"),
        EFI_SHLD = fourCC("SHLD"),
        EFI_LISH = fourCC("LISH"),
        EFI_WABR = fourCC("WABR"),
        EFI_WAWA = fourCC("WAWA"),

        // Conjuration
        EFI_BABO = fourCC("BABO"), // Bound Boots
        EFI_BACU = fourCC("BACU"), // Bound Cuirass
        EFI_BAGA = fourCC("BAGA"), // Bound Gauntlets
        EFI_BAGR = fourCC("BAGR"), // Bound Greaves
        EFI_BAHE = fourCC("BAHE"), // Bound Helmet
        EFI_BASH = fourCC("BASH"), // Bound Shield
        EFI_BWAX = fourCC("BWAX"), // Bound Axe
        EFI_BWBO = fourCC("BWBO"), // Bound Bow
        EFI_BWDA = fourCC("BWDA"), // Bound Dagger
        EFI_BWMA = fourCC("BWMA"), // Bound Mace
        EFI_BWSW = fourCC("BWSW"), // Bound Sword
        EFI_Z001 = fourCC("Z001"), // Summon Rufio's Ghost
        EFI_Z002 = fourCC("Z002"), // Summon Ancestor Guardian
        EFI_Z003 = fourCC("Z003"), // Summon Spiderling
        EFI_Z005 = fourCC("Z005"), // Summon Bear
        EFI_ZCLA = fourCC("ZCLA"), // Summon Clannfear
        EFI_ZDAE = fourCC("ZDAE"), // Summon Daedroth
        EFI_ZDRE = fourCC("ZDRE"), // Summon Dremora
        EFI_ZDRL = fourCC("ZDRL"), // Summon Dremora Lord
        EFI_ZFIA = fourCC("ZFIA"), // Summon Flame Atronach
        EFI_ZFRA = fourCC("ZFRA"), // Summon Frost Atronach
        EFI_ZGHO = fourCC("ZGHO"), // Summon Ghost
        EFI_ZHDZ = fourCC("ZHDZ"), // Summon Headless Zombie
        EFI_ZLIC = fourCC("ZLIC"), // Summon Lich
        EFI_ZSCA = fourCC("ZSCA"), // Summon Scamp
        EFI_ZSKE = fourCC("ZSKE"), // Summon Skeleton
        EFI_ZSKA = fourCC("ZSKA"), // Summon Skeleton Guardian
        EFI_ZSKH = fourCC("ZSKH"), // Summon Skeleton Hero
        EFI_ZSKC = fourCC("ZSKC"), // Summon Skeleton Champion
        EFI_ZSPD = fourCC("ZSPD"), // Summon Spider Daedra
        EFI_ZSTA = fourCC("ZSTA"), // Summon Storm Atronach
        EFI_ZWRA = fourCC("ZWRA"), // Summon Faded Wraith
        EFI_ZWRL = fourCC("ZWRL"), // Summon Gloom Wraith
        EFI_ZXIV = fourCC("ZXIV"), // Summon Xivilai
        EFI_ZZOM = fourCC("ZZOM"), // Summon Zombie
        EFI_TURN = fourCC("TURN"), // Turn Undead

        // Destruction
        EFI_DGAT = fourCC("DGAT"), // Damage Attribute
        EFI_DGFA = fourCC("DGFA"), // Damage Fatigue
        EFI_DGHE = fourCC("DGHE"), // Damage Health
        EFI_DGSP = fourCC("DGSP"), // Damage Magicka
        EFI_DIAR = fourCC("DIAR"), // Disintegrate Armor
        EFI_DIWE = fourCC("DIWE"), // Disintegrate Weapon
        EFI_DRAT = fourCC("DRAT"), // Drain Attribute
        EFI_DRFA = fourCC("DRFA"), // Drain Fatigue
        EFI_DRHE = fourCC("DRHE"), // Drain Health
        EFI_DRSP = fourCC("DRSP"), // Drain Magicka
        EFI_DRSK = fourCC("DRSK"), // Drain Skill
        EFI_FIDG = fourCC("FIDG"), // Fire Damage
        EFI_FRDG = fourCC("FRDG"), // Frost Damage
        EFI_SHDG = fourCC("SHDG"), // Shock Damage
        EFI_WKDI = fourCC("WKDI"), // Weakness to Disease
        EFI_WKFI = fourCC("WKFI"), // Weakness to Fire
        EFI_WKFR = fourCC("WKFR"), // Weakness to Frost
        EFI_WKMA = fourCC("WKMA"), // Weakness to Magic
        EFI_WKNW = fourCC("WKNW"), // Weakness to Normal Weapons
        EFI_WKPO = fourCC("WKPO"), // Weakness to Poison
        EFI_WKSH = fourCC("WKSH"), // Weakness to Shock

        // Illusion
        EFI_CALM = fourCC("CALM"), // Calm
        EFI_CHML = fourCC("CHML"), // Chameleon
        EFI_CHRM = fourCC("CHRM"), // Charm
        EFI_COCR = fourCC("COCR"), // Command Creature
        EFI_COHU = fourCC("COHU"), // Command Humanoid
        EFI_DEMO = fourCC("DEMO"), // Demoralize
        EFI_FRNZ = fourCC("FRNZ"), // Frenzy
        EFI_INVI = fourCC("INVI"), // Invisibility
        EFI_LGHT = fourCC("LGHT"), // Light
        EFI_NEYE = fourCC("NEYE"), // Night-Eye
        EFI_PARA = fourCC("PARA"), // Paralyze
        EFI_RALY = fourCC("RALY"), // Rally
        EFI_SLNC = fourCC("SLNC"), // Silence

        // Mysticism
        EFI_DTCT = fourCC("DTCT"), // Detect Life
        EFI_DSPL = fourCC("DSPL"), // Dispel
        EFI_REDG = fourCC("REDG"), // Reflect Damage
        EFI_RFLC = fourCC("RFLC"), // Reflect Spell
        EFI_STRP = fourCC("STRP"), // Soul Trap
        EFI_SABS = fourCC("SABS"), // Spell Absorption
        EFI_TELE = fourCC("TELE"), // Telekinesis

        // Restoration
        EFI_ABAT = fourCC("ABAT"), // Absorb Attribute
        EFI_ABFA = fourCC("ABFA"), // Absorb Fatigue
        EFI_ABHe = fourCC("ABHe"), // Absorb Health
        EFI_ABSP = fourCC("ABSP"), // Absorb Magicka
        EFI_ABSK = fourCC("ABSK"), // Absorb Skill
        EFI_1400 = fourCC("1400"), // Cure Disease
        EFI_CUPA = fourCC("CUPA"), // Cure Paralysis
        EFI_CUPO = fourCC("CUPO"), // Cure Poison
        EFI_FOAT = fourCC("FOAT"), // Fortify Attribute
        EFI_FOFA = fourCC("FOFA"), // Fortify Fatigue
        EFI_FOHE = fourCC("FOHE"), // Fortify Health
        EFI_FOSP = fourCC("FOSP"), // Fortify Magicka
        EFI_FOSK = fourCC("FOSK"), // Fortify Skill
        EFI_RSDI = fourCC("RSDI"), // Resist Disease
        EFI_RSFI = fourCC("RSFI"), // Resist Fire
        EFI_RSFR = fourCC("RSFR"), // Resist Frost
        EFI_RSMA = fourCC("RSMA"), // Resist Magic
        EFI_RSNW = fourCC("RSNW"), // Resist Normal Weapons
        EFI_RSPA = fourCC("RSPA"), // Resist Paralysis
        EFI_RSPO = fourCC("RSPO"), // Resist Poison
        EFI_RSSH = fourCC("RSSH"), // Resist Shock
        EFI_REAT = fourCC("REAT"), // Restore Attribute
        EFI_REFA = fourCC("REFA"), // Restore Fatigue
        EFI_REHE = fourCC("REHE"), // Restore Health
        EFI_RESP = fourCC("RESP"), // Restore Magicka

        // Effects
        EFI_LOCK = fourCC("LOCK"), // Lock Lock
        EFI_SEFF = fourCC("SEFF"), // Script Effect
        EFI_Z020 = fourCC("Z020"), // Summon 20 Extra
        EFI_MYHL = fourCC("MYHL"), // Summon Mythic Dawn Helmet
        EFI_MYTH = fourCC("MYTH"), // Summon Mythic Dawn Armor
        EFI_REAN = fourCC("REAN"), // Reanimate
        EFI_DISE = fourCC("DISE"), // Disease Info
        EFI_POSN = fourCC("POSN"), // Poison Info
        EFI_DUMY = fourCC("DUMY"), // Mehrunes Dagon Custom Effect
        EFI_STMA = fourCC("STMA"), // Stunted Magicka
        EFI_SUDG = fourCC("SUDG"), // Sun Damage
        EFI_VAMP = fourCC("VAMP"), // Vampirism
        EFI_DARK = fourCC("DARK"), // Darkness
        EFI_RSWD = fourCC("RSWD")  // Resist Water Damage
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
        Rec_VisDistant = 0x00008000, // Visible when distant
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

#pragma pack(push, 1)
    // NOTE: the label field of a group is not reliable (http://www.uesp.net/wiki/Tes4Mod:Mod_File_Format)
    union GroupLabel
    {
        std::uint32_t value;     // formId, blockNo or raw int representation of type
        char recordType[4];      // record type in ascii
        std::int16_t grid[2];    // grid y, x (note the reverse order)
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

    struct Vertex
    {
        float x;
        float y;
        float z;
    };
#pragma pack(pop)

    // For pretty printing GroupHeader labels
    std::string printLabel(const GroupLabel& label, const std::uint32_t type);

    void gridToString(std::int16_t x, std::int16_t y, std::string& str);
}

#endif // ESM4_COMMON_H
