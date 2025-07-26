GUI Settings
############

.. omw-setting::
   :title: scaling factor
   :type: float32
   :range: 0.5 to 8.0
   :default: 1.0
   :location: :bdg-success:`Launcher > Settings > Interface`

   This setting scales GUI windows.
   A value of 1.0 results in normal scale.
   Larger values increase GUI scale for high resolution displays.

.. omw-setting::
   :title: font size
   :type: int
   :range: 12 to 18
   :default: 16
   :location: :bdg-success:`Launcher > Settings > Interface`

   Specifies glyph size for in-game fonts.
   Default bitmap fonts work best at 16px; others may be blurry.
   trueType fonts do not have this issue.

.. omw-setting::
   :title: menu transparency
   :type: float32
   :range: 0.0 (transparent) to 1.0 (opaque)
   :default: 0.84

   Controls transparency of GUI windows.
   Adjustable in game via Menu Transparency slider in the Prefs panel of Options.

.. omw-setting::
   :title: tooltip delay
   :type: float32
   :range: ≥ 0.0
   :default: 0.0

   Seconds delay before tooltip appears when hovering over an item in GUI mode.
   Tooltips show context-sensitive info (weight, value, damage, etc.).
   Adjustable in game with Menu Help Delay slider in Prefs panel.

.. omw-setting::
   :title: keyboard navigation
   :type: boolean
   :range: true, false
   :default: true

   Enable or disable keyboard navigation (arrow keys, tab focus, spacebar, Use key).

.. omw-setting::
   :title: stretch menu background
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Interface`

   Stretch or shrink main menu, splash screens, intro movies, and cut scenes
   to fill the video resolution, possibly distorting aspect ratio.
   Bethesda assets are 4:3 ratio; others may differ.
   If false, assets are centered with black bars filling remainder.

.. omw-setting::
   :title: subtitles
   :type: boolean
   :range: true, false
   :default: false

   Enable or disable subtitles for NPC dialog and some sound effects.
   Subtitles appear in a tooltip box at screen lower center.
   Toggleable in game with Subtitles button in Prefs panel.

.. omw-setting::
   :title: hit fader
   :type: boolean
   :range: true, false
   :default: true


   Enables or disables the red flash overlay when the character takes damage.
   Disabling causes the player to "bleed" like NPCs.

.. omw-setting::
   :title: werewolf overlay
   :type: boolean
   :range: true, false
   :default: true


   Enable or disable the werewolf visual effect in first-person mode.

.. omw-setting::
   :title: color background owned
   :type: color
   :range: [0, 1]
   :default: 0.15 0.0 0.0 1.0


   Background color of tooltip and crosshair when hovering over an NPC-owned item.
   Four floating point values: red, green, blue, alpha (alpha ignored).
   No effect if "show owned" in Game Settings is false.

.. omw-setting::
   :title: color crosshair owned
   :type: color
   :range: [0, 1]
   :default: 1.0 0.15 0.15 1.0


   Crosshair color when hovering over an NPC-owned item.
   Four floating point values: red, green, blue, alpha (alpha ignored).
   No effect if crosshair setting in HUD is false or "show owned" in Game Settings is false.

.. omw-setting::
   :title: color topic enable
   :type: boolean
   :range: true, false
   :default: false
   :location: :bdg-success:`Launcher > Settings > Interface`

   Controls whether dialogue topics in the list are colored by their state.
   See related "color topic specific" and "color topic exhausted".

.. omw-setting::
   :title: color topic specific
   :type: color
   :range: [0, 1]
   :default: 0.45 0.5 0.8 1

   Overrides color of dialogue topics with unique actor responses.
   Four floating point values: red, green, blue, alpha (alpha ignored).
   Unique if Actor filter matches speaking actor and not read yet.

.. omw-setting::
   :title: color topic specific over
   :type: color
   :range: [0, 1]
   :default: 0.6 0.6 0.85 1

   "Over" color for dialogue topics meeting "color topic specific" criteria.
   Four floating point values; alpha ignored.
   Active GUI element via keyboard or mouse events.

.. omw-setting::
   :title: color topic specific pressed
   :type: color
   :range: [0, 1]
   :default: 0.3 0.35 0.75 1

   "Pressed" color for dialogue topics meeting "color topic specific".
   Four floating point values; alpha ignored.
   Active GUI element receiving sustained input.

.. omw-setting::
   :title: color topic exhausted
   :type: color
   :range: [0, 1]
   :default: 0.3 0.3 0.3 1

   Overrides color of dialogue topics exhausted by the player.
   Four floating point values; alpha ignored.
   Exhausted if response has been seen.

.. omw-setting::
   :title: color topic exhausted over
   :type: color
   :range: [0, 1]
   :default: 0.55 0.55 0.55 1

   "Over" color for dialogue topics meeting "color topic exhausted".
   Four floating point values; alpha ignored.
   Active GUI element via keyboard or mouse.

.. omw-setting::
   :title: color topic exhausted pressed
   :type: color
   :range: [0, 1]
   :default: 0.45 0.45 0.45 1

   "Pressed" color for dialogue topics meeting "color topic exhausted".
   Four floating point values; alpha ignored.
   Active GUI element receiving sustained input.
