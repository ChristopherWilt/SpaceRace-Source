#include "GameComponents.h"
#include "../CCL.h"
#include <random>
#include "GameAudio.h"
#include <sstream>
#include "../DRAW/DrawComponents.h"
#include <cmath>
#include <memory>

//#include <chrono>

namespace GAME
{

    void HandleMovement(entt::registry& registry);
    void CheckCollisions(entt::registry& registry);

    void MarkForDestroy(entt::registry& registry, const entt::entity& entity);

    void HandleCollision(
        entt::registry& registry,
        const entt::entity& entityA,
        const entt::entity& entityB,
        GW::MATH::GOBBF& colliderA,
        GW::MATH::GOBBF& colliderB
    );

    void HandlePowerUpSpawns(
        entt::registry& registry,
        const entt::entity& enemyEntity
    );

    //void HandleShatter(
    //  entt::registry& registry,
    //  const entt::entity& enemyEntity
    //);

    void HandleExplosion(entt::registry& registry, const entt::entity& enemyEntity);

    void HandleFlashRed(entt::registry& registry);

    void UpdateHUD(entt::registry& registry);

    void PatchPlayer(entt::registry& registry);

    void PatchWaveLogic(entt::registry& registry);

    void DestroyMarkedEntities(entt::registry& registry);

    // Centralized Game Over / Initials sequence
    void StartGameOverSequence(entt::registry& registry);

    // Nuke Powerup Functions
    void HandleNukeActivation(entt::registry& registry);
    void HandleNukeBlastWave(entt::registry& registry);
    // End Nuke Powerup Functions

    void HandleStarMovement(entt::registry& registry);

    void HandleRespawnDelay(entt::registry& registry)
    {
        GAME::RespawnDelay* respawnDelay = registry.ctx().find<GAME::RespawnDelay>();
        if (!respawnDelay) {
            return;
        }

        UTIL::DeltaTime* deltaTimeComponent = registry.ctx().find<UTIL::DeltaTime>();
        if (!deltaTimeComponent) {
            return;
        }

        respawnDelay->timeRemaining -= deltaTimeComponent->dtSec;

        if (respawnDelay->timeRemaining <= 0.0f) {
            std::cout << "Respawning player" << std::endl;
            GAME::SpawnPlayer(registry);
            registry.ctx().erase<GAME::RespawnDelay>();
        }
    }

    void Update_GameManager(entt::registry& registry, entt::entity entity)
    {
        if (registry.ctx().find<GAME::GameOver>()) {
            return;
        }

        // Destroy marked entities before any updates
        DestroyMarkedEntities(registry);
        HandleRespawnDelay(registry);
        //PatchPlayer(registry);
        //PatchWaveLogic(registry);
        CheckCollisions(registry);
        HandleFlashRed(registry);
        DestroyMarkedEntities(registry);

        // Check for and activate nuke
        HandleNukeActivation(registry);
        // Update nuke destruction logic
        HandleNukeBlastWave(registry);

        HandleMovement(registry);
        PatchPlayer(registry);    //HandleMovement messes up ship rotations
        PatchWaveLogic(registry); //HandleMovement messes up ship rotations
        HandleStarMovement(registry);

        GAME::UpdateHUD(registry);
    }

    Score& GetGlobalScore(entt::registry& registry)
    {
        if (auto* s = registry.ctx().find<Score>()) {
            return *s;
        }
        // Create it if it does not exist yet
        return registry.ctx().emplace<Score>();
    }

