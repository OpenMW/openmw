#ifndef CSM_WORLD_FIXCELLEDGETOOL_H
#define CSM_WORLD_FIXCELLEDGETOOL_H

#include <QWidget>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>

#include "../doc/document.hpp"

#include "cellcoordinates.hpp"

namespace CSMWorld
{
    class FixCellEdgeTool : public QWidget
    {
        Q_OBJECT

        public:
        FixCellEdgeTool(CSMDoc::Document& document, QWidget* parent = 0);
        ~FixCellEdgeTool ();

        private:
        void createInterface();
        void fixEdges(CSMWorld::CellCoordinates cellCoords);

        CSMDoc::Document& mDocument;
        QGroupBox *mSpinBoxGroup;
        QLabel *mFixCellLabel;
        QSpinBox *mCellXSpinBox;
        QSpinBox *mCellYSpinBox;
        QPushButton *mActionButton;

        const int landSize {ESM::Land::LAND_SIZE};

        private slots:
        void fixNow();
    };
}

#endif
