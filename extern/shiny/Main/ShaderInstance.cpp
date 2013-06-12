#include "ShaderInstance.hpp"

#include <stdexcept>
#include <iostream>
#include <fstream>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>

#include <boost/filesystem.hpp>

#include "Preprocessor.hpp"
#include "Factory.hpp"
#include "ShaderSet.hpp"

namespace
{
	std::string convertLang (sh::Language lang)
	{
		if (lang == sh::Language_CG)
			return "SH_CG";
		else if (lang == sh::Language_HLSL)
			return "SH_HLSL";
		else if (lang == sh::Language_GLSL)
			return "SH_GLSL";
		else if (lang == sh::Language_GLSLES)
			return "SH_GLSLES";
		throw std::runtime_error("invalid language");
	}

	char getComponent(int num)
	{
		if (num == 0)
			return 'x';
		else if (num == 1)
			return 'y';
		else if (num == 2)
			return 'z';
		else if (num == 3)
			return 'w';
		else
			throw std::runtime_error("invalid component");
	}

	std::string getFloat(sh::Language lang, int num_components)
	{
		if (lang == sh::Language_CG || lang == sh::Language_HLSL)
			return (num_components == 1) ? "float" : "float" + boost::lexical_cast<std::string>(num_components);
		else
			return (num_components == 1) ? "float" : "vec" + boost::lexical_cast<std::string>(num_components);
	}

	bool isCmd (const std::string& source, size_t pos, const std::string& cmd)
	{
		return (source.size() >= pos + cmd.size() && source.substr(pos, cmd.size()) == cmd);
	}

	void writeDebugFile (const std::string& content, const std::string& filename)
	{
		boost::filesystem::path full_path(boost::filesystem::current_path());
		std::ofstream of ((full_path / filename ).string().c_str() , std::ios_base::out);
		of.write(content.c_str(), content.size());
		of.close();
	}
}

namespace sh
{
	std::string Passthrough::expand_assign(std::string toAssign)
	{
		std::string res;

		int i = 0;
		int current_passthrough = passthrough_number;
		int current_component_left = component_start;
		int current_component_right = 0;
		int components_left = num_components;
		int components_at_once;
		while (i < num_components)
		{
			if (components_left + current_component_left <= 4)
				components_at_once = components_left;
			else
				components_at_once = 4 - current_component_left;

			std::string componentStr = ".";
			for (int j = 0; j < components_at_once; ++j)
				componentStr += getComponent(j + current_component_left);
			std::string componentStr2 = ".";
			for (int j = 0; j < components_at_once; ++j)
				componentStr2 += getComponent(j + current_component_right);
			if (num_components == 1)
			{
				componentStr2 = "";
			}
			res += "passthrough" + boost::lexical_cast<std::string>(current_passthrough) + componentStr + " = " + toAssign + componentStr2;

			current_component_left += components_at_once;
			current_component_right += components_at_once;
			components_left -= components_at_once;

			i += components_at_once;

			if (components_left == 0)
			{
				// finished
				return res;
			}
			else
			{
				// add semicolon to every instruction but the last
				res += "; ";
			}

			if (current_component_left == 4)
			{
				current_passthrough++;
				current_component_left = 0;
			}
		}
		throw std::runtime_error("expand_assign error"); // this should never happen, but gets us rid of the "control reaches end of non-void function" warning
	}

	std::string Passthrough::expand_receive()
	{
		std::string res;

		res += getFloat(lang, num_components) + "(";

		int i = 0;
		int current_passthrough = passthrough_number;
		int current_component = component_start;
		int components_left = num_components;
		while (i < num_components)
		{
			int components_at_once = std::min(components_left, 4 - current_component);

			std::string componentStr;
			for (int j = 0; j < components_at_once; ++j)
				componentStr += getComponent(j + current_component);

			res += "passthrough" + boost::lexical_cast<std::string>(current_passthrough) + "." + componentStr;

			current_component += components_at_once;

			components_left -= components_at_once;

			i += components_at_once;

			if (components_left == 0)
			{
				// finished
				return res + ")";
;
			}
			else
			{
				// add comma to every variable but the last
				res += ", ";
			}

			if (current_component == 4)
			{
				current_passthrough++;
				current_component = 0;
			}
		}

		throw std::runtime_error("expand_receive error"); // this should never happen, but gets us rid of the "control reaches end of non-void function" warning
	}

