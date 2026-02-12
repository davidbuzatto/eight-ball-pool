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

#define TEST_BALL_POSITIONING true
#define SHUFFLE_BALLS false
#define BG_MUSIC_ENABLED false

#define BALL_COUNT 15
#define BALL_RADIUS 10
#define BALL_FRICTION 0.99f
#define BALL_ELASTICITY 0.9f

static const Color EBP_YELLOW = { 255, 215, 0,   255 };
static const Color EBP_BLUE   = { 0,   100, 200, 255 };
static const Color EBP_RED    = { 220, 20,  60,  255 };
static const Color EBP_PURPLE = { 75,  0,   130, 255 };
static const Color EBP_ORANGE = { 255, 100, 0,   255 };
static const Color EBP_GREEN  = { 0,   128, 0,   255 };
static const Color EBP_BROWN  = { 139, 69,  19,  255 };

static const Color BG_COLOR = { 28, 38, 58, 255 };
static const Color TABLE_COLOR = { 135, 38, 8, 255 };
static const Color SCORE_BG_COLOR = { 14, 18, 33, 255 };
static const Color SCORE_POCKET_COLOR = { 23, 23, 27, 255 };

static const int MARGIN = 100;
static const int TABLE_MARGIN = 40;

static float marksSpacing = 0;
static Ball *selectedBall = NULL;
static Vector2 pressOffset = { 0 };

static float highlighCurrentPlayerTime = 0.8f;
static float highlighCurrentPlayerCounter = 0.0f;

