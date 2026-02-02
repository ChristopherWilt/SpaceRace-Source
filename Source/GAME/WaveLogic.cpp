#include "../UTIL/Utilities.h"
#include "../DRAW/DrawComponents.h"
#include "GameComponents.h"
#include "../CCL.h"
#include "GameAudio.h"



namespace GAME
{

	bool shouldSpawnWave(entt::registry& registry);
	void spawnWave(entt::registry& registry);
	int randomNumber(int min, int max, bool includeMax = false);
	GW::MATH::GMATRIXF selectSpawnLocation(int index);
	void spawnDefaultAlien(entt::registry& registry, GW::MATH::GMATRIXF spawnLocation, GW::MATH::GMATRIXF destinationLocation);
	void moveAliensToDestination(entt::registry& registry);
	void setDestination(entt::registry& registry, entt::entity alien, float destX, float destZ);
	void checkLanesForClearing(entt::registry& registry);
	bool noEnemiesRemain(entt::registry& registry);
	void applyStageModifiers(entt::registry& registry);
	void checkForEnemyShoot(entt::registry& registry);
	void spawnEnemyBullet(entt::registry& registry, entt::entity theShooter);


	std::shared_ptr<const GameConfig> config;
	GAME::WaveLogic* waveLogic;
	


	void Update_WaveLogic(entt::registry& registry, entt::entity entity)
	{
		UTIL::DeltaTime* deltaTimeComponent = registry.ctx().find<UTIL::DeltaTime>();
		GAME::SpecialCooldown * specialCooldown = registry.ctx().find<GAME::SpecialCooldown>();
		if (deltaTimeComponent && specialCooldown)
		{
			float deltaTime = deltaTimeComponent->dtSec;
			specialCooldown->blueCooldown -= deltaTime;
			specialCooldown->redCooldown -= deltaTime;
			specialCooldown->yellowCooldown -= deltaTime;
		}

		if (registry.ctx().find<GAME::PauseWave>())
		{
			return; //game is paused
		}





		if (registry.ctx().find<GAME::StageIntermission>())
		{
			//pause before you start next stage
			float deltaTime = registry.ctx().find<UTIL::DeltaTime>()->dtSec;
			StageIntermission* intermission = &registry.ctx().get<GAME::StageIntermission>();
			
			intermission->time -= deltaTime;

			if (intermission->time <= 0.0f) 
			{
				registry.ctx().erase<GAME::StageIntermission>();
				waveLogic->stageInfo.EnemiesAlreadySpawned = 0;
				waveLogic->stageInfo.timeSinceLastEnemyShoot = 0.0f;
				applyStageModifiers(registry);

				///TODO:: stop displaying previous stage's info on screen
			}
			return;
		}
		
		
		checkLanesForClearing(registry);

	

		//if not currently in a wave, see if it's time to spawn a new wave
		if (!waveLogic->waveInfo.waveInProgress)
		{
			waveLogic->waveInfo.waveInProgress = shouldSpawnWave(registry);
		}
		
		if (waveLogic->waveInfo.waveInProgress)
		{
			//spawn enemies in wave at set intervals
			spawnWave(registry);
		}


		checkForEnemyShoot(registry);
		

		moveAliensToDestination(registry);
	}


	void Construct_WaveLogic(entt::registry& registry, entt::entity entity)
	{
		config = registry.ctx().get<UTIL::Config>().gameConfig;

		waveLogic = &registry.get<GAME::WaveLogic>(entity);
	

		
		//lanes
		int numOfLanes = (*config).at("WaveInfo").at("numberOfLanes").as<int>();
		int lanePadding = (*config).at("WaveInfo").at("paddingBetweenLanes").as<int>();
		for (int i = 0; i < numOfLanes; ++i)
		{
			Lane newLane;
			float laneX = (*config).at("WaveInfo").at("LaneX").as<float>();
			float laneZ = (*config).at("WaveInfo").at("LaneZ").as<float>();

			laneZ -= lanePadding * i; //vertical spacing between lanes

			newLane.location = GW::MATH::GMATRIXF{ {
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				laneX, 0, laneZ, 1.0f
			} };

			waveLogic->waveInfo.lanesContainer.push_back(newLane);
		}
	}


