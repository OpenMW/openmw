local M = {}
M.__Content = true

function validateContentChild(v)
    if not (type(v) == 'table' or v.__type and v.__type.name == 'LuaUi::Element') then
        error('Content can only contain tables and Elements')
    end
end

M.new = function(source)
    local result = {}
    result.__nameIndex = {}
    for i, v in ipairs(source) do
        validateContentChild(v)
        result[i] = v
        if type(v.name) == 'string' then
            result.__nameIndex[v.name] = i
        end
    end
    return setmetatable(result, M)
end
local function validateIndex(self, index)
    if type(index) ~= 'number' then
        error('Unexpected Content key: ' .. tostring(index))
    end
    if index < 1 or (#self + 1) < index then
        error('Invalid Content index: ' .. tostring(index))
    end
end

local function getIndexFromKey(self, key)
    local index = key
    if type(key) == 'string' then
        index = self.__nameIndex[key]
        if not index then
            error('Unexpected content key:' .. key)
        end
    end
    validateIndex(self, index)
    return index
end

local methods = {
    insert = function(self, index, value)
        validateIndex(self, index)
        validateContentChild(value)
        for i = #self, index, -1 do
            rawset(self, i + 1, rawget(self, i))
            local name = rawget(self, i + 1)
            if name then
                self.__nameIndex[name] = i + 1
            end
        end
        rawset(self, index, value)
        if value.name then
            self.__nameIndex[value.name] = index
        end
    end,
    indexOf = function(self, value)
        if type(value) == 'string' then
            return self.__nameIndex[value]
        else
            for i = 1, #self do
                if rawget(self, i) == value then
                    return i
                end
            end
        end
        return nil
    end,
    add = function(self, value)
        self:insert(#self + 1, value)
        return #self
    end,
}
M.__index = function(self, key)
    if methods[key] then return methods[key] end
    local index = getIndexFromKey(self, key)
    return rawget(self, index)
end
local function nameAt(self, index)
    local v = rawget(self, index)
    return v and type(v.name) == 'string' and v.name
end

local function remove(self, index)
    local oldName = nameAt(self, index)
    if oldName then
        self.__nameIndex[oldName] = nil
    end
    if index > #self then
        error('Invalid Content index:' .. tostring(index))
    end
    for i = index, #self - 1 do
        local v = rawget(self, i + 1)
        rawset(self, i, v)
        if type(v.name) == 'string' then
            self.__nameIndex[v.name] = i
        end
    end
    rawset(self, #self, nil)
end

local function assign(self, index, value)
    local oldName = nameAt(self, index)
    if oldName then
        self.__nameIndex[oldName] = nil
    end
    rawset(self, index, value)
    if value.name then
        self.__nameIndex[value.name] = index
    end
end

M.__newindex = function(self, key, value)
    local index = getIndexFromKey(self, key)
    if value == nil then
        remove(self, index)
    else
        validateContentChild(value)
        assign(self, index, value)
    end
end
M.__tostring = function(self)
    return ('UiContent{%d layouts}'):format(#self)
end
local function next(self, index)
    local v = rawget(self, index)
    if v then
        return index + 1, v
    else
        return nil, nil
    end
end

M.__pairs = function(self)
    return next, self, 1
end
M.__ipairs = M.__pairs
M.__metatable = false

return M
