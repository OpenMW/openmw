#include "helpviewer.hpp"

#include <QDesktopServices>
#include <QString>
#include <QUrl>

void Misc::HelpViewer::openHelp(const char* url)
{
    QString link{ OPENMW_DOC_BASEURL };
    link.append(url);
    QDesktopServices::openUrl(QUrl(link));
}
