#ifndef OPENMW_CONSTANTS_H
#define OPENMW_CONSTANTS_H

#include <string>

namespace Constants
{

// The game uses 64 units per yard
const float UnitsPerMeter = 69.99125109f;
const float UnitsPerFoot = 21.33333333f;

// Sound speed in meters per second
const float SoundSpeedInAir = 343.3f;
const float SoundSpeedUnderwater = 1484.0f;

// Gravity constant in m/sec^2
// Note: 8.96 m/sec^2 = 9.8 yards/sec^2
// Probaly original engine's developers just forgot
// that their engine uses yards instead of meters
// and used standart gravity value as it is
const float GravityConst = 8.96f;

// Size of one exterior cell in game units
const int CellSizeInUnits = 8192;

// Size of active cell grid in cells (it is a square with the (2 * CellGridRadius + 1) cells side)
const int CellGridRadius = 1;

// A label to mark night/day visual switches
const std::string NightDayLabel = "NightDaySwitch";

// A label to mark visual switches for herbalism feature
const std::string HerbalismLabel = "HerbalismSwitch";

// Percentage height at which projectiles are spawned from an actor
const float TorsoHeight = 0.75f;

}

#endif
