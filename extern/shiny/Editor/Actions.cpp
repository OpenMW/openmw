#include "Actions.hpp"

#include "../Main/Factory.hpp"

namespace sh
{

	void ActionDeleteMaterial::execute()
	{
		sh::Factory::getInstance().destroyMaterialInstance(mName);
	}

	void ActionCloneMaterial::execute()
	{
		sh::MaterialInstance* sourceMaterial = sh::Factory::getInstance().getMaterialInstance(mSourceName);
		std::string sourceMaterialParent = static_cast<sh::MaterialInstance*>(sourceMaterial->getParent())->getName();
		sh::MaterialInstance* material = sh::Factory::getInstance().createMaterialInstance(
					mDestName, sourceMaterialParent);
		sourceMaterial->copyAll(material, sourceMaterial, false);

		material->setSourceFile(sourceMaterial->getSourceFile());
	}

	void ActionSaveAll::execute()
	{
		sh::Factory::getInstance().saveAll();
	}

	void ActionChangeGlobalSetting::execute()
	{
		sh::Factory::getInstance().setGlobalSetting(mName, mNewValue);
	}

	void ActionCreateConfiguration::execute()
	{
		sh::Configuration newConfiguration;
		sh::Factory::getInstance().createConfiguration(mName);
	}

	void ActionDeleteConfiguration::execute()
	{
		sh::Factory::getInstance().destroyConfiguration(mName);
	}

	void ActionChangeConfiguration::execute()
	{
		sh::Configuration* c = sh::Factory::getInstance().getConfiguration(mName);
		c->setProperty(mKey, sh::makeProperty(new sh::StringValue(mValue)));
		sh::Factory::getInstance().notifyConfigurationChanged();
	}

	void ActionDeleteConfigurationProperty::execute()
	{
		sh::Configuration* c = sh::Factory::getInstance().getConfiguration(mName);
		c->deleteProperty(mKey);
		sh::Factory::getInstance().notifyConfigurationChanged();
	}

	void ActionSetMaterialProperty::execute()
	{
		sh::MaterialInstance* m = sh::Factory::getInstance().getMaterialInstance(mName);
		m->setProperty(mKey, sh::makeProperty(mValue));

		sh::Factory::getInstance().notifyConfigurationChanged();
	}

	void ActionDeleteMaterialProperty::execute()
	{
		sh::MaterialInstance* m = sh::Factory::getInstance().getMaterialInstance(mName);
		m->deleteProperty(mKey);

		sh::Factory::getInstance().notifyConfigurationChanged();
	}

	void ActionCreatePass::execute()
	{
		sh::MaterialInstance* m = sh::Factory::getInstance().getMaterialInstance(mName);
		m->createPass();

		sh::Factory::getInstance().notifyConfigurationChanged();
	}

	void ActionDeletePass::execute()
	{
		sh::MaterialInstance* m = sh::Factory::getInstance().getMaterialInstance(mName);
		m->deletePass(mPassIndex);

		sh::Factory::getInstance().notifyConfigurationChanged();
	}

	void ActionSetPassProperty::execute()
	{
		sh::MaterialInstance* m = sh::Factory::getInstance().getMaterialInstance(mName);
		assert (m->getPasses()->size() > mPassIndex);
		m->getPasses()->at(mPassIndex).setProperty (mKey, sh::makeProperty(mValue));

		sh::Factory::getInstance().notifyConfigurationChanged();
	}

	void ActionDeletePassProperty::execute()
	{
		sh::MaterialInstance* m = sh::Factory::getInstance().getMaterialInstance(mName);
		assert (m->getPasses()->size() > mPassIndex);
		m->getPasses()->at(mPassIndex).deleteProperty(mKey);

		sh::Factory::getInstance().notifyConfigurationChanged();
	}