    void ResetGame(entt::registry& registry)
    {
        std::cout << "Restarting game\n";

        // Wait for Vulkan to finish all rendering before destroying entities
        auto displayView = registry.view<DRAW::VulkanRenderer>();
        if (!displayView.empty()) {
            auto displayEntity = displayView.front();
            auto& vulkanRenderer = registry.get<DRAW::VulkanRenderer>(displayEntity);
            if (vulkanRenderer.device != nullptr) {
                vkDeviceWaitIdle(vulkanRenderer.device);
            }
        }

        if (registry.ctx().find<GAME::GameOver>())
            registry.ctx().erase<GAME::GameOver>();

        // Destroy mesh collections first, then parent entities
        std::vector<entt::entity> parentEntities;
        std::vector<entt::entity> meshEntities;

        // Collect all players
        auto playerView = registry.view<GAME::Player>();
        for (auto entity : playerView) {
            parentEntities.push_back(entity);

            // Collect mesh entities if they exist
            if (registry.any_of<DRAW::MeshCollection>(entity)) {
                DRAW::MeshCollection& meshCollection = registry.get<DRAW::MeshCollection>(entity);
                for (entt::entity meshEntity : meshCollection.entities) {
                    meshEntities.push_back(meshEntity);
                }
            }
        }

        // Collect all enemies
        auto enemyView = registry.view<GAME::Enemy>();
        for (auto entity : enemyView) {
            parentEntities.push_back(entity);

            // Collect mesh entities if they exist
            if (registry.any_of<DRAW::MeshCollection>(entity)) {
                DRAW::MeshCollection& meshCollection = registry.get<DRAW::MeshCollection>(entity);
                for (entt::entity meshEntity : meshCollection.entities) {
                    meshEntities.push_back(meshEntity);
                }
            }
        }

        // Collect all bullets
        auto bulletView = registry.view<GAME::Bullet>();
        for (auto entity : bulletView) {
            parentEntities.push_back(entity);

            // Collect mesh entities if they exist
            if (registry.any_of<DRAW::MeshCollection>(entity)) {
                DRAW::MeshCollection& meshCollection = registry.get<DRAW::MeshCollection>(entity);
                for (entt::entity meshEntity : meshCollection.entities) {
                    meshEntities.push_back(meshEntity);
                }
            }
        }

        // Collect game over screen entities
        auto gameOverView = registry.view<GAME::GameOverScreen>();
        for (auto entity : gameOverView) {
            parentEntities.push_back(entity);
        }

        // First destroy all mesh entities
        for (auto meshEntity : meshEntities) {
            if (registry.valid(meshEntity)) {
                registry.destroy(meshEntity);
            }
        }

        // Then destroy all parent entities
        for (auto entity : parentEntities) {
            if (registry.valid(entity)) {
                registry.destroy(entity);
            }
        }

        // Reset player count
        auto* playerCount = registry.ctx().find<GAME::PlayerCount>();
        if (playerCount) {
            *playerCount = GAME::PlayerCount{ 0, 0 };
        }

        // Clear respawn delay if it exists
        if (registry.ctx().find<GAME::RespawnDelay>()) {
            registry.ctx().erase<GAME::RespawnDelay>();
        }

        // Clear saved lives if they exist
        if (registry.ctx().find<GAME::Lives>()) {
            registry.ctx().erase<GAME::Lives>();
        }

        Score& globalScore = GetGlobalScore(registry);
        globalScore.score = 0;

        // Respawn the player and enemies
        GAME::SpawnPlayer(registry);
        /*GAME::SpawnEnemy(registry);
        GAME::SpawnExplosiveEnemy(registry, std::nullopt);*/

        GAME::WaveStageFunctions::playStage(registry, 1);
    }

    // Centralized function to start the Game Over / Initials flow
    void StartGameOverSequence(entt::registry& registry)
    {
        // Avoid running this twice
        if (registry.ctx().find<GAME::GameOver>()) {
            return;
        }

        // Set the GameOver flag
        registry.ctx().emplace<GAME::GameOver>();
        std::cout << "You lose, game over! Enter initials..." << std::endl;

        // Play GameOver audio (if available)
        try {
            auto& audio = registry.ctx().get<GameAudio>();
            audio.Play("GameOver");
        }
        catch (...) {
            std::cerr << "[GameOver] Could not play GameOver sound.\n";
        }

        // Destroy any existing InitialsEntryScreen entities, just in case
        auto initialsView = registry.view<GAME::InitialsEntryScreen>();
        for (auto e : initialsView) {
            if (registry.valid(e)) {
                registry.destroy(e);
            }
        }

        // Create a fresh InitialsEntryScreen entity (tag only)
        entt::entity initialsEntity = registry.create();
        registry.emplace<GAME::InitialsEntryScreen>(initialsEntity);

        // Ensure PlayerNameInput exists in CONTEXT (not as a component)
        if (auto* input = registry.ctx().find<GAME::PlayerNameInput>()) {
            *input = GAME::PlayerNameInput{}; // reset to default (AAA, index 0, etc.)
        }
        else {
            registry.ctx().emplace<GAME::PlayerNameInput>(); // default-constructed
        }

        std::cout << "InitialsEntryScreen entity spawned id=" << (int)initialsEntity << std::endl;
    }

    void HandleMovement(entt::registry& registry)
    {
        auto view = registry.view<GAME::Transform, DRAW::MeshCollection>();
        auto gpuInstanceView = registry.view<DRAW::GPUInstance>();
        auto movementView = registry.view<GAME::EntityMovement>();
        auto toDestroyView = registry.view<GAME::ToDestroy>();

        for (const entt::entity& viewEntity : view) {
            // Skip entities marked for destruction
            if (toDestroyView.contains(viewEntity)) {
                continue;
            }
            GAME::Transform& transform = view.get<GAME::Transform>(viewEntity);
            DRAW::MeshCollection& meshCollection = view.get<DRAW::MeshCollection>(viewEntity);

            if (movementView.contains(viewEntity)) {
                GW::MATH::GMATRIXF transformMatrix = transform.transformMatrix;
                GAME::EntityMovement& entityMovement = movementView.get<GAME::EntityMovement>(viewEntity);
                GW::MATH::GVECTORF deltaPosition;

                UTIL::DeltaTime* deltaTimeComponent = registry.ctx().find<UTIL::DeltaTime>();
                if (deltaTimeComponent) {
                    GW::MATH::GVector::ScaleF(entityMovement.velocity, deltaTimeComponent->dtSec, deltaPosition);
                }

                GW::MATH::GMatrix::TranslateGlobalF(transformMatrix, deltaPosition, transform.transformMatrix);
            }

            for (const entt::entity& meshEntity : meshCollection.entities) {
                // Skip invalid or marked mesh entities
                if (!registry.valid(meshEntity) || toDestroyView.contains(meshEntity)) {
                    continue;
                }
                if (!gpuInstanceView.contains(meshEntity)) {
                    continue;
                }

                DRAW::GPUInstance& gpuInstance = registry.get<DRAW::GPUInstance>(meshEntity);
                gpuInstance.transform = transform.transformMatrix;
            }
        }
    }

