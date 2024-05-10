local testing = require('testing_util')
local core = require('openmw.core')

function iterateOverRecords(records)
    local firstRecordId = nil
    local lastRecordId = nil
    local count = 0
    for _, v in ipairs(records) do
        firstRecordId = firstRecordId or v.id
        lastRecordId = v.id
        count = count + 1
    end
    return firstRecordId, lastRecordId, count
end

return {
    {'Should support iteration over journal dialogues', function()
        local firstRecordId, lastRecordId, count = iterateOverRecords(core.dialogue.journal.records)
        testing.expectEqual(firstRecordId, '11111 test journal')
        testing.expectEqual(lastRecordId, 'va_vamprich')
        testing.expectEqual(count, 632)
    end},
    {'Should support iteration over topic dialogues', function()
        local firstRecordId, lastRecordId, count = iterateOverRecords(core.dialogue.topic.records)
        testing.expectEqual(firstRecordId, '1000-drake pledge')
        testing.expectEqual(lastRecordId, 'zenithar')
        testing.expectEqual(count, 1698)
    end},
    {'Should support iteration over greeting dialogues', function()
        local firstRecordId, lastRecordId, count = iterateOverRecords(core.dialogue.greeting.records)
        testing.expectEqual(firstRecordId, 'greeting 0')
        testing.expectEqual(lastRecordId, 'greeting 9')
        testing.expectEqual(count, 10)
    end},
    {'Should support iteration over persuasion dialogues', function()
        local firstRecordId, lastRecordId, count = iterateOverRecords(core.dialogue.persuasion.records)
        testing.expectEqual(firstRecordId, 'admire fail')
        testing.expectEqual(lastRecordId, 'taunt success')
        testing.expectEqual(count, 10)
    end},
    {'Should support iteration over voice dialogues', function()
        local firstRecordId, lastRecordId, count = iterateOverRecords(core.dialogue.voice.records)
        testing.expectEqual(firstRecordId, 'alarm')
        testing.expectEqual(lastRecordId, 'thief')
        testing.expectEqual(count, 8)
    end},
}
