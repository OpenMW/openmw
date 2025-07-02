#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <components/files/configurationmanager.hpp>
#include <components/fx/technique.hpp>
#include <components/resource/imagemanager.hpp>
#include <components/settings/settings.hpp>
#include <components/testing/util.hpp>

namespace
{
    constexpr VFS::Path::NormalizedView techniquePropertiesPath("shaders/technique_properties.omwfx");

    TestingOpenMW::VFSTestFile techniqueProperties(R"(
    fragment main {}
    vertex main {}
    technique {
        passes = main;
        version = "0.1a";
        description = "description";
        author = "author";
        glsl_version = 330;
        glsl_profile = "compatability";
        glsl_extensions = GL_EXT_gpu_shader4, GL_ARB_uniform_buffer_object;
        flags = disable_sunglare;
        hdr = true;
    }
)");

    constexpr VFS::Path::NormalizedView rendertargetPropertiesPath("shaders/rendertarget_properties.omwfx");

    TestingOpenMW::VFSTestFile rendertargetProperties{ R"(
    render_target rendertarget {
        width_ratio = 0.5;
        height_ratio = 0.5;
        internal_format = r16f;
        source_type = float;
        source_format = red;
        mipmaps = true;
        wrap_s = clamp_to_edge;
        wrap_t = repeat;
        min_filter = linear;
        mag_filter = nearest;
    }
    fragment downsample2x(target=rendertarget) {

        omw_In vec2 omw_TexCoord;

        void main()
        {
            omw_FragColor.r = omw_GetLastShader(omw_TexCoord).r;
        }
    }
    fragment main { }
    technique { passes = downsample2x, main; }
)" };

    constexpr VFS::Path::NormalizedView uniformPropertiesPath("shaders/uniform_properties.omwfx");

    TestingOpenMW::VFSTestFile uniformProperties{ R"(
    uniform_vec4 uVec4 {
        default = vec4(0,0,0,0);
        min = vec4(0,1,0,0);
        max = vec4(0,0,1,0);
        step = 0.5;
        header = "header";
        static = true;
        description = "description";
    }
    fragment main { }
    technique { passes = main; }
)" };

    constexpr VFS::Path::NormalizedView missingSamplerSourcePath("shaders/missing_sampler_source.omwfx");

    TestingOpenMW::VFSTestFile missingSamplerSource{ R"(
    sampler_1d mysampler1d { }
    fragment main { }
    technique { passes = main; }
)" };

    constexpr VFS::Path::NormalizedView repeatedSharedBlockPath("shaders/repeated_shared_block.omwfx");

    TestingOpenMW::VFSTestFile repeatedSharedBlock{ R"(
    shared {
        float myfloat = 1.0;
    }
    shared {}
    fragment main { }
    technique { passes = main; }
)" };

    using namespace testing;
    using namespace fx;

    struct TechniqueTest : Test
    {
        std::unique_ptr<VFS::Manager> mVFS;
        Resource::ImageManager mImageManager;
        std::unique_ptr<Technique> mTechnique;

        TechniqueTest()
            : mVFS(TestingOpenMW::createTestVFS({
                { techniquePropertiesPath, &techniqueProperties },
                { rendertargetPropertiesPath, &rendertargetProperties },
                { uniformPropertiesPath, &uniformProperties },
                { missingSamplerSourcePath, &missingSamplerSource },
                { repeatedSharedBlockPath, &repeatedSharedBlock },
            }))
            , mImageManager(mVFS.get(), 0)
        {
        }

        void compile(const std::string& name)
        {
            mTechnique = std::make_unique<Technique>(
                *mVFS.get(), mImageManager, Technique::makeFileName(name), name, 1, 1, true, true);
            mTechnique->compile();
        }
    };

