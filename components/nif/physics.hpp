#ifndef OPENMW_COMPONENTS_NIF_PHYSICS_HPP
#define OPENMW_COMPONENTS_NIF_PHYSICS_HPP

#include "extra.hpp"
#include "niftypes.hpp"
#include "record.hpp"
#include "recordptr.hpp"

#include <osg/Plane>
#include <osg/Quat>
#include <osg/Vec3f>
#include <osg/Vec4f>

#include <cstdint>
#include <vector>

// This header contains certain record definitions
// specific to Bethesda implementation of Havok physics
namespace Nif
{

    class NIFStream;
    class Reader;

    /// Non-record data types

    struct bhkWorldObjCInfoProperty
    {
        uint32_t mData;
        uint32_t mSize;
        uint32_t mCapacityAndFlags;

        void read(NIFStream* nif);
    };

    enum class BroadPhaseType : uint8_t
    {
        BroadPhase_Invalid = 0,
        BroadPhase_Entity = 1,
        BroadPhase_Phantom = 2,
        BroadPhase_Border = 3
    };

    struct bhkWorldObjectCInfo
    {
        BroadPhaseType mPhaseType;
        bhkWorldObjCInfoProperty mProperty;

        void read(NIFStream* nif);
    };

    struct HavokMaterial
    {
        uint32_t mMaterial;

        void read(NIFStream* nif);
    };

    struct HavokFilter
    {
        uint8_t mLayer;
        uint8_t mFlags;
        uint16_t mGroup;

        void read(NIFStream* nif);
    };

    struct hkSubPartData
    {
        HavokMaterial mHavokMaterial;
        uint32_t mNumVertices;
        HavokFilter mHavokFilter;

        void read(NIFStream* nif);
    };

    enum class HkResponseType : uint8_t
    {
        Response_Invalid = 0,
        Response_SimpleContact = 1,
        Response_Reporting = 2,
        Response_None = 3
    };

    struct bhkEntityCInfo
    {
        HkResponseType mResponseType;
        uint16_t mProcessContactDelay;

        void read(NIFStream* nif);
    };

    struct hkpMoppCode
    {
        osg::Vec4f mOffset;
        uint8_t mBuildType;
        std::vector<uint8_t> mData;

        void read(NIFStream* nif);
    };

    struct TriangleData
    {
        std::array<uint16_t, 3> mTriangle;
        uint16_t mWeldingInfo;
        osg::Vec3f mNormal;

        void read(NIFStream* nif);
    };

    struct bhkMeshMaterial
    {
        HavokMaterial mHavokMaterial;
        HavokFilter mHavokFilter;

        void read(NIFStream* nif);
    };

    struct bhkQsTransform
    {
        osg::Vec4f mTranslation;
        osg::Quat mRotation;

        void read(NIFStream* nif);
    };

    struct bhkCMSBigTri
    {
        std::array<uint16_t, 3> mTriangle;
        uint32_t mMaterial;
        uint16_t mWeldingInfo;

        void read(NIFStream* nif);
    };

    struct bhkCMSChunk
    {
        osg::Vec4f mTranslation;
        uint32_t mMaterialIndex;
        uint16_t mReference;
        uint16_t mTransformIndex;
        std::vector<uint16_t> mVertices;
        std::vector<uint16_t> mIndices;
        std::vector<uint16_t> mStrips;
        std::vector<uint16_t> mWeldingInfos;

        void read(NIFStream* nif);
    };

    enum class HkMotionType : uint8_t
    {
        Motion_Invalid = 0,
        Motion_Dynamic = 1,
        Motion_SphereInertia = 2,
        Motion_SphereStabilized = 3,
        Motion_BoxInertia = 4,
        Motion_BoxStabilized = 5,
        Motion_Keyframed = 6,
        Motion_Fixed = 7,
        Motion_ThinBox = 8,
        Motion_Character = 9
    };

    enum class HkDeactivatorType : uint8_t
    {
        Deactivator_Invalid = 0,
        Deactivator_Never = 1,
        Deactivator_Spatial = 2
    };

