#include "Engine.hpp"

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <memory>
#include <spdlog/spdlog.h>

#include <tracy/Tracy.hpp>

#include "Camera.hpp"
#include "CharacterEntity.hpp"
#include "GLFW.hpp"
#include "Image.hpp"
#include "Input.hpp"
#include "Jolt/Math/Vec3.h"
#include "Jolt/Physics/Body/Body.h"
#include "Jolt/Physics/Body/MotionType.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"
#include "PhysicsHelpers.hpp"
#include "PhysicsManager.hpp"
#include "RigidBody.hpp"
#include "SampleGLTFFilePaths.hpp"
#include "Scene.hpp"
#include "Utils.hpp"
#include "glm/ext/quaternion_trigonometric.hpp"
#include "glm/fwd.hpp"
#include "glm/trigonometric.hpp"

#include "CharacterVirtualTest.h"

#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/PhysicsMaterialSimple.h>
#include <Jolt/Geometry/Triangle.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Core/HashCombine.h>
#include <Jolt/Geometry/IndexedTriangle.h>

#define TEMP_DISABLE_PHYSICS 0

Engine::Engine() {
    m_isRunning = false;
    m_lastFrameTime = 0.0;
}

void Engine::InitScene() {
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
    mScene->Load(gltfPath);
    mScene->AddLightSource(directionalLight);

    // Loop through the positions and instantiate a light
    // and pass to the scene to add the lights to the scene
    for (const auto &position : spotLightPositions) {
        Light spotLight = {};
        spotLight.Type = LightType::Spot;
        spotLight.position = position;
        spotLight.colour = glm::vec4(glm::linearRand(0.0f, 1.0f), glm::linearRand(0.0f, 1.f),
                                     glm::linearRand(0.0f, 1.0f), 1.0f);
        mScene->AddLightSource(spotLight);
    }

    SPDLOG_DEBUG("Number of Lights: {}", mScene->GetLights().size());
}

