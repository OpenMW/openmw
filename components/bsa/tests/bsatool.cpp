#include "../bsa_file.hpp"

#include "bsatool_cmd.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <exception>

#include "../../mangle/stream/filters/buffer_stream.hpp"

using namespace std;
using namespace Mangle::Stream;
using namespace Bsa;

int main(int argc, char** argv)
{
  gengetopt_args_info info;

  if(cmdline_parser(argc, argv, &info) != 0)
    return 1;

  if(info.inputs_num != 1)
    {
      if(info.inputs_num == 0)
        cout << "ERROR: missing BSA file\n\n";
      else
        cout << "ERROR: more than one BSA file specified\n\n";
      cmdline_parser_print_help();
      return 1;
    }

  // Open file
  BSAFile bsa;
  char *arcname = info.inputs[0];
  try { bsa.open(arcname); }
  catch(exception &e)
    {
      cout << "ERROR reading BSA archive '" << arcname
           << "'\nDetails:\n" << e.what() << endl;
      return 2;
    }

  if(info.extract_given)
    {
      char *file = info.extract_arg;

      if(!bsa.exists(file))
        {
          cout << "ERROR: file '" << file << "' not found\n";
          cout << "In archive: " << arcname << endl;
          return 3;
        }

      // Find the base name of the file
      int pos = strlen(file);
      while(pos > 0 && file[pos] != '\\') pos--;
      char *base = file+pos+1;

      // TODO: We might add full directory name extraction later. We
      // could also allow automatic conversion from / to \ in
      // parameter file names.

      // Load the file into a memory buffer
      BufferStream data(bsa.getFile(file));

      // Write the file to disk
      ofstream out(base, ios::binary);
      out.write((char*)data.getPtr(), data.size());
      out.close();

      return 0;
    }

  // List all files
  const BSAFile::FileList &files = bsa.getList();
  for(int i=0; i<files.size(); i++)
    {
      if(info.long_given)
        {
          // Long format
          cout << setw(50) << left << files[i].name;
          cout << setw(8) << left << dec << files[i].fileSize;
          cout << "@ 0x" << hex << files[i].offset << endl;
        }
      else
        cout << files[i].name << endl;
    }

  // Done!
  return 0;
}
