#include "records.hpp"

/** Implementation for some of the load() functions. Most are found in
    the header files themselves, but this is a bit irritating to
    compile if you're changing the functions often, as virtually the
    entire engine depends on these headers.
 */

namespace ESM
{
  void NPC::load(ESMReader &esm, const std::string& id)
  {
    mId = id;

    npdt52.gold = -10;

    model = esm.getHNOString("MODL");
    name = esm.getHNOString("FNAM");

    race = esm.getHNString("RNAM");
    cls = esm.getHNString("CNAM");
    faction = esm.getHNString("ANAM");
    head = esm.getHNString("BNAM");
    hair = esm.getHNString("KNAM");

    script = esm.getHNOString("SCRI");

    esm.getSubNameIs("NPDT");
    esm.getSubHeader();
    if(esm.getSubSize() == 52) esm.getExact(&npdt52, 52);
    else if(esm.getSubSize() == 12) esm.getExact(&npdt12, 12);
    else esm.fail("NPC_NPDT must be 12 or 52 bytes long");

    esm.getHNT(flags, "FLAG");

    inventory.load(esm);
    spells.load(esm);

    if(esm.isNextSub("AIDT"))
      {
	esm.getHExact(&AI, sizeof(AI));
	hasAI = true;
      }
    else hasAI = false;

    esm.skipRecord();
  }

  void DialInfo::load(ESMReader &esm)
  {
    id = esm.getHNString("INAM");
    prev = esm.getHNString("PNAM");
    next = esm.getHNString("NNAM");

    // Not present if deleted
    if(esm.isNextSub("DATA"))
      esm.getHT(data, 12);

    // What follows is somewhat spaghetti-ish, but it's worth if for
    // an extra speedup. INFO is by far the most common record type.

    // subName is a reference to the original, so it changes whenever
    // a new sub name is read. esm.isEmptyOrGetName() will get the
    // next name for us, or return true if there are no more records.
    esm.getSubName();
    const NAME &subName = esm.retSubName();

    if(subName.val == REC_ONAM)
      {
        actor = esm.getHString();
	if(esm.isEmptyOrGetName()) return;
      }
    if(subName.val == REC_RNAM)
      {
        race = esm.getHString();
	if(esm.isEmptyOrGetName()) return;
      }
    if(subName.val == REC_CNAM)
      {
        clas = esm.getHString();
	if(esm.isEmptyOrGetName()) return;
      }

    factionLess = false;
    if(subName.val == REC_FNAM)
      {
        npcFaction = esm.getHString();
	if(npcFaction == "FFFF") factionLess = true;
	if(esm.isEmptyOrGetName()) return;
      }
    if(subName.val == REC_ANAM)
      {
	cell = esm.getHString();
	if(esm.isEmptyOrGetName()) return;
      }
    if(subName.val == REC_DNAM)
      {
        pcFaction = esm.getHString();
	if(esm.isEmptyOrGetName()) return;
      }
    if(subName.val == REC_SNAM)
      {
        sound = esm.getHString();
	if(esm.isEmptyOrGetName()) return;
      }
    if(subName.val == REC_NAME)
      {
	response = esm.getHString();
	if(esm.isEmptyOrGetName()) return;
      }

    while(subName.val == REC_SCVR)
      {
        SelectStruct ss;

        ss.selectRule = esm.getHString();
        esm.isEmptyOrGetName();

        if(subName.val == REC_INTV)
          {
            ss.type = VT_Int;
            esm.getHT(ss.i);
          }
        else if(subName.val == REC_FLTV)
          {
            ss.type = VT_Float;
            esm.getHT(ss.f);
          }
        else
          esm.fail("INFO.SCVR must precede INTV or FLTV, not "
		   + subName.toString());

        selects.push_back(ss);

	if(esm.isEmptyOrGetName()) return;
      }

    if(subName.val == REC_BNAM)
      {
	resultScript = esm.getHString();
	if(esm.isEmptyOrGetName()) return;
      }

    questStatus = QS_None;

    if     (subName.val == REC_QSTN) questStatus = QS_Name;
    else if(subName.val == REC_QSTF) questStatus = QS_Finished;
    else if(subName.val == REC_QSTR) questStatus = QS_Restart;
    else if(subName.val == REC_DELE) questStatus = QS_Deleted;
    else
      esm.fail("Don't know what to do with " + subName.toString() + " in INFO " + id);

    if(questStatus != QS_None)
      // Skip rest of record
      esm.skipRecord();
  }
}
