#ifndef OENGINE_OGRE_FADE_H
#define OENGINE_OGRE_FADE_H

/*
  A class that handles fading in the screen from black or fading it out to black.
  
  To achieve this, it uses a full-screen Ogre::Overlay
  
  inspired by http://www.ogre3d.org/tikiwiki/FadeEffectOverlay (heavily adjusted)
 */
 
#include <OgreFrameListener.h>

namespace Ogre
{
    class TextureUnitState;
    class Overlay;
}

namespace OEngine {
namespace Render
{
  class Fader
  {
  public:
    Fader();
    
    void update(float dt);
    
    void fadeIn(const float time);
    void fadeOut(const float time);
    void fadeTo(const int percent, const float time);
    
  private:
    enum FadingMode
    {
        FadingMode_In,
        FadingMode_Out,
        FadingMode_To,
        FadingMode_None // no fading
    };
  
    Ogre::TextureUnitState* mFadeTextureUnit;
    Ogre::Overlay* mOverlay;
    
    FadingMode mMode;
    float mRemainingTime;
    float mTargetTime;
    float mTargetAlpha;
    
  protected:
  };
}}
#endif
