/*
  Test of the NIFFile class
 */

#include "../nif_file.h"
#include <iostream>
#include <iomanip>
#include "../../mangle/stream/servers/file_stream.h"
#include "../node.h"
#include "../controller.h"

using namespace Mangle::Stream;
using namespace std;
using namespace Nif;

void doVector(const Vector *vec)
{
  cout << "["
       << vec->array[0] << ","
       << vec->array[1] << ","
       << vec->array[2] << "]\n";
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
        case RC_NiStringExtraData: doNiStringExtraData((NiStringExtraData*)r); break;
        case RC_NiSequenceStreamHelper: doNamed((Named*)r); break;
        case RC_NiTextKeyExtraData: doNiTextKeyExtraData((NiTextKeyExtraData*)r); break;
        case RC_NiKeyframeController: doNiKeyframeController((NiKeyframeController*)r); break;
        }

      cout << endl;
    }
}
