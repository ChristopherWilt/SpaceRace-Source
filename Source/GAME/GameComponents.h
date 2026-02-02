#ifndef GAME_COMPONENTS_H_
#define GAME_COMPONENTS_H_

#include "../UTIL/Utilities.h"
#include <optional> //allows optional parameters without needing to assign default values
#include <chrono>
#include <string>
#include <array>
#include "../DRAW/DrawComponents.h"

namespace GAME
{
	void CleanupGameplayEntities(entt::registry& registry);

	static void ApplyPowerUps(
		entt::registry& registry,
		std::string enemyConfigPath,
		entt::entity enemyEntity
	);

	///*** Tags ***///
	struct StartScreen { bool active = true;; };
	struct GameOverScreen {};
	struct RequestReset {};
	struct RequestMainMenu {};
	struct GameOver {};
	struct EndCreditScreen {};
	struct HighScoresScreen {};
	struct SettingsScreen{};
	struct SettingsFromPause {};
	
	struct InitialsEntryScreen
	{
		int score = 0;
	};

	struct PlayerTotalScore
	{
		long total = 0;
	};

	struct HUDData
	{
		std::string text;
		float x = 20.0f;
		float y = 30.0f;
	};

	struct HowToPlayScreen {};

	struct HowToPlayState
	{
		// 0 = Controls & Power Ups, 1 = Enemy Types & Drops
		int page = 0;
	};

	struct SettingsData {
		float masterVolume = 1.0f;
		float musicVolume = 1.0f;
		float sfxVolume = 1.0f;

		int selectedSlider = 0;
	};
	
	// Game Over overlay data for fade-in timing
	struct GameOverOverlayData {
		std::chrono::steady_clock::time_point start;
	};

	struct EndCreditsOverlayData  
	{
		std::chrono::steady_clock::time_point start;
	};

	struct PauseMenuState
	{
		bool active = false;          
		int selected = 0;            

		enum class ConfirmType { None, Restart, MainMenu, Quit };
		ConfirmType confirm = ConfirmType::None;

		bool confirmYesSelected = true; 
	};



	///*** Components ***///
	struct Player {};
	
	struct Enemy
	{
		float speed;
	};

	struct Bullet
	{
		entt::entity ownerEntity = entt::null;
	};

	struct EnemyBullet {};
	struct Star {};

	struct PlayerNameInput
	{
		std::string name = "AAA";  // the initials the player is editing
		int currentIndex = 0;      // which character is selected (0..maxLength-1)
		int maxLength = 3;         // how many characters allowed
		bool entryComplete = false; // set to true when the player confirms the name
	};


	// Track player lives
	struct Lives
	{
		int remaining = 3;
	};

	// Handle respawn delay
	struct RespawnDelay
	{
		float timeRemaining = 0.0f;
		float totalDelay = 2.0f;  // Delay before respawn
	};

	// Tag to indicate a player death is being processed
	struct PlayerDying {};

	struct Transform
	{
		GW::MATH::GMATRIXF transformMatrix;
	};

	struct GameManager {};

	struct FireState
	{
		float cooldown;
	};

	struct EntityMovement
	{
		GW::MATH::GVECTORF velocity;
	};

	struct Collidable {};

	struct Obstacle {};

	struct ToDestroy {};

	struct Health
	{
		int hitPoints;
	};

	struct Shatters
	{
		int shatterPoints;
	};

	struct Invulnerable
	{
		float cooldown;
	};

	struct FlashRed
	{
		H2B::VECTOR originalColor;
		double timeLeft;
	};

	struct PlayerCount {
		int total;
		int alive;
	};

	struct SpawnValues
	{
		float speed;
		int hitPoints;
		int shatterPoints;
		GW::MATH::GVECTORF shatterScale;
		GW::MATH::GMATRIXF* spawnLocation;
	};

	struct Explosive
	{
		int radius;
		int damage;
		float nukeChance;
	};

	struct ActivePowerUps
	{
		double doubleFireRate;
		double doubleGun;
	};

