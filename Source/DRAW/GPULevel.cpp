#include "DrawComponents.h"
#include "../GAME/GameComponents.h"
#include "../CCL.h"

namespace DRAW
{

	void Construct_GPULevel(entt::registry& registry, entt::entity entity)
	{
		GW::SYSTEM::GLog log;
		log.EnableConsoleLogging(true);

		DRAW::CPULevel* cpuLevel = registry.try_get<DRAW::CPULevel>(entity);
		if (!cpuLevel) {
			log.Log("Could not get the CPU Level from the entity!");
			return;
		}

		// Emplace buffers
		VulkanVertexBuffer& vertexBuffer = registry.emplace<VulkanVertexBuffer>(entity);
		VulkanIndexBuffer& indexBuffer = registry.emplace<VulkanIndexBuffer>(entity);

		// Emplace respective data
		registry.emplace<std::vector<H2B::VERTEX>>(entity, cpuLevel->levelData.levelVertices);
		registry.emplace<std::vector<unsigned int>>(entity, cpuLevel->levelData.levelIndices);

		// Patch to entity (calls respective components' update method)
		registry.patch<VulkanVertexBuffer>(entity);
		registry.patch<VulkanIndexBuffer>(entity);

		ModelManager* modelManager = registry.ctx().find<ModelManager>();
		if (!modelManager) {
			modelManager = &registry.ctx().emplace<ModelManager>();
		}

		std::vector<Level_Data::BLENDER_OBJECT>& blenderObjects = cpuLevel->levelData.blenderObjects;
		for (const Level_Data::BLENDER_OBJECT& blenderObject : blenderObjects) {
			int modelIndex = blenderObject.modelIndex;
			Level_Data::LEVEL_MODEL& levelModel = cpuLevel->levelData.levelModels[modelIndex];

			int meshStart = levelModel.meshStart;
			int meshCount = levelModel.meshCount;
			int meshEnd = meshStart + meshCount;

			int vertexStart = levelModel.vertexStart;
			int transformIndex = blenderObject.transformIndex;
			GW::MATH::GMATRIXF levelTransform = cpuLevel->levelData.levelTransforms[transformIndex];

			MeshCollection meshCollection = {};

			for (int meshIndex = meshStart; meshIndex < meshEnd; ++meshIndex) {
				H2B::MESH& mesh = cpuLevel->levelData.levelMeshes[meshIndex];
				H2B::BATCH& drawInfo = mesh.drawInfo;

				entt::entity meshEntity = registry.create();

				DRAW::GeometryData geometryData = {
					levelModel.indexStart + drawInfo.indexOffset,
					drawInfo.indexCount,
					vertexStart
				};

				registry.emplace<GeometryData>(meshEntity, geometryData);
				
				if (levelModel.isDynamic) {
					registry.emplace<DoNotRender>(meshEntity);
				}

				int materialIndex = levelModel.materialStart + mesh.materialIndex;
				H2B::MATERIAL& material = cpuLevel->levelData.levelMaterials[materialIndex];

				DRAW::GPUInstance gpuInstance = {
					levelTransform,
					material.attrib
				};
				registry.emplace<GPUInstance>(meshEntity, gpuInstance);

				if (levelModel.isDynamic) {
					meshCollection.entities.push_back(meshEntity);
				}
			}

			int levelColliderIndex = levelModel.colliderIndex;
			meshCollection.collider = cpuLevel->levelData.levelColliders[levelColliderIndex];

			if (levelModel.isCollidable) {
				entt::entity colliderEntity = registry.create();
				registry.emplace<GAME::Collidable>(colliderEntity);
				registry.emplace<GAME::Obstacle>(colliderEntity);

				GAME::Transform& transform = registry.emplace<GAME::Transform>(colliderEntity);
				transform.transformMatrix = levelTransform;

				DRAW::MeshCollection& colliderMeshCollection = registry.emplace<DRAW::MeshCollection>(colliderEntity);
				colliderMeshCollection.collider = meshCollection.collider;
			}

			modelManager->meshCollections[blenderObject.blendername] = meshCollection;
		}
	}

	void Destroy_ModelManager(entt::registry& registry, entt::entity entity)
	{
		ModelManager* modelManager = registry.try_get<ModelManager>(entity);
		if (!modelManager) {
			modelManager = registry.ctx().find<ModelManager>();
			if (!modelManager) {
				return;
			}
		}

		for (auto& [blenderName, meshCollection] : modelManager->meshCollections) {
			for (entt::entity meshEntity : meshCollection.entities) {
				if (registry.valid(meshEntity)) {
					registry.destroy(meshEntity);
				}
			}
		}

		modelManager->meshCollections.clear();
	}

	CONNECT_COMPONENT_LOGIC()
	{
		registry.on_construct<GPULevel>().connect<Construct_GPULevel>();
		registry.on_destroy<ModelManager>().connect<Destroy_ModelManager>();
	}

}