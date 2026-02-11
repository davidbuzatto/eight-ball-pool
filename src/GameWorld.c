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

#include "raylib/raylib.h"
#include "raylib/raymath.h"
//#define RAYGUI_IMPLEMENTATION    // to use raygui, comment these three lines.
//#include "raylib/raygui.h"       // other compilation units must only include
//#undef RAYGUI_IMPLEMENTATION     // raygui.h

#include "Ball.h"
#include "CueStick.h"
#include "Cushion.h"
#include "GameWorld.h"
#include "Pocket.h"
#include "ResourceManager.h"
#include "Types.h"

#define BALL_COUNT 15
#define BALL_RADIUS 10
#define BALL_FRICTION 0.99f
#define BALL_ELASTICITY 0.9f
#define SHUFFLE_BALLS true

static const Color EBP_YELLOW = { .r = 255, .g = 215, .b = 0,   .a = 255 };
static const Color EBP_BLUE   = { .r = 0,   .g = 100, .b = 200, .a = 255 };
static const Color EBP_RED    = { .r = 220, .g = 20,  .b = 60,  .a = 255 };
static const Color EBP_PURPLE = { .r = 75,  .g = 0,   .b = 130, .a = 255 };
static const Color EBP_ORANGE = { .r = 255, .g = 100, .b = 0,   .a = 255 };
static const Color EBP_GREEN  = { .r = 0,   .g = 128, .b = 0,   .a = 255 };
static const Color EBP_BROWN  = { .r = 139, .g = 69,  .b = 19,  .a = 255 };

static const int MARGIN = 100;
static const int TABLE_MARGIN = 40;

static float marksSpacing;

static Color scoreColors[] = { 
    EBP_YELLOW, // 1
    EBP_BLUE,   // 2
    EBP_RED,    // 3
    EBP_PURPLE, // 4
    EBP_ORANGE, // ...
    EBP_GREEN,
    EBP_BROWN,
    { 0, 0, 0, 255 },
    EBP_YELLOW,
    EBP_BLUE,
    EBP_RED,
    EBP_PURPLE,
    EBP_ORANGE,
    EBP_GREEN,
    EBP_BROWN   // 15
};

