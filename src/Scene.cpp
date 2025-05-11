#include "Scene.hpp"

#include <unordered_map>

#include <tracy/Tracy.hpp>

#include <glm/vec3.hpp>

#include "Engine.hpp"
#include "Jolt/Physics/Collision/Shape/ConvexHullShape.h"
#include "Jolt/Physics/Collision/Shape/MeshShape.h"
#include "ResourceManager.hpp"

#include "ImGuiRenderer.hpp"
#include "SampleGLTFFilePaths.hpp"

#include "RenderPassCommon.hpp"
#include "Saving.hpp"

#include "Debugging.hpp"

#include "NetworkEntitiesManager.hpp"

namespace {
JPH::Shape::ShapeResult CreateMeshShapeResult(VertexList inVertices,
                                              IndexedTriangleList inTriangles) {
    // Create the settings object for a mesh shape
    JPH::MeshShapeSettings settings(inVertices, inTriangles);

    // Create shape
    JPH::Shape::ShapeResult result = settings.Create();
    if (!result.IsValid()) // if the shape is valid
    {
        // give an error statement
        SPDLOG_ERROR("Shape result is invalid. {}", result.GetError());
    }

    return result;
}

JPH::Shape::ShapeResult CreateConvexHullShapeResult(const Vec3 *inPoints, int inNumPoints) {
    // Create the settings object for a mesh shape
    JPH::ConvexHullShapeSettings consettings(inPoints, inNumPoints);

    // Create shape
    JPH::Shape::ShapeResult result = consettings.Create();
    if (!result.IsValid()) // if the shape is valid
    {
        // give an error statement
        SPDLOG_ERROR("Shape result is invalid. {}", result.GetError());
    }

    return result;
}
} // namespace

void Scene::Update(double aDeltaTime) {
    ZoneScopedN("Scene::Update");
    float timeScale = Engine::get().GetTimeScale();

    GlobalUtil::totalTime += aDeltaTime;

    // unscaled update the entities
    for(auto &entity : m_Entities) {
        entity->UnscaledUpdate(GlobalUtil::unscaledDeltaTime);
    }
    for(auto &entity : m_Entities) {
        entity->BaseUpdate(aDeltaTime);
        entity->PreUpdate(aDeltaTime);
        entity->Update(aDeltaTime);
    }
    // late update the entities
    for(auto &entity : m_Entities) {
        entity->LateUpdate(aDeltaTime);
    }


    LightManager::getInstance().Update();
    UpdateCameraTransforms();
}

void Scene::UpdateCameraTransforms() {
    size_t activePlayerCount = GetActivePlayerCount();
    for (size_t i = 0; i < activePlayerCount; ++i) {
        ViewportSize viewportSize = CalcViewportSize(mContext->extent, activePlayerCount, i);

        float width = viewportSize.width;
        float height = viewportSize.height;

        auto *camera = mCameras[i];
        const auto &pos = camera->GetPosition();
        const auto &dir = camera->GetDirection();
        const auto &up = camera->GetUp();

        auto &playerCameraTransform = mPlayerCameraTransforms[i];
        playerCameraTransform.view = glm::lookAt(pos, pos + dir, up);
        playerCameraTransform.projection = glm::perspective(playerCameraTransform.fov, width / height, playerCameraTransform.nearPlane, playerCameraTransform.farPlane);
        playerCameraTransform.projection[1][1] *= -1;
        playerCameraTransform.inverseProjection = glm::inverse(playerCameraTransform.projection);
        playerCameraTransform.inverseView = glm::inverse(playerCameraTransform.view);
        playerCameraTransform.cameraPosition = glm::vec4(pos.x, pos.y, pos.z, 1.0);
        playerCameraTransform.viewportSize = glm::vec2(width, height);
        playerCameraTransform.nearPlane = playerCameraTransform.nearPlane;
        playerCameraTransform.farPlane = playerCameraTransform.farPlane;
        playerCameraTransform.fov = playerCameraTransform.fov;
    }
}

