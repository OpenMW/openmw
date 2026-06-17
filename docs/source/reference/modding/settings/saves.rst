Saves Settings
##############

.. omw-setting::
   :title: character
   :type: string
   :default: ""

   The default character shown in the Load Game menu.
   Automatically updated when a different character is selected.

.. omw-setting::
   :title: autosave
   :type: boolean
   :range: true, false
   :default: true
   :location: :bdg-info:`In Game > Options > Prefs`

   Determines whether the game auto-saves when the character rests.

.. omw-setting::
   :title: max quicksaves
   :type: int
   :range: >0
   :default: 1
   :location: :bdg-success:`Launcher > Settings > Miscellaneous`

   Number of quicksave and autosave slots available.
   If greater than 1, quicksaves are created sequentially.
   When the max is reached, the oldest quicksave is overwritten on the next quicksave.