#ifndef OENGINE_OGRE_FADE_H
#define OENGINE_OGRE_FADE_H

/*
  A class that handles fading in the screen from black or fading it out to black.
  
  To achieve this, it uses a full-screen Rectangle2d
 */
 
namespace Ogre
{
    class TextureUnitState;
    class Rectangle2D;
    class SceneManager;
}

namespace OEngine {
namespace Render
{
    class Fader
    {
    public:
        Fader(Ogre::SceneManager* sceneMgr);
        ~Fader();

        void update(float dt);

        void fadeIn(const float time);
        void fadeOut(const float time);
        void fadeTo(const int percent, const float time);

        void setFactor (float factor) { mFactor = factor; }

    private:
        enum FadingMode
        {
            FadingMode_In,
            FadingMode_Out
        };

        void applyAlpha();

        Ogre::TextureUnitState* mFadeTextureUnit;
        Ogre::Rectangle2D* mRectangle;

        FadingMode mMode;

        float mRemainingTime;
        float mTargetTime;
        float mTargetAlpha;
        float mCurrentAlpha;
        float mStartAlpha;

        float mFactor;

        Ogre::SceneManager* mSceneMgr;
    };
    }}
#endif