static void drawHud( GameWorld *gw );
static void setupGameWorld( GameWorld *gw );
static void shuffleColorsAndNumbers( Color *colors, int *numbers, int size );
static void prepareBallData( Color *colors, bool *striped, int *numbers, bool shuffleBalls );

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

    // prev positions here (needed for cushion collision)
    for ( int i = 0; i <= BALL_COUNT; i++ ) {
        gw->balls[i].prevPos = gw->balls[i].center;
    }

    if ( gw->state == GAME_STATE_BALLS_STOPPED ) {

        if ( IsMouseButtonPressed( MOUSE_BUTTON_LEFT ) ) {
            CueStick *cc = gw->currentCueStick;
            gw->cueBall->vel.x = cc->power * cosf( DEG2RAD * cc->angle );
            gw->cueBall->vel.y = cc->power * sinf( DEG2RAD * cc->angle );
            cc->power = cc->minPower;
            if ( cc == &gw->cueStickP1 ) {
                gw->currentCueStick = &gw->cueStickP2;
            } else {
                gw->currentCueStick = &gw->cueStickP1;
            }
        }

        updateCueStick( gw->currentCueStick, delta );

    }

    bool ballsMoving = false;

    for ( int i = 0; i <= BALL_COUNT; i++ ) {

        Ball *b = &gw->balls[i];

        if ( b->pocketed ) {
            continue;
        }

        updateBall( b, delta );

        // cushion collision
        for ( int j = 0; j < 6; j++ ) {

            Cushion *c = &gw->cushions[j];
            CollisionResult collision = ballCushionCollision( b, c );

            if ( collision.hasCollision ) {

                // puts the ball in the exact point of contact
                Vector2 movement = Vector2Subtract( b->center, b->prevPos );
                b->center = Vector2Add( b->prevPos, Vector2Scale( movement, collision.t ) );

                // calculates the reflection of the velocity
                float dotProduct = Vector2DotProduct( b->vel, collision.normal );
                b->vel = Vector2Subtract( b->vel, Vector2Scale( collision.normal, 2.0f * dotProduct ) ); 

                // aplies elasticity
                b->vel = Vector2Scale( b->vel, b->elasticity );

                // apply some offset to prevent continuous collision
                b->center = Vector2Add( b->center, Vector2Scale( collision.normal, 0.1f ) );

            }

        }

        // ball x ball
        for ( int j = 0; j <= BALL_COUNT; j++ ) {
            if ( j != i ) {
                Ball *bt = &gw->balls[j];
                if ( bt->pocketed ) {
                    continue;
                }
                if ( CheckCollisionCircles( b->center, b->radius, bt->center, bt->radius ) ) {
                    resolveCollisionBallBall( b, bt );
                }
            }
        }

        // ball x pockets
        for ( int j = 0; j < 6; j++ ) {

            float dist = Vector2Distance( b->center,  gw->pockets[j].center );

            // more than 50% of ball is inside the pocket
            if ( dist < gw->pockets[j].radius - b->radius * 0.5f ) {

                b->pocketed = true;
                b->vel = (Vector2) { 0 };
                b->moving = false;

                // player 2 pocketed
                if ( gw->currentCueStick == &gw->cueStickP1 ) {
                    gw->cueStickP2.pocketedBalls[gw->cueStickP2.pocketedCount++] = b->number;
                } else {
                    gw->cueStickP1.pocketedBalls[gw->cueStickP1.pocketedCount++] = b->number;
                }

                break;

            }

        }

        if ( !ballsMoving && b->moving ) {
            ballsMoving = true;
        }

    }

    gw->currentCueStick->target = gw->cueBall->center;

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
    ClearBackground( (Color) { 28, 38, 58, 255 } );

    DrawRectangleRounded( 
        (Rectangle) {
            gw->boundarie.x - TABLE_MARGIN,
            gw->boundarie.y - TABLE_MARGIN,
            gw->boundarie.width + TABLE_MARGIN * 2,
            gw->boundarie.height + TABLE_MARGIN * 2
        },
        0.1f,
        10,
        (Color) { 135, 38, 8, 255 }
    );

    DrawRectangleRoundedLines( 
        (Rectangle) {
            gw->boundarie.x - TABLE_MARGIN,
            gw->boundarie.y - TABLE_MARGIN,
            gw->boundarie.width + TABLE_MARGIN * 2,
            gw->boundarie.height + TABLE_MARGIN * 2
        },
        0.1f,
        10,
        BLACK
    );

    DrawRectangle( 
        gw->boundarie.x - TABLE_MARGIN / 3,
        gw->boundarie.y - TABLE_MARGIN / 3,
        gw->boundarie.width + TABLE_MARGIN / 3 * 2,
        gw->boundarie.height + TABLE_MARGIN / 3 * 2,
        DARKGREEN
    );

    // top marks
    for ( int i = 1; i <= 7; i++ ) {
        if ( i != 4 ) {
            DrawCircle( 
                gw->boundarie.x + marksSpacing * i, 
                gw->boundarie.y - TABLE_MARGIN / 3 * 2, 
                3, WHITE
            );
            DrawCircle( 
                gw->boundarie.x + marksSpacing * i, 
                gw->boundarie.y + gw->boundarie.height + TABLE_MARGIN / 3 * 2,
                3, WHITE
            );
        }
    }

    // bottom marks
    for ( int i = 1; i <= 3; i++ ) {
        DrawCircle( 
            gw->boundarie.x - TABLE_MARGIN / 3 * 2, 
            gw->boundarie.y + marksSpacing * i, 
            3, WHITE
        );
        DrawCircle( 
            gw->boundarie.x + gw->boundarie.width + TABLE_MARGIN / 3 * 2, 
            gw->boundarie.y + marksSpacing * i, 
            3, WHITE
        );
    }

    DrawLine( 
        gw->boundarie.x + marksSpacing * 2, 
        gw->boundarie.y,
        gw->boundarie.x + marksSpacing * 2, 
        gw->boundarie.y + gw->boundarie.height,
        WHITE
    );

    // pockets
    for ( int i = 0; i < 6; i++ ) {
        drawPocket( &gw->pockets[i] );
    }

    // cushions
    for ( int i = 0; i < 6; i++ ) {
        drawCushion( &gw->cushions[i] );
    }

    for ( int i = 0; i <= BALL_COUNT; i++ ) {
        drawBall( &gw->balls[i] );
    }

    if ( gw->state == GAME_STATE_BALLS_STOPPED ) {
        drawCueStick( gw->currentCueStick );
    }

    drawHud( gw );

    EndDrawing();

}

