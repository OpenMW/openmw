#include "container.hpp"

#include "../mwworld/ptr.hpp"

namespace MWGui
{

    class ItemSelectionDialog : public ContainerBase, public WindowModal
    {
    public:
        ItemSelectionDialog(const std::string& label, ContainerBase::Filter filter, MWBase::WindowManager& parWindowManager);

        typedef MyGUI::delegates::CMultiDelegate0 EventHandle_Void;
        typedef MyGUI::delegates::CMultiDelegate1<MWWorld::Ptr> EventHandle_Item;

        EventHandle_Item eventItemSelected;
        EventHandle_Void eventDialogCanceled;


    private:
        virtual void onReferenceUnavailable() { ; }

        virtual void onSelectedItemImpl(MWWorld::Ptr item);

        void onCancelButtonClicked(MyGUI::Widget* sender);
    };

}
