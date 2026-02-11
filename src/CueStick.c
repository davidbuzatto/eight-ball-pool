/**
 * @file CueStick.c
 * @author Prof. Dr. David Buzatto
 * @brief Cue Stick implementation.
 * 
 * @copyright Copyright (c) 2026
 */

#include <math.h>

#include "raylib/raylib.h"

#include "Types.h"
#include "CueStick.h"

void updateCueStick( CueStick *cs, float delta ) {

    cs->angle = RAD2DEG * atan2f( GetMouseY() - cs->target.y, GetMouseX() - cs->target.x );

    float mouseWheelMove = GetMouseWheelMove();
    
    if ( mouseWheelMove < 0.0f ) {
        cs->impulse += 10;
    } else if ( mouseWheelMove > 0.0f ) {
        cs->impulse -= 10;
    }

    if ( cs->impulse < cs->minImpulse ) {
        cs->impulse = cs->minImpulse;
    } else if ( cs->impulse > cs->maxImpulse ) {
        cs->impulse = cs->maxImpulse;
    }

}

void drawCueStick( CueStick *cs ) {

    float c = cosf( DEG2RAD * cs->angle );
    float s = sinf( DEG2RAD * cs->angle );

    float wSize = cs->size * c;
    float hSize = cs->size * s;

    float inpulsePerc = getCueStickImpulsePercentage( cs );
    float wDist = ( inpulsePerc * 100 + cs->distanceFromTarget ) * c;
    float hDist = ( inpulsePerc * 100 + cs->distanceFromTarget ) * s;

    float wPath = 1000 * c;
    float hPath = 1000 * s;

    DrawLineEx( 
        (Vector2) {
            cs->target.x - wDist - wSize,
            cs->target.y - hDist - hSize
        },
        (Vector2) {
            cs->target.x - wDist,
            cs->target.y - hDist
        },
        5,
        DARKBROWN
    );

    DrawLineV( 
        cs->target,
        (Vector2) {
            cs->target.x + wPath,
            cs->target.y + hPath
        },
        Fade( WHITE, 0.5f )
    );

}

float getCueStickImpulsePercentage( CueStick *cs ) {
    return cs->impulse / ( (float) cs->maxImpulse - (float) cs->minImpulse );
}
