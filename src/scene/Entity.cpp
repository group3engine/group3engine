#include "Entity.hpp"

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include <utility>

#include <glm/ext.hpp>
#include <spdlog/spdlog.h>

#include "Utils.hpp"
#include "Scene.hpp"

std::atomic<uint32_t> Entity::kEntityCount{0};


Entity::Entity(std::string aName, Entity *aParent, Mesh *aMesh, glm::mat4 aLocalTransform)
    : mName(std::move(aName)), mParent(aParent), mMesh(aMesh), mHasMesh(true) {
    // convert the transformation matrix to a translation, rotation
    // (quaternion), scale
    glm::vec3 translation, scale;
    glm::quat rotation;
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(aLocalTransform, scale, rotation, translation, skew, perspective);
    mLocalTransform = {.translation = translation, .rotation = rotation, .scale = scale};
    mLocalTransform.UpdateMatrix();
    UpdateChildrenTransform();
}

void Entity::SetParent(Entity *aParent) {
    if(mParent) {
        mParent->RemoveChild(this);
    }
    mParent = aParent;
    mParent->AddChild(this);
}

void Entity::UpdateWorldTransform()
{
    if (mHasOffset || mHasCharacter) {
        mWorldTransform = glm::translate(mCharacterPositionOffset) * mParentTransform * mLocalTransform.getMatrix();
    }
    else if ((GetPhysicsType() == PhysicsType::KINEMATIC || GetPhysicsType() == PhysicsType::DYNAMIC) && mHasRigidBody) {
        // also apply physics transformations
        auto physicsTransform = glm::transpose(mRigidBody->GetWorldTransform());
        // get the physicsTransform in the same space as the local transform
        glm::mat4 physicsTransformInLocalSpace = glm::inverse(mParentTransform) * physicsTransform;
        // decompose the physics transform - only the translation and rotation are needed
        glm::vec3 translation, scale;
        glm::quat rotation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(physicsTransformInLocalSpace, scale, rotation, translation, skew, perspective);
        // create a new transform with the physics transform
        Transform physicsTransformLocalSpace = {.translation = translation, .rotation = rotation, .scale = mLocalTransform.scale};
        physicsTransformLocalSpace.UpdateMatrix();

        mWorldTransform = mParentTransform * physicsTransformLocalSpace.getMatrix();
    }
    else
    {
        mWorldTransform = mParentTransform * mLocalTransform.getMatrix();
    }
}

glm::mat4 Entity::GetWorldTransform() const {
    return mWorldTransform;
}
Transform Entity::GetWorldTransformComponents() const
{
    glm::mat4 worldTransform = GetWorldTransform();
    glm::vec3 translation, scale;
    glm::quat rotation;
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(worldTransform, scale, rotation, translation, skew, perspective);
    //    assert(perspective.w == 1.0f && perspective.x == 0.0f &&
    //           perspective.y == 0.0f && perspective.z == 0.0f);
    //    assert(skew.x == 0.0f && skew.y == 0.0f && skew.z == 0.0f);
    Transform ret =  {.translation = translation, .rotation = rotation, .scale = scale};
    ret.UpdateMatrix();
    return ret;
}


void Entity::SetTransform(glm::mat4 aTransform) {
    glm::vec3 translation, scale;
    glm::quat rotation;
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(aTransform, scale, rotation, translation, skew, perspective);
    //    assert(perspective.w == 1.0f && perspective.x == 0.0f &&
    //           perspective.y == 0.0f && perspective.z == 0.0f);
    //    assert(skew.x == 0.0f && skew.y == 0.0f && skew.z == 0.0f);
    mLocalTransform = {
        .translation = translation, .rotation = rotation, .scale = scale};
    mLocalTransform.UpdateMatrix();
    UpdateWorldTransform();
    SetPhysicsTransform();
    UpdateChildrenTransform();
}
void Entity::RecordDrawOpaque(VkCommandBuffer aCmdBuff,
                              VkPipelineLayout aPipelineLayout) const {
    if (mHasMesh && mAnimator == nullptr && mIsVisible) {
        // push the model matrix
        vkutil::MeshPushConstants pc = {};
        pc.ModelMatrix = GetWorldTransform();
        vkCmdPushConstants(aCmdBuff, aPipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           sizeof(vkutil::MeshPushConstants), &pc);

        // for each mesh primitive
        for (const auto &meshPrimitive : mMesh->meshPrimitives) {
            // skip alpha cutout materials
            if (meshPrimitive.material->alphaCutout) {
                continue;
            }

            // bind the mesh primitives material
            vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipelineLayout, 1,
                                    1, &meshPrimitive.material->descriptorSet, 0, nullptr);

            // bind the vertex buffers
            VkBuffer buffers[] = {meshPrimitive.meshGPU->mVertices.buffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(aCmdBuff, 0, sizeof(buffers) / sizeof(buffers[0]), buffers,
                                   offsets);

            // bind the index buffer
            vkCmdBindIndexBuffer(aCmdBuff, meshPrimitive.meshGPU->mIndices.buffer, 0,
                                 VK_INDEX_TYPE_UINT32);

            // draw the mesh
            vkCmdDrawIndexed(aCmdBuff, meshPrimitive.meshGPU->mIndexCount, 1, 0, 0, 0);
        }
    }
}

