# AI-Driven Dialogue Engine Feasibility Analysis

## Executive Summary

**Feasibility:** Medium to High - The architecture is well-suited for AI integration
**Difficulty:** Medium-Hard (6-7/10)
**Estimated Effort:** 3-6 weeks for a proof of concept, 3-6 months for production-ready

The existing dialogue system has clean separation between data, logic, and presentation, making it a good candidate for AI enhancement. The journal system naturally maps to a context-building system for AI prompts.

---

## Current Architecture Overview

### Dialogue System Components

1. **DialogueManager** (`apps/openmw/mwdialogue/dialoguemanagerimp.cpp`)
   - Manages conversation state
   - Tracks known topics and actor relationships
   - Handles disposition and persuasion
   - Executes result scripts

2. **Filter** (`apps/openmw/mwdialogue/filter.cpp`)
   - **This is the key component to replace/augment**
   - Evaluates 100+ condition types (player stats, quest states, items, etc.)
   - Selects appropriate dialogue response from pre-written options
   - Multi-stage filtering: Actor → Player → Conditions → Disposition

3. **Journal System** (`apps/openmw/mwdialogue/journalimp.cpp`)
   - Tracks quest progress via indexed entries
   - Stores conversation history
   - Used by dialogue conditions to gate responses

4. **ESM Data Structures**
   - `ESM::DialInfo`: Pre-written responses with conditions and result scripts
   - `ESM::DialogueCondition`: 100+ condition functions for response filtering
   - `ESM::JournalEntry`: Quest state tracking

### Current Dialogue Flow

```
1. Player initiates dialogue with NPC
2. Filter.search() finds matching greeting based on:
   - NPC identity (ID, race, class, faction)
   - Player state (level, stats, items, location)
   - Quest state (journal indices)
   - Disposition threshold
3. Pre-written response text is displayed
4. Result script executes (may update journal, variables, etc.)
5. Topics are extracted from response text via keyword search
6. Player selects topic
7. Process repeats from step 2
```

---

## Proposed AI-Driven Architecture

### Core Concept

Replace the deterministic **Filter** system with an AI that:
- Receives context about NPC, player, and world state
- Uses journal entries as incremental context/lore
- Generates contextually appropriate responses
- Triggers appropriate game state changes

### Architecture Changes

```
┌─────────────────────────────────────────────────────────────┐
│                      GUI Layer (unchanged)                   │
│  DialogueWindow → ResponseCallback → Display                │
└─────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────┐
│              DialogueManager (modified)                      │
│  • Builds AI context instead of calling Filter              │
│  • Manages conversation history                             │
│  • Parses AI response for state changes                     │
└─────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────┐
│         NEW: AI Context Builder                              │
│  Aggregates:                                                 │
│  • NPC characteristics (race, class, faction, disposition)  │
│  • Unlocked journal entries (as lore/context)               │
│  • Current quest states                                      │
│  • Player stats, skills, status effects                     │
│  • Recent conversation history                              │
│  • Location/weather/time context                            │
└─────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────┐
│         NEW: AI Dialogue Generator                           │
│  • Calls LLM API with structured prompt                     │
│  • Returns response + structured commands                    │
│  • Commands: add_journal, set_quest_index, add_topic, etc. │
└─────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────┐
│         NEW: Command Interpreter                             │
│  • Parses AI output for game commands                       │
│  • Validates commands against game state                    │
│  • Executes safe subset of script operations                │
└─────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────┐
│              Journal System (enhanced)                       │
│  • Entries become AI context snippets                       │
│  • Quest states gate available context                      │
│  • New entries expand AI knowledge                          │
└─────────────────────────────────────────────────────────────┘
```

---

## Implementation Approach

### Phase 1: Context Building System (2-3 weeks)

**New Component:** `AIContextBuilder`