    void CheckCollisions(entt::registry& registry) {
        auto collidableView = registry.view<GAME::Collidable>();
        auto toDestroyView = registry.view<GAME::ToDestroy>();

        for (auto itA = collidableView.begin(); itA != collidableView.end(); ++itA) {
            const entt::entity& entityA = *itA;
            // Skip if already marked for destruction
            if (toDestroyView.contains(entityA)) {
                continue;
            }
            GW::MATH::GOBBF colliderA = GAME::GetCollider(registry, entityA);

            for (auto itB = std::next(itA); itB != collidableView.end(); ++itB) {
                const entt::entity& entityB = *itB;
                // Skip if already marked for destruction
                if (toDestroyView.contains(entityB)) {
                    continue;
                }
                GW::MATH::GOBBF colliderB = GAME::GetCollider(registry, entityB);

                GW::MATH::GCollision::GCollisionCheck collisionCheck;
                GW::MATH::GCollision::TestOBBToOBBF(colliderA, colliderB, collisionCheck);

                if (collisionCheck == GW::MATH::GCollision::GCollisionCheck::COLLISION) {
                    HandleCollision(registry, entityA, entityB, colliderA, colliderB);
                }
            }
        }
    }

    void HandleCollision(
        entt::registry& registry,
        const entt::entity& entityA,
        const entt::entity& entityB,
        GW::MATH::GOBBF& colliderA,
        GW::MATH::GOBBF& colliderB
    ) {
        bool aIsObstacle = registry.any_of<GAME::Obstacle>(entityA);
        bool bIsObstacle = registry.any_of<GAME::Obstacle>(entityB);

        bool aIsProjectile = registry.any_of<GAME::Bullet>(entityA);
        bool bIsProjectile = registry.any_of<GAME::Bullet>(entityB);

        bool projectileHitObstacle = (aIsProjectile && bIsObstacle) || (bIsProjectile && aIsObstacle);
        if (projectileHitObstacle) {
            MarkForDestroy(registry, aIsProjectile ? entityA : entityB);
            return;
        }

        bool aIsEnemy = registry.any_of<GAME::Enemy>(entityA);
        bool bIsEnemy = registry.any_of<GAME::Enemy>(entityB);

        //enemy hitting obstacle now destroys enemy
        bool enemyHitObstacle = (aIsEnemy && bIsObstacle) || (bIsEnemy && aIsObstacle);
        if (enemyHitObstacle) {
            entt::entity enemyEntity = aIsEnemy ? entityA : entityB;
            std::cout << "Destroyed enemy!" << std::endl;
            MarkForDestroy(registry, enemyEntity);
            return;
        }

        bool projectileHitEnemy = (aIsProjectile && bIsEnemy) || (aIsEnemy && bIsProjectile);
        if (projectileHitEnemy) {
            entt::entity enemyEntity = aIsEnemy ? entityA : entityB;
            entt::entity projectileEntity = aIsEnemy ? entityB : entityA;
            GW::MATH::GOBBF& enemyCollider = aIsEnemy ? colliderA : colliderB;
            GW::MATH::GOBBF& projectileCollider = aIsEnemy ? colliderB : colliderA;

            MarkForDestroy(registry, projectileEntity);

            DRAW::MeshCollection* enemyMeshCollection = registry.try_get<DRAW::MeshCollection>(enemyEntity);
            if (enemyMeshCollection) {
                for (entt::entity& meshEntity : enemyMeshCollection->entities) {
                    DRAW::GPUInstance* gpuInstance = registry.try_get<DRAW::GPUInstance>(meshEntity);
                    if (!gpuInstance) {
                        continue;
                    }

                    GAME::FlashRed* flashRed = registry.try_get<GAME::FlashRed>(meshEntity);
                    if (flashRed) {
                        continue;
                    }

                    flashRed = &registry.emplace<GAME::FlashRed>(meshEntity);
                    flashRed->originalColor = gpuInstance->matData.Kd;
                    flashRed->timeLeft = 0.05;
                    gpuInstance->matData.Kd = { 1.0f, 0.0f, 0.0f };
                }
            }

            GAME::Health* enemyHealth = registry.try_get<GAME::Health>(enemyEntity);
            if (!enemyHealth) {
                return;
            }

            enemyHealth->hitPoints -= 1;
            // --- Play Enemy Hit Audio ---
            auto& audio = registry.ctx().get<GameAudio>();
            audio.Play("EnemyHit");

            if (enemyHealth->hitPoints <= 0) {
                HandlePowerUpSpawns(registry, enemyEntity);

                Score* enemyScore = registry.try_get<Score>(enemyEntity);
                Bullet* bullet = registry.try_get<Bullet>(projectileEntity);
                if (enemyScore && bullet) {
                    std::cout << "Enemy has score, and bullet component exists" << std::endl;
                    entt::entity playerEntity = bullet->ownerEntity;
                    if (playerEntity != entt::null) {
                        std::cout << "Player Entity from bullet" << std::endl;
                        Score* playerScore = registry.try_get<Score>(playerEntity);
                        if (playerScore) {
                            std::cout << "Player has score" << std::endl;
                            // Optional per-life score
                            playerScore->score += enemyScore->score;
                        }
                    }
                }

                // Global run score (used for HUD + high scores)
                if (enemyScore) {
                    Score& globalScore = GetGlobalScore(registry);
                    globalScore.score += enemyScore->score;
                    std::cout << "+ " << enemyScore->score
                        << "\t Total Score: " << globalScore.score << std::endl;
                }

                if (registry.try_get<GAME::Explosive>(enemyEntity))
                {
                    HandleExplosion(registry, enemyEntity);

                    // --- Play Enemy Hit Audio ---
                    auto& audio = registry.ctx().get<GameAudio>();
                    audio.Play("EnemyExplosion");

                    return;
                }

                MarkForDestroy(registry, enemyEntity);
            }

            return;
        }

        bool aIsPlayer = registry.any_of<GAME::Player>(entityA);
        bool bIsPlayer = registry.any_of<GAME::Player>(entityB);
        bool enemyHitPlayer = (aIsPlayer && bIsEnemy) || (bIsPlayer && aIsEnemy);

        if (enemyHitPlayer) {
            entt::entity playerEntity = aIsPlayer ? entityA : entityB;
            if (registry.try_get<GAME::Invulnerable>(playerEntity)) {
                return;
            }

            GAME::Health* health = registry.try_get<GAME::Health>(playerEntity);
            if (health) {
                GAME::MakePlayerInvulnerable(registry, playerEntity);
                health->hitPoints--;

                //make player flash red
                DRAW::MeshCollection* playerMeshCollection = registry.try_get<DRAW::MeshCollection>(playerEntity);
                if (playerMeshCollection) {
                    for (entt::entity& meshEntity : playerMeshCollection->entities) {
                        DRAW::GPUInstance* gpuInstance = registry.try_get<DRAW::GPUInstance>(meshEntity);
                        if (!gpuInstance) {
                            continue;
                        }

                        GAME::FlashRed* flashRed = registry.try_get<GAME::FlashRed>(meshEntity);
                        if (flashRed) {
                            continue;
                        }

                        flashRed = &registry.emplace<GAME::FlashRed>(meshEntity);
                        flashRed->originalColor = gpuInstance->matData.Kd;
                        flashRed->timeLeft = 0.05;
                        gpuInstance->matData.Kd = { 1.0f, 0.0f, 0.0f };
                    }
                }

                std::cout << "Player was hit! " << health->hitPoints << " HP remaining!" << std::endl;

                // --- Play PlayerHit Audio ---
                auto& audio = registry.ctx().get<GameAudio>();
                audio.Play("PlayerHit");

                if (health->hitPoints <= 0) {
                    // Player HP reached 0, check lives
                    GAME::Lives* lives = registry.try_get<GAME::Lives>(playerEntity);

                    // Check if player has extra lives (remaining > 0)
                    if (lives && lives->remaining > 0) {
                        // Player has lives remaining, handle respawn
                        lives->remaining--;
                        std::cout << "Player died! Lives after death: " << lives->remaining << std::endl;

                        // Save lives to context before destroying player
                        if (registry.ctx().find<GAME::Lives>()) {
                            registry.ctx().erase<GAME::Lives>();
                        }
                        registry.ctx().emplace<GAME::Lives>(*lives);

                        // Mark player for destruction
                        MarkForDestroy(registry, playerEntity);

                        // Decrement alive count but don't trigger game over
                        PlayerCount* playerCount = registry.ctx().find<GAME::PlayerCount>();
                        if (playerCount) {
                            playerCount->alive--;
                        }

                        // Create respawn delay
                        if (registry.ctx().find<GAME::RespawnDelay>()) {
                            registry.ctx().erase<GAME::RespawnDelay>();
                        }
                        GAME::RespawnDelay& respawnDelay = registry.ctx().emplace<GAME::RespawnDelay>();
                        respawnDelay.timeRemaining = respawnDelay.totalDelay;
                        std::cout << "Respawning in " << respawnDelay.totalDelay << " seconds..." << std::endl;
                    }
                    else {
                        // No lives remaining = game over
                        std::cout << "No lives remaining. Game Over!" << std::endl;

                        PlayerCount* playerCount = registry.ctx().find<GAME::PlayerCount>();
                        if (playerCount) {
                            playerCount->alive--;
                        }

                        if (!playerCount || playerCount->alive <= 0) {
                            StartGameOverSequence(registry);
                        }
                    }
                }
            }
        }

        bool aIsPowerUp = registry.any_of<GAME::PowerUp>(entityA);
        bool bIsPowerUp = registry.any_of<GAME::PowerUp>(entityB);
        bool playerHitPowerup = (aIsPlayer && bIsPowerUp) || (bIsPlayer && aIsPowerUp);

        if (playerHitPowerup) {
            entt::entity playerEntity = aIsPlayer ? entityA : entityB;
            entt::entity powerUpEntity = aIsPlayer ? entityB : entityA;

            GAME::PowerUp powerUp = registry.get<GAME::PowerUp>(powerUpEntity);
            MarkForDestroy(registry, powerUpEntity);

            // --- Play PowerUp Audio ---
            auto& audio = registry.ctx().get<GameAudio>();
            audio.Play("PowerUp");

            Score* powerUpScore = registry.try_get<Score>(powerUpEntity);
            if (powerUpScore) {
                // Optional per-player score
                Score* playerScore = registry.try_get<Score>(playerEntity);
                if (playerScore) {
                    playerScore->score += powerUpScore->score;
                }

                // Global run score
                Score& globalScore = GetGlobalScore(registry);
                globalScore.score += powerUpScore->score;
                std::cout << "+ " << powerUpScore->score
                    << "\t Total Score: " << globalScore.score << std::endl;
            }

            switch (powerUp.powerUpType) {
            case GAME::PowerUpType::EXTRA_HEALTH: {
                GAME::Health& playerHealth = registry.get<GAME::Health>(playerEntity);
                playerHealth.hitPoints++;
                std::cout << "You gained 1 HP!" << std::endl;
                break;
            }
            case GAME::PowerUpType::DOUBLE_FIRE_RATE: {
                GAME::ActivePowerUps& activePowerUps = registry.get_or_emplace<GAME::ActivePowerUps>(playerEntity);
                activePowerUps.doubleFireRate = powerUp.duration;
                break;
            }
            case GAME::PowerUpType::DOUBLE_GUN: {
                GAME::ActivePowerUps& activePowerUps = registry.get_or_emplace<GAME::ActivePowerUps>(playerEntity);
                activePowerUps.doubleGun = powerUp.duration;
                break;
            }
            case GAME::PowerUpType::NUKE: {
                std::cout << "NUKE Powerup collected! Activating..." << std::endl;
                // Emplace the tag to have the nuke system handle it
                if (!registry.ctx().find<GAME::ActivateNuke>()) {
                    GAME::ActivateNuke activateNuke = { playerEntity };
                    registry.ctx().emplace<GAME::ActivateNuke>(activateNuke);
                }
                break;
            }
            }

            return;
        }

        bool powerUpHitObstacle = (aIsPowerUp && bIsObstacle) || (bIsPowerUp && aIsObstacle);
        if (powerUpHitObstacle) {
            entt::entity powerUpEntity = aIsPowerUp ? entityA : entityB;
            MarkForDestroy(registry, powerUpEntity);
            return;
        }

        bool aIsEnemyBullet = registry.any_of<GAME::EnemyBullet>(entityA);
        bool bIsEnemyBullet = registry.any_of<GAME::EnemyBullet>(entityB);
        bool enemyBulletHitPlayer = (aIsPlayer && bIsEnemyBullet) || (bIsPlayer && aIsEnemyBullet);

        if (enemyBulletHitPlayer) {

            //mark enemy bullet for destruction
            MarkForDestroy(registry, aIsEnemyBullet ? entityA : entityB);

            entt::entity playerEntity = aIsPlayer ? entityA : entityB;
            if (registry.try_get<GAME::Invulnerable>(playerEntity)) {
                return;
            }

            GAME::Health* health = registry.try_get<GAME::Health>(playerEntity);
            if (health) {
                GAME::MakePlayerInvulnerable(registry, playerEntity);
                health->hitPoints--;

                //make player flash red
                DRAW::MeshCollection* playerMeshCollection = registry.try_get<DRAW::MeshCollection>(playerEntity);
                if (playerMeshCollection) {
                    for (entt::entity& meshEntity : playerMeshCollection->entities) {
                        DRAW::GPUInstance* gpuInstance = registry.try_get<DRAW::GPUInstance>(meshEntity);
                        if (!gpuInstance) {
                            continue;
                        }

                        GAME::FlashRed* flashRed = registry.try_get<GAME::FlashRed>(meshEntity);
                        if (flashRed) {
                            continue;
                        }

                        flashRed = &registry.emplace<GAME::FlashRed>(meshEntity);
                        flashRed->originalColor = gpuInstance->matData.Kd;
                        flashRed->timeLeft = 0.05;
                        gpuInstance->matData.Kd = { 1.0f, 0.0f, 0.0f };
                    }
                }

                std::cout << "Player was hit! " << health->hitPoints << " HP remaining!" << std::endl;

                // --- Play PlayerHit Audio ---
                auto& audio = registry.ctx().get<GameAudio>();
                audio.Play("PlayerHit");

                if (health->hitPoints <= 0) {
                    // Player HP reached 0, check lives
                    GAME::Lives* lives = registry.try_get<GAME::Lives>(playerEntity);

                    // Check if player has extra lives (remaining > 0)
                    if (lives && lives->remaining > 0) {
                        // Player has lives remaining, handle respawn
                        lives->remaining--;
                        std::cout << "Player died! Lives after death: " << lives->remaining << std::endl;

                        // Save lives to context before destroying player
                        if (registry.ctx().find<GAME::Lives>()) {
                            registry.ctx().erase<GAME::Lives>();
                        }
                        registry.ctx().emplace<GAME::Lives>(*lives);

                        // Mark player for destruction
                        MarkForDestroy(registry, playerEntity);

                        // Decrement alive count but don't trigger game over
                        PlayerCount* playerCount = registry.ctx().find<GAME::PlayerCount>();
                        if (playerCount) {
                            playerCount->alive--;
                        }

                        // Create respawn delay
                        if (registry.ctx().find<GAME::RespawnDelay>()) {
                            registry.ctx().erase<GAME::RespawnDelay>();
                        }
                        GAME::RespawnDelay& respawnDelay = registry.ctx().emplace<GAME::RespawnDelay>();
                        respawnDelay.timeRemaining = respawnDelay.totalDelay;
                        std::cout << "Respawning in " << respawnDelay.totalDelay << " seconds..." << std::endl;
                    }
                    else {
                        // No lives remaining = game over
                        std::cout << "No lives remaining. Game Over!" << std::endl;

                        PlayerCount* playerCount = registry.ctx().find<GAME::PlayerCount>();
                        if (playerCount) {
                            playerCount->alive--;
                        }

                        if (!playerCount || playerCount->alive <= 0) {
                            StartGameOverSequence(registry);
                        }
                    }
                }
            }
        }

        bool enemyBulletHitObstacle = (aIsEnemyBullet && bIsObstacle) || (bIsEnemyBullet && aIsObstacle);
        if (enemyBulletHitObstacle) {
            MarkForDestroy(registry, aIsEnemyBullet ? entityA : entityB);
            return;
        }
    }

