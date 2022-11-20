#####
Music
#####


The music system in OpenMW is hard-coded and audio files are used automatically 
when placed into correct folders.

* ``data/music/explore`` - Folder contents are played outside combat, shuffling between available files.
* ``data/music/battle`` - Folder contents are played during combat, shuffling between available files.

Three special files also require a specific name. In this example a ``.mp3`` 
format is used, but any other format will also work as long as the filename is 
correct.

* ``data/music/special/morrowind title.mp3`` - Main menu music.
* ``data/music/special/mw_death.mp3`` - Plays when the player dies.
* ``data/music/special/mw_triumph.mp3`` - Plays when the level up menu appears.


Supported Formats
*****************

OpenMW uses `FFmpeg framework <https://ffmpeg.org/>`_ and thus supports a great 
variety of formats. Using either ``.mp3`` or ``.ogg`` for music files is a 
common choice.
