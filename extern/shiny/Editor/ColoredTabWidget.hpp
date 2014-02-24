#ifndef SHINY_EDITOR_COLOREDTABWIDGET_H
#define SHINY_EDITOR_COLOREDTABWIDGET_H

#include <QTabWidget>

namespace sh
{

/// Makes tabBar() public to allow changing tab title colors.
class ColoredTabWidget : public QTabWidget
{
public:
	ColoredTabWidget(QWidget* parent = 0)
	: QTabWidget(parent) {}

	QTabBar* tabBar()
	{
		return QTabWidget::tabBar();
	}
};

}

#endif
