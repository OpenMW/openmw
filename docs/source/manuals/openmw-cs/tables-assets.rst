#############
Assets Tables
#############


Sounds
******

Sounds are Sound Files wrapped by OpenMW, allowing you to adjust how they behave
once they are played in-game. They are used for many audio events including
spells, NPCs commenting, environment ambients, doors, UI events, when moving items
in the inventory, and so on.

Volume
    More is louder, less is quieter. Maximum is 255.
Min Range
    When the player is within Min Range distance from the Sound, the Volume will
    always be at its maximum defined value.
Max Range
    When the player is beyond Max Range distance from the Sound, the volume will
    fade to 0.
Sound File
    Source sound file on the hard-drive that holds all the actual audio information.
    All available records are visible in the Sound Files table. 


Sound Generators
****************

Sound generators are a way to play Sounds driven by animation events. Animated
creatures or NPCs always require to be paired with a corresponding textkeys file.
This textkeys file defines the extents of each animation, and relevant in this
case, events and triggers occuring at particular animation frames.

For example, a typical textkey entry intended for a Sound Generator would be
named ``SoundGen: Left`` and be hand placed by an animator whenever the left leg
of the creature touches the ground. In OpenMW-CS, the appropriate Sound
Generator of an appropriate type would then be connected to the animated creature.
In this case the type would be `Left Foot`. Once in-game, OpenMW will play the
sound whenever its textkey occurs in the currently playing animation.

Creature
    Which creature uses this effect.
Sound
    Which record of the Sound type is used as the audio source
Sound Generator Type
    Type of the sound generator that is matched to corresponding textkeys.

    * Land
    * Left Foot
    * Moan
    * Right Foot
    * Roar
    * Scream
    * Swim Left
    * Swim Right

    
Meshes
******

Meshes are 3D assets that need to be assigned to objects to render them in the 
game world. Entries in this table are based on contents of ``data/meshes`` 
folder and cannot be edited from OpenMW-CS.  


Icons
*****

Icons are images used in the user interface to represent inventory items, 
spells, and attributes. They can be assigned to relevant records through other 
dialogues in the editor. Entries in this table are based on contents of 
``data/icons`` folder and cannot be edited from OpenMW-CS.  
 

Music Files
***********

Music is played in the background during the game and at special events such as 
intro, death, or level up. Entries in this table are based on contents of 
``data/music`` folder and cannot be edited from OpenMW-CS.  


Sound Files
***********

Sound files are the source audio files on the hard-drive and are used by other
records related to sound. Entries in this table are based on contents of
``data/sounds`` folder and cannot be edited from OpenMW-CS. 


Textures
********

Textures are images used by 3D objects, particle effects, weather system, user 
interface and more. Definitions which mesh uses which texture are included in 
the mesh files and cannot be assigned through the editor. To use a texture to 
paint the terrain, a separate entry is needed in the Land Textures table. 
Entries in this table are based on contents of ``data/textures`` folder and 
cannot be edited from OpenMW-CS.


Videos
******

Videos can be shown at various points in the game, depending on where they are 
called. Entries in this table are based on contents of ``data/videos`` folder 
and cannot be edited from OpenMW-CS.
