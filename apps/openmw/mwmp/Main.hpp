#include <apps/openmw/mwworld/ptr.hpp>
#include <boost/program_options.hpp>
#include "Networking.hpp"
#include "LocalPlayer.hpp"
#include "GUIController.hpp"

namespace mwmp
{
    class Main
    {
    public:
        Main();
        ~Main();

        static void OptionsDesc(boost::program_options::options_description *desc);
        static void Configure(const boost::program_options::variables_map &variables);
        static void Create();
        static void Destroy();
        static const Main &get();
        static void Frame(float dt);
        static void PressedKey(int key);

        Networking *getNetworking() const;
        LocalPlayer *getLocalPlayer() const;
        GUIController *getGUIController() const;

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

        std::string server;
        unsigned short port;
    };
}