	struct Score
	{
		long score;
	};

	static enum PowerUpType
	{
		NONE,
		EXTRA_HEALTH,
		DOUBLE_GUN,
		DOUBLE_FIRE_RATE,
		NUKE
	};

	struct PowerUp {
		PowerUpType powerUpType;
		double duration;

		std::string GetConfigPath() {
			switch (powerUpType) {
				case PowerUpType::EXTRA_HEALTH: return "PowerUpExtraHealth";
				case PowerUpType::DOUBLE_FIRE_RATE: return "PowerUpDoubleFireRate";
				case PowerUpType::DOUBLE_GUN: return "PowerUpDoubleGun";
				case PowerUpType::NUKE: return "PowerUpNuke";
			}
			return ""; // Added a default return to avoid warnings
		}
	};

	struct SpawnsPowerUp {
		PowerUpType powerUpType;
		float spawnChance;
	};

	static GAME::PowerUpType PowerUpFromString(std::string powerUpType) {
		static const std::unordered_map<std::string, GAME::PowerUpType> powerUpTypes = {
			{"NONE",				GAME::PowerUpType::NONE				},
			{"EXTRA_HEALTH",		GAME::PowerUpType::EXTRA_HEALTH		},
			{"DOUBLE_FIRE_RATE",	GAME::PowerUpType::DOUBLE_FIRE_RATE	},
			{"DOUBLE_GUN",			GAME::PowerUpType::DOUBLE_GUN		},
			{"NUKE",				GAME::PowerUpType::NUKE				}
		};

		if (auto it = powerUpTypes.find(powerUpType); it != powerUpTypes.end()) {
			return it->second;
		}

		return GAME::PowerUpType::NONE;
	}

	static H2B::VECTOR GetPowerUpColor(GAME::PowerUpType powerUpType) {
		switch (powerUpType) {
			case DOUBLE_FIRE_RATE:	{ return { 0.189992, 0.239881, 0.800726 }; }
			case DOUBLE_GUN:		{ return { 0.800726, 0.239881, 0.189992 }; }
			case NUKE:				{ return { 0.700726, 0.739881, 0.000103 }; }
			default:				{ return { 0.239881, 0.800726, 0.189992 }; }
		}
	}

	struct SpecialCooldown
	{
		double blueCooldown;
		double redCooldown;
		double yellowCooldown;
	};

	///wave stuff///
	struct Lane
	{
		GW::MATH::GMATRIXF location = GW::MATH::GMATRIXF{ {
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
			} };
		bool isOccupied = false;
		std::vector<entt::entity> enemiesInLane;

		//time lane became occupied
		float timeSpentInLane;

		//time between enemy dives from lane
		float timeSinceLastEnemyDive;

		//how long enemies can stay in lane before leaving
		int duration;
	};
	struct WaveInfo
	{
		std::vector<Lane> lanesContainer;
		float timeSinceLastWaveStart = 0.0f;
		float timeLastEnemySpawned = 0.0f;
		GW::MATH::GMATRIXF SpawnLocation;
		Lane* SelectedLane;
		int EnemiesToSpawn;
		int EnemiesAlreadySpawned = 0;
		bool waveInProgress = false;
	};
	struct StageInfo
	{
		int EnemiesToSpawn = 0;
		float SpeedModifier = 0.0f;
		int EnemiesAlreadySpawned = 0;
		int StageNumber = 0;
		int enemiesEscaped = 0;
		int enemiesKilled = 0; //need to change this in collision check?
		float timeSinceLastEnemyShoot = 0.0f;
	};
	struct EntityDirectionalMovement
	{
		GW::MATH::GVECTORF velocity;
		float destinationX;
		float destinationZ;
		GW::MATH::GMATRIXF finalDestination;
	};
	struct StageIntermission
	{
		float time;
	};
	struct WaveLogic
	{
		WaveInfo waveInfo;
		StageInfo stageInfo;
	};
	struct PauseWave {};	
	struct enemyState
	{
		enum states
		{
			Spawn,
			MovingToMidPoint,
			Looping1,
			Looping2,
			Looping3,
			Looping4,
			MovingToLane,
			InLane,
			Shooting,
			Diving,
			Leaving
		};

