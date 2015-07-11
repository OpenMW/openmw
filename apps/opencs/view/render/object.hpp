#ifndef OPENCS_VIEW_OBJECT_H
#define OPENCS_VIEW_OBJECT_H

#include <string>



#include <osg/ref_ptr>

class QModelIndex;


namespace osg
{
    class PositionAttitudeTransform;
    class Group;
}

namespace Resource
{
    class ResourceSystem;
}

namespace CSMWorld
{
    class Data;
    struct CellRef;
}

namespace CSVRender
{
    class Object
    {
            const CSMWorld::Data& mData;
            std::string mReferenceId;
            std::string mReferenceableId;
            osg::ref_ptr<osg::PositionAttitudeTransform> mBaseNode;
            osg::Group* mParentNode;
            Resource::ResourceSystem* mResourceSystem;
            bool mForceBaseToZero;

            /// Not implemented
            Object (const Object&);

            /// Not implemented
            Object& operator= (const Object&);

            /// Remove object from node (includes deleting)
            void clear();

            /// Update model
            /// @note Make sure adjustTransform() was called first so world space particles get positioned correctly
            void update();

            /// Adjust position, orientation and scale
            void adjustTransform();

            /// Throws an exception if *this was constructed with referenceable
            const CSMWorld::CellRef& getReference() const;

        public:

            Object (CSMWorld::Data& data, osg::Group *cellNode,
                const std::string& id, bool referenceable,
                bool forceBaseToZero = false);
            /// \param forceBaseToZero If this is a reference ignore the coordinates and place
            /// it at 0, 0, 0 instead.

            ~Object();

            /// \return Did this call result in a modification of the visual representation of
            /// this object?
            bool referenceableDataChanged (const QModelIndex& topLeft,
                const QModelIndex& bottomRight);

            /// \return Did this call result in a modification of the visual representation of
            /// this object?
            bool referenceableAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            /// \return Did this call result in a modification of the visual representation of
            /// this object?
            bool referenceDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            /// Returns an empty string if this is a refereceable-type object.
            std::string getReferenceId() const;

            std::string getReferenceableId() const;
    };
}

#endif
