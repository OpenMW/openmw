#ifndef CHARACTER_CREATION_HPP
#define CHARACTER_CREATION_HPP

#include "window_manager.hpp"

#include "../mwmechanics/mechanicsmanager.hpp"
#include "../mwmechanics/stat.hpp"
#include "../mwworld/world.hpp"
#include <components/esm_store/store.hpp>

namespace MWGui
{
    class WindowManager;
    class WindowBase;
    
    class TextInputDialog;
    class InfoBoxDialog;
    class RaceDialog;
    class DialogueWindow;
    class ClassChoiceDialog;
    class GenerateClassResultDialog;
    class PickClassDialog;
    class CreateClassDialog;
    class BirthDialog;
    class ReviewDialog;
    class MessageBoxManager;

    class CharacterCreation
    {
    public:
    typedef std::vector<int> SkillList;
    
    CharacterCreation(WindowManager* _wm, MWWorld::Environment* _environment);
    ~CharacterCreation();
    
    //Show a dialog
    void spawnDialog(const char id);
    
    private:
    //Dialogs
    TextInputDialog* mNameDialog;
    RaceDialog* mRaceDialog;
    DialogueWindow* mDialogueWindow;
    ClassChoiceDialog* mClassChoiceDialog;
    InfoBoxDialog* mGenerateClassQuestionDialog;
    GenerateClassResultDialog* mGenerateClassResultDialog;
    PickClassDialog* mPickClassDialog;
    CreateClassDialog* mCreateClassDialog;
    BirthDialog* mBirthSignDialog;
    ReviewDialog* mReviewDialog;
    
    WindowManager* mWM;
    MWWorld::Environment* mEnvironment;
    
    //Player data
    std::string mPlayerName;
    std::string mPlayerRaceId;
    std::string mPlayerBirthSignId;
    ESM::Class mPlayerClass;
    std::map<ESM::Attribute::AttributeID, MWMechanics::Stat<int> > mPlayerAttributes;
    SkillList mPlayerMajorSkills, mPlayerMinorSkills;
    std::map<ESM::Skill::SkillEnum, MWMechanics::Stat<float> > mPlayerSkillValues;
    
    //Class generation vars
    unsigned mGenerateClassStep;                 // Keeps track of current step in Generate Class dialog
    unsigned mGenerateClassSpecializations[3];   // A counter for each specialization which is increased when an answer is chosen
    std::string mGenerateClass;                  // In order: Stealth, Combat, Magic
    
    ////Dialog events
    //Name dialog
    void onNameDialogDone(WindowBase* parWindow);
    
    //Race dialog
    void onRaceDialogDone(WindowBase* parWindow);
    void onRaceDialogBack();
    
    //Class dialogs
    void onClassChoice(int _index);
    void onPickClassDialogDone(WindowBase* parWindow);
    void onPickClassDialogBack();
    void onCreateClassDialogDone(WindowBase* parWindow);
    void onCreateClassDialogBack();
    void showClassQuestionDialog();
    void onClassQuestionChosen(int _index);
    void onGenerateClassBack();
    void onGenerateClassDone(WindowBase* parWindow);
    
    //Birthsign dialog
    void onBirthSignDialogDone(WindowBase* parWindow);
    void onBirthSignDialogBack();
    
    //Review dialog
    void onReviewDialogDone(WindowBase* parWindow);
    void onReviewDialogBack();
    void onReviewActivateDialog(int parDialog);
    
    enum CSE    //Creation Stage Enum
    {
        CSE_NotStarted,
        CSE_NameChosen,
        CSE_RaceChosen,
        CSE_ClassChosen,
        CSE_BirthSignChosen,
        CSE_ReviewNext
    };

    CSE mCreationStage; // Which state the character creating is in, controls back/next/ok buttons
    };
   
    struct Step
    {
        const char* mText;
        const char* mButtons[3];
        ESM::Class::Specialization mSpecializations[3]; // The specialization for each answer
    };