    TEST_F(TechniqueTest, technique_properties)
    {
        std::unordered_set<std::string> targetExtensions = { "GL_EXT_gpu_shader4", "GL_ARB_uniform_buffer_object" };

        compile("technique_properties");

        EXPECT_EQ(mTechnique->getVersion(), "0.1a");
        EXPECT_EQ(mTechnique->getDescription(), "description");
        EXPECT_EQ(mTechnique->getAuthor(), "author");
        EXPECT_EQ(mTechnique->getGLSLVersion(), 330);
        EXPECT_EQ(mTechnique->getGLSLProfile(), "compatability");
        EXPECT_EQ(mTechnique->getGLSLExtensions(), targetExtensions);
        EXPECT_EQ(mTechnique->getFlags(), Technique::Flag_Disable_SunGlare);
        EXPECT_EQ(mTechnique->getHDR(), true);
        EXPECT_EQ(mTechnique->getPasses().size(), 1);
        EXPECT_EQ(mTechnique->getPasses().front()->getName(), "main");
    }

    TEST_F(TechniqueTest, rendertarget_properties)
    {
        compile("rendertarget_properties");

        EXPECT_EQ(mTechnique->getRenderTargetsMap().size(), 1);

        const std::string_view name = mTechnique->getRenderTargetsMap().begin()->first;
        auto& rt = mTechnique->getRenderTargetsMap().begin()->second;
        auto& texture = rt.mTarget;

        EXPECT_EQ(name, "rendertarget");
        EXPECT_EQ(rt.mMipMap, true);
        EXPECT_EQ(rt.mSize.mWidthRatio, 0.5f);
        EXPECT_EQ(rt.mSize.mHeightRatio, 0.5f);
        EXPECT_EQ(texture->getWrap(osg::Texture::WRAP_S), osg::Texture::CLAMP_TO_EDGE);
        EXPECT_EQ(texture->getWrap(osg::Texture::WRAP_T), osg::Texture::REPEAT);
        EXPECT_EQ(texture->getFilter(osg::Texture::MIN_FILTER), osg::Texture::LINEAR);
        EXPECT_EQ(texture->getFilter(osg::Texture::MAG_FILTER), osg::Texture::NEAREST);
        EXPECT_EQ(texture->getSourceType(), static_cast<GLenum>(GL_FLOAT));
        EXPECT_EQ(texture->getSourceFormat(), static_cast<GLenum>(GL_RED));
        EXPECT_EQ(texture->getInternalFormat(), static_cast<GLint>(GL_R16F));

        EXPECT_EQ(mTechnique->getPasses().size(), 2);
        EXPECT_EQ(mTechnique->getPasses()[0]->getTarget(), "rendertarget");
    }

    TEST_F(TechniqueTest, uniform_properties)
    {
        compile("uniform_properties");

        EXPECT_EQ(mTechnique->getUniformMap().size(), 1);

        const auto& uniform = mTechnique->getUniformMap().front();

        EXPECT_TRUE(uniform->mStatic);
        EXPECT_DOUBLE_EQ(uniform->mStep, 0.5);
        EXPECT_EQ(uniform->getDefault<osg::Vec4f>(), osg::Vec4f(0, 0, 0, 0));
        EXPECT_EQ(uniform->getMin<osg::Vec4f>(), osg::Vec4f(0, 1, 0, 0));
        EXPECT_EQ(uniform->getMax<osg::Vec4f>(), osg::Vec4f(0, 0, 1, 0));
        EXPECT_EQ(uniform->mHeader, "header");
        EXPECT_EQ(uniform->mDescription, "description");
        EXPECT_EQ(uniform->mName, "uVec4");
    }

    TEST_F(TechniqueTest, fail_with_missing_source_for_sampler)
    {
        internal::CaptureStdout();

        compile("missing_sampler_source");

        std::string output = internal::GetCapturedStdout();
        Log(Debug::Error) << output;
        EXPECT_THAT(output, HasSubstr("sampler_1d 'mysampler1d' requires a filename"));
    }

    TEST_F(TechniqueTest, fail_with_repeated_shared_block)
    {
        internal::CaptureStdout();

        compile("repeated_shared_block");

        std::string output = internal::GetCapturedStdout();
        Log(Debug::Error) << output;
        EXPECT_THAT(output, HasSubstr("repeated 'shared' block"));
    }
}