```cpp
// apps/openmw/mwdialogue/aicontextbuilder.hpp
class AIContextBuilder {
public:
    struct DialogueContext {
        // NPC Information
        std::string npcName;
        std::string npcRace;
        std::string npcClass;
        std::string npcFaction;
        int disposition;

        // Player State
        int playerLevel;
        std::map<std::string, int> playerSkills;
        std::vector<std::string> playerItems;

        // Journal Context (KEY FEATURE)
        std::vector<JournalEntryContext> unlockedEntries;
        std::map<std::string, int> questStates;

        // Conversation History
        std::vector<std::pair<std::string, std::string>> recentExchanges;

        // World Context
        std::string location;
        std::string weather;
        std::string timeOfDay;
    };

    DialogueContext buildContext(const MWWorld::Ptr& actor,
                                  const ESM::RefId& topic);

    std::string formatAsPrompt(const DialogueContext& context);
};
```

**Key Implementation:**
- Extract journal entries from `Journal::mJournal` and `Journal::mTopics`
- Each unlocked entry becomes part of the AI's knowledge base
- Quest indices determine what lore is available
- Example: "Quest 'MS_FargothRing' index 10" → AI knows player found the ring

### Phase 2: AI Integration Layer (2-3 weeks)

**New Component:** `AIDialogueGenerator`

```cpp
// apps/openmw/mwdialogue/aidialoguegenerator.hpp
class AIDialogueGenerator {
public:
    struct AIResponse {
        std::string responseText;
        std::vector<GameCommand> commands;
        std::vector<std::string> newTopics;
    };

    struct GameCommand {
        enum Type {
            ADD_JOURNAL,
            SET_QUEST_INDEX,
            ADD_TOPIC,
            MODIFY_DISPOSITION,
            GIVE_ITEM,
            START_COMBAT
        };
        Type type;
        std::map<std::string, std::string> parameters;
    };

    AIResponse generateResponse(const AIContextBuilder::DialogueContext& context,
                                 const std::string& playerInput);
private:
    // LLM API client
    // Prompt templates
    // Response parser
};
```

**Integration Points:**
1. Replace `Filter::search()` call in `DialogueManager::executeTopic()`
2. Instead of selecting from `ESM::DialInfo` records, call AI
3. Parse AI response for structured commands
4. Execute commands to update game state

**Prompt Structure Example:**
```
You are roleplaying as {npcName}, a {npcRace} {npcClass}.

CHARACTER TRAITS:
- Faction: {npcFaction}
- Disposition toward player: {disposition}/100
- Location: {location}

WORLD KNOWLEDGE (based on unlocked journal entries):
{foreach unlockedEntry}
- {entry.text}
{/foreach}

CURRENT QUEST STATES:
{foreach quest}
- {quest.name}: Stage {quest.index}
{/foreach}

PLAYER INFORMATION:
- Level: {playerLevel}
- Notable items: {playerItems}

CONVERSATION HISTORY:
{foreach exchange}
Player: {exchange.input}
You: {exchange.response}
{/foreach}

CURRENT TOPIC: {topic}

Respond in character. If your response should update the game state,
include commands in the following format:
[COMMAND:ADD_JOURNAL:quest_id:index:text]
[COMMAND:ADD_TOPIC:topic_id]
[COMMAND:MODIFY_DISPOSITION:amount]

Your response:
```

### Phase 3: Command Execution System (1-2 weeks)

**New Component:** `AICommandInterpreter`

```cpp
// apps/openmw/mwdialogue/aicommandinterpreter.hpp
class AICommandInterpreter {
public:
    void executeCommands(const std::vector<AIDialogueGenerator::GameCommand>& commands,
                        const MWWorld::Ptr& actor);
private:
    // Whitelist of safe commands
    // Validation logic
    // Execution delegates to existing systems
};
```

**Safety Considerations:**
- Whitelist approach: only allow specific game state changes
- Validate quest IDs and indices against existing data
- Prevent arbitrary script execution
- Log all AI-initiated state changes for debugging

### Phase 4: Fallback System (1 week)

**Hybrid Approach for Production:**
- Keep original Filter system as fallback
- Use AI for general conversation
- Use traditional system for critical quest dialogues
- Configuration flag to enable/disable AI per NPC or topic

```cpp
enum DialogueMode {
    TRADITIONAL,  // Use Filter system
    AI_ONLY,      // Pure AI generation
    AI_HYBRID     // AI with script-based constraints
};
```

---

## Journal as Additive Prompt System

### Current Journal System

The journal already functions as a knowledge progression system:
- Entries are unlocked through dialogue and quests
- Quest indices track progress through storylines
- Topics record what the player has learned

