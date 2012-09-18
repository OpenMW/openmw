#include "waitdialog.hpp"

namespace MWGui
{

    WaitDialog::WaitDialog(MWBase::WindowManager &parWindowManager)
        : WindowBase("openmw_wait_dialog.layout", parWindowManager)
    {
        center();
    }

}
