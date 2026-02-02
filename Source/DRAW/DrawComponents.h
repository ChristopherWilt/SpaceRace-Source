#ifndef DRAW_COMPONENTS_H
#define DRAW_COMPONENTS_H

#include "./Utility/load_data_oriented.h"
#include "./Utility/FontData.h"
#include "./Utility/FontLoader.h"

namespace DRAW
{
	//*** TAGS ***//

	//*** COMPONENTS ***//
	struct VulkanRendererInitialization
	{
		std::string vertexShaderName;
		std::string fragmentShaderName;
		VkClearColorValue clearColor;
		VkClearDepthStencilValue depthStencil;
		float fovDegrees;
		float nearPlane;
		float farPlane;
	};

	struct VulkanRenderer
	{
		GW::GRAPHICS::GVulkanSurface vlkSurface;
		VkDevice device = nullptr;
		VkPhysicalDevice physicalDevice = nullptr;
		VkRenderPass renderPass;
		VkShaderModule vertexShader = nullptr;
		VkShaderModule fragmentShader = nullptr;
		VkPipeline pipeline = nullptr;
		VkPipelineLayout pipelineLayout = nullptr;
		GW::MATH::GMATRIXF projMatrix;
		VkDescriptorSetLayout descriptorLayout = nullptr;
		VkDescriptorPool descriptorPool = nullptr;
		std::vector<VkDescriptorSet> descriptorSets;
		VkClearValue clrAndDepth[2];

		// NEW: text rendering stuff
		VkShaderModule textVertexShader = VK_NULL_HANDLE;
		VkShaderModule textFragmentShader = VK_NULL_HANDLE;
		VkPipeline     textPipeline = VK_NULL_HANDLE;
		VkPipelineLayout textPipelineLayout = VK_NULL_HANDLE;

		VkBuffer        textVertexBuffer = VK_NULL_HANDLE;
		VkDeviceMemory  textVertexMemory = VK_NULL_HANDLE;
		size_t          textVertexCapacity = 0;
	};

	struct VulkanVertexBuffer
	{
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
	};

	struct VulkanIndexBuffer
	{
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
	};

	struct GeometryData
	{
		unsigned int indexStart, indexCount, vertexStart;
		inline bool operator < (const GeometryData a) const {
			return indexStart < a.indexStart;
		}
	};
	
	struct GPUInstance
	{
		GW::MATH::GMATRIXF	transform;
		H2B::ATTRIBUTES		matData;
	};

	struct VulkanGPUInstanceBuffer
	{
		unsigned long long element_count = 1;
		std::vector<VkBuffer> buffer;
		std::vector<VkDeviceMemory> memory;
	};

	struct SceneData
	{
		GW::MATH::GVECTORF sunDirection, sunColor, sunAmbient, camPos;
		GW::MATH::GMATRIXF viewMatrix, projectionMatrix;
	};

	struct VulkanUniformBuffer
	{
		std::vector<VkBuffer> buffer;
		std::vector<VkDeviceMemory> memory;
	};


	struct Camera
	{
		GW::MATH::GMATRIXF camMatrix;
	};	

	struct CPULevel
	{
		std::string levelFile;
		std::string modelPath;
		Level_Data levelData;
	};

	struct GPULevel{};

	struct DoNotRender{};

	struct MeshCollection
	{
		std::vector<entt::entity> entities;
		GW::MATH::GOBBF collider;
	};

	struct ModelManager
	{
		std::map<std::string, MeshCollection> meshCollections;
	};

	//*** HELPER FUNCTIONS ***//
	static std::vector<entt::entity> GetRenderableEntities(entt::registry& registry, const std::string blenderName)
	{
		DRAW::ModelManager* modelManager = registry.ctx().find<DRAW::ModelManager>();
		if (!modelManager) {
			return {};
		}

		auto iterator = modelManager->meshCollections.find(blenderName);
		if (iterator == modelManager->meshCollections.end()) {
			return {};
		}

		return iterator->second.entities;
	}

	static GW::MATH::GMATRIXF CopyComponents(
		entt::registry& registry,
		const std::vector<entt::entity>& entitiesToCopy,
		DRAW::MeshCollection& meshCollection
	) {
		GW::MATH::GMATRIXF transform = GW::MATH::GIdentityMatrixF;
		bool setTransform = false;

		for (const entt::entity& entityToCopy : entitiesToCopy) {
			DRAW::GPUInstance* gpuInstance = registry.try_get<DRAW::GPUInstance>(entityToCopy);
			if (!gpuInstance) {
				continue;
			}

			DRAW::GeometryData* geometryData = registry.try_get<DRAW::GeometryData>(entityToCopy);
			if (!geometryData) {
				continue;
			}

			if (!setTransform) {
				transform = gpuInstance->transform;
				setTransform = true;
			}

			entt::entity newEntity = registry.create();
			registry.emplace<DRAW::GPUInstance>(newEntity, *gpuInstance);
			registry.emplace<DRAW::GeometryData>(newEntity, *geometryData);

			meshCollection.entities.push_back(newEntity);
		}

		return transform;
	}

	struct TextRenderer
	{
		Font font;
	};

	struct TextPipelineData
	{
		VkPipeline pipeline = VK_NULL_HANDLE;
		VkPipelineLayout layout = VK_NULL_HANDLE;
	};
} // namespace DRAW
#endif // !DRAW_COMPONENTS_H
