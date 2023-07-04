Drag and Drop
#############

Many tasks in OpenMW-CS become noticeably easier and faster by using the drag and
drop functionality. This article covers the many situations where this works.


Records to Input Fields
***********************

Certain record types in OpenMW-CS have input fields where other records need to 
be assigned. Assignment can be done manually by typing the ID of the record in 
the chosen input field. This can be slow though, and in the spirit of this 
article, a record can simply be dragged into a valid field that accepts its 
type.

.. figure:: https://gitlab.com/OpenMW/openmw-docs/-/raw/master/docs/source/manuals/openmw-cs/_static/drag_model_to_field.webp
    :align: center
    
    Object record accepts a Mesh record into one of its input fields.

Keep in mind that not all records expose all of their input fields in the 
tables. Some input fields are only visible and accessible in the detailed view of 
a particular instance.


Objects to 3D scene
*******************

Records can be dragged from the Objects table into the 3D View to create a new 
instance of that object in the target cell.


Filtering
*********

Using regular expressions to filter table contents is powerful, but can at the same 
time be rather complicated for users not familiar with the syntax. OpenMW-CS thus
supports drag and drop of records or variables directly into the filters field,
automatically creating the appropriate filter.

Below are just two examples of how easy it is to create filters on-the-fly to 
improve your workflow. Many more uses are possible as the same logic works and
can be used for filtering across OpenMW-CS.

.. figure:: https://gitlab.com/OpenMW/openmw-docs/-/raw/master/docs/source/manuals/openmw-cs/_static/drag_object_filter.webp
    :align: center
    
    In Objects table, drag a Record Type field into the filters field and show only
    objects of this particular record type.

.. figure:: https://gitlab.com/OpenMW/openmw-docs/-/raw/master/docs/source/manuals/openmw-cs/_static/drag_instance_filter.webp
    :align: center
    
    In Instances table, drag a cell ID into the filters field
    and show only instances that match the newly created filter.
    

Creating Topic Infos
********************

Topic Infos can be created by hand, but since a typical game can have hundreds of 
Topic Infos this can become very time-consuming . An alternative, and quite faster 
way, is to drag one or more Topic records into the Topic Infos table. This will 
automatically create relevant Topic Info records, already connected to the
appropriate Topic.


Land Texture for Terrain Painting
*********************************

Drag and dropping a Land Texture record onto the paint brush button in the 3D 
view will allow painting the terrain with that particular texture.

.. figure:: https://gitlab.com/OpenMW/openmw-docs/-/raw/master/docs/source/manuals/openmw-cs/_static/drag_land_texture.webp
    :align: center
    
    Drag a Land Texture to the brush widget in the 3D View to paint the terrain with this texture.
