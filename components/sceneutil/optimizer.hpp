/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/

/* Modified for OpenMW */

#ifndef OPENMW_OSGUTIL_OPTIMIZER
#define OPENMW_OSGUTIL_OPTIMIZER

#include <osg/NodeVisitor>
#include <osg/Matrix>
#include <osg/Geometry>
#include <osg/Transform>
#include <osg/Texture2D>

//#include <osgUtil/Export>

#include <set>

//namespace osgUtil {
namespace SceneUtil {

// forward declare
class Optimizer;

/** Helper base class for implementing Optimizer techniques.*/
class BaseOptimizerVisitor : public osg::NodeVisitor
{
    public:

        BaseOptimizerVisitor(Optimizer* optimizer, unsigned int operation):
            osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
            _optimizer(optimizer),
            _operationType(operation)
        {
            setNodeMaskOverride(0xffffffff);
        }

        inline bool isOperationPermissibleForObject(const osg::StateSet* object) const;
        inline bool isOperationPermissibleForObject(const osg::StateAttribute* object) const;
        inline bool isOperationPermissibleForObject(const osg::Drawable* object) const;
        inline bool isOperationPermissibleForObject(const osg::Node* object) const;

    protected:

        Optimizer*      _optimizer;
        unsigned int _operationType;
};

/** Traverses scene graph to improve efficiency. See OptimizationOptions.
  * For example of usage see examples/osgimpostor or osgviewer.
  */

class Optimizer
{

    public:

        Optimizer() {}
        virtual ~Optimizer() {}

        enum OptimizationOptions
        {
            FLATTEN_STATIC_TRANSFORMS = (1 << 0),
            REMOVE_REDUNDANT_NODES =    (1 << 1),
            REMOVE_LOADED_PROXY_NODES = (1 << 2),
            COMBINE_ADJACENT_LODS =     (1 << 3),
            SHARE_DUPLICATE_STATE =     (1 << 4),
            MERGE_GEOMETRY =            (1 << 5),
            CHECK_GEOMETRY =            (1 << 6), // deprecated, currently no-op
            MAKE_FAST_GEOMETRY =        (1 << 7),
            SPATIALIZE_GROUPS =         (1 << 8),
            COPY_SHARED_NODES =         (1 << 9),
            TRISTRIP_GEOMETRY =         (1 << 10),
            TESSELLATE_GEOMETRY =       (1 << 11),
            OPTIMIZE_TEXTURE_SETTINGS = (1 << 12),
            MERGE_GEODES =              (1 << 13),
            FLATTEN_BILLBOARDS =        (1 << 14),
            TEXTURE_ATLAS_BUILDER =     (1 << 15),
            STATIC_OBJECT_DETECTION =   (1 << 16),
            FLATTEN_STATIC_TRANSFORMS_DUPLICATING_SHARED_SUBGRAPHS = (1 << 17),
            INDEX_MESH =                (1 << 18),
            VERTEX_POSTTRANSFORM =      (1 << 19),
            VERTEX_PRETRANSFORM =       (1 << 20),
            DEFAULT_OPTIMIZATIONS = FLATTEN_STATIC_TRANSFORMS |
                                REMOVE_REDUNDANT_NODES |
                                REMOVE_LOADED_PROXY_NODES |
                                COMBINE_ADJACENT_LODS |
                                SHARE_DUPLICATE_STATE |
                                MERGE_GEOMETRY |
                                MAKE_FAST_GEOMETRY |
                                CHECK_GEOMETRY |
                                OPTIMIZE_TEXTURE_SETTINGS |
                                STATIC_OBJECT_DETECTION,
            ALL_OPTIMIZATIONS = FLATTEN_STATIC_TRANSFORMS_DUPLICATING_SHARED_SUBGRAPHS |
                                REMOVE_REDUNDANT_NODES |
                                REMOVE_LOADED_PROXY_NODES |
                                COMBINE_ADJACENT_LODS |
                                SHARE_DUPLICATE_STATE |
                                MERGE_GEODES |
                                MERGE_GEOMETRY |
                                MAKE_FAST_GEOMETRY |
                                CHECK_GEOMETRY |
                                SPATIALIZE_GROUPS |
                                COPY_SHARED_NODES |
                                TRISTRIP_GEOMETRY |
                                OPTIMIZE_TEXTURE_SETTINGS |
                                TEXTURE_ATLAS_BUILDER |
                                STATIC_OBJECT_DETECTION
        };

