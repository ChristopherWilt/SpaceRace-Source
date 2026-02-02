#include "DrawComponents.h"
#include "../CCL.h"
// component dependencies
#include "Utility/FileIntoString.h"
#include "Utility/TextMeshBuilder.h"
#include "../GAME/GameComponents.h"
#include "../DRAW/Utility/FontLoader.h"

#include <cstring>
#include <iostream>

#include "shaderc/shaderc.h" // needed for compiling shaders at runtime
#ifdef _WIN32 // must use MT platform DLL libraries on windows
#pragma comment(lib, "shaderc_combined.lib") 
#endif

namespace DRAW
{
	//*** HELPER METHODS ***//


	VkViewport CreateViewportFromWindowDimensions(unsigned int windowWidth, unsigned int windowHeight)
	{
		VkViewport retval = {};
		retval.x = 0;
		retval.y = 0;
		retval.width = static_cast<float>(windowWidth);
		retval.height = static_cast<float>(windowHeight);
		retval.minDepth = 0;
		retval.maxDepth = 1;
		return retval;
	}

	VkRect2D CreateScissorFromWindowDimensions(unsigned int windowWidth, unsigned int windowHeight)
	{
		VkRect2D retval = {};
		retval.offset.x = 0;
		retval.offset.y = 0;
		retval.extent.width = windowWidth;
		retval.extent.height = windowHeight;
		return retval;
	}

	void InitializeDescriptors(entt::registry& registry, entt::entity entity)
	{
		auto& vulkanRenderer = registry.get<VulkanRenderer>(entity);

		unsigned int frameCount;
		vulkanRenderer.vlkSurface.GetSwapchainImageCount(frameCount);
		vulkanRenderer.descriptorSets.resize(frameCount);

#pragma region Descriptor Layout
		VkDescriptorSetLayoutBinding layoutBinding[2] = {};
		layoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layoutBinding[0].descriptorCount = 1;
		layoutBinding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		layoutBinding[0].binding = 0;
		layoutBinding[0].pImmutableSamplers = nullptr;
		
		layoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		layoutBinding[1].descriptorCount = 1;
		layoutBinding[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		layoutBinding[1].binding = 1;
		layoutBinding[1].pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo setCreateInfo = {};
		setCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		setCreateInfo.bindingCount = 2;
		setCreateInfo.pBindings = layoutBinding;
		setCreateInfo.flags = 0;
		setCreateInfo.pNext = nullptr;
		vkCreateDescriptorSetLayout(vulkanRenderer.device, &setCreateInfo, nullptr, &vulkanRenderer.descriptorLayout);
#pragma endregion

#pragma region Descriptor Pool
		VkDescriptorPoolCreateInfo descriptorpool_create_info = {};
		descriptorpool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		VkDescriptorPoolSize descriptorpool_size[2] = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, frameCount },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frameCount }
		};
		descriptorpool_create_info.poolSizeCount = 2;
		descriptorpool_create_info.pPoolSizes = descriptorpool_size;
		descriptorpool_create_info.maxSets = frameCount;
		descriptorpool_create_info.flags = 0;
		descriptorpool_create_info.pNext = nullptr;
		vkCreateDescriptorPool(vulkanRenderer.device, &descriptorpool_create_info, nullptr, &vulkanRenderer.descriptorPool);
#pragma endregion

#pragma region Allocate Descriptor Sets
		VkDescriptorSetAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.descriptorPool = vulkanRenderer.descriptorPool;
		allocateInfo.pSetLayouts = &vulkanRenderer.descriptorLayout;
		allocateInfo.pNext = nullptr;
		for (int i = 0; i < frameCount; i++)
		{
			vkAllocateDescriptorSets(vulkanRenderer.device, &allocateInfo, &vulkanRenderer.descriptorSets[i]);
		}