		states currentState = states::Spawn;
	};
	///***///

	///*** Nuke ***///

	// This component is added to the context to run the *destruction logic*
	// It will last 1.5 seconds and expand to its max radius.
	struct NukeBlastWave {
		entt::entity playerEntity = entt::null;
		float totalTime = 1.0f;   // Total time to destroy enemies
		float timeActive = 0.0f;
		float maxRadius = 60.0f;  // Max radius to clear
	};

	// This tag is added to the context to trigger the nuke
	struct ActivateNuke
	{
		entt::entity playerEntity = entt::null;
	};
	///***///

	static void SetColor(entt::registry& registry, entt::entity entity, H2B::VECTOR color) {
		UTIL::DeltaTime* deltaTimeComponent = registry.ctx().find<UTIL::DeltaTime>();
		if (!deltaTimeComponent) {
			return;
		}

		DRAW::GPUInstance* gpuInstance = registry.try_get<DRAW::GPUInstance>(entity);
		if (!gpuInstance) {
			return;
		}

		gpuInstance->matData.Kd = color;
	}

	static GW::MATH::GOBBF GetCollider(entt::registry& registry, const entt::entity& entity)
	{
		DRAW::MeshCollection meshCollection = registry.get<DRAW::MeshCollection>(entity);
		GAME::Transform& transform = registry.get<GAME::Transform>(entity);
		GW::MATH::GOBBF collider = meshCollection.collider;

		GW::MATH::GVECTORF scale;
		GW::MATH::GMatrix::GetScaleF(transform.transformMatrix, scale);

		collider.extent.x *= std::fabs(scale.x);
		collider.extent.y *= std::fabs(scale.y);
		collider.extent.z *= std::fabs(scale.z);

		GW::MATH::GVECTORF localCenter = { collider.center.x, collider.center.y, collider.center.z, 1.0f };
		GW::MATH::GVECTORF worldCenter;
		GW::MATH::GVector::VectorXMatrixF(localCenter, transform.transformMatrix, worldCenter);
		collider.center = worldCenter;

		GW::MATH::GQUATERNIONF rotation;
		GW::MATH::GQuaternion::SetByMatrixF(transform.transformMatrix, rotation);
		GW::MATH::GQuaternion::MultiplyQuaternionF(collider.rotation, rotation, collider.rotation);

		return collider;
	}

	static void SpawnPlayer(entt::registry& registry)
	{
		std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;
		DRAW::ModelManager* modelManager = registry.ctx().find<DRAW::ModelManager>();
		if (!modelManager) {
			return;
		}
		entt::entity playerEntity = registry.create();
		DRAW::MeshCollection& playerMeshCollection = registry.emplace<DRAW::MeshCollection>(playerEntity);
		GAME::Transform& playerTransform = registry.emplace<GAME::Transform>(playerEntity);
		registry.emplace<GAME::Player>(playerEntity);
		registry.emplace<GAME::Collidable>(playerEntity);
		registry.emplace<GAME::Score>(playerEntity);

		GAME::Health& playerHealth = registry.emplace<GAME::Health>(playerEntity);
		playerHealth.hitPoints = (*config).at("Player").at("hitpoints").as<int>();

		// Add Lives component or check if lives exist in context from previous death
		GAME::Lives* contextLives = registry.ctx().find<GAME::Lives>();
		if (contextLives) {
			// Restore lives from context
			registry.emplace<GAME::Lives>(playerEntity, *contextLives);
			registry.ctx().erase<GAME::Lives>();
			std::cout << "Player respawned with " << contextLives->remaining << " lives remaining" << std::endl;
		}
		else {
			// Read lives from config
			GAME::Lives& lives = registry.emplace<GAME::Lives>(playerEntity);
			lives.remaining = (*config).at("Player").at("lives").as<int>();
			std::cout << "Player spawned with " << lives.remaining << " lives" << std::endl;
		}

		std::string playerModel = (*config).at("Player").at("model").as<std::string>();
		std::vector<entt::entity> playerMeshEntities = DRAW::GetRenderableEntities(registry, playerModel);
		playerTransform.transformMatrix = DRAW::CopyComponents(registry, playerMeshEntities, playerMeshCollection);
		playerMeshCollection.collider = modelManager->meshCollections[playerModel].collider;

		registry.emplace<GAME::ActivePowerUps>(playerEntity);

		PlayerCount* playerCount = registry.ctx().find<PlayerCount>();
		if (!playerCount) {
			playerCount = &registry.ctx().emplace<PlayerCount>();
		}
		playerCount->total++;
		playerCount->alive++;
	}