	bool shouldSpawnWave(entt::registry& registry)
	{
		//see if we have exceeded max spawn for this stage
		if (waveLogic->stageInfo.EnemiesAlreadySpawned >= waveLogic->stageInfo.EnemiesToSpawn)
		{
			//see if enemies are still on board
			if (!noEnemiesRemain(registry))
			{
				return false;
			}
			
			///TODO:: display stage info (enemies killed/escaped, score, etc)
			

			//start next stage
			GAME::WaveStageFunctions::playStage(registry, waveLogic->stageInfo.StageNumber + 2);

			return false;
		}



		//try to spawn new wave every x seconds
		float deltaTime = registry.ctx().find<UTIL::DeltaTime>()->dtSec;
		waveLogic->waveInfo.timeSinceLastWaveStart += deltaTime;

		if (waveLogic->waveInfo.timeSinceLastWaveStart >= (*config).at("WaveInfo").at("timeBetweenWaves").as<int>())
		{
			//see if there is an open lane
			for (int i = 0; i < waveLogic->waveInfo.lanesContainer.size(); ++i)
			{
				if (waveLogic->waveInfo.lanesContainer[i].isOccupied)
				{
					continue;
				}
				
				//found an open lane, set it as occupied and setup wave parameters

				waveLogic->waveInfo.lanesContainer[i].isOccupied = true;

				//wave stays on board for x seconds
				waveLogic->waveInfo.lanesContainer[i].timeSpentInLane = 0.0f;
				int minTime = (*config).at("WaveInfo").at("minTimeBeforeWaveLeaves").as<int>();
				int maxTime = (*config).at("WaveInfo").at("maxTimeBeforeWaveLeaves").as<int>();
				waveLogic->waveInfo.lanesContainer[i].duration = randomNumber(minTime, maxTime, true);

				//initialize dive timer
				waveLogic->waveInfo.lanesContainer[i].timeSinceLastEnemyDive = 0.0f;

				//set a random spawn location
				int numOfSpawns = (*config).at("WaveInfo").at("numberOfSpawns").as<int>();
				waveLogic->waveInfo.SpawnLocation = selectSpawnLocation(randomNumber(0, numOfSpawns));

				//set the open lane as selected lane
				waveLogic->waveInfo.SelectedLane = &waveLogic->waveInfo.lanesContainer[i];

				//get random number of enemies to spawn in wave
				int maxNumberOfEnemiesToSpawn = (*config).at("WaveInfo").at("maxNumberOfEnemiesToSpawn").as<int>();
				int minNumberOfEnemiesToSpawn = (*config).at("WaveInfo").at("minNumberOfEnemiesToSpawn").as<int>();
				waveLogic->waveInfo.EnemiesToSpawn = randomNumber(minNumberOfEnemiesToSpawn, maxNumberOfEnemiesToSpawn, true);
				//don't let number of enemies to spawn exceed stage's count cap
				int countRemaining = waveLogic->stageInfo.EnemiesToSpawn - waveLogic->stageInfo.EnemiesAlreadySpawned;
				waveLogic->waveInfo.EnemiesToSpawn = std::clamp(waveLogic->waveInfo.EnemiesToSpawn, waveLogic->waveInfo.EnemiesToSpawn, countRemaining);


				return true;				
			}
		}
		return false;
	}


	void spawnWave(entt::registry& registry)
	{	
		//check to see if all enemies for wave have been spawned
		if (waveLogic->waveInfo.EnemiesAlreadySpawned == waveLogic->waveInfo.EnemiesToSpawn)
		{
			//reset wave bool and reset timer
			waveLogic->waveInfo.waveInProgress = false;
			waveLogic->waveInfo.EnemiesAlreadySpawned = 0;
			waveLogic->waveInfo.timeSinceLastWaveStart = 0.0f;
			return;
		}


		//check to see if enough time has passed since last enemy spawned
		float delay = (*config).at("WaveInfo").at("timeBetweenIndividualSpawns").as<float>();
		float deltaTime = registry.ctx().find<UTIL::DeltaTime>()->dtSec;
		waveLogic->waveInfo.timeLastEnemySpawned += deltaTime;


		///lanes were being cleared before enemies spawned, counter confirms at least 1 enemy spawns before checking if lane is clear
		if (waveLogic->waveInfo.EnemiesAlreadySpawned == 0 || waveLogic->waveInfo.timeLastEnemySpawned >= delay)
		{
			//get position in lane enemy will be in
			GW::MATH::GMATRIXF finalDestination = waveLogic->waveInfo.SelectedLane->location;
			finalDestination.row4.x += waveLogic->waveInfo.EnemiesAlreadySpawned * ((*config).at("WaveInfo").at("paddingBetweenShips").as<float>());

			spawnDefaultAlien(registry, waveLogic->waveInfo.SpawnLocation, finalDestination);
			
			waveLogic->waveInfo.timeLastEnemySpawned = 0.0f;
			++waveLogic->waveInfo.EnemiesAlreadySpawned;
			++waveLogic->stageInfo.EnemiesAlreadySpawned;
		}
	}