bool Engine::Initialize() {
    // TODO: Could probably store this somewhere else
    int windowWidth = 1280;
    int windowHeight = 720;

    Platform::get().StartUp(windowWidth, windowHeight);

    if (m_context.MakeContext(Platform::get().window)) {
        m_isRunning = true;
    }

    mScene = std::make_shared<Scene>(m_context);

    mRenderer = std::make_unique<Renderer>(m_context, mScene);

    // NOTE: This has to come after the renderer has been initialised currently.
    InitScene();

    // NOTE: This has to happen after the scene has been initialised currently
    // TODO: Not ideal having this as a separate public method just to be able
    // to initialise the render passes correctly after the scene is initialised.
    mRenderer->CreateRenderPasses();

    PhysicsManager::get().StartUp();

    // ---PHYSICS TEST INITIALISATION---

    // Add rigid body to the physics system
    // NOTE: Doing this outside of the constructor gives us a bit more flexibility
    // floor.Init(PhysicsManager::get());
    // floor.SetPosition(glm::vec3(0.f, -0.5f, 0.f));
    // floor.SetRotation(glm::quat(glm::angleAxis(glm::radians(4.0f), glm::vec3(0.0f, 0.f, 1.f))));


    // --- Mesh shape for each entity ----------------------------------------

    // Shapes are refcounted and can be shared between bodies
    JPH::Ref<Shape> shape;

    for (auto it = mScene->GetEntities().begin(); it != mScene->GetEntities().end(); ++it) {
        const auto &entity = *it;


        if (!entity->IsCharacter()) {

            if (entity->HasAnimator()) {
                SPDLOG_INFO("Skipping entity {}, as it has an animator", entity->GetName());
                continue;
            }

            const auto *mesh = entity->GetMesh();

            if (!mesh) {
                SPDLOG_WARN("Entity {} does not have mesh", entity->GetName());
                continue;
            }

            size_t totalVertices = 0;
            size_t totalTriangles = 0;
            // no physics to add if it isn't a sesnor or solid
            if(entity->IsSensor() || entity->IsSolid()) {
                // calculate the scaling matrix to apply to the vertices
                glm::mat4 entityWorldTransform = entity->GetWorldTransform();
                // decompose the world transform to get the scale
                glm::vec3 position, scale;
                glm::quat rotation;
                glm::vec3 skew;
                glm::vec4 perspective;
                glm::decompose(entityWorldTransform, scale, rotation, position, skew, perspective);
                // create a scaling matrix
                glm::mat4 scalingMatrix = glm::scale(glm::mat4(1.0f), scale);
                VertexList vertices;
                for (const auto &primitive : mesh->meshPrimitives) {
                    // Create an array of vertices
                    VertexList vertices;
                    vector<JPH::Vec3> list_of_points;
                    for (const auto &vertex : primitive.vertices) {
                        auto worldPos = scalingMatrix * glm::vec4(vertex.pos, 1.0f);
                        vertices.emplace_back(worldPos.x, worldPos.y, worldPos.z);
                        list_of_points.emplace_back(Vec3(worldPos.x, worldPos.y, worldPos.z));
                        ++totalVertices;
                    }

                    IndexedTriangleList indexedTriangles;
                    for (size_t i = 0; i < primitive.indices.size(); i += 3) {
                        indexedTriangles.push_back(IndexedTriangle(primitive.indices[i],
                                                                   primitive.indices[i + 1],
                                                                   primitive.indices[i + 2]));
                        ++totalTriangles;
                    }

                    assert(!vertices.empty());
                    assert(!indexedTriangles.empty());

                    // Create the settings object for a mesh shape
                    JPH::MeshShapeSettings settings(vertices, indexedTriangles);

                    // Create shape
                    JPH::Shape::ShapeResult result = settings.Create();
                    if (result.IsValid()) {
                        shape = result.Get();
                    } else {
                        SPDLOG_ERROR("Shape result is invalid. {}", result.GetError());
                    }

                    Transform entity_transform = entity->GetWorldTransformComponents();
                    EMotionType motionType = EMotionType::Static;
                    if(entity->IsKinematic()) {
                        motionType = EMotionType::Kinematic;
                    }
                    BodyCreationSettings bodyCreationSettings = {
                        result.Get(), RVec3(entity_transform.translation.x, entity_transform.translation.y, entity_transform.translation.z), Quat(entity_transform.rotation.x, entity_transform.rotation.y, entity_transform.rotation.z, entity_transform.rotation.w),
                        motionType, Layers::NON_MOVING};
                    
                    // HARDCODED A BOUNCY FLOOR FOR DEMONSTRATION
                    if(entity->GetName() == "floor.001")
                    {
                        bodyCreationSettings.mFriction = 0.1f;
                        bodyCreationSettings.mRestitution = 0.7f;
                    }
                    bodyCreationSettings.mIsSensor = entity->IsSensor();
                    if(entity->IsKinematic()) {
                        bodyCreationSettings.mMassPropertiesOverride.mMass = 1.0f;
                        bodyCreationSettings.mMassPropertiesOverride.mInertia =
                            JPH::Mat44::sIdentity();
                        bodyCreationSettings.mOverrideMassProperties =
                            EOverrideMassProperties::MassAndInertiaProvided;
                    }
                    bool activate = false;

                    if(entity->CompareTag("dynamic"))
                    {
                        // Create the settings object for a mesh shape
                        JPH::ConvexHullShapeSettings consettings(list_of_points.data(), list_of_points.size());

                        // Create shape
                        JPH::Shape::ShapeResult result = consettings.Create();
                        if (result.IsValid()) {
                            shape = result.Get();
                        } else {
                            SPDLOG_ERROR("Shape result is invalid. {}", result.GetError());
                        }
                        motionType = EMotionType::Dynamic;
                        bodyCreationSettings = {
                        result.Get(), RVec3(entity_transform.translation.x, entity_transform.translation.y, entity_transform.translation.z), Quat(entity_transform.rotation.x, entity_transform.rotation.y, entity_transform.rotation.z, entity_transform.rotation.w),
                        motionType, Layers::MOVING};
                        activate = true;
                        entity->SetAsKinematic();

                        // RESTITUTION ALSO HARDCODED FOR SHOWING IT WORKING
                        bodyCreationSettings.mRestitution = 0.8f;
                    }

                    RigidBody entity_rigid_body = RigidBody(bodyCreationSettings);


                    entity_rigid_body.Init(PhysicsManager::get(), activate);
                    // only do this part if its supposed to DO something when collided with (i.e. sensors)
                    PhysicsManager::get().RegisterEntity(entity, entity_rigid_body.mBodyId);
                    PhysicsManager::get().mPhysicsSystem.OptimizeBroadPhase();
                    entity->AddRigidBody(std::make_unique<RigidBody>(entity_rigid_body));

                }
            }

            SPDLOG_INFO("total vertices {}", totalVertices);
            SPDLOG_INFO("total triangles {}", totalTriangles);
        } else {
            SPDLOG_INFO("Skipping character");
        }
    }


    // Find character
    auto &entities = mScene->GetEntities();
    auto it = std::find_if(entities.begin(), entities.end(),
                           [](const auto &entity) { return entity->IsCharacter(); });

    if (it != entities.end()) {
        CharacterEntity* characterEntity = dynamic_cast<CharacterEntity*>(*it);
        characterEntity->SetScene(mScene.get());

        auto characterVirtual = std::make_unique<CharacterVirtualTest>();
        characterVirtual->SetPhysicsSystem(&PhysicsManager::get().mPhysicsSystem);
        characterVirtual->SetJobSystem(PhysicsManager::get().mJobSystem.get());
        characterVirtual->SetTempAllocator(PhysicsManager::get().mTempAllocator.get());
        characterVirtual->SetCustomContactListener(&PhysicsManager::get().mContactListener);
        characterVirtual->Initialize();
        PhysicsManager::get().RegisterEntity(characterEntity, characterVirtual->GetCharacter()->GetInnerBodyID());

        mScene->CreateCharacter(characterEntity, std::move(characterVirtual));
        mScene->SetHasCharacter(true);

        characterEntity->Reset();
    }



    // call the scene awake function
    mScene->Awake();

    // ---END OF PHYSICS TEST INITIALISATION---

    SPDLOG_DEBUG("Engine initialised.");



    return m_isRunning;
}