	static void MakePlayerInvulnerable(entt::registry& registry, const entt::entity& playerEntity)
	{
		std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;
		GAME::Invulnerable& invulnerable = registry.emplace<GAME::Invulnerable>(playerEntity);
		invulnerable.cooldown = (*config).at("Player").at("invulnPeriod").as<float>();
	}

	static void EmplaceScore(entt::registry& registry, entt::entity entity, std::string configPath)
	{
		std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;
		if (!config) {
			return;
		}

		auto minScoreIt = (*config).at(configPath).find("minScore");
		auto maxScoreIt = (*config).at(configPath).find("maxScore");

		long minScore = 0;
		long maxScore = 0;

		if (minScoreIt != ((*config).at(configPath).end())) {
			minScore = (*config).at(configPath).at("minScore").as<long>();
		}

		if (maxScoreIt != ((*config).at(configPath).end())) {
			maxScore = (*config).at(configPath).at("maxScore").as<long>();
		}

		if (minScore > maxScore) {
			long tempMax = maxScore;
			maxScore = minScore;
			minScore = tempMax;
		}

		if (maxScore == 0) {
			return;
		}

		GAME::Score& scoreComponent = registry.emplace<GAME::Score>(entity);
		if (minScore == maxScore) {
			scoreComponent.score = minScore * 10;
			return;
		}

		long score = 0;
		UTIL::Random random(minScore, maxScore);
		score = random.next() * 10;

		scoreComponent.score = score;
	}

	static void SpawnPowerup(entt::registry& registry, PowerUpType powerUpType, Transform spawn) {
		std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;
		DRAW::ModelManager* modelManager = registry.ctx().find<DRAW::ModelManager>();
		if (!modelManager) {
			return;
		}

		GAME::PowerUp powerUp;
		powerUp.powerUpType = powerUpType;
		powerUp.duration = 0.0;

		std::string configPath = powerUp.GetConfigPath();
		std::string model = (*config).at(configPath).at("model").as<std::string>();
		float speed = (*config).at(configPath).at("speed").as<float>();

		auto durationIt = (*config).at(configPath).find("duration");
		if (durationIt != ((*config).at(configPath).end())) {
			powerUp.duration = (*config).at(configPath).at("duration").as<double>();
		}

		std::vector<entt::entity> meshEntities = DRAW::GetRenderableEntities(registry, model);

		H2B::VECTOR color = GetPowerUpColor(powerUpType);
		for (entt::entity meshEntity : meshEntities) {
			SetColor(registry, meshEntity, color);
		}

		entt::entity powerUpEntity = registry.create();
		registry.emplace<GAME::Collidable>(powerUpEntity);
		registry.emplace<GAME::PowerUp>(powerUpEntity, powerUp);
		EmplaceScore(registry, powerUpEntity, configPath);

		DRAW::MeshCollection& meshCollection = registry.emplace<DRAW::MeshCollection>(powerUpEntity);
		DRAW::CopyComponents(registry, meshEntities, meshCollection);

		meshCollection.collider = modelManager->meshCollections[model].collider;

		GAME::Transform& transform = registry.emplace<GAME::Transform>(powerUpEntity);
		GW::MATH::GMatrix::IdentityF(transform.transformMatrix);
		transform.transformMatrix.row4 = spawn.transformMatrix.row4;

		GAME::EntityMovement& movement = registry.emplace<GAME::EntityMovement>(powerUpEntity);
		movement.velocity = { 0.0f, 0.0f, -speed };
	}