	int randomNumber(int min, int max, bool includeMax)
	{
		//when searching for a random number to use an index
		//leave includeMax as false

		
		//if max = 20, min = 19, and includeMax = true
		int newMax = max - min; //20 - 19 = 1
		int num = rand() % (newMax + includeMax); // turn newMax into 2, return = 0 - 1
		return num + min; //if 0 return 19, if 1 return 20
	}


	GW::MATH::GMATRIXF selectSpawnLocation(int index)
	{
		///select the side of the screen to spawn wave based on index		
		///if index == 0, get x and z in .ini file for left side of screen
		///take x and z and place them into a matrix
		///return matrix



		std::string xValue = std::to_string(index) + "SpawnX";
		std::string zValue = std::to_string(index) + "SpawnZ";

		float spawnX = (*config).at("WaveInfo").at(xValue).as<float>();
		float spawnZ = (*config).at("WaveInfo").at(zValue).as<float>();



		return GW::MATH::GMATRIXF{ {
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				spawnX, 0.0f, spawnZ, 1.0f
		} };
	}

	
	void spawnDefaultAlien(entt::registry& registry, GW::MATH::GMATRIXF spawnLocation, GW::MATH::GMATRIXF finalDestination)
	{
		DRAW::ModelManager* modelManager = registry.ctx().find<DRAW::ModelManager>();
		if (!modelManager) {
			return;
		}

		std::string enemyPath = "EnemyGreen";

		UTIL::Random* random = registry.ctx().find<UTIL::Random>();
		GAME::SpecialCooldown* specialCooldown = registry.ctx().find<GAME::SpecialCooldown>();

		if (random) {
			double val = random->next();
			std::cout << "Random: " << val << std::endl;
			if (val > 9 && val <= 14) {
				if (specialCooldown && specialCooldown->blueCooldown <= 0) {
					enemyPath = "EnemyBlue";
					specialCooldown->blueCooldown = 10;
				}
				else if (!specialCooldown) {
					enemyPath = "EnemyBlue";
				}
			}
			else if (val > 4 && val <= 9) {
				if (specialCooldown && specialCooldown->redCooldown <= 0) {
					enemyPath = "EnemyRed";
					specialCooldown->redCooldown = 10;
				}
				else if (!specialCooldown) {
					enemyPath = "EnemyRed";
				}
			}
			else if (val < 4) {
				if (specialCooldown && specialCooldown->yellowCooldown <= 0) {
					enemyPath = "EnemyYellow";
					specialCooldown->yellowCooldown = 10;
				}
				else if (!specialCooldown) {
					enemyPath = "EnemyYellow";
				}
			}
		}

		float speed = (*config).at(enemyPath).at("speed").as<float>();
		speed += waveLogic->stageInfo.SpeedModifier;
		int hitPoints = (*config).at(enemyPath).at("hitpoints").as<int>();

		entt::entity enemyEntity = registry.create();

		DRAW::MeshCollection& enemyMeshCollection = registry.emplace<DRAW::MeshCollection>(enemyEntity);
		GAME::Transform& enemyTransform = registry.emplace<GAME::Transform>(enemyEntity);
		GAME::Enemy& enemyComponent = registry.emplace<GAME::Enemy>(enemyEntity);
		enemyComponent.speed = speed;

		registry.emplace<GAME::Collidable>(enemyEntity);

		GAME::EmplaceScore(registry, enemyEntity, enemyPath);

		//save lane position that enemy will eventually go to
		GAME::EntityDirectionalMovement& enemyMovement = registry.emplace<GAME::EntityDirectionalMovement>(enemyEntity);
		enemyMovement.finalDestination = finalDestination;


		//position
		std::string enemyModel = (*config).at(enemyPath).at("model").as<std::string>();
		std::vector<entt::entity> enemyMeshEntities = DRAW::GetRenderableEntities(registry, enemyModel);
		enemyTransform.transformMatrix = DRAW::CopyComponents(registry, enemyMeshEntities, enemyMeshCollection);
		enemyTransform.transformMatrix.row4.x = spawnLocation.row4.x;
		enemyTransform.transformMatrix.row4.z = spawnLocation.row4.z;

		

		//collision box
		enemyMeshCollection.collider = modelManager->meshCollections[enemyModel].collider;

		//health component
		GAME::Health& enemyHealthComponent = registry.emplace<GAME::Health>(enemyEntity);
		enemyHealthComponent.hitPoints = hitPoints;


		//state
		GAME::enemyState& enemyStateComponent = registry.emplace<GAME::enemyState>(enemyEntity);
		enemyStateComponent.currentState = enemyStateComponent.Spawn;

		//add power-up
		GAME::ApplyPowerUps(registry, enemyPath, enemyEntity);

		//pushback this eneity into the current wave's lane container
		waveLogic->waveInfo.SelectedLane->enemiesInLane.push_back(enemyEntity);
	}


