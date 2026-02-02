#ifndef UTILITIES_H_
#define UTILITIES_H_

#include "GameConfig.h"
#include <random>

namespace UTIL
{
	struct Config
	{
		std::shared_ptr<GameConfig> gameConfig = std::make_shared<GameConfig>();
	};

	struct DeltaTime
	{
		double dtSec;
	};

	struct Input
	{
		GW::INPUT::GController gamePads; // controller support
		GW::INPUT::GInput immediateInput; // twitch keybaord/mouse
		GW::INPUT::GBufferedInput bufferedInput; // event keyboard/mouse
	};

	struct Random {
		std::uniform_int_distribution<> distribution;
		std::mt19937 generator;

		Random() : generator(std::random_device{}()), distribution(1, 100) {}
		Random(int min, int max) : generator(std::random_device{}()), distribution(min, max) {}

		int next() {
			return distribution(generator);
		}
	};

	/// Method declarations

	/// Creates a normalized vector pointing in a random direction on the X/Z plane
	GW::MATH::GVECTORF GetRandomVelocityVector();

} // namespace UTIL
#endif // !UTILITIES_H_