	static void ApplyPowerUps(entt::registry& registry, std::string enemyConfigPath, entt::entity enemyEntity) {
		std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;
		auto powerUpTypeIterator = (*config).at(enemyConfigPath).find("powerUpType");

		if (powerUpTypeIterator == (*config).at(enemyConfigPath).end()) {
			return;
		}

		std::string powerUpTypeString = (*config).at(enemyConfigPath).at("powerUpType").as<std::string>();
		GAME::PowerUpType powerUpType = PowerUpFromString(powerUpTypeString);

		if (powerUpType == GAME::PowerUpType::NONE) {
			return;
		}

		GAME::SpawnsPowerUp& spawns = registry.emplace<GAME::SpawnsPowerUp>(enemyEntity);
		spawns.powerUpType = powerUpType;

		auto powerUpTypeSpawnChanceIterator = (*config).at(enemyConfigPath).find("powerUpSpawnChance");
		if (powerUpTypeSpawnChanceIterator == (*config).at(enemyConfigPath).end()) {
			spawns.spawnChance = 100.0;
			return;
		}

		spawns.spawnChance = (*config).at(enemyConfigPath).at("powerUpSpawnChance").as<float>();
	}

	//GAME::WaveStageFunctions::desiredFunctionName
	namespace WaveStageFunctions
	{
		//clear board of enemies and set stage player should be on
		static void playStage(entt::registry& registry, unsigned int stageNumber)
		{

			//clear board of all enemies
			auto enemyView = registry.view<GAME::Enemy>();
			for (auto enemy : enemyView)
			{
				registry.emplace_or_replace<GAME::ToDestroy>(enemy);
			}

			//reset stage/wave data
			auto waveView = registry.view<GAME::WaveLogic>();
			for (auto entity : waveView)
			{
				GAME::WaveLogic* logic = &registry.get<GAME::WaveLogic>(entity);

				//default wave/stage data
				GAME::WaveInfo newWave;
				GAME::StageInfo newStage;

				//carry over new stage number, lane data, and midPoint data
				newStage.StageNumber = (stageNumber - 1);
				newWave.lanesContainer = logic->waveInfo.lanesContainer;
				
				for (auto laneVector : newWave.lanesContainer)
				{
					//reset these lane values
					laneVector.enemiesInLane.clear();
					laneVector.isOccupied = false;
				}

				//update WaveLogic component
				logic->waveInfo = newWave;
				logic->stageInfo = newStage;


				//emplace an intermission tag
				std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;
				float time = (*config).at("StageInfo").at("timeBetweenStages").as<float>();
				if (registry.ctx().find<GAME::StageIntermission>())
				{
					registry.ctx().erase<GAME::StageIntermission>();
				}
				registry.ctx().emplace<GAME::StageIntermission>(GAME::StageIntermission{ time });

				//display stage # on screen
				std::cout << "Now Entering: Stage " << (logic->stageInfo.StageNumber + 1) << std::endl;
			}
		}


		//get current stage number
		static int getStageNumber(entt::registry& registry)
		{
			auto waveView = registry.view<GAME::WaveLogic>();
			for (auto entity : waveView)
			{
				GAME::WaveLogic* logic = &registry.get<GAME::WaveLogic>(entity);
				return logic->stageInfo.StageNumber + 1;
			}
		}


		//removes WaveLogic component from registry
		static void removeWaveLogicComponent(entt::registry& registry)
		{
			auto waveView = registry.view<GAME::WaveLogic>();
			for (auto entity : waveView)
			{
				registry.erase<GAME::WaveLogic>(entity);
			}
		}


