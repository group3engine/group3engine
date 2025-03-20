#include "Scene.hpp"

#include <unordered_map>

#include <tracy/Tracy.hpp>

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

    // Pass the light data to the GPU to update all light properties
    m_LightUBO[vkutil::currentFrame].WriteToBuffer(m_LightBuffer, sizeof(vkutil::LightBuffer));
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

void Scene::Destroy()
{
	for (auto& buffer : m_LightUBO)
	{
		buffer.Destroy();
	}
        // delete the mesh manager, material manager and texture manager
        delete mMeshManager;
        delete mMaterialManager;
        delete mTextureManager;

        // delete the entities
        for (auto &entity : m_Entities) {
            delete entity;
        }
        m_Entities.clear();
}

void Scene::Load(const std::filesystem::path &aFilepath) {
    // Load the GLTF file
    LoadGLTF(aFilepath, *mMeshManager, *mMaterialManager, *mTextureManager,
             m_Entities, false, m_Animations, m_Skins);

}

void Scene::Initialise()
{
    // Current path is the current working directory, i.e., where the root CMakeLists.txt is
    std::filesystem::path basePath = std::filesystem::path(CMAKE_SOURCE_DIR) / "assets";
    std::filesystem::path gltfPath = basePath / Sample::SampleObby;

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
    Load(gltfPath);
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
                        RigidBody entity_rigid_body = RigidBody(bodyCreationSettings);

                        // initialise the body in the physics manager and do not activate it
                        entity_rigid_body.Init(PhysicsManager::get(), false);

                        if(entity->IsSensor())
                        {
                            // only do this part if its supposed to DO something when collided with (i.e. sensors)
                            PhysicsManager::get().RegisterEntity(entity, entity_rigid_body.mBodyId);
                        }
                        
                        PhysicsManager::get().mPhysicsSystem.OptimizeBroadPhase();
                        entity->AddRigidBody(std::make_unique<RigidBody>(entity_rigid_body));
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
                        RigidBody entity_rigid_body = RigidBody(bodyCreationSettings);

                        // initialise the body in the physics manager and activate it
                        entity_rigid_body.Init(PhysicsManager::get(), true);

                        if(entity->IsSensor())
                        {
                            // only do this part if its supposed to DO something when collided with (i.e. sensors)
                            PhysicsManager::get().RegisterEntity(entity, entity_rigid_body.mBodyId);
                        }
                        PhysicsManager::get().mPhysicsSystem.OptimizeBroadPhase();
                        entity->AddRigidBody(std::make_unique<RigidBody>(entity_rigid_body));
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
                        RigidBody entity_rigid_body = RigidBody(bodyCreationSettings);

                        // initialise the body in the physics manager and activate it
                        entity_rigid_body.Init(PhysicsManager::get(), true);

                        if(entity->IsSensor())
                        {
                            // only do this part if its supposed to DO something when collided with (i.e. sensors)
                            PhysicsManager::get().RegisterEntity(entity, entity_rigid_body.mBodyId);
                        }
                        PhysicsManager::get().mPhysicsSystem.OptimizeBroadPhase();
                        entity->AddRigidBody(std::make_unique<RigidBody>(entity_rigid_body));
                    }

                }
            }

            SPDLOG_INFO("total vertices {}", totalVertices);
            SPDLOG_INFO("total triangles {}", totalTriangles);
        }
    }


    // Find character
    auto &entities = GetEntities();
    auto it = std::find_if(entities.begin(), entities.end(),
                           [](const auto &entity) { return entity->IsCharacter(); });

    if (it != entities.end()) {
        CharacterEntity* characterEntity = dynamic_cast<CharacterEntity*>(*it);
        characterEntity->SetScene(this);

        auto characterVirtual = std::make_unique<CharacterVirtualTest>();
        characterVirtual->SetPhysicsSystem(&PhysicsManager::get().mPhysicsSystem);
        characterVirtual->SetJobSystem(PhysicsManager::get().mJobSystem.get());
        characterVirtual->SetTempAllocator(PhysicsManager::get().mTempAllocator.get());
        characterVirtual->SetCustomContactListener(&PhysicsManager::get().mContactListener);
        characterVirtual->Initialize();
        PhysicsManager::get().RegisterEntity(characterEntity, characterVirtual->GetCharacter()->GetInnerBodyID());

        CreateCharacter(characterEntity, std::move(characterVirtual));
        SetHasCharacter(true);

        characterEntity->Reset();
    }
}

void Scene::Awake()
{
    // call the awake function on all entities
    for (auto &entity : m_Entities) {
        entity->Awake();
    }
}

Scene::Scene(Context &context)
    : context(context) {
    m_LightUBO.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
    // Light uniform buffers
    for (auto &buffer : m_LightUBO) {
        buffer = CreateBuffer(
            "LightUBO", context, sizeof(vkutil::LightBuffer),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    }

    // create the mesh manager, material manager and texture manager
    mMeshManager = new MeshManager(context);
    mMaterialManager = new MaterialManager(context);
    mTextureManager = new TextureManager(context);
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