        /** Reset internal data to initial state - the getPermissibleOptionsMap is cleared.*/
        void reset();

        /** Traverse the node and its subgraph with a series of optimization
          * visitors, specified by the OptimizationOptions.*/
        virtual void optimize(osg::Node* node, unsigned int options);


        /** Callback for customizing what operations are permitted on objects in the scene graph.*/
        struct IsOperationPermissibleForObjectCallback : public osg::Referenced
        {
            virtual bool isOperationPermissibleForObjectImplementation(const Optimizer* optimizer, const osg::StateSet* stateset,unsigned int option) const
            {
                return optimizer->isOperationPermissibleForObjectImplementation(stateset,option);
            }

            virtual bool isOperationPermissibleForObjectImplementation(const Optimizer* optimizer, const osg::StateAttribute* attribute,unsigned int option) const
            {
                return optimizer->isOperationPermissibleForObjectImplementation(attribute,option);
            }

            virtual bool isOperationPermissibleForObjectImplementation(const Optimizer* optimizer, const osg::Drawable* drawable,unsigned int option) const
            {
                return optimizer->isOperationPermissibleForObjectImplementation(drawable,option);
            }

            virtual bool isOperationPermissibleForObjectImplementation(const Optimizer* optimizer, const osg::Node* node,unsigned int option) const
            {
                return optimizer->isOperationPermissibleForObjectImplementation(node,option);
            }

        };

        /** Set the callback for customizing what operations are permitted on objects in the scene graph.*/
        void setIsOperationPermissibleForObjectCallback(IsOperationPermissibleForObjectCallback* callback) { _isOperationPermissibleForObjectCallback=callback; }

        /** Get the callback for customizing what operations are permitted on objects in the scene graph.*/
        IsOperationPermissibleForObjectCallback* getIsOperationPermissibleForObjectCallback() { return _isOperationPermissibleForObjectCallback.get(); }

        /** Get the callback for customizing what operations are permitted on objects in the scene graph.*/
        const IsOperationPermissibleForObjectCallback* getIsOperationPermissibleForObjectCallback() const { return _isOperationPermissibleForObjectCallback.get(); }


        inline void setPermissibleOptimizationsForObject(const osg::Object* object, unsigned int options)
        {
            _permissibleOptimizationsMap[object] = options;
        }

        inline unsigned int getPermissibleOptimizationsForObject(const osg::Object* object) const
        {
            PermissibleOptimizationsMap::const_iterator itr = _permissibleOptimizationsMap.find(object);
            if (itr!=_permissibleOptimizationsMap.end()) return itr->second;
            else return 0xffffffff;
        }


        inline bool isOperationPermissibleForObject(const osg::StateSet* object, unsigned int option) const
        {
            if (_isOperationPermissibleForObjectCallback.valid())
                return _isOperationPermissibleForObjectCallback->isOperationPermissibleForObjectImplementation(this,object,option);
            else
                return isOperationPermissibleForObjectImplementation(object,option);
        }

        inline bool isOperationPermissibleForObject(const osg::StateAttribute* object, unsigned int option) const
        {
            if (_isOperationPermissibleForObjectCallback.valid())
                return _isOperationPermissibleForObjectCallback->isOperationPermissibleForObjectImplementation(this,object,option);
            else
                return isOperationPermissibleForObjectImplementation(object,option);
        }

        inline bool isOperationPermissibleForObject(const osg::Drawable* object, unsigned int option) const
        {
            if (_isOperationPermissibleForObjectCallback.valid())
                return _isOperationPermissibleForObjectCallback->isOperationPermissibleForObjectImplementation(this,object,option);
            else
                return isOperationPermissibleForObjectImplementation(object,option);
        }

        inline bool isOperationPermissibleForObject(const osg::Node* object, unsigned int option) const
        {
            if (_isOperationPermissibleForObjectCallback.valid())
                return _isOperationPermissibleForObjectCallback->isOperationPermissibleForObjectImplementation(this,object,option);
            else
                return isOperationPermissibleForObjectImplementation(object,option);
        }