    enum class HkSolverDeactivation : uint8_t
    {
        SolverDeactivation_Invalid = 0,
        SolverDeactivation_Off = 1,
        SolverDeactivation_Low = 2,
        SolverDeactivation_Medium = 3,
        SolverDeactivation_High = 4,
        SolverDeactivation_Max = 5
    };

    enum class HkQualityType : uint8_t
    {
        Quality_Invalid = 0,
        Quality_Fixed = 1,
        Quality_Keyframed = 2,
        Quality_Debris = 3,
        Quality_Moving = 4,
        Quality_Critical = 5,
        Quality_Bullet = 6,
        Quality_User = 7,
        Quality_Character = 8,
        Quality_KeyframedReport = 9
    };

    struct bhkRigidBodyCInfo
    {
        HavokFilter mHavokFilter;
        HkResponseType mResponseType;
        uint16_t mProcessContactDelay;
        osg::Vec4f mTranslation;
        osg::Quat mRotation;
        osg::Vec4f mLinearVelocity;
        osg::Vec4f mAngularVelocity;
        Matrix3 mInertiaTensor;
        osg::Vec4f mCenter;
        float mMass;
        float mLinearDamping;
        float mAngularDamping;
        float mTimeFactor{ 1.f };
        float mGravityFactor{ 1.f };
        float mFriction;
        float mRollingFrictionMult;
        float mRestitution;
        float mMaxLinearVelocity;
        float mMaxAngularVelocity;
        float mPenetrationDepth;
        HkMotionType mMotionType;
        HkDeactivatorType mDeactivatorType;
        bool mEnableDeactivation{ true };
        HkSolverDeactivation mSolverDeactivation;
        HkQualityType mQualityType;
        uint8_t mAutoRemoveLevel;
        uint8_t mResponseModifierFlags;
        uint8_t mNumContactPointShapeKeys;
        bool mForceCollidedOntoPPU;

        void read(NIFStream* nif);
    };

    enum class ConstraintPriority : uint32_t
    {
        Priority_Invalid = 0,
        Priority_PhysicsTime = 1,
        Priority_TimeOfImpact = 3
    };

    struct bhkConstraintCInfo
    {
        bhkEntityPtr mEntityA;
        bhkEntityPtr mEntityB;
        ConstraintPriority mPriority;

        void read(NIFStream* nif);
        void post(Reader& nif);
    };

    enum class HkMotorType : uint8_t
    {
        Motor_None = 0,
        Motor_Position = 1,
        Motor_Velocity = 2,
        Motor_SpringDamper = 3
    };

    struct bhkPositionConstraintMotor
    {
        float mMinForce, mMaxForce;
        float mTau;
        float mDamping;
        float mProportionalRecoveryVelocity;
        float mConstantRecoveryVelocity;
        bool mEnabled;

        void read(NIFStream* nif);
    };

    struct bhkVelocityConstraintMotor
    {
        float mMinForce, mMaxForce;
        float mTau;
        float mTargetVelocity;
        bool mUseVelocityTarget;
        bool mEnabled;

        void read(NIFStream* nif);
    };

    struct bhkSpringDamperConstraintMotor
    {
        float mMinForce, mMaxForce;
        float mSpringConstant;
        float mSpringDamping;
        bool mEnabled;

        void read(NIFStream* nif);
    };

    struct bhkConstraintMotorCInfo
    {
        HkMotorType mType;
        bhkPositionConstraintMotor mPositionMotor;
        bhkVelocityConstraintMotor mVelocityMotor;
        bhkSpringDamperConstraintMotor mSpringDamperMotor;

        void read(NIFStream* nif);
    };