    void HandlePowerUpSpawns(entt::registry& registry, const entt::entity& enemyEntity)
    {
        GAME::SpawnsPowerUp* spawnsPowerUp = registry.try_get<GAME::SpawnsPowerUp>(enemyEntity);
        if (!spawnsPowerUp) {
            return;
        }

        UTIL::Random* random = registry.ctx().find<UTIL::Random>();
        if (!random) {
            return;
        }

        int randomValue = random->next();
        if (randomValue > spawnsPowerUp->spawnChance) {
            return;
        }

        Transform& enemyTransform = registry.get<GAME::Transform>(enemyEntity);
        GAME::SpawnPowerup(registry, spawnsPowerUp->powerUpType, enemyTransform);
    }

    void HandleExplosion(entt::registry& registry, const entt::entity& enemyEntity)
    {
        ///once fully implemented, entt should destroy itself without this here
        MarkForDestroy(registry, enemyEntity);

        ///to be implemented
        float nukeChance;
        int damageAmount;
        int explosionRadius;

        GAME::Explosive* explosiveComponent = registry.try_get<GAME::Explosive>(enemyEntity);
        if (explosiveComponent)
        {
            nukeChance = explosiveComponent->nukeChance;
            damageAmount = explosiveComponent->damage;
            explosionRadius = explosiveComponent->radius;
        }
        else
        {
            return;
        }

        //NUKE CHANCE
        UTIL::Random* random = registry.ctx().find<UTIL::Random>();
        if (!random) {

        }
        else {
            int randomValue = random->next(); // Gets an int from 1 to 100

            // nukeChance is read from the .ini, e.g., "15" for 15%
            // We cast it to an int for the comparison.
            if (randomValue <= (int)nukeChance)
            {
                // Activate the nuke
                if (!registry.ctx().find<GAME::ActivateNuke>()) {
                    registry.ctx().emplace<GAME::ActivateNuke>();
                }
                return; // The nuke will kill everything, so don't also do area damage
            }
        }
    }

