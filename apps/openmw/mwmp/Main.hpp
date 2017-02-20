#include "../mwworld/ptr.hpp"
#include <boost/program_options.hpp>

namespace mwmp
{
    class WorldController;
    class GUIController;
    class LocalPlayer;
    class Networking;

    class Main
    {
    public:
        Main();
        ~Main();

        static void optionsDesc(boost::program_options::options_description *desc);
        static void configure(const boost::program_options::variables_map &variables);
        static bool init(std::vector<std::string> &content);
        static void postInit();
        static void destroy();
        static const Main &get();
        static void frame(float dt);
        static void pressedKey(int key);

        static bool isValidPacketScript(std::string script);

        Networking *getNetworking() const;
        LocalPlayer *getLocalPlayer() const;
        GUIController *getGUIController() const;
        WorldController *getWorldController() const;

        void updateWorld(float dt) const;

    private:
        static std::string addr;
        static std::string passw;
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
