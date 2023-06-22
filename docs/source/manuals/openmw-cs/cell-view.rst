Cell View
#########

This window deals with the manipulation of instances within one or more cells.


Grid Snapping
*************

When manipulating one or more instances within the cell view, whether it's position,
rotation, or scale, the instances can be snapped to specific values configured within
the Edit->Preferences->3D Scene editing menu.

To begin snapping an instance, hold down CTRL when transforming the instance, whether
it's with the gizmos or by dragging the mouse, and the instance will snap to the closest
value as you manipulate the instance.

.. note::
    The key to enable snapping must be held before the transformation starts, otherwise snapping won't be enabled.


Snap to reference
=================

If you want to snap instances relative to another instance, you can select a snap target
with SHIFT + Middle Mouse Button to select a snap target. This will highlight the
instance in a yellow wireframe. Then, just with regular snapping, you hold down CTRL
when manipulating the instance(s) and the transformed values will be snapped to
the snap values relative to the snap target's world space.


Editing Instance Properties
===========================

In the 3D view, when you click with SHIFT + Left Mouse Button on an instance,
an Instances table view will open, if it's not open already. The clicked instance will
be highlighted in the Instances table.
