
local M = {}

function M.remap(value, min, max, newMin, newMax)
    return newMin + (value - min) * (newMax - newMin) / (max - min)
end

function M.round(value)
    return value >= 0 and math.floor(value + 0.5) or math.ceil(value - 0.5)
end

function M.clamp(value, low, high)
    return value < low and low or (value > high and high or value)
end

function M.normalizeAngle(angle)
    local fullTurns = angle / (2 * math.pi) + 0.5
    return (fullTurns - math.floor(fullTurns) - 0.5) * (2 * math.pi)
end

return M