void Scene::UploadCameras(VkCommandBuffer cmdBuff) {
    // Write new data to the buffer to update uniform
    VkDeviceSize size = sizeof(CameraTransform);

    for (size_t i = 0; i < GetActivePlayerCount(); ++i) {
        auto &cameraUBO = mPlayerCameraUbos[i];
        cameraUBO[vkutil::currentFrame].Upload(cmdBuff, &mPlayerCameraTransforms[i], size);
    }
}


void Scene::UpdateUi(double aDeltaTime) {
    ZoneScopedN("Scene::UpdateUi");

    for (auto &entity : m_Entities) {
        entity->UpdateUi(aDeltaTime);
    }

    ImGuiRenderer::NewActivePlayerCountOverride(GetActiveScene(),
                                                mGuiActivePlayerCountOverride);
}

void Scene::Unload()
{
    // delete the entities
    for (auto *entity : m_Entities) {
        delete entity;
    }
    m_Entities.clear();

    for (auto &[parent, children] : mCharacterEntities) {
        delete parent;
        for (auto *child : children) {
            delete child;
        }
    }
    mCharacterEntities.clear();

    m_FrontMeshes.clear();
    m_BackMeshes.clear();
    m_Animations.clear();
    m_Skins.clear();

    mPlayerCount = 0;
    mActivePlayerCount = 0;
    mActivePlayerCountOverride = {Override::INACTIVE, 1};

    mCameras.clear();

    // Zero out camera transforms for safety
    for (auto &cameraTransform : mPlayerCameraTransforms) {
        cameraTransform = {};
    }

    mSceneFilename = "";

    mGuiActivePlayerCountOverride = {};

    mSignalSystem.Clear();

    mNetworkEntitiesManager = nullptr;

    kEntityCount = 0;
}

void Scene::LoadGLTF(const std::filesystem::path &aFilepath, size_t playerCount) {
    // Load the GLTF file
    ResourceLoader::LoadGLTF(aFilepath, *mMeshManager, *mMaterialManager,
                             *mTextureManager, m_Entities, false, m_Animations,
                             m_Skins, mCharacterEntities, this);

    size_t playersAddedCount = 0;
    // Add each character entity and its children until the player count is reached
    for (auto &[parent, children] : mCharacterEntities) {
        m_Entities.push_back(parent);
        for (auto *child : children) {
            m_Entities.push_back(child);
        }

        ++playersAddedCount;
        if (playersAddedCount >= playerCount) {
            break;
        }
    }

    if(!mCharacterEntities.empty())
        mCharacterEntities.erase(mCharacterEntities.begin(), std::next(mCharacterEntities.begin(), playerCount));

    if (playersAddedCount < playerCount) {
        SPDLOG_ERROR("Failed to add the selected number of players ({}). Only {} players were "
                     "found in the character entities list. Add more characters to the scene.",
                     playerCount, playersAddedCount);
        std::exit(EXIT_FAILURE);
    }

    assert(playersAddedCount > 0 && playersAddedCount <= playerCount);
}

