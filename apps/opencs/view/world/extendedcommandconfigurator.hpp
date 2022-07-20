#ifndef CSVWORLD_EXTENDEDCOMMANDCONFIGURATOR_HPP
#define CSVWORLD_EXTENDEDCOMMANDCONFIGURATOR_HPP

#include <map>

#include <QWidget>

#include "../../model/world/universalid.hpp"

class QPushButton;
class QGroupBox;
class QCheckBox;
class QLabel;
class QHBoxLayout;

namespace CSMDoc
{
    class Document;
}

namespace CSMWorld
{
    class CommandDispatcher;
    class Data;
}

namespace CSVWorld
{
    class ExtendedCommandConfigurator : public QWidget
    {
            Q_OBJECT

        public:
            enum Mode { Mode_None, Mode_Delete, Mode_Revert };
        
        private:
            typedef std::map<QCheckBox *, CSMWorld::UniversalId> CheckBoxMap;

            QPushButton *mPerformButton;
            QPushButton *mCancelButton;
            QGroupBox *mTypeGroup;
            CheckBoxMap mTypeCheckBoxes;
            int mNumUsedCheckBoxes;
            int mNumChecked;

            Mode mMode;
            CSMWorld::CommandDispatcher *mCommandDispatcher;
            CSMWorld::Data &mData;
            std::vector<std::string> mSelectedIds;
            
            bool mEditLock;

            void setupGroupLayout();
            void setupCheckBoxes(const std::vector<CSMWorld::UniversalId> &types);
            void lockWidgets(bool locked);

        public:
            ExtendedCommandConfigurator(CSMDoc::Document &document,
                                        const CSMWorld::UniversalId &id,
                                        QWidget *parent = nullptr);

            void configure(Mode mode, const std::vector<std::string> &selectedIds);
            void setEditLock(bool locked);

        protected:
            void resizeEvent(QResizeEvent *event) override;

        private slots:
            void performExtendedCommand();
            void checkBoxStateChanged(int state);
            void dataIdListChanged();

        signals:
            void done();
    };
}

#endif
