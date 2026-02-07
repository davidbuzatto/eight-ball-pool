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
#define BALL_FRICTION 59.4
#define BALL_ELASTICITY 0.9f
#define SHUFFLE_BALLS true

static void resolveCollisionBallBall( Ball *b1, Ball *b2 );
static void shuffleColorsAndNumbers( Color *colors, int *numbers, int size );
static void prepareBallData( Color *colors, bool *striped, int *numbers, bool shuffleBalls );

static void setupGameWorld( GameWorld *gw ) {

    int margin = 100;

    Color colors[15];
    bool striped[15];
    int numbers[15];

    prepareBallData( colors, striped, numbers, SHUFFLE_BALLS );

    gw->boundarie = (Rectangle) {
        margin,
        margin,
        700,
        350
    };

    // cue ball
    gw->cueBall = &gw->balls[0];
    gw->balls[0] = (Ball) {
        .center = { gw->boundarie.x + gw->boundarie.width / 4, GetScreenHeight() / 2 },
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
                    gw->boundarie.x + gw->boundarie.width - gw->boundarie.width / 4 + ( BALL_RADIUS * 2 ) * i - 2.5f * i, 
                    iniY + ( BALL_RADIUS * 2 ) * j + 0.5f * j
                },
                .prevPos = { 0 },
                .radius = BALL_RADIUS,
                .vel = { 0, 0 },
                .friction = BALL_FRICTION,
                .elasticity = BALL_ELASTICITY,
                .color = colors[k-1],
                .striped = striped[k-1],
                .number = numbers[k-1]
            };
            gw->balls[k].prevPos = gw->balls[k].center;
            k++;
        }
    }
    
    gw->cueStick = (CueStick) {
        .target = gw->cueBall->center,
        .distanceFromTarget = BALL_RADIUS,
        .size = 200,
        .angle = 0,
        .impulse = 100,
        .minImpulse = 100,
        .maxImpulse = 1400
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

        /*if ( IsKeyPressed( KEY_SPACE ) ) {
            gw->cueBall->vel.x = gw->cueStick.impulse * cosf( DEG2RAD * gw->cueStick.angle );
            gw->cueBall->vel.y = gw->cueStick.impulse * sinf( DEG2RAD * gw->cueStick.angle );
        }*/

        if ( IsMouseButtonPressed( MOUSE_BUTTON_LEFT ) ) {
            gw->cueBall->vel.x = gw->cueStick.impulse * cosf( DEG2RAD * gw->cueStick.angle );
            gw->cueBall->vel.y = gw->cueStick.impulse * sinf( DEG2RAD * gw->cueStick.angle );
            gw->cueStick.impulse = gw->cueStick.minImpulse;
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
    ClearBackground( DARKBLUE );

    float space = gw->boundarie.width / 8;
    int tableMargin = 40;

    DrawRectangleRounded( 
        (Rectangle) {
            gw->boundarie.x - tableMargin,
            gw->boundarie.y - tableMargin,
            gw->boundarie.width + tableMargin * 2,
            gw->boundarie.height + tableMargin * 2
        },
        0.1f,
        10,
        BROWN
    );

    DrawRectangle( 
        gw->boundarie.x - tableMargin / 3,
        gw->boundarie.y - tableMargin / 3,
        gw->boundarie.width + tableMargin / 3 * 2,
        gw->boundarie.height + tableMargin / 3 * 2,
        GREEN
    );

    for ( int i = 1; i <= 7; i++ ) {
        if ( i != 4 ) {
            DrawCircle( 
                gw->boundarie.x + space * i, 
                gw->boundarie.y - tableMargin / 3 * 2, 
                3, WHITE
            );
            DrawCircle( 
                gw->boundarie.x + space * i, 
                gw->boundarie.y + gw->boundarie.height + tableMargin / 3 * 2,
                3, WHITE
            );
        }
    }

    for ( int i = 1; i <= 3; i++ ) {
        DrawCircle( 
            gw->boundarie.x - tableMargin / 3 * 2, 
            gw->boundarie.y + space * i, 
            3, WHITE
        );
        DrawCircle( 
            gw->boundarie.x + gw->boundarie.width + tableMargin / 3 * 2, 
            gw->boundarie.y + space * i, 
            3, WHITE
        );
    }

    // pockets
    DrawCircle( 
        gw->boundarie.x - tableMargin / 2 + 6, 
        gw->boundarie.y - tableMargin / 2 + 6, 
        tableMargin / 2, 
        BLACK
    );

    DrawCircle( 
        gw->boundarie.x + gw->boundarie.width / 2, 
        gw->boundarie.y - tableMargin / 2 + 3, 
        tableMargin / 2.5, 
        BLACK
    );

    DrawCircle( 
        gw->boundarie.x + gw->boundarie.width + tableMargin / 2 - 6, 
        gw->boundarie.y - tableMargin / 2 + 6, 
        tableMargin / 2, 
        BLACK
    );

    DrawCircle( 
        gw->boundarie.x - tableMargin / 2 + 6, 
        gw->boundarie.y + gw->boundarie.height + tableMargin / 2 - 6, 
        tableMargin / 2, 
        BLACK
    );

    DrawCircle( 
        gw->boundarie.x + gw->boundarie.width / 2, 
        gw->boundarie.y + gw->boundarie.height + tableMargin / 2 - 3, 
        tableMargin / 2.5, 
        BLACK
    );

    DrawCircle( 
        gw->boundarie.x + gw->boundarie.width + tableMargin / 2 - 6, 
        gw->boundarie.y + gw->boundarie.height + tableMargin / 2 - 6, 
        tableMargin / 2, 
        BLACK
    );

    DrawRectangleRec( gw->boundarie, DARKGREEN );

    DrawLine( 
        gw->boundarie.x + space * 2, 
        gw->boundarie.y,
        gw->boundarie.x + space * 2, 
        gw->boundarie.y + gw->boundarie.height,
        WHITE
    );

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
    if ( distance >= minDistance ) {
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

static void shuffleColorsAndNumbers( Color *colors, int *numbers, int size ) {
    for ( int i = 0; i < size; i++ ) {
        int p = GetRandomValue( 0, size - 1 );
        Color c = colors[i];
        colors[i] = colors[p];
        colors[p] = c;
        int n = numbers[i];
        numbers[i] = numbers[p];
        numbers[p] = n;
    }
}

static void prepareBallData( Color *colors, bool *striped, int *numbers, bool shuffleBalls ) {

    Color solidColors[] = {
        (Color) { .r = 255, .g = 215, .b = 0, .a = 255 }, // yellow
        (Color) { .r = 0, .g = 100, .b = 200, .a = 255 }, // blue
        (Color) { .r = 220, .g = 20, .b = 60, .a = 255 }, // red
        (Color) { .r = 75, .g = 0, .b = 130, .a = 255 },  // purple
        (Color) { .r = 255, .g = 100, .b = 0, .a = 255 }, // orange
        (Color) { .r = 0, .g = 128, .b = 0, .a = 255 },   // green
        (Color) { .r = 139, .g = 69, .b = 19, .a = 255 }, // brown
    };
    int solidNumbers[] = { 1, 2, 3, 4, 5, 6, 7 };

    Color stripeColors[] = {
        (Color) { .r = 255, .g = 215, .b = 0, .a = 255 }, // yellow
        (Color) { .r = 0, .g = 100, .b = 200, .a = 255 }, // blue
        (Color) { .r = 220, .g = 20, .b = 60, .a = 255 }, // red
        (Color) { .r = 75, .g = 0, .b = 130, .a = 255 },  // purple
        (Color) { .r = 255, .g = 100, .b = 0, .a = 255 }, // orange
        (Color) { .r = 0, .g = 128, .b = 0, .a = 255 },   // green
        (Color) { .r = 139, .g = 69, .b = 19, .a = 255 }, // brown
    };
    int stripeNumbers[] = { 9, 10, 11, 12, 13, 14, 15 };

    if ( shuffleBalls ) {
        shuffleColorsAndNumbers( solidColors, solidNumbers, 7 );
        shuffleColorsAndNumbers( stripeColors, stripeNumbers, 7 );
    }

    Color colorQueue[12];
    int numberQueue[12];

    for ( int i = 1; i < 7; i++ ) {
        colorQueue[i-1] = solidColors[i];
        colorQueue[i+5] = stripeColors[i];
        numberQueue[i-1] = solidNumbers[i];
        numberQueue[i+5] = stripeNumbers[i];
    }

    if ( shuffleBalls ) {
        shuffleColorsAndNumbers( colorQueue, numberQueue, 12 );
    }

    int q = 0;
    for ( int i = 0; i < 15; i++ ) {
        if ( i == 4 ) {
            colors[i] = BLACK;
            numbers[i] = 8;
            striped[i] = false;
        } else if ( i == 10 ) {
            colors[i] = solidColors[0];
            numbers[i] = solidNumbers[0];
            striped[i] = false;
        } else if ( i == 14 ) {
            colors[i] = stripeColors[0];
            numbers[i] = stripeNumbers[0];
            striped[i] = true;
        } else {
            colors[i] = colorQueue[q];
            numbers[i] = numberQueue[q];
            striped[i] = numbers[i] > 8;
            q++;
        }
    }

}