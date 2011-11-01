#include "components/esm_store/cell_store.hpp"

#include "../mwworld/refdata.hpp"
#include "../mwworld/ptr.hpp"

namespace MWRender{
class Objects{
private:
	OEngine::Render::OgreRenderer &rend;
public:
    Objects(OEngine::Render::OgreRenderer& _rend): rend(_rend){}
    ~Objects(){}
   void insertBegin (const MWWorld::Ptr& ptr, bool enabled, bool static_);
    void insertMesh (const MWWorld::Ptr& ptr, const std::string& mesh);
	void insertLight (const MWWorld::Ptr& ptr, float r, float g, float b, float radius);
	
    /// insert a light related to the most recent insertBegin call.
   
};
}