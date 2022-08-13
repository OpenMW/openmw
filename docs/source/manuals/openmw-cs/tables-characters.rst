#################
Characters Tables
#################

These tables deal with records that make up the player and NPCs. Together we'll
refer to them as characters.


Skills
******

Characters in OpenMW can perform various actions and skills define how successful
a character is in performing these actions. The skills themselves are hardcoded
and cannot be added through the editor but can have some of their parameters
adjusted.

Attribute
    Each skill is governed by an attribute. The value of the attribute will
    contribute or hinder your success in using a skill.
    
Specialization
    A skill can belong to combat, magic, or stealth category. If the player
    specializes in the same category, they get a +5 starting bonus to the skill.
    
Use Value 1, 2, 3, 4
    Use Values are the amount of experience points the player will be awarded 
    for successful use of this skill. Skills can have one or multiple in-game 
    actions associated with them and each action is tied to one of the Use Values. 
    For example, Block skill has Use Value 1 = 2,5. This means that on each 
    successful blocked attack, the player's block skill will increase by 2,5 
    experience points. Athletics have Use Value 1 = 0,02, and Use Value 2 = 0,03. 
    For each second of running the player's athletics skill will be raised by 0,02 
    experience points, while for each second of swimming by 0,03 points. Note that not 
    all of the skills use all of the Use Values and OpenMW-CS currently doesn't tell 
    what action a skill's Use Value is associated with.
    
Description
    Flavour text that appears in the character information window.


Classes
*******

All characters in OpenMW need to be of a certain class. This table lists existing
classes and allows you to create, delete or modify them.

Name
    How this class is called.
    
Attribute 1 and 2
    Characters of this class receive a +10 starting bonus on the two chosen
    atributes.
    
Specialization
    Characters can specialize in combat, magic, or stealth. Each skill that
    belongs to the chosen specialization receives a +5 starting bonus and is
    easier to train.
    
Major Skill 1 to 5
    Training these skills contributes to the player leveling up. Each of these
    skills receives a +25 starting bonus and is easier to train.
    
Minor Skill 1 to 5
    Training these skills contributes to the player leveling up. Each of these
    skills receives a +10 starting bonus and is easier to train.
         
Playable
    When enabled, the player can choose to play as this class at character creation.
    If disabled, only NPCs can be of this class.
    
Description
    Flavour text that appears in the character information window.


Faction
*******

Characters in OpenMW can belong to different factions that represent interest
groups within the game's society. The most obvious example of factions are
guilds which the player can join, perform quests for and rise through its ranks.
Membership in factions can be a good source of interesting situations and quests.
If nothing else, membership affects NPC disposition towards the player.

Name
    How this faction is called.
    
Attribute 1 and 2
    Two attributes valued by the faction and required to rise through its ranks.
    
Hidden
    When enabled, the player's membership in this faction will not be displayed
    in the character window.
    
Skill 1 to 7
    Skills valued by the faction and required to rise through its ranks. Not all
    of the skill slots need to be filled.
    
Reactions
    Change of NPC disposition towards the player when joining this faction.
    Disposition change depends on which factions the NPCs belong to.
    
Ranks
    Positions in the hierarchy of this faction. Every rank has a requirement in
    attributes and skills before the player is promoted. Every rank gives the
    player an NPC disposition bonus within the faction.


Races
*****

All characters in OpenMW need to be of a certain race. This table list existing
races and allows you to create, delete or modify them. After a race is created,
it can be assigned to an NPC in the objects table. Race affects character's
starting attributes and skill bonuses, appearance, and voice lines. In addition,
a race assigned to the player can affect NPC disposition, quests, dialogues and
other things.

Name
    How this race is called.

Description
    Flavour text that appears in the character information window.

Playable
    When enabled, the player can choose to play as this race. If disabled, only
    the NPCs can belong to it.

Beast Race
    When enabled, characters of this race will use alternative animations to
    depict humanoids of different proportions and movement styles. This is done
    by using a different animation file compared to the main one and thus a new
    set of animations needs to be created for this to work properly.
    In addition, beast races can't wear closed helmets or boots  

Male Weight
    Scaling factor of the default body type. Values above 1.0 will make members
    of this race wider. Values below 1.0 will make them thinner. Applies to males. 

Male Height
    Scaling factor of the default body type. Values above 1.0 will make members
    of this race taller. Values below 1.0 will make them shorter. Applies to males.

Female Weight
    Scaling factor of the default body type. Values above 1.0 will make members
    of this race wider. Values below 1.0 will make them thinner. Applies to females. 

