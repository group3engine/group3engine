//
// Created by thomas on 07/02/25.
//

#include "Entity.hpp"

#include <glm/gtx/matrix_decompose.hpp>
#include <iostream>
#include <utility>

#include <glm/gtx/io.hpp>
namespace vk {
Entity::Entity(std::string aName, Entity *aParent, Mesh *aMesh,
               glm::mat4 aLocalTransform)
    : mName(std::move(aName)), mParent(aParent), mMesh(aMesh), mHasMesh(true) {
    // convert the transformation matrix to a translation, rotation
    // (quaternion), scale
    glm::vec3 translation, scale;
    glm::quat rotation;
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(aLocalTransform, scale, rotation, translation, skew,
                   perspective);
    mLocalTransform = {
        .translation = translation, .rotation = rotation, .scale = scale};
}
void Entity::SetParent(Entity *aParent) {
    mParent = aParent;
    mParent->AddChild(this);
}
glm::mat4 Entity::getWorldTransform() const {
    if (mParent) {
        return mParent->getWorldTransform() * mLocalTransform.getMatrix();
    } else {
        return mLocalTransform.getMatrix();
    }
}
glm::mat4 Entity::getLocalTransform() { return mLocalTransform.getMatrix(); }
void Entity::SetTransform(glm::mat4 aTransform) {
    glm::vec3 translation, scale;
    glm::quat rotation;
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(aTransform, scale, rotation, translation, skew, perspective);
    assert(perspective.w == 1.0f && perspective.x == 0.0f &&
           perspective.y == 0.0f && perspective.z == 0.0f);
    assert(skew.x == 0.0f && skew.y == 0.0f && skew.z == 0.0f);
    mLocalTransform = {
        .translation = translation, .rotation = rotation, .scale = scale};
}
void Entity::RecordDrawOpaque(VkCommandBuffer aCmdBuff,
                              VkPipelineLayout aPipelineLayout) {
    if (mHasMesh && mAnimator == nullptr) {
        // push the model matrix
        glm::mat4 mModelMatrix = getWorldTransform();
        vkCmdPushConstants(aCmdBuff, aPipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT |
                               VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(glm::mat4), &mModelMatrix);
        // for each mesh primitive
        for (const auto &meshPrimitive : mMesh->meshPrimitives) {
            // skip alpha cutout materials
            if (meshPrimitive.material->alphaCutout) {
                continue;
            }
            // bind the mesh primitives material
            vkCmdBindDescriptorSets(
                aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipelineLayout, 1,
                1, &meshPrimitive.material->descriptorSet, 0, nullptr);
            // bind the vertex buffers
            VkBuffer buffers[] = {meshPrimitive.meshGPU->mVertices.buffer};

            VkDeviceSize offsets[] = {0};

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
void Entity::RecordDrawShadow(VkCommandBuffer aCmdBuff,
                              VkPipelineLayout aPipelineLayout) const {
    if (mHasMesh) {
        // push the model matrix
        glm::mat4 mModelMatrix = getWorldTransform();
        vkCmdPushConstants(aCmdBuff, aPipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT |
                               VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(glm::mat4), &mModelMatrix);
        // for each mesh primitive
        for (const auto &meshPrimitive : mMesh->meshPrimitives) {
            // bind the mesh primitives material
            vkCmdBindDescriptorSets(
                aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipelineLayout, 1,
                1, &meshPrimitive.material->descriptorSet, 0, nullptr);
            // bind the vertex buffers
            VkBuffer buffers[] = {meshPrimitive.meshGPU->mVertices.buffer};

            VkDeviceSize offsets[] = {0};

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
void Entity::RecordDrawCutout(VkCommandBuffer aCmdBuff,
                              VkPipelineLayout aPipelineLayout) {
    if (mHasMesh && mAnimator == nullptr) {
        // push the model matrix
        glm::mat4 mModelMatrix = getWorldTransform();
        vkCmdPushConstants(aCmdBuff, aPipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT |
                               VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(glm::mat4), &mModelMatrix);
        // for each mesh primitive
        for (const auto &meshPrimitive : mMesh->meshPrimitives) {
            // skip alpha cutout materials
            if (!meshPrimitive.material->alphaCutout) {
                continue;
            }
            // bind the mesh primitives material
            vkCmdBindDescriptorSets(
                aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipelineLayout, 1,
                1, &meshPrimitive.material->descriptorSet, 0, nullptr);
            // bind the vertex buffers - positions, texcoords, normals
            VkBuffer buffers[] = {meshPrimitive.meshGPU->mVertices.buffer};

            VkDeviceSize offsets[] = {0};

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
void Entity::SetAnimator(Animator *aAnimator) { mAnimator = aAnimator; }
void Entity::Update(float deltaTime) {
    if (mAnimator)
        mAnimator->Update(deltaTime, this);
}
Entity::~Entity() {
    // delete the animator if it exists
    delete mAnimator;
}
void Entity::RecordDrawSkinned(VkCommandBuffer aCmdBuff,
                               VkPipelineLayout aPipeLayout) {
    // we only need to render if we have a skinned mesh
    if (mHasMesh && mAnimator != nullptr) {
        // push the model matrix
        glm::mat4 mModelMatrix = getWorldTransform();
        vkCmdPushConstants(aCmdBuff, aPipeLayout,
                           VK_SHADER_STAGE_VERTEX_BIT |
                               VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(glm::mat4), &mModelMatrix);
        // bind the joint descriptor set
        mAnimator->BindDescriptorSet(aCmdBuff, aPipeLayout, 2);
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
    if(mParent) {
        return mParent->getSkinnedWorldTransform(aRoot) *  mAnimationTransform * mLocalTransform.getMatrix();
    } else {
        return mAnimationTransform *  mLocalTransform.getMatrix();
    }
}
} // namespace vk