#include "keybindingpage.hpp"

#include <cassert>
#include <string>
#include <utility>
#include <vector>

#include <QComboBox>
#include <QGridLayout>
#include <QPushButton>
#include <QStackedLayout>
#include <QVBoxLayout>

#include <apps/opencs/view/prefs/pagebase.hpp>

#include "../../model/prefs/category.hpp"
#include "../../model/prefs/setting.hpp"
#include "../../model/prefs/state.hpp"

namespace CSVPrefs
{
    KeyBindingPage::KeyBindingPage(CSMPrefs::Category& category, QWidget* parent)
        : PageBase(category, parent)
        , mStackedLayout(nullptr)
        , mPageLayout(nullptr)
        , mPageSelector(nullptr)
    {
        // Need one widget for scroll area
        QWidget* topWidget = new QWidget();
        QVBoxLayout* topLayout = new QVBoxLayout(topWidget);

        // Allows switching between "pages"
        QWidget* stackedWidget = new QWidget();
        mStackedLayout = new QStackedLayout(stackedWidget);

        mPageSelector = new QComboBox();
        connect(mPageSelector, qOverload<int>(&QComboBox::currentIndexChanged), mStackedLayout,
            &QStackedLayout::setCurrentIndex);

        QFrame* lineSeparator = new QFrame(topWidget);
        lineSeparator->setFrameShape(QFrame::HLine);
        lineSeparator->setFrameShadow(QFrame::Sunken);

        // Reset key bindings button
        QPushButton* resetButton = new QPushButton("Reset to Defaults", topWidget);
        connect(resetButton, &QPushButton::clicked, this, &KeyBindingPage::resetKeyBindings);

        topLayout->addWidget(mPageSelector);
        topLayout->addWidget(stackedWidget);
        topLayout->addWidget(lineSeparator);
        topLayout->addWidget(resetButton);
        topLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);

        // Add each option
        for (CSMPrefs::Category::Iterator iter = category.begin(); iter != category.end(); ++iter)
            addSetting(*iter);

        setWidgetResizable(true);
        setWidget(topWidget);
    }

    void KeyBindingPage::addSetting(CSMPrefs::Setting* setting)
    {
        std::pair<QWidget*, QWidget*> widgets = setting->makeWidgets(this);

        if (widgets.first)
        {
            // Label, Option widgets
            assert(mPageLayout);

            int next = mPageLayout->rowCount();
            mPageLayout->addWidget(widgets.first, next, 0);
            mPageLayout->addWidget(widgets.second, next, 1);
        }
        else if (widgets.second)
        {
            // Wide single widget
            assert(mPageLayout);

            int next = mPageLayout->rowCount();
            mPageLayout->addWidget(widgets.second, next, 0, 1, 2);
        }
        else
        {
            if (setting->getLabel().isEmpty())
            {
                // Insert empty space
                assert(mPageLayout);

                int next = mPageLayout->rowCount();
                mPageLayout->addWidget(new QWidget(), next, 0);
            }
            else
            {
                // Create new page
                QWidget* pageWidget = new QWidget();
                mPageLayout = new QGridLayout(pageWidget);
                mPageLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);

                mStackedLayout->addWidget(pageWidget);

                mPageSelector->addItem(setting->getLabel());
            }
        }
    }

    void KeyBindingPage::resetKeyBindings()
    {
        CSMPrefs::State::get().resetCategory("Key Bindings");
    }
}
