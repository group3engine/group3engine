//
// Created by thomas on 12/12/24.
//

#ifndef MYPROJECT_STANDARDMESH_HPP
#define MYPROJECT_STANDARDMESH_HPP

#include "../labutils/vkbuffer.hpp"
#include "baked_model.hpp"

namespace GraphicsThings
{

    class StandardMesh
    {
    public:
        StandardMesh(const BakedMeshData &mesh, labutils::VulkanContext const &aContext,
                     labutils::Allocator const &aAllocator, VkPipelineLayout pipelineLayout, std::vector<VkDescriptorSet> const &aMaterialDescriptors);

        // Move constructor
        StandardMesh(StandardMesh &&) noexcept = default;

        // Move assignment operator
        StandardMesh &operator=(StandardMesh &&) noexcept = default;

        ~StandardMesh() = default;

        // method to record the draw command for this mesh
        void record_draw(VkCommandBuffer aCmdBuff) const;

        // method to record the draw command for shadow mapping for this mesh
        void record_draw_shadow(VkCommandBuffer aCmdBuff) const;

    private:
        labutils::Buffer mPositions;
        labutils::Buffer mTexcoords;
        labutils::Buffer mTBNFrames;
        labutils::Buffer mIndices;
        std::uint32_t mIndexCount;
        VkPipelineLayout mPipelineLayout;
        std::uint32_t mMaterialIndex;

        // pointer to the set of material descriptors - one of these will be ours :)
        std::vector<VkDescriptorSet> const *mMaterialDescriptors;

        glm::mat4 mModelMatrix;
    };

} // GraphicsThings

#endif //MYPROJECT_STANDARDMESH_HPP
