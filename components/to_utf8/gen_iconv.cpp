// This program generates the file tables_gen.hpp

#include <iostream>
#include <iomanip>
using namespace std;

#include <iconv.h>
#include <assert.h>

void tab() { cout << "   "; }

// write one number with a space in front of it and a comma after it
void num(unsigned char i, bool last)
{
  cout << " 0x" << (unsigned)i;
  if(!last) cout << ",";
}

// Write one table entry (UTF8 value), 1-5 bytes
void writeChar(char *value, int length, bool last, const std::string &comment="")
{
  assert(length >= 1 && length <= 5);
  tab();
  num(length, false);
  for(int i=0;i<5;i++)
    num(value[i], last && i==4);

  if(comment != "")
    cout << " // " << comment;

  cout << endl;
}

// What to write on missing characters
void writeMissing(bool last)
{
  // Just write a space character
  char value[5];
  value[0] = ' ';
  for(int i=1; i<5; i++)
    value[i] = 0;
  writeChar(value, 1, last, "not part of this charset");
}

int write_table(const std::string &charset, const std::string &tableName)
{
  // Write table header
  cout << "static char " << tableName << "[] =\n{\n";

  // Open conversion system
  iconv_t cd = iconv_open ("UTF-8", charset.c_str());

  // Convert each character from 0 to 255
  for(int i=0; i<256; i++)
    {
      bool last = (i==255);

      char input = i;
      char *iptr = &input;
      size_t ileft = 1;

      char output[5];
      for(int k=0; k<5; k++) output[k] = 0;
      char *optr = output;
      size_t oleft = 5;

      size_t res = iconv(cd, &iptr, &ileft, &optr, &oleft);

      if(res) writeMissing(last);
      else writeChar(output, 5-oleft, last);
    }

  iconv_close (cd);

  // Finish table
  cout << "};\n";
}

int main()
{
  cout << hex;

  // English
  write_table("WINDOWS-1252", "windows_1252");
  return 0;
}
