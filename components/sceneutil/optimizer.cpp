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

#include <stdlib.h>
#include <string.h>

#include "optimizer.hpp"

#include <osg/Transform>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include <osg/LOD>
#include <osg/Billboard>
#include <osg/Geometry>
#include <osg/Notify>
#include <osg/Timer>
#include <osg/io_utils>

#include <osgUtil/TransformAttributeFunctor>
#include <osgUtil/Statistics>
#include <osgUtil/MeshOptimizers>

#include <typeinfo>
#include <algorithm>
#include <numeric>

#include <iterator>

using namespace osgUtil;

namespace SceneUtil
{

void Optimizer::reset()
{
}

void Optimizer::optimize(osg::Node* node, unsigned int options)
{
    StatsVisitor stats;

    if (osg::getNotifyLevel()>=osg::INFO)
    {
        node->accept(stats);
        stats.totalUpStats();
        OSG_NOTICE<<std::endl<<"Stats before:"<<std::endl;
        stats.print(osg::notify(osg::NOTICE));
    }

    if (options & FLATTEN_STATIC_TRANSFORMS)
    {
        OSG_INFO<<"Optimizer::optimize() doing FLATTEN_STATIC_TRANSFORMS"<<std::endl;

        int i=0;
        bool result = false;
        do
        {
            OSG_DEBUG << "** RemoveStaticTransformsVisitor *** Pass "<<i<<std::endl;
            FlattenStaticTransformsVisitor fstv(this);
            node->accept(fstv);
            result = fstv.removeTransforms(node);
            ++i;
        } while (result);

        // now combine any adjacent static transforms.
        CombineStaticTransformsVisitor cstv(this);
        node->accept(cstv);
        cstv.removeTransforms(node);
    }

    if (options & REMOVE_REDUNDANT_NODES)
    {
        OSG_INFO<<"Optimizer::optimize() doing REMOVE_REDUNDANT_NODES"<<std::endl;

        RemoveEmptyNodesVisitor renv(this);
        node->accept(renv);
        renv.removeEmptyNodes();

        RemoveRedundantNodesVisitor rrnv(this);
        node->accept(rrnv);
        rrnv.removeRedundantNodes();

        MergeGroupsVisitor mgrp(this);
        node->accept(mgrp);
    }

    if (options & MERGE_GEOMETRY)
    {
        OSG_INFO<<"Optimizer::optimize() doing MERGE_GEOMETRY"<<std::endl;

        osg::Timer_t startTick = osg::Timer::instance()->tick();

        MergeGeometryVisitor mgv(this);
        mgv.setTargetMaximumNumberOfVertices(10000);
        node->accept(mgv);

        osg::Timer_t endTick = osg::Timer::instance()->tick();

        OSG_INFO<<"MERGE_GEOMETRY took "<<osg::Timer::instance()->delta_s(startTick,endTick)<<std::endl;
    }

    if (options & VERTEX_POSTTRANSFORM)
    {
        OSG_INFO<<"Optimizer::optimize() doing VERTEX_POSTTRANSFORM"<<std::endl;
        VertexCacheVisitor vcv;
        node->accept(vcv);
        vcv.optimizeVertices();
    }

    if (options & VERTEX_PRETRANSFORM)
    {
        OSG_INFO<<"Optimizer::optimize() doing VERTEX_PRETRANSFORM"<<std::endl;
        VertexAccessOrderVisitor vaov;
        node->accept(vaov);
        vaov.optimizeOrder();
    }

    if (osg::getNotifyLevel()>=osg::INFO)
    {
        stats.reset();
        node->accept(stats);
        stats.totalUpStats();
        OSG_NOTICE<<std::endl<<"Stats after:"<<std::endl;
        stats.print(osg::notify(osg::NOTICE));
    }
}




////////////////////////////////////////////////////////////////////////////
// Flatten static transforms
////////////////////////////////////////////////////////////////////////////

class CollectLowestTransformsVisitor : public BaseOptimizerVisitor
{
    public:


        CollectLowestTransformsVisitor(Optimizer* optimizer=0):
                    BaseOptimizerVisitor(optimizer,Optimizer::FLATTEN_STATIC_TRANSFORMS),
                    _transformFunctor(osg::Matrix())
        {
            setTraversalMode(osg::NodeVisitor::TRAVERSE_PARENTS);
        }

        virtual void apply(osg::Node& node)
        {
            if (node.getNumParents())
            {
                traverse(node);
            }
            else
            {
                // for all current objects mark a nullptr transform for them.
                registerWithCurrentObjects(0);
            }
        }

        virtual void apply(osg::LOD& lod)
        {
            _currentObjectList.push_back(&lod);

            traverse(lod);

            _currentObjectList.pop_back();
        }

        virtual void apply(osg::Transform& transform)
        {
            // for all current objects associated this transform with them.
            registerWithCurrentObjects(&transform);
        }

        virtual void apply(osg::Geode& geode)
        {
            traverse(geode);
        }

        virtual void apply(osg::Billboard& geode)
        {
            traverse(geode);
        }


        void collectDataFor(osg::Node* node)
        {
            _currentObjectList.push_back(node);

            node->accept(*this);

            _currentObjectList.pop_back();
        }

        void collectDataFor(osg::Billboard* billboard)
        {
            _currentObjectList.push_back(billboard);

            billboard->accept(*this);

            _currentObjectList.pop_back();
        }

        void collectDataFor(osg::Drawable* drawable)
        {
            _currentObjectList.push_back(drawable);

            const osg::Drawable::ParentList& parents = drawable->getParents();
            for(osg::Drawable::ParentList::const_iterator itr=parents.begin();
                itr!=parents.end();
                ++itr)
            {
                (*itr)->accept(*this);
            }

            _currentObjectList.pop_back();
        }

        void setUpMaps();
        void disableTransform(osg::Transform* transform);
        bool removeTransforms(osg::Node* nodeWeCannotRemove);

        inline bool isOperationPermissibleForObject(const osg::Object* object) const
        {
            const osg::Node* node = object->asNode();
            if (node)
            {
                const osg::Drawable* drawable = node->asDrawable();
                if (drawable)
                     return isOperationPermissibleForObject(drawable);
                else
                    return isOperationPermissibleForObject(node);
            }
            return true;
        }

        inline bool isOperationPermissibleForObject(const osg::Drawable* drawable) const
        {
            return BaseOptimizerVisitor::isOperationPermissibleForObject(drawable);
        }

        inline bool isOperationPermissibleForObject(const osg::Node* node) const
        {
            return BaseOptimizerVisitor::isOperationPermissibleForObject(node);
        }

    protected:

        struct TransformStruct
        {
            typedef std::set<osg::Object*> ObjectSet;

            TransformStruct():_canBeApplied(true) {}

            void add(osg::Object* obj)
            {
                _objectSet.insert(obj);
            }

            bool        _canBeApplied;
            ObjectSet   _objectSet;
        };

        struct ObjectStruct
        {
            typedef std::set<osg::Transform*> TransformSet;

            ObjectStruct():_canBeApplied(true),_moreThanOneMatrixRequired(false) {}

            void add(osg::Transform* transform, bool canOptimize)
            {
                if (transform)
                {
                    if (!canOptimize) _moreThanOneMatrixRequired=true;
                    else if (transform->getReferenceFrame()!=osg::Transform::RELATIVE_RF) _moreThanOneMatrixRequired=true;
                    else
                    {
                        if (_transformSet.empty()) transform->computeLocalToWorldMatrix(_firstMatrix,0);
                        else
                        {
                            osg::Matrix matrix;
                            transform->computeLocalToWorldMatrix(matrix,0);
                            if (_firstMatrix!=matrix) _moreThanOneMatrixRequired=true;
                        }
                    }
                }
                else
                {
                    if (!_transformSet.empty())
                    {
                        if (!_firstMatrix.isIdentity()) _moreThanOneMatrixRequired=true;
                    }

                }
                _transformSet.insert(transform);
            }

            bool            _canBeApplied;
            bool            _moreThanOneMatrixRequired;
            osg::Matrix     _firstMatrix;
            TransformSet    _transformSet;
        };