		//pauses all functionality in wave logic
		static void pauseWaveLogic(entt::registry& registry)
		{
			if (!registry.ctx().find<GAME::PauseWave>())
			{
				registry.ctx().emplace<GAME::PauseWave>();
			}
		}


		//unpauses wave logic
		static void resumeWaveLogic(entt::registry& registry)
		{
			if (registry.ctx().find<GAME::PauseWave>())
			{
				registry.ctx().erase<GAME::PauseWave>();
			}
		}
	}

	//static void SpawnEnemy(entt::registry& registry, const GAME::SpawnValues& spawnValues)
//{
//	std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;
//	DRAW::ModelManager* modelManager = registry.ctx().find<DRAW::ModelManager>();
//	if (!modelManager) {
//		return;
//	}

//	entt::entity enemyEntity = registry.create();
//	DRAW::MeshCollection& enemyMeshCollection = registry.emplace<DRAW::MeshCollection>(enemyEntity);
//	GAME::Transform& enemyTransform = registry.emplace<GAME::Transform>(enemyEntity);
//	registry.emplace<GAME::Enemy>(enemyEntity);
//	registry.emplace<GAME::Collidable>(enemyEntity);

//	GAME::EntityMovement& enemyMovement = registry.emplace<GAME::EntityMovement>(enemyEntity);
//	GW::MATH::GVector::ScaleF(UTIL::GetRandomVelocityVector(), spawnValues.speed, enemyMovement.velocity);

//	std::string enemyModel = (*config).at("Enemy1").at("model").as<std::string>();
//	std::vector<entt::entity> enemyMeshEntities = DRAW::GetRenderableEntities(registry, enemyModel);
//	enemyTransform.transformMatrix = DRAW::CopyComponents(registry, enemyMeshEntities, enemyMeshCollection);
//	if (spawnValues.spawnLocation) {
//		enemyTransform.transformMatrix = *spawnValues.spawnLocation;
//	}

//	enemyMeshCollection.collider = modelManager->meshCollections[enemyModel].collider;

//	GAME::Health& enemyHealthComponent = registry.emplace<GAME::Health>(enemyEntity);
//	enemyHealthComponent.hitPoints = spawnValues.hitPoints;

//	if (spawnValues.shatterPoints > 0) {
//		GAME::Shatters& enemyShattersComponent = registry.emplace<GAME::Shatters>(enemyEntity);
//		enemyShattersComponent.shatterPoints = spawnValues.shatterPoints;
//	}

//	GW::MATH::GMatrix::ScaleGlobalF(enemyTransform.transformMatrix, spawnValues.shatterScale, enemyTransform.transformMatrix);
//	ApplyPowerUps(registry, "Enemy1", enemyEntity);
//}

//static void SpawnEnemy(entt::registry& registry)
//{
//	std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;
//	DRAW::ModelManager* modelManager = registry.ctx().find<DRAW::ModelManager>();
//	if (!modelManager) {
//		return;
//	}

//	GAME::SpawnValues spawnValues = {};
//	spawnValues.speed = (*config).at("Enemy1").at("speed").as<float>();
//	spawnValues.hitPoints = (*config).at("Enemy1").at("hitpoints").as<int>();
//	spawnValues.shatterPoints = (*config).at("Enemy1").at("initialShatterCount").as<int>();
//	spawnValues.shatterScale = { 1.0f, 1.0f, 1.0f, 0.0f };

//	SpawnEnemy(registry, spawnValues);
//}

