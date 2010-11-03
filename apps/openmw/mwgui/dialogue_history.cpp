#include "dialogue_history.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"
#include "window_manager.hpp"
#include "widgets.hpp"
#include "components/esm_store/store.hpp"

#include <assert.h>
#include <iostream>
#include <iterator>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

using namespace MWGui;
using namespace Widgets;

UString DialogeHistory::getColorAtPos(size_t _pos)
{
    UString colour = TextIterator::convertTagColour(mText->getTextColour());
    TextIterator iterator(mText->getCaption());
    while(iterator.moveNext())
    {
        size_t pos = iterator.getPosition();
        iterator.getTagColour(colour);
        if (pos < _pos)
            continue;
        else if (pos == _pos)
            break;
    }
    return colour;
}

UString DialogeHistory::getColorTextAt(size_t _pos)
{
    bool breakOnNext = false;
    UString colour = TextIterator::convertTagColour(mText->getTextColour());
    UString colour2 = colour;
    TextIterator iterator(mText->getCaption());
    TextIterator col_start = iterator;
    while(iterator.moveNext())
    {
        size_t pos = iterator.getPosition();
        iterator.getTagColour(colour);
        if(colour != colour2)
        {
            if(breakOnNext)
            {
                return getOnlyText().substr(col_start.getPosition(), iterator.getPosition()-col_start.getPosition());
            }
            col_start = iterator;
            colour2 = colour;
        }
        if (pos < _pos)
            continue;
        else if (pos == _pos)
        {
            breakOnNext = true;
        }
    }
    return "";
}

void DialogeHistory::addDialogHeading(const UString& parText)
{
    UString head("\n#00FF00");
    head.append(parText);
    head.append("#FFFFFF\n");
    addText(head);
}

void DialogeHistory::addDialogText(const UString& parText)
{
    addText(parText);
    addText("\n");
}