    void HandleNukeActivation(entt::registry& registry)
    {
        GAME::ActivateNuke* activateNuke = registry.ctx().find<GAME::ActivateNuke>();
        if (!activateNuke) {
            return;
        }

        // Check if the ActivateNuke tag exists in the context
        // Only start a new NukeBlastWave if one isn't already running
        if (registry.ctx().find<GAME::NukeBlastWave>()) {
            return;
        }

        // Play Nuke Audio
        try {
            auto& audio = registry.ctx().get<GameAudio>();
            audio.Play("NukeExplosion"); // Placeholder. Replace with your "nuke_sound"
            std::cout << "BOOM! Nuke activated." << std::endl;
        }
        catch (...) {
            std::cerr << "Error playing nuke sound." << std::endl;
        }

        // Add the NukeBlastWave component to the context to start the logic
        GAME::NukeBlastWave nukeBlastWave;
        nukeBlastWave.playerEntity = activateNuke->playerEntity;
        registry.ctx().emplace<GAME::NukeBlastWave>(nukeBlastWave);

        // Remove the tag so this only runs once per activation
        registry.ctx().erase<GAME::ActivateNuke>();
    }

    void HandleNukeBlastWave(entt::registry& registry)
    {
        // Check if the nuke destruction logic is active
        GAME::NukeBlastWave* nukeLogic = registry.ctx().find<GAME::NukeBlastWave>();
        if (!nukeLogic) {
            return; // No nuke active, do nothing
        }

        UTIL::DeltaTime* dt = registry.ctx().find<UTIL::DeltaTime>();
        if (!dt) return; // Can't update without delta time

        nukeLogic->timeActive += dt->dtSec;

        // Handle enemy destruction
        if (nukeLogic->timeActive <= nukeLogic->totalTime) { // totalTime is 1.5s
            float expansionPercent = nukeLogic->timeActive / nukeLogic->totalTime;
            float currentRadius = expansionPercent * nukeLogic->maxRadius;

            // Loop through all enemies
            auto enemyView = registry.view<GAME::Enemy, GAME::Transform>();
            for (auto entity : enemyView) {
                // Skip enemies already marked for death
                if (registry.any_of<GAME::ToDestroy>(entity)) {
                    continue;
                }

                // Get enemy position
                auto& transform = enemyView.get<GAME::Transform>(entity);
                float x = transform.transformMatrix.row4.x;
                float z = transform.transformMatrix.row4.z;

                // Calculate distance from center (0,0)
                float distance = std::sqrt(x * x + z * z);

                if (distance <= currentRadius) {
                    entt::entity playerEntity = nukeLogic->playerEntity;
                    Score* enemyScore = registry.try_get<Score>(entity);

                    if (enemyScore && playerEntity != entt::null) {
                        // Optional per-player score
                        if (Score* playerScore = registry.try_get<Score>(playerEntity)) {
                            playerScore->score += enemyScore->score;
                        }

                        // Global run score
                        Score& globalScore = GetGlobalScore(registry);
                        globalScore.score += enemyScore->score;
                        std::cout << "+ " << enemyScore->score
                            << "!\tTotal: " << globalScore.score << std::endl;
                    }

                    MarkForDestroy(registry, entity);
                }
            }
        }

        // Clean up the logic component when it's done
        if (nukeLogic->timeActive >= nukeLogic->totalTime) {
            registry.ctx().erase<GAME::NukeBlastWave>();
        }
    }

