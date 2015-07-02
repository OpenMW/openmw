#ifndef CSVWORLD_EXTENDEDCOMMANDCONFIGURATOR_HPP
#define CSVWORLD_EXTENDEDCOMMANDCONFIGURATOR_HPP

#include <map>

#include <QWidget>

class QPushButton;
class QGroupBox;
class QCheckBox;
class QHBoxLayout;

namespace CSMDoc
{
    class Document;
}

namespace CSMWorld
{
    class CommandDispatcher;
    class UniversalId;
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
            QHBoxLayout *mButtonLayout;
            QGroupBox *mTypeGroup;
            CheckBoxMap mTypeCheckBoxes;
            int mNumUsedCheckBoxes;

            Mode mMode;
            CSMWorld::CommandDispatcher *mCommandDispatcher;
            
            void setupGroupLayout();
            void setupCheckBoxes(const std::vector<CSMWorld::UniversalId> &types);

        public:
            ExtendedCommandConfigurator(CSMDoc::Document &document,
                                        const CSMWorld::UniversalId &id,
                                        QWidget *parent = 0);
            virtual ~ExtendedCommandConfigurator();

            void configure(Mode mode);

        protected:
            virtual void resizeEvent(QResizeEvent *event);

        private slots:
            void performExtendedCommand();

        signals:
            void done();
    };
}

#endif
