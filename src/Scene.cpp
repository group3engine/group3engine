#include "Scene.hpp"

#include <unordered_map>

#include <tracy/Tracy.hpp>

#include <glm/vec3.hpp>

#include "Jolt/Physics/Collision/Shape/ConvexHullShape.h"
#include "Jolt/Physics/Collision/Shape/MeshShape.h"
#include "ResourceManager.hpp"

#include "ImGuiRenderer.hpp"
#include "SampleGLTFFilePaths.hpp"

void Scene::AddLightSource(Light &LightSource) {
    m_Lights.push_back(std::move(LightSource));
}

void Scene::Update(double aDeltaTime) {
    ZoneScopedN("Scene::Update");

    // update the entities
    for(auto &entity : m_Entities) {
        entity->BaseUpdate(aDeltaTime);
        entity->Update(aDeltaTime);
    }
    // late update the entities
    for(auto &entity : m_Entities) {
        entity->LateUpdate(aDeltaTime);
    }

    for (auto& light : m_Lights)
    {
        glm::mat4 ortho = glm::ortho(-light.view, light.view, -light.view, light.view, light.near, light.far);
        glm::mat4 view = glm::lookAt(glm::vec3(light.position), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0));
        light.LightSpaceMatrix = ortho * view;
    }

    // Fill GPU Data with data defined for the scene
    for (size_t i = 0; i < m_Lights.size(); i++) {
        m_LightBuffer.lights[i].type = static_cast<int>(m_Lights[i].Type);
        m_LightBuffer.lights[i].LightPosition = m_Lights[i].position;
        m_LightBuffer.lights[i].LightColour = m_Lights[i].colour;
        m_LightBuffer.lights[i].LightSpaceMatrix = m_Lights[i].LightSpaceMatrix;
    }

    UpdateCameraTransforms();
}

