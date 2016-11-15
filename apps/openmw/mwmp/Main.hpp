#include <apps/openmw/mwworld/ptr.hpp>
#include <boost/program_options.hpp>
#include "Networking.hpp"
#include "LocalPlayer.hpp"
#include "GUIController.hpp"
#include "WorldController.hpp"

namespace mwmp
{
    class Main
    {
    public:
        Main();
        ~Main();

        static void OptionsDesc(boost::program_options::options_description *desc);
        static void Configure(const boost::program_options::variables_map &variables);
        static void Init(std::vector<std::string> &content);
        static void PostInit();
        static void Destroy();
        static const Main &get();
        static void Frame(float dt);
        static void PressedKey(int key);

        static bool isValidPacketScript(std::string script);

        Networking *getNetworking() const;
        LocalPlayer *getLocalPlayer() const;
        GUIController *getGUIController() const;
        WorldController *getWorldController() const;

        void UpdateWorld(float dt) const;

    private:
        static std::string addr;
        Main (const Main&);
        ///< not implemented
        Main& operator= (const Main&);
        ///< not implemented
        static Main *pMain;
        Networking *mNetworking;
        LocalPlayer *mLocalPlayer;

        GUIController *mGUIController;
        WorldController *mWorldController;
        std::string server;
        unsigned short port;
    };
}
