//
// Created by thomas on 07/02/25.
//

#include "Entity.hpp"

#include <utility>

namespace vk {
Entity::Entity(std::string aName, Entity *aParent, Mesh *aMesh,
               glm::mat4 aLocalTransform, VkPipelineLayout aPipelineLayout)
    : mName(std::move(aName)),
      mParent(aParent),
      mMesh(aMesh),
      mHasMesh(true),
      mPipelineLayout(aPipelineLayout) {
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
    mLocalTransform = {
        .translation = translation, .rotation = rotation, .scale = scale};
}
void Entity::RecordDrawOpaque(VkCommandBuffer aCmdBuff) {
    if (mHasMesh) {
        // push the model matrix
        glm::mat4 mModelMatrix = getWorldTransform();
        vkCmdPushConstants(
            aCmdBuff, mPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
            sizeof(glm::mat4), &mModelMatrix);
        // for each mesh primitive
        for (auto meshPrimitive : mMesh->meshPrimitives) {
            // skip alpha cutout materials
            if (meshPrimitive.material->alphaCutout) {
                continue;
            }
            // bind the mesh primitives material
            vkCmdBindDescriptorSets(
                aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 1,
                1, &meshPrimitive.material->descriptorSet, 0, nullptr);
            // bind the vertex buffers - positions, texcoords, normals
            VkBuffer buffers[] = {meshPrimitive.meshGPU->mPositions.buffer,
                                  meshPrimitive.meshGPU->mTexcoords.buffer,
                                  meshPrimitive.meshGPU->mNormals.buffer};

            VkDeviceSize offsets[] = {0, 0, 0};

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
void Entity::record_draw_shadow(VkCommandBuffer aCmdBuff) const {
    if (mHasMesh) {
        // push the model matrix
        glm::mat4 mModelMatrix = getWorldTransform();
        vkCmdPushConstants(
            aCmdBuff, mPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
            sizeof(glm::mat4), &mModelMatrix);
        // for each mesh primitive
        for (auto meshPrimitive : mMesh->meshPrimitives) {
            // bind the mesh primitives material
            vkCmdBindDescriptorSets(
                aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 1,
                1, &meshPrimitive.material->descriptorSet, 0, nullptr);
            // bind the vertex buffers - positions, texcoords, normals
            VkBuffer buffers[] = {meshPrimitive.meshGPU->mPositions.buffer};

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
void Entity::RecordDrawCutout(VkCommandBuffer aCmdBuff) {
    if (mHasMesh) {
        // push the model matrix
        glm::mat4 mModelMatrix = getWorldTransform();
        vkCmdPushConstants(
            aCmdBuff, mPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
            sizeof(glm::mat4), &mModelMatrix);
        // for each mesh primitive
        for (auto meshPrimitive : mMesh->meshPrimitives) {
            // skip alpha cutout materials
            if (!meshPrimitive.material->alphaCutout) {
                continue;
            }
            // bind the mesh primitives material
            vkCmdBindDescriptorSets(
                aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 1,
                1, &meshPrimitive.material->descriptorSet, 0, nullptr);
            // bind the vertex buffers - positions, texcoords, normals
            VkBuffer buffers[] = {meshPrimitive.meshGPU->mPositions.buffer,
                                  meshPrimitive.meshGPU->mTexcoords.buffer,
                                  meshPrimitive.meshGPU->mNormals.buffer};

            VkDeviceSize offsets[] = {0, 0, 0};

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
}  // namespace vk