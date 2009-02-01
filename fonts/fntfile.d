
/*
  Font loader for the Morrowind .FNT/.TEX font file pair. The current
  code has just been used for testing purposes and to decode the file
  format. To make it work with MyGUI, we will have to cooperate with a
  custom sub-class of the MyGUI::Font class in C++.
 */
module fonts.fntfile;

import std.stream;

import monster.util.string;

align(1)
struct FntEntry
{
  // Positions, as fractions of the entire texture surface. The
  // xstart2 etc values are just repeated information. The reason for
  // this is that the characters are stored as four points, with an
  // (x,y) coordinate each. Since all the rectangles are aligned with
  // the image however, this is pretty redundant information.

  float xstart, ystart;
  float xend;
  float ystart2, xstart2;
  float yend;
  float xend2, yend2;

  float width;
  float height;
  int zero1;        // Empty heigth? (always zero)
  float emptyWidth; // Width for empty characters
  float unk2;       // Vertical displacement?
  int zero2;        // Horizontal displacement?

  bool isUsed() { return width != 0; }
}
static assert(FntEntry.sizeof == 56);

align(1)
struct FntFile
{
  float size;
  int unk2, unk3;
  char[128] texname;

  ubyte[160] filler; // Makes the header exactly 300 bytes long

  FntEntry[255] chars;
}

align(1)
union Color
{
  ubyte[4] rgba;
  uint val;
}
static assert(Color.sizeof == 4);

// One character
struct Char
{
  Color[][] pixels;
  bool isUsed;
  int width, height;
  char chr;
}

FntFile fnt;
Char[255] chars;

// Load a fnt-file
void loadFont(char[] fntFile)
{
  assert(iEnds(fntFile, ".fnt"),
         "loadFont() can only load .fnt files");

  File s = new File(fntFile);
  s.readExact(&fnt, fnt.sizeof);
  s.close();

  // Load the .tex file
  int texWidth, texHeight;
  char[] tfile = stripz(fnt.texname)~".tex";
  // DIRTY hack since we can't do case-insensitive file searching yet
  if(tfile[0] == 'D') tfile[0] = 'd';
  s.open(tfile);
  s.read(texWidth);
  s.read(texHeight);
  assert(s.size() == 4*(texWidth*texHeight + 2));

  ubyte[] buf;
  buf.length = s.size - s.position;
  s.readExact(buf.ptr, buf.length);
  delete s;

  // Get the pixel buffer as a series of ints
  uint[] pixelBuf = cast(uint[]) buf;

  foreach(i, ch; fnt.chars)
    with(chars[i])
    {
      // Store the char, if it is printable
      if(i > 33 && i < 127)
        chr = i;
      else chr = ' ';

      // Store the pixel dimensions
      isUsed = ch.isUsed;
      height = cast(int)ch.height;
      if(isUsed)
        width = cast(int)ch.width;
      else
        width = cast(int)ch.emptyWidth;

      assert(ch.emptyWidth == 0 || ch.emptyWidth == -1 || !isUsed);
      assert(ch.zero1 == 0);
      assert(ch.zero2 == 0);
      assert(ch.xstart2 == ch.xstart);
      assert(ch.ystart2 == ch.ystart);
      assert(ch.xend2 == ch.xend);
      assert(ch.yend2 == ch.yend);

      // If the character is not present, skip to the next one now.
      if(!isUsed) continue;

      // Get the pixel coordinates of this character
      int startX = cast(int) (ch.xstart * texWidth);
      int startY = cast(int) (ch.ystart * texHeight);
      int endX = cast(int) (ch.xend * texWidth);
      int endY = cast(int) (ch.yend * texHeight);

      assert(endX-startX == width);
      assert(endY-startY == height);

      // Set up the pixel array
      pixels.length = height;
      foreach(line, ref slice; pixels)
        {
          // First pixel in the line
          int strt = texWidth*(startY+line) + startX;
          // Get a slice of the pixel data
          slice = cast(Color[])pixelBuf[strt..strt+width];
        }
    }
}
