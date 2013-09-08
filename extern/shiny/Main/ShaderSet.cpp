#include "ShaderSet.hpp"

#include <fstream>
#include <sstream>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/functional/hash.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include "Factory.hpp"

namespace sh
{
	ShaderSet::ShaderSet (const std::string& type, const std::string& cgProfile, const std::string& hlslProfile, const std::string& sourceFile, const std::string& basePath,
						  const std::string& name, PropertySetGet* globalSettingsPtr)
		: mBasePath(basePath)
		, mName(name)
		, mCgProfile(cgProfile)
		, mHlslProfile(hlslProfile)
	{
		if (type == "vertex")
			mType = GPT_Vertex;
		else // if (type == "fragment")
			mType = GPT_Fragment;

		std::ifstream stream(sourceFile.c_str(), std::ifstream::in);
		std::stringstream buffer;

		boost::filesystem::path p (sourceFile);
		p = p.branch_path();
		mBasePath = p.string();

		buffer << stream.rdbuf();
		stream.close();
		mSource = buffer.str();
		parse();
	}

	ShaderSet::~ShaderSet()
	{
		for (ShaderInstanceMap::iterator it = mInstances.begin(); it != mInstances.end(); ++it)
		{
			sh::Factory::getInstance().getPlatform()->destroyGpuProgram(it->second.getName());
		}
	}

	void ShaderSet::parse()
	{
		std::string currentToken;
		bool tokenIsRecognized = false;
		bool isInBraces = false;
		for (std::string::const_iterator it = mSource.begin(); it != mSource.end(); ++it)
		{
			char c = *it;
			if (((c == ' ') && !isInBraces) || (c == '\n') ||
					(   ((c == '(') || (c == ')'))
						  && !tokenIsRecognized))
			{
				if (tokenIsRecognized)
				{
					if (boost::starts_with(currentToken, "@shGlobalSetting"))
					{
						assert ((currentToken.find('(') != std::string::npos) && (currentToken.find(')') != std::string::npos));
						size_t start = currentToken.find('(')+1;
						mGlobalSettings.push_back(currentToken.substr(start, currentToken.find(')')-start));
					}
					else if (boost::starts_with(currentToken, "@shPropertyHasValue"))
					{
						assert ((currentToken.find('(') != std::string::npos) && (currentToken.find(')') != std::string::npos));
						size_t start = currentToken.find('(')+1;
						mPropertiesToExist.push_back(currentToken.substr(start, currentToken.find(')')-start));
					}
					else if (boost::starts_with(currentToken, "@shPropertyEqual"))
					{
						assert ((currentToken.find('(') != std::string::npos) && (currentToken.find(')') != std::string::npos)
								&& (currentToken.find(',') != std::string::npos));
						size_t start = currentToken.find('(')+1;
						size_t end = currentToken.find(',');
						mProperties.push_back(currentToken.substr(start, end-start));
					}
					else if (boost::starts_with(currentToken, "@shProperty"))
					{
						assert ((currentToken.find('(') != std::string::npos) && (currentToken.find(')') != std::string::npos));
						size_t start = currentToken.find('(')+1;
						std::string propertyName = currentToken.substr(start, currentToken.find(')')-start);
						// if the property name is constructed dynamically (e.g. through an iterator) then there is nothing we can do
						if (propertyName.find("@") == std::string::npos)
							mProperties.push_back(propertyName);
					}
				}

				currentToken = "";
			}
			else
			{
				if (currentToken == "")
				{
					if (c == '@')
						tokenIsRecognized = true;
					else
						tokenIsRecognized = false;
				}
				else
				{
					if (c == '@')
					{
						// ouch, there are nested macros
						// ( for example @shForeach(@shPropertyString(foobar)) )
						currentToken = "";
					}
				}

				if (c == '(' && tokenIsRecognized)
					isInBraces = true;
				else if (c == ')' && tokenIsRecognized)
					isInBraces = false;

				currentToken += c;

			}
		}
	}

	ShaderInstance* ShaderSet::getInstance (PropertySetGet* properties)
	{
		size_t h = buildHash (properties);
		if (std::find(mFailedToCompile.begin(), mFailedToCompile.end(), h) != mFailedToCompile.end())
			return NULL;
		if (mInstances.find(h) == mInstances.end())
		{
			ShaderInstance newInstance(this, mName + "_" + boost::lexical_cast<std::string>(h), properties);
			if (!newInstance.getSupported())
			{
				mFailedToCompile.push_back(h);
				return NULL;
			}
			mInstances.insert(std::make_pair(h, newInstance));
		}
		return &mInstances.find(h)->second;
	}

	size_t ShaderSet::buildHash (PropertySetGet* properties)
	{
		size_t seed = 0;
		PropertySetGet* currentGlobalSettings = getCurrentGlobalSettings ();

		for (std::vector<std::string>::iterator it = mProperties.begin(); it != mProperties.end(); ++it)
		{
			std::string v = retrieveValue<StringValue>(properties->getProperty(*it), properties->getContext()).get();
			boost::hash_combine(seed, v);
		}
		for (std::vector <std::string>::iterator it = mGlobalSettings.begin(); it != mGlobalSettings.end(); ++it)
		{
			boost::hash_combine(seed, retrieveValue<StringValue>(currentGlobalSettings->getProperty(*it), NULL).get());
		}
		for (std::vector<std::string>::iterator it = mPropertiesToExist.begin(); it != mPropertiesToExist.end(); ++it)
		{
			std::string v = retrieveValue<StringValue>(properties->getProperty(*it), properties->getContext()).get();
			boost::hash_combine(seed, static_cast<bool>(v != ""));
		}
		boost::hash_combine(seed, static_cast<int>(Factory::getInstance().getCurrentLanguage()));
		return seed;
	}

	PropertySetGet* ShaderSet::getCurrentGlobalSettings() const
	{
		return Factory::getInstance ().getCurrentGlobalSettings ();
	}

	std::string ShaderSet::getBasePath() const
	{
		return mBasePath;
	}

	std::string ShaderSet::getSource() const
	{
		return mSource;
	}

	std::string ShaderSet::getCgProfile() const
	{
		return mCgProfile;
	}

	std::string ShaderSet::getHlslProfile() const
	{
		return mHlslProfile;
	}

	int ShaderSet::getType() const
	{
		return mType;
	}
}
