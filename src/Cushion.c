/**
 * @file Cushion.c
 * @author Prof. Dr. David Buzatto
 * @brief Cushion implementation.
 * 
 * @copyright Copyright (c) 2026
 */

#include <math.h>

#include "raylib/raylib.h"
#include "raylib/raymath.h"

#include "Types.h"
#include "Cushion.h"

void drawCushion( Cushion *c ) {
    for ( int j = 0; j < 4; j++ ) {
        DrawLineV( c->vertices[j], c->vertices[(j+1)%4], BLACK );
    }
}
