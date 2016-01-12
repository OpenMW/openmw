#include <apps/openmw/mwworld/ptr.hpp>
#include "Networking.hpp"
#include "LocalPlayer.hpp"
#include "GUIChat.hpp"
#include "GUILogin.hpp"

namespace mwmp
{
    class Main
    {
    public:
        Main();
        ~Main();

        static void Create();
        static void Destroy();
        static const Main &get();
        static void Frame(float dt);
        static void PressedKey(int key);

        Networking *getNetworking() const;
        LocalPlayer *getLocalPlayer() const;
        GUIChat *getChatBox() const;
        GUILogin *getGUILogin() const;

        void UpdateWorld(float dt) const;

    private:
        Main (const Main&);
        ///< not implemented
        Main& operator= (const Main&);
        ///< not implemented
        static Main *pMain;
        Networking *mNetworking;
        LocalPlayer *mLocalPlayer;

        GUIChat *mChat;
        GUILogin *mGUILogin;

        std::string server;
        unsigned short port;

        int keySay;
        int keyChatMode;
    };
}
