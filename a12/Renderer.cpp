//
// Created by thomas on 29/01/25.
//

#include "Renderer.hpp"

#include "../labutils/vkutil.hpp"
#include "Light.hpp"
#include "glsl.hpp"

#include "../gltf/Entity.hpp"
#include "PipelineBuilder.hpp"

namespace GraphicsThings {

Renderer::Renderer() {
  // create the window
  mWindow = labutils::make_vulkan_window();
  // configure the GLFW window
  glfwSetWindowUserPointer(mWindow.window, &mState);
  glfwSetKeyCallback(mWindow.window, &lut::user_state_key_press);
  glfwSetMouseButtonCallback(mWindow.window, &lut::user_state_button);
  glfwSetCursorPosCallback(mWindow.window, &lut::user_state_motion);

  // create the VMA allocator
  mAllocator = lut::create_allocator(mWindow);

  // create the render passes
  mForwardRenderPass =
      create_render_pass_to_texture(mWindow, cfg::kDepthFormat);
  mPostProcessRenderPass =
      create_postprocess_render_pass(mWindow, true, cfg::kDepthFormat);
  mBloomRenderPass =
      create_postprocess_render_pass(mWindow, false, cfg::kDepthFormat);

  // create the framebuffers
  // right now there are 3:
  // 1. framebuffers for the swapchain images
  // 2. framebuffers for the render to texture stage (forward render pass)
  // 3. framebuffers for the bloom stage

  create_swapchain_framebuffers(mWindow, mPostProcessRenderPass.handle);

  // generate a render texture
  std::tie(mRenderTexture, mRenderTextureView) =
      create_render_texture(mWindow, mAllocator);
  // create the depth buffer
  std::tie(mDepthBuffer, mDepthBufferView) =
      create_depth_buffer(mWindow, mAllocator, cfg::kDepthFormat);
  // create the intermediate bloom blur buffer
  std::tie(mBloomBuffer, mBloomBufferView) =
      create_render_texture(mWindow, mAllocator);

  // create the render texture framebuffer
  mRenderTextureFramebuffer =
      create_framebuffer(mWindow, mForwardRenderPass.handle,
                         {mRenderTextureView.handle, mDepthBufferView.handle});

  // create the bloom buffer framebuffer
  mBloomBufferFramebuffer = create_framebuffer(mWindow, mBloomRenderPass.handle,
                                               {mBloomBufferView.handle});

  // create the shadow light manager
  mShadowLightManager = new ShadowLightManager(&mWindow, &mAllocator);

  // Create per-frame resources
  mPerFrameResources = new PerFrameResource(&mWindow);
  // create the UBOs
  // scene uniforms
  mSceneUBO = lut::create_buffer(mAllocator, sizeof(glsl::SceneUniform),
                                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
  mUbos.emplace_back(glsl::UBO{&mSceneUniforms, sizeof(glsl::SceneUniform),
                               mSceneUBO.buffer,
                               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                                   VK_PIPELINE_STAGE_VERTEX_SHADER_BIT});

  // create the lighting UBO
  mLightingUBO = lut::create_buffer(mAllocator, sizeof(glsl::LightingUniform),
                                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                    0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
  mUbos.emplace_back(glsl::UBO{&Light::lightingUniforms,
                               sizeof(glsl::LightingUniform),
                               mLightingUBO.buffer,
                               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                                   VK_PIPELINE_STAGE_VERTEX_SHADER_BIT});

  // create the light UBO
  mLightUBO = lut::create_buffer(mAllocator, sizeof(glsl::LightUniform),
                                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
  mUbos.emplace_back(glsl::UBO{&Light::lightUniforms,
                               sizeof(glsl::LightUniform), mLightUBO.buffer,
                               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                                   VK_PIPELINE_STAGE_VERTEX_SHADER_BIT});

  // create the VP^-1 matrix
  mVPInverse = glm::identity<glm::mat4>();
  // create the VP^-1 matrix UBO
  mVPInverseUBO = lut::create_buffer(mAllocator, sizeof(glm::mat4),
                                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                         VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                     0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
  mUbos.emplace_back(glsl::UBO{&mVPInverse, sizeof(glm::mat4),
                               mVPInverseUBO.buffer,
                               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT});

  // create the camera position UBO
  mCameraPosition = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
  mCameraPositionUBO = lut::create_buffer(
      mAllocator, sizeof(glm::vec4),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0,
      VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
  mUbos.emplace_back(glsl::UBO{&mCameraPosition, sizeof(glm::vec4),
                               mCameraPositionUBO.buffer,
                               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT});

  // create the descriptor pool for descriptor sets that are only created once
  mStaticDescriptorPool = lut::create_descriptor_pool(mWindow);
  // create the descriptor poool for descriptor sets that get remade sometimes
  mDynamicDescriptorPool = lut::create_descriptor_pool(mWindow, 1024);

  // create the descriptor set layout
  mSceneLayout = create_scene_descriptor_layout(mWindow);
  // allocate the descriptor set for the scene
  mSceneDescriptors = create_scene_descriptor_set(
      mWindow, mStaticDescriptorPool, mSceneUBO, mSceneLayout);

  // create the lighting set layout
  mLightingLayout = create_lighting_descriptor_layout(mWindow);

  // allocate the descriptor set for the lighting
  mLightingDescriptors = create_lighting_descriptor_set(
      mWindow, mStaticDescriptorPool, mLightingUBO, mLightUBO, mLightingLayout);

  // create default texture sampler
  mDefaultSampler = lut::create_default_sampler(mWindow);
  // craete a post process sampler that doesn't repeat
  mPostProcessSampler = lut::create_postprocess_sampler(mWindow);

  mPostProcessLayout = lut::create_postprocess_descriptor_layout(mWindow);
  // create the post process descriptor set
  mPostProcessDescriptors = create_post_process_descriptor_set(
      mWindow, mDynamicDescriptorPool, mRenderTextureView, mDepthBufferView,
      mPostProcessSampler, mPostProcessLayout);
  // create the bloom descriptor set - just contains the bloom texture
  mBloomLayout = create_bloom_descriptor_layout(mWindow);
  mBloomDescriptors = create_bloom_descriptor_set(
      mWindow, mDynamicDescriptorPool, mBloomBufferView, mPostProcessSampler,
      mBloomLayout);

  // create the screen size ubo
  mScreenSizeUBO = lut::create_buffer(mAllocator, sizeof(glm::vec2),
                                      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                      0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

  // create the pipelines
  // for each pipeline
  // create the default pipeline layout

  // create the descriptor set layout for materials
  mMaterialLayout = create_material_descriptor_layout(mWindow);
  VkDescriptorSetLayout layouts[] = {
      mSceneLayout.handle, mMaterialLayout.handle, mLightingLayout.handle,
      mShadowLightManager->allShadowMapDescriptorSetLayout.handle};
  mPipelineLayout = create_basic_pipeline_layout(
      mWindow, layouts, (sizeof(layouts) / sizeof(layouts[0])));
  mPostProcessPipeLayout = create_postprocess_pipeline_layout(
      mWindow, &mPostProcessLayout.handle, 1);
  VkDescriptorSetLayout bloomLayouts[] = {mPostProcessLayout.handle,
                                          mBloomLayout.handle};
  mBloomPipeLayout =
      create_postprocess_pipeline_layout(mWindow, bloomLayouts, 2);

  VkPushConstantRange pushConstant{};
  pushConstant.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstant.offset = 0;
  pushConstant.size = sizeof(glm::mat4);

  // Let's try adding my version
  auto forwardPipeline =
      vkEngine::PipelineBuilder(mWindow.device, PipelineType::GRAPHICS,
                                VertexBinding::BIND, 0)
          .AddShader(cfg::kPBRVertexShaderPath, ShaderType::VERTEX)
          .AddShader(cfg::kPBRFragmentShaderPath, ShaderType::FRAGMENT)
          .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
          .SetDynamicState(
              {{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR}})
          .SetRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT,
                                 VK_FRONT_FACE_COUNTER_CLOCKWISE)
          .SetPipelineLayout(
              {{mSceneLayout.handle, mMaterialLayout.handle,
                mLightingLayout.handle,
                mShadowLightManager->allShadowMapDescriptorSetLayout.handle}},
              pushConstant)
          .SetSampling(VK_SAMPLE_COUNT_1_BIT)
          .AddBlendAttachmentState()
          .SetDepthState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL)
          .SetRenderPass(mForwardRenderPass.handle)
          .Build();

  m_ForwardPipeline = forwardPipeline.first;
  m_ForwardPipelineLayout = forwardPipeline.second;

  // mBasicPipeline = new Pipeline(
  //     mWindow, &(mPipelineLayout.handle), mForwardRenderPass.handle,
  //     create_basic_pipeline, cfg::kPBRVertexShaderPath,
  //     cfg::kPBRFragmentShaderPath, 1, "Basic Pipeline");
  //  create the alpha pipeline
  mAlphaPipeline = new Pipeline(
      mWindow, &(mPipelineLayout.handle), mForwardRenderPass.handle,
      create_alpha_pipeline, cfg::kPBRVertexShaderPath,
      cfg::kPBRFragmentShaderPathAlpha, VK_BLEND_FACTOR_SRC_ALPHA,
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_CULL_MODE_NONE, VK_TRUE);

  auto postProcessPipeline =
      vkEngine::PipelineBuilder(mWindow.device, PipelineType::GRAPHICS,
                                VertexBinding::NONE, 0)
          .AddShader(cfg::kPostProcessVertexShaderPath, ShaderType::VERTEX)
          .AddShader(cfg::kPostProcessFragmentShaderPath, ShaderType::FRAGMENT)
          .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
          .SetDynamicState(
              {{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR}})
          .SetRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
                                 VK_FRONT_FACE_COUNTER_CLOCKWISE)
          .SetPipelineLayout({mPostProcessLayout.handle})
          .SetSampling(VK_SAMPLE_COUNT_1_BIT)
          .AddBlendAttachmentState()
          .SetDepthState(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL)
          .SetRenderPass(mPostProcessRenderPass.handle)
          .Build();

  m_PostProcessPipeline = postProcessPipeline.first;
  m_PostProcessPipelineLayout = postProcessPipeline.second;

  auto mosiacPipeline =
      vkEngine::PipelineBuilder(mWindow.device, PipelineType::GRAPHICS,
                                VertexBinding::NONE, 0)
          .AddShader(cfg::kPostProcessVertexShaderPath, ShaderType::VERTEX)
          .AddShader(cfg::kMosaicFragmentShaderPath, ShaderType::FRAGMENT)
          .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
          .SetDynamicState(
              {{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR}})
          .SetRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
                                 VK_FRONT_FACE_COUNTER_CLOCKWISE)
          .SetPipelineLayout({mPostProcessLayout.handle})
          .SetSampling(VK_SAMPLE_COUNT_1_BIT)
          .AddBlendAttachmentState()
          .SetDepthState(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL)
          .SetRenderPass(mPostProcessRenderPass.handle)
          .Build();

  m_MosiacPipeline = mosiacPipeline.first;
  m_MosiacPipelineLayout = mosiacPipeline.second;

  // the bloom pipeline has an extra descriptor set, so we need a different
  // layout

  auto bloomHorizontalPass =
      vkEngine::PipelineBuilder(mWindow.device, PipelineType::GRAPHICS,
                                VertexBinding::NONE, 0)
          .AddShader(cfg::kPostProcessVertexShaderPath, ShaderType::VERTEX)
          .AddShader(cfg::kPPBloomFragmentShaderPath, ShaderType::FRAGMENT)
          .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
          .SetDynamicState(
              {{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR}})
          .SetRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
                                 VK_FRONT_FACE_COUNTER_CLOCKWISE)
          .SetPipelineLayout({mPostProcessLayout.handle, mBloomLayout.handle})
          .SetSampling(VK_SAMPLE_COUNT_1_BIT)
          .AddBlendAttachmentState()
          .SetDepthState(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL)
          .SetRenderPass(mPostProcessRenderPass.handle)
          .Build();

  m_BloomHorizontalPipeline = bloomHorizontalPass.first;
  m_BloomHorizontalPipelineLayout = bloomHorizontalPass.second;

  auto bloomVerticalPass =
      vkEngine::PipelineBuilder(mWindow.device, PipelineType::GRAPHICS,
                                VertexBinding::NONE, 0)
          .AddShader(cfg::kPostProcessVertexShaderPath, ShaderType::VERTEX)
          .AddShader(cfg::kBloomVerticalFragmentShaderPath,
                     ShaderType::FRAGMENT)
          .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
          .SetDynamicState(
              {{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR}})
          .SetRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
                                 VK_FRONT_FACE_COUNTER_CLOCKWISE)
          .SetPipelineLayout({mPostProcessLayout.handle})
          .SetSampling(VK_SAMPLE_COUNT_1_BIT)
          .AddBlendAttachmentState()
          .SetDepthState(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL)
          .SetRenderPass(mBloomRenderPass.handle)
          .Build();

  m_BloomVerticalPipeline = bloomVerticalPass.first;
  m_BloomVerticalPipelineLayout = bloomVerticalPass.second;

  // TODO: replace
  {
    // load scene data - meshes, textures
    // get the baked models
    auto model = load_baked_model(cfg::kBakedModelPath);

    mTextures.reserve(model.textures.size());
    mTextureViews.reserve(model.textures.size());
    // load the textures
    loadTexturesToImage(mAllocator, mWindow, model.textures, mTextures,
                        mTextureViews);
    // add one more texture for the default normal map
    mTextures.emplace_back(
        load_image_texture2d("assets/a12/default_normal.png", mWindow,
                             lut::create_command_pool(mWindow).handle,
                             mAllocator, VK_FORMAT_R8G8B8A8_SRGB));
    mTextureViews.emplace_back(create_image_view_texture2d(
        mWindow, mTextures.back().image, VK_FORMAT_R8G8B8A8_SRGB));

    int dummyNormalTextureId = int(mTextureViews.size() - 1);
    // update the normal texture id for each material
    for (auto &material : model.materials) {
      if (material.normalMapTextureId == 0xFFFFFFFF) {
        material.normalMapTextureId = dummyNormalTextureId;
      }
    }

    // create a descriptor set for each material
    mMaterialDescriptorSets.reserve(model.materials.size());
    // for each material
    for (auto const &material : model.materials) {
      // create the descriptor set
      mMaterialDescriptorSets.push_back(create_material_descriptor_set(
          mWindow, mStaticDescriptorPool, material, mTextureViews,
          mDefaultSampler, mMaterialLayout));
    }

    // create references of the meshes that are rendered by the alpha
    // pipeline and the opaque pipeline

    mAllMeshes.reserve(model.meshes.size());
    mAlphaMeshes.reserve(model.meshes.size());
    mOpaqueMeshes.reserve(model.meshes.size());
    // for each mesh in the model
    for (auto const &mesh : model.meshes) {
      // create the standard mesh
      mAllMeshes.emplace_back(mesh, mWindow, mAllocator,
                              m_ForwardPipelineLayout, mMaterialDescriptorSets);
      if (model.materials[mesh.materialId].alpha()) {
        mAlphaMeshes.push_back(&mAllMeshes.back());
      } else {
        mOpaqueMeshes.push_back(&mAllMeshes.back());
      }
    }
  }
  // set up mesh manager
  mMeshManager = new MeshManager(mWindow, mAllocator);
  mMaterialManager = new MaterialManager();
  mTextureManager = new TextureManager(mWindow, mAllocator);
  // Hello other group members, change this to be the path to this git repository
  // https://github.com/KhronosGroup/glTF-Sample-Models/tree/main
  // probably don't download it on the lab machines its like 3GB
  std::string base_path = "/home/thomas/Downloads/glTF-Sample-Models/2.0/";
  std::string Suzanne = "Suzanne/glTF/Suzanne.gltf";
  std::string IridescenceMetallicSpheres = "IridescenceMetallicSpheres/glTF/IridescenceMetallicSpheres.gltf";
  std::string SimpleSparseAccessor = "SimpleSparseAccessor/glTF/SimpleSparseAccessor.gltf";
  std::string MosquitoInAmber = "MosquitoInAmber/glTF/MosquitoInAmber.gltf";
  std::string SheenDamask = "SheenDamask/glTF/SheenDamask.gltf";
  std::string MeshPrimitiveModes = "MeshPrimitiveModes/glTF/MeshPrimitiveModes.gltf";
  std::string MetalRoughSpheres = "MetalRoughSpheres/glTF/MetalRoughSpheres.gltf";
  std::string Fox = "Fox/glTF/Fox.gltf";
  std::string SheenHighHeel = "SheenHighHeel/glTF/SheenHighHeel.gltf";
  std::string ReciprocatingSaw = "ReciprocatingSaw/glTF/ReciprocatingSaw.gltf";
  std::string EmissiveStrengthTest = "EmissiveStrengthTest/glTF/EmissiveStrengthTest.gltf";
  std::string LightsPunctualLamp = "LightsPunctualLamp/glTF/LightsPunctualLamp.gltf";
  std::string Box_With_Spaces = "Box With Spaces/glTF/Box With Spaces.gltf";
  std::string IridescenceSuzanne = "IridescenceSuzanne/glTF/IridescenceSuzanne.gltf";
  std::string SimpleMorph = "SimpleMorph/glTF/SimpleMorph.gltf";
  std::string TextureTransformMultiTest = "TextureTransformMultiTest/glTF/TextureTransformMultiTest.gltf";
  std::string TextureCoordinateTest = "TextureCoordinateTest/glTF/TextureCoordinateTest.gltf";
  std::string NormalTangentTest = "NormalTangentTest/glTF/NormalTangentTest.gltf";
  std::string ClearcoatSphere = "ClearcoatSphere/glTF/ClearcoatSphere.gltf";
  std::string SpecularTest = "SpecularTest/glTF/SpecularTest.gltf";
  std::string BoomBoxWithAxes = "BoomBoxWithAxes/glTF/BoomBoxWithAxes.gltf";
  std::string VertexColorTest = "VertexColorTest/glTF/VertexColorTest.gltf";
  std::string PrimitiveModeNormalsTest = "PrimitiveModeNormalsTest/glTF/PrimitiveModeNormalsTest.gltf";
  std::string UnlitTest = "UnlitTest/glTF/UnlitTest.gltf";
  std::string BoxAnimated = "BoxAnimated/glTF/BoxAnimated.gltf";
  std::string SheenCloth = "SheenCloth/glTF/SheenCloth.gltf";
  std::string DragonAttenuation = "DragonAttenuation/glTF/DragonAttenuation.gltf";
  std::string MultipleScenes = "MultipleScenes/glTF/MultipleScenes.gltf";
  std::string AnimatedMorphSphere = "AnimatedMorphSphere/glTF/AnimatedMorphSphere.gltf";
  std::string AnimatedTriangle = "AnimatedTriangle/glTF/AnimatedTriangle.gltf";
  std::string ToyCar = "ToyCar/glTF/ToyCar.gltf";
  std::string ClearcoatRing = "ClearcoatRing/glTF/ClearcoatRing.gltf";
  std::string ClearCoatTest = "ClearCoatTest/glTF/ClearCoatTest.gltf";
  std::string ClearcoatWicker = "ClearcoatWicker/glTF/ClearcoatWicker.gltf";
  std::string ClearCoatCarPaint = "ClearCoatCarPaint/glTF/ClearCoatCarPaint.gltf";
  std::string NegativeScaleTest = "NegativeScaleTest/glTF/NegativeScaleTest.gltf";
  std::string TextureLinearInterpolationTest = "TextureLinearInterpolationTest/glTF/TextureLinearInterpolationTest.gltf";
  std::string MetalRoughSpheresNoTextures = "MetalRoughSpheresNoTextures/glTF/MetalRoughSpheresNoTextures.gltf";
  std::string DamagedHelmet = "DamagedHelmet/glTF/DamagedHelmet.gltf";
  std::string UnicodTest = "Unicode❤♻Test/glTF/Unicode❤♻Test.gltf";
  std::string TransmissionRoughnessTest = "TransmissionRoughnessTest/glTF/TransmissionRoughnessTest.gltf";
  std::string Triangle = "Triangle/glTF/Triangle.gltf";
  std::string GlamVelvetSofa = "GlamVelvetSofa/glTF/GlamVelvetSofa.gltf";
  std::string WaterBottle = "WaterBottle/glTF/WaterBottle.gltf";
  std::string BoxInterleaved = "BoxInterleaved/glTF/BoxInterleaved.gltf";
  std::string SimpleSkin = "SimpleSkin/glTF/SimpleSkin.gltf";
  std::string ABeautifulGame = "ABeautifulGame/glTF/ABeautifulGame.gltf";
  std::string CarbonFibre = "CarbonFibre/glTF/CarbonFibre.gltf";
  std::string SimpleMeshes = "SimpleMeshes/glTF/SimpleMeshes.gltf";
  std::string BoxTextured = "BoxTextured/glTF/BoxTextured.gltf";
  std::string EnvironmentTest = "EnvironmentTest/glTF/EnvironmentTest.gltf";
  std::string AnimatedMorphCube = "AnimatedMorphCube/glTF/AnimatedMorphCube.gltf";
  std::string Cube = "Cube/glTF/Cube.gltf";
  std::string AnimatedCube = "AnimatedCube/glTF/AnimatedCube.gltf";
  std::string Cameras = "Cameras/glTF/Cameras.gltf";
  std::string StainedGlassLamp = "StainedGlassLamp/glTF/StainedGlassLamp.gltf";
  std::string OrientationTest = "OrientationTest/glTF/OrientationTest.gltf";
  std::string Avocado = "Avocado/glTF/Avocado.gltf";
  std::string VC = "VC/glTF/VC.gltf";
  std::string XmpMetadataRoundedCube = "XmpMetadataRoundedCube/glTF/xmp.gltf";
  std::string CesiumMan = "CesiumMan/glTF/CesiumMan.gltf";
  std::string SimpleInstancing = "SimpleInstancing/glTF/SimpleInstancing.gltf";
  std::string TransmissionSuzanne = "TransmissionSuzanne/glTF/TransmissionSuzanne.gltf";
  std::string DirectionalLight = "DirectionalLight/glTF/DirectionalLight.gltf";
  std::string MorphStressTest = "MorphStressTest/glTF/MorphStressTest.gltf";
  std::string MaterialsVariantsShoe = "MaterialsVariantsShoe/glTF/MaterialsVariantsShoe.gltf";
  std::string BoxTexturedNonPowerOfTwo = "BoxTexturedNonPowerOfTwo/glTF/BoxTexturedNonPowerOfTwo.gltf";
  std::string a2CylinderEngine = "2CylinderEngine/glTF/2CylinderEngine.gltf";
  std::string TriangleWithoutIndices = "TriangleWithoutIndices/glTF/TriangleWithoutIndices.gltf";
  std::string BarramundiFish = "BarramundiFish/glTF/BarramundiFish.gltf";
  std::string FlightHelmet = "FlightHelmet/glTF/FlightHelmet.gltf";
  std::string AttenuationTest = "AttenuationTest/glTF/AttenuationTest.gltf";
  std::string BoxVertexColors = "BoxVertexColors/glTF/BoxVertexColors.gltf";
  std::string InterpolationTest = "InterpolationTest/glTF/InterpolationTest.gltf";
  std::string Lantern = "Lantern/glTF/Lantern.gltf";
  std::string TextureEncodingTest = "TextureEncodingTest/glTF/TextureEncodingTest.gltf";
  std::string IridescenceDielectricSpheres = "IridescenceDielectricSpheres/glTF/IridescenceDielectricSpheres.gltf";
  std::string SuzanneMorphSparse = "SuzanneMorphSparse/glTF/SuzanneMorphSparse.gltf";
  std::string Box = "Box/glTF/Box.gltf";
  std::string SheenChair = "SheenChair/glTF/SheenChair.gltf";
  std::string Duck = "Duck/glTF/Duck.gltf";
  std::string MorphPrimitivesTest = "MorphPrimitivesTest/glTF/MorphPrimitivesTest.gltf";
  std::string CesiumMilkTruck = "CesiumMilkTruck/glTF/CesiumMilkTruck.gltf";
  std::string MultiUVTest = "MultiUVTest/glTF/MultiUVTest.gltf";
  std::string GearboxAssy = "GearboxAssy/glTF/GearboxAssy.gltf";
  std::string TextureTransformTest = "TextureTransformTest/glTF/TextureTransformTest.gltf";
  std::string SpecGlossVsMetalRough = "SpecGlossVsMetalRough/glTF/SpecGlossVsMetalRough.gltf";
  std::string RiggedSimple = "RiggedSimple/glTF/RiggedSimple.gltf";
  std::string AlphaBlendModeTest = "AlphaBlendModeTest/glTF/AlphaBlendModeTest.gltf";
  std::string RecursiveSkeletons = "RecursiveSkeletons/glTF/RecursiveSkeletons.gltf";
  std::string TextureSettingsTest = "TextureSettingsTest/glTF/TextureSettingsTest.gltf";
  std::string IridescentDishWithOlives = "IridescentDishWithOlives/glTF/IridescentDishWithOlives.gltf";
  std::string Sponza = "Sponza/glTF/Sponza.gltf";
  std::string AntiqueCamera = "AntiqueCamera/glTF/AntiqueCamera.gltf";
  std::string BrainStem = "BrainStem/glTF/BrainStem.gltf";
  std::string RiggedFigure = "RiggedFigure/glTF/RiggedFigure.gltf";
  std::string Buggy = "Buggy/glTF/Buggy.gltf";
  std::string TransmissionTest = "TransmissionTest/glTF/TransmissionTest.gltf";
  std::string TwoSidedPlane = "TwoSidedPlane/glTF/TwoSidedPlane.gltf";
  std::string BoomBox = "BoomBox/glTF/BoomBox.gltf";
  std::string NormalTangentMirrorTest = "NormalTangentMirrorTest/glTF/NormalTangentMirrorTest.gltf";
  std::string SciFiHelmet = "SciFiHelmet/glTF/SciFiHelmet.gltf";
  std::string IridescenceLamp = "IridescenceLamp/glTF/IridescenceLamp.gltf";
  std::string Corset = "Corset/glTF/Corset.gltf";


  // load in a gltf
  auto gltf = LoadGLTF(base_path + Sponza, *mMeshManager,
                       *mMaterialManager, *mTextureManager,
                       m_ForwardPipelineLayout, mEntities, false);
  if (gltf == GLTF_LOAD_FAIL) {
    std::cout << "Failed to load gltf\n";
    exit(1);
  }

  mPreviousClock = Clock_::now();
}

bool Renderer::Render() {
  if (glfwWindowShouldClose(mWindow.window)) {
    return false;
  }
  if (mRecreateSwapchain) {
    RecreateSwapchain();
  }
  // let GLFW process events
  glfwPollEvents();

  // record all the shadow maps
  for (size_t i = 0; i < mShadowLightManager->numShadowLights; i++) {
    mShadowLightManager->shadowLights[i]->record_shadow_map(mOpaqueMeshes);
  }

  // wait for the per-frame resources to finish and become available
  if (!mPerFrameResources->proceed_to_next_frame()) {
    mRecreateSwapchain = true;
    return true;
  }
  auto imageIndex = mPerFrameResources->get_image_index();

  // update user state
  auto const now = Clock_::now();
  auto const deltaTime =
      std::chrono::duration_cast<Secondsf_>(now - mPreviousClock).count();
  mPreviousClock = now;

  lut::update_user_state(mState, deltaTime, cfg::kCameraSettings);

  // prepare the per-scene data for this frame
  UpdateSceneUniforms();
  // lighting and light uniforms
  Light::update_lighting_uniforms();

  // update the VP^-1 matrix
  mVPInverse = glm::inverse(mSceneUniforms.viewProjection);

  // update the camera position
  mCameraPosition = glm::vec4(mState.cameraPos, 1);
  // choose the post process pipeline to use
  VkPipeline const *postProcessPipeline = nullptr;
  switch (mState.postProcessPipelineState) {
  case lut::PostProcessPipelineState::none:
    postProcessPipeline = &m_PostProcessPipeline;
    break;
  case lut::PostProcessPipelineState::mosaic:
    postProcessPipeline = &m_MosiacPipeline;
    break;
  case lut::PostProcessPipelineState::bloom:
    postProcessPipeline = &m_BloomHorizontalPipeline;
    break;
  default:
    assert(false);
  }
  // record the commands for this frame

  mPerFrameResources->begin_recording_commands();

  auto commandBuffer = mPerFrameResources->get_command_buffer();

  UploadPerSceneUniforms(commandBuffer);

  RecordForwardRenderPass(commandBuffer);

  std::vector<VkDescriptorSet> ppDescs = {mPostProcessDescriptors};
  if (mState.postProcessPipelineState == lut::PostProcessPipelineState::bloom) {
    std::vector<VkDescriptorSet> ppbloomDescriptors = {mPostProcessDescriptors,
                                                       mBloomDescriptors};
    // record another render pass for the bloom
    RecordPostProcessRenderPass(
        commandBuffer, mBloomRenderPass.handle, mBloomBufferFramebuffer.handle,
        mWindow.swapchainExtent, m_BloomVerticalPipeline,
        m_BloomVerticalPipelineLayout, ppDescs);

    RecordPostProcessRenderPass(
        commandBuffer, mPostProcessRenderPass.handle,
        mWindow.swapFramebuffers[imageIndex].handle, mWindow.swapchainExtent,
        m_BloomHorizontalPipeline, m_BloomHorizontalPipelineLayout,
        ppbloomDescriptors);
  } else {
    RecordPostProcessRenderPass(commandBuffer, mPostProcessRenderPass.handle,
                                mWindow.swapFramebuffers[imageIndex].handle,
                                mWindow.swapchainExtent, *postProcessPipeline,
                                mPostProcessPipeLayout.handle, ppDescs);
  }

  // end recording the commands
  mPerFrameResources->end_recording_commands();

  // submit the commands for this frame
  mPerFrameResources->submit_commands();

  // present rendered images.
  PresentResults(imageIndex);
  return true;
}

Renderer::~Renderer() {
  // wait for the device to finish
  vkDeviceWaitIdle(mWindow.device);
  // destroy the lights
  GraphicsThings::Light::destroy_lights();
  // delete mBasicPipeline;
  delete mAlphaPipeline;
  // delete mPostProcessPipeline;
  // delete mMosaicPipeline;
  // delete mBloomFirstPassPipeline;
  // delete mBloomSecondPassPipeline;
  delete mShadowLightManager;
  delete mPerFrameResources;
  delete mMeshManager;
  delete mMaterialManager;
  delete mTextureManager;

  // Destroy forward pass pbr pipeline + pipelinelayout
  vkDestroyPipeline(mWindow.device, m_ForwardPipeline, nullptr);
  vkDestroyPipelineLayout(mWindow.device, m_ForwardPipelineLayout, nullptr);

  // Destroy horizontal bloom pipeline + pipelinelayout
  vkDestroyPipeline(mWindow.device, m_BloomHorizontalPipeline, nullptr);
  vkDestroyPipelineLayout(mWindow.device, m_BloomHorizontalPipelineLayout,
                          nullptr);

  // Destroy vertical bloom pipeline + pipelinelayout
  vkDestroyPipeline(mWindow.device, m_BloomVerticalPipeline, nullptr);
  vkDestroyPipelineLayout(mWindow.device, m_BloomVerticalPipelineLayout,
                          nullptr);

  // Destroy mosiac pipeline + pipelinelayout
  vkDestroyPipeline(mWindow.device, m_MosiacPipeline, nullptr);
  vkDestroyPipelineLayout(mWindow.device, m_MosiacPipelineLayout, nullptr);

  // Destroy post process pipeline + pipelinelayout
  vkDestroyPipeline(mWindow.device, m_PostProcessPipeline, nullptr);
  vkDestroyPipelineLayout(mWindow.device, m_PostProcessPipelineLayout, nullptr);
}
void Renderer::RecreateSwapchain() {
  // re-create swapchain and associated resources!
  vkDeviceWaitIdle(mWindow.device);

  // recreate swapchain
  recreate_swapchain(mWindow);

  create_swapchain_framebuffers(mWindow, mPostProcessRenderPass.handle);
  mForwardRenderPass =
      create_render_pass_to_texture(mWindow, cfg::kDepthFormat);
  mPostProcessRenderPass =
      create_postprocess_render_pass(mWindow, true, cfg::kDepthFormat);
  mBloomRenderPass =
      create_postprocess_render_pass(mWindow, false, cfg::kDepthFormat);

  std::tie(mDepthBuffer, mDepthBufferView) =
      create_depth_buffer(mWindow, mAllocator, cfg::kDepthFormat);
  std::tie(mRenderTexture, mRenderTextureView) =
      create_render_texture(mWindow, mAllocator);
  std::tie(mBloomBuffer, mBloomBufferView) =
      create_render_texture(mWindow, mAllocator);
  mBloomBufferFramebuffer = create_framebuffer(mWindow, mBloomRenderPass.handle,
                                               {mBloomBufferView.handle});

  // create the render texture framebuffer
  mRenderTextureFramebuffer =
      create_framebuffer(mWindow, mForwardRenderPass.handle,
                         {mRenderTextureView.handle, mDepthBufferView.handle});

  // free the old post process descriptor set and reallocate
  vkFreeDescriptorSets(mWindow.device, mDynamicDescriptorPool.handle, 1,
                       &mPostProcessDescriptors);
  // free the old bloom descriptor set and reallocate
  vkFreeDescriptorSets(mWindow.device, mDynamicDescriptorPool.handle, 1,
                       &mBloomDescriptors);
  // reset the descriptor set pool
  vkResetDescriptorPool(mWindow.device, mDynamicDescriptorPool.handle, 0);
  mPostProcessDescriptors = create_post_process_descriptor_set(
      mWindow, mDynamicDescriptorPool, mRenderTextureView, mDepthBufferView,
      mPostProcessSampler, mPostProcessLayout);

  mBloomDescriptors = create_bloom_descriptor_set(
      mWindow, mDynamicDescriptorPool, mBloomBufferView, mPostProcessSampler,
      mBloomLayout);

  // recreate the pipelines
  PipelineBaseClass::recreate_all_pipelines();

  mRecreateSwapchain = false;
}
void Renderer::UpdateSceneUniforms() {
  float const aspect = float(mWindow.swapchainExtent.width) /
                       float(mWindow.swapchainExtent.height);

  mSceneUniforms.projection =
      glm::perspectiveRH_ZO(lut::Radians(cfg::kCameraFov).value(), aspect,
                            cfg::kCameraNear, cfg::kCameraFar);
  mSceneUniforms.projection[1][1] *= -1.f; // mirror the Y axis

  mSceneUniforms.view = glm::inverse(mState.camera2world);

  mSceneUniforms.viewProjection =
      mSceneUniforms.projection * mSceneUniforms.view;

  mSceneUniforms.cameraPosition = glm::vec4(mState.cameraPos, 1.f);
}
void Renderer::UploadPerSceneUniforms(VkCommandBuffer aCmdBuff) {
  for (auto const &uniform : mUbos) {
    lut::buffer_barrier(
        aCmdBuff, uniform.uniformBuffer, VK_ACCESS_UNIFORM_READ_BIT,
        VK_ACCESS_TRANSFER_WRITE_BIT, uniform.externalStageFlags,
        VK_PIPELINE_STAGE_TRANSFER_BIT);

    vkCmdUpdateBuffer(aCmdBuff, uniform.uniformBuffer, 0, uniform.uniformSize,
                      uniform.uniformData);

    lut::buffer_barrier(
        aCmdBuff, uniform.uniformBuffer, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_UNIFORM_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        uniform.externalStageFlags);
  }
}
void Renderer::RecordForwardRenderPass(VkCommandBuffer aCmdBuff) {
  // begin standard forward render pass
  VkClearValue clearValues[2]{};
  clearValues[0].color = cClearColor;

  clearValues[1].depthStencil.depth = 1.f;
  BeginRenderPass(aCmdBuff, mForwardRenderPass.handle,
                  mRenderTextureFramebuffer.handle, mWindow.swapchainExtent,
                  clearValues, 2);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)mWindow.swapchainExtent.width;
  viewport.height = (float)mWindow.swapchainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(aCmdBuff, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = {mWindow.swapchainExtent.width,
                    mWindow.swapchainExtent.height};
  vkCmdSetScissor(aCmdBuff, 0, 1, &scissor);

  // begin drawing
  // bind the scene descriptor set
  vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          m_ForwardPipelineLayout, 0, 1, &mSceneDescriptors, 0,
                          nullptr);
  // bind the lighting descriptor set
  vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          m_ForwardPipelineLayout, 2, 1, &mLightingDescriptors,
                          0, nullptr);

  // bind the shadow descriptor set
  vkCmdBindDescriptorSets(
      aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ForwardPipelineLayout, 3, 1,
      &mShadowLightManager->allShadowMapDescriptorSet, 0, nullptr);

  vkCmdBindPipeline(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    m_ForwardPipeline);
  //    for (const auto &mesh : mOpaqueMeshes) {
  //        mesh->record_draw(aCmdBuff);
  //    }
  //    vkCmdBindPipeline(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
  //                      *mAlphaPipeline->get_pipeline());
  //    for (const auto &mesh : mAlphaMeshes) {
  //        mesh->record_draw(aCmdBuff);
  //    }

  // record drawing the mesh manager
  vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          m_ForwardPipelineLayout, 1, 1,
                          &mMaterialDescriptorSets[0], 0, nullptr);
  for (auto &entity : mEntities) {
    entity.RecordDraw(aCmdBuff);
  }
  // end drawing
  vkCmdEndRenderPass(aCmdBuff);
}
void Renderer::BeginRenderPass(VkCommandBuffer const &aCmdBuff,
                               VkRenderPass const &aRenderPass,
                               VkFramebuffer const &aFramebuffer,
                               VkExtent2D const &aImageExtent,
                               VkClearValue *clearValues,
                               size_t numClearValues) {
  VkRenderPassBeginInfo passInfo{};
  passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  passInfo.renderPass = aRenderPass;
  passInfo.framebuffer = aFramebuffer;
  passInfo.renderArea.offset = VkOffset2D{0, 0};
  passInfo.renderArea.extent =
      VkExtent2D{aImageExtent.width, aImageExtent.height};
  passInfo.clearValueCount = numClearValues;
  passInfo.pClearValues = clearValues;

  vkCmdBeginRenderPass(aCmdBuff, &passInfo, VK_SUBPASS_CONTENTS_INLINE);
}
void Renderer::RecordPostProcessRenderPass(
    VkCommandBuffer aCmdBuff, VkRenderPass aPostProcessRenderPass,
    VkFramebuffer aSwapchainFramebuffer, const VkExtent2D &aImageExtent,
    VkPipeline const &aPPPipe, VkPipelineLayout aPostProcessPipelineLayout,
    const std::vector<VkDescriptorSet> &aPostProcessDescriptors) {
  // now we render the post process
  // begin render pass
  VkClearValue clearValuesPP[1]{};

  BeginRenderPass(aCmdBuff, aPostProcessRenderPass, aSwapchainFramebuffer,
                  aImageExtent, clearValuesPP, 1);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)mWindow.swapchainExtent.width;
  viewport.height = (float)mWindow.swapchainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(aCmdBuff, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = {mWindow.swapchainExtent.width,
                    mWindow.swapchainExtent.height};
  vkCmdSetScissor(aCmdBuff, 0, 1, &scissor);

  // begin drawing
  vkCmdBindPipeline(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, aPPPipe);

  // bind the descriptor set for the post process
  for (size_t i = 0; i < aPostProcessDescriptors.size(); ++i) {
    vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            aPostProcessPipelineLayout, i, 1,
                            &aPostProcessDescriptors[i], 0, nullptr);
  }

  // draw the post process
  vkCmdDraw(aCmdBuff, 3, 1, 0, 0);

  // end drawing
  vkCmdEndRenderPass(aCmdBuff);
}

void Renderer::PresentResults(std::uint32_t aImageIndex) {
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  VkSemaphore const waitSemaphores[] = {
      mPerFrameResources->get_render_finished_semaphore()};
  presentInfo.pWaitSemaphores = waitSemaphores;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &mWindow.swapchain;
  presentInfo.pImageIndices = &aImageIndex;
  presentInfo.pResults = nullptr;

  auto const presentRes = vkQueuePresentKHR(mWindow.presentQueue, &presentInfo);

  if (VK_SUBOPTIMAL_KHR == presentRes ||
      VK_ERROR_OUT_OF_DATE_KHR == presentRes) {
    mRecreateSwapchain = true;
  } else if (VK_SUCCESS != presentRes) {
    throw lut::Error("Can't present image\n"
                     "vkQueuePresentKHR() returned %s",
                     lut::to_string(presentRes).c_str());
  }
}
} // namespace GraphicsThings
