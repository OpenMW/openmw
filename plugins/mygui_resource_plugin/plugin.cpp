#include "plugin.hpp"

#include <MyGUI_LogManager.h>
#include <MyGUI_FactoryManager.h>
#include <MyGUI_ScrollBar.h>
#include <MyGUI_Gui.h>
#include <MyGUI_Window.h>
#include <MyGUI_LanguageManager.h>

#include <components/bsa/resources.hpp>
#include <components/files/configurationmanager.hpp>
#include <components/fontloader/fontloader.hpp>

#include <components/widgets/tags.hpp>
#include <components/widgets/widgets.hpp>

#include <OgreTextureManager.h>
#include <OgreHardwarePixelBuffer.h>

//FIXME: code duplication
namespace boost
{
struct FallbackMap {
    std::map<std::string,std::string> mMap;
};

void validate(boost::any &v, std::vector<std::string> const &tokens, FallbackMap*, int)
{
    if(v.empty())
    {
        v = boost::any(FallbackMap());
    }

    FallbackMap *map = boost::any_cast<FallbackMap>(&v);

    for(std::vector<std::string>::const_iterator it=tokens.begin(); it != tokens.end(); ++it)
    {
        int sep = it->find(",");
        if(sep < 1 || sep == (int)it->length()-1)
#if (BOOST_VERSION < 104200)
            throw boost::program_options::validation_error("invalid value");
#else
            throw boost::program_options::validation_error(boost::program_options::validation_error::invalid_option_value);
#endif

        std::string key(it->substr(0,sep));
        std::string value(it->substr(sep+1));

        if(map->mMap.find(key) == map->mMap.end())
        {
            map->mMap.insert(std::make_pair (key,value));
        }
    }
}
}

namespace MyGUIPlugin
{

    // Dummy - obsolete when using MyGUI git, because the ScrollBar there has autorepeat support added.
    class MWScrollBar : public MyGUI::ScrollBar
    {
        MYGUI_RTTI_DERIVED(MWScrollBar)
    };

    const std::string& ResourcePlugin::getName() const
    {
        static const std::string name = "OpenMW resource plugin";
        return name;
    }

    void ResourcePlugin::install()
    {

    }
    void ResourcePlugin::uninstall()
    {

    }

    void ResourcePlugin::registerResources()
    {
        boost::program_options::variables_map variables;

        boost::program_options::options_description desc("Allowed options");
        desc.add_options()
        ("data", boost::program_options::value<Files::PathContainer>()->default_value(Files::PathContainer(), "data")->multitoken()->composing())
        ("data-local", boost::program_options::value<std::string>()->default_value(""))
        ("fs-strict", boost::program_options::value<bool>()->implicit_value(true)->default_value(false))
        ("fallback-archive", boost::program_options::value<std::vector<std::string> >()->
            default_value(std::vector<std::string>(), "fallback-archive")->multitoken())
        ("encoding", boost::program_options::value<std::string>()->default_value("win1252"))
        ("fallback", boost::program_options::value<boost::FallbackMap>()->default_value(boost::FallbackMap(), "")
            ->multitoken()->composing(), "fallback values");

        boost::program_options::notify(variables);

        Files::ConfigurationManager cfgManager;
        cfgManager.readConfiguration(variables, desc);

        std::vector<std::string> archives = variables["fallback-archive"].as<std::vector<std::string> >();
        bool fsStrict = variables["fs-strict"].as<bool>();

        Files::PathContainer dataDirs, dataLocal;
        if (!variables["data"].empty()) {
            dataDirs = Files::PathContainer(variables["data"].as<Files::PathContainer>());
        }

        std::string local = variables["data-local"].as<std::string>();
        if (!local.empty()) {
            dataLocal.push_back(Files::PathContainer::value_type(local));
        }

        cfgManager.processPaths (dataDirs);
        cfgManager.processPaths (dataLocal, true);

        if (!dataLocal.empty())
            dataDirs.insert (dataDirs.end(), dataLocal.begin(), dataLocal.end());

        Files::Collections collections (dataDirs, !fsStrict);

        Bsa::registerResources(collections, archives, true, fsStrict);

        std::string encoding(variables["encoding"].as<std::string>());
        std::cout << ToUTF8::encodingUsingMessage(encoding) << std::endl;

        Gui::FontLoader loader(ToUTF8::calculateEncoding(encoding));
        loader.loadAllFonts(false);

        mFallbackMap = variables["fallback"].as<boost::FallbackMap>().mMap;
    }

    void ResourcePlugin::registerWidgets()
    {
        MyGUI::FactoryManager::getInstance().registerFactory<MWScrollBar>("Widget");

        Gui::registerAllWidgets();
    }

    void ResourcePlugin::createTransparentBGTexture()
    {
        // This texture is manually created in OpenMW to be able to change its opacity at runtime in the options menu
        Ogre::TexturePtr tex = Ogre::TextureManager::getSingleton().createManual(
                        "transparent.png",
                        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                        Ogre::TEX_TYPE_2D,
                        1, 1,
                        0,
                        Ogre::PF_A8R8G8B8,
                        Ogre::TU_WRITE_ONLY);
        std::vector<Ogre::uint32> buffer;
        buffer.resize(1);
        const float val = 0.7;
        buffer[0] = (int(255*val) << 24) | (255 << 16) | (255 << 8) | 255;
        memcpy(tex->getBuffer()->lock(Ogre::HardwareBuffer::HBL_DISCARD), &buffer[0], 1*4);
        tex->getBuffer()->unlock();
    }

    void ResourcePlugin::initialize()
    {
        MYGUI_LOGGING("OpenMW_Resource_Plugin", Info, "initialize");

        registerResources();
        registerWidgets();
        createTransparentBGTexture();

        MyGUI::LanguageManager::getInstance().eventRequestTag = MyGUI::newDelegate(this, &ResourcePlugin::onRetrieveTag);
    }

    void ResourcePlugin::shutdown()
    {
        /// \todo cleanup

        MYGUI_LOGGING("OpenMW_Resource_Plugin", Info, "shutdown");
    }

    void ResourcePlugin::onRetrieveTag(const MyGUI::UString& tag, MyGUI::UString& out)
    {
        if (!Gui::replaceTag(tag, out, mFallbackMap))
            out = tag;
    }

}