    void UpdateHUD(entt::registry& registry)
    {
        auto view = registry.view<Player, Health, Score>();
        if (view.begin() == view.end())
            return;

        entt::entity player = *view.begin();
        auto& health = view.get<Health>(player);

        // Prefer global run score
        long scoreValue = 0;
        if (auto* globalScore = registry.ctx().find<Score>()) {
            scoreValue = globalScore->score;
        }
        else {
            // Fallback: per-player score
            auto& scoreComp = view.get<Score>(player);
            scoreValue = scoreComp.score;
        }

        int lives = 0;
        if (auto* livesCtx = registry.ctx().find<Lives>()) {
            lives = livesCtx->remaining;
        }
        else if (auto* livesComp = registry.try_get<Lives>(player)) {
            lives = livesComp->remaining;
        }

        char buf[128];
        sprintf_s(buf, "HP: %d   Lives: %d   Score: %ld",
            health.hitPoints, lives, scoreValue);

        auto* hud = registry.ctx().find<HUDData>();
        if (hud) {
            hud->text = buf;
        }
        else {
            HUDData data;
            data.text = buf;
            data.x = 20.0f;  // pixels from left
            data.y = 40.0f;  // pixels from top
            registry.ctx().emplace<HUDData>(data);
        }
    }

    void HandleStarMovement(entt::registry& registry)
    {
        ///stars move down constantly (done in HandleMovement)
        ///if star goes below certain z depth, set its z pos to top of screen

        std::shared_ptr<const GameConfig> config = registry.ctx().get<UTIL::Config>().gameConfig;
        int maxDepth = (*config).at("SpaceBackground").at("maxDepth").as<int>();
        int resetHeight = (*config).at("SpaceBackground").at("resetHeight").as<int>();

        auto starView = registry.view<GAME::Star, GAME::Transform>();
        for (auto& star : starView)
        {
            GAME::Transform& starTransform = *registry.try_get<GAME::Transform>(star);

            if (starTransform.transformMatrix.row4.z <= maxDepth)
            {
                starTransform.transformMatrix.row4.z = resetHeight;
            }
        }
    }