static void drawHud( GameWorld *gw ) {
    
    int width = 16;
    int height = 200;

    int x = GetScreenWidth() - width - 20;
    int y = GetScreenHeight() / 2 - height / 2;

    float powerP = getCueStickPowerPercentage( gw->currentCueStick );
    float powerHeight = ( height - 4 ) * powerP;
    float powerHue = 120.0f - 120.0f * powerP;

    DrawRectangle( x, y, width, height, BLACK );
    DrawRectangle( x + 3, y + 3, width - 6, height - 6, RAYWHITE );
    DrawRectangle( x + 3, y + 3 + height - 4 - powerHeight, width - 6, powerHeight, ColorFromHSV( powerHue, 1.0f, 1.0f ) );

    const char *percT = TextFormat( "%.2f%%", powerP * 100 );
    int wPercT = MeasureText( percT, 10 );

    DrawText( percT, x + width / 2 - wPercT / 2, y + height + 5, 10, WHITE );

    DrawRectangleRounded( 
        (Rectangle) {
            -50, -50, 330, 90
        },
        0.4f,
        10,
        (Color) { 14, 18, 33, 255 }
    );

    DrawRectangleRoundedLines( 
        (Rectangle) {
            -50, -50, 330, 90
        },
        0.4f,
        10,
        RAYWHITE
    );

    DrawRectangleRounded( 
        (Rectangle) {
            5, 5, 40, 28
        },
        0.4f,
        10,
        gw->cueStickP1.color
    );

    DrawText( "P1", 15, 10, 20, RAYWHITE );

    DrawRectangleRounded( 
        (Rectangle) {
            GetScreenWidth() / 2 + 170, -50, 330, 90
        },
        0.4f,
        10,
        (Color) { 14, 18, 33, 255 }
    );

    DrawRectangleRoundedLines( 
        (Rectangle) {
            GetScreenWidth() / 2 + 170, -50, 330, 90
        },
        0.4f,
        10,
        RAYWHITE
    );

    DrawRectangleRounded( 
        (Rectangle) {
            GetScreenWidth() - 45, 5, 40, 28
        },
        0.4f,
        10,
        gw->cueStickP2.color
    );

    DrawText( "P2", GetScreenWidth() - 45 + 8, 10, 20, RAYWHITE );

    if ( gw->currentCueStick == &gw->cueStickP1 ) {
        DrawRectangleRoundedLines( 
            (Rectangle) {
                5, 5, 40, 28
            },
            0.4f,
            10,
            RAYWHITE
        );
    } else {
        DrawRectangleRoundedLines( 
            (Rectangle) {
                GetScreenWidth() - 45, 5, 40, 28
            },
            0.4f,
            10,
            RAYWHITE
        );
    }

    int spacing = 8;
    int radius = BALL_RADIUS + 2;

    int startScoreP1 = 65;
    int startScoreP2 = 642;

    // TODO: refactor
    for ( int i = 0; i < 7; i++ ) {
        int x = startScoreP1 + ( radius * 2 + spacing ) * i;
        DrawCircle( x, 19, radius, (Color) { 23, 23, 27, 255 } );
        DrawCircleLines( x, 19, radius, GRAY );
        if ( i < gw->cueStickP1.pocketedCount ) {
            int number = gw->cueStickP1.pocketedBalls[i];
            DrawCircle( x, 19, radius - 2, scoreColors[number-1] );
            if ( number > 8 ) {
                DrawLineEx( 
                    (Vector2) { x - radius + 2, 19 }, 
                    (Vector2) { x + radius - 2, 19 }, 
                    4, 
                    WHITE
                );
            }
            const char *numberText = TextFormat( "%d", number );
            int w = MeasureText( numberText, 14 );
            DrawText( numberText, x - w / 2, 13, 14, BLACK );
        }
    }
    
    for ( int i = 0; i < 7; i++ ) {
        int x = startScoreP2 + ( radius * 2 + spacing ) * i;
        DrawCircle( x, 19, radius, (Color) { 23, 23, 27, 255 } );
        DrawCircleLines( x, 19, radius, GRAY );
        if ( i < gw->cueStickP2.pocketedCount ) {
            int number = gw->cueStickP2.pocketedBalls[i];
            DrawCircle( x, 19, radius - 2, scoreColors[number-1] );
            if ( number > 8 ) {
                DrawLineEx( 
                    (Vector2) { x - radius + 2, 19 }, 
                    (Vector2) { x + radius - 2, 19 }, 
                    4, 
                    WHITE
                );
            }
            const char *numberText = TextFormat( "%d", number );
            int w = MeasureText( numberText, 14 );
            DrawText( numberText, x - w / 2, 13, 14, BLACK );
        }
    }

}