#pragma endregion

		// Add the 2 buffers, this will create the initial buffers so we can finish building our descriptor set
		auto& storageBuffer = registry.emplace<VulkanGPUInstanceBuffer>(entity,
			VulkanGPUInstanceBuffer{16}); // Start with a reasonable size of elements. The Buffer will grow if it needs to later
		auto& uniformBuffer = registry.emplace<VulkanUniformBuffer>(entity);

		 
		for (int i = 0; i < frameCount; i++)
		{
			
			VkDescriptorBufferInfo uniformBufferInfo = { uniformBuffer.buffer[i], 0, VK_WHOLE_SIZE};
			VkWriteDescriptorSet uniformWrite = {};
			uniformWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			uniformWrite.dstSet = vulkanRenderer.descriptorSets[i];
			uniformWrite.dstBinding = 0; // 0 For the uniform buffer
			uniformWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uniformWrite.descriptorCount = 1;
			uniformWrite.pBufferInfo = &uniformBufferInfo;

			VkDescriptorBufferInfo storageBufferInfo = { storageBuffer.buffer[i], 0, VK_WHOLE_SIZE };
			VkWriteDescriptorSet storageWrite = {};
			storageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			storageWrite.dstSet = vulkanRenderer.descriptorSets[i];
			storageWrite.dstBinding = 1; // 1 For the storage buffer
			storageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			storageWrite.descriptorCount = 1;
			storageWrite.pBufferInfo = &storageBufferInfo;

			VkWriteDescriptorSet descriptorWrites[] = { uniformWrite, storageWrite };
			vkUpdateDescriptorSets(vulkanRenderer.device, 2, descriptorWrites, 0, nullptr);
		}

	}

	void InitializeGraphicsPipeline(entt::registry& registry, entt::entity entity)
	{
		auto& vulkanRenderer = registry.get<VulkanRenderer>(entity);
		GW::SYSTEM::GWindow win = registry.get<GW::SYSTEM::GWindow>(entity);

		// Create Pipeline & Layout (Thanks Tiny!)
		VkPipelineShaderStageCreateInfo stage_create_info[2] = {};
		// Create Stage Info for Vertex Shader
		stage_create_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stage_create_info[0].module = vulkanRenderer.vertexShader;
		stage_create_info[0].pName = "main";

		// Create Stage Info for Fragment Shader
		stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stage_create_info[1].module = vulkanRenderer.fragmentShader;
		stage_create_info[1].pName = "main";

		VkPipelineInputAssemblyStateCreateInfo assembly_create_info = {};
		assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		assembly_create_info.primitiveRestartEnable = false;

		VkVertexInputBindingDescription vertex_binding_description = {};
		vertex_binding_description.binding = 0;
		vertex_binding_description.stride = sizeof(H2B::VERTEX);
		vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription vertex_attribute_description[3];
		vertex_attribute_description[0].binding = 0;
		vertex_attribute_description[0].location = 0;
		vertex_attribute_description[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_attribute_description[0].offset = offsetof(H2B::VERTEX, pos);

		vertex_attribute_description[1].binding = 0;
		vertex_attribute_description[1].location = 1;
		vertex_attribute_description[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_attribute_description[1].offset = offsetof(H2B::VERTEX, uvw);

		vertex_attribute_description[2].binding = 0;
		vertex_attribute_description[2].location = 2;
		vertex_attribute_description[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_attribute_description[2].offset = offsetof(H2B::VERTEX, nrm);

		VkPipelineVertexInputStateCreateInfo input_vertex_info = {};
		input_vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		input_vertex_info.vertexBindingDescriptionCount = 1;
		input_vertex_info.pVertexBindingDescriptions = &vertex_binding_description;
		input_vertex_info.vertexAttributeDescriptionCount = 3;
		input_vertex_info.pVertexAttributeDescriptions = vertex_attribute_description;

		unsigned int windowWidth, windowHeight;
		win.GetClientWidth(windowWidth);
		win.GetClientHeight(windowHeight);
		VkViewport viewport = CreateViewportFromWindowDimensions(windowWidth, windowHeight);

		VkRect2D scissor = CreateScissorFromWindowDimensions(windowWidth, windowHeight);

		VkPipelineViewportStateCreateInfo viewport_create_info = {};
		viewport_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_create_info.viewportCount = 1;
		viewport_create_info.pViewports = &viewport;
		viewport_create_info.scissorCount = 1;
		viewport_create_info.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterization_create_info = {};
		rasterization_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization_create_info.rasterizerDiscardEnable = VK_FALSE;
		rasterization_create_info.polygonMode = VK_POLYGON_MODE_FILL;
		rasterization_create_info.lineWidth = 1.0f;
		rasterization_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterization_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterization_create_info.depthClampEnable = VK_FALSE;
		rasterization_create_info.depthBiasEnable = VK_FALSE;
		rasterization_create_info.depthBiasClamp = 0.0f;
		rasterization_create_info.depthBiasConstantFactor = 0.0f;
		rasterization_create_info.depthBiasSlopeFactor = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisample_create_info = {};
		multisample_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisample_create_info.sampleShadingEnable = VK_FALSE;
		multisample_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisample_create_info.minSampleShading = 1.0f;
		multisample_create_info.pSampleMask = VK_NULL_HANDLE;
		multisample_create_info.alphaToCoverageEnable = VK_FALSE;
		multisample_create_info.alphaToOneEnable = VK_FALSE;

		VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = {};
		depth_stencil_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_stencil_create_info.depthTestEnable = VK_TRUE;
		depth_stencil_create_info.depthWriteEnable = VK_TRUE;
		depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
		depth_stencil_create_info.depthBoundsTestEnable = VK_FALSE;
		depth_stencil_create_info.minDepthBounds = 0.0f;
		depth_stencil_create_info.maxDepthBounds = 1.0f;
		depth_stencil_create_info.stencilTestEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
		color_blend_attachment_state.colorWriteMask = 0xF;
		color_blend_attachment_state.blendEnable = VK_FALSE;
		color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
		color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
		color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
		color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo color_blend_create_info = {};
		color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blend_create_info.logicOpEnable = VK_FALSE;
		color_blend_create_info.logicOp = VK_LOGIC_OP_COPY;
		color_blend_create_info.attachmentCount = 1;
		color_blend_create_info.pAttachments = &color_blend_attachment_state;
		color_blend_create_info.blendConstants[0] = 0.0f;
		color_blend_create_info.blendConstants[1] = 0.0f;
		color_blend_create_info.blendConstants[2] = 0.0f;
		color_blend_create_info.blendConstants[3] = 0.0f;

		// Dynamic State 
		VkDynamicState dynamic_states[2] =
		{
			// By setting these we do not need to re-create the pipeline on Resize
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamic_create_info = {};
		dynamic_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_create_info.dynamicStateCount = 2;
		dynamic_create_info.pDynamicStates = dynamic_states;

		InitializeDescriptors(registry, entity);

		VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
		pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_create_info.setLayoutCount = 1;
		pipeline_layout_create_info.pSetLayouts = &vulkanRenderer.descriptorLayout;
		pipeline_layout_create_info.pushConstantRangeCount = 0;
		pipeline_layout_create_info.pPushConstantRanges = nullptr;

		vkCreatePipelineLayout(vulkanRenderer.device, &pipeline_layout_create_info, nullptr, &vulkanRenderer.pipelineLayout);

		// Pipeline State... (FINALLY) 
		VkGraphicsPipelineCreateInfo pipeline_create_info = {};
		pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_create_info.stageCount = 2;
		pipeline_create_info.pStages = stage_create_info;
		pipeline_create_info.pInputAssemblyState = &assembly_create_info;
		pipeline_create_info.pVertexInputState = &input_vertex_info;
		pipeline_create_info.pViewportState = &viewport_create_info;
		pipeline_create_info.pRasterizationState = &rasterization_create_info;
		pipeline_create_info.pMultisampleState = &multisample_create_info;
		pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
		pipeline_create_info.pColorBlendState = &color_blend_create_info;
		pipeline_create_info.pDynamicState = &dynamic_create_info;
		pipeline_create_info.layout = vulkanRenderer.pipelineLayout;
		pipeline_create_info.renderPass = vulkanRenderer.renderPass;
		pipeline_create_info.subpass = 0;
		pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;

		vkCreateGraphicsPipelines(vulkanRenderer.device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &vulkanRenderer.pipeline);
	}

	void InitializeTextPipeline(entt::registry& registry, entt::entity entity)
	{
		auto& vr = registry.get<VulkanRenderer>(entity);
		VkDevice device = vr.device;

		// Vertex layout: TextVertex { float x, y; float u, v; }
		VkVertexInputBindingDescription binding{};
		binding.binding = 0;
		binding.stride = sizeof(DRAW::TextVertex);
		binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription attributes[2]{};
		// position
		attributes[0].location = 0;
		attributes[0].binding = 0;
		attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributes[0].offset = offsetof(DRAW::TextVertex, x);
		// uv (unused for now)
		attributes[1].location = 1;
		attributes[1].binding = 0;
		attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
		attributes[1].offset = offsetof(DRAW::TextVertex, u);

		VkPipelineVertexInputStateCreateInfo vertexInput{};
		vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInput.vertexBindingDescriptionCount = 1;
		vertexInput.pVertexBindingDescriptions = &binding;
		vertexInput.vertexAttributeDescriptionCount = 2;
		vertexInput.pVertexAttributeDescriptions = attributes;

		VkPipelineInputAssemblyStateCreateInfo assembly{};
		assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkPipelineRasterizationStateCreateInfo raster{};
		raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		raster.polygonMode = VK_POLYGON_MODE_FILL;
		raster.cullMode = VK_CULL_MODE_NONE;
		raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		raster.lineWidth = 1.0f;

		VkPipelineDepthStencilStateCreateInfo depth{};
		depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth.depthTestEnable = VK_FALSE;
		depth.depthWriteEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState blendAttach{};
		blendAttach.colorWriteMask = 0xF;
		blendAttach.blendEnable = VK_TRUE;
		blendAttach.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttach.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttach.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttach.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttach.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo blend{};
		blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		blend.attachmentCount = 1;
		blend.pAttachments = &blendAttach;

		VkDynamicState dynStates[2] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamic{};
		dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic.dynamicStateCount = 2;
		dynamic.pDynamicStates = dynStates;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = nullptr; // dynamic
		viewportState.scissorCount = 1;
		viewportState.pScissors = nullptr; // dynamic

		// No descriptors, no push constants
		VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = 0;
		layoutInfo.pSetLayouts = nullptr;
		layoutInfo.pushConstantRangeCount = 0;
		layoutInfo.pPushConstantRanges = nullptr;

		VkPipelineLayout layout = VK_NULL_HANDLE;
		if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
			std::cout << "Failed to create text pipeline layout\n";
			return;
		}

		VkPipelineShaderStageCreateInfo stages[2]{};
		stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stages[0].module = vr.textVertexShader;
		stages[0].pName = "main";

		stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stages[1].module = vr.textFragmentShader;
		stages[1].pName = "main";

		VkGraphicsPipelineCreateInfo pipe{};
		pipe.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipe.stageCount = 2;
		pipe.pStages = stages;
		pipe.pVertexInputState = &vertexInput;
		pipe.pInputAssemblyState = &assembly;
		pipe.pRasterizationState = &raster;
		pipe.pDepthStencilState = &depth;
		pipe.pColorBlendState = &blend;
		pipe.pDynamicState = &dynamic;
		pipe.pViewportState = &viewportState;
		pipe.layout = layout;
		pipe.renderPass = vr.renderPass;
		pipe.subpass = 0;

		VkPipeline pipeline = VK_NULL_HANDLE;
		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipe, nullptr, &pipeline) != VK_SUCCESS) {
			std::cout << "Failed to create text graphics pipeline\n";
			vkDestroyPipelineLayout(device, layout, nullptr);
			return;
		}

		vr.textPipelineLayout = layout;
		vr.textPipeline = pipeline;
	}


	//*** SYSTEMS ***//

	static uint32_t FindMemoryType(
		VkPhysicalDevice physicalDevice,
		uint32_t typeFilter,
		VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
			if ((typeFilter & (1u << i)) &&
				(memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		// If you want to be safer, you can assert/abort here instead
		throw std::runtime_error("Failed to find suitable Vulkan memory type for text buffer.");
	}

	// Helper: create or resize the text vertex buffer (host-visible)
	static void CreateOrResizeTextBuffer(
		entt::registry& registry,
		entt::entity entity,
		VkDeviceSize requiredSizeBytes)
	{
		auto& vr = registry.get<VulkanRenderer>(entity);
		VkDevice device = vr.device;

		// If we already have a buffer large enough, keep it
		if (vr.textVertexBuffer != VK_NULL_HANDLE &&
			requiredSizeBytes <= vr.textVertexCapacity) {
			return;
		}

		// Destroy old buffer if it exists
		if (vr.textVertexBuffer != VK_NULL_HANDLE) {
			vkDestroyBuffer(device, vr.textVertexBuffer, nullptr);
			vkFreeMemory(device, vr.textVertexMemory, nullptr);
			vr.textVertexBuffer = VK_NULL_HANDLE;
			vr.textVertexMemory = VK_NULL_HANDLE;
			vr.textVertexCapacity = 0;
		}

		// Optionally pad size or grow by factor; for now, just use required size
		VkDeviceSize newSize = requiredSizeBytes;
		if (newSize == 0) {
			// Avoid creating zero-sized buffer
			newSize = sizeof(DRAW::TextVertex) * 256;
		}

		VkBufferCreateInfo bufInfo{};
		bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufInfo.size = newSize;
		bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufInfo, nullptr, &vr.textVertexBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create text vertex buffer.");
		}

		VkMemoryRequirements memReq{};
		vkGetBufferMemoryRequirements(device, vr.textVertexBuffer, &memReq);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memReq.size;
		allocInfo.memoryTypeIndex = FindMemoryType(
			vr.physicalDevice,
			memReq.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &vr.textVertexMemory) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate memory for text vertex buffer.");
		}

		vkBindBufferMemory(device, vr.textVertexBuffer, vr.textVertexMemory, 0);

		vr.textVertexCapacity = newSize;
	}

	// run this code when a VulkanRenderer component is connected
	void Construct_VulkanRenderer(entt::registry& registry, entt::entity entity)
	{
		if (!registry.all_of<GW::SYSTEM::GWindow>(entity))
		{
			std::cout << "Window not added to the registry yet!" << std::endl;
			abort();
			return;
		}

		if (!registry.all_of<VulkanRendererInitialization>(entity))
		{
			std::cout << "Initialization Data not added to the registry yet!" << std::endl;
			abort();
			return;
		}

		auto& vulkanRenderer = registry.get<VulkanRenderer>(entity);
		auto& initializationData = registry.get<VulkanRendererInitialization>(entity);

		GW::SYSTEM::GWindow win = registry.get<GW::SYSTEM::GWindow>(entity);		
#ifndef NDEBUG
		const char* debugLayers[] = {
			"VK_LAYER_KHRONOS_validation", // standard validation layer
		};
		if (-vulkanRenderer.vlkSurface.Create(win, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT,
			sizeof(debugLayers) / sizeof(debugLayers[0]),
			debugLayers, 0, nullptr, 0, nullptr, false))
#else
		if (-vulkanRenderer.vlkSurface.Create(win, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT))
#endif
		{
			std::cout << "Failed to create Vulkan Surface!" << std::endl;
			abort();
			return;
		}

		vulkanRenderer.clrAndDepth[0].color = initializationData.clearColor;
		vulkanRenderer.clrAndDepth[1].depthStencil = initializationData.depthStencil;

		// Create Projection matrix
		float aspectRatio;
		vulkanRenderer.vlkSurface.GetAspectRatio(aspectRatio);
		GW::MATH::GMatrix::ProjectionVulkanLHF(G2D_DEGREE_TO_RADIAN_F(initializationData.fovDegrees), aspectRatio, initializationData.nearPlane, initializationData.farPlane, vulkanRenderer.projMatrix);

		
		vulkanRenderer.vlkSurface.GetDevice((void**)&vulkanRenderer.device);
		vulkanRenderer.vlkSurface.GetPhysicalDevice((void**)&vulkanRenderer.physicalDevice);
		vulkanRenderer.vlkSurface.GetRenderPass((void**)&vulkanRenderer.renderPass);

		// Intialize runtime shader compiler HLSL -> SPIRV
		shaderc_compiler_t compiler = shaderc_compiler_initialize();
		shaderc_compile_options_t options = shaderc_compile_options_initialize();
		shaderc_compile_options_set_source_language(options, shaderc_source_language_hlsl);
		shaderc_compile_options_set_invert_y(options, false);
#ifndef NDEBUG
		shaderc_compile_options_set_generate_debug_info(options);
#endif

		// Vertex Shader
		std::string vertexShaderSource = ReadFileIntoString(initializationData.vertexShaderName.c_str());

		shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
			compiler, vertexShaderSource.c_str(), vertexShaderSource.length(),
			shaderc_vertex_shader, "main.vert", "main", options);

		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
		{
			std::cout << "Vertex Shader Errors : \n" << shaderc_result_get_error_message(result) << std::endl;
			abort();
			return;
		}

		GvkHelper::create_shader_module(vulkanRenderer.device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &vulkanRenderer.vertexShader);

		shaderc_result_release(result); // done

		// Fragment Shader
		std::string fragmentShaderSource = ReadFileIntoString(initializationData.fragmentShaderName.c_str());

		result = shaderc_compile_into_spv( // compile
			compiler, fragmentShaderSource.c_str(), fragmentShaderSource.length(),
			shaderc_fragment_shader, "main.frag", "main", options);

		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
		{
			std::cout << "Fragment Shader Errors : \n" << shaderc_result_get_error_message(result) << std::endl;
			abort();
			return;
		}

		GvkHelper::create_shader_module(vulkanRenderer.device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &vulkanRenderer.fragmentShader);

		shaderc_result_release(result); // done

		// --- Text vertex shader ---
		{
			auto vsPath = ResolveAssetPath("Shaders/TextVertexShader.hlsl");
			std::cout << "Text VS path: " << vsPath << std::endl;
			std::string textVSsrc = ReadFileIntoString(vsPath.c_str());

			result = shaderc_compile_into_spv(
				compiler,
				textVSsrc.c_str(),
				textVSsrc.length(),
				shaderc_vertex_shader,
				"TextVertexShader.hlsl",
				"main",
				options
			);

			if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success)
			{
				std::cout << "Text Vertex Shader Errors:\n"
					<< shaderc_result_get_error_message(result) << std::endl;
				abort();
				return;
			}

			GvkHelper::create_shader_module(
				vulkanRenderer.device,
				shaderc_result_get_length(result),
				(char*)shaderc_result_get_bytes(result),
				&vulkanRenderer.textVertexShader
			);

			shaderc_result_release(result);
		}

		// --- Text pixel shader ---
		{
			auto psPath = ResolveAssetPath("Shaders/TextPixel.hlsl");
			std::cout << "Text PS path: " << psPath << std::endl;
			std::string textFSsrc = ReadFileIntoString(psPath.c_str());

			result = shaderc_compile_into_spv(
				compiler,
				textFSsrc.c_str(),
				textFSsrc.length(),
				shaderc_fragment_shader,
				"TextPixel.hlsl",
				"main",
				options
			);

			if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success)
			{
				std::cout << "Text Pixel Shader Errors:\n"
					<< shaderc_result_get_error_message(result) << std::endl;
				abort();
				return;
			}

			GvkHelper::create_shader_module(
				vulkanRenderer.device,
				shaderc_result_get_length(result),
				(char*)shaderc_result_get_bytes(result),
				&vulkanRenderer.textFragmentShader
			);

			shaderc_result_release(result);
		}

		// Free runtime shader compiler resources
		shaderc_compile_options_release(options);
		shaderc_compiler_release(compiler);

		InitializeGraphicsPipeline(registry, entity);

		// Remove the initializtion data as we no longer need it
		registry.remove<VulkanRendererInitialization>(entity);

		// Load font & create TextRenderer
		DRAW::Font consolas = DRAW::LoadFont(
			ResolveAssetPath("Assets/Fonts/consolas_font.json"),
			ResolveAssetPath("Assets/Fonts/consolas_font.png")
		);
		registry.ctx().emplace<DRAW::TextRenderer>(DRAW::TextRenderer{ consolas });

		// Create text overlay pipeline (you still need to fill this in with real shaders)
		InitializeTextPipeline(registry, entity);

		// Create initial HUD text buffer
		VkDeviceSize initialBytes = sizeof(DRAW::TextVertex) * 512;
		CreateOrResizeTextBuffer(registry, entity, initialBytes);
	}

	// run this code when a VulkanRenderer component is updated
	void Update_VulkanRenderer(entt::registry& registry, entt::entity entity)
	{
		auto& vulkanRenderer = registry.get<VulkanRenderer>(entity);

		if (-vulkanRenderer.vlkSurface.StartFrame(2, vulkanRenderer.clrAndDepth))
		{
			std::cout << "Failed to start frame!" << std::endl;
			return;
		}

		auto win = registry.get<GW::SYSTEM::GWindow>(entity);
		unsigned int frame;
		vulkanRenderer.vlkSurface.GetSwapchainCurrentImage(frame);

		VkCommandBuffer commandBuffer;
		unsigned int currentBuffer;
		vulkanRenderer.vlkSurface.GetSwapchainCurrentImage(currentBuffer);
		vulkanRenderer.vlkSurface.GetCommandBuffer(currentBuffer, (void**)&commandBuffer);

		unsigned int windowWidth, windowHeight;
		win.GetClientWidth(windowWidth);
		win.GetClientHeight(windowHeight);
		VkViewport viewport = CreateViewportFromWindowDimensions(windowWidth, windowHeight);
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor = CreateScissorFromWindowDimensions(windowWidth, windowHeight);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanRenderer.pipeline);

		// Update uniform and storage buffers
		registry.patch<VulkanUniformBuffer>(entity);

		// TODO: Build draw instructions

		// Check for presence of the buffers first as they take a few frames before they are created
		if (registry.all_of< VulkanVertexBuffer, VulkanIndexBuffer>(entity))
		{
			auto& vertexBuffer = registry.get<VulkanVertexBuffer>(entity);
			auto& indexBuffer = registry.get<VulkanIndexBuffer>(entity);

			std::map<GeometryData, int> geometryData;

			if (vertexBuffer.buffer != VK_NULL_HANDLE && indexBuffer.buffer != VK_NULL_HANDLE)
			{
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, offsets);
				vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VkIndexType::VK_INDEX_TYPE_UINT32);

				auto group = registry.group<GeometryData>(entt::get<GPUInstance>, entt::exclude<DoNotRender>);
				group.sort<GeometryData>([](const GeometryData& a, const GeometryData& b) {
					return a < b;
					});

				std::vector<GPUInstance> gpuInstances;

				for (auto [meshEntity, geometry, gpuInstance] : group.each()) {
					gpuInstances.push_back(gpuInstance);
					geometryData[geometry] += 1;
				}
				registry.emplace_or_replace<std::vector<GPUInstance>>(entity, std::move(gpuInstances));
				registry.patch<VulkanGPUInstanceBuffer>(entity);
			}

			// TODO: Update buffers here before the bind of the descriptor sets

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanRenderer.pipelineLayout, 0, 1, &vulkanRenderer.descriptorSets[frame], 0, nullptr);

			// TODO: Draw all the things that need drawing
			int instanceOffset = 0;
			for (auto& [geometry, instanceCount] : geometryData) {
				vkCmdDrawIndexed(
					commandBuffer,
					geometry.indexCount,
					instanceCount,
					geometry.indexStart,
					geometry.vertexStart,
					instanceOffset
				);

				instanceOffset += instanceCount;
			}
		}

		if (registry.ctx().contains<DRAW::TextRenderer>() &&
			registry.ctx().contains<GAME::HUDData>() &&
			vulkanRenderer.textPipeline != VK_NULL_HANDLE)
		{
			auto& tr = registry.ctx().get<DRAW::TextRenderer>();
			auto& hud = registry.ctx().get<GAME::HUDData>();

			if (!hud.text.empty()) {
				std::vector<DRAW::TextVertex> verts =
					DRAW::BuildTextMesh(
						tr.font,
						hud.text,
						hud.x,
						hud.y
					);

				if (!verts.empty()) {
					VkDevice device = vulkanRenderer.device;
					VkDeviceSize neededBytes =
						static_cast<VkDeviceSize>(verts.size() * sizeof(DRAW::TextVertex));

					CreateOrResizeTextBuffer(registry, entity, neededBytes);

					if (vulkanRenderer.textVertexBuffer != VK_NULL_HANDLE) {
						void* mapped = nullptr;
						if (vkMapMemory(device, vulkanRenderer.textVertexMemory,
							0, neededBytes, 0, &mapped) == VK_SUCCESS)
						{
							std::memcpy(mapped, verts.data(), static_cast<size_t>(neededBytes));
							vkUnmapMemory(device, vulkanRenderer.textVertexMemory);

							vkCmdBindPipeline(
								commandBuffer,
								VK_PIPELINE_BIND_POINT_GRAPHICS,
								vulkanRenderer.textPipeline
							);

							VkBuffer buffers[] = { vulkanRenderer.textVertexBuffer };
							VkDeviceSize offsets[] = { 0 };
							vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

							vkCmdDraw(
								commandBuffer,
								static_cast<uint32_t>(verts.size()),
								1,
								0,
								0
							);
						}
					}
				}
			}
		}
		vulkanRenderer.vlkSurface.EndFrame(true);
	}

	// run this code when a VulkanRenderer component is updated
	void Destroy_VulkanRenderer(entt::registry& registry, entt::entity entity)
	{
		auto& vulkanRenderer = registry.get<VulkanRenderer>(entity);
		// wait till everything has completed
		vkDeviceWaitIdle(vulkanRenderer.device);

		// Remove Buffer components
		registry.remove<VulkanIndexBuffer>(entity);
		registry.remove<VulkanVertexBuffer>(entity);
		registry.remove<VulkanGPUInstanceBuffer>(entity);
		registry.remove<VulkanUniformBuffer>(entity);

		// Text rendering cleanup
		if (vulkanRenderer.textVertexBuffer != VK_NULL_HANDLE) {
			vkDestroyBuffer(vulkanRenderer.device, vulkanRenderer.textVertexBuffer, nullptr);
			vulkanRenderer.textVertexBuffer = VK_NULL_HANDLE;
		}
		if (vulkanRenderer.textVertexMemory != VK_NULL_HANDLE) {
			vkFreeMemory(vulkanRenderer.device, vulkanRenderer.textVertexMemory, nullptr);
			vulkanRenderer.textVertexMemory = VK_NULL_HANDLE;
		}
		if (vulkanRenderer.textPipeline != VK_NULL_HANDLE) {
			vkDestroyPipeline(vulkanRenderer.device, vulkanRenderer.textPipeline, nullptr);
			vulkanRenderer.textPipeline = VK_NULL_HANDLE;
		}
		if (vulkanRenderer.textPipelineLayout != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(vulkanRenderer.device, vulkanRenderer.textPipelineLayout, nullptr);
			vulkanRenderer.textPipelineLayout = VK_NULL_HANDLE;
		}
		if (vulkanRenderer.textVertexShader != VK_NULL_HANDLE) {
			vkDestroyShaderModule(vulkanRenderer.device, vulkanRenderer.textVertexShader, nullptr);
			vulkanRenderer.textVertexShader = VK_NULL_HANDLE;
		}
		if (vulkanRenderer.textFragmentShader != VK_NULL_HANDLE) {
			vkDestroyShaderModule(vulkanRenderer.device, vulkanRenderer.textFragmentShader, nullptr);
			vulkanRenderer.textFragmentShader = VK_NULL_HANDLE;
		}

		vkDestroyDescriptorSetLayout(vulkanRenderer.device, vulkanRenderer.descriptorLayout, nullptr);
		vkDestroyDescriptorPool(vulkanRenderer.device, vulkanRenderer.descriptorPool, nullptr);

		// Release allocated shaders & pipeline for 3D
		vkDestroyShaderModule(vulkanRenderer.device, vulkanRenderer.vertexShader, nullptr);
		vkDestroyShaderModule(vulkanRenderer.device, vulkanRenderer.fragmentShader, nullptr);
		vkDestroyPipelineLayout(vulkanRenderer.device, vulkanRenderer.pipelineLayout, nullptr);
		vkDestroyPipeline(vulkanRenderer.device, vulkanRenderer.pipeline, nullptr);
	}


	// Use this MACRO to connect the EnTT Component Logic
	CONNECT_COMPONENT_LOGIC() {
		// register the Window component's logic
		registry.on_construct<VulkanRenderer>().connect<Construct_VulkanRenderer>();
		registry.on_update<VulkanRenderer>().connect<Update_VulkanRenderer>();
		registry.on_destroy<VulkanRenderer>().connect<Destroy_VulkanRenderer>();
	}

} // namespace DRAW