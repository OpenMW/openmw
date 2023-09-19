local ambient = require('openmw.ambient')
local core = require('openmw.core')
local self = require('openmw.self')
local storage = require('openmw.storage')
local types = require('openmw.types')

local musicSettings = storage.playerSection('SettingsOMWMusic')

local helpers = require('scripts.omw.music.helpers')

local registeredPlaylists = {}
local playlistsTracksOrder = helpers.getStoredTracksOrder()
local fightingActors = {}
local registrationOrder = 0

local currentPlaylist = nil
local skippedOneFrame = false

local battlePriority = 10
local explorePriority = 100

local function registerPlaylist(playlist)
    helpers.initMissingPlaylistFields(playlist)

    local existingOrder = playlistsTracksOrder[playlist.id]
    if not existingOrder or next(existingOrder) == nil or math.max(unpack(existingOrder)) > #playlist.tracks then
        local newPlaylistOrder = helpers.initTracksOrder(playlist.tracks, playlist.randomize)
        playlistsTracksOrder[playlist.id] = newPlaylistOrder
        helpers.setStoredTracksOrder(playlist.id, newPlaylistOrder)
    else
        playlistsTracksOrder[playlist.id] = existingOrder
    end

    if registeredPlaylists[playlist.id] == nil then
        playlist.registrationOrder = registrationOrder
        registrationOrder = registrationOrder + 1
    end

    registeredPlaylists[playlist.id] = playlist
end

local function setPlaylistActive(id, state)
    if id == nil then
        error("Playlist ID is nil")
    end

    local playlist = registeredPlaylists[id]
    if playlist then
        playlist.active = state
    else
        error(string.format("Playlist '%s' is not registered.", id))
    end
end

local function onCombatTargetsChanged(eventData)
    if eventData.actor == nil then return end

    if next(eventData.targets) ~= nil then
        fightingActors[eventData.actor.id] = true
    else
        fightingActors[eventData.actor.id] = nil
    end
end

local function playerDied()
    ambient.streamMusic("music/special/mw_death.mp3")
end

local function switchPlaylist(newPlaylist)
    local newPlaylistOrder = playlistsTracksOrder[newPlaylist.id]
    local nextTrackIndex = table.remove(newPlaylistOrder)

    if nextTrackIndex == nil then
        error("Can not fetch track: nextTrackIndex is nil")
    end

    -- If there are no tracks left, fill playlist again.
    if next(newPlaylistOrder) == nil then
        newPlaylistOrder = helpers.initTracksOrder(newPlaylist.tracks, newPlaylist.randomize)

        if not newPlaylist.cycleTracks then
            newPlaylist.deactivateAfterEnd = true
        end

        -- If next track for randomized playist will be the same as one we want to play, swap it with random track.
        if newPlaylist.randomize and #newPlaylistOrder > 1 and newPlaylistOrder[1] == nextTrackIndex then
            local index = math.random(2, #newPlaylistOrder)
            newPlaylistOrder[1], newPlaylistOrder[index] = newPlaylistOrder[index], newPlaylistOrder[1]
        end

        playlistsTracksOrder[newPlaylist.id] = newPlaylistOrder
    end

    helpers.setStoredTracksOrder(newPlaylist.id, newPlaylistOrder)

    local trackPath = newPlaylist.tracks[nextTrackIndex]
    if trackPath == nil then
        error(string.format("Can not fetch track with index %s from playlist '%s'.", nextTrackIndex, newPlaylist.id))
    else
        ambient.streamMusic(trackPath, newPlaylist.fadeOut)
        if newPlaylist.playOneTrack then
            newPlaylist.deactivateAfterEnd = true
        end
    end

    currentPlaylist = newPlaylist
end

local function onFrame(dt)
    -- Skip a first frame to allow other scripts to update playlists state.
    if not skippedOneFrame then
        skippedOneFrame = true
        return
    end

    if not core.sound.isEnabled() then return end
	
	-- Do not allow to switch playlists when player is dead
	local musicPlaying = ambient.isMusicPlaying()
	if types.Actor.isDead(self) and musicPlaying then return end

    local combatMusicEnabled = musicSettings:get("CombatMusicEnabled") and helpers.isInCombat(fightingActors)
    setPlaylistActive("battle", combatMusicEnabled)

    local newPlaylist = helpers.getActivePlaylistByPriority(registeredPlaylists)

    if not newPlaylist then
        ambient.stopMusic()

        if currentPlaylist ~= nil then
            currentPlaylist.deactivateAfterEnd = nil
        end

        currentPlaylist = nil
        return
    end

    if newPlaylist == currentPlaylist and musicPlaying then return end

    if newPlaylist and newPlaylist.deactivateAfterEnd then
        newPlaylist.deactivateAfterEnd = nil
        newPlaylist.active = false
        return
    end

    switchPlaylist(newPlaylist)
end

registerPlaylist({ id = "battle", priority = battlePriority, randomize = true })
registerPlaylist({ id = "explore", priority = explorePriority, randomize = true, active = true })

return {
    ---
    -- @module Music
    -- @usage require('openmw.interfaces').Music
    interfaceName = 'Music',
    interface = {
        --- Interface version
        -- @field [parent=#Music] #number version
        version = 0,
        ---
        -- Set state for playlist with given ID
        -- @function [parent=#Music] setPlaylistActive
        -- @param #string id Playlist ID
        -- @param #boolean state Playlist is active
        setPlaylistActive = setPlaylistActive,
        ---
        -- Register given playlist
        -- @function [parent=#Music] registerPlaylist
        -- @param #table playlist Playlist data. Can contain:
        --
        -- * `id` - #string, playlist ID
        -- * `priority` - #number, playlist priority (lower value means higher priority)
        -- * `fadeOut` - #number, Time in seconds to fade out current track before starting this one. If nil, allow the engine to choose the value.
        -- * `tracks` - #list<#string>, Paths of track files for playlist (if nil, use all tracks from 'music/{id}/' folder)
        -- * `active` - #boolean, tells if playlist is active (default is false)
        -- * `randomize` - #boolean, tells if playlist should shuffle its tracks during playback (default is false). When all tracks are played, they are randomized again.
        -- * `playOne` - #boolean, tells if playlist should be automatically deactivated after one track is played (default is false)
        -- * `cycleTracks` - #boolean, if true, tells to start playlist from beginning once all tracks are played, otherwise playlist becomes deactivated (default is true).
        registerPlaylist = registerPlaylist
    },
    engineHandlers = {
        onFrame = onFrame
    },
    eventHandlers = {
        Died = playerDied,
        OMWMusicCombatTargetsChanged = onCombatTargetsChanged
    }
}
