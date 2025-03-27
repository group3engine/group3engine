//
// Created by thomas on 22/03/25.
//

#ifndef PARTICLESYSTEM_HPP
#define PARTICLESYSTEM_HPP
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "GLTFImportStructs.hpp"
#include "Entity.hpp"

#define SHADER_DIR std::filesystem::path(CMAKE_SOURCE_DIR) / "assets/shaders/"
#define PARTICLE_BILLBOARD_VERTEX_SHADER (SHADER_DIR / "billboardParticle.vert.spv")
#define PARTICLE_MESH_VERTEX_SHADER (SHADER_DIR / "defaultParticle.vert.spv")
#define PARTICLE_SHADER_PBR_FRAGMENT (SHADER_DIR / "alpha_masking.frag.spv")
#define PARTICLE_SHADER_SIMPLE_FRAGMENT (SHADER_DIR / "alpha_masking.frag.spv")
#define PARTICLE_SHADER_UNLIT_FRAGMENT (SHADER_DIR / "alpha_masking.frag.spv")

// alias colour to vec4
using Colour = glm::vec4;
// alias size to vec3
using Size = glm::vec3;

struct Emission
{
    glm::vec3 Position;
    glm::vec3 Velocity;
};

/// The space in which the simulation is done
enum class SimulationSpace {
    /// The simulation is done in world space
    World,
    /// The simulation is done in local space, particles are "attached" to the object
    Local
};

/// The emission settings of a particle system
struct ParticleSystemEmissionSettings
{
    /// The number of particles to emit per second
    float rateOverTime = 10.0f;
    /// The number of particles to emit per distance unit
    float rateOverDistance = 0.0f;
};

/// Ways a shape can emit particles
enum class EmitFrom {
    /// Emit from the volume of the shape
    Volume,
    /// Emit from the surface of the shape
    Surface,
};
/// union of shapes a particle can be emitted from
union EmitterShape{
    /// a sphere shape
    struct {
        /// the radius of the sphere
        float radius = 1.0f;
        /// How the particles are emitted from the sphere
        EmitFrom emitFrom = EmitFrom::Volume;
    } sphere;
    /// a box shape
    struct {
        /// the size of the box
        Size size = {1.0f, 1.0f, 1.0f};
        /// How the particles are emitted from the box
        EmitFrom emitFrom = EmitFrom::Volume;
    } box;
    /// a cone shape
    struct {
        /// the angle of the cone, in radians
        float angle = 25.0f;
        /// the radius of the cone
        float radius = 1.0f;
        /// the height of the cone
        float height = 1.0f;
        /// How the particles are emitted from the cone
        EmitFrom emitFrom = EmitFrom::Volume;
    } cone;
};

// the functions to randomly choose a particle spawn location
// we will do these with the method where you choose a random point in the bounding box and repeat until it is inside the sphere
// sphere
Emission SphereParticleSpawn(size_t &seed, EmitterShape const &sphereShape);
// box
Emission BoxParticleSpawn(size_t &seed, EmitterShape const &boxShape);
// cone
Emission ConeParticleSpawn(size_t &seed, EmitterShape const &coneShape);


/// The type of shape
enum class ShapeType {
    Sphere,
    Box,
    Cone
};

/// The shape of the emission
struct ParticleSystemEmissionShape
{

    /// The location of the shape
    Transform transform {};
    /// The type of shape
    ShapeType type = ShapeType::Cone;
    /// The shape of the emission
    EmitterShape shape = EmitterShape{};
    /// Whether to align the particles to the direction when emitted
    bool alignToDirection = false;

};

/// The mesh types a particle can be rendered as
enum class ParticleMeshType {
    /// A billboard particle, always facing the camera and axis aligned
    Billboard,
    /// A horizontal billboard particle (always perpendicular to the y axis)
    HorizontalBillboard,
    /// A vertical billboard particle (always parallel to the y axis)
    VerticalBillboard,
    /// A quad, not aligned to any axis
    Quad,
    /// A mesh particle
    Mesh,
};

/// The types of shading available to a particle
enum class ParticleShadingType {
    /// The particles are shaded with pbr
    PBR,
    /// The particles are shaded with a simple shader
    Simple,
    /// The particles are unlit (base colour only)
    Unlit
};

/// The way particles are rendered
struct ParticleSystemRenderSettings
{
    /// The mesh type to render the particles as
    ParticleMeshType type = ParticleMeshType::Billboard;
    /// The mesh to use (only used if type is Mesh)
    Mesh *mesh = nullptr;
    /// The type of shading to use
    ParticleShadingType shadingType = ParticleShadingType::PBR;
    /// Allow alpha masking
    bool alphaMask = false;
    /// Render the back faces of the particles
    bool renderBackFaces = false;
    /// TODO: sort mode
    int sortMode = 0;
    /// TODO: shadow casting mode
    bool castShadows = false;
};


