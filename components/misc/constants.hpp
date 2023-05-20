#ifndef OPENMW_CONSTANTS_H
#define OPENMW_CONSTANTS_H

#include <string>

namespace Constants
{

    // The game uses 64 units per yard
    constexpr float UnitsPerMeter = 69.99125109f;
    constexpr float UnitsPerFoot = 21.33333333f;

    // Sound speed in meters per second
    constexpr float SoundSpeedInAir = 343.3f;
    constexpr float SoundSpeedUnderwater = 1484.0f;

    // Gravity constant in m/sec^2
    // Note: 8.96 m/sec^2 = 9.8 yards/sec^2
    // Probably original engine's developers just forgot
    // that their engine uses yards instead of meters
    // and used standard gravity value as it is
    constexpr float GravityConst = 8.96f;

    // Size of one exterior cell in game units
    constexpr int CellSizeInUnits = 8192;
    constexpr int ESM4CellSizeInUnits = 4096;

    // Size of active cell grid in cells (it is a square with the (2 * CellGridRadius + 1) cells side)
    constexpr int CellGridRadius = 1;

    // ESM4 cells are twice smaller, so the active grid should have more cells.
    constexpr int ESM4CellGridRadius = CellGridRadius * 2;

    // A label to mark night/day visual switches
    const std::string NightDayLabel = "NightDaySwitch";

    // A label to mark visual switches for herbalism feature
    const std::string HerbalismLabel = "HerbalismSwitch";

    // Percentage height at which projectiles are spawned from an actor
    constexpr float TorsoHeight = 0.75f;

    static constexpr float sStepSizeUp = 34.0f;
    static constexpr float sMaxSlope = 46.0f;

    // Identifier for main scene camera
    const std::string SceneCamera = "SceneCam";

}

#endif
