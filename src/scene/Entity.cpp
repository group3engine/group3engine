#include "Entity.hpp"

#include <glm/gtx/matrix_decompose.hpp>
#include <iostream>
#include <utility>

#include <glm/ext.hpp>
#include <spdlog/spdlog.h>

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
}

void Entity::SetParent(Entity *aParent) {
    mParent = aParent;
    mParent->AddChild(this);
}

glm::mat4 Entity::getWorldTransform() const {
    glm::mat4 return_matrix;
    if (mParent) {
        return_matrix = mParent->getWorldTransform() * mLocalTransform.getMatrix();
    } else {
        return_matrix = mLocalTransform.getMatrix();
    }

    if (mHasCharacter) {
        auto mat = glm::identity<glm::mat4>();
        mat[0][3] = mPosition.x;
        mat[1][3] = mPosition.y;
        mat[2][3] = mPosition.z;
        return glm::transpose(mat);
    }
    else if (mHasRigidBody) {
        // also apply physics transformations
        auto mat = glm::transpose(mRigidBody->GetWorldTransform());
        return mat;
        //return_matrix = mat * return_matrix;
    }

    return return_matrix;
}

glm::mat4 Entity::getLocalTransform() {
    glm::mat4 return_matrix = mLocalTransform.getMatrix();

    if (mHasRigidBody) {
        // also apply physics' transformations
        return_matrix = return_matrix;
    }

    return return_matrix;
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
}
void Entity::RecordDrawOpaque(VkCommandBuffer aCmdBuff,
                              VkPipelineLayout aPipelineLayout) {
    if (mHasMesh && mAnimator == nullptr) {
        // push the model matrix
        glm::mat4 mModelMatrix = getWorldTransform();
        vkCmdPushConstants(aCmdBuff, aPipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           sizeof(glm::mat4), &mModelMatrix);

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

void Entity::RecordDrawShadow(VkCommandBuffer aCmdBuff, VkPipelineLayout aPipelineLayout) const {
    if (mHasMesh) {
        // push the model matrix
        glm::mat4 mModelMatrix = getWorldTransform();
        vkCmdPushConstants(aCmdBuff, aPipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           sizeof(glm::mat4), &mModelMatrix);

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
                              VkPipelineLayout aPipelineLayout) {
    if (mHasMesh && mAnimator == nullptr) {
        // push the model matrix
        glm::mat4 mModelMatrix = getWorldTransform();
        vkCmdPushConstants(aCmdBuff, aPipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           sizeof(glm::mat4), &mModelMatrix);

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
void Entity::Update(double deltaTime) {
    frameNumber++;
    if (mAnimator) {
        mAnimator->Update(deltaTime, this);
        // TMP FOR VISUALISATION
        if ((frameNumber / 300) % 3 == 0) {
            mAnimator->SetActiveAnimation("laugh", 1.f);
        }
        else if ((frameNumber / 300) % 3 == 1) {
            mAnimator->SetActiveAnimation("rumba", 0.5f);
        } else {
            mAnimator->SetActiveAnimation("idle", 0.5f);
        }
        //END TMP
    }
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
    if (mParent) {
        return mParent->getSkinnedWorldTransform(aRoot) *
               mLocalTransform.getMatrix();
    } else {
        return mLocalTransform.getMatrix();
    }
}

Transform Entity::GetTransform() { return mLocalTransform; }
