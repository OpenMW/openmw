#ifndef GAME_RENDER_NPCANIMATION_H
#define GAME_RENDER_NPCANIMATION_H

#include "animation.hpp"

#include "../mwworld/inventorystore.hpp"

#include "weaponanimation.hpp"

namespace ESM
{
    struct NPC;
}

namespace MWRender
{

class HeadAnimationTime : public Ogre::ControllerValue<Ogre::Real>
{
private:
    MWWorld::Ptr mReference;
    float mTalkStart;
    float mTalkStop;
    float mBlinkStart;
    float mBlinkStop;

    float mBlinkTimer;

    bool mEnabled;

    float mValue;
private:
    void resetBlinkTimer();
public:
    HeadAnimationTime(MWWorld::Ptr reference);

    void update(float dt);

    void setEnabled(bool enabled);

    void setTalkStart(float value);
    void setTalkStop(float value);
    void setBlinkStart(float value);
    void setBlinkStop(float value);

    virtual Ogre::Real getValue() const;
    virtual void setValue(Ogre::Real value)
    { }
};

class NpcAnimation : public Animation, public WeaponAnimation, public MWWorld::InventoryStoreListener
{
public:
    virtual void equipmentChanged();
    virtual void permanentEffectAdded(const ESM::MagicEffect *magicEffect, bool isNew, bool playSound);

public:
    typedef std::map<ESM::PartReferenceType,std::string> PartBoneMap;

    enum ViewMode {
        VM_Normal,
        VM_FirstPerson,
        VM_HeadOnly
    };

private:
    static const PartBoneMap sPartList;

    bool mListenerDisabled;

    // Bounded Parts
    NifOgre::ObjectScenePtr mObjectParts[ESM::PRT_Count];
    std::string mSoundIds[ESM::PRT_Count];

    const ESM::NPC *mNpc;
    std::string    mHeadModel;
    std::string    mHairModel;
    ViewMode       mViewMode;
    bool mShowWeapons;
    bool mShowCarriedLeft;

    enum NpcType
    {
        Type_Normal,
        Type_Werewolf,
        Type_Vampire
    };
    NpcType mNpcType;

    int mVisibilityFlags;

    int mPartslots[ESM::PRT_Count];  //Each part slot is taken by clothing, armor, or is empty
    int mPartPriorities[ESM::PRT_Count];

    Ogre::Vector3 mFirstPersonOffset;

    Ogre::SharedPtr<HeadAnimationTime> mHeadAnimationTime;
    Ogre::SharedPtr<WeaponAnimationTime> mWeaponAnimationTime;

    float mAlpha;
    bool mSoundsDisabled;

    Ogre::Radian mHeadYaw;
    Ogre::Radian mHeadPitch;

    void updateNpcBase();

    NifOgre::ObjectScenePtr insertBoundedPart(const std::string &model, int group, const std::string &bonename,
                                              const std::string &bonefilter,
                                          bool enchantedGlow, Ogre::Vector3* glowColor=NULL);

    void removeIndividualPart(ESM::PartReferenceType type);
    void reserveIndividualPart(ESM::PartReferenceType type, int group, int priority);

    bool addOrReplaceIndividualPart(ESM::PartReferenceType type, int group, int priority, const std::string &mesh,
                                    bool enchantedGlow=false, Ogre::Vector3* glowColor=NULL);
    void removePartGroup(int group);
    void addPartGroup(int group, int priority, const std::vector<ESM::PartReference> &parts,
                                    bool enchantedGlow=false, Ogre::Vector3* glowColor=NULL);

    void applyAlpha(float alpha, Ogre::Entity* ent, NifOgre::ObjectScenePtr scene);

public:
    /**
     * @param ptr
     * @param node
     * @param visibilityFlags
     * @param disableListener  Don't listen for equipment changes and magic effects. InventoryStore only supports
     *                         one listener at a time, so you shouldn't do this if creating several NpcAnimations
     *                         for the same Ptr, eg preview dolls for the player.
     *                         Those need to be manually rendered anyway.
     * @param disableSounds    Same as \a disableListener but for playing items sounds
     * @param viewMode
     */
    NpcAnimation(const MWWorld::Ptr& ptr, Ogre::SceneNode* node, int visibilityFlags, bool disableListener = false,
                 bool disableSounds = false, ViewMode viewMode=VM_Normal);
    virtual ~NpcAnimation();

    virtual void enableHeadAnimation(bool enable);

    virtual void setWeaponGroup(const std::string& group) { mWeaponAnimationTime->setGroup(group); }

    virtual Ogre::Vector3 runAnimation(float timepassed);

    /// A relative factor (0-1) that decides if and how much the skeleton should be pitched
    /// to indicate the facing orientation of the character.
    virtual void setPitchFactor(float factor) { mPitchFactor = factor; }

    virtual void setHeadPitch(Ogre::Radian pitch);
    virtual void setHeadYaw(Ogre::Radian yaw);
    virtual Ogre::Radian getHeadPitch() const;
    virtual Ogre::Radian getHeadYaw() const;

    virtual void showWeapons(bool showWeapon);
    virtual void showCarriedLeft(bool show);

    virtual void attachArrow();
    virtual void releaseArrow();

    // WeaponAnimation
    virtual NifOgre::ObjectScenePtr getWeapon() { return mObjectParts[ESM::PRT_Weapon]; }
    virtual void showWeapon(bool show) { showWeapons(show); }
    virtual void configureAddedObject(NifOgre::ObjectScenePtr object, MWWorld::Ptr ptr, int slot);

    void setViewMode(ViewMode viewMode);

    void updateParts();

    /// \brief Applies a translation to the arms and hands.
    /// This may be called multiple times before the animation
    /// is updated to add additional offsets.
    void addFirstPersonOffset(const Ogre::Vector3 &offset);

    /// Rebuilds the NPC, updating their root model, animation sources, and equipment.
    void rebuild();

    /// Make the NPC only partially visible
    virtual void setAlpha(float alpha);

    virtual void setVampire(bool vampire);

    /// Prepare this animation for being rendered with \a camera (rotates billboard nodes)
    virtual void preRender (Ogre::Camera* camera);
};

}

#endif
