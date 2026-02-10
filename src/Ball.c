#include <math.h>
#include "raylib/raylib.h"

#include "Types.h"
#include "Ball.h"

void updateBall( Ball *b, float delta ) {

    b->center.x += b->vel.x * delta;
    b->center.y += b->vel.y * delta;

    b->vel.x *= b->friction;
    b->vel.y *= b->friction;

    float speed = sqrtf( b->vel.x * b->vel.x + b->vel.y * b->vel.y );
    if ( speed < 0.5f ) {  // less than 0.5 pixels/second
        b->vel.x = 0;
        b->vel.y = 0;
        b->moving = false;
    } else {
        b->moving = true;
    }

}

void drawBall( Ball *b ) {

    if ( b->pocketed ) {
        return;
    }

    DrawCircleV( b->center, b->radius, b->color );

    if ( b->striped ) {
        DrawLineEx( 
            (Vector2) { b->center.x - b->radius, b->center.y}, 
            (Vector2) { b->center.x + b->radius, b->center.y}, 
            2, 
            WHITE
        );
    }
    
    DrawCircleLinesV( b->center, b->radius, BLACK );

    /*if ( b->moving ) {
        DrawCircleLinesV( b->center, b->radius, WHITE );
    } else {
        DrawCircleLinesV( b->center, b->radius, BLACK );
    }*/

    if ( b->number != 0 ) {
        const char *n = TextFormat( "%d", b->number );
        int w = MeasureText( n, 14 );
        DrawText( 
            n, 
            b->center.x - w / 2, 
            b->center.y - 6, 
            14, 
            b->number == 8 ? WHITE : BLACK
        );
    }

}
