#include "dialogue_history.hpp"
#include "window_manager.hpp"
#include "widgets.hpp"
#include "components/esm_store/store.hpp"

#include <iostream>
#include <iterator>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

using namespace MWGui;
using namespace Widgets;

UString DialogueHistory::getColorAtPos(size_t _pos)
{
    UString colour = TextIterator::convertTagColour(getTextColour());
    TextIterator iterator(getCaption());
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

UString DialogueHistory::getColorTextAt(size_t _pos)
{
    bool breakOnNext = false;
    UString colour = TextIterator::convertTagColour(getTextColour());
    UString colour2 = colour;
    TextIterator iterator(getCaption());
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

void DialogueHistory::addDialogHeading(const UString& parText)
{
    UString head("\n#D8C09A");
    head.append(parText);
    head.append("#B29154\n");
    addText(head);
}

void DialogueHistory::addDialogText(const UString& parText)
{
    addText(parText);
    addText("\n");
}

