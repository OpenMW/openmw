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

return module
