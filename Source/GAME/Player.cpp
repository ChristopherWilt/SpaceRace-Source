#include "../UTIL/Utilities.h"
#include "../DRAW/DrawComponents.h"
#include "GameComponents.h"
#include "../CCL.h"
#include "GameAudio.h"

namespace GAME {

	void Movement(
		GW::INPUT::GInput& input,
		GAME::Transform* transform,
		const float& speed,
		const float& deltaTime,
		entt::registry& registry
	);

	void Fire(
		entt::registry& registry,
		std::shared_ptr<const GameConfig>& config,
		entt::entity& playerEntity,
		GAME::Transform* playerTransform,
		GW::INPUT::GInput& input,
		const float& deltaTime
	);

	entt::entity CreateBullet(
		entt::registry& registry,
		std::shared_ptr<const GameConfig>& config,
		entt::entity playerEntity,
		GAME::Transform* playerTransform,
		GW::MATH::GVECTORF& bulletVelocity
	);

	void Update_Player(entt::registry& registry, entt::entity entity)
	{
		// Get Components
		UTIL::Input* inputComponent = registry.ctx().find<UTIL::Input>();
		if (!inputComponent) {
			return;
		}

		UTIL::DeltaTime* deltaTimeComponent = registry.ctx().find<UTIL::DeltaTime>();
		if (!deltaTimeComponent) {
			return;
		}

		float deltaTime = deltaTimeComponent->dtSec;

		GAME::ActivePowerUps* activePowerUps = registry.try_get<GAME::ActivePowerUps>(entity);
		if (activePowerUps) {
			activePowerUps->doubleFireRate -= deltaTime;
			activePowerUps->doubleGun -= deltaTime;
		}

		std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;
		float speed = (*config).at("Player").at("speed").as<float>();

		GW::INPUT::GInput& input = inputComponent->immediateInput;

		// Handle Different Updates
		GAME::Transform* transform = registry.try_get<GAME::Transform>(entity);
		if (transform) {
			Movement(input, transform, speed, deltaTime, registry);
		}

		//// Debug: Press 'k' to reduce player health by 1
		//static bool kPressedLast = false;
		//float kKey = 0.0f;
		//input.GetState(G_KEY_K, kKey);

		//if (kKey > 0.5f && !kPressedLast) {
		//	GAME::Health* health = registry.try_get<GAME::Health>(entity);
		//	GAME::Lives* lives = registry.try_get<GAME::Lives>(entity);

		//	if (health && health->hitPoints > 0) {
		//		health->hitPoints -= 1;
		//		std::cout << "Player health reduced by debug command. Remaining HP: " << health->hitPoints << std::endl;

		//		if (health->hitPoints <= 0) {
		//			std::cout << "Player HP reached 0!" << std::endl;

		//			// Check if player has lives remaining
		//			if (lives && lives->remaining > 0) {
		//				// Player has lives = trigger respawn
		//				lives->remaining--;
		//				std::cout << "Player died! " << lives->remaining << " lives remaining!" << std::endl;

		//				// Save lives to context before destroying player
		//				if (registry.ctx().find<GAME::Lives>()) {
		//					registry.ctx().erase<GAME::Lives>();
		//				}
		//				registry.ctx().emplace<GAME::Lives>(*lives);

		//				// Mark player for destruction (DestroyMarkedEntities)
		//				if (!registry.any_of<GAME::ToDestroy>(entity)) {
		//					registry.emplace<GAME::ToDestroy>(entity);
		//				}

		//				// Decrement alive count
		//				PlayerCount* playerCount = registry.ctx().find<GAME::PlayerCount>();
		//				if (playerCount) {
		//					playerCount->alive--;
		//				}

		//				// Create respawn delay
		//				if (registry.ctx().find<GAME::RespawnDelay>()) {
		//					registry.ctx().erase<GAME::RespawnDelay>();
		//				}
		//				GAME::RespawnDelay& respawnDelay = registry.ctx().emplace<GAME::RespawnDelay>();
		//				respawnDelay.timeRemaining = respawnDelay.totalDelay;
		//				std::cout << "Respawning in " << respawnDelay.totalDelay << " seconds..." << std::endl;
		//			}
		//			else {
		//				// No lives remaining = game over
		//				std::cout << "No lives remaining - Game Over!" << std::endl;

		//				PlayerCount* playerCount = registry.ctx().find<GAME::PlayerCount>();
		//				if (playerCount) {
		//					playerCount->alive--;
		//				}

		//				if (!registry.ctx().find<GAME::GameOver>()) {
		//					registry.ctx().emplace<GAME::GameOver>();

		//					auto& audio = registry.ctx().get<GameAudio>();
		//					audio.Play("GameOver");

		//					// Reset the name input in CONTEXT
		//					if (auto* existing = registry.ctx().find<GAME::PlayerNameInput>()) {
		//						registry.ctx().erase<GAME::PlayerNameInput>();
		//					}
		//					auto& nameInput = registry.ctx().emplace<GAME::PlayerNameInput>();
		//					nameInput.name = "AAA";
		//					nameInput.currentIndex = 0;
		//					nameInput.maxLength = 3;
		//					nameInput.entryComplete = false;

		//					// Create a marker entity for the initials screen
		//					entt::entity initialsEntity = registry.create();
		//					registry.emplace<GAME::InitialsEntryScreen>(initialsEntity);
		//					std::cout << "Created InitialsEntryScreen entity: " << (int)initialsEntity << std::endl;
		//				}
		//			}
		//		}
		//	}
		//}
		//kPressedLast = (kKey > 0.5f);

		//// Debug: Press 'n' to activate Nuke
		//static bool nPressedLast = false;
		//float nKey = 0.0f;
		//input.GetState(G_KEY_N, nKey);

		//if (nKey > 0.5f && !nPressedLast) {
		//	// Check if a nuke is already active
		//	if (!registry.ctx().find<GAME::ActivateNuke>()) {
		//		std::cout << "DEBUG: 'N' key pressed. Activating Nuke!" << std::endl;
		//		registry.ctx().emplace<GAME::ActivateNuke>();
		//	}
		//}
		//nPressedLast = (nKey > 0.5f);
		//// End of Nuke Debug Key

		GAME::Invulnerable* invulnerable = registry.try_get<GAME::Invulnerable>(entity);
		if (invulnerable) {
			invulnerable->cooldown -= deltaTime;
			if (invulnerable->cooldown <= 0.0f) {
				registry.remove<GAME::Invulnerable>(entity);
			}
		}

		GAME::FireState* fireState = registry.try_get<GAME::FireState>(entity);
		Fire(registry, config, entity, transform, input, deltaTime);
	}