void Scene::UpdateCameraTransforms() {
    for (size_t i = 0; i < mPlayerCount; ++i) {
        float width = static_cast<float>(mContext->extent.width);
        float height = static_cast<float>(mContext->extent.height);

        auto *camera = mCameras[i];
        const auto &pos = camera->GetPosition();
        const auto &dir = camera->GetDirection();
        const auto &up = camera->GetUp();

        if (mPlayerCount == 2) {
            width = static_cast<float>(width) / mPlayerCount;
            height = static_cast<float>(height);
        }

        auto &playerCameraTransform = mPlayerCameraTransforms[i];
        playerCameraTransform.view = glm::lookAt(pos, pos + dir, up);
        playerCameraTransform.projection =
            glm::perspective(playerCameraTransform.fov, width / height,
                             playerCameraTransform.nearPlane, playerCameraTransform.farPlane);
        playerCameraTransform.projection[1][1] *= -1;
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

    for (size_t i = 0; i < mPlayerCount; ++i) {
        auto &cameraUBO = mPlayerCameraUbos[i];
        cameraUBO[vkutil::currentFrame].Upload(cmdBuff, &mPlayerCameraTransforms[i], size);
    }
}

void Scene::UploadLights(VkCommandBuffer cmdBuff) {
    // Pass the light data to the GPU to update all light properties
    m_LightUBO[vkutil::currentFrame].Upload(cmdBuff, &m_LightBuffer, sizeof(vkutil::LightBuffer));
}

void Scene::UpdateUi(double aDeltaTime) {
    ZoneScopedN("Scene::UpdateUi");

    // New timer window
    mGuiTimerData.time += aDeltaTime;
    ImGuiRenderer::NewTimer(mGuiTimerData);

    for (auto &entity : m_Entities) {
        entity->UpdateUi(aDeltaTime);
    }
}

void Scene::Unload()
{
    // delete the entities
    for (auto &entity : m_Entities) {
        delete entity;
    }
    m_Entities.clear();

    m_FrontMeshes.clear();
    m_BackMeshes.clear();
    m_Lights.clear();
    m_Entities.clear();
    m_Animations.clear();
    m_Skins.clear();

    mHasCharacter = false;
    mCharacter = nullptr;
}

void Scene::LoadGLTF(const std::filesystem::path &aFilepath) {
    // Load the GLTF file
    ResourceLoader::LoadGLTF(aFilepath, *mMeshManager, *mMaterialManager,
                             *mTextureManager, m_Entities, false, m_Animations,
                             m_Skins);
}

void Scene::Load(const std::filesystem::path &filePath)
{
    mSceneFilename = filePath.stem();

    // Current path is the current working directory, i.e., where the root CMakeLists.txt is
    std::filesystem::path basePath = std::filesystem::path(CMAKE_SOURCE_DIR) / "assets";
    std::filesystem::path gltfPath = basePath / filePath;

    // Define Light sources
    Light directionalLight;
    directionalLight.Type = LightType::Directional;
    directionalLight.position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    directionalLight.colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    std::vector<glm::vec4> spotLightPositions;

    // Random spot light positions put side by side each other
    for (size_t i = 0; i < 25; i++) {
        spotLightPositions.push_back(glm::vec4(-9.0 + i * 0.8, 4.4f, 0.5f, 1.0f));
    }

    // Create the scene which will store models and lights
    // Add GLTF to the scene
    // Add a directional light source defined earlier
    LoadGLTF(gltfPath);
    AddLightSource(directionalLight);

    // Loop through the positions and instantiate a light
    // and pass to the scene to add the lights to the scene
    for (const auto &position : spotLightPositions) {
        Light spotLight = {};
        spotLight.Type = LightType::Spot;
        spotLight.position = position;
        spotLight.colour = glm::vec4(glm::linearRand(0.0f, 1.0f), glm::linearRand(0.0f, 1.f),
                                     glm::linearRand(0.0f, 1.0f), 1.0f);
        AddLightSource(spotLight);
    }

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
        } 
        else // if the entity isnt the character (NPCs, Obstacles, Moving platforms etc)
        {
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
                SPDLOG_WARN("Entity {} does not have mesh", entity->GetName());
                continue;
            }


            size_t totalVertices = 0;
            size_t totalTriangles = 0;
            // if the entity has physics (either as it is solid or is a sensor and needs collision response)
            if(entity->IsSensor() || entity->IsSolid()) 
            {
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

                // for all primatives in the mesh
                for (const auto &primitive : mesh->meshPrimitives) 
                {
                    // Create an array of vertices (and a copy in the points format) and the list of indexed faces
                    VertexList vertices;
                    vector<JPH::Vec3> list_of_points;
                    IndexedTriangleList indexedTriangles;

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
                        // add the indexed face to the list
                        indexedTriangles.push_back(IndexedTriangle(primitive.indices[i],
                                                                   primitive.indices[i + 1],
                                                                   primitive.indices[i + 2]));
                        ++totalTriangles;
                    }

                    // make sure the lists contain something
                    assert(!vertices.empty());
                    assert(!indexedTriangles.empty());

                    if(entity->GetPhysicsType() == PhysicsType::STATIC)
                    {
                        // Create the settings object for a mesh shape
                        JPH::MeshShapeSettings settings(vertices, indexedTriangles);

                        // Create shape
                        JPH::Shape::ShapeResult result = settings.Create();
                        if(result.IsValid()) // if the shape is valid
                        {
                            shape = result.Get(); // set the shape as the result
                        } 
                        else // if it isnt valid
                        {
                            // give an error statement
                            SPDLOG_ERROR("Shape result is invalid. {}", result.GetError());
                        }

                        // get the transform for the entity's physics rigid body
                        Transform entity_transform = entity->GetWorldTransformComponents();

                        // set the motion type as static
                        EMotionType motionType = EMotionType::Static;

                        // Set the body creation settings as static and not moving
                        BodyCreationSettings bodyCreationSettings = {
                        result.Get(), RVec3(entity_transform.translation.x, entity_transform.translation.y, entity_transform.translation.z), Quat(entity_transform.rotation.x, entity_transform.rotation.y, entity_transform.rotation.z, entity_transform.rotation.w),
                        motionType, Layers::NON_MOVING};

                        // set if it is a sensor
                        bodyCreationSettings.mIsSensor = entity->IsSensor();

                        // make the rigid body with the settings
                        auto entity_rigid_body = std::make_unique<RigidBody>(bodyCreationSettings);

                        // initialise the body in the physics manager and do not activate it
                        entity_rigid_body->Init(PhysicsManager::get(), false);

                        if(entity->IsSensor())
                        {
                            // only do this part if its supposed to DO something when collided with (i.e. sensors)
                            PhysicsManager::get().RegisterEntity(entity, entity_rigid_body->mBodyId);
                        }
                        
                        PhysicsManager::get().mPhysicsSystem.OptimizeBroadPhase();
                        entity->AddRigidBody(std::move(entity_rigid_body));
                    }
                    else if(entity->GetPhysicsType() == PhysicsType::KINEMATIC)
                    {
                        // Create the settings object for a mesh shape
                        JPH::MeshShapeSettings settings(vertices, indexedTriangles);

                        // Create shape
                        JPH::Shape::ShapeResult result = settings.Create();
                        if(result.IsValid()) // if the shape is valid
                        {
                            shape = result.Get(); // set the shape as the result
                        } 
                        else // if it isnt valid
                        {
                            // give an error statement
                            SPDLOG_ERROR("Shape result is invalid. {}", result.GetError());
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
                        auto entity_rigid_body = std::make_unique<RigidBody>(bodyCreationSettings);

                        // initialise the body in the physics manager and activate it
                        entity_rigid_body->Init(PhysicsManager::get(), true);

                        if(entity->IsSensor())
                        {
                            // only do this part if its supposed to DO something when collided with (i.e. sensors)
                            PhysicsManager::get().RegisterEntity(entity, entity_rigid_body->mBodyId);
                        }
                        PhysicsManager::get().mPhysicsSystem.OptimizeBroadPhase();
                        entity->AddRigidBody(std::move(entity_rigid_body));
                    }
                    else if(entity->GetPhysicsType() == PhysicsType::DYNAMIC)
                    {
                        // Create the settings object for a mesh shape
                        JPH::ConvexHullShapeSettings consettings(list_of_points.data(), list_of_points.size());

                        // Create shape
                        JPH::Shape::ShapeResult result = consettings.Create();
                        if(result.IsValid()) // if the shape is valid
                        {
                            shape = result.Get(); // set the shape as the result
                        } 
                        else // if it isnt valid
                        {
                            // give an error statement
                            SPDLOG_ERROR("Shape result is invalid. {}", result.GetError());
                        }

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
                        auto entity_rigid_body = std::make_unique<RigidBody>(bodyCreationSettings);

                        // initialise the body in the physics manager and activate it
                        entity_rigid_body->Init(PhysicsManager::get(), true);

                        if(entity->IsSensor())
                        {
                            // only do this part if its supposed to DO something when collided with (i.e. sensors)
                            PhysicsManager::get().RegisterEntity(entity, entity_rigid_body->mBodyId);
                        }
                        PhysicsManager::get().mPhysicsSystem.OptimizeBroadPhase();
                        entity->AddRigidBody(std::move(entity_rigid_body));
                    }

                }
            }

            SPDLOG_INFO("total vertices {}", totalVertices);
            SPDLOG_INFO("total triangles {}", totalTriangles);
        }
    }

}

