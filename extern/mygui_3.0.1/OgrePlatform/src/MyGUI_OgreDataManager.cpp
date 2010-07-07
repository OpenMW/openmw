/*!
	@file
	@author		Albert Semenov
	@date		05/2008
	@module
*/

#include "MyGUI_OgreDataManager.h"
#include "MyGUI_OgreDiagnostic.h"
#include "MyGUI_OgreDataStream.h"

#include <Ogre.h>

#include "MyGUI_LastHeader.h"

#if MYGUI_PLATFORM == MYGUI_PLATFORM_APPLE
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace MyGUI
{

	#if MYGUI_PLATFORM == MYGUI_PLATFORM_APPLE
	// This function will locate the path to our application on OS X,
	// unlike windows you can not rely on the curent working directory
	// for locating your configuration files and resources.
	std::string MYGUI_EXPORT macBundlePath()
	{
		char path[1024];
		CFBundleRef mainBundle = CFBundleGetMainBundle();    assert(mainBundle);
		CFURLRef mainBundleURL = CFBundleCopyBundleURL(mainBundle);    assert(mainBundleURL);
		CFStringRef cfStringRef = CFURLCopyFileSystemPath( mainBundleURL, kCFURLPOSIXPathStyle);    assert(cfStringRef);
		CFStringGetCString(cfStringRef, path, 1024, kCFStringEncodingASCII);
		CFRelease(mainBundleURL);
		CFRelease(cfStringRef);
		return std::string(path);
	}
	#endif

	MYGUI_INSTANCE_IMPLEMENT(OgreDataManager)

	void OgreDataManager::initialise(const std::string& _group)
	{
		MYGUI_PLATFORM_ASSERT(!mIsInitialise, INSTANCE_TYPE_NAME << " initialised twice");
		MYGUI_PLATFORM_LOG(Info, "* Initialise: " << INSTANCE_TYPE_NAME);

		mGroup = _group;

		MYGUI_PLATFORM_LOG(Info, INSTANCE_TYPE_NAME << " successfully initialized");
		mIsInitialise = true;
	}

	void OgreDataManager::shutdown()
	{
		if (!mIsInitialise) return;
		MYGUI_PLATFORM_LOG(Info, "* Shutdown: " << INSTANCE_TYPE_NAME);

		MYGUI_PLATFORM_LOG(Info, INSTANCE_TYPE_NAME << " successfully shutdown");
		mIsInitialise = false;
	}

	IDataStream* OgreDataManager::getData(const std::string& _name)
	{
		try
		{
			Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource(_name, mGroup);
			OgreDataStream* data = new OgreDataStream(stream);

			return data;
		}
		catch(Ogre::FileNotFoundException)
		{
		}

		return nullptr;
	}

	bool OgreDataManager::isDataExist(const std::string& _name)
	{
		const VectorString& files = getDataListNames(_name);
		return (files.size() == 1);
	}

	const VectorString& OgreDataManager::getDataListNames(const std::string& _pattern)
	{
		return getDataListNames(_pattern, false);
	}

	const VectorString& OgreDataManager::getDataListNames(const std::string& _pattern, bool _fullpath)
	{
		static VectorString result;
		result.clear();

		Ogre::FileInfoListPtr pFileInfo = Ogre::ResourceGroupManager::getSingleton().findResourceFileInfo(mGroup, _pattern);

		result.reserve(pFileInfo->size());

		for (Ogre::FileInfoList::iterator fi = pFileInfo->begin(); fi != pFileInfo->end(); ++fi )
		{
			if (fi->path.empty())
			{
				bool found = false;
				for (VectorString::iterator iter=result.begin(); iter!=result.end(); ++iter)
				{
					if (*iter == fi->filename)
					{
						found = true;
						break;
					}
				}
				if (!found)
				{
					result.push_back(_fullpath ? fi->archive->getName() + "/" + fi->filename : fi->filename);
				}

			}
		}

		pFileInfo.setNull();

		return result;
	}

	const std::string& OgreDataManager::getDataPath(const std::string& _name)
	{
		static std::string result;

		const VectorString& files = getDataListNames(_name, true);
		result = files.size() == 1 ? files[0] : "";

		return result;
	}

} // namespace MyGUI