	void Movement(
		GW::INPUT::GInput& input,
		GAME::Transform* transform,
		const float& speed,
		const float& deltaTime,
		entt::registry& registry
	) {
		GW::MATH::GVECTORF moveVector = { 0.0f, 0.0f, 0.0f, 0.0f };

		// Inputs
		float keyState = 0.0f;

		/*input.GetState(G_KEY_W, keyState);
		if (keyState > 0.5f) {
			moveVector.z += 1.0f;
		}*/

		input.GetState(G_KEY_A, keyState);
		if (keyState > 0.5f) {
			moveVector.x -= 1.0f;
		}

		/*input.GetState(G_KEY_S, keyState);
		if (keyState > 0.5f) {
			moveVector.z -= 1.0f;
		}*/

		input.GetState(G_KEY_D, keyState);
		if (keyState > 0.5f) {
			moveVector.x += 1.0f;
		}


		//tilt player ship in direction of movement
		auto& playerView = registry.view<GAME::Player, DRAW::MeshCollection>();
		auto& gpuInstanceView = registry.view<DRAW::GPUInstance>();
		std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;

		if (playerView.begin() != playerView.end())
		{
			// Player exists
			DRAW::MeshCollection& meshCollection = registry.get<DRAW::MeshCollection>(*playerView.begin());
			for (const entt::entity& meshEntity : meshCollection.entities)
			{
				if (!gpuInstanceView.contains(meshEntity))
				{
					continue;
				}

				DRAW::GPUInstance& gpuInstance = registry.get<DRAW::GPUInstance>(meshEntity);


				//rotate ship model in direction ship is moving
				float rot = (*config).at("WaveInfo").at("enemyMovementRot").as<float>();
				if (moveVector.x < 0)
				{
					rot *= -1;
				}
				else if (moveVector.x == 0)
				{
					rot = 0;
				}
				GW::MATH::GMatrix::RotateXLocalF(gpuInstance.transform, G2D_DEGREE_TO_RADIAN_F(rot), gpuInstance.transform);
			}
		}





		// Movement Math
		float length = sqrt(moveVector.x * moveVector.x + moveVector.z * moveVector.z);
		if (length > 0.0f) {
			moveVector.x /= length;
			moveVector.z /= length;
		}

		moveVector.x *= speed * deltaTime;
		moveVector.z *= speed * deltaTime;

		// Apply Movement
		transform->transformMatrix.row4.x += moveVector.x;
		transform->transformMatrix.row4.x = std::fmax(-42.0f, std::fmin(42.0f, transform->transformMatrix.row4.x));
		transform->transformMatrix.row4.z += moveVector.z;
	}