	void moveAliensToDestination(entt::registry& registry)
	{
		///enemies stop moving once they reach their destination
		///IMPORTANT, if enemy is moving too fast then they overshoot their destination and keep moving


		auto gpuInstanceView = registry.view<DRAW::GPUInstance>();
		auto movementView = registry.view<GAME::Enemy, GAME::Transform, DRAW::MeshCollection, GAME::EntityDirectionalMovement, GAME::enemyState>();


		for (auto& viewEntity : movementView) 
		{
			GAME::Transform& transform = movementView.get<GAME::Transform>(viewEntity);
			GAME::EntityDirectionalMovement& directionalMovement = movementView.get<GAME::EntityDirectionalMovement>(viewEntity);
			GAME::enemyState& state = movementView.get<GAME::enemyState>(viewEntity);
			DRAW::MeshCollection& meshCollection = movementView.get<DRAW::MeshCollection>(viewEntity);


			///check if enemy just spawned in, set destination to midpoint
			if (state.currentState == state.Spawn)
			{
				state.currentState = state.MovingToMidPoint;

				float xDest = (*config).at("WaveInfo").at("MidPointX").as<float>();
				float zDest = (*config).at("WaveInfo").at("MidPointZ").as<float>();
				setDestination(registry, viewEntity, xDest, zDest);
			}

			///destroy enemy if too low
			else if (state.currentState == state.Leaving)
			{
				///wall beneath player doesn't always destroy enemies????????
				int maxDepth = (*config).at("WaveInfo").at("zLevelToDestroyEnemy").as<int>();
				if (state.currentState == state.Leaving
					&& transform.transformMatrix.row4.z <= maxDepth)
				{
					//too low, destroy
					++waveLogic->stageInfo.enemiesEscaped;
					registry.emplace_or_replace<GAME::ToDestroy>(viewEntity);
					continue;
				}
			}

			///target player once past specified depth
			else if (state.currentState == state.Diving)
			{
				int targetDepth = (*config).at("WaveInfo").at("zLevelToTargetPlayer").as<int>();

				if (transform.transformMatrix.row4.z <= targetDepth)
				{
					//set state to leaving
					state.currentState = state.Leaving;

					//set destination to player
					auto playerView = registry.view<GAME::Player, GAME::Transform>();

					if (playerView.begin() == playerView.end())
					{
						// No player exists (respawning), continue diving straight down
						setDestination(registry, viewEntity, transform.transformMatrix.row4.x, targetDepth - 100.0f);
					}
					else
					{
						// Player exists, target them
						GAME::Transform& playerTransform = playerView.get<GAME::Transform>(*playerView.begin());

						float xPos = playerTransform.transformMatrix.row4.x;
						float zPos = playerTransform.transformMatrix.row4.z;

						//set direction towards player
						setDestination(registry, viewEntity, xPos, zPos);
					}
				}
			}
			
			///check to see if entity has reached destination
			else if (transform.transformMatrix.row4.x == directionalMovement.destinationX &&
					 transform.transformMatrix.row4.z == directionalMovement.destinationZ)
			{

				///see what state they WERE in
				if (state.currentState == state.InLane)
				{
					//skip
					continue;
				}
				else if (state.currentState == state.MovingToLane)
				{
					//set InLane + skip
					state.currentState = state.InLane;
					continue;
				}
				else if (state.currentState == state.MovingToMidPoint)
				{
					state.currentState = state.Looping1;
					float radius = (*config).at("WaveInfo").at("MidPointRadius").as<float>();
								
					//need to get current direction of enemy, so they continue in loop in that direction
					int direction = (directionalMovement.velocity.x > 0 ? 1 : -1);

					float xDest = transform.transformMatrix.row4.x + (radius * direction);
					float zDest = transform.transformMatrix.row4.z + radius;
					
					

					setDestination(registry, viewEntity, xDest, zDest);
				}
				else if (state.currentState == state.Looping1)
				{
					state.currentState = state.Looping2;
					float radius = (*config).at("WaveInfo").at("MidPointRadius").as<float>();
					
					//need to get reverse direction of enemy
					int direction = (directionalMovement.velocity.x > 0 ? -1 : 1);

					
					float xDest = transform.transformMatrix.row4.x + radius * direction;
					float zDest = transform.transformMatrix.row4.z + radius;
					setDestination(registry, viewEntity, xDest, zDest);
				}	
				else if (state.currentState == state.Looping2)
				{
					state.currentState = state.Looping3;
					float radius = (*config).at("WaveInfo").at("MidPointRadius").as<float>();
					
					//need to get current direction of enemy
					int direction = (directionalMovement.velocity.x > 0 ? 1 : -1);
					
					float xDest = transform.transformMatrix.row4.x + radius * direction;
					float zDest = transform.transformMatrix.row4.z - radius;

					setDestination(registry, viewEntity, xDest, zDest);
				}
				else if (state.currentState == state.Looping3)
				{
					state.currentState = state.Looping4;
					float radius = (*config).at("WaveInfo").at("MidPointRadius").as<float>();
					
					//need to get reverse direction of enemy
					int direction = (directionalMovement.velocity.x > 0 ? -1 : 1);
					
					float xDest = transform.transformMatrix.row4.x + radius * direction;
					float zDest = transform.transformMatrix.row4.z - radius;
					setDestination(registry, viewEntity, xDest, zDest);
				}
				else if (state.currentState == state.Looping4)
				{
					state.currentState = state.MovingToLane;
					setDestination(registry, viewEntity, directionalMovement.finalDestination.row4.x, directionalMovement.finalDestination.row4.z);
				}
				else if (state.currentState == state.Shooting)
				{
					//have enemy shoot down
					spawnEnemyBullet(registry, viewEntity);

					//move back to lane
					state.currentState = state.MovingToLane;
					setDestination(registry, viewEntity, directionalMovement.finalDestination.row4.x, directionalMovement.finalDestination.row4.z);
				}
			}
			

			




			///stop enemy if near destination, ignore destination for leaving/diving enemies
			if (state.currentState != state.Leaving && state.currentState != state.Diving)
			{
				float distance = sqrt(
					pow(directionalMovement.destinationX - transform.transformMatrix.row4.x, 2)
					+
					pow(directionalMovement.destinationZ - transform.transformMatrix.row4.z, 2)
				);
				//have enemy keep moving in current direction if leaving/diving
				if (distance <= 0.25f)
				{
					//entity has reached destination
					transform.transformMatrix.row4.x = directionalMovement.destinationX;
					transform.transformMatrix.row4.z = directionalMovement.destinationZ;


					//adjust mesh to position + set rotation rot to 0
					for (const entt::entity& meshEntity : meshCollection.entities)
					{
						if (!gpuInstanceView.contains(meshEntity))
						{
							continue;
						}

						DRAW::GPUInstance& gpuInstance = registry.get<DRAW::GPUInstance>(meshEntity);
						gpuInstance.transform = transform.transformMatrix;
					}

					continue;
				}
			}
			






			///enemy is still moving, update pos/rot
			GW::MATH::GMATRIXF transformMatrix = transform.transformMatrix;
			GW::MATH::GVECTORF deltaPosition;


			UTIL::DeltaTime* deltaTimeComponent = registry.ctx().find<UTIL::DeltaTime>();
			if (deltaTimeComponent) 
			{
				GW::MATH::GVector::ScaleF(directionalMovement.velocity, deltaTimeComponent->dtSec, deltaPosition);
			}

			GW::MATH::GMatrix::TranslateGlobalF(transformMatrix, deltaPosition, transform.transformMatrix);
			

			//update mesh to reflect new position
			for (const entt::entity& meshEntity : meshCollection.entities) 
			{
				if (!gpuInstanceView.contains(meshEntity)) 
				{
					continue;
				}

				DRAW::GPUInstance& gpuInstance = registry.get<DRAW::GPUInstance>(meshEntity);
				gpuInstance.transform = transform.transformMatrix;


				//rotate ship model in direction ship is moving
				float rot = (*config).at("WaveInfo").at("enemyMovementRot").as<float>();
				if (directionalMovement.velocity.x > 0)
				{
					rot *= -1;
				}
				else if (directionalMovement.velocity.x == 0)
				{
					rot = 0;
				}
				GW::MATH::GMatrix::RotateXLocalF(gpuInstance.transform, G2D_DEGREE_TO_RADIAN_F(rot), gpuInstance.transform);
			}
		}
	}


