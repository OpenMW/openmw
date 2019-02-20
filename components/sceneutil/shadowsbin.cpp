#include "shadowsbin.hpp"
#include <unordered_set>
#include <osg/StateSet>
#include <osg/Material>
#include <osgUtil/StateGraph>

using namespace osgUtil;

namespace
{
    template <typename T>
    inline void accumulateState(T& currentValue, T newValue, bool& isOverride, unsigned int overrideFlags)
    {
        if (isOverride && !(overrideFlags & osg::StateAttribute::PROTECTED)) return;

        if (overrideFlags & osg::StateAttribute::OVERRIDE)
            isOverride = true;

        currentValue = newValue;
    }

    inline void accumulateModeState(const osg::StateSet* ss, bool& currentValue, bool& isOverride, int mode)
    {
        const osg::StateSet::ModeList& l = ss->getModeList();
        osg::StateSet::ModeList::const_iterator mf = l.find(mode);
        if (mf == l.end())
            return;
        int flags = mf->second;
        bool newValue = flags & osg::StateAttribute::ON;
        accumulateState(currentValue, newValue, isOverride, ss->getMode(mode));
    }

    inline bool materialNeedShadows(osg::Material* m)
    {
        return m->getDiffuse(osg::Material::FRONT).a() > 0.5;
    }
}

namespace SceneUtil
{

ShadowsBin::ShadowsBin()
{
    mStateSet = new osg::StateSet;
    mStateSet->addUniform(new osg::Uniform("useDiffuseMapForShadowAlpha", false));
}

bool ShadowsBin::cullStateGraph(StateGraph* sg, StateGraph* root, std::unordered_set<StateGraph*>& uninteresting)
{
    std::vector<StateGraph*> return_path;
    State state;
    StateGraph* sg_new = sg;
    do
    {
        if (uninteresting.find(sg_new) != uninteresting.end())
            break;
        return_path.push_back(sg_new);
        sg_new = sg_new->_parent;
    } while (sg_new && sg_new != root);
    for(std::vector<StateGraph*>::reverse_iterator itr=return_path.rbegin(); itr!=return_path.rend(); ++itr)
    {
        const osg::StateSet* ss = (*itr)->getStateSet();
        if (!ss) continue;
        accumulateModeState(ss, state.mAlphaBlend, state.mAlphaBlendOverride, GL_BLEND);
        accumulateModeState(ss, state.mAlphaTest, state.mAlphaTestOverride, GL_ALPHA_TEST);
        const osg::StateSet::AttributeList& l = ss->getAttributeList();
        osg::StateSet::AttributeList::const_iterator f = l.find(std::make_pair(osg::StateAttribute::MATERIAL, 0));
        if (f != l.end())
        {
            const osg::StateSet::RefAttributePair* rap = &f->second;
            accumulateState(state.mMaterial, static_cast<osg::Material*>(rap->first.get()), state.mMaterialOverride, rap->second);
            if (state.mMaterial && !materialNeedShadows(state.mMaterial))
                state.mMaterial = nullptr;
        }
        f = l.find(std::make_pair(osg::StateAttribute::FRONTFACE, 0));
        if (f != l.end())
            state.mImportantState = true;
        if ((*itr) != sg && !state.interesting())
            uninteresting.insert(*itr);
    }

    if (!state.needShadows())
        return true;

    if (!state.needTexture() && !state.mImportantState)
    {
        for (RenderLeaf* leaf : sg->_leaves)
        {
            leaf->_parent = root;
            root->_leaves.push_back(leaf);
        }
        return true;
    }
    return false;
}

bool ShadowsBin::State::needShadows() const
{
    if (!mMaterial)
        return true;
    return materialNeedShadows(mMaterial);
}

void ShadowsBin::sortImplementation()
{
    if (!_stateGraphList.size())
        return;
    StateGraph* root = _stateGraphList[0];
    while (root->_parent)
    {
        root = root->_parent;
        const osg::StateSet* ss = root->getStateSet();
        if (ss->getMode(GL_NORMALIZE) & osg::StateAttribute::ON // that is root stategraph of renderingmanager cpp
           || ss->getAttribute(osg::StateAttribute::VIEWPORT)) // fallback to rendertargets sg just in case
            break;
        if (!root->_parent)
            return;
    }
    root = root->find_or_insert(mStateSet.get());
    root->_leaves.reserve(_stateGraphList.size());
    StateGraphList newList;
    std::unordered_set<StateGraph*> uninteresting;
    for (StateGraph* graph : _stateGraphList)
    {
        if (!cullStateGraph(graph, root, uninteresting))
            newList.push_back(graph);
    }
    if (!root->_leaves.empty())
        newList.push_back(root);
    _stateGraphList = newList;
}

}