static void setupGameWorld( GameWorld *gw ) {

    Color colors[15];
    bool striped[15];
    int numbers[15];

    prepareBallData( colors, striped, numbers, SHUFFLE_BALLS );

    gw->boundarie = (Rectangle) {
        MARGIN,
        MARGIN,
        700,
        350
    };

    marksSpacing = gw->boundarie.width / 8;

    // pockets
    // top left
    gw->pockets[0] = (Pocket) {
        .center = {
            gw->boundarie.x - TABLE_MARGIN / 2 + 6, 
            gw->boundarie.y - TABLE_MARGIN / 2 + 6
        },
        .radius = TABLE_MARGIN / 2
    };

    // top center
    gw->pockets[1] = (Pocket) {
        .center = {
            gw->boundarie.x + gw->boundarie.width / 2, 
            gw->boundarie.y - TABLE_MARGIN / 2 + 3, 
        },
        .radius = TABLE_MARGIN / 2.5f
    };

    // top right
    gw->pockets[2] = (Pocket) {
        .center = {
            gw->boundarie.x + gw->boundarie.width + TABLE_MARGIN / 2 - 6, 
            gw->boundarie.y - TABLE_MARGIN / 2 + 6
        },
        .radius = TABLE_MARGIN / 2
    };

    // bottom left
    gw->pockets[3] = (Pocket) {
        .center = {
            gw->boundarie.x - TABLE_MARGIN / 2 + 6, 
            gw->boundarie.y + gw->boundarie.height + TABLE_MARGIN / 2 - 6
        },
        .radius = TABLE_MARGIN / 2
    };

    // bottom center
    gw->pockets[4] = (Pocket) {
        .center = {
            gw->boundarie.x + gw->boundarie.width / 2, 
            gw->boundarie.y + gw->boundarie.height + TABLE_MARGIN / 2 - 3
        },
        .radius = TABLE_MARGIN / 2.5f
    };

    // bottom right
    gw->pockets[5] = (Pocket) {
        .center = {
            gw->boundarie.x + gw->boundarie.width + TABLE_MARGIN / 2 - 6, 
            gw->boundarie.y + gw->boundarie.height + TABLE_MARGIN / 2 - 6
        },
        .radius = TABLE_MARGIN / 2
    };

    // top left
    gw->cushions[0] = (Cushion) {
        {
            { 105, 86 },
            { 435, 86 },
            { 430, 100 },
            { 120, 100 }
        }
    };

    // top right
    gw->cushions[1] = (Cushion) {
        {
            { 465, 86 },
            { 795, 86 },
            { 780, 100 },
            { 470, 100 }
        }
    };

    // bottom left
    gw->cushions[2] = (Cushion) {
        {
            { 120, 450 },
            { 430, 450 },
            { 435, 464 },
            { 105, 464 }
        }
    };

    // bottom right
    gw->cushions[3] = (Cushion) {
        {
            { 470, 450 },
            { 780, 450 },
            { 795, 464 },
            { 465, 464 }
        }
    };

    // head
    gw->cushions[4] = (Cushion) {
        {
            { 86, 105 },
            { 100, 120 },
            { 100, 430 },
            { 86, 445 }
        }
    };

    // foot
    gw->cushions[5] = (Cushion) {
        {
            { 800, 120 },
            { 814, 105 },
            { 814, 445 },
            { 800, 430 }
        }
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
        .number = 0,
        .pocketed = false
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
                .number = numbers[k-1],
                .pocketed = false
            };
            gw->balls[k].prevPos = gw->balls[k].center;
            k++;
        }
    }
    
    int powerTick = 10;
    int maxPower = 1400;

    gw->cueStickP1 = (CueStick) {
        .target = gw->cueBall->center,
        .distanceFromTarget = BALL_RADIUS,
        .size = 300,
        .angle = 0,
        .powerTick = powerTick,
        .power = powerTick,
        .minPower = powerTick,
        .maxPower = maxPower,
        .color = { 17, 50, 102, 255 },
        .pocketedBalls = { 0 },
        .pocketedCount = 0
        /*.pocketedBalls = { 1, 2, 3, 4, 5, 6, 7 },
        .pocketedCount = 7*/
    };

    gw->cueStickP2 = (CueStick) {
        .target = gw->cueBall->center,
        .distanceFromTarget = BALL_RADIUS,
        .size = 300,
        .angle = 0,
        .powerTick = powerTick,
        .power = powerTick,
        .minPower = powerTick,
        .maxPower = maxPower,
        .color = { 102, 17, 37, 255 },
        .pocketedBalls = { 0 },
        .pocketedCount = 0
        /*.pocketedBalls = { 9, 10, 11, 12, 13, 14, 15 },
        .pocketedCount = 7*/
    };

    gw->currentCueStick = &gw->cueStickP1;

    gw->state = GAME_STATE_BALLS_STOPPED;

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
        EBP_YELLOW,
        EBP_BLUE,
        EBP_RED,
        EBP_PURPLE,
        EBP_ORANGE,
        EBP_GREEN,
        EBP_BROWN
    };
    int solidNumbers[] = { 1, 2, 3, 4, 5, 6, 7 };

    Color stripeColors[] = {
        EBP_YELLOW,
        EBP_BLUE,
        EBP_RED,
        EBP_PURPLE,
        EBP_ORANGE,
        EBP_GREEN,
        EBP_BROWN
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
