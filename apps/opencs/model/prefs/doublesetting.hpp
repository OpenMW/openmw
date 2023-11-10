#ifndef CSM_PREFS_DOUBLESETTING_H
#define CSM_PREFS_DOUBLESETTING_H

#include "setting.hpp"

class QDoubleSpinBox;

namespace CSMPrefs
{
    class Category;

    class DoubleSetting : public Setting
    {
        Q_OBJECT

        int mPrecision;
        double mMin;
        double mMax;
        std::string mTooltip;
        double mDefault;
        QDoubleSpinBox* mWidget;

    public:
        DoubleSetting(Category* parent, QMutex* mutex, const std::string& key, const QString& label, double default_);

        DoubleSetting& setPrecision(int precision);

        // defaults to [0, std::numeric_limits<double>::max()]
        DoubleSetting& setRange(double min, double max);

        DoubleSetting& setMin(double min);

        DoubleSetting& setMax(double max);

        DoubleSetting& setTooltip(const std::string& tooltip);

        /// Return label, input widget.
        std::pair<QWidget*, QWidget*> makeWidgets(QWidget* parent) override;

        void updateWidget() override;

    private slots:

        void valueChanged(double value);
    };
}

#endif
