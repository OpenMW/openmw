#ifndef OPENMW_COMPONENTS_SCENEUTIL_VISMASK_H
#define OPENMW_COMPONENTS_SCENEUTIL_VISMASK_H

namespace SceneUtil
{

    /// Node masks used for controlling visibility of game objects.
    /// @par Any node in the OSG scene graph can have a node mask. When traversing the scene graph,
    /// the node visitor's traversal mask is bitwise AND'ed with the node mask. If the result of this test is
    /// 0, then the node <i>and all its child nodes</i> are not processed.
    /// @par Important traversal masks are the camera's cull mask (determines what is visible),
    /// the update visitor mask (what is updated) and the intersection visitor mask (what is
    /// selectable through mouse clicks or other intersection tests).
    /// @par In practice, it can be useful to make a "hierarchy" out of the node masks - e.g. in OpenMW,
    /// all 3D rendering nodes are child of a Scene Root node with Mask_Scene. When we do not want 3D rendering,
    /// we can just omit Mask_Scene from the traversal mask, and do not need to omit all the individual
    /// element masks (water, sky, terrain, etc.) since the traversal will already have stopped at the Scene root node.
    /// @par The comments within the VisMask enum should give some hints as to what masks are commonly "child" of
    /// another mask, or what type of node this mask is usually set on.
    /// @note The mask values are not serialized within models, nor used in any other way that would break backwards
    /// compatibility if the enumeration values were to be changed. Feel free to change them when it makes sense.
    enum VisMask
    {
        Mask_Disabled = 0, // For hidden nodes

        Mask_UpdateVisitor = 0x1, // reserved for separating UpdateVisitors from CullVisitors

        // child of Scene
        Mask_Effect = (1<<1),
        Mask_Debug = (1<<2),
        Mask_Actor = (1<<3),
        Mask_Player = (1<<4),
        Mask_Sky = (1<<5),
        Mask_Water = (1<<6), // choose Water or SimpleWater depending on detail required
        Mask_SimpleWater = (1<<7),
        Mask_Terrain = (1<<8),
        Mask_FirstPerson = (1<<9),
        Mask_Object = (1<<10),
        Mask_Static = (1<<11),

        // child of Sky
        Mask_Sun = (1<<12),
        Mask_WeatherParticles = (1<<13),

        // top level masks
        Mask_Scene = (1<<14),
        Mask_GUI = (1<<15),

        // Set on a ParticleSystem Drawable
        Mask_ParticleSystem = (1<<16),

        // Set on cameras within the main scene graph
        Mask_RenderToTexture = (1<<17),

        Mask_PreCompile = (1<<18),

        // Set on a camera's cull mask to enable the LightManager
        Mask_Lighting = (1<<19),

        // For pathgrid nodes debugging
        Mask_Pathgrid = (1<<20),

        // Editor control elements
        Mask_EditorCellMarker = (1<<21),
        Mask_EditorCellArrow = (1<<22),
        Mask_EditorCellBorder = (1<<23),
        Mask_EditorReference = (1<<24),

        // Default mask for OSG nodes
        Mask_Default = 0xffffffff
    };

}

#endif
