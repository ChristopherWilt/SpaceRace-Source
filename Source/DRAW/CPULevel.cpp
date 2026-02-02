#include "DrawComponents.h"
#include "../CCL.h"

namespace DRAW {

	void Construct_CPULevel(entt::registry& registry, entt::entity entity)
	{
		GW::SYSTEM::GLog log;
		log.EnableConsoleLogging(true);

		DRAW::CPULevel* cpuLevel = registry.try_get<DRAW::CPULevel>(entity);
		if (!cpuLevel) {
			log.Log("Could not get the CPU Level!");
			return;
		}

		if (!cpuLevel->levelData.LoadLevel(cpuLevel->levelFile.c_str(), cpuLevel->modelPath.c_str(), log)) {
			log.Log(("Could not load level from " + cpuLevel->levelFile).c_str());
		}
	}

	CONNECT_COMPONENT_LOGIC()
	{
		registry.on_construct<CPULevel>().connect<Construct_CPULevel>();
	}

}