void Scene::Load(const std::filesystem::path &filePath, size_t playerCount)
{
    mSceneFilename = filePath.stem();
    std::filesystem::path saveFilename = "save_";
    saveFilename += Scene::get().GetActiveScene()->GetSceneFilename();
    saveFilename += ".json";
    Saving::get().UpdateSceneName(saveFilename);

    // Current path is the current working directory, i.e., where the root CMakeLists.txt is
    std::filesystem::path basePath = assetsPath;
    std::filesystem::path gltfPath = basePath / filePath;

    LoadGLTF(gltfPath, playerCount);

    SPDLOG_DEBUG("Number of Lights: {}", GetLights().size());

    // --- Mesh shape for each entity ----------------------------------------

    // Shapes are refcounted and can be shared between bodies
    JPH::Ref<Shape> shape;

    // for all entities in the scene
    for (auto it = GetEntities().begin(); it != GetEntities().end(); ++it)
    {
        const auto &entity = *it;

        // if the entity is the character
        if (entity->IsCharacter())
        {
            // we skip it and handle it later
            SPDLOG_INFO("Skipping character");
            continue;
        }

        // if the entity isnt the character (NPCs, Obstacles, Moving platforms etc)
        // if the entity has an animator
        if (entity->HasAnimator())
        {
            // also skip it
            SPDLOG_INFO("Skipping entity {}, as it has an animator", entity->GetName());
            continue;
        }

        // (try to) get the entity's mesh
        const auto *mesh = entity->GetMesh();

        // if it doesnt have a mesh
        if (!mesh)
        {
            // skip the entity
            continue;
        }

        // if the entity has physics (either as it is solid or is a sensor and needs collision response)
        bool hasPhysics = entity->IsSensor() || entity->IsSolid();
        // Skip if the entity does not have physics
        if (!hasPhysics) {
            continue;
        }

        size_t totalVertices = 0;
        size_t totalTriangles = 0;
        // calculate the transforms for the rigid body
        glm::mat4 entityWorldTransform = entity->GetWorldTransform();

        // decompose the world transform
        glm::vec3 position, scale;
        glm::quat rotation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(entityWorldTransform, scale, rotation, position, skew, perspective);

        // create a scaling matrix
        glm::mat4 scalingMatrix = glm::scale(glm::mat4(1.0f), scale);

        // Create an array of vertices (and a copy in the points format) and the list of indexed faces
        VertexList vertices;
        vector<JPH::Vec3> list_of_points;
        IndexedTriangleList indexedTriangles;

        size_t indexOffset = 0;
        // Construct a unified list of vertices and indices from each mesh primitive
        for (const auto &primitive : mesh->meshPrimitives)
        {
            // for all vertices
            for (const auto &vertex : primitive.vertices)
            {
                // calculate the position of the vertex in world space
                auto worldPos = scalingMatrix * glm::vec4(vertex.pos, 1.0f);

                // add it to the list
                vertices.emplace_back(worldPos.x, worldPos.y, worldPos.z);
                list_of_points.emplace_back(Vec3(worldPos.x, worldPos.y, worldPos.z));

                ++totalVertices;
            }

            // for all indexed faces
            for (size_t i = 0; i < primitive.indices.size(); i += 3)
            {
                // Offset the index to make sure the vertices we are indexing are from this mesh primitive
                indexedTriangles.push_back(IndexedTriangle(primitive.indices[i] + indexOffset,
                                                            primitive.indices[i + 1] + indexOffset,
                                                            primitive.indices[i + 2] + indexOffset));
                ++totalTriangles;
            }

            // Once we have read all the vertices for a mesh primitive then
            // offset by this many vertices when calculating the indices of the
            // next mesh primitive's vertices
            indexOffset += primitive.vertices.size();
        }

        // make sure the lists contain something
        assert(!vertices.empty());
        assert(!indexedTriangles.empty());

        if(entity->GetPhysicsType() == PhysicsType::STATIC)
        {
            Shape::ShapeResult result;
            if (entity->GetColliderType() == ColliderType::MeshShape ||
                entity->GetColliderType() == ColliderType::Fallback) {
                result = CreateMeshShapeResult(vertices, indexedTriangles);
            } else if (entity->GetColliderType() == ColliderType::ConvexHullShape) {
                result = CreateConvexHullShapeResult(list_of_points.data(), list_of_points.size());
            }

            // get the transform for the entity's physics rigid body
            Transform entity_transform = entity->GetWorldTransformComponents();

            // set the motion type as static
            EMotionType motionType = EMotionType::Static;

            // if its a sensor set the layer to SENSOR, otherwise set it to NON_MOVING
            ObjectLayer layer = entity->IsSensor() ? Layers::SENSORS : Layers::NON_MOVING;

            // Set the body creation settings as static and not moving
            BodyCreationSettings bodyCreationSettings = {
            result.Get(), RVec3(entity_transform.translation.x, entity_transform.translation.y, entity_transform.translation.z), Quat(entity_transform.rotation.x, entity_transform.rotation.y, entity_transform.rotation.z, entity_transform.rotation.w),
            motionType, layer};

            // set if it is a sensor
            bodyCreationSettings.mIsSensor = entity->IsSensor();

            // make the rigid body with the settings
            entity->AddRigidBody(bodyCreationSettings);
        }
        else if(entity->GetPhysicsType() == PhysicsType::KINEMATIC)
        {
            Shape::ShapeResult result;
            if (entity->GetColliderType() == ColliderType::MeshShape) {
                result = CreateMeshShapeResult(vertices, indexedTriangles);
            } else if (entity->GetColliderType() == ColliderType::ConvexHullShape ||
                       entity->GetColliderType() == ColliderType::Fallback) {
                result = CreateConvexHullShapeResult(list_of_points.data(), list_of_points.size());
            }

            // get the transform for the entity's physics rigid body
            Transform entity_transform = entity->GetWorldTransformComponents();

            // set the motion type as kinematic
            EMotionType motionType = EMotionType::Kinematic;

            // Set the body creation settings as kinematic and moving
            BodyCreationSettings bodyCreationSettings = {
            result.Get(), RVec3(entity_transform.translation.x, entity_transform.translation.y, entity_transform.translation.z), Quat(entity_transform.rotation.x, entity_transform.rotation.y, entity_transform.rotation.z, entity_transform.rotation.w),
            motionType, Layers::MOVING};

            // set information about the body's physical properties
            bodyCreationSettings.mMassPropertiesOverride.mMass = 1.0f;
            bodyCreationSettings.mMassPropertiesOverride.mInertia =
                JPH::Mat44::sIdentity();
            bodyCreationSettings.mOverrideMassProperties =
                EOverrideMassProperties::MassAndInertiaProvided;

            // set if it is a sensor
            bodyCreationSettings.mIsSensor = entity->IsSensor();

            // make the rigid body with the settings
            entity->AddRigidBody(bodyCreationSettings);
        }
        else if(entity->GetPhysicsType() == PhysicsType::DYNAMIC)
        {
            DEBUG_ASSERT(entity->GetColliderType() == ColliderType::Fallback ||
                         entity->GetColliderType() == ColliderType::ConvexHullShape);

            Shape::ShapeResult result =
                CreateConvexHullShapeResult(list_of_points.data(), list_of_points.size());

            // get the transform for the entity's physics rigid body
            Transform entity_transform = entity->GetWorldTransformComponents();

            // set the motion type as dynamic
            EMotionType motionType = EMotionType::Dynamic;

            // Set the body creation settings as dynamic and moving
            BodyCreationSettings bodyCreationSettings = {
            result.Get(), RVec3(entity_transform.translation.x, entity_transform.translation.y, entity_transform.translation.z), Quat(entity_transform.rotation.x, entity_transform.rotation.y, entity_transform.rotation.z, entity_transform.rotation.w),
            motionType, Layers::MOVING};

            // set information about the body's physical properties
            bodyCreationSettings.mMassPropertiesOverride.mMass = 1.0f;
            bodyCreationSettings.mMassPropertiesOverride.mInertia =
                JPH::Mat44::sIdentity();
            bodyCreationSettings.mOverrideMassProperties =
                EOverrideMassProperties::MassAndInertiaProvided;

            // set if it is a sensor
            bodyCreationSettings.mIsSensor = entity->IsSensor();

            // make the rigid body with the settings
            entity->AddRigidBody(bodyCreationSettings);
        }

        // SPDLOG_INFO("total vertices {}", totalVertices);
        // SPDLOG_INFO("total triangles {}", totalTriangles);
    }

}