        void registerWithCurrentObjects(osg::Transform* transform)
        {
            for(ObjectList::iterator itr=_currentObjectList.begin();
                itr!=_currentObjectList.end();
                ++itr)
            {
                _objectMap[*itr].add(transform, transform && isOperationPermissibleForObject(transform));
            }
        }

        typedef std::map<osg::Transform*,TransformStruct>   TransformMap;
        typedef std::map<osg::Object*,ObjectStruct>         ObjectMap;
        typedef std::vector<osg::Object*>                   ObjectList;

        void disableObject(osg::Object* object)
        {
            disableObject(_objectMap.find(object));
        }

        void disableObject(ObjectMap::iterator itr);
        void doTransform(osg::Object* obj,osg::Matrix& matrix);

        osgUtil::TransformAttributeFunctor _transformFunctor;
        TransformMap    _transformMap;
        ObjectMap       _objectMap;
        ObjectList      _currentObjectList;

};


void CollectLowestTransformsVisitor::doTransform(osg::Object* obj,osg::Matrix& matrix)
{
    osg::Node* node = obj->asNode();
    if (!node)
        return;
    osg::Drawable* drawable = node->asDrawable();
    if (drawable)
    {
        osgUtil::TransformAttributeFunctor tf(matrix);
        drawable->accept(tf);
        drawable->dirtyBound();
        drawable->dirtyDisplayList();

        return;
    }

    osg::LOD* lod = dynamic_cast<osg::LOD*>(obj);
    if (lod)
    {
        osg::Matrix matrix_no_trans = matrix;
        matrix_no_trans.setTrans(0.0f,0.0f,0.0f);

        osg::Vec3 v111(1.0f,1.0f,1.0f);
        osg::Vec3 new_v111 = v111*matrix_no_trans;
        float ratio = new_v111.length()/v111.length();

        // move center point.
        lod->setCenter(lod->getCenter()*matrix);

        // adjust ranges to new scale.
        for(unsigned int i=0;i<lod->getNumRanges();++i)
        {
            lod->setRange(i,lod->getMinRange(i)*ratio,lod->getMaxRange(i)*ratio);
        }

        lod->dirtyBound();
        return;
    }

    osg::Billboard* billboard = dynamic_cast<osg::Billboard*>(obj);
    if (billboard)
    {
        osg::Matrix matrix_no_trans = matrix;
        matrix_no_trans.setTrans(0.0f,0.0f,0.0f);

        osgUtil::TransformAttributeFunctor tf(matrix_no_trans);

        osg::Vec3 axis = osg::Matrix::transform3x3(tf._im,billboard->getAxis());
        axis.normalize();
        billboard->setAxis(axis);

        osg::Vec3 normal = osg::Matrix::transform3x3(tf._im,billboard->getNormal());
        normal.normalize();
        billboard->setNormal(normal);


        for(unsigned int i=0;i<billboard->getNumDrawables();++i)
        {
            billboard->setPosition(i,billboard->getPosition(i)*matrix);
            billboard->getDrawable(i)->accept(tf);
            billboard->getDrawable(i)->dirtyBound();
        }

        billboard->dirtyBound();

        return;
    }
}

void CollectLowestTransformsVisitor::disableObject(ObjectMap::iterator itr)
{
    if (itr==_objectMap.end())
    {
        return;
    }

    if (itr->second._canBeApplied)
    {
        // we haven't been disabled yet so we need to disable,
        itr->second._canBeApplied = false;

        // and then inform everybody we have been disabled.
        for(ObjectStruct::TransformSet::iterator titr = itr->second._transformSet.begin();
            titr != itr->second._transformSet.end();
            ++titr)
        {
            disableTransform(*titr);
        }
    }
}

void CollectLowestTransformsVisitor::disableTransform(osg::Transform* transform)
{
    TransformMap::iterator itr=_transformMap.find(transform);
    if (itr==_transformMap.end())
    {
        return;
    }

    if (itr->second._canBeApplied)
    {

        // we haven't been disabled yet so we need to disable,
        itr->second._canBeApplied = false;
        // and then inform everybody we have been disabled.
        for(TransformStruct::ObjectSet::iterator oitr = itr->second._objectSet.begin();
            oitr != itr->second._objectSet.end();
            ++oitr)
        {
            disableObject(*oitr);
        }
    }
}

void CollectLowestTransformsVisitor::setUpMaps()
{
    // create the TransformMap from the ObjectMap
    ObjectMap::iterator oitr;
    for(oitr=_objectMap.begin();
        oitr!=_objectMap.end();
        ++oitr)
    {
        osg::Object* object = oitr->first;
        ObjectStruct& os = oitr->second;

        for(ObjectStruct::TransformSet::iterator titr = os._transformSet.begin();
            titr != os._transformSet.end();
            ++titr)
        {
            _transformMap[*titr].add(object);
        }
    }

    // disable all the objects which have more than one matrix associated
    // with them, and then disable all transforms which have an object associated
    // them that can't be applied, and then disable all objects which have
    // disabled transforms associated, recursing until all disabled
    // associativity.
    // and disable all objects that the operation is not permisable for)
    for(oitr=_objectMap.begin();
        oitr!=_objectMap.end();
        ++oitr)
    {
        osg::Object* object = oitr->first;
        ObjectStruct& os = oitr->second;
        if (os._canBeApplied)
        {
            if (os._moreThanOneMatrixRequired || !isOperationPermissibleForObject(object))
            {
                disableObject(oitr);
            }
        }
    }

}

bool CollectLowestTransformsVisitor::removeTransforms(osg::Node* nodeWeCannotRemove)
{
    // transform the objects that can be applied.
    for(ObjectMap::iterator oitr=_objectMap.begin();
        oitr!=_objectMap.end();
        ++oitr)
    {
        osg::Object* object = oitr->first;
        ObjectStruct& os = oitr->second;
        if (os._canBeApplied)
        {
            doTransform(object,os._firstMatrix);
        }
    }


    bool transformRemoved = false;

    // clean up the transforms.
    for(TransformMap::iterator titr=_transformMap.begin();
        titr!=_transformMap.end();
        ++titr)
    {
        if (titr->first!=0 && titr->second._canBeApplied)
        {
            if (titr->first!=nodeWeCannotRemove)
            {
                transformRemoved = true;

                osg::ref_ptr<osg::Transform> transform = titr->first;
                osg::ref_ptr<osg::Group>     group = new osg::Group;
                group->setName( transform->getName() );
                group->setDataVariance(osg::Object::STATIC);
                group->setNodeMask(transform->getNodeMask());
                group->setStateSet(transform->getStateSet());
                group->setUpdateCallback(transform->getUpdateCallback());
                group->setEventCallback(transform->getEventCallback());
                group->setCullCallback(transform->getCullCallback());
                group->setUserDataContainer(transform->getUserDataContainer());
                group->setDescriptions(transform->getDescriptions());
                for(unsigned int i=0;i<transform->getNumChildren();++i)
                {
                    group->addChild(transform->getChild(i));
                }

                for(int i2=transform->getNumParents()-1;i2>=0;--i2)
                {
                    transform->getParent(i2)->replaceChild(transform.get(),group.get());
                }
            }
            else
            {
                osg::MatrixTransform* mt = titr->first->asMatrixTransform();
                if (mt) mt->setMatrix(osg::Matrix::identity());
                else
                {
                    osg::PositionAttitudeTransform* pat = titr->first->asPositionAttitudeTransform();
                    if (pat)
                    {
                        pat->setPosition(osg::Vec3(0.0f,0.0f,0.0f));
                        pat->setAttitude(osg::Quat());
                        pat->setPivotPoint(osg::Vec3(0.0f,0.0f,0.0f));
                    }
                    else
                    {
                        OSG_WARN<<"Warning:: during Optimize::CollectLowestTransformsVisitor::removeTransforms(Node*)"<<std::endl;
                        OSG_WARN<<"          unhandled of setting of indentity matrix on "<< titr->first->className()<<std::endl;
                        OSG_WARN<<"          model will appear in the incorrect position."<<std::endl;
                    }
                }

            }
        }
    }
    _objectMap.clear();
    _transformMap.clear();

    return transformRemoved;
}

void Optimizer::FlattenStaticTransformsVisitor::apply(osg::Node& node)
{
    traverse(node);
}


void Optimizer::FlattenStaticTransformsVisitor::apply(osg::Drawable& drawable)
{
    osg::Geometry *geometry = drawable.asGeometry();
    if((geometry) && (isOperationPermissibleForObject(&drawable)))
    {
        if(geometry->getVertexArray() && geometry->getVertexArray()->referenceCount() > 1) {
            geometry->setVertexArray(dynamic_cast<osg::Array*>(geometry->getVertexArray()->clone(osg::CopyOp::DEEP_COPY_ALL)));
        }
        if(geometry->getNormalArray() && geometry->getNormalArray()->referenceCount() > 1) {
            geometry->setNormalArray(dynamic_cast<osg::Array*>(geometry->getNormalArray()->clone(osg::CopyOp::DEEP_COPY_ALL)));
        }
    }
    _drawableSet.insert(&drawable);
}

void Optimizer::FlattenStaticTransformsVisitor::apply(osg::Billboard& billboard)
{
    if (!_transformStack.empty())
    {
        _billboardSet.insert(&billboard);
    }
}

void Optimizer::FlattenStaticTransformsVisitor::apply(osg::Transform& transform)
{
    if (!_transformStack.empty())
    {
        // we need to disable any transform higher in the list.
        _transformSet.insert(_transformStack.back());
    }

    _transformStack.push_back(&transform);

    // simple traverse the children as if this Transform didn't exist.
    traverse(transform);

    _transformStack.pop_back();
}

bool Optimizer::FlattenStaticTransformsVisitor::removeTransforms(osg::Node* nodeWeCannotRemove)
{
    CollectLowestTransformsVisitor cltv(_optimizer);

    for(NodeSet::iterator nitr=_excludedNodeSet.begin();
        nitr!=_excludedNodeSet.end();
        ++nitr)
    {
        cltv.collectDataFor(*nitr);
    }

    for(DrawableSet::iterator ditr=_drawableSet.begin();
        ditr!=_drawableSet.end();
        ++ditr)
    {
        cltv.collectDataFor(*ditr);
    }

    for(BillboardSet::iterator bitr=_billboardSet.begin();
        bitr!=_billboardSet.end();
        ++bitr)
    {
        cltv.collectDataFor(*bitr);
    }

    cltv.setUpMaps();

    for(TransformSet::iterator titr=_transformSet.begin();
        titr!=_transformSet.end();
        ++titr)
    {
        cltv.disableTransform(*titr);
    }


    return cltv.removeTransforms(nodeWeCannotRemove);
}

////////////////////////////////////////////////////////////////////////////
// CombineStaticTransforms
////////////////////////////////////////////////////////////////////////////

void Optimizer::CombineStaticTransformsVisitor::apply(osg::MatrixTransform& transform)
{
    if (transform.getDataVariance()==osg::Object::STATIC &&
        transform.getNumChildren()==1 &&
        transform.getChild(0)->asTransform()!=0 &&
        transform.getChild(0)->asTransform()->asMatrixTransform()!=0 &&
        transform.getChild(0)->asTransform()->getDataVariance()==osg::Object::STATIC &&
        isOperationPermissibleForObject(&transform) && isOperationPermissibleForObject(transform.getChild(0)))
    {
        _transformSet.insert(&transform);
    }

    traverse(transform);
}

bool Optimizer::CombineStaticTransformsVisitor::removeTransforms(osg::Node* nodeWeCannotRemove)
{
    if (nodeWeCannotRemove && nodeWeCannotRemove->asTransform()!=0 && nodeWeCannotRemove->asTransform()->asMatrixTransform()!=0)
    {
        // remove topmost node from transform set if it exists there.
        TransformSet::iterator itr = _transformSet.find(nodeWeCannotRemove->asTransform()->asMatrixTransform());
        if (itr!=_transformSet.end()) _transformSet.erase(itr);
    }

    bool transformRemoved = false;

    while (!_transformSet.empty())
    {
        // get the first available transform to combine.
        osg::ref_ptr<osg::MatrixTransform> transform = *_transformSet.begin();
        _transformSet.erase(_transformSet.begin());

        if (transform->getNumChildren()==1 &&
            transform->getChild(0)->asTransform()!=0 &&
            transform->getChild(0)->asTransform()->asMatrixTransform()!=0 &&
            transform->getChild(0)->asTransform()->getDataVariance()==osg::Object::STATIC)
        {
            // now combine with its child.
            osg::MatrixTransform* child = transform->getChild(0)->asTransform()->asMatrixTransform();

            osg::Matrix newMatrix = child->getMatrix()*transform->getMatrix();
            child->setMatrix(newMatrix);
            if (transform->getStateSet())
            {
                if(child->getStateSet()) child->getStateSet()->merge(*transform->getStateSet());
                else child->setStateSet(transform->getStateSet());
            }

            transformRemoved = true;

            osg::Node::ParentList parents = transform->getParents();
            for(osg::Node::ParentList::iterator pitr=parents.begin();
                pitr!=parents.end();
                ++pitr)
            {
                (*pitr)->replaceChild(transform.get(),child);
            }

        }

    }
    return transformRemoved;
}

////////////////////////////////////////////////////////////////////////////
// RemoveEmptyNodes.
////////////////////////////////////////////////////////////////////////////

void Optimizer::RemoveEmptyNodesVisitor::apply(osg::Group& group)
{
    if (group.getNumParents()>0)
    {
        // only remove empty groups, but not empty occluders.
        if (group.getNumChildren()==0 && isOperationPermissibleForObject(&group) &&
            (typeid(group)==typeid(osg::Group) || (group.asTransform())) &&
            (group.getNumChildrenRequiringUpdateTraversal()==0 && group.getNumChildrenRequiringEventTraversal()==0) )
        {
            _redundantNodeList.insert(&group);
        }
    }
    traverse(group);
}

void Optimizer::RemoveEmptyNodesVisitor::removeEmptyNodes()
{

    NodeList newEmptyGroups;

    // keep iterator through until scene graph is cleaned of empty nodes.
    while (!_redundantNodeList.empty())
    {
        for(NodeList::iterator itr=_redundantNodeList.begin();
            itr!=_redundantNodeList.end();
            ++itr)
        {

            osg::ref_ptr<osg::Node> nodeToRemove = (*itr);

            // take a copy of parents list since subsequent removes will modify the original one.
            osg::Node::ParentList parents = nodeToRemove->getParents();

            for(osg::Node::ParentList::iterator pitr=parents.begin();
                pitr!=parents.end();
                ++pitr)
            {
                osg::Group* parent = *pitr;
                if (!parent->asSwitch() && !dynamic_cast<osg::LOD*>(parent))
                {
                    parent->removeChild(nodeToRemove.get());
                    if (parent->getNumChildren()==0 && isOperationPermissibleForObject(parent)) newEmptyGroups.insert(parent);
                }
            }
        }

        _redundantNodeList.clear();
        _redundantNodeList.swap(newEmptyGroups);
    }
}


////////////////////////////////////////////////////////////////////////////
// RemoveRedundantNodes.
////////////////////////////////////////////////////////////////////////////

bool Optimizer::RemoveRedundantNodesVisitor::isOperationPermissible(osg::Node& node)
{
    return node.getNumParents()>0 &&
           !node.getStateSet() &&
           !node.getCullCallback() &&
           !node.getEventCallback() &&
           !node.getUpdateCallback() &&
           isOperationPermissibleForObject(&node);
}

void Optimizer::RemoveRedundantNodesVisitor::apply(osg::LOD& lod)
{
    // don't remove any direct children of the LOD because they are used to define each LOD level.
    for (unsigned int i=0; i<lod.getNumChildren(); ++i)
        traverse(*lod.getChild(i));
}

void Optimizer::RemoveRedundantNodesVisitor::apply(osg::Switch& switchNode)
{
    // We should keep all switch child nodes since they reflect different switch states.
    for (unsigned int i=0; i<switchNode.getNumChildren(); ++i)
        traverse(*switchNode.getChild(i));
}

void Optimizer::RemoveRedundantNodesVisitor::apply(osg::Group& group)
{
    if (typeid(group)==typeid(osg::Group) &&
        isOperationPermissible(group))
    {
        _redundantNodeList.insert(&group);
    }

    traverse(group);
}



void Optimizer::RemoveRedundantNodesVisitor::apply(osg::Transform& transform)
{
    if (transform.getDataVariance()==osg::Object::STATIC &&
        isOperationPermissible(transform))
    {
        osg::Matrix matrix;
        transform.computeWorldToLocalMatrix(matrix,nullptr);
        if (matrix.isIdentity())
        {
            _redundantNodeList.insert(&transform);
        }
    }
    traverse(transform);
}


void Optimizer::RemoveRedundantNodesVisitor::removeRedundantNodes()
{

    for(NodeList::iterator itr=_redundantNodeList.begin();
        itr!=_redundantNodeList.end();
        ++itr)
    {
        osg::ref_ptr<osg::Group> group = (*itr)->asGroup();
        if (group.valid())
        {
            // take a copy of parents list since subsequent removes will modify the original one.
            osg::Node::ParentList parents = group->getParents();

            for(osg::Node::ParentList::iterator pitr=parents.begin();
                pitr!=parents.end();
                ++pitr)
            {
                unsigned int childIndex = (*pitr)->getChildIndex(group);
                for (unsigned int i=0; i<group->getNumChildren(); ++i)
                {
                    osg::Node* child = group->getChild(i);
                    (*pitr)->insertChild(childIndex++, child);
                }

                (*pitr)->removeChild(group);
            }

            group->removeChildren(0, group->getNumChildren());
        }
        else
        {
            OSG_WARN<<"Optimizer::RemoveRedundantNodesVisitor::removeRedundantNodes() - failed dynamic_cast"<<std::endl;
        }
    }
    _redundantNodeList.clear();
}



////////////////////////////////////////////////////////////////////////////
// code to merge geometry object which share, state, and attribute bindings.
////////////////////////////////////////////////////////////////////////////

#define COMPARE_BINDING(lhs, rhs) \
        if (osg::getBinding(lhs)<osg::getBinding(rhs)) return true; \
        if (osg::getBinding(rhs)<osg::getBinding(lhs)) return false;


struct LessGeometry
{
    bool operator() (const osg::ref_ptr<osg::Geometry>& lhs,const osg::ref_ptr<osg::Geometry>& rhs) const
    {
        if (lhs->getStateSet()<rhs->getStateSet()) return true;
        if (rhs->getStateSet()<lhs->getStateSet()) return false;

        COMPARE_BINDING(lhs->getNormalArray(), rhs->getNormalArray())
        COMPARE_BINDING(lhs->getColorArray(), rhs->getColorArray())
        COMPARE_BINDING(lhs->getSecondaryColorArray(), rhs->getSecondaryColorArray())
        COMPARE_BINDING(lhs->getFogCoordArray(), rhs->getFogCoordArray())


        if (lhs->getNumTexCoordArrays()<rhs->getNumTexCoordArrays()) return true;
        if (rhs->getNumTexCoordArrays()<lhs->getNumTexCoordArrays()) return false;

        // therefore lhs->getNumTexCoordArrays()==rhs->getNumTexCoordArrays()

        unsigned int i;
        for(i=0;i<lhs->getNumTexCoordArrays();++i)
        {
            if (rhs->getTexCoordArray(i))
            {
                if (!lhs->getTexCoordArray(i)) return true;
            }
            else if (lhs->getTexCoordArray(i)) return false;
        }

        for(i=0;i<lhs->getNumVertexAttribArrays();++i)
        {
            if (rhs->getVertexAttribArray(i))
            {
                if (!lhs->getVertexAttribArray(i)) return true;
            }
            else if (lhs->getVertexAttribArray(i)) return false;
        }


        if (osg::getBinding(lhs->getNormalArray())==osg::Array::BIND_OVERALL)
        {
            // assumes that the bindings and arrays are set up correctly, this
            // should be the case after running computeCorrectBindingsAndArraySizes();
            const osg::Array* lhs_normalArray = lhs->getNormalArray();
            const osg::Array* rhs_normalArray = rhs->getNormalArray();
            if (lhs_normalArray->getType()<rhs_normalArray->getType()) return true;
            if (rhs_normalArray->getType()<lhs_normalArray->getType()) return false;
            switch(lhs_normalArray->getType())
            {
            case(osg::Array::Vec3bArrayType):
                if ((*static_cast<const osg::Vec3bArray*>(lhs_normalArray))[0]<(*static_cast<const osg::Vec3bArray*>(rhs_normalArray))[0]) return true;
                if ((*static_cast<const osg::Vec3bArray*>(rhs_normalArray))[0]<(*static_cast<const osg::Vec3bArray*>(lhs_normalArray))[0]) return false;
                break;
            case(osg::Array::Vec3sArrayType):
                if ((*static_cast<const osg::Vec3sArray*>(lhs_normalArray))[0]<(*static_cast<const osg::Vec3sArray*>(rhs_normalArray))[0]) return true;
                if ((*static_cast<const osg::Vec3sArray*>(rhs_normalArray))[0]<(*static_cast<const osg::Vec3sArray*>(lhs_normalArray))[0]) return false;
                break;
            case(osg::Array::Vec3ArrayType):
                if ((*static_cast<const osg::Vec3Array*>(lhs_normalArray))[0]<(*static_cast<const osg::Vec3Array*>(rhs_normalArray))[0]) return true;
                if ((*static_cast<const osg::Vec3Array*>(rhs_normalArray))[0]<(*static_cast<const osg::Vec3Array*>(lhs_normalArray))[0]) return false;
                break;
            default:
                break;
            }
        }

        if (osg::getBinding(lhs->getColorArray())==osg::Array::BIND_OVERALL)
        {
            const osg::Array* lhs_colorArray = lhs->getColorArray();
            const osg::Array* rhs_colorArray = rhs->getColorArray();
            if (lhs_colorArray->getType()<rhs_colorArray->getType()) return true;
            if (rhs_colorArray->getType()<lhs_colorArray->getType()) return false;
            switch(lhs_colorArray->getType())
            {
                case(osg::Array::Vec4ubArrayType):
                    if ((*static_cast<const osg::Vec4ubArray*>(lhs_colorArray))[0]<(*static_cast<const osg::Vec4ubArray*>(rhs_colorArray))[0]) return true;
                    if ((*static_cast<const osg::Vec4ubArray*>(rhs_colorArray))[0]<(*static_cast<const osg::Vec4ubArray*>(lhs_colorArray))[0]) return false;
                    break;
                case(osg::Array::Vec3ArrayType):
                    if ((*static_cast<const osg::Vec3Array*>(lhs_colorArray))[0]<(*static_cast<const osg::Vec3Array*>(rhs_colorArray))[0]) return true;
                    if ((*static_cast<const osg::Vec3Array*>(rhs_colorArray))[0]<(*static_cast<const osg::Vec3Array*>(lhs_colorArray))[0]) return false;
                    break;
                case(osg::Array::Vec4ArrayType):
                    if ((*static_cast<const osg::Vec4Array*>(lhs_colorArray))[0]<(*static_cast<const osg::Vec4Array*>(rhs_colorArray))[0]) return true;
                    if ((*static_cast<const osg::Vec4Array*>(rhs_colorArray))[0]<(*static_cast<const osg::Vec4Array*>(lhs_colorArray))[0]) return false;
                    break;
                default:
                    break;
            }

        }

        return false;

    }
};

struct LessGeometryPrimitiveType
{
    bool operator() (const osg::ref_ptr<osg::Geometry>& lhs,const osg::ref_ptr<osg::Geometry>& rhs) const
    {
        for(unsigned int i=0;
            i<lhs->getNumPrimitiveSets() && i<rhs->getNumPrimitiveSets();
            ++i)
        {
            if (lhs->getPrimitiveSet(i)->getType()<rhs->getPrimitiveSet(i)->getType()) return true;
            else if (rhs->getPrimitiveSet(i)->getType()<lhs->getPrimitiveSet(i)->getType()) return false;

            if (lhs->getPrimitiveSet(i)->getMode()<rhs->getPrimitiveSet(i)->getMode()) return true;
            else if (rhs->getPrimitiveSet(i)->getMode()<lhs->getPrimitiveSet(i)->getMode()) return false;

        }
        return lhs->getNumPrimitiveSets()<rhs->getNumPrimitiveSets();
    }
};


/// Shortcut to get size of an array, even if pointer is nullptr.
inline unsigned int getSize(const osg::Array * a) { return a ? a->getNumElements() : 0; }

/// When merging geometries, tests if two arrays can be merged, regarding to their number of components, and the number of vertices.
bool isArrayCompatible(unsigned int numVertice1, unsigned int numVertice2, const osg::Array* compare1, const osg::Array* compare2)
{
    // Sumed up truth table:
    //  If array (1 or 2) not empty and vertices empty => error, should not happen (allows simplification in formulae below)
    //  If one side has both vertices and array, and the other side has only vertices => then arrays cannot be merged
    //  Else, arrays can be merged
    //assert(numVertice1 || !getSize(compare1));
    //assert(numVertice2 || !getSize(compare2));
    return !(   (numVertice1 && !getSize(compare1) && getSize(compare2))
             || (numVertice2 && !getSize(compare2) && getSize(compare1)) );
}

/// Return true only if both geometries have same array type and if arrays (such as TexCoords) are compatible (i.e. both empty or both filled)
bool isAbleToMerge(const osg::Geometry& g1, const osg::Geometry& g2)
{
    unsigned int numVertice1( getSize(g1.getVertexArray()) );
    unsigned int numVertice2( getSize(g2.getVertexArray()) );

    // first verify arrays size
    if (!isArrayCompatible(numVertice1,numVertice2,g1.getNormalArray(),g2.getNormalArray()) ||
        !isArrayCompatible(numVertice1,numVertice2,g1.getColorArray(),g2.getColorArray()) ||
        !isArrayCompatible(numVertice1,numVertice2,g1.getSecondaryColorArray(),g2.getSecondaryColorArray()) ||
        !isArrayCompatible(numVertice1,numVertice2,g1.getFogCoordArray(),g2.getFogCoordArray()) ||
        g1.getNumTexCoordArrays()!=g2.getNumTexCoordArrays()) return false;

    for (unsigned int eachTexCoordArray=0;eachTexCoordArray<g1.getNumTexCoordArrays();++eachTexCoordArray)
    {
        if (!isArrayCompatible(numVertice1,numVertice2,g1.getTexCoordArray(eachTexCoordArray),g2.getTexCoordArray(eachTexCoordArray))) return false;
    }

    // then verify data type compatibility
    if (g1.getVertexArray() && g2.getVertexArray() && g1.getVertexArray()->getDataType()!=g2.getVertexArray()->getDataType()) return false;
    if (g1.getNormalArray() && g2.getNormalArray() && g1.getNormalArray()->getDataType()!=g2.getNormalArray()->getDataType()) return false;
    if (g1.getColorArray() && g2.getColorArray() && g1.getColorArray()->getDataType()!=g2.getColorArray()->getDataType()) return false;
    if (g1.getSecondaryColorArray() && g2.getSecondaryColorArray() && g1.getSecondaryColorArray()->getDataType()!=g2.getSecondaryColorArray()->getDataType()) return false;
    if (g1.getFogCoordArray() && g2.getNormalArray() && g1.getFogCoordArray()->getDataType()!=g2.getFogCoordArray()->getDataType()) return false;
    return true;
}


void Optimizer::MergeGeometryVisitor::pushStateSet(osg::StateSet *stateSet)
{
    _stateSetStack.push_back(stateSet);
    checkAllowedToMerge();
}

void Optimizer::MergeGeometryVisitor::popStateSet()
{
    _stateSetStack.pop_back();
    checkAllowedToMerge();
}

void Optimizer::MergeGeometryVisitor::checkAllowedToMerge()
{
    int renderingHint = 0;
    bool override = false;
    for (std::vector<osg::StateSet*>::const_iterator it = _stateSetStack.begin(); it != _stateSetStack.end(); ++it)
    {
        osg::StateSet* stateSet = *it;
        osg::StateSet::RenderBinMode mode = stateSet->getRenderBinMode();
        if (override && !(mode & osg::StateSet::PROTECTED_RENDERBIN_DETAILS))
            continue;
        if (mode & osg::StateSet::USE_RENDERBIN_DETAILS)
            renderingHint = stateSet->getRenderingHint();
        if (mode & osg::StateSet::OVERRIDE_RENDERBIN_DETAILS)
            override = true;
    }
    // Can't merge Geometry that are using a transparent sorting bin as that would cause the sorting to break.
    _allowedToMerge = renderingHint != osg::StateSet::TRANSPARENT_BIN;
}

void Optimizer::MergeGeometryVisitor::apply(osg::Group &group)
{
    if (group.getStateSet())
        pushStateSet(group.getStateSet());

    if (_allowedToMerge)
        mergeGroup(group);

    traverse(group);

    if (group.getStateSet())
        popStateSet();
}

bool Optimizer::MergeGeometryVisitor::mergeGroup(osg::Group& group)
{
    if (!isOperationPermissibleForObject(&group)) return false;

    if (group.getNumChildren()>=2)
    {

        typedef std::vector< osg::ref_ptr<osg::Geometry> >                          DuplicateList;
        typedef std::vector< osg::ref_ptr<osg::Node> >                              Nodes;
        typedef std::map< osg::ref_ptr<osg::Geometry> ,DuplicateList,LessGeometry>  GeometryDuplicateMap;

        typedef std::vector<DuplicateList> MergeList;

        GeometryDuplicateMap geometryDuplicateMap;
        Nodes standardChildren;

        unsigned int i;
        for(i=0;i<group.getNumChildren();++i)
        {
            osg::Node* child = group.getChild(i);
            osg::Geometry* geom = child->asGeometry();
            if (geom)
            {
                if (!geometryContainsSharedArrays(*geom) &&
                    geom->getDataVariance()!=osg::Object::DYNAMIC &&
                    isOperationPermissibleForObject(geom))
                {
                    geometryDuplicateMap[geom].push_back(geom);
                }
                else
                {
                    standardChildren.push_back(geom);
                }
            }
            else
            {
                standardChildren.push_back(child);
            }
        }

        // first try to group geometries with the same properties
        // (i.e. array types) to avoid loss of data during merging
        MergeList mergeListChecked;        // List of drawables just before merging, grouped by "compatibility" and vertex limit
        MergeList mergeList;            // Intermediate list of drawables, grouped ony by "compatibility"
        for(GeometryDuplicateMap::iterator itr=geometryDuplicateMap.begin();
            itr!=geometryDuplicateMap.end();
            ++itr)
        {
            if (itr->second.empty()) continue;
            if (itr->second.size()==1)
            {
                mergeList.push_back(DuplicateList());
                DuplicateList* duplicateList = &mergeList.back();
                duplicateList->push_back(itr->second[0]);
                continue;
            }

            std::sort(itr->second.begin(),itr->second.end(),LessGeometryPrimitiveType());

            // initialize the temporary list by pushing the first geometry
            MergeList mergeListTmp;
            mergeListTmp.push_back(DuplicateList());
            DuplicateList* duplicateList = &mergeListTmp.back();
            duplicateList->push_back(itr->second[0]);

            for(DuplicateList::iterator dupItr=itr->second.begin()+1;
                dupItr!=itr->second.end();
                ++dupItr)
            {
                osg::Geometry* geomToPush = dupItr->get();

                // try to group geomToPush with another geometry
                MergeList::iterator eachMergeList=mergeListTmp.begin();
                for(;eachMergeList!=mergeListTmp.end();++eachMergeList)
                {
                    if (!eachMergeList->empty() && eachMergeList->front()!=nullptr
                        && isAbleToMerge(*eachMergeList->front(),*geomToPush))
                    {
                        eachMergeList->push_back(geomToPush);
                        break;
                    }
                }

                // if no suitable group was found, then a new one is created
                if (eachMergeList==mergeListTmp.end())
                {
                    mergeListTmp.push_back(DuplicateList());
                    duplicateList = &mergeListTmp.back();
                    duplicateList->push_back(geomToPush);
                }
            }

            // copy the group in the mergeListChecked
            for(MergeList::iterator eachMergeList=mergeListTmp.begin();eachMergeList!=mergeListTmp.end();++eachMergeList)
            {
                mergeListChecked.push_back(*eachMergeList);
            }
        }

        // then build merge list using _targetMaximumNumberOfVertices
        bool needToDoMerge = false;
        // dequeue each DuplicateList when vertices limit is reached or when all elements has been checked
        for(MergeList::iterator itr=mergeListChecked.begin(); itr!=mergeListChecked.end(); ++itr)
        {
            DuplicateList& duplicateList(*itr);
            if (duplicateList.size()==0)
            {
                continue;
            }

            if (duplicateList.size()==1)
            {
                mergeList.push_back(duplicateList);
                continue;
            }

            unsigned int totalNumberVertices = 0;
            DuplicateList subset;
            for(DuplicateList::iterator ditr = duplicateList.begin();
                ditr != duplicateList.end();
                ++ditr)
            {
                osg::Geometry* geometry = ditr->get();
                unsigned int numVertices = (geometry->getVertexArray() ? geometry->getVertexArray()->getNumElements() : 0);
                if ((totalNumberVertices+numVertices)>_targetMaximumNumberOfVertices && !subset.empty())
                {
                    mergeList.push_back(subset);
                    subset.clear();
                    totalNumberVertices = 0;
                }
                totalNumberVertices += numVertices;
                subset.push_back(geometry);
                if (subset.size()>1) needToDoMerge = true;
            }
            if (!subset.empty()) mergeList.push_back(subset);
        }

        if (needToDoMerge)
        {
            // to avoid performance issues associated with incrementally removing a large number children, we remove them all and add back the ones we need.
            group.removeChildren(0, group.getNumChildren());

            for(Nodes::iterator itr = standardChildren.begin();
                itr != standardChildren.end();
                ++itr)
            {
                group.addChild(*itr);
            }

            // now do the merging of geometries
            for(MergeList::iterator mitr = mergeList.begin();
                mitr != mergeList.end();
                ++mitr)
            {
                DuplicateList& duplicateList = *mitr;
                if (!duplicateList.empty())
                {
                    DuplicateList::iterator ditr = duplicateList.begin();
                    osg::ref_ptr<osg::Geometry> lhs = *ditr++;
                    group.addChild(lhs.get());
                    for(;
                        ditr != duplicateList.end();
                        ++ditr)
                    {
                        mergeGeometry(*lhs, **ditr);
                    }
                }
            }
        }

    }


    // convert all polygon primitives which has 3 indices into TRIANGLES, 4 indices into QUADS.
    unsigned int i;
    for(i=0;i<group.getNumChildren();++i)
    {
        osg::Drawable* drawable = group.getChild(i)->asDrawable();
        if (!drawable)
            continue;
        osg::Geometry* geom = drawable->asGeometry();
        if (geom)
        {
            osg::Geometry::PrimitiveSetList& primitives = geom->getPrimitiveSetList();
            for(osg::Geometry::PrimitiveSetList::iterator itr=primitives.begin();
                itr!=primitives.end();
                ++itr)
            {
                osg::PrimitiveSet* prim = itr->get();
                if (prim->getMode()==osg::PrimitiveSet::POLYGON)
                {
                    if (prim->getNumIndices()==3)
                    {
                        prim->setMode(osg::PrimitiveSet::TRIANGLES);
                    }
                    else if (prim->getNumIndices()==4)
                    {
                        prim->setMode(osg::PrimitiveSet::QUADS);
                    }
                }
            }
        }
    }

    // now merge any compatible primitives.
    for(i=0;i<group.getNumChildren();++i)
    {
        osg::Drawable* drawable = group.getChild(i)->asDrawable();
        if (!drawable)
            continue;
        osg::Geometry* geom = drawable->asGeometry();
        if (geom)
        {
            if (geom->getNumPrimitiveSets()>0 &&
                osg::getBinding(geom->getNormalArray())!=osg::Array::BIND_PER_PRIMITIVE_SET &&
                osg::getBinding(geom->getColorArray())!=osg::Array::BIND_PER_PRIMITIVE_SET &&
                osg::getBinding(geom->getSecondaryColorArray())!=osg::Array::BIND_PER_PRIMITIVE_SET &&
                osg::getBinding(geom->getFogCoordArray())!=osg::Array::BIND_PER_PRIMITIVE_SET)
            {

#if 1
                bool doneCombine = false;

                osg::Geometry::PrimitiveSetList& primitives = geom->getPrimitiveSetList();
                unsigned int lhsNo=0;
                unsigned int rhsNo=1;
                while(rhsNo<primitives.size())
                {
                    osg::PrimitiveSet* lhs = primitives[lhsNo].get();
                    osg::PrimitiveSet* rhs = primitives[rhsNo].get();

                    bool combine = false;

                    if (lhs->getType()==rhs->getType() &&
                        lhs->getMode()==rhs->getMode())
                    {

                        switch(lhs->getMode())
                        {
                        case(osg::PrimitiveSet::POINTS):
                        case(osg::PrimitiveSet::LINES):
                        case(osg::PrimitiveSet::TRIANGLES):
                        case(osg::PrimitiveSet::QUADS):
                            combine = true;
                            break;
                        }

                    }

                    if (combine)
                    {

                        switch(lhs->getType())
                        {
                        case(osg::PrimitiveSet::DrawArraysPrimitiveType):
                            combine = mergePrimitive(*(static_cast<osg::DrawArrays*>(lhs)),*(static_cast<osg::DrawArrays*>(rhs)));
                            break;
                        case(osg::PrimitiveSet::DrawArrayLengthsPrimitiveType):
                            combine = mergePrimitive(*(static_cast<osg::DrawArrayLengths*>(lhs)),*(static_cast<osg::DrawArrayLengths*>(rhs)));
                            break;
                        case(osg::PrimitiveSet::DrawElementsUBytePrimitiveType):
                            combine = mergePrimitive(*(static_cast<osg::DrawElementsUByte*>(lhs)),*(static_cast<osg::DrawElementsUByte*>(rhs)));
                            break;
                        case(osg::PrimitiveSet::DrawElementsUShortPrimitiveType):
                            combine = mergePrimitive(*(static_cast<osg::DrawElementsUShort*>(lhs)),*(static_cast<osg::DrawElementsUShort*>(rhs)));
                            break;
                        case(osg::PrimitiveSet::DrawElementsUIntPrimitiveType):
                            combine = mergePrimitive(*(static_cast<osg::DrawElementsUInt*>(lhs)),*(static_cast<osg::DrawElementsUInt*>(rhs)));
                            break;
                        default:
                            combine = false;
                            break;
                        }
                    }

                    if (combine)
                    {
                        // make this primitive set as invalid and needing cleaning up.
                        rhs->setMode(0xffffff);
                        doneCombine = true;
                        ++rhsNo;
                    }
                    else
                    {
                        lhsNo = rhsNo;
                        ++rhsNo;
                    }
                }

    #if 1
                if (doneCombine)
                {
                    // now need to clean up primitiveset so it no longer contains the rhs combined primitives.

                    // first swap with a empty primitiveSet to empty it completely.
                    osg::Geometry::PrimitiveSetList oldPrimitives;
                    primitives.swap(oldPrimitives);

                    // now add the active primitive sets
                    for(osg::Geometry::PrimitiveSetList::iterator pitr = oldPrimitives.begin();
                        pitr != oldPrimitives.end();
                        ++pitr)
                    {
                        if ((*pitr)->getMode()!=0xffffff) primitives.push_back(*pitr);
                    }
                }
    #endif

#else

                osg::Geometry::PrimitiveSetList& primitives = geom->getPrimitiveSetList();
                unsigned int primNo=0;
                while(primNo+1<primitives.size())
                {
                    osg::PrimitiveSet* lhs = primitives[primNo].get();
                    osg::PrimitiveSet* rhs = primitives[primNo+1].get();

                    bool combine = false;

                    if (lhs->getType()==rhs->getType() &&
                        lhs->getMode()==rhs->getMode())
                    {

                        switch(lhs->getMode())
                        {
                        case(osg::PrimitiveSet::POINTS):
                        case(osg::PrimitiveSet::LINES):
                        case(osg::PrimitiveSet::TRIANGLES):
                        case(osg::PrimitiveSet::QUADS):
                            combine = true;
                            break;
                        }

                    }

                    if (combine)
                    {

                        switch(lhs->getType())
                        {
                        case(osg::PrimitiveSet::DrawArraysPrimitiveType):
                            combine = mergePrimitive(*(static_cast<osg::DrawArrays*>(lhs)),*(static_cast<osg::DrawArrays*>(rhs)));
                            break;
                        case(osg::PrimitiveSet::DrawArrayLengthsPrimitiveType):
                            combine = mergePrimitive(*(static_cast<osg::DrawArrayLengths*>(lhs)),*(static_cast<osg::DrawArrayLengths*>(rhs)));
                            break;
                        case(osg::PrimitiveSet::DrawElementsUBytePrimitiveType):
                            combine = mergePrimitive(*(static_cast<osg::DrawElementsUByte*>(lhs)),*(static_cast<osg::DrawElementsUByte*>(rhs)));
                            break;
                        case(osg::PrimitiveSet::DrawElementsUShortPrimitiveType):
                            combine = mergePrimitive(*(static_cast<osg::DrawElementsUShort*>(lhs)),*(static_cast<osg::DrawElementsUShort*>(rhs)));
                            break;
                        case(osg::PrimitiveSet::DrawElementsUIntPrimitiveType):
                            combine = mergePrimitive(*(static_cast<osg::DrawElementsUInt*>(lhs)),*(static_cast<osg::DrawElementsUInt*>(rhs)));
                            break;
                        default:
                            break;
                        }
                    }
                    if (combine)
                    {
                        primitives.erase(primitives.begin()+primNo+1);
                    }

                    if (!combine)
                    {
                        primNo++;
                    }
                }
#endif
            }
        }


    }

//    geode.dirtyBound();


    return false;
}

bool Optimizer::MergeGeometryVisitor::geometryContainsSharedArrays(osg::Geometry& geom)
{
    if (geom.getVertexArray() && geom.getVertexArray()->referenceCount()>1) return true;
    if (geom.getNormalArray() && geom.getNormalArray()->referenceCount()>1) return true;
    if (geom.getColorArray() && geom.getColorArray()->referenceCount()>1) return true;
    if (geom.getSecondaryColorArray() && geom.getSecondaryColorArray()->referenceCount()>1) return true;
    if (geom.getFogCoordArray() && geom.getFogCoordArray()->referenceCount()>1) return true;


    for(unsigned int unit=0;unit<geom.getNumTexCoordArrays();++unit)
    {
        osg::Array* tex = geom.getTexCoordArray(unit);
        if (tex && tex->referenceCount()>1) return true;
    }

    // shift the indices of the incoming primitives to account for the pre existing geometry.
    for(osg::Geometry::PrimitiveSetList::iterator primItr=geom.getPrimitiveSetList().begin();
        primItr!=geom.getPrimitiveSetList().end();
        ++primItr)
    {
        if ((*primItr)->referenceCount()>1) return true;
    }


    return false;
}


class MergeArrayVisitor : public osg::ArrayVisitor
{
    protected:
        osg::Array* _lhs;
    public:
        MergeArrayVisitor() :
            _lhs(0) {}