    void HandleFlashRed(entt::registry& registry)
    {
        UTIL::DeltaTime* deltaTimeComponent = registry.ctx().find<UTIL::DeltaTime>();
        if (!deltaTimeComponent) {
            return;
        }

        auto view = registry.view<GAME::FlashRed>();

        for (auto it = view.begin(); it != view.end(); ++it) {
            entt::entity entity = *it;

            GAME::FlashRed* flashRed = registry.try_get<GAME::FlashRed>(entity);
            if (!flashRed) {
                return;
            }

            DRAW::GPUInstance* gpuInstance = registry.try_get<DRAW::GPUInstance>(entity);
            if (!gpuInstance) {
                return;
            }

            flashRed->timeLeft -= deltaTimeComponent->dtSec;
            if (flashRed->timeLeft <= 0) {
                gpuInstance->matData.Kd = flashRed->originalColor;
                registry.remove<GAME::FlashRed>(entity);
            }
        }
    }

    void PatchPlayer(entt::registry& registry)
    {
        auto playerView = registry.view<GAME::Player>();
        auto toDestroyView = registry.view<GAME::ToDestroy>();

        for (const entt::entity& playerEntity : playerView) {
            // Skip players marked for destruction
            if (toDestroyView.contains(playerEntity)) {
                continue;
            }
            registry.patch<Player>(playerEntity);
        }
    }

