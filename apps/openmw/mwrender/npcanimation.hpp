#ifndef GAME_RENDER_NPCANIMATION_H
#define GAME_RENDER_NPCANIMATION_H

#include "actoranimation.hpp"
#include "animation.hpp"
#include "weaponanimation.hpp"

#include <components/vfs/pathutil.hpp>

#include "../mwworld/inventorystore.hpp"

#include <array>

namespace ESM
{
    struct NPC;
    struct BodyPart;
}

namespace MWSound
{
    class Sound;
}

namespace MWRender
{

    class RotateController;
    class HeadAnimationTime;

    class NpcAnimation : public ActorAnimation, public WeaponAnimation, public MWWorld::InventoryStoreListener
    {
    public:
        void equipmentChanged() override;

    public:
        typedef std::map<ESM::PartReferenceType, std::string> PartBoneMap;

        enum ViewMode
        {
            VM_Normal,
            VM_FirstPerson,
            VM_HeadOnly
        };

    private:
        static const PartBoneMap sPartList;

        // Bounded Parts
        PartHolderPtr mObjectParts[ESM::PRT_Count];
        std::array<MWSound::Sound*, ESM::PRT_Count> mSounds;

        const ESM::NPC* mNpc;
        VFS::Path::Normalized mHeadModel;
        VFS::Path::Normalized mHairModel;
        ViewMode mViewMode;
        bool mShowWeapons;
        bool mShowCarriedLeft;

        enum NpcType
        {
            Type_Normal,
            Type_Werewolf,
            Type_Vampire
        };
        NpcType mNpcType;

        int mPartslots[ESM::PRT_Count]; // Each part slot is taken by clothing, armor, or is empty
        int mPartPriorities[ESM::PRT_Count];

        osg::Vec3f mFirstPersonOffset;
        // Field of view to use when rendering first person meshes
        float mFirstPersonFieldOfView;

        std::shared_ptr<HeadAnimationTime> mHeadAnimationTime;
        std::shared_ptr<WeaponAnimationTime> mWeaponAnimationTime;

        bool mSoundsDisabled;

        bool mAccurateAiming;
        float mAimingFactor;

        void updateNpcBase();

        NpcType getNpcType() const;

        PartHolderPtr insertBoundedPart(VFS::Path::NormalizedView model, std::string_view bonename,
            std::string_view bonefilter, bool enchantedGlow, osg::Vec4f* glowColor, bool isLight);

        void removeIndividualPart(ESM::PartReferenceType type);
        void reserveIndividualPart(ESM::PartReferenceType type, int group, int priority);

        bool addOrReplaceIndividualPart(ESM::PartReferenceType type, int group, int priority,
            VFS::Path::NormalizedView mesh, bool enchantedGlow = false, osg::Vec4f* glowColor = nullptr,
            bool isLight = false);
        void removePartGroup(int group);
        void addPartGroup(int group, int priority, const std::vector<ESM::PartReference>& parts,
            bool enchantedGlow = false, osg::Vec4f* glowColor = nullptr);

        void setRenderBin();

        osg::ref_ptr<RotateController> mFirstPersonNeckController;

        static bool isFemalePart(const ESM::BodyPart* bodypart);
        static NpcType getNpcType(const MWWorld::Ptr& ptr);

    protected:
        void addControllers() override;
        bool isArrowAttached() const override;
        std::string getSheathedShieldMesh(const MWWorld::ConstPtr& shield) const override;

    public:
        /**
         * @param ptr
         * @param disableListener  Don't listen for equipment changes and magic effects. InventoryStore only supports
         *                         one listener at a time, so you shouldn't do this if creating several NpcAnimations
         *                         for the same Ptr, eg preview dolls for the player.
         *                         Those need to be manually rendered anyway.
         * @param disableSounds    Same as \a disableListener but for playing items sounds
         * @param viewMode
         */
        NpcAnimation(const MWWorld::Ptr& ptr, osg::ref_ptr<osg::Group> parentNode,
            Resource::ResourceSystem* resourceSystem, bool disableSounds = false, ViewMode viewMode = VM_Normal,
            float firstPersonFieldOfView = 55.f);
        virtual ~NpcAnimation();

        void enableHeadAnimation(bool enable) override;

        /// 1: the first person meshes follow the camera's rotation completely
        /// 0: the first person meshes follow the camera with a reduced factor, so you can look down at your own hands
        void setAccurateAiming(bool enabled) override;

        void setWeaponGroup(const std::string& group, bool relativeDuration) override;

        osg::Vec3f runAnimation(float timepassed) override;

        /// A relative factor (0-1) that decides if and how much the skeleton should be pitched
        /// to indicate the facing orientation of the character.
        void setPitchFactor(float factor) override { mPitchFactor = factor; }

        bool getWeaponsShown() const override { return mShowWeapons; }
        void showWeapons(bool showWeapon) override;

        bool updateCarriedLeftVisible(const int weaptype) const override;
        bool getCarriedLeftShown() const override { return mShowCarriedLeft; }
        void showCarriedLeft(bool show) override;

        void attachArrow() override;
        void detachArrow() override;
        void releaseArrow(float attackStrength) override;

        osg::Group* getArrowBone() override;
        osg::Node* getWeaponNode() override;
        Resource::ResourceSystem* getResourceSystem() override;

        // WeaponAnimation
        void showWeapon(bool show) override { showWeapons(show); }

        void setViewMode(ViewMode viewMode);

        void updateParts();

        /// Rebuilds the NPC, updating their root model, animation sources, and equipment.
        void rebuild();

        /// Get the inventory slot that the given node path leads into, or -1 if not found.
        int getSlot(const osg::NodePath& path) const;

        void setVampire(bool vampire) override;

        /// Set a translation offset (in object root space) to apply to meshes when in first person mode.
        void setFirstPersonOffset(const osg::Vec3f& offset);

        void updatePtr(const MWWorld::Ptr& updated) override;

        /// Get a list of body parts that may be used by an NPC of given race and gender.
        /// @note This is a fixed size list, one list item for each ESM::PartReferenceType, may contain nullptr body
        /// parts.
        static const std::vector<const ESM::BodyPart*>& getBodyParts(
            const ESM::RefId& raceId, bool female, bool firstperson, bool werewolf);
    };

}

#endif