        /// try to merge the content of two arrays.
        bool merge(osg::Array* lhs,osg::Array* rhs)
        {
            if (lhs==0 || rhs==0) return true;
            if (lhs->getType()!=rhs->getType()) return false;

            _lhs = lhs;

            rhs->accept(*this);
            return true;
        }

        template<typename T>
        void _merge(T& rhs)
        {
            T* lhs = static_cast<T*>(_lhs);
            lhs->insert(lhs->end(),rhs.begin(),rhs.end());
        }

        virtual void apply(osg::Array&) { OSG_WARN << "Warning: Optimizer's MergeArrayVisitor cannot merge Array type." << std::endl; }


        virtual void apply(osg::ByteArray& rhs) { _merge(rhs); }
        virtual void apply(osg::ShortArray& rhs) { _merge(rhs); }
        virtual void apply(osg::IntArray& rhs) { _merge(rhs); }
        virtual void apply(osg::UByteArray& rhs) { _merge(rhs); }
        virtual void apply(osg::UShortArray& rhs) { _merge(rhs); }
        virtual void apply(osg::UIntArray& rhs) { _merge(rhs); }

        virtual void apply(osg::Vec4ubArray& rhs) { _merge(rhs); }
        virtual void apply(osg::Vec3ubArray& rhs) { _merge(rhs); }
        virtual void apply(osg::Vec2ubArray& rhs) { _merge(rhs); }

