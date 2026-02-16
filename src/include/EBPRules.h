/**
 * @file Rules.c
 * @author Prof. Dr. David Buzatto
 * @brief 8 Ball Pool (EBP) rules functions declarations.
 * 
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "Types.h"

void setupEBP( GameWorld *gw );
void applyRulesEBP( GameWorld *gw );
void resetCueBallPosition( GameWorld *gw );