    void PatchWaveLogic(entt::registry& registry)
    {
        auto waveView = registry.view<GAME::WaveLogic>();
        for (const entt::entity& waveEntity : waveView) {
            registry.patch<WaveLogic>(waveEntity);
        }
    }

    void MarkForDestroy(entt::registry& registry, const entt::entity& entity)
    {
        if (!registry.any_of<GAME::ToDestroy>(entity)) {
            registry.emplace<GAME::ToDestroy>(entity);
        }
    }

    void DestroyMarkedEntities(entt::registry& registry)
    {
        // Wait for GPU to finish before destroying anything
        auto displayView = registry.view<DRAW::VulkanRenderer>();
        if (!displayView.empty()) {
            auto displayEntity = displayView.front();
            if (registry.all_of<DRAW::VulkanRenderer>(displayEntity)) {
                auto& vulkanRenderer = registry.get<DRAW::VulkanRenderer>(displayEntity);
                if (vulkanRenderer.device != nullptr) {
                    vkDeviceWaitIdle(vulkanRenderer.device);
                }
            }
        }
        auto toDestroyView = registry.view<GAME::ToDestroy>();
        std::vector<entt::entity> entitiesToDestroy(toDestroyView.begin(), toDestroyView.end());

        for (auto entity : entitiesToDestroy) {
            if (!registry.valid(entity)) {
                continue;
            }
            if (registry.any_of<DRAW::MeshCollection>(entity)) {
                DRAW::MeshCollection& meshCollection = registry.get<DRAW::MeshCollection>(entity);
                for (entt::entity meshEntity : meshCollection.entities) {
                    if (registry.valid(meshEntity)) {
                        registry.destroy(meshEntity);
                    }
                }
            }
            registry.destroy(entity);
        }
    }

    void GAME::CleanupGameplayEntities(entt::registry& registry)
    {
        std::vector<entt::entity> parentEntities;
        std::vector<entt::entity> meshEntities;

        // Collect all gameplay entities
        auto playerView = registry.view<GAME::Player>();
        auto enemyView = registry.view<GAME::Enemy>();
        auto bulletView = registry.view<GAME::Bullet>();

        for (auto e : playerView) parentEntities.push_back(e);
        for (auto e : enemyView) parentEntities.push_back(e);
        for (auto e : bulletView) parentEntities.push_back(e);

        // Gather associated meshes
        for (auto e : parentEntities)
        {
            if (registry.any_of<DRAW::MeshCollection>(e))
            {
                auto& coll = registry.get<DRAW::MeshCollection>(e);
                for (auto mesh : coll.entities)
                    meshEntities.push_back(mesh);
            }
        }

        // Destroy meshes first
        for (auto mesh : meshEntities)
            if (registry.valid(mesh))
                registry.destroy(mesh);

        // Then destroy the parent entities
        for (auto e : parentEntities)
            if (registry.valid(e))
                registry.destroy(e);

        // Reset player count
        if (auto* pc = registry.ctx().find<GAME::PlayerCount>())
            *pc = GAME::PlayerCount{ 0, 0 };
    }

    CONNECT_COMPONENT_LOGIC()
    {
        registry.on_update<GAME::GameManager>().connect<Update_GameManager>();
    }

}
