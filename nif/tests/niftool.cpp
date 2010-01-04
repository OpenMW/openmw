#include "../nif_file.h"

/*
  Test of the NIFFile class
 */

#include <iostream>
#include "../../mangle/stream/servers/file_stream.h"

using namespace Mangle::Stream;
using namespace std;

int main(int argc, char **args)
{
  if(argc != 2)
    {
      cout << "Specify a NIF file on the command line\n";
      return 1;
    }

  StreamPtr file(new FileStream(args[1]));
  NIFFile nif(file, args[1]);
}