static void drawHud( GameWorld *gw );
static void setupGameWorld( GameWorld *gw );
static void shuffleColorsAndNumbers( Color *colors, int *numbers, int size );
static void prepareBallData( Color *colors, bool *striped, int *numbers, bool shuffleBalls );
static void playBallHitSound( void );
static void playBallCushionHitSound( void );

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

    if ( BG_MUSIC_ENABLED ) {
        UpdateMusicStream( rm.backgroundMusic );
    }

    if ( IsKeyPressed( KEY_R ) ) {
        StopMusicStream( rm.backgroundMusic );
        setupGameWorld( gw );
    }

    // prev positions here (needed for cushion collision)
    for ( int i = 0; i <= BALL_COUNT; i++ ) {
        gw->balls[i].prevPos = gw->balls[i].center;
    }

    if ( gw->state == GAME_STATE_BALLS_STOPPED ) {

        if ( IsMouseButtonPressed( MOUSE_BUTTON_RIGHT ) ) {
            Vector2 mp = GetMousePosition();
            for ( int i = 0; i <= BALL_COUNT; i++ ) {
                Ball *b = &gw->balls[i];
                if ( !b->pocketed && Vector2Distance( b->center, mp ) <= b->radius ) {
                    pressOffset = Vector2Subtract( mp, b->center );
                    selectedBall = b;
                    break;
                }
            }
        } else if ( IsMouseButtonReleased( MOUSE_BUTTON_RIGHT ) ) {
            if ( selectedBall != NULL ) {
                //
            }
            selectedBall = NULL;
        }

        if ( selectedBall != NULL ) {
            selectedBall->center = Vector2Subtract( GetMousePosition(), pressOffset );
        }

        if ( IsMouseButtonPressed( MOUSE_BUTTON_LEFT ) ) {
            PlaySound( rm.cueStickHitSound );
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

                playBallCushionHitSound();

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
                    if ( b == gw->cueBall ) {
                        float speed = sqrtf( b->vel.x * b->vel.x + b->vel.y * b->vel.y );
                        if ( speed > 400.0f ) { // 400 pixels/second
                            PlaySound( rm.cueBallHitSound );
                        } else {
                            playBallHitSound();    
                        }
                    } else {
                        playBallHitSound();
                    }
                    resolveCollisionBallBall( b, bt );
                }
            }
        }

        // ball x pockets
        for ( int j = 0; j < 6; j++ ) {

            float dist = Vector2Distance( b->center,  gw->pockets[j].center );

            // more than 50% of ball is inside the pocket
            if ( dist < gw->pockets[j].radius - b->radius * 0.5f ) {

                PlaySound( rm.ballFallingSound );

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

    highlighCurrentPlayerCounter += delta;
    if ( highlighCurrentPlayerCounter >= highlighCurrentPlayerTime ) {
        highlighCurrentPlayerCounter = 0.0f;
    }

}

/**
 * @brief Draws the state of the game.
 */
void drawGameWorld( GameWorld *gw ) {

    BeginDrawing();
    ClearBackground( BG_COLOR );

    DrawRectangleRounded( 
        (Rectangle) {
            gw->boundarie.x - TABLE_MARGIN,
            gw->boundarie.y - TABLE_MARGIN,
            gw->boundarie.width + TABLE_MARGIN * 2,
            gw->boundarie.height + TABLE_MARGIN * 2
        },
        0.1f,
        10,
        TABLE_COLOR
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

    if ( gw->state == GAME_STATE_BALLS_STOPPED && selectedBall == NULL ) {
        drawCueStick( gw->currentCueStick );
    }

    drawHud( gw );

    EndDrawing();

}

static void drawHud( GameWorld *gw ) {
    
    int cueStickAngleX = GetScreenWidth() - 29;
    int cueStickAngleY = 150;
    int cueStickAngleRadius = 21;
    float cueStickAngle = gw->currentCueStick->angle;
    float cueStickAngleAntiClock = cueStickAngle <= 0 ? fabs( cueStickAngle ) : 360.0f - cueStickAngle;

    DrawCircle( cueStickAngleX, cueStickAngleY, cueStickAngleRadius, GRAY );

    DrawCircleSector( 
        (Vector2) { 
            cueStickAngleX, 
            cueStickAngleY
        },
        cueStickAngleRadius,
        -cueStickAngleAntiClock,
        0,
        30,
        DARKGRAY
    );

    DrawCircleLines( cueStickAngleX, cueStickAngleY, cueStickAngleRadius, RAYWHITE );

    const char *angleText = TextFormat( "%.2f", cueStickAngleAntiClock );
    int wAngleText = MeasureText( angleText, 10 );
    DrawText( angleText, cueStickAngleX - wAngleText / 2, cueStickAngleY + cueStickAngleRadius + 5, 10, WHITE );

    DrawLine( 
        cueStickAngleX, 
        cueStickAngleY, 
        cueStickAngleX + cueStickAngleRadius * cosf( DEG2RAD * cueStickAngle ),
        cueStickAngleY + cueStickAngleRadius * sinf( DEG2RAD * cueStickAngle ),
        RAYWHITE
    );

    int powerBarWidth = 16;
    int powerBarHeight = 200;

    int powerBarX = GetScreenWidth() - powerBarWidth - 21;
    int powerBarY = GetScreenHeight() / 2 - powerBarHeight / 2 + 40;

    float powerP = getCueStickPowerPercentage( gw->currentCueStick );
    float powerHeight = ( powerBarHeight - 4 ) * powerP;
    float powerHue = 120.0f - 120.0f * powerP;

    DrawRectangle( powerBarX, powerBarY, powerBarWidth, powerBarHeight, BLACK );
    DrawRectangle( powerBarX + 3, powerBarY + 3, powerBarWidth - 6, powerBarHeight - 6, RAYWHITE );
    DrawRectangle( powerBarX + 3, powerBarY + 3 + powerBarHeight - 4 - powerHeight, powerBarWidth - 6, powerHeight, ColorFromHSV( powerHue, 1.0f, 1.0f ) );

    const char *powerText = TextFormat( "%.2f%%", powerP * 100 );
    int wPowerText = MeasureText( powerText, 10 );
    DrawText( powerText, powerBarX + powerBarWidth / 2 - wPowerText / 2, powerBarY + powerBarHeight + 5, 10, WHITE );

    DrawRectangleRounded( 
        (Rectangle) {
            -50, -50, 330, 90
        },
        0.4f,
        10,
        SCORE_BG_COLOR
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
        SCORE_BG_COLOR
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
            Fade( RAYWHITE, 1.0f * ( highlighCurrentPlayerCounter / highlighCurrentPlayerTime ) )
        );
    } else {
        DrawRectangleRoundedLines( 
            (Rectangle) {
                GetScreenWidth() - 45, 5, 40, 28
            },
            0.4f,
            10,
            Fade( RAYWHITE, 1.0f * ( highlighCurrentPlayerCounter / highlighCurrentPlayerTime ) )
        );
    }

    int spacing = 8;
    int radius = BALL_RADIUS;

    int startScoreP1 = 65;
    int startScoreP2 = 642;

    // TODO: refactor
    for ( int i = 0; i < 7; i++ ) {
        int x = startScoreP1 + ( ( radius + 2 ) * 2 + spacing ) * i;
        int y = 19;
        DrawCircle( x, y, radius + 2, SCORE_POCKET_COLOR );
        DrawCircleLines( x, y, radius + 2, GRAY );
        if ( i < gw->cueStickP1.pocketedCount ) {
            int number = gw->cueStickP1.pocketedBalls[i];
            DrawTexturePro( 
                rm.ballsTexture, 
                (Rectangle) { 64 * number, 0, 64, 64 }, 
                (Rectangle) { x - radius, y - radius, radius * 2, radius * 2 },
                (Vector2) { 0 },
                0.0f,
                WHITE
            );
        }
    }
    
    for ( int i = 0; i < 7; i++ ) {
        int x = startScoreP2 + ( ( radius + 2 ) * 2 + spacing ) * i;
        int y = 19;
        DrawCircle( x, y, radius + 2, SCORE_POCKET_COLOR );
        DrawCircleLines( x, y, radius + 2, GRAY );
        if ( i < gw->cueStickP2.pocketedCount ) {
            int number = gw->cueStickP2.pocketedBalls[i];
            DrawTexturePro( 
                rm.ballsTexture, 
                (Rectangle) { 64 * number, 0, 64, 64 }, 
                (Rectangle) { x - radius, y - radius, radius * 2, radius * 2 },
                (Vector2) { 0 },
                0.0f,
                WHITE
            );
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
    
    for ( int i = 1; i <= BALL_COUNT; i++ ) {
        gw->balls[i] = (Ball) {
            .center = { 0, 0 },
            .prevPos = { 0 },
            .radius = BALL_RADIUS,
            .vel = { 0, 0 },
            .friction = BALL_FRICTION,
            .elasticity = BALL_ELASTICITY,
            .color = colors[i-1],
            .striped = striped[i-1],
            .number = numbers[i-1],
            .pocketed = false
        };
    }

    if ( TEST_BALL_POSITIONING ) {
        performTestBallPositioning( gw->balls, BALL_RADIUS, gw->boundarie );
    } else {
        performDefaultBallPositioning( gw->balls, BALL_RADIUS, gw->boundarie );
    }
    
    int initPower = 400;
    int powerTick = 10;
    int maxPower = 1400;

    gw->cueStickP1 = (CueStick) {
        .target = gw->cueBall->center,
        .distanceFromTarget = BALL_RADIUS,
        .size = 300,
        .angle = 0,
        .powerTick = powerTick,
        .power = initPower,
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
        .power = initPower,
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

    if ( BG_MUSIC_ENABLED ) {
        PlayMusicStream( rm.backgroundMusic );
    }

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

    if ( TEST_BALL_POSITIONING ) {
        for ( int i = 0; i < 15; i++ ) {
            if ( i == 7 ) {
                colors[i] = BLACK;
                numbers[i] = 8;
                striped[i] = false;
            } else if ( i == 0 ) {
                colors[i] = solidColors[0];
                numbers[i] = solidNumbers[0];
                striped[i] = false;
            } else if ( i == 8 ) {
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
    } else {
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

}

static void playBallHitSound( void ) {
    PlaySound( rm.ballHitSounds[(rm.ballHitIndex++) % rm.ballHitCount] );
}

static void playBallCushionHitSound( void ) {
    PlaySound( rm.ballCushionHitSounds[(rm.ballCushionHitIndex++) % rm.ballCushionHitCount] );
}