        bool isOperationPermissibleForObjectImplementation(const osg::StateSet* stateset, unsigned int option) const
        {
            return (option & getPermissibleOptimizationsForObject(stateset))!=0;
        }

        bool isOperationPermissibleForObjectImplementation(const osg::StateAttribute* attribute, unsigned int option) const
        {
            return (option & getPermissibleOptimizationsForObject(attribute))!=0;
        }

        bool isOperationPermissibleForObjectImplementation(const osg::Drawable* drawable, unsigned int option) const
        {
            if (option & (REMOVE_REDUNDANT_NODES|MERGE_GEOMETRY))
            {
                if (drawable->getUserData()) return false;
                if (drawable->getUpdateCallback()) return false;
                if (drawable->getEventCallback()) return false;
                if (drawable->getCullCallback()) return false;
            }
            return (option & getPermissibleOptimizationsForObject(drawable))!=0;
        }

        bool isOperationPermissibleForObjectImplementation(const osg::Node* node, unsigned int option) const
        {
            if (option & (REMOVE_REDUNDANT_NODES|COMBINE_ADJACENT_LODS|FLATTEN_STATIC_TRANSFORMS))
            {
                if (node->getUserData()) return false;
                if (node->getUpdateCallback()) return false;
                if (node->getEventCallback()) return false;
                if (node->getCullCallback()) return false;
                if (node->getNumDescriptions()>0) return false;
                if (node->getStateSet()) return false;
                if (node->getNodeMask()!=0xffffffff) return false;
                // if (!node->getName().empty()) return false;
            }

            return (option & getPermissibleOptimizationsForObject(node))!=0;
        }

    protected:

        osg::ref_ptr<IsOperationPermissibleForObjectCallback> _isOperationPermissibleForObjectCallback;

        typedef std::map<const osg::Object*,unsigned int> PermissibleOptimizationsMap;
        PermissibleOptimizationsMap _permissibleOptimizationsMap;

    public:

        /** Flatten Static Transform nodes by applying their transform to the
          * geometry on the leaves of the scene graph, then removing the
          * now redundant transforms.  Static transformed subgraphs that have multiple
          * parental paths above them are not flattened, if you require this then
          * the subgraphs have to be duplicated - for this use the
          * FlattenStaticTransformsDuplicatingSharedSubgraphsVisitor. */
        class FlattenStaticTransformsVisitor : public BaseOptimizerVisitor
        {
            public:

                FlattenStaticTransformsVisitor(Optimizer* optimizer=0):
                    BaseOptimizerVisitor(optimizer, FLATTEN_STATIC_TRANSFORMS) {}

                virtual void apply(osg::Node& geode);
                virtual void apply(osg::Drawable& drawable);
                virtual void apply(osg::Billboard& geode);
                virtual void apply(osg::Transform& transform);

                bool removeTransforms(osg::Node* nodeWeCannotRemove);

            protected:

                typedef std::vector<osg::Transform*>                TransformStack;
                typedef std::set<osg::Drawable*>                    DrawableSet;
                typedef std::set<osg::Billboard*>                   BillboardSet;
                typedef std::set<osg::Node* >                       NodeSet;
                typedef std::set<osg::Transform*>                   TransformSet;

                TransformStack  _transformStack;
                NodeSet         _excludedNodeSet;
                DrawableSet     _drawableSet;
                BillboardSet    _billboardSet;
                TransformSet    _transformSet;
        };


        /** Combine Static Transform nodes that sit above one another.*/
        class CombineStaticTransformsVisitor : public BaseOptimizerVisitor
        {
            public:

                CombineStaticTransformsVisitor(Optimizer* optimizer=0):
                    BaseOptimizerVisitor(optimizer, FLATTEN_STATIC_TRANSFORMS) {}

                virtual void apply(osg::MatrixTransform& transform);

                bool removeTransforms(osg::Node* nodeWeCannotRemove);

            protected:

                typedef std::set<osg::MatrixTransform*> TransformSet;
                TransformSet  _transformSet;
        };

        /** Remove rendundant nodes, such as groups with one single child.*/
        class RemoveEmptyNodesVisitor : public BaseOptimizerVisitor
        {
            public:


                typedef std::set<osg::Node*> NodeList;
                NodeList                     _redundantNodeList;