void Scene::Awake()
{
    // Find the network entities manager
    mIsMultiplayer = false;
    for (auto &entity : m_Entities) {
        if (entity->CompareType("NetworkEntitiesManager")) {
            assert(!mNetworkEntitiesManager);
            mNetworkEntitiesManager = static_cast<NetworkEntitiesManager *>(entity);
            mIsMultiplayer = true;
        }
    }
    if (mNetworkEntitiesManager) {
        SPDLOG_INFO("Scene found network entities manager, this is a networked game.");
    } else {
        SPDLOG_INFO("Scene did not find a network entities manager, this is a local game.");
    }

    // Set the scene and call init physics on all entities
    for (auto &entity : m_Entities) {
        entity->SetScene(this);
        entity->InitPhysics();
    }

    // Call the awake function on all entities
    // Make sure all entities have had their physics intialised before this
    for (auto &entity : m_Entities) {
        entity->Awake();
    }

    // Optimize broad phase now all rigid bodies have been added
    PhysicsManager::get().mPhysicsSystem.OptimizeBroadPhase();

    for (auto &playerCameraTransform : mPlayerCameraTransforms) {
        playerCameraTransform.nearPlane = 0.1f;
        playerCameraTransform.farPlane = 1000.0f;
        playerCameraTransform.fov = 45.0f;
    }

    size_t scenePlayerCount = 0;

    for (auto &entity : m_Entities) {
        if (entity->IsCharacter()) {
            ++scenePlayerCount;
            SPDLOG_INFO("Adding character {}", entity->GetName());
        }
    }

    // TODO: Set the active player count to be the number of characters in the scene.
    // This can change in the future to be 1 at first and then players join in
    SetActivePlayerCount(scenePlayerCount);
}