	void Fire(
		entt::registry& registry,
		std::shared_ptr<const GameConfig>& config,
		entt::entity& playerEntity,
		GAME::Transform* playerTransform,
		GW::INPUT::GInput& input,
		const float& deltaTime
	) {
		FireState* fireState = registry.try_get<GAME::FireState>(playerEntity);
		if (fireState) {
			fireState->cooldown -= deltaTime;

			if (fireState->cooldown > 0.0f) {
				return;
			}

			registry.remove<FireState>(playerEntity);
		}

		float up;
		input.GetState(G_KEY_UP, up);

		GW::MATH::GVECTORF direction = { 0.0f, 0.0f, up, 0.0f };

		float magnitude;
		GW::MATH::GVector::MagnitudeF(direction, magnitude);
		if (magnitude == 0.0f) {
			return;
		}

		GW::MATH::GVector::NormalizeF(direction, direction);
		float bulletSpeed = (*config).at("Bullet").at("speed").as<float>();

		GW::MATH::GVECTORF velocity;
		GW::MATH::GVector::ScaleF(direction, bulletSpeed, velocity);

		GAME::ActivePowerUps* activePowerUps = registry.try_get<GAME::ActivePowerUps>(playerEntity);

		if (activePowerUps && activePowerUps->doubleGun > 0.0) {
			GAME::Transform leftTransform { playerTransform->transformMatrix };
			GAME::Transform rightTransform { playerTransform->transformMatrix };

			leftTransform.transformMatrix.row4.x -= 4;
			rightTransform.transformMatrix.row4.x += 4;

			CreateBullet(registry, config, playerEntity, &leftTransform, velocity);
			CreateBullet(registry, config, playerEntity, &rightTransform, velocity);
		}
		else {
			CreateBullet(registry, config, playerEntity, playerTransform, velocity);
		}

		// --- Play Blaster Audio ---
		auto& audio = registry.ctx().get<GameAudio>();
		audio.Play("blaster");


		FireState& newFireState = registry.emplace<FireState>(playerEntity);
		newFireState.cooldown = (*config).at("Player").at("firerate").as<float>();
		
		if (activePowerUps && activePowerUps->doubleFireRate > 0.0) {
			newFireState.cooldown *= 0.5;
		}
	}

	entt::entity CreateBullet(
		entt::registry& registry,
		std::shared_ptr<const GameConfig>& config,
		entt::entity playerEntity,
		GAME::Transform* playerTransform,
		GW::MATH::GVECTORF& bulletVelocity
	) {
		entt::entity entity = registry.create();
		GAME::Bullet& bullet = registry.emplace<GAME::Bullet>(entity);
		bullet.ownerEntity = playerEntity;

		registry.emplace<GAME::Collidable>(entity);

		GAME::Transform& transform = registry.emplace<GAME::Transform>(entity);
		transform.transformMatrix = playerTransform->transformMatrix;

		GAME::EntityMovement& entityMovement = registry.emplace<GAME::EntityMovement>(entity);
		entityMovement.velocity = bulletVelocity;

		std::string model = (*config).at("Bullet").at("model").as<std::string>();
		std::vector<entt::entity> meshEntities = DRAW::GetRenderableEntities(registry, model);

		for (const entt::entity& meshEntity : meshEntities) {
			DRAW::GPUInstance& gpuInstance = registry.get<DRAW::GPUInstance>(meshEntity);
			gpuInstance.transform = transform.transformMatrix;
		}

		DRAW::MeshCollection& meshCollection = registry.emplace<DRAW::MeshCollection>(entity);
		DRAW::CopyComponents(registry, meshEntities, meshCollection);

		DRAW::ModelManager* modelManager = registry.ctx().find<DRAW::ModelManager>();
		if (modelManager) {
			meshCollection.collider = modelManager->meshCollections[model].collider;
		}

		return entity;
	}

	CONNECT_COMPONENT_LOGIC()
	{
		registry.on_update<GAME::Player>().connect<Update_Player>();
	}

}