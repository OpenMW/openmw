#ifndef _ESM_RECORDS_H
#define _ESM_RECORDS_H

#include "loadacti.hpp"
#include "loadalch.hpp"
#include "loadappa.hpp"
#include "loadarmo.hpp"
#include "loadbody.hpp"
#include "loadbook.hpp"
#include "loadbsgn.hpp"
#include "loadcell.hpp"
#include "loadclas.hpp"
#include "loadclot.hpp"
#include "loadcont.hpp"
#include "loadcrea.hpp"
#include "loadcrec.hpp"
#include "loadinfo.hpp"
#include "loaddial.hpp"
#include "loaddoor.hpp"
#include "loadench.hpp"
#include "loadfact.hpp"
#include "loadglob.hpp"
#include "loadgmst.hpp"
#include "loadingr.hpp"
#include "loadland.hpp"
#include "loadlevlist.hpp"
#include "loadligh.hpp"
#include "loadlocks.hpp"
#include "loadltex.hpp"
#include "loadmgef.hpp"
#include "loadmisc.hpp"
#include "loadnpc.hpp"
#include "loadnpcc.hpp"
#include "loadpgrd.hpp"
#include "loadrace.hpp"
#include "loadregn.hpp"
#include "loadscpt.hpp"
#include "loadskil.hpp"
#include "loadsndg.hpp"
#include "loadsoun.hpp"
#include "loadspel.hpp"
#include "loadsscr.hpp"
#include "loadstat.hpp"
#include "loadweap.hpp"

// Special records which are not loaded from ESM
#include "attr.hpp"

namespace ESM {

// Integer versions of all the record names, used for faster lookup
enum RecNameInts
  {
    REC_ACTI = 0x49544341,
    REC_ALCH = 0x48434c41,
    REC_APPA = 0x41505041,
    REC_ARMO = 0x4f4d5241,
    REC_BODY = 0x59444f42,
    REC_BOOK = 0x4b4f4f42,
    REC_BSGN = 0x4e475342,
    REC_CELL = 0x4c4c4543,
    REC_CLAS = 0x53414c43,
    REC_CLOT = 0x544f4c43,
    REC_CNTC = 0x43544e43,
    REC_CONT = 0x544e4f43,
    REC_CREA = 0x41455243,
    REC_CREC = 0x43455243,
    REC_DIAL = 0x4c414944,
    REC_DOOR = 0x524f4f44,
    REC_ENCH = 0x48434e45,
    REC_FACT = 0x54434146,
    REC_GLOB = 0x424f4c47,
    REC_GMST = 0x54534d47,
    REC_INFO = 0x4f464e49,
    REC_INGR = 0x52474e49,
    REC_LAND = 0x444e414c,
    REC_LEVC = 0x4356454c,
    REC_LEVI = 0x4956454c,
    REC_LIGH = 0x4847494c,
    REC_LOCK = 0x4b434f4c,
    REC_LTEX = 0x5845544c,
    REC_MGEF = 0x4645474d,
    REC_MISC = 0x4353494d,
    REC_NPC_ = 0x5f43504e,
    REC_NPCC = 0x4343504e,
    REC_PGRD = 0x44524750,
    REC_PROB = 0x424f5250,
    REC_RACE = 0x45434152,
    REC_REGN = 0x4e474552,
    REC_REPA = 0x41504552,
    REC_SCPT = 0x54504353,
    REC_SKIL = 0x4c494b53,
    REC_SNDG = 0x47444e53,
    REC_SOUN = 0x4e554f53,
    REC_SPEL = 0x4c455053,
    REC_SSCR = 0x52435353,
    REC_STAT = 0x54415453,
    REC_WEAP = 0x50414557
  };
}
#endif