### Mapping to AI Context

**Direct Translation:**

| Journal Component | AI Prompt Component |
|------------------|---------------------|
| Quest entry text | Lore snippet / background info |
| Quest index | Story progression gating |
| Topic entries | Known conversation subjects |
| Quest finished state | Storyline completion marker |

**Example Progression:**

```
Initial state (no journal entries):
- AI knows only basic NPC traits and generic town info

After "A Forged Ring" quest starts:
- AI gains context: "You've been asked to find Fargoth's ring"
- Can reference the quest naturally in conversation

After finding the ring (index 10):
- AI gains context: "You found Fargoth's engraved ring in a barrel"
- Can discuss the discovery, hiding spot

After returning ring (index 20):
- AI gains context: "Fargoth was grateful and mentioned his secret stash"
- Can hint at player's knowledge

Quest finished:
- AI knows complete story arc
- Can reference it in future unrelated conversations
```

### Implementation

**Context Filtering by Quest State:**
```cpp
std::vector<std::string> AIContextBuilder::getRelevantJournalContext(
    const ESM::RefId& topic,
    const ESM::RefId& npcFaction)
{
    std::vector<std::string> context;

    // Get all unlocked journal entries
    for (const auto& entry : journal.mJournal) {
        // Include if:
        // 1. Related to current topic
        // 2. Related to NPC's faction
        // 3. Part of active quests
        // 4. Chronologically recent

        if (isRelevant(entry, topic, npcFaction)) {
            context.push_back(entry.mText);
        }
    }

    // Limit context size for API token limits
    return selectMostRelevant(context, MAX_CONTEXT_ENTRIES);
}
```

---

## Technical Challenges & Solutions

### Challenge 1: Quest Coherence

**Problem:** AI might generate responses that break quest logic

**Solutions:**
1. **Constraint-based Generation:** Provide AI with strict guidelines about quest progression
2. **Validation Layer:** Check AI commands against quest state machine
3. **Hybrid Mode:** Use traditional dialogues for critical quest steps, AI for flavor
4. **Quest Templates:** Provide AI with quest outlines it must follow

### Challenge 2: State Management

**Problem:** AI must correctly update game state (journal, quests, items)

**Solutions:**
1. **Structured Output:** Force AI to output JSON with commands
2. **Command Whitelist:** Only allow safe, validated state changes
3. **State Diffing:** Preview changes before applying
4. **Rollback System:** Undo AI-initiated changes if they break game state

### Challenge 3: Response Time

**Problem:** LLM API calls add latency to dialogue

**Solutions:**
1. **Async Processing:** Show "NPC is thinking" indicator
2. **Caching:** Cache common responses for generic topics
3. **Predictive Generation:** Pre-generate likely responses during player idle time
4. **Local LLM Option:** Integrate llama.cpp for offline play

### Challenge 4: Token Limits

**Problem:** Context (journal + NPC info + conversation) may exceed token limits

**Solutions:**
1. **Relevance Filtering:** Only include journal entries related to current topic/faction
2. **Summarization:** Summarize old journal entries, keep recent ones verbatim
3. **Vector DB:** Use embeddings to retrieve most relevant context
4. **Progressive Context:** Start with minimal context, expand if needed

### Challenge 5: Lore Consistency

**Problem:** AI might generate content that contradicts Elder Scrolls lore

**Solutions:**
1. **Lore Database:** Embed ES3 lore as system prompt
2. **Reference Checking:** Cross-reference AI output against lore database
3. **Human Review:** Flag questionable responses for review
4. **Fine-tuning:** Train model on Morrowind dialogue corpus

### Challenge 6: Cost

**Problem:** LLM API calls for every dialogue can be expensive

**Solutions:**
1. **Hybrid System:** Use AI selectively, traditional for routine dialogue
2. **Local LLM:** Run quantized models locally (Llama 3.1 8B)
3. **Caching:** Aggressive caching of similar contexts
4. **Batching:** Pre-generate dialogue trees for common scenarios

---

## Proof of Concept Roadmap

### Week 1-2: Foundation
- [ ] Create `AIContextBuilder` class
- [ ] Implement journal entry extraction
- [ ] Build prompt template system
- [ ] Basic NPC/player state serialization