    static boost::array<Step, 10> sGenerateClassSteps = { {
        // Question 1
        {"On a clear day you chance upon a strange animal, its legs trapped in a hunter's clawsnare. Judging from the bleeding, it will not survive long.",
        {"Draw your dagger, mercifully endings its life with a single thrust.",
        "Use herbs from your pack to put it to sleep.",
        "Do not interfere in the natural evolution of events, but rather take the opportunity to learn more about a strange animal that you have never seen before."},
        {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 2
        {"One Summer afternoon your father gives you a choice of chores.",
        {"Work in the forge with him casting iron for a new plow.",
        "Gather herbs for your mother who is preparing dinner.",
        "Go catch fish at the stream using a net and line."},
        {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 3
        {"Your cousin has given you a very embarrassing nickname and, even worse, likes to call you it in front of your friends. You asked him to stop, but he finds it very amusing to watch you blush.",
        {"Beat up your cousin, then tell him that if he ever calls you that nickname again, you will bloody him worse than this time.",
        "Make up a story that makes your nickname a badge of honor instead of something humiliating.",
        "Make up an even more embarrassing nickname for him and use it constantly until he learns his lesson."},
        {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 4
        {"There is a lot of heated discussion at the local tavern over a grouped of people called 'Telepaths'. They have been hired by certain City-State kings. Rumor has it these Telepaths read a person's mind and tell their lord whether a follower is telling the truth or not.",
        {"This is a terrible practice. A person's thoughts are his own and no one, not even a king, has the right to make such an invasion into another human's mind.",
        "Loyal followers to the king have nothing to fear from a Telepath. It is important to have a method of finding assassins and spies before it is too late.",
        "In these times, it is a necessary evil. Although you do not necessarily like the idea, a Telepath could have certain advantages during a time of war or in finding someone innocent of a crime."},
        {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 5
        {"Your mother sends you to the market with a list of goods to buy. After you finish you find that by mistake a shopkeeper has given you too much money back in exchange for one of the items.",
        {"Return to the store and give the shopkeeper his hard-earned money, explaining to him the mistake?",
        "Decide to put the extra money to good use and purchase items that would help your family?",
        "Pocket the extra money, knowing that shopkeepers in general tend to overcharge customers anyway?"},
        {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 6
        {"While in the market place you witness a thief cut a purse from a noble. Even as he does so, the noble notices and calls for the city guards. In his haste to get away, the thief drops the purse near you. Surprisingly no one seems to notice the bag of coins at your feet.",
        {"Pick up the bag and signal to the guard, knowing that the only honorable thing to do is return the money to its rightful owner.",
        "Leave the bag there, knowing that it is better not to get involved.",
        "Pick up the bag and pocket it, knowing that the extra windfall will help your family in times of trouble."},
        {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 7
        {"Your father sends you on a task which you loathe, cleaning the stables. On the way there, pitchfork in hand, you run into your friend from the homestead near your own. He offers to do it for you, in return for a future favor of his choosing.",
        {"Decline his offer, knowing that your father expects you to do the work, and it is better not to be in debt.",
        "Ask him to help you, knowing that two people can do the job faster than one, and agree to help him with one task of his choosing in the future.",
        "Accept his offer, reasoning that as long as the stables are cleaned, it matters not who does the cleaning."},
        {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 8
        {"Your mother asks you to help fix the stove. While you are working, a very hot pipe slips its mooring and falls towards her.",
        {"Position yourself between the pipe and your mother.",
        "Grab the hot pipe and try to push it away.",
        "Push your mother out of the way."},
        {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 9
        {"While in town the baker gives you a sweetroll. Delighted, you take it into an alley to enjoy only to be intercepted by a gang of three other kids your age. The leader demands the sweetroll, or else he and his friends will beat you and take it.",
        {"Drop the sweetroll and step on it, then get ready for the fight.",
        "Give him the sweetroll now without argument, knowing that later this afternoon you will have all your friends with you and can come and take whatever he owes you.",
        "Act like you're going to give him the sweetroll, but at the last minute throw it in the air, hoping that they'll pay attention to it long enough for you to get a shot in on the leader."},
        {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 10
        {"Entering town you find that you are witness to a very well-dressed man running from a crowd. He screams to you for help. The crowd behind him seem very angry.",
        {"Rush to the town's aid immediately, despite your lack of knowledge of the circumstances.",
        "Stand aside and allow the man and the mob to pass, realizing it is probably best not to get involved.",
        "Rush to the man's aid immediately, despite your lack of knowledge of the circumstances."},
        {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        }
    } };
}

#endif