	// ------------------------------------------------------------------------------

	void ShaderInstance::parse (std::string& source, PropertySetGet* properties)
	{
		size_t pos = 0;
		while (true)
		{
			pos =  source.find("@", pos);
			if (pos == std::string::npos)
				break;

			if (isCmd(source, pos, "@shProperty"))
			{
				std::vector<std::string> args = extractMacroArguments (pos, source);

				size_t start = source.find("(", pos);
				size_t end = source.find(")", pos);
				std::string cmd = source.substr(pos+1, start-(pos+1));

				std::string replaceValue;
				if (cmd == "shPropertyBool")
				{
					std::string propertyName = args[0];
					PropertyValuePtr value = properties->getProperty(propertyName);
					bool val = retrieveValue<BooleanValue>(value, properties->getContext()).get();
					replaceValue = val ? "1" : "0";
				}
				else if (cmd == "shPropertyString")
				{
					std::string propertyName = args[0];
					PropertyValuePtr value = properties->getProperty(propertyName);
					replaceValue = retrieveValue<StringValue>(value, properties->getContext()).get();
				}
				else if (cmd == "shPropertyEqual")
				{
					std::string propertyName = args[0];
					std::string comparedAgainst = args[1];
					std::string value = retrieveValue<StringValue>(properties->getProperty(propertyName), properties->getContext()).get();
					replaceValue = (value == comparedAgainst) ? "1" : "0";
				}
				else if (isCmd(source, pos, "@shPropertyHasValue"))
				{
					assert(args.size() == 1);
					std::string propertyName = args[0];
					PropertyValuePtr value = properties->getProperty(propertyName);
					std::string val = retrieveValue<StringValue>(value, properties->getContext()).get();
					replaceValue = (val.empty() ? "0" : "1");
				}
				else
					throw std::runtime_error ("unknown command \"" + cmd + "\"");
				source.replace(pos, (end+1)-pos, replaceValue);
			}
			else if (isCmd(source, pos, "@shGlobalSetting"))
			{
				std::vector<std::string> args = extractMacroArguments (pos, source);

				std::string cmd = source.substr(pos+1,  source.find("(", pos)-(pos+1));
				std::string replaceValue;
				if (cmd == "shGlobalSettingBool")
				{
					std::string settingName = args[0];
					std::string value = retrieveValue<StringValue>(mParent->getCurrentGlobalSettings()->getProperty(settingName), NULL).get();
					replaceValue = (value == "true" || value == "1") ? "1" : "0";
				}
				else if (cmd == "shGlobalSettingEqual")
				{
					std::string settingName = args[0];
					std::string comparedAgainst = args[1];
					std::string value = retrieveValue<StringValue>(mParent->getCurrentGlobalSettings()->getProperty(settingName), NULL).get();
					replaceValue = (value == comparedAgainst) ? "1" : "0";
				}
				else if (cmd == "shGlobalSettingString")
				{
					std::string settingName = args[0];
					replaceValue = retrieveValue<StringValue>(mParent->getCurrentGlobalSettings()->getProperty(settingName), NULL).get();
				}
				else
					throw std::runtime_error ("unknown command \"" + cmd + "\"");

				source.replace(pos, (source.find(")", pos)+1)-pos, replaceValue);
			}
			else if (isCmd(source, pos, "@shForeach"))
			{

				assert(source.find("@shEndForeach", pos) != std::string::npos);
				size_t block_end = source.find("@shEndForeach", pos);

				// get the argument for parsing
				size_t start = source.find("(", pos);
				size_t end = start;
				int brace_depth = 1;
				while (brace_depth > 0)
				{
					++end;
					if (source[end] == '(')
						++brace_depth;
					else if (source[end] == ')')
						--brace_depth;
				}
				std::string arg = source.substr(start+1, end-(start+1));
				parse(arg, properties);

				int num = boost::lexical_cast<int>(arg);

				// get the content of the inner block
				std::string content = source.substr(end+1, block_end - (end+1));

				// replace both outer and inner block with content of inner block num times
				std::string replaceStr;
				for (int i=0; i<num; ++i)
				{
					// replace @shIterator with the current iteration
					std::string addStr = content;

					while (true)
					{
						size_t pos2 = addStr.find("@shIterator");
						if (pos2 == std::string::npos)
							break;

						// optional offset parameter.
						size_t openBracePos = pos2 + std::string("@shIterator").length();
						if (addStr[openBracePos] == '(')
						{
							// get the argument for parsing
							size_t _start = openBracePos;
							size_t _end = _start;
							int _brace_depth = 1;
							while (_brace_depth > 0)
							{
								++_end;
								if (addStr[_end] == '(')
									++_brace_depth;
								else if (addStr[_end] == ')')
									--_brace_depth;
							}
							std::string arg = addStr.substr(_start+1, _end-(_start+1));
							parse(arg, properties);

							int offset = boost::lexical_cast<int> (arg);
							addStr.replace(pos2, (_end+1)-pos2, boost::lexical_cast<std::string>(i+offset));
						}
						else
						{
							addStr.replace(pos2, std::string("@shIterator").length(), boost::lexical_cast<std::string>(i));
						}
					}

					replaceStr += addStr;
				}
				source.replace(pos, (block_end+std::string("@shEndForeach").length())-pos, replaceStr);
			}
			else if (source.size() > pos+1)
				++pos; // skip
		}

	}

