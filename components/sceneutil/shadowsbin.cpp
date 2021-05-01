#include "shadowsbin.hpp"
#include <unordered_set>
#include <osg/StateSet>
#include <osg/AlphaFunc>
#include <osg/Material>
#include <osg/Program>
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
        unsigned int flags = mf->second;
        bool newValue = flags & osg::StateAttribute::ON;
        accumulateState(currentValue, newValue, isOverride, flags);
    }

    inline bool materialNeedShadows(osg::Material* m)
    {
        // I'm pretty sure this needs to check the colour mode - vertex colours might override this value.
        return m->getDiffuse(osg::Material::FRONT).a() > 0.5;
    }
}

namespace SceneUtil
{

std::array<osg::ref_ptr<osg::Program>, GL_ALWAYS - GL_NEVER + 1> ShadowsBin::sCastingPrograms = {
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
};

ShadowsBin::ShadowsBin()
{
    mNoTestStateSet = new osg::StateSet;
    mNoTestStateSet->addUniform(new osg::Uniform("useDiffuseMapForShadowAlpha", false));
    mNoTestStateSet->addUniform(new osg::Uniform("alphaTestShadows", false));

    mShaderAlphaTestStateSet = new osg::StateSet;
    mShaderAlphaTestStateSet->addUniform(new osg::Uniform("alphaTestShadows", true));
    mShaderAlphaTestStateSet->setMode(GL_BLEND, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED | osg::StateAttribute::OVERRIDE);

    for (size_t i = 0; i < sCastingPrograms.size(); ++i)
    {
        mAlphaFuncShaders[i] = new osg::StateSet;
        mAlphaFuncShaders[i]->setAttribute(sCastingPrograms[i], osg::StateAttribute::ON | osg::StateAttribute::PROTECTED | osg::StateAttribute::OVERRIDE);
    }
}

StateGraph* ShadowsBin::cullStateGraph(StateGraph* sg, StateGraph* root, std::unordered_set<StateGraph*>& uninterestingCache, bool cullFaceOverridden)
{
    std::vector<StateGraph*> return_path;
    State state;
    StateGraph* sg_new = sg;
    do
    {
        if (uninterestingCache.find(sg_new) != uninterestingCache.end())
            break;
        return_path.push_back(sg_new);
        sg_new = sg_new->_parent;
    } while (sg_new && sg_new != root);

    for(auto itr=return_path.rbegin(); itr!=return_path.rend(); ++itr)
    {
        const osg::StateSet* ss = (*itr)->getStateSet();
        if (!ss)
            continue;

        accumulateModeState(ss, state.mAlphaBlend, state.mAlphaBlendOverride, GL_BLEND);

        const osg::StateSet::AttributeList& attributes = ss->getAttributeList();
        osg::StateSet::AttributeList::const_iterator found = attributes.find(std::make_pair(osg::StateAttribute::MATERIAL, 0));
        if (found != attributes.end())
        {
            const osg::StateSet::RefAttributePair& rap = found->second;
            accumulateState(state.mMaterial, static_cast<osg::Material*>(rap.first.get()), state.mMaterialOverride, rap.second);
            if (state.mMaterial && !materialNeedShadows(state.mMaterial))
                state.mMaterial = nullptr;
        }

        found = attributes.find(std::make_pair(osg::StateAttribute::ALPHAFUNC, 0));
        if (found != attributes.end())
        {
            // As force shaders is on, we know this is really a RemovedAlphaFunc
            const osg::StateSet::RefAttributePair& rap = found->second;
            accumulateState(state.mAlphaFunc, static_cast<osg::AlphaFunc*>(rap.first.get()), state.mAlphaFuncOverride, rap.second);
        }

        if (!cullFaceOverridden)
        {
            // osg::FrontFace specifies triangle winding, not front-face culling. We can't safely reparent anything under it unless GL_CULL_FACE is off or we flip face culling.
            found = attributes.find(std::make_pair(osg::StateAttribute::FRONTFACE, 0));
            if (found != attributes.end())
                state.mImportantState = true;
        }

        if ((*itr) != sg && !state.interesting())
            uninterestingCache.insert(*itr);
    }

    if (!state.needShadows())
        return nullptr;

    if (!state.needTexture() && !state.mImportantState)
    {
        for (RenderLeaf* leaf : sg->_leaves)
        {
            leaf->_parent = root;
            root->_leaves.push_back(leaf);
        }
        return nullptr;
    }

    if (state.mAlphaBlend)
    {
        sg_new = sg->find_or_insert(mShaderAlphaTestStateSet);
        sg_new->_leaves = std::move(sg->_leaves);
        for (RenderLeaf* leaf : sg_new->_leaves)
            leaf->_parent = sg_new;
        sg = sg_new;
    }

    // GL_ALWAYS is set by default by mwshadowtechnique
    if (state.mAlphaFunc && state.mAlphaFunc->getFunction() != GL_ALWAYS)
    {
        sg_new = sg->find_or_insert(mAlphaFuncShaders[state.mAlphaFunc->getFunction() - GL_NEVER]);
        sg_new->_leaves = std::move(sg->_leaves);
        for (RenderLeaf* leaf : sg_new->_leaves)
            leaf->_parent = sg_new;
        sg = sg_new;
    }

    return sg;
}

void ShadowsBin::addPrototype(const std::string & name, const std::array<osg::ref_ptr<osg::Program>, GL_ALWAYS - GL_NEVER + 1>& castingPrograms)
{
    sCastingPrograms = castingPrograms;
    osg::ref_ptr<osgUtil::RenderBin> bin(new ShadowsBin);
    osgUtil::RenderBin::addRenderBinPrototype(name, bin);
}

inline bool ShadowsBin::State::needTexture() const
{
    return mAlphaBlend || (mAlphaFunc && mAlphaFunc->getFunction() != GL_ALWAYS);
}

bool ShadowsBin::State::needShadows() const
{
    if (mAlphaFunc && mAlphaFunc->getFunction() == GL_NEVER)
        return false;
    // other alpha func + material combinations might be skippable
    if (mAlphaBlend && mMaterial)
        return materialNeedShadows(mMaterial);
    return true;
}

void ShadowsBin::sortImplementation()
{
    // The cull visitor contains a stategraph.
    // When a stateset is pushed, it's added/found as a child of the current stategraph node, then that node becomes the new current stategraph node.
    // When a drawable is added, the current stategraph node is added to the current renderbin (if it's not there already) and the drawable is added as a renderleaf to the stategraph
    // This means our list only contains stategraph nodes with directly-attached renderleaves, but they might have parents with more state set that needs to be considered.
    if (!_stateGraphList.size())
        return;
    StateGraph* root = _stateGraphList[0];
    while (root->_parent)
    {
        root = root->_parent;
        const osg::StateSet* ss = root->getStateSet();
        if (ss->getMode(GL_NORMALIZE) & osg::StateAttribute::ON // that is root stategraph of renderingmanager cpp
           || ss->getAttribute(osg::StateAttribute::VIEWPORT)) // fallback to rendertarget's sg just in case
            break;
        if (!root->_parent)
            return;
    }
    StateGraph* noTestRoot = root->find_or_insert(mNoTestStateSet.get());
    // noTestRoot is now a stategraph with useDiffuseMapForShadowAlpha disabled but minimal other state

    bool cullFaceOverridden = false;
    while (root->_parent)
    {
        root = root->_parent;
        if (!root->getStateSet())
            continue;
        unsigned int cullFaceFlags = root->getStateSet()->getMode(GL_CULL_FACE);
        if (cullFaceFlags & osg::StateAttribute::OVERRIDE && !(cullFaceFlags & osg::StateAttribute::ON))
        {
            cullFaceOverridden = true;
            break;
        }
    }

    noTestRoot->_leaves.reserve(_stateGraphList.size());
    StateGraphList newList;
    std::unordered_set<StateGraph*> uninterestingCache;
    for (StateGraph* graph : _stateGraphList)
    {
        // Render leaves which shouldn't use the diffuse map for shadow alpha but do cast shadows become children of root, so graph is now empty. Don't add to newList.
        // Graphs containing just render leaves which don't cast shadows are discarded. Don't add to newList.
        // Graphs containing other leaves need to be in newList.
        StateGraph* graphToAdd = cullStateGraph(graph, noTestRoot, uninterestingCache, cullFaceOverridden);
        if (graphToAdd)
            newList.push_back(graphToAdd);
    }
    if (!noTestRoot->_leaves.empty())
        newList.push_back(noTestRoot);
    _stateGraphList = newList;
}

}
