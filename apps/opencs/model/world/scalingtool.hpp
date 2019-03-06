#ifndef CSM_WORLD_SCALINGTOOL_H
#define CSM_WORLD_SCALINGTOOL_H

#include <QWidget>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>

#include "../doc/document.hpp"

#include "cellcoordinates.hpp"

namespace CSMWorld
{
    class ScalingTool : public QWidget
    {
        Q_OBJECT

        public:
        ScalingTool(CSMDoc::Document& document, QWidget* parent = 0);
        ~ScalingTool ();

        private:
        void createSpinBoxes();
        void scaleLand(int cellX_Source_CornerA, int cellY_Source_CornerA, int cellX_Source_CornerB, int cellY_Source_CornerB,
            int cellX_Target_CornerA, int cellY_Target_CornerA, int cellX_Target_CornerB, int cellY_Target_CornerB);

        CSMDoc::Document& mDocument;
        QGroupBox *mSourceGroupBox;
        QGroupBox *mTargetGroupBox;
        QLabel *mCellLabel_Source_CornerA;
        QLabel *mCellLabel_Source_CornerB;
        QLabel *mCellLabel_Target_CornerA;
        QLabel *mCellLabel_Target_CornerB;
        QSpinBox *mCellX_SpinBox_Source_CornerA;
        QSpinBox *mCellY_SpinBox_Source_CornerA;
        QSpinBox *mCellX_SpinBox_Source_CornerB;
        QSpinBox *mCellY_SpinBox_Source_CornerB;
        QSpinBox *mCellX_SpinBox_Target_CornerA;
        QSpinBox *mCellY_SpinBox_Target_CornerA;
        QSpinBox *mCellX_SpinBox_Target_CornerB;
        QSpinBox *mCellY_SpinBox_Target_CornerB;
        QComboBox *mHeightMethodSelector;
        QComboBox *mTextureMethodSelector;
        QCheckBox *mScaleZCheckBox;
        QPushButton *mActionButton;

        const int landSize {ESM::Land::LAND_SIZE};

        private slots:
        void scaleNow();
    };
}

#endif
