#ifndef OPENMW_ESSIMPORTER_IMPORTER_H
#define OPENMW_ESSIMPORTER_IMPORTER_H

#include <string>

namespace ESSImport {

class Importer {
public:
    Importer(std::string essfile, std::string outfile, std::string encoding);

    void run();

    void compare();

private:
    std::string mEssFile;
    std::string mOutFile;
    std::string mEncoding;
};

}

#endif
