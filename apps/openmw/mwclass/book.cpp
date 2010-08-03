
#include "book.hpp"

#include <components/esm/loadbook.hpp>

namespace MWClass
{
    void Book::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Book);

        registerClass (typeid (ESM::Book).name(), instance);
    }
}
