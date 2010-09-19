#ifndef MANGLE_REND2D_SPRITE_H
#define MANGLE_REND2D_SPRITE_H

namespace Mangle
{
  namespace Rend2D
  {
    /**
       A Sprite is either a bitmap to be drawn or an output of area
       for blitting other bitmaps, or both. They are created by the
       Driver.
    */
    struct Sprite
    {
      /// Draw a sprite in the given position
      virtual void draw(Sprite *s,                // The sprite to draw
                        int x, int y,             // Destination position
                        int sx=0, int sy=0,       // Source position
                        int w=-1, int h=-1        // Amount to draw. -1 means remainder.
                        ) = 0;

      virtual ~Sprite() {}

      // Information retrieval
      virtual int width() = 0;
      virtual int height() = 0;

      /// Fill the sprite with the given pixel value. The pixel format
      /// depends on the format of the sprite.
      virtual void fill(int value) = 0;

      /// Set one pixel value. The pixel format depends on the sprite
      /// format. This is not expected to be fast, and in some
      /// implementations may not work at all.
      virtual void pixel(int x, int y, int value) {}
    };
  }
}
#endif
