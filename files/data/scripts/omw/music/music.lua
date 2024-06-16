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
    engineHandlers = {
        onFrame = onFrame
    },
    eventHandlers = {
        Died = playerDied,
        OMWMusicCombatTargetsChanged = onCombatTargetsChanged
    }
}