/// The settings of a particle system
struct ParticleSystemSettings
{
    /// The Length of time the particle system will run for. If looping is true, this is the length of one cycle.
    float duration = 5.0f;
    /// If true, the particle emission cycle will repeat after the duration.
    bool looping = true;
    /// The length of time that a particle will live for after being emitted.
    float startLifetime = 1.0f;
    /// The starting speed of the particles.
    float startSpeed = 5.0f;
    /// The starting size of the particles.
    Size startSize = {1.0f, 1.0f, 1.0f};
    /// The starting rotation of a particle as a quaternion (x, y, z, w).
    glm::quat startRotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
    /// The starting colour of the particles.
    Colour startColour = {1.0f, 1.0f, 1.0f, 1.0f};
    /// The gravity applied to particles.
    glm::vec3 gravityModifier = {0.0f, -9.81f, 0.0f};
    /// The space in which the simulation is done.
    // TODO: support more than just world
    SimulationSpace simulationSpace = SimulationSpace::World;
    /// The entity that the particles are attached to.
    Entity* attachedEntity = nullptr;
    /// The speed of the simulation.
    float simulationSpeed = 1.0f;
    /// Whether to play the particle system on creation.
    bool playOnAwake = true;
    /// Whether to add the owner entities velocity to the particles.
    bool useOwnerVelocity = false;
    /// The maximum number of particles that can exist at one time. If the particle system tries to create more than this number, the oldest particles will be destroyed.
    size_t maxParticles = 1000;
    /// Whether to randomly seed the particle system each time it is played / looped.
    bool isRandomlySeeded = true;
    /// The random seed of the particle system. Only used if isRandomlySeeded is false.
    int randomSeed = 0;
    /// The material for the particle system to use
    const Material *material = nullptr;
    /// The emission settings of the particle system.
    ParticleSystemEmissionSettings emission = ParticleSystemEmissionSettings();
    /// The shape of the emission.
    ParticleSystemEmissionShape emissionShape = ParticleSystemEmissionShape();
    /// The render settings
    ParticleSystemRenderSettings renderSettings = ParticleSystemRenderSettings();

};

// random function to use for seeding - source: https://github.com/PanosK92/SpartanEngine/blob/master/data/shaders/common.hlsl
inline float get_hash(uint seed)
{
    seed ^= seed >> 17;
    seed *= 0x5bd1e995u;
    seed ^= seed >> 13;
    return float(seed) / float(0xffffffffu);
}
// function to get random from -1 to 1
inline float get_random(size_t seed)
{
    return get_hash(seed++) * 2.0f - 1.0f;
}

// the particle struct used on the gpu for the instanced particles
struct alignas(16) ParticleGPU
{
    // the transform is only really going to be used for those particles that are mesh particles
    glm::mat4 transform; // 4 * 16 bytes
    glm::vec3 colour; // 12 bytes
    int isEnabled = 0;

};
// total size: 48 bytes, 16 * 3
// The particle struct used by the current particles
struct Particle
{
    // the transform of the particle
    Transform transform;
    // the velocity of the particle
    glm::vec3 velocity;
    // the time remaining of the particle
    double lifetime = 0.f;
    // the colour of the particle
    Colour colour;
};



class ParticleSystem {
    // I am sorry for these static guys one day I will atone for my sins
public:
    static void RegisterRenderPass(VkRenderPass aRenderPass);
    static void DrawAll(VkCommandBuffer cmd, VkPipelineLayout aLayout);

// update function
    void Update(double aDeltaTime);

private:
    static VkRenderPass kRenderPass;
    static std::vector<ParticleSystem*> systems;
public:
    ParticleSystem(Context &aContext, ParticleSystemSettings const &aSettings);
    ~ParticleSystem();

    void Start();

    void Stop();

    void Destroy();

    void Render(VkCommandBuffer cmd, VkPipelineLayout aLayout);


private:
    void Emit();
    Transform GenerateTransform(Emission const & aEmission) const;

    void UpdateParticles(double aDeltaTme);


private:
    bool mIsPlaying = false;
    ParticleSystemSettings mSettings = ParticleSystemSettings();
    glm::vec3 mPosition = glm::vec3(0.0f);
    glm::vec3 mPreviousPosition = glm::vec3(0.0f);
    double mDistanceMoved = 0.0f;
    double mPreviousDistance = 0.0f;


    double mTime = 0.0f;
    double mPreviousTime = 0.0f;

    Particle* mParticles = nullptr;
    ParticleGPU* mParticleGPU = nullptr;
    size_t backOfParticleBuffer = 0;
    Buffer mParticlesBuffer;
    double mTimeStepToEmit;
    double mDistanceStepToEmit;
    size_t seed = 0.f;
    Context &mContext;
    VkDescriptorSetLayout mDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet mDescriptorSet = VK_NULL_HANDLE;
    std::pair<VkPipeline, VkPipelineLayout> mPipeline;
    glm::vec3 mAttachedWorldTranslation {};
};



#endif //PARTICLESYSTEM_HPP