    struct bhkRagdollConstraintCInfo
    {
        struct Data
        {
            osg::Vec4f mPivot;
            osg::Vec4f mPlane;
            osg::Vec4f mTwist;
            osg::Vec4f mMotor;
        };
        Data mDataA;
        Data mDataB;
        float mConeMaxAngle;
        float mPlaneMinAngle, mPlaneMaxAngle;
        float mTwistMinAngle, mTwistMaxAngle;
        float mMaxFriction;
        bhkConstraintMotorCInfo mMotor;

        void read(NIFStream* nif);
    };

    struct bhkHingeConstraintCInfo
    {
        struct HingeData
        {
            osg::Vec4f mPivot;
            osg::Vec4f mAxis;
            osg::Vec4f mPerpAxis1;
            osg::Vec4f mPerpAxis2;
        };

        HingeData mDataA;
        HingeData mDataB;

        void read(NIFStream* nif);
    };

    struct bhkLimitedHingeConstraintCInfo
    {
        struct HingeData
        {
            osg::Vec4f mPivot;
            osg::Vec4f mAxis;
            osg::Vec4f mPerpAxis1;
            osg::Vec4f mPerpAxis2;
        };

        HingeData mDataA;
        HingeData mDataB;
        float mMinAngle, mMaxAngle;
        float mMaxFriction;
        bhkConstraintMotorCInfo mMotor;

        void read(NIFStream* nif);
    };

    struct bhkBallAndSocketConstraintCInfo
    {
        osg::Vec4f mPivotA, mPivotB;

        void read(NIFStream* nif);
    };

    struct bhkStiffSpringConstraintCInfo
    {
        osg::Vec4f mPivotA, mPivotB;
        float mLength;

        void read(NIFStream* nif);
    };

    struct bhkPrismaticConstraintCInfo
    {
        struct Data
        {
            osg::Vec4f mSliding;
            osg::Vec4f mRotation;
            osg::Vec4f mPlane;
            osg::Vec4f mPivot;
        };

        Data mDataA;
        Data mDataB;
        float mMinDistance, mMaxDistance;
        float mFriction;
        bhkConstraintMotorCInfo mMotor;

        void read(NIFStream* nif);
    };

    enum class HkConstraintType : uint32_t
    {
        BallAndSocket = 0,
        Hinge = 1,
        LimitedHinge = 2,
        Prismatic = 6,
        Ragdoll = 7,
        StiffSpring = 8,
        Malleable = 13,
    };

    struct bhkWrappedConstraintDataBase
    {
        HkConstraintType mType;
        bhkConstraintCInfo mInfo;
        bhkBallAndSocketConstraintCInfo mBallAndSocketInfo;
        bhkHingeConstraintCInfo mHingeInfo;
        bhkLimitedHingeConstraintCInfo mLimitedHingeInfo;
        bhkPrismaticConstraintCInfo mPrismaticInfo;
        bhkRagdollConstraintCInfo mRagdollInfo;
        bhkStiffSpringConstraintCInfo mStiffSpringInfo;
    };

    struct bhkMalleableConstraintCInfo : bhkWrappedConstraintDataBase
    {
        float mTau;
        float mDamping;
        float mStrength;

        void read(NIFStream* nif);
    };

    struct bhkWrappedConstraintData : bhkWrappedConstraintDataBase
    {
        bhkMalleableConstraintCInfo mMalleableInfo;

        void read(NIFStream* nif);
    };

    struct bhkConstraintChainCInfo
    {
        bhkRigidBodyList mEntities;
        bhkConstraintCInfo mInfo;

        void read(NIFStream* nif);
        void post(Reader& nif);
    };

    /// Record types

    // Abstract Bethesda Havok object
    struct bhkRefObject : public Record
    {
    };

    // Abstract serializable Bethesda Havok object
    struct bhkSerializable : public bhkRefObject
    {
    };

    // Abstract narrowphase collision detection object
    struct bhkShape : public bhkSerializable
    {
    };

    // Abstract bhkShape collection
    struct bhkShapeCollection : public bhkShape
    {
    };

    // Abstract physics system
    struct bhkSystem : public Record
    {
    };

    // Generic collision object
    struct NiCollisionObject : public Record
    {
        // The node that references this object
        NiAVObjectPtr mTarget;