	//static void SpawnExplosiveEnemy(entt::registry& registry, std::optional<GW::MATH::GMATRIXF> spawnLocation)
//{
//	std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;
//	DRAW::ModelManager* modelManager = registry.ctx().find<DRAW::ModelManager>();
//	if (!modelManager) {
//		return;
//	}

//	float speed = (*config).at("EnemyExplosive").at("speed").as<float>();
//	int hitPoints = (*config).at("EnemyExplosive").at("hitpoints").as<int>();
//	int explosionSize = (*config).at("EnemyExplosive").at("explosionSize").as<int>();
//	int explosionDamage = (*config).at("EnemyExplosive").at("explosionDamage").as<int>();
//	float nukeChance = (*config).at("EnemyExplosive").at("nukeChance").as<float>();

//	entt::entity enemyEntity = registry.create();

//	DRAW::MeshCollection& enemyMeshCollection = registry.emplace<DRAW::MeshCollection>(enemyEntity);
//	GAME::Transform& enemyTransform = registry.emplace<GAME::Transform>(enemyEntity);
//	registry.emplace<GAME::Enemy>(enemyEntity);
//	registry.emplace<GAME::Collidable>(enemyEntity);

//	//movement
//	GAME::EntityMovement& enemyMovement = registry.emplace<GAME::EntityMovement>(enemyEntity);
//	GW::MATH::GVector::ScaleF(UTIL::GetRandomVelocityVector(), speed, enemyMovement.velocity);

//	std::string enemyModel = (*config).at("EnemyExplosive").at("model").as<std::string>();
//	std::vector<entt::entity> enemyMeshEntities = DRAW::GetRenderableEntities(registry, enemyModel);
//	enemyTransform.transformMatrix = DRAW::CopyComponents(registry, enemyMeshEntities, enemyMeshCollection); //will default to blender location if no location passed in
//	if (spawnLocation.has_value()) {
//		enemyTransform.transformMatrix = spawnLocation.value();
//	}

//	//collision box
//	enemyMeshCollection.collider = modelManager->meshCollections[enemyModel].collider;

//	//health component
//	GAME::Health& enemyHealthComponent = registry.emplace<GAME::Health>(enemyEntity);
//	enemyHealthComponent.hitPoints = hitPoints;

//	//explosive component
//	GAME::Explosive& explosiveComponent = registry.emplace<GAME::Explosive>(enemyEntity);
//	explosiveComponent.radius = explosionSize;
//	explosiveComponent.damage = explosionDamage;
//	explosiveComponent.nukeChance = nukeChance;
//}



	static void createBackgroundStars(entt::registry& registry)
	{
		std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;
		
		DRAW::ModelManager* modelManager = registry.ctx().find<DRAW::ModelManager>();
		if (!modelManager) {
			return;
		}

		int totalStars = (*config).at("SpaceBackground").at("numOfStars").as<int>();
		for (int i = 0; i < totalStars; ++i)
		{
			entt::entity starEntity = registry.create();
			DRAW::MeshCollection& starMeshCollection = registry.emplace<DRAW::MeshCollection>(starEntity);
			GAME::Transform& starTransform = registry.emplace<GAME::Transform>(starEntity);
			registry.emplace<GAME::Star>(starEntity);
			GAME::EntityMovement& starMovement = registry.emplace<GAME::EntityMovement>(starEntity);


			GW::MATH::GVECTORF direction{ {
			0.0f,
			0.0f,
			-1.0f,
			0.0f
			} };

			float starSpeed = (*config).at("SpaceBackground").at("speed").as<float>();

			GW::MATH::GVECTORF velocity;
			GW::MATH::GVector::ScaleF(direction, starSpeed, velocity);
			starMovement.velocity = velocity;


			
			std::string starModel = (*config).at("SpaceBackground").at("starModel").as<std::string>();
			std::vector<entt::entity> starMeshEntities = DRAW::GetRenderableEntities(registry, starModel);
			starTransform.transformMatrix = DRAW::CopyComponents(registry, starMeshEntities, starMeshCollection);
						
			int xPos = (*config).at("SpaceBackground").at("Xstar" + std::to_string(i)).as<int>();
			int zPos = (*config).at("SpaceBackground").at("Zstar" + std::to_string(i)).as<int>();
			starTransform.transformMatrix.row4.x = xPos;
			starTransform.transformMatrix.row4.z = zPos;
		}		
	}

}// namespace GAME
#endif // !GAME_COMPONENTS_H_