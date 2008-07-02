/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (cpp_overlay.cpp) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

 */

OverlayManager *om;
Overlay *owl;
PanelOverlayElement *cont;

int visible;

void crap(char *p, ColourValue v)
{
  std::cerr << p << ": " << v.getAsRGBA() << "\n";
}

extern "C" void cpp_debug(int32_t i)
{
  std::cerr << "Running cpp_debug(" << i << ")\n";

  if(om)
    {
      if(visible)
	{
	  visible = 0;
	  owl->hide();
	}
      else
	{
	  visible = 1;
	  owl->show();
	}

      return;
    }

  om = OverlayManager::getSingletonPtr();

  owl = om->create("HUD");

  OverlayElement *e = om->createOverlayElementFromFactory("Panel", "Hopp");
  cont = (PanelOverlayElement*)e;

  cont->setDimensions(0.3, 0.5);
  cont->setPosition(0.1, 0.2);
  cont->setMaterialName(LASTNAME);

  /*
   * I suggest creating a custom material based on a "live" texture,
   * where we can specify alpha transparency and draw everything
   * ourselves. (Reinvent the wheel at every chance! :)
   */
  MaterialPtr material = MaterialManager::getSingleton().getByName(LASTNAME);
  std::cerr << "Material " << LASTNAME << "\n";
  Pass* pass = material->getTechnique(0)->getPass(0);
  pass->setSceneBlending(SBT_TRANSPARENT_ALPHA);

  owl->add2D(cont);
  owl->show();
  visible = 1;
}