        void read(NIFStream* nif) override { mTarget.read(nif); }
        void post(Reader& nif) override { mTarget.post(nif); }
    };

    // Bethesda Havok-specific collision object
    struct bhkCollisionObject : public NiCollisionObject
    {
        uint16_t mFlags;
        bhkWorldObjectPtr mBody;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override
        {
            NiCollisionObject::post(nif);
            mBody.post(nif);
        }
    };

    struct bhkNPCollisionObject : NiCollisionObject
    {
        uint16_t mFlags;
        bhkSystemPtr mData;
        uint32_t mBodyID;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct bhkBlendCollisionObject : bhkCollisionObject
    {
        float mHeirGain;
        float mVelGain;

        void read(NIFStream* nif) override;
    };

    struct bhkPhysicsSystem : public bhkSystem
    {
        std::vector<uint8_t> mData;

        void read(NIFStream* nif) override;
    };

    struct bhkRagdollSystem : public bhkSystem
    {
        std::vector<uint8_t> mData;

        void read(NIFStream* nif) override;
    };

    // Abstract Havok shape info record
    struct bhkWorldObject : public bhkSerializable
    {
        bhkShapePtr mShape;
        HavokFilter mHavokFilter;
        bhkWorldObjectCInfo mWorldObjectInfo;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    // Abstract
    struct bhkEntity : public bhkWorldObject
    {
        bhkEntityCInfo mInfo;

        void read(NIFStream* nif) override;
    };

    // Bethesda extension of hkpBvTreeShape
    // hkpBvTreeShape adds a bounding volume tree to an hkpShapeCollection
    struct bhkBvTreeShape : public bhkShape
    {
        bhkShapePtr mShape;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    // bhkBvTreeShape with Havok MOPP code
    struct bhkMoppBvTreeShape : public bhkBvTreeShape
    {
        float mScale;
        hkpMoppCode mMopp;

        void read(NIFStream* nif) override;
    };

    // Bethesda triangle strip-based Havok shape collection
    struct bhkNiTriStripsShape : public bhkShape
    {
        HavokMaterial mHavokMaterial;
        float mRadius;
        uint32_t mGrowBy;
        osg::Vec4f mScale{ 1.f, 1.f, 1.f, 0.f };
        NiTriStripsDataList mData;
        std::vector<HavokFilter> mHavokFilters;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    // Bethesda packed triangle strip-based Havok shape collection
    struct bhkPackedNiTriStripsShape : public bhkShapeCollection
    {
        std::vector<hkSubPartData> mSubshapes;
        uint32_t mUserData;
        float mRadius;
        osg::Vec4f mScale;
        hkPackedNiTriStripsDataPtr mData;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    // bhkPackedNiTriStripsShape data block
    struct hkPackedNiTriStripsData : public bhkShapeCollection
    {
        std::vector<TriangleData> mTriangles;
        std::vector<osg::Vec3f> mVertices;
        std::vector<hkSubPartData> mSubshapes;

        void read(NIFStream* nif) override;
    };

    // Abstract
    struct bhkSphereRepShape : public bhkShape
    {
        HavokMaterial mHavokMaterial;

        void read(NIFStream* nif) override;
    };

    // Abstract
    struct bhkConvexShape : public bhkSphereRepShape
    {
        float mRadius;

        void read(NIFStream* nif) override;
    };

