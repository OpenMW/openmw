#ifndef OENGINE_OGRE_RENDERER_H
#define OENGINE_OGRE_RENDERER_H

/*
  Ogre renderer class
 */

#include <string>
#include <stdint.h>

#include <OgreTexture.h>


struct SDL_Window;
struct SDL_Surface;

namespace Ogre
{
    class Root;
    class RenderWindow;
    class SceneManager;
    class Camera;
    class Viewport;
    class ParticleEmitterFactory;
    class ParticleAffectorFactory;
}

namespace OgreInit
{
    class OgreInit;
}

namespace OEngine
{
    namespace Render
    {
        struct WindowSettings
        {
            bool vsync;
            bool fullscreen;
            bool window_border;
            int window_x, window_y;
            int screen;
            std::string fsaa;
            std::string icon;
        };

        class WindowSizeListener
        {
        public:
            virtual void windowResized (int x, int y) = 0;
        };

        class OgreRenderer
        {
            Ogre::Root *mRoot;
            Ogre::RenderWindow *mWindow;
            SDL_Window *mSDLWindow;
            Ogre::SceneManager *mScene;
            Ogre::Camera *mCamera;
            Ogre::Viewport *mView;

            OgreInit::OgreInit* mOgreInit;

            WindowSizeListener* mWindowListener;

            int mWindowWidth;
            int mWindowHeight;
            bool mOutstandingResize;

            // Store system gamma ramp on window creation. Restore system gamma ramp on exit
            uint16_t mOldSystemGammaRamp[256*3];

        public:
            OgreRenderer()
            : mRoot(NULL)
            , mWindow(NULL)
            , mSDLWindow(NULL)
            , mScene(NULL)
            , mCamera(NULL)
            , mView(NULL)
            , mOgreInit(NULL)
            , mWindowListener(NULL)
            , mWindowWidth(0)
            , mWindowHeight(0)
            , mOutstandingResize(false)
            {
            }

            ~OgreRenderer();

            /** Configure the renderer. This will load configuration files and
            set up the Root and logging classes. */
            void configure(
                const std::string &logPath, // Path to directory where to store log files
                const std::string &renderSystem,
                const std::string &rttMode);      // Enable or disable logging

            /// Create a window with the given title
            void createWindow(const std::string &title, const WindowSettings& settings);

            void setWindowGammaContrast(float gamma, float contrast);
            void restoreWindowGammaRamp();

            /// Set up the scene manager, camera and viewport
            void adjustCamera(
                float fov=55,                      // Field of view angle
                float nearClip=5                   // Near clip distance
            );

            void setFov(float fov);

            /// Kill the renderer.
            void cleanup();

            void update(float dt);

            /// Write a screenshot to file
            void screenshot(const std::string &file, const std::string& imageFormat);

            void windowResized(int x, int y);

            /// Get the Root
            Ogre::Root *getRoot() { return mRoot; }

            /// Get the rendering window
            Ogre::RenderWindow *getWindow() { return mWindow; }

            /// Get the SDL Window
            SDL_Window *getSDLWindow() { return mSDLWindow; }

            /// Get the scene manager
            Ogre::SceneManager *getScene() { return mScene; }

            /// Camera
            Ogre::Camera *getCamera() { return mCamera; }

            /// Viewport
            Ogre::Viewport *getViewport() { return mView; }

            void setWindowListener(WindowSizeListener* listener);

            void adjustViewport();
        };
    }
}
#endif