Female Height
    Scaling factor of the default body type. Values above 1.0 will make members
    of this race taller. Values below 1.0 will make them shorter. Applies to females.


Birthsigns
**********

Birthsigns permanently modify player's attributes, skills, or other abilities.
The player can have a single birthsign which is usually picked during character creation.
Modifications to the player are done through one or more spells added
to a birthsign. Spells with constant effects modify skills and attributes.
Spells that are cast are given to the player in the form of powers.

Name
    Name of the birthsign that will be displayed in the interface.
    
Texture
    An image that will be displayed in the birthsigns selection window.
    
Description
    Flavour text about the birthsign.
    
Powers
    A list of spells that are given to the player. When spells are added by a
    birthsign, they cannot be removed from the spell list in-game by the player.

    
Body Parts
**********

Characters are made from separate parts. Together they form the whole body.
Allows customization of the character appearance. Includes heads, arms, legs,
torso, hand, armor, pauldrons, chestpiece, helmets, wearables, etc.


Topics
******

Topics are, in a broader meaning, dialogue triggers. They can take the form of
clickable keywords in the dialogue, combat events, persuasion events, and other.
What response they produce and under what conditions is then defined by topic infos.
A single topic can be used by unlimited number of topic infos. There are four
different types of topics.

Greeting 0 to 9
    Initial text that appears in the dialogue window when talking to an NPC. Hardcoded.
    
Persuasion
    Persuasion entries produce a dialogue response when using persuasion actions on NPCs. Hardcoded.

    * Admire, Bribe, Intimidate Taunt Fail - Text in the dialogue window that the NPC says when the player fails a persuasion action.
    * Admire, Bribe, Intimidate Taunt Succeed - Text in the dialogue window that the NPC says when the player succeeds a persuasion action.
    * Info Refusal - Text in the dialogue window that the NPC says when the player wishes to talk about a certain topic and the conditions are not met. For example, NPC disposition is too low, the player is not a faction member, etc.
    * Service Refusal - Text in the dialogue window that the NPC says when the player wishes a service from the NPC but the conditions are not met.
    
Topic
    A keyword in the dialogue window that leads to further dialogue text, 
    similar to wiki links. These are the foundation to create dialogues from. 
    Entires can be freely added, edited, or removed.
    
Voice
    Voice entries are specific in-game events used to play a sound. Hardcoded.
    
    * Alarm - NPC enters combat state
    * Attack - NPC performs an attack
    * Flee - NPC loses their motivation to fight
    * Hello - NPC addressing the player when near enough
    * Hit - When NPCs are hit and injured.
    * Idle - Random things NPCs say.
    * Intruder
    * Thief - When an NPC detects the player steal something.
    

Topic Infos
***********

Topic infos take topics as their triggers and define responses. Through their
many parameters they can be assigned to one or more NPCs. Important to note is
that each topic info can take a combination of parameters to accurately define
which NPCs will produce a particular response. These parameters are as follow.

Actor
    A specific NPC.
    
Race
    All members of a race.
    
Class
    NPCs of a chosen class.
    
Faction
    NPCs belonging to a faction.
    
Cell
    NPCs located in a particular cell.
    
Disposition
    NPC disposition towards the player. This is the 0-100 bar visible in the
    conversation window and tells how much an NPC likes the player.
    
Rank
    NPC rank within a faction.
    
Gender
    NPC gender.
    
PC Faction
    Player's faction.
    
PC Rank
    Player's rank within a faction.

Topic infos when triggered provide a response when the correct conditions are met.

Sound File
    Sound file to play when the topic info is triggered
    
Response
    Dialogue text that appears in a dialogue window when clicking on a keyword.
    Dialogue text that appears near the bottom of the screen when a voice topic
    is triggered.

Script
    Script to define further effects or branching dialogue choices when this
    topic info is triggered.
    
Info Conditions.
    Conditions required for this topic info to be active.


Journals
********

Journals are records that define questlines. Entries can be added or removed.
When adding a new entry, you give it a unique ID which cannot be edited afterwards.
Also to remember is that journal IDs are not the actual keywords appearing in
the in-game journal.


Journal Infos
*************

Journal infos are stages of a particular quest. Entries appear in the player's
journal once they are called by a script. The script can be a standalone record
or a part of a topic info. The current command is ``Journal, "Journal ID", "Quest Index"``

Quest Status
    Finished, Name, None, Restart. No need to use them.
    
Quest Index
    A quest can write multiple entries into the player's journal and each of
    these entries is identified by its index. 
    
Quest Description
    Text that appears in the journal for this particular stage of the quest.