	void setDestination(entt::registry& registry, entt::entity alien, float destX, float destZ)
	{		
		if (registry.valid(alien) && registry.all_of<GAME::EntityDirectionalMovement>(alien) && registry.all_of<GAME::Transform>(alien))
		{
			GAME::EntityDirectionalMovement& directionalMovement = registry.get<GAME::EntityDirectionalMovement>(alien);
			GAME::Transform& transform = registry.get<GAME::Transform>(alien);
	

			//set destination
			directionalMovement.destinationX = destX;
			directionalMovement.destinationZ = destZ;

			//get new direction vector
			GW::MATH::GVECTORF directionVector = GW::MATH::GVECTORF{ {
			destX - transform.transformMatrix.row4.x,
			0.0f,
			destZ - transform.transformMatrix.row4.z,
			0.0f
			} };
			GW::MATH::GVector::NormalizeF(directionVector, directionVector);
			
			float speed;
			GAME::Enemy* enemyComponent = registry.try_get<GAME::Enemy>(alien);
			if (enemyComponent) {
				speed = enemyComponent->speed;
			}
			else {
				speed = (*config).at("EnemyGreen").at("speed").as<float>();
			}

			GW::MATH::GVector::ScaleF(directionVector, speed, directionalMovement.velocity);
		}	

		return;
	}