void Scene::Awake()
{
    // call the awake function on all entities
    for (auto &entity : m_Entities) {
        entity->SetScene(this);
        entity->Awake();
    }

    for (auto &playerCameraTransform : mPlayerCameraTransforms) {
        playerCameraTransform.nearPlane = 0.1f;
        playerCameraTransform.farPlane = 100.0f;
        playerCameraTransform.fov = 45.0f;
    }

    bool hasSetActive = false;
    for (auto &entity : m_Entities) {
        if (entity->IsCharacter()) {
            auto *character = static_cast<CharacterEntity*>(entity);
            Camera *camera = character->GetCamera();

            if (!hasSetActive) {
                camera->SetIsActive(true);
                hasSetActive = true;
            }

            Entity *cameraEntity = static_cast<Entity*>(camera);
            m_Entities.push_back(cameraEntity);
        }
    }
}

Camera *Scene::GetActiveCamera() {
    for (auto &entity : m_Entities) {
        if (entity->IsCharacter()) {
            auto *character = static_cast<CharacterEntity*>(entity);
            Camera *camera = character->GetCamera();

            if (camera->IsActive()) {
                return camera;
            }
        }
    }

    SPDLOG_ERROR("Active camera not found.");
    std::exit(EXIT_FAILURE);
}

void Scene::SwitchCamera() {
    CharacterEntity *activeCharacter = nullptr;
    CharacterEntity *inactiveCharacter = nullptr;
    for (auto &entity : m_Entities) {
        if (entity->IsCharacter()) {
            auto *character = static_cast<CharacterEntity*>(entity);
            Camera *camera = character->GetCamera();

            if (camera->IsActive()) {
                activeCharacter = character;
            } else {
                inactiveCharacter = character;
            }
        }
    }

    activeCharacter->GetCamera()->SetIsActive(false);
    inactiveCharacter->GetCamera()->SetIsActive(true);
    mCharacter = inactiveCharacter;
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

    m_LightUBO.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
    // Light uniform buffers
    for (auto &buffer : m_LightUBO) {
        buffer = CreateBuffer("LightUBO", *mContext, sizeof(vkutil::LightBuffer),
                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                              VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                  VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                                  VMA_ALLOCATION_CREATE_MAPPED_BIT);
    }

    mCurrentScene = this;
}

void Scene::ShutDown() {
    for (auto &cameraUBO : mPlayerCameraUbos) {
        for (auto &buffer : cameraUBO) {
            buffer.Destroy();
        }
    }

    for (auto &buffer : m_LightUBO) {
        buffer.Destroy();
    }
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
                          VkPipelineLayout pipelineLayout) {
    for (auto& entity : m_Entities) {
        entity->RecordDrawShadow(cmd, pipelineLayout);
    }
}

void Scene::DrawSkinned(VkCommandBuffer cmd,
                        VkPipelineLayout pipelineLayout) {
    for (auto &entity : m_Entities) {
        entity->RecordDrawSkinned(cmd, pipelineLayout);
    }
}
