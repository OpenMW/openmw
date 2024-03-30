---
-- `openmw.menu` can be used only in menu scripts.
-- @module menu
-- @usage local menu = require('openmw.menu')

---
-- @type STATE
-- @field [parent=#STATE] NoGame
-- @field [parent=#STATE] Running
-- @field [parent=#STATE] Ended

---
-- All possible game states returned by @{#menu.getState}
-- @field [parent=#menu] #STATE STATE

---
-- Current game state
-- @function [parent=#menu] getState
-- @return #STATE

---
-- Start a new game
-- @function [parent=#menu] newGame

---
-- Load the game from a save slot
-- @function [parent=#menu] loadGame
-- @param #string directory name of the save directory (e. g. character)
-- @param #string slotName name of the save slot

---
-- Delete a saved game
-- @function [parent=#menu] deleteGame
-- @param #string directory name of the save directory (e. g. character)
-- @param #string slotName name of the save slot

---
-- Current save directory
-- @function [parent=#menu] getCurrentSaveDir
-- @return #string

---
-- Save the game
-- @function [parent=#menu] saveGame
-- @param #string description human readable description of the save
-- @param #string slotName name of the save slot

---
-- @type SaveInfo
-- @field #string description
-- @field #string playerName
-- @field #string playerLevel
-- @field #number timePlayed Gameplay time for this saved game. Note: available even with [time played](../modding/settings/saves.html#timeplayed) turned off
-- @field #number creationTime Time at which the game was saved, as a timestamp in seconds. Can be passed as the second argument to `os.data`.
-- @field #list<#string> contentFiles

---
-- List of all saves for the given directory
-- @function [parent=#menu] getSaves
-- @param #string directory name of the save directory (e. g. character)
-- @return #list<#SaveInfo>

---
-- List of all available saves, grouped by directory
-- @function [parent=#menu] getAllSaves
-- @return #map<#string, #list<#SaveInfo>>

---
-- Exit the game
-- @function [parent=#menu] quit

return nil
