/**
 * @file GameWorld.h
 * @author Prof. Dr. David Buzatto
 * @brief GameWorld implementation.
 * 
 * @copyright Copyright (c) 2026
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "GameWorld.h"
#include "Types.h"

#include "Ball.h"
#include "CueStick.h"

#include "ResourceManager.h"

#include "raylib/raylib.h"
#include "raylib/raymath.h"
//#define RAYGUI_IMPLEMENTATION    // to use raygui, comment these three lines.
//#include "raylib/raygui.h"       // other compilation units must only include
//#undef RAYGUI_IMPLEMENTATION     // raygui.h

#define BALL_COUNT 15
#define BALL_RADIUS 10
#define BALL_FRICTION 0.99f
#define BALL_ELASTICITY 0.9f
#define CUE_BALL_BASE_IMPULSE 1000

static void resolveCollisionBallBall( Ball *b1, Ball *b2 );

void setupGameWorld( GameWorld *gw ) {

    int margin = 50;
    Color color[] = { 
        (Color) { .r = 255, .g = 215, .b = 0, .a = 255 }, // yellow
        (Color) { .r = 0, .g = 100, .b = 200, .a = 255 }, // blue
        (Color) { .r = 220, .g = 20, .b = 60, .a = 255 }, // red
        (Color) { .r = 75, .g = 0, .b = 130, .a = 255 },  // purple
        BLACK,                                            // black
        (Color) { .r = 255, .g = 100, .b = 0, .a = 255 }, // orange
        (Color) { .r = 0, .g = 128, .b = 0, .a = 255 },   // green
        (Color) { .r = 139, .g = 69, .b = 19, .a = 255 }, // brown
        (Color) { .r = 255, .g = 215, .b = 0, .a = 255 }, // yellow
        (Color) { .r = 0, .g = 100, .b = 200, .a = 255 }, // blue
        (Color) { .r = 220, .g = 20, .b = 60, .a = 255 }, // red
        (Color) { .r = 75, .g = 0, .b = 130, .a = 255 },  // purple
        (Color) { .r = 255, .g = 100, .b = 0, .a = 255 }, // orange
        (Color) { .r = 0, .g = 128, .b = 0, .a = 255 },   // green
        (Color) { .r = 139, .g = 69, .b = 19, .a = 255 }  // brown
    };
    bool striped[] = { false, false, false, false, false, false, false, false, true, true, true, true, true, true, true };
    int numbers[] = { 1, 2, 3, 4, 8, 5, 6, 7, 9, 10, 11, 12, 13, 14, 15 };

    gw->boundarie = (Rectangle) {
        margin,
        margin,
        700,
        350
    };

    // cue ball
    gw->cueBall = &gw->balls[0];
    gw->balls[0] = (Ball) {
        .center = { GetScreenWidth() / 2 - 200, GetScreenHeight() / 2 },
        .radius = BALL_RADIUS,
        .vel = { 0, 0 },
        .friction = BALL_FRICTION,
        .elasticity = BALL_ELASTICITY,
        .color = WHITE,
        .striped = false,
        .number = 0
    };
    gw->balls[0].prevPos = gw->balls[0].center;

    int k = 1;
    for ( int i = 0; i < 5; i++ ) {
        float iniY = GetScreenHeight() / 2 - BALL_RADIUS * i;
        for ( int j = 0; j <= i; j++ ) {
            gw->balls[k] = (Ball) {
                .center = { 
                    GetScreenWidth() / 2 + 150 + ( BALL_RADIUS * 2 ) * i - 2.5f * i, 
                    iniY + ( BALL_RADIUS * 2 ) * j + 0.5f * j
                },
                .prevPos = { 0 },
                .radius = BALL_RADIUS,
                .vel = { 0, 0 },
                .friction = BALL_FRICTION,
                .elasticity = BALL_ELASTICITY,
                .color = color[k-1],
                .striped = striped[k-1],
                .number = numbers[k-1]
            };
            gw->balls[k].prevPos = gw->balls[k].center;
            k++;
        }
    }
    
    gw->cueStick = (CueStick) {
        .target = gw->cueBall->center,
        .distanceFromTarget = 20,
        .size = 200,
        .angle = 0,
        .impulse = 100
    };

    gw->state = GAME_STATE_BALLS_STOPPED;

}

/**
 * @brief Creates a dinamically allocated GameWorld struct instance.
 */
GameWorld* createGameWorld( void ) {
    GameWorld *gw = (GameWorld*) malloc( sizeof( GameWorld ) );
    setupGameWorld( gw );
    return gw;
}

