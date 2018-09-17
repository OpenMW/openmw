#include "sharedstatebutton.hpp"

namespace Gui
{

    SharedStateButton::SharedStateButton()
        : mIsMousePressed(false)
        , mIsMouseFocus(false)
    {
    }

    void SharedStateButton::shutdownOverride()
    {
        ButtonGroup group = mSharedWith; // make a copy so that we don't nuke the vector during iteration
        for (ButtonGroup::iterator it = group.begin(); it != group.end(); ++it)
        {
            (*it)->shareStateWith(ButtonGroup());
        }
    }

    void SharedStateButton::shareStateWith(ButtonGroup shared)
    {
        mSharedWith = shared;
    }

    void SharedStateButton::onMouseButtonPressed(int _left, int _top, MyGUI::MouseButton _id)
    {
        mIsMousePressed = true;
        Base::onMouseButtonPressed(_left, _top, _id);
        updateButtonState();
    }

    void SharedStateButton::onMouseButtonReleased(int _left, int _top, MyGUI::MouseButton _id)
    {
        mIsMousePressed = false;
        Base::onMouseButtonReleased(_left, _top, _id);
        updateButtonState();
    }

    void SharedStateButton::onMouseSetFocus(MyGUI::Widget *_old)
    {
        mIsMouseFocus = true;
        Base::onMouseSetFocus(_old);
        updateButtonState();
    }

    void SharedStateButton::onMouseLostFocus(MyGUI::Widget *_new)
    {
        mIsMouseFocus = false;
        Base::onMouseLostFocus(_new);
        updateButtonState();
    }

    void SharedStateButton::baseUpdateEnable()
    {
        Base::baseUpdateEnable();
        updateButtonState();
    }

    void SharedStateButton::setStateSelected(bool _value)
    {
        Base::setStateSelected(_value);
        updateButtonState();

        for (ButtonGroup::iterator it = mSharedWith.begin(); it != mSharedWith.end(); ++it)
        {
            (*it)->MyGUI::Button::setStateSelected(getStateSelected());
        }
    }

    bool SharedStateButton::_setState(const std::string &_value)
    {
        bool ret = _setWidgetState(_value);
        if (ret)
        {
            for (ButtonGroup::iterator it = mSharedWith.begin(); it != mSharedWith.end(); ++it)
            {
                (*it)->_setWidgetState(_value);
            }
        }
        return ret;
    }

    void SharedStateButton::updateButtonState()
    {
        if (getStateSelected())
        {
            if (!getInheritedEnabled())
            {
                if (!_setState("disabled_checked"))
                    _setState("disabled");
            }
            else if (mIsMousePressed)
            {
                if (!_setState("pushed_checked"))
                    _setState("pushed");
            }
            else if (mIsMouseFocus)
            {
                if (!_setState("highlighted_checked"))
                    _setState("pushed");
            }
            else
                _setState("normal_checked");
        }
        else
        {
            if (!getInheritedEnabled())
                _setState("disabled");
            else if (mIsMousePressed)
                _setState("pushed");
            else if (mIsMouseFocus)
                _setState("highlighted");
            else
                _setState("normal");
        }
    }

    void SharedStateButton::createButtonGroup(ButtonGroup group)
    {
        for (ButtonGroup::iterator it = group.begin(); it != group.end(); ++it)
        {
            (*it)->shareStateWith(group);
        }
    }

}
