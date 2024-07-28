---
-- `openmw.animation` defines functions that allow control of character animations
-- Note that for some methods, such as @{openmw.animation#playBlended} you should use the associated methods on the 
-- [AnimationController](interface_animation.html) interface rather than invoking this API directly.
-- @module animation
-- @usage local anim = require('openmw.animation')

--- Possible @{#Priority} values
-- @field [parent=#animation] #Priority PRIORITY

--- `animation.PRIORITY`
-- @type Priority
-- @field #number Default "0"
-- @field #number WeaponLowerBody "1"
-- @field #number SneakIdleLowerBody "2"
-- @field #number SwimIdle "3"
-- @field #number Jump "4"
-- @field #number Movement "5"
-- @field #number Hit "6"
-- @field #number Weapon "7"
-- @field #number Block "8"
-- @field #number Knockdown "9"
-- @field #number Torch "10"
-- @field #number Storm "11"
-- @field #number Death "12" 
-- @field #number Scripted "13" Special priority used by scripted animations. When any animation with this priority is present, all animations without this priority are paused.

--- Possible @{#BlendMask} values
-- @field [parent=#animation] #BlendMask BLEND_MASK

--- `animation.BLEND_MASK`
-- @type BlendMask
-- @field #number LowerBody "1" All bones from 'Bip01 pelvis' and below
-- @field #number Torso "2" All bones from 'Bip01 Spine1' and up, excluding arms
-- @field #number LeftArm "4" All bones from 'Bip01 L Clavicle' and out
-- @field #number RightArm "8" All bones from 'Bip01 R Clavicle' and out
-- @field #number UpperBody "14" All bones from 'Bip01 Spine1' and up, including arms
-- @field #number All "15" All bones

--- Possible @{#BoneGroup} values
-- @field [parent=#animation] #BoneGroup BONE_GROUP

--- `animation.BONE_GROUP`
-- @type BoneGroup
-- @field #number LowerBody "1" All bones from 'Bip01 pelvis' and below
-- @field #number Torso "2" All bones from 'Bip01 Spine1' and up, excluding arms
-- @field #number LeftArm "3" All bones from 'Bip01 L Clavicle' and out
-- @field #number RightArm "4" All bones from 'Bip01 R Clavicle' and out


---
-- Check if the object has an animation object or not
-- @function [parent=#animation] hasAnimation
-- @param openmw.core#GameObject actor
-- @return #boolean

---
-- Skips animations for one frame, equivalent to mwscript's SkipAnim
-- Can be used only in local scripts on self.
-- @function [parent=#animation] skipAnimationThisFrame
-- @param openmw.core#GameObject actor

---
-- Get the absolute position within the animation track of the given text key
-- @function [parent=#animation] getTextKeyTime
-- @param openmw.core#GameObject actor
-- @param #string text key
-- @return #number

---
-- Check if the given animation group is currently playing
-- @function [parent=#animation] isPlaying
-- @param openmw.core#GameObject actor
-- @param #string groupName
-- @return #boolean

---
-- Get the current absolute time of the given animation group if it is playing, or -1 if it is not playing.
-- @function [parent=#animation] getCurrentTime
-- @param openmw.core#GameObject actor
-- @param #string groupName
-- @return #number

---
-- Check whether the animation is a looping animation or not. This is determined by a combination 
-- of groupName, some of which are hardcoded to be looping, and the presence of loop start/stop keys.
-- The groupNames that are hardcoded as looping are the following, as well as per-weapon-type suffixed variants of each.
-- "walkforward", "walkback", "walkleft", "walkright", "swimwalkforward", "swimwalkback", "swimwalkleft", "swimwalkright", 
-- "runforward", "runback", "runleft", "runright", "swimrunforward", "swimrunback", "swimrunleft", "swimrunright", 
-- "sneakforward", "sneakback", "sneakleft", "sneakright", "turnleft", "turnright", "swimturnleft", "swimturnright",
-- "spellturnleft", "spellturnright", "torch", "idle", "idle2", "idle3", "idle4", "idle5", "idle6", "idle7", "idle8", 
-- "idle9", "idlesneak", "idlestorm", "idleswim", "jump", "inventoryhandtohand", "inventoryweapononehand", 
-- "inventoryweapontwohand", "inventoryweapontwowide"
-- @function [parent=#animation] isLoopingAnimation
-- @param openmw.core#GameObject actor
-- @param #string groupName
-- @return #boolean


---
-- Cancels and removes the animation group from the list of active animations
-- Can be used only in local scripts on self.
-- @function [parent=#animation] cancel
-- @param openmw.core#GameObject actor
-- @param #string groupName

---
-- Enables or disables looping for the given animation group. Looping is enabled by default.
-- Can be used only in local scripts on self.
-- @function [parent=#animation] setLoopingEnabled
-- @param openmw.core#GameObject actor
-- @param #string groupName
-- @param #boolean enabled

---
-- Returns the completion of the animation, or nil if the animation group is not active.
-- @function [parent=#animation] getCompletion
-- @param openmw.core#GameObject actor
-- @param #string groupName
-- @return #number, #nil

---
-- Returns the remaining number of loops, not counting the current loop, or nil if the animation group is not active.
-- @function [parent=#animation] getLoopCount
-- @param openmw.core#GameObject actor
-- @param #string groupName
-- @return #number, #nil

---
-- Get the current playback speed of an animation group, or nil if the animation group is not active.
-- @function [parent=#animation] getSpeed
-- @param openmw.core#GameObject actor
-- @param #string groupName
-- @return #number, #nil

---
-- Modifies the playback speed of an animation group.
-- Note that this is not sticky and only affects the speed until the currently playing sequence ends.
-- Can be used only in local scripts on self.
-- @function [parent=#animation] setSpeed
-- @param openmw.core#GameObject actor
-- @param #string groupName
-- @param #number speed The new animation speed, where speed=1 is normal speed.

---
-- Clears all animations currently in the animation queue. This affects animations played by mwscript, @{openmw.animation#playQueued}, and ai packages, but does not affect animations played using @{openmw.animation#playBlended}.
-- Can be used only in local scripts on self.
-- @function [parent=#animation] clearAnimationQueue
-- @param openmw.core#GameObject actor
-- @param #boolean clearScripted whether to keep animation with priority Scripted or not.

---
-- Acts as a slightly extended version of MWScript's LoopGroup. Plays this animation exclusively
-- until it ends, or the queue is cleared using #clearAnimationQueue. Use #clearAnimationQueue and the `startkey` option
-- to imitate the behavior of LoopGroup's play modes.
-- Can be used only in local scripts on self.
-- @function [parent=#animation] playQueued
-- @param openmw.core#GameObject actor
-- @param #string groupName
-- @param #table options A table of play options.  Can contain:
--
--   * `loops` - a number >= 0, the number of times the animation should loop after the first play (default: infinite).
--   * `speed` - a floating point number >= 0, the speed at which the animation should play (default: 1);
--   * `startKey` - the animation key at which the animation should start (default: "start")
--   * `stopKey` - the animation key at which the animation should end (default: "stop")
--   * `forceLoop` - a boolean, to set if the animation should loop even if it's not a looping animation (default: false)
-- 
-- @usage -- Play death1 without waiting. Equivalent to playgroup, death1, 1
-- anim.clearAnimationQueue(self, false)
-- anim.playQueued(self, 'death1')
-- 
-- @usage -- Play an animation group with custom start/stop keys
-- anim.clearAnimationQueue(self, false)
-- anim.playQueued(self, 'spellcast', { startKey = 'self start', stopKey = 'self stop' })
--

---
-- Play an animation directly. You probably want to use the [AnimationController](interface_animation.html) interface, which will trigger relevant handlers, 
-- instead of calling this directly. Note that the still hardcoded character controller may at any time and for any reason alter
-- or cancel currently playing animations, so making your own calls to this function either directly or through the [AnimationController](interface_animation.html)
-- interface may be of limited utility. For now, use openmw.animation#playQueued to script your own animations.
-- Can be used only in local scripts on self.
-- @function [parent=#animation] playBlended
-- @param openmw.core#GameObject actor
-- @param #string groupName
-- @param #table options A table of play options. Can contain:
--
--   * `loops` - a number >= 0, the number of times the animation should loop after the first play (default: 0).
--   * `priority` - Either a single #Priority value that will be assigned to all bone groups. Or a table mapping bone groups to its priority (default: PRIORITY.Default).
--   * `blendMask` - A mask of which bone groups to include in the animation (Default: BLEND_MASK.All.
--   * `autoDisable` - If true, the animation will be immediately  removed upon finishing, which means information will not be possible to query once completed. (Default: true)
--   * `speed` - a floating point number >= 0, the speed at which the animation should play (default: 1)
--   * `startKey` - the animation key at which the animation should start (default: "start")
--   * `stopKey` - the animation key at which the animation should end (default: "stop")
--   * `startPoint` - a floating point number 0 <= value <= 1, starting completion of the animation (default: 0)
--   * `forceLoop` - a boolean, to set if the animation should loop even if it's not a looping animation (default: false)

---
-- Check if the actor's animation has the given animation group or not.
-- @function [parent=#animation] hasGroup
-- @param openmw.core#GameObject actor
-- @param #string groupName
-- @return #boolean 

---
-- Check if the actor's skeleton has the given bone or not
-- @function [parent=#animation] hasBone
-- @param openmw.core#GameObject actor
-- @param #string boneName
-- @return #boolean 

---
-- Get the current active animation for a bone group
-- @function [parent=#animation] getActiveGroup
-- @param openmw.core#GameObject actor
-- @param #number boneGroup Bone group enum, see @{openmw.animation#BONE_GROUP}
-- @return #string 

---
-- Plays a VFX on the actor.
-- Can be used only in local scripts on self.
-- @function [parent=#animation] addVfx
-- @param openmw.core#GameObject actor
-- @param #any static @{openmw.core#StaticRecord} or #string ID
-- @param #table options optional table of parameters. Can contain:
--
--   * `loop` - boolean, if true the effect will loop until removed (default: 0).
--   * `boneName` - name of the bone to attach the vfx to. (default: "")
--   * `particle` - name of the particle texture to use. (default: "")
--   * `vfxId` - a string ID that can be used to remove the effect later, using #removeVfx, and to avoid duplicate effects. The default value of "" can have duplicates. To avoid interaction with the engine, use unique identifiers unrelated to magic effect IDs. The engine uses this identifier to add and remove magic effects based on what effects are active on the actor. If this is set equal to the @{openmw.core#MagicEffectId} identifier of the magic effect being added, for example core.magic.EFFECT_TYPE.FireDamage, then the engine will remove it once the fire damage effect on the actor reaches 0. (Default: ""). 
-- 
-- @usage local mgef = core.magic.effects.records[myEffectName]
-- anim.addVfx(self, 'VFX_Hands', {boneName = 'Bip01 L Hand', particle = mgef.particle, loop = mgef.continuousVfx, vfxId = mgef.id..'_myuniquenamehere'})
-- -- later:
-- anim.removeVfx(self, mgef.id..'_myuniquenamehere')
-- 

---
-- Removes a specific VFX
-- Can be used only in local scripts on self.
-- @function [parent=#animation] removeVfx
-- @param openmw.core#GameObject actor
-- @param #number vfxId an integer ID that uniquely identifies the VFX to remove

---
-- Removes all vfx from the actor
-- Can be used only in local scripts on self.
-- @function [parent=#animation] removeAllVfx
-- @param openmw.core#GameObject actor




return nil