	void checkLanesForClearing(entt::registry& registry)
	{
		
		for (int i = 0; i < waveLogic->waveInfo.lanesContainer.size(); ++i)
		{



			if (!waveLogic->waveInfo.lanesContainer[i].isOccupied)
			{
				continue; //lane not occupied, skip
			}



			///this checks if enemies left screen
			if (waveLogic->waveInfo.lanesContainer[i].enemiesInLane.empty())
			{
				waveLogic->waveInfo.lanesContainer[i].isOccupied = false;
				waveLogic->waveInfo.lanesContainer[i].enemiesInLane.clear();
			}
			else
			{
				///this checks if they were killed
				for (auto entity : waveLogic->waveInfo.lanesContainer[i].enemiesInLane)
				{
					if (registry.valid(entity))
					{
						break; //at least 1 ship is still alive, now check time left
					}
					else if (entity == waveLogic->waveInfo.lanesContainer[i].enemiesInLane.back())
					{
						waveLogic->waveInfo.lanesContainer[i].isOccupied = false;
						waveLogic->waveInfo.lanesContainer[i].enemiesInLane.clear();
					}
				}
			}

			



			if (!waveLogic->waveInfo.lanesContainer[i].isOccupied)
			{
				continue; //lane was freed, skip time/dive check
			}



			//check to see if time in lane has exceeded threshold
			float deltaTime = registry.ctx().find<UTIL::DeltaTime>()->dtSec;
			waveLogic->waveInfo.lanesContainer[i].timeSpentInLane += deltaTime;

			if (waveLogic->waveInfo.lanesContainer[i].timeSpentInLane < waveLogic->waveInfo.lanesContainer[i].duration)
			{
				continue; //not time to clear lane yet
			}



			///Have enemies dive offscreen

			//check to see if dive cooldown has finished
			float delay = (*config).at("WaveInfo").at("timeBetweenDives").as<float>();
			waveLogic->waveInfo.lanesContainer[i].timeSinceLastEnemyDive += deltaTime;
			
			if (waveLogic->waveInfo.lanesContainer[i].timeSinceLastEnemyDive < delay)
			{
				continue; //not time to dive yet
			}


			std::vector<entt::entity>& temp = waveLogic->waveInfo.lanesContainer[i].enemiesInLane;
			for (auto it = temp.begin(); it != temp.end();)
			{

				entt::entity& entity = *it;

				if (!registry.valid(entity) && !registry.all_of<GAME::EntityDirectionalMovement>(entity))
				{
					++it;
					continue; //no entity found with movement
				}
				

				///flip a coin, see if this enemy should dive now
				if (randomNumber(0, 1, true))
				{
					++it;
					continue;
				}


				//set vector to straight down
				GAME::EntityDirectionalMovement& directionalMovement = registry.get<GAME::EntityDirectionalMovement>(entity);
				GW::MATH::GVECTORF directionVector = GW::MATH::GVECTORF{ {
				0.0f,
				0.0f,
				-1,
				0.0f
				} };
				GW::MATH::GVector::NormalizeF(directionVector, directionVector);
				
				float speed;
				GAME::Enemy* enemyComponent = registry.try_get<GAME::Enemy>(entity);
				if (enemyComponent) {
					speed = enemyComponent->speed;
				}
				else {
					speed = (*config).at("EnemyGreen").at("speed").as<float>();
				}

				GW::MATH::GVector::ScaleF(directionVector, speed, directionalMovement.velocity);

				//set state to dive
				GAME::enemyState& state = *registry.try_get<GAME::enemyState>(entity);
				state.currentState = state.Diving;

				//remove from vector
				temp.erase(it);

				//reset dive cooldown
				waveLogic->waveInfo.lanesContainer[i].timeSinceLastEnemyDive = 0.0f;
			}
		}
	}


