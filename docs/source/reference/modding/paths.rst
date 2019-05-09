Paths
#####

The following describes the locations for the various OpenMW file paths for different types of files on different operating systems.

.. note::
	Actual location depends on your computer's setup. Username, harddrive, and language may vary.

Configuration files and log files
---------------------------------

+--------------+-----------------------------------------------------------------------------------------------+
| OS           | Location                                                                                      |
+==============+===============================================================================================+
| Linux        | ``$HOME/.config/openmw``                                                                      |
+--------------+-----------------------------------------------------------------------------------------------+
| Mac          | ``$HOME/Library/Preferences/openmw``                                                          |
+--------------+---------------+-------------------------------------------------------------------------------+
| Windows      | File Explorer | ``Documents\My Games\OpenMW``                                                 |
|              |               |                                                                               |
|              | PowerShell    | ``Join-Path ([environment]::GetFolderPath("mydocuments")) "My Games\OpenMW"`` |
|              |               |                                                                               |
|              | Example       | ``C:\Users\Username\Documents\My Games\OpenMW``                               |
+--------------+---------------+-------------------------------------------------------------------------------+

Savegames
---------

+--------------+-----------------------------------------------------------------------------------------------------+
| OS           | Location                                                                                            |
+==============+=====================================================================================================+
| Linux        | ``$HOME/.config/openmw/saves``                                                                      |
+--------------+-----------------------------------------------------------------------------------------------------+
| Mac          | ``$HOME/Library/Application\ Support/openmw/saves``                                                 |
+--------------+---------------+-------------------------------------------------------------------------------------+
| Windows      | File Explorer | ``Documents\My Games\OpenMW\saves``                                                 |
|              |               |                                                                                     |
|              | PowerShell    | ``Join-Path ([environment]::GetFolderPath("mydocuments")) "My Games\OpenMW\saves"`` |
|              |               |                                                                                     |
|              | Example       | ``C:\Users\Username\Documents\My Games\OpenMW\saves``                               |
+--------------+---------------+-------------------------------------------------------------------------------------+

Screenshots
-----------

+--------------+-----------------------------------------------------------------------------------------------+
| OS           | Location                                                                                      |
+==============+===============================================================================================+
| Linux        | ``$HOME/.local/share/openmw``                                                                 |
+--------------+-----------------------------------------------------------------------------------------------+
| Mac          | ``$HOME/Library/Application\ Support/openmw``                                                 |
+--------------+---------------+-------------------------------------------------------------------------------+
| Windows      | File Explorer | ``Documents\My Games\OpenMW``                                                 |
|              |               |                                                                               |
|              | PowerShell    | ``Join-Path ([environment]::GetFolderPath("mydocuments")) "My Games\OpenMW"`` |
|              |               |                                                                               |
|              | Example       | ``C:\Users\Username\Documents\My Games\OpenMW``                               |
+--------------+---------------+-------------------------------------------------------------------------------+