void Entity::RecordDrawShadow(VkCommandBuffer aCmdBuff, VkPipelineLayout aPipelineLayout,uint32_t caseCadeIndex) const {
    if (mHasMesh && mIsVisible) {
        // push the model matrix
        vkutil::MeshPushConstants pc = {};
        pc.ModelMatrix = GetWorldTransform();
        pc.cascadeIndex = caseCadeIndex;
        vkCmdPushConstants(aCmdBuff, aPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(vkutil::MeshPushConstants), &pc);

        // for each mesh primitive
        for (const auto &meshPrimitive : mMesh->meshPrimitives) {
            // bind the mesh primitives material
            vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipelineLayout, 1,
                                    1, &meshPrimitive.material->descriptorSet, 0, nullptr);

            // bind the vertex buffers
            VkBuffer buffers[] = {meshPrimitive.meshGPU->mVertices.buffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(aCmdBuff, 0, sizeof(buffers) / sizeof(buffers[0]), buffers,
                                   offsets);

            // bind the index buffer
            vkCmdBindIndexBuffer(aCmdBuff, meshPrimitive.meshGPU->mIndices.buffer, 0,
                                 VK_INDEX_TYPE_UINT32);

            // draw the mesh
            vkCmdDrawIndexed(aCmdBuff, meshPrimitive.meshGPU->mIndexCount, 1, 0, 0, 0);
        }
    }
}
void Entity::RecordDrawCutout(VkCommandBuffer aCmdBuff,
                              VkPipelineLayout aPipelineLayout) const{
    if (mHasMesh && mAnimator == nullptr && mIsVisible) {
        // push the model matrix
        vkutil::MeshPushConstants pc = {};
        pc.ModelMatrix = GetWorldTransform();
        vkCmdPushConstants(aCmdBuff, aPipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           sizeof(vkutil::MeshPushConstants), &pc);

        // for each mesh primitive
        for (const auto &meshPrimitive : mMesh->meshPrimitives) {
            // skip alpha cutout materials
            if (!meshPrimitive.material->alphaCutout) {
                continue;
            }

            // bind the mesh primitives material
            vkCmdBindDescriptorSets(aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipelineLayout, 1,
                                    1, &meshPrimitive.material->descriptorSet, 0, nullptr);

            // bind the vertex buffers - positions, texcoords, normals
            VkBuffer buffers[] = {meshPrimitive.meshGPU->mVertices.buffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(aCmdBuff, 0, sizeof(buffers) / sizeof(buffers[0]), buffers,
                                   offsets);

            // bind the index buffer
            vkCmdBindIndexBuffer(aCmdBuff, meshPrimitive.meshGPU->mIndices.buffer, 0,
                                 VK_INDEX_TYPE_UINT32);

            // draw the mesh
            vkCmdDrawIndexed(aCmdBuff, meshPrimitive.meshGPU->mIndexCount, 1, 0, 0, 0);
        }
    }
}
void Entity::SetAnimator(Animator *aAnimator) { mAnimator = aAnimator; }
Entity::~Entity() {
    // delete the animator if it exists
    delete mAnimator;
}
void Entity::RecordDrawSkinned(VkCommandBuffer aCmdBuff, VkPipelineLayout aPipeLayout, uint32_t caseCadeIndex) const {
    // we only need to render if we have a skinned mesh
    if (mHasMesh && mAnimator != nullptr && mIsVisible) {
        // push the model matrix
        vkutil::MeshPushConstants pc = {};
        pc.ModelMatrix = GetWorldTransform();
        pc.cascadeIndex = caseCadeIndex;
        vkCmdPushConstants(aCmdBuff, aPipeLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(vkutil::MeshPushConstants), &pc);
        // bind the joint descriptor set
        mAnimator->BindDescriptorSet(aCmdBuff, aPipeLayout, 2, vkutil::currentFrame);
        // for each mesh primitive
        for (const auto &meshPrimitive : mMesh->meshPrimitives) {
            // bind the mesh primitives material
            vkCmdBindDescriptorSets(
                aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipeLayout, 1, 1,
                &meshPrimitive.material->descriptorSet, 0, nullptr);
            // bind the vertex buffers
            VkBuffer buffers[] = {meshPrimitive.meshGPU->mVertices.buffer};

            VkDeviceSize offsets[] = {0, 0};

            vkCmdBindVertexBuffers(aCmdBuff, 0,
                                   sizeof(buffers) / sizeof(buffers[0]),
                                   buffers, offsets);

            // bind the index buffer
            vkCmdBindIndexBuffer(aCmdBuff,
                                 meshPrimitive.meshGPU->mIndices.buffer, 0,
                                 VK_INDEX_TYPE_UINT32);

            // draw the mesh
            vkCmdDrawIndexed(aCmdBuff, meshPrimitive.meshGPU->mIndexCount, 1, 0,
                             0, 0);
        }
    }
}
glm::mat4 Entity::getSkinnedWorldTransform(Entity const *aRoot) const {
    if (mParent) {
        return mParent->getSkinnedWorldTransform(aRoot) *
               mLocalTransform.getMatrix();
    } else {
        return mLocalTransform.getMatrix();
    }
}

Transform Entity::GetLocalTransform() const { return mLocalTransform; }

bool Entity::CompareTag(const std::string& aTag) const
{
    return std::ranges::any_of(mTags, [&aTag](const auto& tag) {
        return tag == aTag;
    });
}

bool Entity::IsCharacter() const {return mHasCharacter;}
void Entity::SetPhysicsTransform() {
    if (mHasRigidBody) {
        // get the world transform, decompose it, and set the position and rotation of the rigid body
        glm::vec3 translation, scale;
        glm::quat rotation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(mWorldTransform, scale, rotation, translation, skew, perspective);
        mRigidBody->SetPosition(translation);
        mRigidBody->SetRotation(rotation);
    }
}
void Entity::RemoveChild(Entity *aChild) {
    auto it = std::find(mChildren.begin(), mChildren.end(), aChild);
    if (it != mChildren.end()) {
            mChildren.erase(it);
    }
}
void Entity::BaseUpdate(double deltaTime) {
    mFrameNumber++;
    mTotalTime += deltaTime;
    // get the world transform of this entity
    glm::mat4 worldTransform = GetWorldTransform();
    glm::vec3 worldTranslation = worldTransform[3];
    // initialise the min distance to a camera to max
    float minDistance = std::numeric_limits<float>::max();
    // get the world transform of all the cameras
    for (auto &camera : mScene->GetCameras()) {
        // get the distance from the camera to this entity
        float dist = glm::length(glm::vec3(camera->GetWorldTransform()[3]) - worldTranslation);
        // if the distance is less than the min distance, set the min distance to the distance
        minDistance = std::min(minDistance, dist);
    }

    if (mAnimator) {
        // work out how often to update the animator based on distance
        float t = (minDistance - MIN_ANIMATOR_UPDATE_DISTANCE) / (MAX_ANIMATOR_UPDATE_DISTANCE - MIN_ANIMATOR_UPDATE_DISTANCE);
        // clamp t to 0 and 1
        t = std::clamp(t, 0.0f, 1.0f);
        // use t to lerp between 1 and LOWEST_ANIMATOR_UPDATE_RATE
        float updateRate = lerp(1, LOWEST_ANIMATOR_UPDATE_RATE, t);
        // convert to an int
        int updateRateInt = static_cast<int>(updateRate);
        // if the current frame number is divisible by the update rate, update the animator
        if ((mFrameNumber + mEntityID) % updateRateInt == 0 || mFrameNumber <= 10) {
            // removing for now
        }
        mAnimator->Update(deltaTime, this);

    }
    if(mHasRigidBody)
    {
        mRigidBody->PrePhysicsUpdate(deltaTime);
    }
    if(GetPhysicsType() == PhysicsType::KINEMATIC || GetPhysicsType() == PhysicsType::DYNAMIC || mHasCharacter || mHasOffset)
    {
        UpdateWorldTransform();
        UpdateChildrenTransform();
    }

}
void Entity::UpdateChildrenTransform() {
    for (auto &child : mChildren) {
        child->SetParentTransform(GetWorldTransform());
    }
}
void Entity::SetTransform(Transform aTransform) {
    mLocalTransform = aTransform;

    mLocalTransform.UpdateMatrix();
    UpdateWorldTransform();
    SetPhysicsTransform();
    UpdateChildrenTransform();
}
void Entity::SetParentTransform(glm::mat4 aParentTransform)  {
    mParentTransform = aParentTransform;
    UpdateWorldTransform();
    UpdateChildrenTransform();
}