### Week 3-4: AI Integration
- [ ] Implement `AIDialogueGenerator` with OpenAI API
- [ ] Create response parser for structured output
- [ ] Add conversation history tracking
- [ ] Implement topic extraction from AI responses

### Week 5-6: Game Integration
- [ ] Create `AICommandInterpreter`
- [ ] Integrate with `DialogueManager`
- [ ] Implement command execution (journal updates, etc.)
- [ ] Add fallback to traditional system

### Week 7-8: Polish
- [ ] Caching system
- [ ] Error handling and validation
- [ ] Performance optimization
- [ ] UI for AI vs traditional toggle
- [ ] Testing with various NPCs and quests

---

## Code Complexity Assessment

### Files to Modify

**Minimal Changes (facade pattern):**
- `apps/openmw/mwdialogue/dialoguemanagerimp.cpp` - Add AI path alongside Filter path
- `apps/openmw/mwbase/dialoguemanager.hpp` - Add AI configuration interface

**New Files (~2000-3000 lines total):**
- `apps/openmw/mwdialogue/aicontextbuilder.{hpp,cpp}` (~500 lines)
- `apps/openmw/mwdialogue/aidialoguegenerator.{hpp,cpp}` (~800 lines)
- `apps/openmw/mwdialogue/aicommandinterpreter.{hpp,cpp}` (~400 lines)
- `apps/openmw/mwdialogue/aiprompttemplate.{hpp,cpp}` (~300 lines)
- `components/ai/llmclient.{hpp,cpp}` (~600 lines)
- `components/ai/contextserializer.{hpp,cpp}` (~400 lines)

**Testing Files:**
- Unit tests for context building
- Integration tests for command execution
- Mock LLM for testing without API calls

### Estimated Lines of Code
- Core functionality: ~3,000 lines
- Tests: ~1,500 lines
- Configuration/UI: ~500 lines
- **Total: ~5,000 lines**

### Dependencies to Add
- HTTP client library (for LLM API) - likely libcurl (already in OpenMW)
- JSON library (already have: components/misc/json.hpp)
- Optional: llama.cpp for local inference
- Optional: Vector database for context retrieval (hnswlib, faiss)

---

## Difficulty Rating Breakdown

| Aspect | Difficulty (1-10) | Notes |
|--------|-------------------|-------|
| Context Building | 4 | Straightforward data extraction |
| Prompt Engineering | 6 | Requires iteration for quality |
| AI Integration | 5 | Standard API calls, but async handling needed |
| Command Parsing | 6 | Structured output parsing with error handling |
| State Validation | 7 | Must ensure game state consistency |
| Quest Coherence | 8 | Hardest part - maintaining storyline logic |
| Performance | 6 | Caching and optimization needed |
| Testing | 7 | Complex state space, many edge cases |
| **Overall** | **6.5/10** | **Medium-Hard** |

---

## Advantages of This Approach

1. **Natural Extension:** Journal system already structured for context building
2. **Incremental Context:** New entries naturally expand AI knowledge
3. **Clean Separation:** AI layer can be added without gutting existing code
4. **Fallback Option:** Can keep traditional system for critical paths
5. **Lore Consistency:** Journal entries are canonical lore, grounding AI responses
6. **Dynamic Dialogue:** NPCs can discuss any topic naturally, not just pre-scripted
7. **Replay Value:** Different conversations each playthrough
8. **Modding Potential:** Modders can add quests without writing all dialogue

---

## Disadvantages & Risks

1. **Quest Breaking:** AI might generate dialogue that breaks quest logic
2. **Lore Violations:** Without constraints, AI might contradict established lore
3. **Performance:** API latency could frustrate players
4. **Cost:** API calls add ongoing expense (mitigated by local LLM)
5. **Determinism Loss:** Can't reproduce exact dialogue, harder to debug
6. **Edge Cases:** AI might generate inappropriate or game-breaking responses
7. **Testing Complexity:** Harder to test all possible dialogue paths

---

## Recommended Implementation Strategy

### Option 1: Conservative Hybrid (Recommended for First Implementation)

- Keep traditional dialogue for main quests
- Use AI for:
  - Generic NPC chatter
  - Flavor dialogue
  - Procedural rumors/gossip
  - Optional side content