	ShaderInstance::ShaderInstance (ShaderSet* parent, const std::string& name, PropertySetGet* properties)
		: mName(name)
		, mParent(parent)
		, mSupported(true)
		, mCurrentPassthrough(0)
		, mCurrentComponent(0)
	{
		std::string source = mParent->getSource();
		int type = mParent->getType();
		std::string basePath = mParent->getBasePath();
		size_t pos;

		bool readCache = Factory::getInstance ().getReadSourceCache () && boost::filesystem::exists(
					Factory::getInstance ().getCacheFolder () + "/" + mName);
		bool writeCache = Factory::getInstance ().getWriteSourceCache ();


		if (readCache)
		{
			std::ifstream ifs( std::string(Factory::getInstance ().getCacheFolder () + "/" + mName).c_str() );
			std::stringstream ss;
			ss << ifs.rdbuf();
			source = ss.str();
		}
		else
		{
			std::vector<std::string> definitions;

			if (mParent->getType() == GPT_Vertex)
				definitions.push_back("SH_VERTEX_SHADER");
			else
				definitions.push_back("SH_FRAGMENT_SHADER");
			definitions.push_back(convertLang(Factory::getInstance().getCurrentLanguage()));

			parse(source, properties);

			if (Factory::getInstance ().getShaderDebugOutputEnabled ())
				writeDebugFile(source, name + ".pre");

			// why do we need our own preprocessor? there are several custom commands available in the shader files
			// (for example for binding uniforms to properties or auto constants) - more below. it is important that these
			// commands are _only executed if the specific code path actually "survives" the compilation.
			// thus, we run the code through a preprocessor first to remove the parts that are unused because of
			// unmet #if conditions (or other preprocessor directives).
			source = Preprocessor::preprocess(source, basePath, definitions, name);

			// parse counter
			std::map<int, int> counters;
			while (true)
			{
				pos = source.find("@shCounter");
				if (pos == std::string::npos)
					break;

				size_t end = source.find(")", pos);

				std::vector<std::string> args = extractMacroArguments (pos, source);
				assert(args.size());

				int index = boost::lexical_cast<int>(args[0]);

				if (counters.find(index) == counters.end())
					counters[index] = 0;

				source.replace(pos, (end+1)-pos, boost::lexical_cast<std::string>(counters[index]++));
			}

			// parse passthrough declarations
			while (true)
			{
				pos = source.find("@shAllocatePassthrough");
				if (pos == std::string::npos)
					break;

				if (mCurrentPassthrough > 7)
					throw std::runtime_error ("too many passthrough's requested (max 8)");

				std::vector<std::string> args = extractMacroArguments (pos, source);
				assert(args.size() == 2);

				size_t end = source.find(")", pos);

				Passthrough passthrough;

				passthrough.num_components = boost::lexical_cast<int>(args[0]);
				assert (passthrough.num_components != 0);

				std::string passthroughName = args[1];
				passthrough.lang = Factory::getInstance().getCurrentLanguage ();
				passthrough.component_start = mCurrentComponent;
				passthrough.passthrough_number = mCurrentPassthrough;

				mPassthroughMap[passthroughName] = passthrough;

				mCurrentComponent += passthrough.num_components;
				if (mCurrentComponent > 3)
				{
					mCurrentComponent -= 4;
					++mCurrentPassthrough;
				}

				source.erase(pos, (end+1)-pos);
			}

			// passthrough assign
			while (true)
			{
				pos = source.find("@shPassthroughAssign");
				if (pos == std::string::npos)
					break;

				std::vector<std::string> args = extractMacroArguments (pos, source);
				assert(args.size() == 2);

				size_t end = source.find(")", pos);

				std::string passthroughName = args[0];
				std::string assignTo = args[1];

				assert(mPassthroughMap.find(passthroughName) != mPassthroughMap.end());
				Passthrough& p = mPassthroughMap[passthroughName];

				source.replace(pos, (end+1)-pos, p.expand_assign(assignTo));
			}

			// passthrough receive
			while (true)
			{
				pos = source.find("@shPassthroughReceive");
				if (pos == std::string::npos)
					break;

				std::vector<std::string> args = extractMacroArguments (pos, source);
				assert(args.size() == 1);

				size_t end = source.find(")", pos);
				std::string passthroughName = args[0];

				assert(mPassthroughMap.find(passthroughName) != mPassthroughMap.end());
				Passthrough& p = mPassthroughMap[passthroughName];

				source.replace(pos, (end+1)-pos, p.expand_receive());
			}

			// passthrough vertex outputs
			while (true)
			{
				pos = source.find("@shPassthroughVertexOutputs");
				if (pos == std::string::npos)
					break;

				std::string result;
				for (int i = 0; i < mCurrentPassthrough+1; ++i)
				{
					// not using newlines here, otherwise the line numbers reported by compiler would be messed up..
					if (Factory::getInstance().getCurrentLanguage () == Language_CG || Factory::getInstance().getCurrentLanguage () == Language_HLSL)
						result += ", out float4 passthrough" + boost::lexical_cast<std::string>(i) + " : TEXCOORD" + boost::lexical_cast<std::string>(i);

					/*
					else
						result += "out vec4 passthrough" + boost::lexical_cast<std::string>(i) + "; ";
						*/
					else
						result += "varying vec4 passthrough" + boost::lexical_cast<std::string>(i) + "; ";
				}

				source.replace(pos, std::string("@shPassthroughVertexOutputs").length(), result);
			}

			// passthrough fragment inputs
			while (true)
			{
				pos = source.find("@shPassthroughFragmentInputs");
				if (pos == std::string::npos)
					break;

				std::string result;
				for (int i = 0; i < mCurrentPassthrough+1; ++i)
				{
					// not using newlines here, otherwise the line numbers reported by compiler would be messed up..
					if (Factory::getInstance().getCurrentLanguage () == Language_CG || Factory::getInstance().getCurrentLanguage () == Language_HLSL)
						result += ", in float4 passthrough" + boost::lexical_cast<std::string>(i) + " : TEXCOORD" + boost::lexical_cast<std::string>(i);
					/*
					else
						result += "in vec4 passthrough" + boost::lexical_cast<std::string>(i) + "; ";
						*/
					else
						result += "varying vec4 passthrough" + boost::lexical_cast<std::string>(i) + "; ";
				}

				source.replace(pos, std::string("@shPassthroughFragmentInputs").length(), result);
			}
		}

		// save to cache _here_ - we want to preserve some macros
		if (writeCache && !readCache)
		{
			std::ofstream of (std::string(Factory::getInstance ().getCacheFolder () + "/" + mName).c_str(), std::ios_base::out);
			of.write(source.c_str(), source.size());
			of.close();
		}


		// parse shared parameters
		while (true)
		{
			pos = source.find("@shSharedParameter");
			if (pos == std::string::npos)
				break;

			std::vector<std::string> args = extractMacroArguments (pos, source);
			assert(args.size());

			size_t end = source.find(")", pos);

			mSharedParameters.push_back(args[0]);

			source.erase(pos, (end+1)-pos);
		}

		// parse auto constants
		typedef std::map< std::string, std::pair<std::string, std::string> > AutoConstantMap;
		AutoConstantMap autoConstants;
		while (true)
		{
			pos = source.find("@shAutoConstant");
			if (pos == std::string::npos)
				break;

			std::vector<std::string> args = extractMacroArguments (pos, source);
			assert(args.size() >= 2);

			size_t end = source.find(")", pos);

			std::string autoConstantName, uniformName;
			std::string extraData;

			uniformName = args[0];
			autoConstantName = args[1];
			if (args.size() > 2)
				extraData = args[2];

			autoConstants[uniformName] = std::make_pair(autoConstantName, extraData);

			source.erase(pos, (end+1)-pos);
		}

		// parse uniform properties
		while (true)
		{
			pos = source.find("@shUniformProperty");
			if (pos == std::string::npos)
				break;

			std::vector<std::string> args = extractMacroArguments (pos, source);
			assert(args.size() == 2);

			size_t start = source.find("(", pos);
			size_t end = source.find(")", pos);
			std::string cmd = source.substr(pos, start-pos);

			ValueType vt;
			if (cmd == "@shUniformProperty4f")
				vt = VT_Vector4;
			else if (cmd == "@shUniformProperty3f")
				vt = VT_Vector3;
			else if (cmd == "@shUniformProperty2f")
				vt = VT_Vector2;
			else if (cmd == "@shUniformProperty1f")
				vt = VT_Float;
			else if (cmd == "@shUniformPropertyInt")
				vt = VT_Int;
			else
				throw std::runtime_error ("unsupported command \"" + cmd + "\"");


			std::string propertyName, uniformName;
			uniformName = args[0];
			propertyName = args[1];
			mUniformProperties[uniformName] = std::make_pair(propertyName, vt);

			source.erase(pos, (end+1)-pos);
		}

		// parse texture samplers used
		while (true)
		{
			pos = source.find("@shUseSampler");
			if (pos == std::string::npos)
				break;

			size_t end = source.find(")", pos);

			mUsedSamplers.push_back(extractMacroArguments (pos, source)[0]);
			source.erase(pos, (end+1)-pos);
		}

		// convert any left-over @'s to #
		boost::algorithm::replace_all(source, "@", "#");

		Platform* platform = Factory::getInstance().getPlatform();

		std::string profile;
		if (Factory::getInstance ().getCurrentLanguage () == Language_CG)
			profile = mParent->getCgProfile ();
		else if (Factory::getInstance ().getCurrentLanguage () == Language_HLSL)
			profile = mParent->getHlslProfile ();


		if (type == GPT_Vertex)
			mProgram = boost::shared_ptr<GpuProgram>(platform->createGpuProgram(GPT_Vertex, "", mName, profile, source, Factory::getInstance().getCurrentLanguage()));
		else if (type == GPT_Fragment)
			mProgram = boost::shared_ptr<GpuProgram>(platform->createGpuProgram(GPT_Fragment, "", mName, profile, source, Factory::getInstance().getCurrentLanguage()));


		if (Factory::getInstance ().getShaderDebugOutputEnabled ())
			writeDebugFile(source, name);

		if (!mProgram->getSupported())
		{
			std::cerr << "        Full source code below: \n" << source << std::endl;
			mSupported = false;
			return;
		}

		// set auto constants
		for (AutoConstantMap::iterator it = autoConstants.begin(); it != autoConstants.end(); ++it)
		{
			mProgram->setAutoConstant(it->first, it->second.first, it->second.second);
		}
	}

	std::string ShaderInstance::getName ()
	{
		return mName;
	}

	bool ShaderInstance::getSupported () const
	{
		return mSupported;
	}

	std::vector<std::string> ShaderInstance::getUsedSamplers()
	{
		return mUsedSamplers;
	}

	void ShaderInstance::setUniformParameters (boost::shared_ptr<Pass> pass, PropertySetGet* properties)
	{
		for (UniformMap::iterator it = mUniformProperties.begin(); it != mUniformProperties.end(); ++it)
		{
			pass->setGpuConstant(mParent->getType(), it->first, it->second.second, properties->getProperty(it->second.first), properties->getContext());
		}
	}

	std::vector<std::string> ShaderInstance::extractMacroArguments (size_t pos, const std::string& source)
	{
		size_t start = source.find("(", pos);
		size_t end = source.find(")", pos);
		std::string args = source.substr(start+1, end-(start+1));
		std::vector<std::string> results;
		boost::algorithm::split(results, args, boost::is_any_of(","));
		std::for_each(results.begin(), results.end(),
			boost::bind(&boost::trim<std::string>,
			_1, std::locale() ));
		return results;
	}
}