/**
 * @brief Destroys a GameWindow object and its dependecies.
 */
void destroyGameWorld( GameWorld *gw ) {
    free( gw );
}

/**
 * @brief Reads user input and updates the state of the game.
 */
void updateGameWorld( GameWorld *gw, float delta ) {

    if ( IsKeyPressed( KEY_R ) ) {
        setupGameWorld( gw );
    }

    if ( gw->state == GAME_STATE_BALLS_STOPPED ) {

        if ( IsKeyPressed( KEY_SPACE ) ) {
            gw->cueBall->vel.x = gw->cueStick.impulse * cosf( DEG2RAD * gw->cueStick.angle );
            gw->cueBall->vel.y = gw->cueStick.impulse * sinf( DEG2RAD * gw->cueStick.angle );
        }

        updateCueStick( &gw->cueStick, delta );

    }

    bool ballsMoving = false;

    for ( int i = 0; i <= BALL_COUNT; i++ ) {

        Ball *b = &gw->balls[i];
        updateBall( b, delta );

        // boundarie collision
        float left = gw->boundarie.x;
        float right = gw->boundarie.x + gw->boundarie.width;
        float top = gw->boundarie.y;
        float down = gw->boundarie.y + gw->boundarie.height;

        if ( b->center.x - b->radius < left ) {
            b->center.x = left + b->radius;
            b->vel.x = -b->vel.x * b->elasticity;
        } else if ( b->center.x + b->radius > right ) {
            b->center.x = right - b->radius;
            b->vel.x = -b->vel.x * b->elasticity;
        }

        if ( b->center.y - b->radius < top ) {
            b->center.y = top + b->radius;
            b->vel.y = -b->vel.y * b->elasticity;
        } else if ( b->center.y + b->radius > down ) {
            b->center.y = down - b->radius;
            b->vel.y = -b->vel.y * b->elasticity;
        }

        for ( int j = 0; j <= BALL_COUNT; j++ ) {
            if ( j != i ) {
                Ball *bt = &gw->balls[j];
                if ( CheckCollisionCircles( b->center, b->radius, bt->center, bt->radius ) ) {
                    resolveCollisionBallBall( b, bt );
                }
            }
        }

        if ( !ballsMoving && b->moving ) {
            ballsMoving = true;
        }

    }

    gw->cueStick.target = gw->cueBall->center;

    if ( ballsMoving ) {
        gw->state = GAME_STATE_BALLS_MOVING;
    } else {
        gw->state = GAME_STATE_BALLS_STOPPED;
    }

}

/**
 * @brief Draws the state of the game.
 */
void drawGameWorld( GameWorld *gw ) {

    BeginDrawing();
    ClearBackground( BLACK );

    DrawRectangleRec( gw->boundarie, DARKGREEN );

    for ( int i = 0; i <= BALL_COUNT; i++ ) {
        drawBall( &gw->balls[i] );
    }

    if ( gw->state == GAME_STATE_BALLS_STOPPED ) {
        drawCueStick( &gw->cueStick );
    }

    EndDrawing();

}

static void resolveCollisionBallBall( Ball *b1, Ball *b2 ) {

    // vector between centers
    Vector2 d = {
        b2->center.x - b1->center.x,
        b2->center.y - b1->center.y
    };

    float distance = Vector2Distance( b1->center, b2->center );
    float minDistance = b1->radius + b2->radius;

    // the balls are not colliding
    if (distance >= minDistance) {
        return;
    }

    // normalizing
    Vector2 norm = {
        d.x / distance,
        d.y / distance
    };

    // important: separate the balls
    float overlap = minDistance - distance;
    float separation = overlap / 2.0f;

    b1->center.x -= separation * norm.x;
    b1->center.y -= separation * norm.y;
    b2->center.x += separation * norm.x;
    b2->center.y += separation * norm.y;

    // relative velocity
    Vector2 v = {
        b1->vel.x - b2->vel.x,
        b1->vel.y - b2->vel.y
    };

    // velocity on normal
    float dotProduct = Vector2DotProduct( v, norm );

    // do not collide if the balls are moving away
    if ( dotProduct <= 0 ) {
        return;
    }

    // transfer momentum (equal mass)
    b1->vel.x -= dotProduct * norm.x;
    b1->vel.y -= dotProduct * norm.y;
    b2->vel.x += dotProduct * norm.x;
    b2->vel.y += dotProduct * norm.y;

}