- Strict command whitelist
- Extensive logging and monitoring

### Option 2: AI-First with Guardrails

- Use AI for all dialogue
- Provide strict quest templates AI must follow
- Validation layer prevents quest-breaking responses
- Fall back to traditional for critical errors
- Requires more upfront work on constraints

### Option 3: Parallel System (Best for Development)

- Implement both systems side-by-side
- Configuration toggle to switch modes
- A/B test with players
- Gradual migration as AI improves
- Can revert if issues arise

---

## Minimal Viable Product (MVP) Scope

**Goal:** Prove the concept with one NPC and one quest

**Scope:**
1. Select one simple NPC (e.g., generic guard)
2. Select one simple quest (e.g., fetch quest)
3. Implement basic context building (no optimization)
4. Integrate with OpenAI API
5. Support only journal add/update commands
6. No caching, no optimization
7. Simple error handling (fall back to "I don't understand")

**Estimated Time:** 2-3 weeks
**Success Criteria:**
- NPC responds naturally to player questions
- Correctly updates journal at quest milestones
- Maintains character voice
- Doesn't break quest progression

---

## Long-term Vision

### Enhanced Features

1. **Dynamic Quest Generation:** AI creates side quests on the fly
2. **Consequence Modeling:** NPC remembers all interactions, not just journal entries
3. **Emergent Storylines:** NPCs react to player actions with novel dialogue
4. **Voice Synthesis:** Integrate TTS for fully voiced AI dialogue
5. **Personality Consistency:** Fine-tune models per NPC for unique voices
6. **Multi-NPC Conversations:** NPCs discuss events with each other
7. **World State Awareness:** NPCs comment on player reputation, major events

### Technical Evolution

1. **Local LLM Pipeline:** Full offline play with llama.cpp
2. **Vector Database:** Efficient context retrieval from massive lore database
3. **Fine-tuned Models:** Train on Morrowind dialogue corpus for voice matching
4. **Reinforcement Learning:** Train AI to maximize player engagement
5. **Automated Testing:** AI-driven QA for dialogue paths

---

## Conclusion

**The good news:** OpenMW's architecture is well-suited for this enhancement. The dialogue system has clean interfaces, the journal system maps naturally to AI context, and the codebase is modular enough to add AI without major refactoring.

**The challenge:** Maintaining quest coherence and lore consistency while giving the AI creative freedom. This is fundamentally a game design problem, not just a technical one.

**Recommended next step:** Build the MVP with a single NPC to prove the concept and identify integration challenges. Start with conservative constraints and gradually loosen them as the system proves reliable.

**Bottom line:** This is definitely doable, and could create a revolutionary RPG dialogue experience. The difficulty is managing the AI's creativity while preserving game structure.

---

## Questions for Further Design

1. Should the AI have access to *all* unlocked journal entries, or only quest-relevant ones?
2. How much creative freedom should the AI have in advancing quests?
3. Should there be a "personality file" per NPC, or derive personality from existing game data?
4. What's the acceptable latency for dialogue responses? (Target: <2 seconds)
5. Local LLM or cloud API for production? (Privacy vs. quality tradeoff)
6. Should the system support custom voice training per NPC?
7. How to handle dialogue in multiplayer (TES3MP)?

---

## References

**Key Source Files:**
- `/home/user/openmw-ai/apps/openmw/mwdialogue/dialoguemanagerimp.cpp` - Main dialogue orchestration
- `/home/user/openmw-ai/apps/openmw/mwdialogue/filter.cpp` - Response selection logic (to replace)
- `/home/user/openmw-ai/apps/openmw/mwdialogue/journalimp.cpp` - Journal management
- `/home/user/openmw-ai/components/esm3/loadinfo.hpp` - Dialogue data structures
- `/home/user/openmw-ai/components/esm3/dialoguecondition.hpp` - Condition system (100+ functions)

**Integration Points:**
- `DialogueManager::executeTopic()` - Where AI call would replace Filter::search()
- `Journal::addEntry()` - Where AI commands would update quest state
- `Filter::getSelectStructInteger()` (line 358-360) - Where journal state feeds into dialogue conditions
