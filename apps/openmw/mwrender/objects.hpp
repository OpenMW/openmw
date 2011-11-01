#include "components/esm_store/cell_store.hpp"

#include "../mwworld/refdata.hpp"

namespace MWRender{
class Objects{
public:
    Objects(){}
    ~Objects(){}
    void insertBegin (ESM::CellRef &ref, bool static_ = false);
    void insertMesh(const std::string &mesh);
	
    /// insert a light related to the most recent insertBegin call.
    void insertLight(float r, float g, float b, float radius);
    void insertObjectPhysics();
	
};
}