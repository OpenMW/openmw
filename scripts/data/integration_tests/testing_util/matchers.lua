local module = {}

---
-- Matcher verifying that distance between given value and expected is not greater than maxDistance.
-- @function elementsAreArray
-- @param expected#vector.
-- @usage
-- expectThat(util.vector2(0, 0), closeToVector(util.vector2(0, 1), 1))
function module.closeToVector(expected, maxDistance)
    return function(actual)
        local distance = (expected - actual):length()
        if distance <= maxDistance then
            return ''
        end
        return string.format('%s is too far from expected %s: %s > %s', actual, expected, distance, maxDistance)
    end
end

---
-- Matcher verifying that given value is an array each element of which matches elements of expected.
-- @function elementsAreArray
-- @param expected#array of values or matcher functions.
-- @usage
-- local t = {42, 13}
-- local matcher = function(actual)
--     if actual ~= 42 then
--         return string.format('%s is not 42', actual)
--     end
--     return ''
-- end
-- expectThat({42, 13}, elementsAreArray({matcher, 13}))
function module.elementsAreArray(expected)
    local expected_matchers = {}
    for i, v in ipairs(expected) do
        if type(v) == 'function' then
            expected_matchers[i] = v
        else
            expected_matchers[i] = function (other)
                if expected[i].__eq(expected[i], other) then
                    return ''
                end
                return string.format('%s element %s does no match expected: %s', i, other, expected[i])
            end
        end
    end
    return function(actual)
        if #actual < #expected_matchers then
            return string.format('number of elements is less than expected: %s < %s', #actual, #expected_matchers)
        end
        local message = ''
        for i, v in ipairs(actual) do
            if i > #expected_matchers then
                message = string.format('%s\n%s element is out of expected range: %s', message, i, #expected_matchers)
                break
            end
            local match_message = expected_matchers[i](v)
            if match_message ~= '' then
                message = string.format('%s\n%s', message, match_message)
            end
        end
        return message
    end
end

---
-- Matcher verifying that given number is not a nan.
-- @function isNotNan
-- @usage
-- expectThat(value, isNotNan())
function module.isNotNan()
    return function(actual)
        if actual ~= actual then
            return 'actual value is nan, expected to be not nan'
        end
        return ''
    end
end

---
-- Matcher accepting any value.
-- @function isAny
-- @usage
-- expectThat(value, isAny())
function module.isAny()
    return function(actual)
        return ''
    end
end

local function serializeArray(a)
    local result = nil
    for _, v in ipairs(a) do
        if result == nil then
            result = string.format('{%s', serialize(v))
        else
            result = string.format('%s, %s', result, serialize(v))
        end
    end
    if result == nil then
        return '{}'
    end
    return string.format('%s}', result)
end

local function serializeTable(t)
    local result = nil
    for k, v in pairs(t) do
        if result == nil then
            result = string.format('{%q = %s', k, serialize(v))
        else
            result = string.format('%s, %q = %s', result, k, serialize(v))
        end
    end
    if result == nil then
        return '{}'
    end
    return string.format('%s}', result)
end

local function isArray(t)
    local i = 1
    for _ in pairs(t) do
        if t[i] == nil then
            return false
        end
        i = i + 1
    end
    return true
end

function serialize(v)
    local t = type(v)
    if t == 'string' then
        return string.format('%q', v)
    elseif t == 'table' then
        if isArray(v) then
            return serializeArray(v)
        end
        return serializeTable(v)
    end
    return string.format('%s', v)
end

local function compareScalars(v1, v2)
    if v1 == v2 then
        return ''
    end
    if type(v1) == 'string' then
        return string.format('%q ~= %q', v1, v2)
    end
    return string.format('%s ~= %s', v1, v2)
end

local function collectKeys(t)
    local result = {}
    for key in pairs(t) do
        table.insert(result, key)
    end
    table.sort(result)
    return result
end

local function compareTables(t1, t2)
    local keys1 = collectKeys(t1)
    local keys2 = collectKeys(t2)
    if #keys1 ~= #keys2 then
        return string.format('table size mismatch: %d ~= %d', #keys1, #keys2)
    end
    for i = 1, #keys1 do
        local key1 = keys1[i]
        local key2 = keys2[i]
        if key1 ~= key2 then
            return string.format('table keys mismatch: %q ~= %q', key1, key2)
        end
        local d = compare(t1[key1], t2[key2])
        if d ~= '' then
            return string.format('table values mismatch at key %s: %s', serialize(key1), d)
        end
    end
    return ''
end

function compare(v1, v2)
    local type1 = type(v1)
    local type2 = type(v2)
    if type2 == 'function' then
        return v2(v1)
    end
    if type1 ~= type2 then
        return string.format('types mismatch: %s ~= %s', type1, type2)
    end
    if type1 == 'nil' then
        return ''
    elseif type1 == 'table' then
        return compareTables(v1, v2)
    elseif type1 == 'nil' or type1 == 'boolean' or type1 == 'number' or type1 == 'string' then
        return compareScalars(v1, v2)
    end
    error('unsupported type: %s', type1)
end

---
-- Matcher verifying that given value is equal to expected. Accepts nil, boolean, number, string and table or matcher
-- function.
-- @function equalTo
-- @usage
-- expectThat({a = {42, 'foo', {b = true}}}, equalTo({a = {42, 'foo', {b = true}}}))
function module.equalTo(expected)
    return function(actual)
        local diff = compare(actual, expected)
        if diff == '' then
            return ''
        end
        return string.format('%s; actual: %s; expected: %s', diff, serialize(actual, ''), serialize(expected, ''))
    end
end

return module
