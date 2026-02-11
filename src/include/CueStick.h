/**
 * @file CueStick.h
 * @author Prof. Dr. David Buzatto
 * @brief Cue Stick function declarations.
 * 
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "Types.h"

void updateCueStick( CueStick *cs, float delta );
void drawCueStick( CueStick *cs );
float getCueStickImpulsePercentage( CueStick *cs );