/*
  Test of the NIFFile class
 */

#include "../nif_file.hpp"
#include <iostream>
#include <iomanip>
#include "../../mangle/stream/servers/file_stream.hpp"
#include "../node.hpp"
#include "../controller.hpp"
#include "../data.hpp"

using namespace Mangle::Stream;
using namespace std;
using namespace Nif;

// Display very verbose information
bool verbose = false;

void doVector(const Vector *vec)
{
  cout << "["
       << vec->array[0] << ","
       << vec->array[1] << ","
       << vec->array[2] << "]\n";
}

void doVector4(const Vector4 *vec)
{
  cout << "["
       << vec->array[0] << ","
       << vec->array[1] << ","
       << vec->array[2] << ","
       << vec->array[3] << "]\n";
}

void doMatrix(const Matrix *mat)
{
  cout << "Matrix:\n";
  for(int i=0; i<3; i++)
    {
      cout << "  ";
      doVector(&mat->v[i]);
    }
}

void doTrafo(const Transformation* trafo)
{
  cout << "--- transformation:\n";
  cout << "Pos: "; doVector(&trafo->pos);
  cout << "Rot: "; doMatrix(&trafo->rotation);
  cout << "Scale: " << trafo->scale << endl;
  cout << "Vel: "; doVector(&trafo->velocity);
  cout << "--- end transformation\n";
}

void doExtra(Extra *e)
{
  cout << "Extra: " << e->extra.getIndex() << endl;
}

void doControlled(Controlled *c)
{
  doExtra(c);
  cout << "Controller: " << c->controller.getIndex() << endl;
}

void doNamed(Named *n)
{
  doControlled(n);
  cout << "Name: " << n->name.toString() << endl;
}

void doNode(Node *n)
{
  doNamed(n);

  cout << "Flags: 0x" << hex << n->flags << dec << endl;
  doTrafo(n->trafo);

  cout << "Properties:";
  for(int i=0; i<n->props.length(); i++)
    cout << " " << n->props.getIndex(i);
  cout << endl;

  if(n->hasBounds)
    {
      cout << "Bounding box:\n";
      doVector(n->boundPos);
      doMatrix(n->boundRot);
      doVector(n->boundXYZ);
    }

  if(n->boneTrafo)
    {
      cout << "This is a bone: ";
      if(n->boneIndex == -1)
        cout << "root bone\n";
      else
        cout << "index " << n->boneIndex << endl;
    }
}

void doNiTriShape(NiTriShape *n)
{
  doNode(n);

  cout << "Shape data: " << n->data.getIndex() << endl;
  cout << "Skin instance: " << n->skin.getIndex() << endl;
}

void doNiSkinData(NiSkinData *n)
{
  int c = n->bones.size();

  cout << "Global transformation:\n";
  doMatrix(&n->trafo->rotation);
  doVector(&n->trafo->trans);
  cout << "Scale: " << n->trafo->scale << endl;

  cout << "Bone number: " << c << endl;
  for(int i=0; i<c; i++)
    {
      NiSkinData::BoneInfo &bi = n->bones[i];

      cout << "-- Bone " << i << ":\n";
      doMatrix(&bi.trafo->rotation);
      doVector(&bi.trafo->trans);
      cout << "Scale: " << bi.trafo->scale << endl;
      cout << "Unknown: "; doVector4(bi.unknown);
      cout << "Weight number: " << bi.weights.length << endl;

      if(verbose)
        for(int j=0; j<bi.weights.length; j++)
          {
            const NiSkinData::VertWeight &w = bi.weights.ptr[j];
            cout << "  vert:" << w.vertex << " weight:" << w.weight << endl;
          }
    }
}

void doNiSkinInstance(NiSkinInstance *n)
{
  cout << "Data: " << n->data.getIndex() << endl;
  cout << "Root: " << n->root.getIndex() << endl;
  cout << "Bones:";
  for(int i=0; i<n->bones.length(); i++)
    cout << " " << n->bones.getIndex(i);
  cout << endl;
}

void doNiNode(NiNode *n)
{
  doNode(n);

  cout << "Children:";
  for(int i=0; i<n->children.length(); i++)
    cout << " " << n->children.getIndex(i);
  cout << endl;

  cout << "Effects:";
  for(int i=0; i<n->effects.length(); i++)
    cout << " " << n->effects.getIndex(i);
  cout << endl;
}

void doNiStringExtraData(NiStringExtraData *s)
{
  doExtra(s);
  cout << "String: " << s->string.toString() << endl;
}

void doNiTextKeyExtraData(NiTextKeyExtraData *t)
{
  doExtra(t);
  for(int i=0; i<t->list.size(); i++)
    {
      cout << "@time " << t->list[i].time << ":\n\""
           << t->list[i].text.toString() << "\"" << endl;
    }
}

void doController(Controller *r)
{
  cout << "Next controller: " << r->next.getIndex() << endl;
  cout << "Flags: " << hex << r->flags << dec << endl;
  cout << "Frequency: " << r->frequency << endl;
  cout << "Phase: " << r->phase << endl;
  cout << "Time start: " << r->timeStart << endl;
  cout << "Time stop: " << r->timeStop << endl;
  cout << "Target: " << r->target.getIndex() << endl;
}

void doNiKeyframeController(NiKeyframeController *k)
{
  doController(k);
  cout << "Data: " << k->data.getIndex() << endl;
}

int main(int argc, char **args)
{
  if(argc != 2)
    {
      cout << "Specify a NIF file on the command line\n";
      return 1;
    }

  StreamPtr file(new FileStream(args[1]));
  NIFFile nif(file, args[1]);

  int num = nif.numRecords();

  for(int i=0; i<num; i++)
    {
      Record *r = nif.getRecord(i);
      cout << i << ": " << r->recName.toString() << endl;

      switch(r->recType)
        {
        case RC_NiNode: doNiNode((NiNode*)r); break;
        case RC_NiSkinData: doNiSkinData((NiSkinData*)r); break;
        case RC_NiSkinInstance: doNiSkinInstance((NiSkinInstance*)r); break;
        case RC_NiTriShape: doNiTriShape((NiTriShape*)r); break;
        case RC_NiStringExtraData: doNiStringExtraData((NiStringExtraData*)r); break;
        case RC_NiSequenceStreamHelper: doNamed((Named*)r); break;
        case RC_NiTextKeyExtraData: doNiTextKeyExtraData((NiTextKeyExtraData*)r); break;
        case RC_NiKeyframeController: doNiKeyframeController((NiKeyframeController*)r); break;
        }

      cout << endl;
    }
}