void Scene::StartUp(Context *context, MaterialManager *materialManager,
                    MeshManager *meshManager, TextureManager *textureManager) {
    mContext = context;
    mMaterialManager = materialManager;
    mMeshManager = meshManager;
    mTextureManager = textureManager;

    for (auto &cameraUBO : mPlayerCameraUbos) {
        cameraUBO.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
        for (auto &buffer : cameraUBO) {
            buffer = CreateBuffer("cameraUBO", *mContext, sizeof(CameraTransform),
                                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                      VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                                      VMA_ALLOCATION_CREATE_MAPPED_BIT);
        }
    }

    // call startup on the light manager
    LightManager::getInstance().StartUp(*mContext);

    mCurrentScene = this;
}

void Scene::ShutDown() {
    for (auto &cameraUBO : mPlayerCameraUbos) {
        for (auto &buffer : cameraUBO) {
            buffer.Destroy();
        }
    }

    // call shutdown on the light manager
    LightManager::getInstance().Destroy();
}

void Scene::DrawOpaque(VkCommandBuffer cmd,
                       VkPipelineLayout pipelineLayout) {
    for (auto &entity : m_Entities) {
        entity->RecordDrawOpaque(cmd, pipelineLayout);
    }
}

void Scene::DrawAlphaMasked(VkCommandBuffer cmd,
                            VkPipelineLayout pipelineLayout) {
    for (auto &entity : m_Entities) {
        entity->RecordDrawCutout(cmd, pipelineLayout);
    }
}
void Scene::DrawShadowMap(VkCommandBuffer cmd,
                          VkPipelineLayout pipelineLayout, uint32_t cascadeIndex) {
    for (auto& entity : m_Entities) {
        entity->RecordDrawShadow(cmd, pipelineLayout, cascadeIndex);
    }
}

void Scene::DrawSkinned(VkCommandBuffer cmd,
                        VkPipelineLayout pipelineLayout, uint32_t cascadeIndex) {
    for (auto &entity : m_Entities) {
        entity->RecordDrawSkinned(cmd, pipelineLayout, cascadeIndex);
    }
}

void Scene::DrawLava(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout) {
    for (auto *entity : m_Entities) {
        if (entity->CompareTag("lava")) {
            entity->RecordDrawLava(cmd, pipelineLayout);
        }
    }
}