                RemoveEmptyNodesVisitor(Optimizer* optimizer=0):
                    BaseOptimizerVisitor(optimizer, REMOVE_REDUNDANT_NODES) {}

                virtual void apply(osg::Group& group);

                void removeEmptyNodes();

        };

        /** Remove redundant nodes, such as groups with one single child.*/
        class RemoveRedundantNodesVisitor : public BaseOptimizerVisitor
        {
            public:

                typedef std::set<osg::Node*> NodeList;
                NodeList                     _redundantNodeList;

                RemoveRedundantNodesVisitor(Optimizer* optimizer=0):
                    BaseOptimizerVisitor(optimizer, REMOVE_REDUNDANT_NODES) {}

                virtual void apply(osg::Group& group);
                virtual void apply(osg::Transform& transform);
                virtual void apply(osg::LOD& lod);
                virtual void apply(osg::Switch& switchNode);

                bool isOperationPermissible(osg::Node& node);

                void removeRedundantNodes();

        };

        /** Merge adjacent Groups that have the same StateSet. */
        class MergeGroupsVisitor : public SceneUtil::BaseOptimizerVisitor
        {
        public:
            MergeGroupsVisitor(SceneUtil::Optimizer* optimizer)
                : BaseOptimizerVisitor(optimizer, REMOVE_REDUNDANT_NODES)
            {
            }

            bool isOperationPermissible(osg::Group& node);

            virtual void apply(osg::Group& group);
            virtual void apply(osg::LOD& lod);
            virtual void apply(osg::Switch& switchNode);
        };

        class MergeGeometryVisitor : public BaseOptimizerVisitor
        {
            public:

                /// default to traversing all children.
                MergeGeometryVisitor(Optimizer* optimizer=0) :
                    BaseOptimizerVisitor(optimizer, MERGE_GEOMETRY),
                    _targetMaximumNumberOfVertices(10000), _allowedToMerge(true) {}

                void setTargetMaximumNumberOfVertices(unsigned int num)
                {
                    _targetMaximumNumberOfVertices = num;
                }

                unsigned int getTargetMaximumNumberOfVertices() const
                {
                    return _targetMaximumNumberOfVertices;
                }

                void pushStateSet(osg::StateSet* stateSet);
                void popStateSet();
                void checkAllowedToMerge();

                virtual void apply(osg::Group& group);
                virtual void apply(osg::Billboard&) { /* don't do anything*/ }

                bool mergeGroup(osg::Group& group);

                static bool geometryContainsSharedArrays(osg::Geometry& geom);

                static bool mergeGeometry(osg::Geometry& lhs,osg::Geometry& rhs);

                static bool mergePrimitive(osg::DrawArrays& lhs,osg::DrawArrays& rhs);
                static bool mergePrimitive(osg::DrawArrayLengths& lhs,osg::DrawArrayLengths& rhs);
                static bool mergePrimitive(osg::DrawElementsUByte& lhs,osg::DrawElementsUByte& rhs);
                static bool mergePrimitive(osg::DrawElementsUShort& lhs,osg::DrawElementsUShort& rhs);
                static bool mergePrimitive(osg::DrawElementsUInt& lhs,osg::DrawElementsUInt& rhs);

            protected:

                unsigned int _targetMaximumNumberOfVertices;
                std::vector<osg::StateSet*> _stateSetStack;
                bool _allowedToMerge;
        };

};

inline bool BaseOptimizerVisitor::isOperationPermissibleForObject(const osg::StateSet* object) const
{
    return _optimizer ? _optimizer->isOperationPermissibleForObject(object,_operationType) :  true;
}

inline bool BaseOptimizerVisitor::isOperationPermissibleForObject(const osg::StateAttribute* object) const
{
    return _optimizer ? _optimizer->isOperationPermissibleForObject(object,_operationType) :  true;
}

inline bool BaseOptimizerVisitor::isOperationPermissibleForObject(const osg::Drawable* object) const
{
    return _optimizer ? _optimizer->isOperationPermissibleForObject(object,_operationType) :  true;
}

inline bool BaseOptimizerVisitor::isOperationPermissibleForObject(const osg::Node* object) const
{
    return _optimizer ? _optimizer->isOperationPermissibleForObject(object,_operationType) :  true;
}

}

#endif
