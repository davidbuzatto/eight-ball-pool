#include "raylib/raylib.h"

#include "Types.h"
#include "Ball.h"

void updateBall( Ball *b, float delta ) {
    b->center.x += b->vel.x * delta;
    b->center.y += b->vel.y * delta;
    b->vel.x *= b->friction * delta;
    b->vel.y *= b->friction * delta;
    /*b->vel.x *= b->friction;
    b->vel.y *= b->friction;*/
    if ( (int) b->vel.x == 0 && (int) b->vel.y == 0 ) {
        b->moving = false;
    } else {
        b->moving = true;
    }
    b->prevPos = b->center;
}

void drawBall( Ball *b ) {

    DrawCircleV( b->center, b->radius, b->color );

    if ( b->striped ) {
        DrawLineEx( 
            (Vector2) { b->center.x - b->radius, b->center.y}, 
            (Vector2) { b->center.x + b->radius, b->center.y}, 
            2, 
            WHITE
        );
    }

    if ( b->moving ) {
        DrawCircleLinesV( b->center, b->radius, WHITE );
    } else {
        DrawCircleLinesV( b->center, b->radius, BLACK );
    }

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