	bool noEnemiesRemain(entt::registry& registry)
	{
		auto enemyView = registry.view<GAME::Enemy>();
		return (enemyView.size() <= 0 ? true : false);
	}


	void applyStageModifiers(entt::registry& registry)
	{
		//enemy count for this stage
		int defaultEnemyCount = (*config).at("StageInfo").at("numberOfEnemiesInStage").as<int>();
		int modifierBonus = (*config).at("StageInfo").at("stageCountModifier").as<int>();
		int newCount = defaultEnemyCount + (waveLogic->stageInfo.StageNumber * modifierBonus);
		//clamp value againt max count
		int maxCount = (*config).at("StageInfo").at("stageCountCap").as<int>();
		waveLogic->stageInfo.EnemiesToSpawn = std::clamp(newCount, defaultEnemyCount, maxCount);


		//enemy speed boost for this stage
		float newSpeed = (*config).at("StageInfo").at("stageSpeedModifier").as<float>() * waveLogic->stageInfo.StageNumber;
		//clamp value againt max speed
		float maxSpeed = (*config).at("StageInfo").at("stageSpeedCap").as<float>();
		waveLogic->stageInfo.SpeedModifier = std::clamp(newSpeed, 0.0f, maxSpeed);
	}


	void checkForEnemyShoot(entt::registry& registry)
	{
		///see if it is time to have an alien break formation to shoot

		float deltaTime = registry.ctx().find<UTIL::DeltaTime>()->dtSec;
		waveLogic->stageInfo.timeSinceLastEnemyShoot += deltaTime;

		if (waveLogic->stageInfo.timeSinceLastEnemyShoot >= (*config).at("StageInfo").at("timeBetweenEnemyShooting").as<int>())
		{
			auto View = registry.view<GAME::Enemy, GAME::EntityDirectionalMovement, GAME::enemyState>();

			std::vector<entt::entity> entityVector;


			for (auto& viewEntity : View)
			{
				GAME::enemyState& state = View.get<GAME::enemyState>(viewEntity);

				//only select aliens that are in lane for shooting
				if (state.currentState != state.InLane)
				{
					continue;
				}

				entityVector.push_back(viewEntity);
			}

			if (!entityVector.empty())
			{
				//select a random enemy in vector
				int index = randomNumber(0, entityVector.size());

				auto& shooter = entityVector[index];
				

				//set destination to above player
				auto playerView = registry.view<GAME::Player, GAME::Transform>();

				if (playerView.begin() == playerView.end())
				{
					// No player exists (respawning), do nothing
				}
				else
				{
					// Player exists, target them (add extra height to z value so ship is above player
										
					GAME::enemyState& state = View.get<GAME::enemyState>(shooter);
					state.currentState = state.Shooting;
					
					GAME::Transform& playerTransform = playerView.get<GAME::Transform>(*playerView.begin());

					float xPos = playerTransform.transformMatrix.row4.x;
					float zPos = playerTransform.transformMatrix.row4.z + (*config).at("StageInfo").at("shooterDistanceFromPlayer").as<int>();

					//set direction towards player
					setDestination(registry, shooter, xPos, zPos);

					//reset timer
					waveLogic->stageInfo.timeSinceLastEnemyShoot = 0.0f;
				}
			}
		}
	}


