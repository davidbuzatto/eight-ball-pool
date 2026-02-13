/**
 * @file CueStick.c
 * @author Prof. Dr. David Buzatto
 * @brief Cue Stick implementation. Each cue stick represents a player.
 * 
 * @copyright Copyright (c) 2026
 */

#include <math.h>

#include "raylib/raylib.h"

#include "CueStick.h"
#include "ResourceManager.h"
#include "Types.h"

//[[maybe_unused]] static const Color HANDLE_COLOR = { 36, 9, 1, 255 };
static const __attribute__((unused)) Color HANDLE_COLOR = { 36, 9, 1, 255 };

static float hitAnimationTime = 0.1f;
static float hitAnimationCounter = 0.0f;

void updateCueStick( CueStick *cs, float delta ) {

    if ( cs->state == CUE_STICK_STATE_READY ) {

        cs->angle = RAD2DEG * atan2f( GetMouseY() - cs->target.y, GetMouseX() - cs->target.x );

        float mouseWheelMove = GetMouseWheelMove();
        
        if ( mouseWheelMove > 0.0f ) {
            cs->power += cs->powerTick;
        } else if ( mouseWheelMove < 0.0f ) {
            cs->power -= cs->powerTick;
        }

        if ( cs->power < cs->minPower ) {
            cs->power = cs->minPower;
        } else if ( cs->power > cs->maxPower ) {
            cs->power = cs->maxPower;
        }

    } else if ( cs->state == CUE_STICK_STATE_HITING ) {

        hitAnimationCounter += delta;

        if ( hitAnimationCounter > hitAnimationTime ) {
            hitAnimationCounter = 0.0f;
            cs->state = CUE_STICK_STATE_HIT;
        }

    }

}

void drawCueStick( CueStick *cs ) {

    float c = cosf( DEG2RAD * cs->angle );
    float s = sinf( DEG2RAD * cs->angle );
    float powerP = getCueStickPowerPercentage( cs );

    float wSize = cs->size * c;
    float hSize = cs->size * s;

    float wDist = ( powerP * 100 * ( 1.0f - hitAnimationCounter / hitAnimationTime ) + cs->distanceFromTarget ) * c;
    float hDist = ( powerP * 100 * ( 1.0f - hitAnimationCounter / hitAnimationTime ) + cs->distanceFromTarget ) * s;

    int h = (int) ( cs->size / rm.cueSticksTexture.width * 14 );

    DrawTexturePro( 
        rm.cueSticksTexture, 
        (Rectangle) { 0, 14 * cs->type, 510, 14 },
        (Rectangle) { cs->target.x - wDist - wSize, cs->target.y - hDist - hSize, cs->size, h },
        (Vector2) { 0, h / 2 },
        cs->angle,
        WHITE
    );

    /*float wTipSize = 5 * c;
    float hTipSize = 5 * s;

    float wHandSize = 80 * c;
    float hHandSize = 80 * s;

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
        cs->color
    );

    DrawLineEx( 
        (Vector2) {
            cs->target.x - wDist - wTipSize,
            cs->target.y - hDist - hTipSize
        },
        (Vector2) {
            cs->target.x - wDist,
            cs->target.y - hDist
        },
        5,
        WHITE
    );

    DrawLineEx( 
        (Vector2) {
            cs->target.x - wDist - wSize,
            cs->target.y - hDist - hSize
        },
        (Vector2) {
            cs->target.x - wDist - wSize + wHandSize,
            cs->target.y - hDist - hSize + hHandSize
        },
        5,
        HANDLE_COLOR
    );*/

    float wPath = 1200 * c;
    float hPath = 1200 * s;

    DrawLineV( 
        cs->target,
        (Vector2) {
            cs->target.x + wPath,
            cs->target.y + hPath
        },
        Fade( WHITE, 0.5f )
    );

}

float getCueStickPowerPercentage( CueStick *cs ) {
    return cs->power / (float) cs->maxPower;
}
