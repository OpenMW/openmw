#ifndef OPENMW_COMPONENTS_MORPHGEOMETRY_H
#define OPENMW_COMPONENTS_MORPHGEOMETRY_H

#include <osg/Geometry>

namespace SceneUtil
{

    /// @brief Vertex morphing implementation.
    /// @note The internal Geometry used for rendering is double buffered, this allows updates to be done in a thread safe way while
    /// not compromising rendering performance. This is crucial when using osg's default threading model of DrawThreadPerContext.
    class MorphGeometry : public osg::Drawable
    {
    public:
        MorphGeometry();
        MorphGeometry(const MorphGeometry& copy, const osg::CopyOp& copyop);

        META_Object(SceneUtil, MorphGeometry)

        /// Initialize this geometry from the source geometry.
        /// @note The source geometry will not be modified.
        void setSourceGeometry(osg::ref_ptr<osg::Geometry> sourceGeom);

        // Currently empty as this is difficult to implement. Technically we would need to compile both internal geometries in separate frames but this method is only called once. Alternatively we could compile just the static parts of the model.
        void compileGLObjects(osg::RenderInfo& renderInfo) const override {}

        class MorphTarget
        {
        protected:
            osg::ref_ptr<osg::Vec3Array> mOffsets;
            float mWeight;
        public:
            MorphTarget(osg::Vec3Array* offsets, float w = 1.0) : mOffsets(offsets), mWeight(w) {}
            void setWeight(float weight) { mWeight = weight; }
            float getWeight() const { return mWeight; }
            osg::Vec3Array* getOffsets() { return mOffsets.get(); }
            const osg::Vec3Array* getOffsets() const { return mOffsets.get(); }
            void setOffsets(osg::Vec3Array* offsets) { mOffsets = offsets; }
        };

        typedef std::vector<MorphTarget> MorphTargetList;

        virtual void addMorphTarget( osg::Vec3Array* offsets, float weight = 1.0 );

        /** Set the MorphGeometry dirty.*/
        void dirty();

        /** Get the list of MorphTargets.*/
        const MorphTargetList& getMorphTargetList() const { return mMorphTargets; }

        /** Get the list of MorphTargets. Warning if you modify this array you will have to call dirty() */
        MorphTargetList& getMorphTargetList() { return mMorphTargets; }

        /** Return the \c MorphTarget at position \c i.*/
        inline const MorphTarget& getMorphTarget( unsigned int i ) const { return mMorphTargets[i]; }

        /** Return the \c MorphTarget at position \c i.*/
        inline MorphTarget& getMorphTarget( unsigned int i ) { return mMorphTargets[i]; }

        osg::ref_ptr<osg::Geometry> getSourceGeometry() const;

        void accept(osg::NodeVisitor &nv) override;
        bool supports(const osg::PrimitiveFunctor&) const override { return true; }
        void accept(osg::PrimitiveFunctor&) const override;

        osg::BoundingBox computeBoundingBox() const override;

    private:
        void cull(osg::NodeVisitor* nv);

        MorphTargetList mMorphTargets;

        osg::ref_ptr<osg::Geometry> mSourceGeometry;

        osg::ref_ptr<osg::Geometry> mGeometry[2];
        osg::Geometry* getGeometry(unsigned int frame) const;

        unsigned int mLastFrameNumber;
        bool mDirty; // Have any morph targets changed?

        mutable bool mMorphedBoundingBox;
    };

}

#endif