	void spawnEnemyBullet(entt::registry& registry, entt::entity theShooter)
	{

		if (!registry.valid(theShooter))
		{
			return;
		}


		entt::entity enemyBullet = registry.create();
		registry.emplace<GAME::EnemyBullet>(enemyBullet);
		registry.emplace<GAME::Collidable>(enemyBullet);


		//pos
		GAME::Transform& bulletTransform = registry.emplace<GAME::Transform>(enemyBullet);
		GAME::Transform ShooterTransform = *registry.try_get<GAME::Transform>(theShooter);
		bulletTransform.transformMatrix = ShooterTransform.transformMatrix;

		//bullet height needs to be same height as player ship, else bullet misses
		auto playerView = registry.view<GAME::Player, GAME::Transform>();
		if (playerView.begin() != playerView.end())
		{
			bulletTransform.transformMatrix.row4.y = registry.get<GAME::Transform>(*playerView.begin()).transformMatrix.row4.y;
		}
		

		//velocity - set to straight down
		GAME::EntityMovement& entityMovement = registry.emplace<GAME::EntityMovement>(enemyBullet);
		
		GW::MATH::GVECTORF direction { {
			0.0f,
			0.0f,
			-1.0f,
			0.0f
		} };

		float bulletSpeed = (*config).at("Bullet").at("speed").as<float>();

		GW::MATH::GVECTORF velocity;
		GW::MATH::GVector::ScaleF(direction, bulletSpeed, velocity);
		entityMovement.velocity = velocity;


		//model
		std::string model = (*config).at("Bullet").at("model").as<std::string>();
		std::vector<entt::entity> meshEntities = DRAW::GetRenderableEntities(registry, model);
		DRAW::MeshCollection& meshCollection = registry.emplace<DRAW::MeshCollection>(enemyBullet);
		DRAW::CopyComponents(registry, meshEntities, meshCollection);
		DRAW::ModelManager* modelManager = registry.ctx().find<DRAW::ModelManager>();
		if (modelManager) {
			meshCollection.collider = modelManager->meshCollections[model].collider;
		}

		///this is not working???? 
		///model always has a red tint added to it
		H2B::VECTOR color = H2B::VECTOR{ 0.0, 0.0, 1.0 };
		for (entt::entity meshEntity : meshCollection.entities) {
			SetColor(registry, meshEntity, color);
		}



		//play shooting noise
		auto& audio = registry.ctx().get<GameAudio>();
		audio.Play("blaster");
	}


	CONNECT_COMPONENT_LOGIC()
	{
		registry.on_construct<GAME::WaveLogic>().connect<Construct_WaveLogic>();
		registry.on_update<GAME::WaveLogic>().connect<Update_WaveLogic>();
	}

}
