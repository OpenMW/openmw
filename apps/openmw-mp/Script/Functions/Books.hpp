#ifndef OPENMW_BOOKAPI_HPP
#define OPENMW_BOOKAPI_HPP

#define BOOKAPI \
    {"InitializeBookChanges",  BookFunctions::InitializeBookChanges},\
    \
    {"GetBookChangesSize",     BookFunctions::GetBookChangesSize},\
    \
    {"AddBook",                BookFunctions::AddBook},\
    \
    {"GetBookId",              BookFunctions::GetBookId},\
    \
    {"SendBookChanges",        BookFunctions::SendBookChanges}

class BookFunctions
{
public:

    static void InitializeBookChanges(unsigned short pid) noexcept;

    static unsigned int GetBookChangesSize(unsigned short pid) noexcept;

    static void AddBook(unsigned short pid, const char* bookId) noexcept;

    static const char *GetBookId(unsigned short pid, unsigned int i) noexcept;

    static void SendBookChanges(unsigned short pid, bool toOthers = false) noexcept;
private:

};

#endif //OPENMW_BOOKAPI_HPP
