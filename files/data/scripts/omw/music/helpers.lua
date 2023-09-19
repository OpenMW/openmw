local debug = require('openmw.debug')
local storage = require('openmw.storage')
local vfs = require('openmw.vfs')

local playlistsSection = storage.playerSection('OMWMusicPlaylistsTrackOrder')
playlistsSection:setLifeTime(storage.LIFE_TIME.GameSession)

local function getTracksFromDirectory(path)
    local result = {}
    for fileName in vfs.pathsWithPrefix(path) do
        table.insert(result, fileName)
    end

    return result
end

local function initMissingPlaylistFields(playlist)
    if playlist.id == nil or playlist.priority == nil then
        error("Can not register playlist: 'id' and 'priority' are mandatory fields")
    end

    if playlist.tracks == nil then
        playlist.tracks = getTracksFromDirectory(string.format("music/%s/", playlist.id))
    end

    if playlist.active == nil then
        playlist.active = false
    end

    if playlist.randomize == nil then
        playlist.randomize = false
    end

    if playlist.cycleTracks == nil then
        playlist.cycleTracks = true
    end

    if playlist.playOneTrack == nil then
        playlist.playOneTrack = false
    end
end

local function shuffle(data)
    for i = #data, 1, -1 do
        local j = math.random(i)
        data[i], data[j] = data[j], data[i]
    end
    return data
end

local function initTracksOrder(tracks, randomize)
    local tracksOrder = {}
    for i, track in ipairs(tracks) do
        tracksOrder[i] = i
    end

    if randomize then
        shuffle(tracksOrder)
    end

    return tracksOrder
end

local function isPlaylistActive(playlist)
    return playlist.active and next(playlist.tracks) ~= nil
end

local function getStoredTracksOrder()
    -- We need a writeable playlists table here.
    return playlistsSection:asTable()
end

local function setStoredTracksOrder(playlistId, playlistTracksOrder)
    playlistsSection:set(playlistId, playlistTracksOrder)
end

local function getActivePlaylistByPriority(playlists)
    local newPlaylist = nil
    for _, playlist in pairs(playlists) do
        if isPlaylistActive(playlist) then
            if newPlaylist == nil or playlist.priority < newPlaylist.priority or
                (playlist.priority == newPlaylist.priority and playlist.registrationOrder > newPlaylist.registrationOrder) then
                newPlaylist = playlist
            end
        end
    end

    return newPlaylist
end

local function isInCombat(fightingActors)
    return next(fightingActors) ~= nil and debug.isAIEnabled()
end

local functions = {
    getActivePlaylistByPriority = getActivePlaylistByPriority,
    getStoredTracksOrder = getStoredTracksOrder,
    getTracksFromDirectory = getTracksFromDirectory,
    initMissingPlaylistFields = initMissingPlaylistFields,
    initTracksOrder = initTracksOrder,
    isInCombat = isInCombat,
    isPlaylistActive = isPlaylistActive,
    setStoredTracksOrder = setStoredTracksOrder
}

return functions
