/*!
	@file
	@author		Albert Semenov
	@date		11/2007
	@module
*/
/*
	This file is part of MyGUI.

	MyGUI is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MyGUI is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with MyGUI.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "MyGUI_Precompiled.h"
#include "MyGUI_RenderOut.h"
#include "MyGUI_Utility.h"

#include "MyGUI_Gui.h"
#include "MyGUI_FontManager.h"
#include "MyGUI_LayerManager.h"
#include "MyGUI_SkinManager.h"
#include "MyGUI_StaticText.h"

namespace MyGUI
{
	namespace implement
	{

		// структура информации об одной строке
		struct info
		{
			info() : num(0), count(1)  { }
			info(size_t _num, const std::string& _line) : num(_num), count(1), line(_line) { }

			size_t num;
			size_t count;
			std::string line;
		};

		void render_out(const std::string& _value)
		{
			// очередь
			typedef std::deque<info> DequeInfo;

			// текущая строка
			static size_t num = 0;
			// очередь всех наших строк
			static DequeInfo lines;

			const int offset = 10;
			const size_t count_lines = 20;
			static const std::string font = "DejaVuSans.14";
			static const std::string layer = "Statistic";
			static const std::string skin = "StaticText";

			static StaticText* widget = nullptr;
			static StaticText* widget_shadow = nullptr;

			if (widget == nullptr)
			{
				Gui * gui = Gui::getInstancePtr();
				if (gui == nullptr) return;

				const IntSize& size = gui->getViewSize();

				if (!LayerManager::getInstance().isExist(layer)) return;
				if (!SkinManager::getInstance().isExist(skin)) return;


				widget_shadow = gui->createWidget<StaticText>(skin, IntCoord(offset + 1, offset + 1, size.width - offset - offset, size.height - offset - offset), Align::Stretch, layer);
				widget_shadow->setNeedMouseFocus(false);
				widget_shadow->setTextAlign(Align::Default);
				widget_shadow->setTextColour(Colour::Black);

				widget = gui->createWidget<StaticText>(skin, IntCoord(offset, offset, size.width - offset - offset, size.height - offset - offset), Align::Stretch, layer);
				widget->setNeedMouseFocus(false);
				widget->setTextAlign(Align::Default);
				widget->setTextColour(Colour::White);

				if (FontManager::getInstance().getByName(font) != nullptr)
				{
					widget_shadow->setFontName(font);
					widget->setFontName(font);
				}
			}

			// первый раз просто добавляем
			if (lines.empty())
			{
				lines.push_back(info(num++, _value));

			}
			// не первый раз мы тут
			else
			{
				// сравниваем последнюю строку
				if (lines.back().line == _value) lines.back().count ++;
				else
				{
					lines.push_back(info(num++, _value));
					// удаляем лишнее
					if (lines.size() > count_lines) lines.pop_front();
				}

			}

			// а вот теперь выводми строки
			std::string str_out;
			str_out.reserve(2048);

			for (DequeInfo::iterator iter=lines.begin(); iter != lines.end(); ++iter)
			{
				str_out += utility::toString("[ ", (unsigned int)iter->num, (iter->count > 1) ? (" , " + utility::toString((unsigned int)iter->count)) : "", " ]  ", iter->line, "\n");
			}

			// непосредственный вывод
			widget_shadow->setCaption(str_out);
			widget->setCaption(str_out);
		}
	}

} // namespace MyGUI
