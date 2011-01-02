#include "layouts.hpp"

#include "../mwworld/class.hpp"
#include "../mwmechanics/mechanicsmanager.hpp"
#include "../mwgui/window_manager.hpp"

#include <cmath>
#include <algorithm>
#include <iterator>

using namespace MWGui;


HUD::HUD(int width, int height, bool fpsSwitch)
    : Layout("openmw_hud_layout.xml")
{
    setCoord(0,0, width, height);

    // Energy bars
    getWidget(health, "Health");
    getWidget(magicka, "Magicka");
    getWidget(stamina, "Stamina");

    // Item and spell images and status bars
    getWidget(weapImage, "WeapImage");
    getWidget(weapStatus, "WeapStatus");
    getWidget(spellImage, "SpellImage");
    getWidget(spellStatus, "SpellStatus");

    getWidget(effectBox, "EffectBox");
    getWidget(effect1, "Effect1");

    getWidget(minimap, "MiniMap");
    getWidget(compass, "Compass");

    getWidget(crosshair, "Crosshair");

    getWidget(fpsbox, "FPSBox");
    getWidget(fpscounter, "FPSCounter");

    fpsbox->setVisible(fpsSwitch);

    compass->setImageTexture("textures\\compass.dds");
    crosshair->setImageTexture("textures\\target.dds");

    // These are just demo values, you should replace these with
    // real calls from outside the class later.
    setWeapIcon("icons\\w\\tx_knife_iron.dds");
    setWeapStatus(90, 100);
    setSpellIcon("icons\\s\\b_tx_s_rstor_health.dds");
    setSpellStatus(65, 100);
    setEffect("icons\\s\\tx_s_chameleon.dds");
}

void HUD::setFPS(float fps)
{
    fpscounter->setCaption(boost::lexical_cast<std::string>((int)fps));
}


void HUD::setStats(int h, int hmax, int m, int mmax, int s, int smax)
{
    health->setProgressRange(hmax);
    health->setProgressPosition(h);
    magicka->setProgressRange(mmax);
    magicka->setProgressPosition(m);
    stamina->setProgressRange(smax);
    stamina->setProgressPosition(s);
}

void HUD::setWeapIcon(const char *str)
{
    weapImage->setImageTexture(str);
}

void HUD::setSpellIcon(const char *str)
{
    spellImage->setImageTexture(str);
}

void HUD::setWeapStatus(int s, int smax)
{
    weapStatus->setProgressRange(smax);
    weapStatus->setProgressPosition(s);
}

void HUD::setSpellStatus(int s, int smax)
{
    spellStatus->setProgressRange(smax);
    spellStatus->setProgressPosition(s);
}

void HUD::setEffect(const char *img)
{
    effect1->setImageTexture(img);
}

void HUD::setValue(const std::string& id, const MWMechanics::DynamicStat<int>& value)
{
    static const char *ids[] =
    {
        "HBar", "MBar", "FBar", 0
    };

    for (int i=0; ids[i]; ++i)
        if (ids[i]==id)
        {
            switch (i)
            {
                case 0:
                    health->setProgressRange (value.getModified());
                    health->setProgressPosition (value.getCurrent());
                    break;
                case 1:
                    magicka->setProgressRange (value.getModified());
                    magicka->setProgressPosition (value.getCurrent());
                    break;
                case 2:
                    stamina->setProgressRange (value.getModified());
                    stamina->setProgressPosition (value.getCurrent());
                    break;
            }
        }
}
