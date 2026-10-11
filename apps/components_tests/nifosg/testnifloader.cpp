#include "../nif/node.hpp"

#include <components/nif/controller.hpp>
#include <components/nif/data.hpp>
#include <components/nif/node.hpp>
#include <components/nif/particle.hpp>
#include <components/nif/property.hpp>
#include <components/nifosg/nifloader.hpp>
#include <components/resource/bgsmfilemanager.hpp>
#include <components/resource/imagemanager.hpp>
#include <components/sceneutil/serialize.hpp>
#include <components/sceneutil/visitor.hpp>
#include <components/vfs/manager.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <osgDB/Registry>
#include <osgParticle/ParticleSystem>

#include <array>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
    using namespace testing;
    using namespace NifOsg;
    using namespace Nif::Testing;

    constexpr VFS::Path::NormalizedView testNif("test.nif");

    struct BaseNifOsgLoaderTest
    {
        VFS::Manager mVfs;
        Resource::ImageManager mImageManager{ &mVfs, 0 };
        Resource::BgsmFileManager mMaterialManager{ &mVfs, 0 };
        const osgDB::ReaderWriter* mReaderWriter = osgDB::Registry::instance()->getReaderWriterForExtension("osgt");
        osg::ref_ptr<osgDB::Options> mOptions = new osgDB::Options;

        BaseNifOsgLoaderTest()
        {
            SceneUtil::registerSerializers();

            if (mReaderWriter == nullptr)
                throw std::runtime_error("osgt reader writer is not found");

            mOptions->setPluginStringData("fileType", "Ascii");
            mOptions->setPluginStringData("WriteImageHint", "UseExternal");
        }

        std::string serialize(const osg::Node& node) const
        {
            std::stringstream stream;
            mReaderWriter->writeNode(node, stream, mOptions);
            std::string result;
            for (std::string line; std::getline(stream, line);)
            {
                if (line.starts_with('#'))
                    continue;
                line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);
                result += line;
                result += '\n';
            }
            return result;
        }
    };

    struct NifOsgLoaderTest : Test, BaseNifOsgLoaderTest
    {
    };

    TEST_F(NifOsgLoaderTest, shouldLoadFileWithDefaultNode)
    {
        Nif::NiAVObject node;
        init(node);
        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&node);
        auto result = Loader::load(file, &mImageManager, &mMaterialManager);
        EXPECT_EQ(serialize(*result), R"(
osg::Group {
  UniqueID 1
  DataVariance STATIC
  UserDataContainer TRUE {
    osg::DefaultUserDataContainer {
      UniqueID 2
      UDC_UserObjects 1 {
        osg::StringValueObject {
          UniqueID 3
          Name "fileHash"
        }
      }
    }
  }
  Children 1 {
    osg::Group {
      UniqueID 4
      DataVariance STATIC
      UserDataContainer TRUE {
        osg::DefaultUserDataContainer {
          UniqueID 5
          UDC_UserObjects 1 {
            osg::UIntValueObject {
              UniqueID 6
              Name "recordIndex"
              Value 4294967295
            }
          }
        }
      }
    }
  }
}
)");
    }

    std::string formatOsgNodeForBSShaderProperty(std::string_view shaderPrefix)
    {
        std::ostringstream oss;
        oss << R"(
osg::Group {
  UniqueID 1
  DataVariance STATIC
  UserDataContainer TRUE {
    osg::DefaultUserDataContainer {
      UniqueID 2
      UDC_UserObjects 1 {
        osg::StringValueObject {
          UniqueID 3
          Name "fileHash"
        }
      }
    }
  }
  Children 1 {
    osg::Group {
      UniqueID 4
      DataVariance STATIC
      UserDataContainer TRUE {
        osg::DefaultUserDataContainer {
          UniqueID 5
          UDC_UserObjects 2 {
            osg::UIntValueObject {
              UniqueID 6
              Name "recordIndex"
              Value 4294967295
            }
            osg::StringValueObject {
              UniqueID 7
              Name "shaderPrefix"
              Value ")"
            << shaderPrefix << R"("
            }
          }
        }
      }
      StateSet TRUE {
        osg::StateSet {
          UniqueID 8
        }
      }
    }
  }
}
)";
        return oss.str();
    }

    std::string formatOsgNodeForBSLightingShaderProperty(std::string_view shaderPrefix)
    {
        std::ostringstream oss;
        oss << R"(
osg::Group {
  UniqueID 1
  DataVariance STATIC
  UserDataContainer TRUE {
    osg::DefaultUserDataContainer {
      UniqueID 2
      UDC_UserObjects 1 {
        osg::StringValueObject {
          UniqueID 3
          Name "fileHash"
        }
      }
    }
  }
  Children 1 {
    osg::Group {
      UniqueID 4
      DataVariance STATIC
      UserDataContainer TRUE {
        osg::DefaultUserDataContainer {
          UniqueID 5
          UDC_UserObjects 2 {
            osg::UIntValueObject {
              UniqueID 6
              Name "recordIndex"
              Value 4294967295
            }
            osg::StringValueObject {
              UniqueID 7
              Name "shaderPrefix"
              Value ")"
            << shaderPrefix << R"("
            }
          }
        }
      }
      StateSet TRUE {
        osg::StateSet {
          UniqueID 8
          ModeList 1 {
            GL_DEPTH_TEST ON
          }
          AttributeList 1 {
            osg::Depth {
              UniqueID 9
              Function LEQUAL
            }
            Value OFF
          }
        }
      }
    }
  }
}
)";
        return oss.str();
    }

    struct ShaderPrefixParams
    {
        unsigned int mShaderType;
        std::string_view mExpectedShaderPrefix;
    };

    struct NifOsgLoaderBSShaderPrefixTest : TestWithParam<ShaderPrefixParams>, BaseNifOsgLoaderTest
    {
        static constexpr std::array sParams = {
            ShaderPrefixParams{ static_cast<unsigned int>(Nif::BSShaderType::ShaderType_Default), "bs/default" },
            ShaderPrefixParams{ static_cast<unsigned int>(Nif::BSShaderType::ShaderType_NoLighting), "bs/nolighting" },
            ShaderPrefixParams{ static_cast<unsigned int>(Nif::BSShaderType::ShaderType_Tile), "bs/default" },
            ShaderPrefixParams{ std::numeric_limits<unsigned int>::max(), "bs/default" },
        };
    };

    TEST_P(NifOsgLoaderBSShaderPrefixTest, shouldAddShaderPrefix)
    {
        Nif::NiAVObject node;
        init(node);
        Nif::BSShaderPPLightingProperty property;
        property.mRecordType = Nif::RC_BSShaderPPLightingProperty;
        property.mTextureSet = nullptr;
        property.mController = nullptr;
        property.mType = GetParam().mShaderType;
        node.mProperties.push_back(Nif::RecordPtrT<Nif::NiProperty>(&property));
        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&node);
        auto result = Loader::load(file, &mImageManager, &mMaterialManager);
        EXPECT_EQ(serialize(*result), formatOsgNodeForBSShaderProperty(GetParam().mExpectedShaderPrefix));
    }

    INSTANTIATE_TEST_SUITE_P(Params, NifOsgLoaderBSShaderPrefixTest, ValuesIn(NifOsgLoaderBSShaderPrefixTest::sParams));

    struct NifOsgLoaderBSLightingShaderPrefixTest : TestWithParam<ShaderPrefixParams>, BaseNifOsgLoaderTest
    {
        static constexpr std::array sParams = {
            ShaderPrefixParams{
                static_cast<unsigned int>(Nif::BSLightingShaderType::ShaderType_Default), "bs/default" },
            ShaderPrefixParams{ static_cast<unsigned int>(Nif::BSLightingShaderType::ShaderType_Cloud), "bs/default" },
            ShaderPrefixParams{ std::numeric_limits<unsigned int>::max(), "bs/default" },
        };
    };

    TEST_P(NifOsgLoaderBSLightingShaderPrefixTest, shouldAddShaderPrefix)
    {
        Nif::NiAVObject node;
        init(node);
        Nif::BSLightingShaderProperty property;
        property.mRecordType = Nif::RC_BSLightingShaderProperty;
        property.mTextureSet = nullptr;
        property.mController = nullptr;
        property.mType = GetParam().mShaderType;
        property.mShaderFlags1 |= Nif::BSShaderFlags1::BSSFlag1_DepthTest;
        property.mShaderFlags2 |= Nif::BSShaderFlags2::BSSFlag2_DepthWrite;
        node.mProperties.push_back(Nif::RecordPtrT<Nif::NiProperty>(&property));
        Nif::NIFFile file(testNif);
        file.mRoots.push_back(&node);
        auto result = Loader::load(file, &mImageManager, &mMaterialManager);
        EXPECT_EQ(serialize(*result), formatOsgNodeForBSLightingShaderProperty(GetParam().mExpectedShaderPrefix));
    }

    INSTANTIATE_TEST_SUITE_P(
        Params, NifOsgLoaderBSLightingShaderPrefixTest, ValuesIn(NifOsgLoaderBSLightingShaderPrefixTest::sParams));

    TEST_F(NifOsgLoaderTest, shouldApplyAgeAffectorsToSavedParticlesOnLoad)
    {
        Nif::NiColorData colorData;
        colorData.mKeyMap = std::make_shared<Nif::Vector4KeyMap>();
        colorData.mKeyMap->mInterpolationType = Nif::InterpolationType_Linear;
        colorData.mKeyMap->mKeys.push_back({ 0.f, { osg::Vec4f(1.f, 1.f, 1.f, 1.f), {}, {} } });
        colorData.mKeyMap->mKeys.push_back({ 1.f, { osg::Vec4f(0.2f, 0.2f, 0.2f, 0.f), {}, {} } });

        Nif::NiParticleColorModifier color;
        color.mRecordType = Nif::RC_NiParticleColorModifier;
        color.mNext = nullptr;
        color.mController = nullptr;
        color.mData = &colorData;

        Nif::NiParticleGrowFade growFade;
        growFade.mRecordType = Nif::RC_NiParticleGrowFade;
        growFade.mNext = &color;
        growFade.mController = nullptr;
        growFade.mGrowTime = 1.f;
        growFade.mFadeTime = 0.f;

        Nif::NiParticlesData data;
        data.mNumParticles = 2;
        data.mActiveCount = 2;
        data.mVertices = { osg::Vec3f(0.f, 0.f, 0.f), osg::Vec3f(10.f, 0.f, 0.f) };
        data.mBoundingSphere = osg::BoundingSpheref(osg::Vec3f(), 10.f);

        for (const bool emits : { true, false })
        {
            SCOPED_TRACE(emits ? "emits" : "emits nothing");

            Nif::NiNode root;
            init(root);
            root.mRecordType = Nif::RC_NiNode;
            root.mRecordIndex = 0;

            Nif::NiParticleSystemController controller{};
            init(controller);
            controller.mRecordType = Nif::RC_NiParticleSystemController;
            controller.mFlags = Nif::NiTimeController::Flag_Active;
            controller.mInitialColor = osg::Vec4f(1.f, 1.f, 1.f, 1.f);
            controller.mInitialSize = 4.f;
            controller.mEmitter = emits ? &root : nullptr;
            controller.mModifier = &growFade;
            controller.mCollider = nullptr;
            for (const float age : { 0.5f, 1.5f })
            {
                Nif::NiParticleInfo& info = controller.mParticles.emplace_back();
                info.mAge = age;
                info.mLifespan = 2.f;
                info.mCode = static_cast<uint16_t>(controller.mParticles.size() - 1);
            }

            Nif::NiParticles particles;
            init(particles);
            particles.mRecordType = Nif::RC_NiParticles;
            particles.mRecordIndex = 1;
            particles.mData = Nif::NiGeometryDataPtr(&data);
            particles.mController = &controller;
            root.mChildren.push_back(Nif::NiAVObjectPtr(&particles));

            Nif::NIFFile file(testNif);
            file.mRoots.push_back(&root);
            auto result = Loader::load(file, &mImageManager, &mMaterialManager);

            SceneUtil::FindByClassVisitor find("ParticleSystem");
            result->accept(find);
            ASSERT_EQ(find.mFoundNodes.size(), 1u);
            const auto& system = static_cast<const osgParticle::ParticleSystem&>(*find.mFoundNodes.front());
            ASSERT_EQ(system.numParticles(), 2);

            // Age 0.5 of 2: colour 1 - 0.8 * 0.25 = 0.8, alpha 0.75, size 4 * 0.5 / 1 = 2
            const osgParticle::Particle& young = *system.getParticle(0);
            EXPECT_FLOAT_EQ(young.getCurrentColor().r(), 0.8f);
            EXPECT_FLOAT_EQ(young.getCurrentAlpha(), 0.75f);
            EXPECT_FLOAT_EQ(young.getCurrentSize(), 2.f);

            // Age 1.5 of 2: colour 1 - 0.8 * 0.75 = 0.4, alpha 0.25, fully grown so size 4
            const osgParticle::Particle& old = *system.getParticle(1);
            EXPECT_FLOAT_EQ(old.getCurrentColor().r(), 0.4f);
            EXPECT_FLOAT_EQ(old.getCurrentAlpha(), 0.25f);
            EXPECT_FLOAT_EQ(old.getCurrentSize(), 4.f);
        }
    }
}