void Engine::Shutdown() {
    mRenderer->Destroy();
    mRenderer.reset();
    mScene->Destroy();
    m_context.Destroy(); // Free vulkan device, allocator, window
    Platform::get().ShutDown();
    PhysicsManager::get().ShutDown();
}

void Engine::Run() {
    auto camera = static_cast<Camera *>(glfwGetWindowUserPointer(Platform::get().window));
    camera->SetPhysics(&PhysicsManager::get());
    camera->SetScene(mScene.get());

    m_lastFrameTime = glfwGetTime();

    while (m_isRunning && !glfwWindowShouldClose(m_context.mWindow)) {
        double currentFrameTime = glfwGetTime();
        GlobalUtil::deltaTime = currentFrameTime - m_lastFrameTime;
        m_lastFrameTime = currentFrameTime;

        // See imgui.cpp
        // "(So you want to try calling NewFrame() as early as you can in your main loop to be able to use Dear ImGui everywhere)"
        ImGuiRenderer::NewFrame();

        PollInputEvents();



        ProcessInputParams processInputParams{};
        auto cameraForward = camera->GetDirection();
        processInputParams.mCameraState.mForward = {cameraForward.x, cameraForward.y, cameraForward.z};

        if (mScene->HasCharacter()) {
            mScene->GetCharacter().ProcessInput(processInputParams);
        }

        PreUpdateParams preUpdateParams{};
        preUpdateParams.mDeltaTime = GlobalUtil::deltaTime;

        if (mScene->HasCharacter()) {
            mScene->GetCharacter().PrePhysicsUpdate(preUpdateParams);
        }

        if (mScene->HasCharacter()) {
            auto characterVirtualPos = mScene->GetCharacter().GetCharacterPosition();
            mScene->GetCharacter().SetCharacterPositionOffset(
                characterVirtualPos.GetX(), characterVirtualPos.GetY(), characterVirtualPos.GetZ());

            Update(GlobalUtil::deltaTime);
        } else {
            Update(GlobalUtil::deltaTime);
        }

        ImGuiRenderer::EndFrame();

        Render();

        FrameMark;
    }

    Shutdown();
}

void Engine::UpdateLogic() {
    if (IsKeyDown(KEY::eESCAPE)) {
        glfwSetWindowShouldClose(Platform::get().window, GLFW_TRUE);
    }

    auto camera = static_cast<Camera *>(glfwGetWindowUserPointer(Platform::get().window));
    assert(camera);

    camera->SetInput(EInputState::FORWARD, IsKeyDown(KEY::eW));
    camera->SetInput(EInputState::BACKWARD, IsKeyDown(KEY::eS));
    camera->SetInput(EInputState::LEFT, IsKeyDown(KEY::eA));
    camera->SetInput(EInputState::RIGHT, IsKeyDown(KEY::eD));

    camera->SetInput(EInputState::DOWN, IsKeyDown(KEY::eQ));
    camera->SetInput(EInputState::UP, IsKeyDown(KEY::eE));

    camera->SetInput(EInputState::FAST, IsKeyDown(KEY::eLEFT_SHIFT));
    camera->SetInput(EInputState::SLOW, IsKeyDown(KEY::eLEFT_CONTROL));

    camera->SetInput(EInputState::SWITCHVIEW, IsKeyPressed(KEY::eV));

    camera->SetInput(EInputState::TELEPORT, IsKeyPressed(KEY::eT));

    camera->SetInput(EInputState::ZOOM_IN, IsKeyPressed(KEY::eY));
    camera->SetInput(EInputState::ZOOM_OUT, IsKeyPressed(KEY::eU));

    if (IsKeyPressed(KEY::e5)) {
        vkutil::postProcessSettings.Enable = vkutil::postProcessSettings.Enable == true ? false : true;

        const std::string result = vkutil::postProcessSettings.Enable == true ? "Enabled" : "Disabled";

        SPDLOG_INFO("Post process: {}", result);
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON::eRIGHT)) {
        auto &flag = camera->inputMap[std::size_t(EInputState::MOUSING)];
        flag = !flag;

        if (flag) {
            glfwSetInputMode(Platform::get().window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(Platform::get().window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    if(IsKeyPressed(KEY::eP))
    {
        SPDLOG_INFO("Camera Location: {}", glm::to_string(camera->GetPosition()));
    }
}

void Engine::Update(double deltaTime) {
    ZoneScopedN("Engine::Update");

    UpdateLogic();
    mScene->Update(deltaTime);
    mScene->UpdateUi(deltaTime);
    PhysicsManager::get().UpdatePhysics(deltaTime);
    mRenderer->Update(deltaTime);

}

void Engine::Render() {
    ZoneScopedN("Engine::Render");

    mRenderer->Render();
}