    // A list of convex shapes sharing the same properties
    struct bhkConvexListShape : public bhkShape
    {
        bhkShapeList mSubShapes;
        HavokMaterial mMaterial;
        float mRadius;
        bhkWorldObjCInfoProperty mChildShapeProperty;
        bool mUseCachedAABB;
        float mClosestPointMinDistance;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct bhkConvexSweepShape : bhkShape
    {
        bhkConvexShapePtr mShape;
        HavokMaterial mMaterial;
        float mRadius;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    // A convex shape built from vertices
    struct bhkConvexVerticesShape : public bhkConvexShape
    {
        bhkWorldObjCInfoProperty mVerticesProperty;
        bhkWorldObjCInfoProperty mNormalsProperty;
        std::vector<osg::Vec4f> mVertices;
        std::vector<osg::Vec4f> mNormals;

        void read(NIFStream* nif) override;
    };

    struct bhkConvexTransformShape : public bhkShape
    {
        bhkShapePtr mShape;
        HavokMaterial mHavokMaterial;
        float mRadius;
        osg::Matrixf mTransform;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    // A box
    struct bhkBoxShape : public bhkConvexShape
    {
        osg::Vec3f mExtents;

        void read(NIFStream* nif) override;
    };

    // A capsule
    struct bhkCapsuleShape : public bhkConvexShape
    {
        osg::Vec3f mPoint1, mPoint2;
        float mRadius1, mRadius2;

        void read(NIFStream* nif) override;
    };

    // A cylinder
    struct bhkCylinderShape : public bhkConvexShape
    {
        osg::Vec4f mVertexA, mVertexB;
        float mCylinderRadius;

        void read(NIFStream* nif) override;
    };

    // Abstract shape that can collide with an array of spheres
    struct bhkHeightfieldShape : bhkShape
    {
        HavokMaterial mHavokMaterial;

        void read(NIFStream* nif) override;
    };

    // A plane bounded by an AABB
    struct bhkPlaneShape : bhkHeightfieldShape
    {
        osg::Plane mPlane;
        osg::Vec4f mExtents;
        osg::Vec4f mCenter;

        void read(NIFStream* nif) override;
    };

    /// A shape based on triangle strips
    struct bhkMeshShape : bhkShape
    {
        float mRadius;
        osg::Vec4f mScale;
        std::vector<bhkWorldObjCInfoProperty> mShapeProperties;
        NiTriStripsDataList mDataList;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    // A sphere
    using bhkSphereShape = bhkConvexShape;

    // Multiple spheres
    struct bhkMultiSphereShape : bhkSphereRepShape
    {
        bhkWorldObjCInfoProperty mShapeProperty;
        std::vector<osg::BoundingSpheref> mSpheres;

        void read(NIFStream* nif) override;
    };

    // A list of shapes
    struct bhkListShape : public bhkShapeCollection
    {
        bhkShapeList mSubshapes;
        HavokMaterial mHavokMaterial;
        bhkWorldObjCInfoProperty mChildShapeProperty;
        bhkWorldObjCInfoProperty mChildFilterProperty;
        std::vector<HavokFilter> mHavokFilters;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct bhkCompressedMeshShape : public bhkShape
    {
        NiAVObjectPtr mTarget;
        uint32_t mUserData;
        float mRadius;
        osg::Vec4f mScale;
        bhkCompressedMeshShapeDataPtr mData;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct bhkCompressedMeshShapeData : public bhkRefObject
    {
        uint32_t mBitsPerIndex, mBitsPerWIndex;
        uint32_t mMaskWIndex, mMaskIndex;
        float mError;
        osg::Vec4f mAabbMin, mAabbMax;
        uint8_t mWeldingType;
        uint8_t mMaterialType;
        std::vector<bhkMeshMaterial> mMaterials;
        std::vector<bhkQsTransform> mChunkTransforms;
        std::vector<osg::Vec4f> mBigVerts;
        std::vector<bhkCMSBigTri> mBigTris;
        std::vector<bhkCMSChunk> mChunks;

        void read(NIFStream* nif) override;
    };

    struct bhkRigidBody : public bhkEntity
    {
        bhkRigidBodyCInfo mInfo;
        bhkSerializableList mConstraints;
        uint32_t mBodyFlags;

        void read(NIFStream* nif) override;
    };

    // Abstract non-physical object that receives collision events
    struct bhkPhantom : bhkWorldObject
    {
    };

    // A Phantom with an AABB
    struct bhkAabbPhantom : bhkPhantom
    {
        osg::Vec4f mAabbMin, mAabbMax;

        void read(NIFStream* nif) override;
    };

    // Abstract Phantom with a collision shape
    struct bhkShapePhantom : bhkPhantom
    {
    };

    // A ShapePhantom with a transformation
    struct bhkSimpleShapePhantom : bhkShapePhantom
    {
        osg::Matrixf mTransform;

        void read(NIFStream* nif) override;
    };

    // Abstract constraint
    struct bhkConstraint : public bhkSerializable
    {
        bhkConstraintCInfo mInfo;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct bhkRagdollConstraint : public bhkConstraint
    {
        bhkRagdollConstraintCInfo mConstraint;

        void read(NIFStream* nif) override;
    };

    struct bhkHingeConstraint : public bhkConstraint
    {
        bhkHingeConstraintCInfo mConstraint;

        void read(NIFStream* nif) override;
    };

    struct bhkLimitedHingeConstraint : public bhkConstraint
    {
        bhkLimitedHingeConstraintCInfo mConstraint;

        void read(NIFStream* nif) override;
    };

    struct bhkBallAndSocketConstraint : bhkConstraint
    {
        bhkBallAndSocketConstraintCInfo mConstraint;

        void read(NIFStream* nif) override;
    };

    struct bhkBallSocketConstraintChain : bhkSerializable
    {
        std::vector<bhkBallAndSocketConstraintCInfo> mConstraints;
        float mTau;
        float mDamping;
        float mConstraintForceMixing;
        float mMaxErrorDistance;
        bhkConstraintChainCInfo mConstraintChainInfo;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct bhkStiffSpringConstraint : bhkConstraint
    {
        bhkStiffSpringConstraintCInfo mConstraint;

        void read(NIFStream* nif) override;
    };

    struct bhkPrismaticConstraint : bhkConstraint
    {
        bhkPrismaticConstraintCInfo mConstraint;

        void read(NIFStream* nif) override;
    };

    struct bhkMalleableConstraint : bhkConstraint
    {
        bhkMalleableConstraintCInfo mConstraint;

        void read(NIFStream* nif) override;
    };

    struct bhkBreakableConstraint : bhkConstraint
    {
        bhkWrappedConstraintData mConstraint;
        float mThreshold;
        bool mRemoveWhenBroken;

        void read(NIFStream* nif) override;
    };

    // Abstract action applied during the simulation
    struct bhkAction : bhkSerializable
    {
    };

    struct bhkUnaryAction : bhkAction
    {
        bhkRigidBodyPtr mEntity;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct bhkLiquidAction : bhkAction
    {
        float mInitialStickForce;
        float mStickStrength;
        float mNeighborDistance;
        float mNeighborStrength;

        void read(NIFStream* nif) override;
    };

    struct bhkOrientHingedBodyAction : bhkUnaryAction
    {
        osg::Vec4f mHingeAxisLS;
        osg::Vec4f mForwardLS;
        float mStrength;
        float mDamping;

        void read(NIFStream* nif) override;
    };

    struct bhkRagdollTemplate : Extra
    {
        bhkRagdollTemplateDataList mBones;

        void read(NIFStream* nif) override;
        void post(Reader& nif) override;
    };

    struct bhkRagdollTemplateData : Record
    {
        std::string mName;
        float mMass;
        float mRestitution;
        float mFriction;
        float mRadius;
        HavokMaterial mHavokMaterial;
        std::vector<bhkWrappedConstraintData> mConstraints;

        void read(NIFStream* nif) override;
    };

    struct bhkPoseArray : Record
    {
        struct BoneTransform
        {
            osg::Vec3f mTranslation;
            // FIXME: this and some other quaternions are meant to be read in direct order
            osg::Quat mRotation;
            osg::Vec3f mScale;

            void read(NIFStream* nif);
        };

        std::vector<std::string> mBones;
        std::vector<std::vector<BoneTransform>> mPoses;

        void read(NIFStream* nif) override;
    };

} // Namespace
#endif
