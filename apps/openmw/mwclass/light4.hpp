#ifndef OPENW_MWCLASS_LIGHT4
#define OPENW_MWCLASS_LIGHT4

#include "../mwworld/registeredclass.hpp"

#include "esm4base.hpp"

namespace MWClass
{
    class ESM4Light : public MWWorld::RegisteredClass<ESM4Light, ESM4Base<ESM4::Light>>
    {
        friend MWWorld::RegisteredClass<ESM4Light, ESM4Base<ESM4::Light>>;

        ESM4Light();

    public:
        void insertObjectRendering(const MWWorld::Ptr& ptr, const std::string& model,
            MWRender::RenderingInterface& renderingInterface) const override;
        ///< Add reference into a cell for rendering
    };
}
#endif
