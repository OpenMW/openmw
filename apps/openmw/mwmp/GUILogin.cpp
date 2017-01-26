//
// Created by koncord on 19.05.16.
//

#include "GUILogin.hpp"
#include <MyGUI_EditBox.h>
#include <MyGUI_Button.h>


GUILogin::GUILogin() : WindowModal("tes3mp_login.layout")
{
    center(); // center window

    setVisible(false);


    getWidget(mLogin, "EditLogin");
    getWidget(mServer, "EditServer");
    getWidget(mPort, "EditPort");
    getWidget(mConnect, "ButtonConnect");

}
