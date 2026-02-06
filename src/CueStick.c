#include <math.h>

#include "raylib/raylib.h"

#include "Types.h"
#include "CueStick.h"

void updateCueStick( CueStick *cs, float delta ) {

    if ( IsKeyDown( KEY_LEFT_CONTROL ) ) {
        if ( IsKeyPressed( KEY_LEFT ) ) {
            cs->angle -= 0.5f;
        } else if ( IsKeyPressed( KEY_RIGHT ) ) {
            cs->angle += 0.5f;
        } else if ( IsKeyPressed( KEY_UP ) ) {
            cs->impulse -= 50;
        } else if ( IsKeyPressed( KEY_DOWN ) ) {
            cs->impulse += 50;
        }
    } else {
        if ( IsKeyDown( KEY_LEFT ) ) {
            cs->angle -= 0.5f;
        } else if ( IsKeyDown( KEY_RIGHT ) ) {
            cs->angle += 0.5f;
        } else if ( IsKeyDown( KEY_UP ) ) {
            cs->impulse -= 50;
        } else if ( IsKeyDown( KEY_DOWN ) ) {
            cs->impulse += 50;
        }
    }

    if ( cs->impulse < 100 ) {
        cs->impulse = 100;
    } else if ( cs->impulse > 1400 ) {
        cs->impulse = 1400;
    }

}

void drawCueStick( CueStick *cs ) {

    float c = cosf( DEG2RAD * cs->angle );
    float s = sinf( DEG2RAD * cs->angle );

    float wSize = cs->size * c;
    float hSize = cs->size * s;

    float wDist = ( ( cs->impulse / 100.0f ) * 8 + 10 ) * c;
    float hDist = ( ( cs->impulse / 100.0f ) * 8 + 10 ) * s;

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
