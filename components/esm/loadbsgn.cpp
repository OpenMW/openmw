#include "loadbsgn.hpp"

#include "esm_reader.hpp"
#include "esm_writer.hpp"

namespace ESM
{

void BirthSign::load(ESMReader &esm)
{
    mName = esm.getHNString("FNAM");
    mTexture = esm.getHNOString("TNAM");
    mDescription = esm.getHNOString("DESC");

    mPowers.load(esm);
}

void BirthSign::save(ESMWriter &esm)
{
    esm.writeHNCString("FNAM", mName);
    esm.writeHNOCString("TNAM", mTexture);
    esm.writeHNOCString("DESC", mDescription);

    mPowers.save(esm);
}

}
