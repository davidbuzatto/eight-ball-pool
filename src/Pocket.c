/**
 * @file Pocket.c
 * @author Prof. Dr. David Buzatto
 * @brief Pocket implementation.
 * 
 * @copyright Copyright (c) 2026
 */

#include <math.h>

#include "raylib/raylib.h"
#include "raylib/raymath.h"

#include "Types.h"
#include "Pocket.h"

void drawPocket( Pocket *p ) {
    DrawCircleV( 
        p->center,
        p->radius,
        BLACK
    );
}