        virtual void apply(osg::Vec4usArray& rhs) { _merge(rhs); }
        virtual void apply(osg::Vec3usArray& rhs) { _merge(rhs); }
        virtual void apply(osg::Vec2usArray& rhs) { _merge(rhs); }

        virtual void apply(osg::FloatArray& rhs) { _merge(rhs); }
        virtual void apply(osg::Vec2Array& rhs) { _merge(rhs); }
        virtual void apply(osg::Vec3Array& rhs) { _merge(rhs); }
        virtual void apply(osg::Vec4Array& rhs) { _merge(rhs); }

        virtual void apply(osg::DoubleArray& rhs) { _merge(rhs); }
        virtual void apply(osg::Vec2dArray& rhs) { _merge(rhs); }
        virtual void apply(osg::Vec3dArray& rhs) { _merge(rhs); }
        virtual void apply(osg::Vec4dArray& rhs) { _merge(rhs); }

        virtual void apply(osg::Vec2bArray&  rhs) { _merge(rhs); }
        virtual void apply(osg::Vec3bArray&  rhs) { _merge(rhs); }
        virtual void apply(osg::Vec4bArray&  rhs) { _merge(rhs); }

        virtual void apply(osg::Vec2sArray& rhs) { _merge(rhs); }
        virtual void apply(osg::Vec3sArray& rhs) { _merge(rhs); }
        virtual void apply(osg::Vec4sArray& rhs) { _merge(rhs); }
};

bool Optimizer::MergeGeometryVisitor::mergeGeometry(osg::Geometry& lhs,osg::Geometry& rhs)
{

    MergeArrayVisitor merger;

    unsigned int base = 0;
    if (lhs.getVertexArray() && rhs.getVertexArray())
    {

        base = lhs.getVertexArray()->getNumElements();
        if (!merger.merge(lhs.getVertexArray(),rhs.getVertexArray()))
        {
            OSG_DEBUG << "MergeGeometry: vertex array not merged. Some data may be lost." <<std::endl;
        }
    }
    else if (rhs.getVertexArray())
    {
        base = 0;
        lhs.setVertexArray(rhs.getVertexArray());
    }


    if (lhs.getNormalArray() && rhs.getNormalArray() && lhs.getNormalArray()->getBinding()!=osg::Array::BIND_OVERALL)
    {
        if (!merger.merge(lhs.getNormalArray(),rhs.getNormalArray()))
        {
            OSG_DEBUG << "MergeGeometry: normal array not merged. Some data may be lost." <<std::endl;
        }
    }
    else if (rhs.getNormalArray())
    {
        lhs.setNormalArray(rhs.getNormalArray());
    }


    if (lhs.getColorArray() && rhs.getColorArray() && lhs.getColorArray()->getBinding()!=osg::Array::BIND_OVERALL)
    {
        if (!merger.merge(lhs.getColorArray(),rhs.getColorArray()))
        {
            OSG_DEBUG << "MergeGeometry: color array not merged. Some data may be lost." <<std::endl;
        }
    }
    else if (rhs.getColorArray())
    {
        lhs.setColorArray(rhs.getColorArray());
    }

    if (lhs.getSecondaryColorArray() && rhs.getSecondaryColorArray() && lhs.getSecondaryColorArray()->getBinding()!=osg::Array::BIND_OVERALL)
    {
        if (!merger.merge(lhs.getSecondaryColorArray(),rhs.getSecondaryColorArray()))
        {
            OSG_DEBUG << "MergeGeometry: secondary color array not merged. Some data may be lost." <<std::endl;
        }
    }
    else if (rhs.getSecondaryColorArray())
    {
        lhs.setSecondaryColorArray(rhs.getSecondaryColorArray());
    }

    if (lhs.getFogCoordArray() && rhs.getFogCoordArray() && lhs.getFogCoordArray()->getBinding()!=osg::Array::BIND_OVERALL)
    {
        if (!merger.merge(lhs.getFogCoordArray(),rhs.getFogCoordArray()))
        {
            OSG_DEBUG << "MergeGeometry: fog coord array not merged. Some data may be lost." <<std::endl;
        }
    }
    else if (rhs.getFogCoordArray())
    {
        lhs.setFogCoordArray(rhs.getFogCoordArray());
    }


    unsigned int unit;
    for(unit=0;unit<lhs.getNumTexCoordArrays();++unit)
    {
        if (!merger.merge(lhs.getTexCoordArray(unit),rhs.getTexCoordArray(unit)))
        {
            OSG_DEBUG << "MergeGeometry: tex coord array not merged. Some data may be lost." <<std::endl;
        }
    }

    for(unit=0;unit<lhs.getNumVertexAttribArrays();++unit)
    {
        if (!merger.merge(lhs.getVertexAttribArray(unit),rhs.getVertexAttribArray(unit)))
        {
            OSG_DEBUG << "MergeGeometry: vertex attrib array not merged. Some data may be lost." <<std::endl;
        }
    }


    // shift the indices of the incoming primitives to account for the pre existing geometry.
    osg::Geometry::PrimitiveSetList::iterator primItr;
    for(primItr=rhs.getPrimitiveSetList().begin(); primItr!=rhs.getPrimitiveSetList().end(); ++primItr)
    {
        osg::PrimitiveSet* primitive = primItr->get();

        switch(primitive->getType())
        {
        case(osg::PrimitiveSet::DrawElementsUBytePrimitiveType):
            {
                osg::DrawElementsUByte* primitiveUByte = static_cast<osg::DrawElementsUByte*>(primitive);
                unsigned int currentMaximum = 0;
                for(osg::DrawElementsUByte::iterator eitr=primitiveUByte->begin();
                    eitr!=primitiveUByte->end();
                    ++eitr)
                {
                    currentMaximum = osg::maximum(currentMaximum,(unsigned int)*eitr);
                }
                if ((base+currentMaximum)>=65536)
                {
                    // must promote to a DrawElementsUInt
                    osg::DrawElementsUInt* new_primitive = new osg::DrawElementsUInt(primitive->getMode());
                    std::copy(primitiveUByte->begin(),primitiveUByte->end(),std::back_inserter(*new_primitive));
                    new_primitive->offsetIndices(base);
                    (*primItr) = new_primitive;
                } else if ((base+currentMaximum)>=256)
                {
                    // must promote to a DrawElementsUShort
                    osg::DrawElementsUShort* new_primitive = new osg::DrawElementsUShort(primitive->getMode());
                    std::copy(primitiveUByte->begin(),primitiveUByte->end(),std::back_inserter(*new_primitive));
                    new_primitive->offsetIndices(base);
                    (*primItr) = new_primitive;
                }
                else
                {
                    primitive->offsetIndices(base);
                }
            }
            break;

        case(osg::PrimitiveSet::DrawElementsUShortPrimitiveType):
            {
                osg::DrawElementsUShort* primitiveUShort = static_cast<osg::DrawElementsUShort*>(primitive);
                unsigned int currentMaximum = 0;
                for(osg::DrawElementsUShort::iterator eitr=primitiveUShort->begin();
                    eitr!=primitiveUShort->end();
                    ++eitr)
                {
                    currentMaximum = osg::maximum(currentMaximum,(unsigned int)*eitr);
                }
                if ((base+currentMaximum)>=65536)
                {
                    // must promote to a DrawElementsUInt
                    osg::DrawElementsUInt* new_primitive = new osg::DrawElementsUInt(primitive->getMode());
                    std::copy(primitiveUShort->begin(),primitiveUShort->end(),std::back_inserter(*new_primitive));
                    new_primitive->offsetIndices(base);
                    (*primItr) = new_primitive;
                }
                else
                {
                    primitive->offsetIndices(base);
                }
            }
            break;

        case(osg::PrimitiveSet::DrawArraysPrimitiveType):
        case(osg::PrimitiveSet::DrawArrayLengthsPrimitiveType):
        case(osg::PrimitiveSet::DrawElementsUIntPrimitiveType):
        default:
            primitive->offsetIndices(base);
            break;
        }
    }

    for(primItr=rhs.getPrimitiveSetList().begin(); primItr!=rhs.getPrimitiveSetList().end(); ++primItr)
    {
        lhs.addPrimitiveSet(primItr->get());
    }

    lhs.dirtyBound();
    lhs.dirtyDisplayList();

    return true;
}

bool Optimizer::MergeGeometryVisitor::mergePrimitive(osg::DrawArrays& lhs,osg::DrawArrays& rhs)
{
    if (lhs.getFirst()+lhs.getCount()==rhs.getFirst())
    {
        lhs.setCount(lhs.getCount()+rhs.getCount());
        return true;
    }
    return false;
}

bool Optimizer::MergeGeometryVisitor::mergePrimitive(osg::DrawArrayLengths& lhs,osg::DrawArrayLengths& rhs)
{
    int lhs_count = std::accumulate(lhs.begin(),lhs.end(),0);

    if (lhs.getFirst()+lhs_count==rhs.getFirst())
    {
        lhs.insert(lhs.end(),rhs.begin(),rhs.end());
        return true;
    }
    return false;
}

bool Optimizer::MergeGeometryVisitor::mergePrimitive(osg::DrawElementsUByte& lhs,osg::DrawElementsUByte& rhs)
{
    lhs.insert(lhs.end(),rhs.begin(),rhs.end());
    return true;
}

bool Optimizer::MergeGeometryVisitor::mergePrimitive(osg::DrawElementsUShort& lhs,osg::DrawElementsUShort& rhs)
{
    lhs.insert(lhs.end(),rhs.begin(),rhs.end());
    return true;
}

bool Optimizer::MergeGeometryVisitor::mergePrimitive(osg::DrawElementsUInt& lhs,osg::DrawElementsUInt& rhs)
{
    lhs.insert(lhs.end(),rhs.begin(),rhs.end());
    return true;
}



bool Optimizer::MergeGroupsVisitor::isOperationPermissible(osg::Group& node)
{
    return !node.getCullCallback() &&
           !node.getEventCallback() &&
           !node.getUpdateCallback() &&
            isOperationPermissibleForObject(&node) &&
           typeid(node)==typeid(osg::Group);
}

void Optimizer::MergeGroupsVisitor::apply(osg::LOD &lod)
{
    // don't merge the direct children of the LOD because they are used to define each LOD level.
    traverse(lod);
}

void Optimizer::MergeGroupsVisitor::apply(osg::Switch &switchNode)
{
    // We should keep all switch child nodes since they reflect different switch states.
    traverse(switchNode);
}

void Optimizer::MergeGroupsVisitor::apply(osg::Group &group)
{
    if (group.getNumChildren() <= 1)
        traverse(group);
    else
    {
        typedef std::map<osg::StateSet*, std::set<osg::Group*> > GroupMap;
        GroupMap childGroups;
        for (unsigned int i=0; i<group.getNumChildren(); ++i)
        {
            osg::Node* child = group.getChild(i);
            osg::Group* childGroup = child->asGroup();
            if (childGroup && isOperationPermissible(*childGroup))
            {
                childGroups[childGroup->getStateSet()].insert(childGroup);
            }
        }

        for (GroupMap::iterator it = childGroups.begin(); it != childGroups.end(); ++it)
        {
            const std::set<osg::Group*>& groupSet = it->second;
            if (groupSet.size() <= 1)
                continue;
            else
            {
                osg::Group* first = *groupSet.begin();
                for (std::set<osg::Group*>::const_iterator groupIt = ++groupSet.begin(); groupIt != groupSet.end(); ++groupIt)
                {
                    osg::Group* toMerge = *groupIt;
                    for (unsigned int i=0; i<toMerge->getNumChildren(); ++i)
                        first->addChild(toMerge->getChild(i));
                    toMerge->removeChildren(0, toMerge->getNumChildren());
                    group.removeChild(toMerge);
                }
            }
        }
        traverse(group);
    }
}

}