	void ActionSetShaderProperty::execute()
	{
		sh::MaterialInstance* m = sh::Factory::getInstance().getMaterialInstance(mName);
		assert (m->getPasses()->size() > mPassIndex);
		m->getPasses()->at(mPassIndex).mShaderProperties.setProperty (mKey, sh::makeProperty(mValue));

		sh::Factory::getInstance().notifyConfigurationChanged();
	}

	void ActionDeleteShaderProperty::execute()
	{
		sh::MaterialInstance* m = sh::Factory::getInstance().getMaterialInstance(mName);
		assert (m->getPasses()->size() > mPassIndex);
		m->getPasses()->at(mPassIndex).mShaderProperties.deleteProperty (mKey);

		sh::Factory::getInstance().notifyConfigurationChanged();
	}

	void ActionSetTextureProperty::execute()
	{
		sh::MaterialInstance* m = sh::Factory::getInstance().getMaterialInstance(mName);
		assert (m->getPasses()->size() > mPassIndex);
		assert (m->getPasses()->at(mPassIndex).mTexUnits.size() > mTextureIndex);
		m->getPasses()->at(mPassIndex).mTexUnits.at(mTextureIndex).setProperty(mKey, sh::makeProperty(mValue));

		sh::Factory::getInstance().notifyConfigurationChanged();
	}

	void ActionDeleteTextureProperty::execute()
	{
		sh::MaterialInstance* m = sh::Factory::getInstance().getMaterialInstance(mName);
		assert (m->getPasses()->size() > mPassIndex);
		assert (m->getPasses()->at(mPassIndex).mTexUnits.size() > mTextureIndex);
		m->getPasses()->at(mPassIndex).mTexUnits.at(mTextureIndex).deleteProperty(mKey);

		sh::Factory::getInstance().notifyConfigurationChanged();
	}

	void ActionCreateTextureUnit::execute()
	{
		sh::MaterialInstance* m = sh::Factory::getInstance().getMaterialInstance(mName);
		assert (m->getPasses()->size() > mPassIndex);
		m->getPasses()->at(mPassIndex).createTextureUnit(mTexUnitName);

		sh::Factory::getInstance().notifyConfigurationChanged();
	}

	void ActionDeleteTextureUnit::execute()
	{
		sh::MaterialInstance* m = sh::Factory::getInstance().getMaterialInstance(mName);
		assert (m->getPasses()->size() > mPassIndex);
		assert (m->getPasses()->at(mPassIndex).mTexUnits.size() > mTextureIndex);

		m->getPasses()->at(mPassIndex).mTexUnits.erase(m->getPasses()->at(mPassIndex).mTexUnits.begin() + mTextureIndex);

		sh::Factory::getInstance().notifyConfigurationChanged();
	}

	void ActionMoveTextureUnit::execute()
	{
		sh::MaterialInstance* m = sh::Factory::getInstance().getMaterialInstance(mName);
		assert (m->getPasses()->size() > mPassIndex);
		assert (m->getPasses()->at(mPassIndex).mTexUnits.size() > mTextureIndex);
		if (!mMoveUp)
			assert (m->getPasses()->at(mPassIndex).mTexUnits.size() > mTextureIndex+1);

		std::vector<MaterialInstanceTextureUnit> textures = m->getPasses()->at(mPassIndex).mTexUnits;
		if (mMoveUp)
			std::swap(textures[mTextureIndex-1], textures[mTextureIndex]);
		else
			std::swap(textures[mTextureIndex+1], textures[mTextureIndex]);
		m->getPasses()->at(mPassIndex).mTexUnits = textures;

		sh::Factory::getInstance().notifyConfigurationChanged();
	}

	void ActionChangeTextureUnitName::execute()
	{
		sh::MaterialInstance* m = sh::Factory::getInstance().getMaterialInstance(mName);
		assert (m->getPasses()->size() > mPassIndex);
		assert (m->getPasses()->at(mPassIndex).mTexUnits.size() > mTextureIndex);

		m->getPasses()->at(mPassIndex).mTexUnits[mTextureIndex].setName(mTexUnitName);

		sh::Factory::getInstance().notifyConfigurationChanged();
	}
}
