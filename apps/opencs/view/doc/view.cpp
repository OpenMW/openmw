
#include "view.hpp"

CSVDoc::View::View (CSMDoc::Document *document) : mDocument (document)
{
    resize (200, 200);
    setWindowTitle ("New Document");
}