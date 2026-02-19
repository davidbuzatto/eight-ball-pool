/**
 * @file GameWorld.h
 * @author Prof. Dr. David Buzatto
 * @brief GameWorld implementation.
 * 
 * @copyright Copyright (c) 2026
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib/raylib.h"
#include "raylib/raymath.h"
//#define RAYGUI_IMPLEMENTATION    // to use raygui, comment these three lines.
//#include "raylib/raygui.h"       // other compilation units must only include
//#undef RAYGUI_IMPLEMENTATION     // raygui.h

#include "Ball.h"
#include "CommonMacros.h"
#include "CueStick.h"
#include "Cushion.h"
#include "EBPRules.h"
#include "GameWorld.h"
#include "Pocket.h"
#include "ResourceManager.h"
#include "Types.h"

static const Color BG_COLOR = { 28, 38, 58, 255 };
static const Color TABLE_COLOR = { 135, 38, 8, 255 };
static const Color TABLE_POCKETS_BALLS_SUPPORT_COLOR = { 84, 23, 4, 255 };
static const Color SCORE_BG_COLOR = { 14, 18, 33, 255 };
static const Color SCORE_POCKET_COLOR = { 23, 23, 27, 255 };

static Ball *selectedBall = NULL;
static Vector2 pressOffset = { 0 };

static float highlighCurrentPlayerTime = 0.8f;
static float highlighCurrentPlayerCounter = 0.0f;

static bool showHelp = SHOW_HELP;
static bool bgMusicEnabled = BG_MUSIC_ENABLED;

static const char *gameStateNames[] = { 
    "Breaking", 
    "Open Table", 
    "Playing", 
    "Ball In Hand", 
    "Game Over"
};

static void drawHud( GameWorld *gw );
static void drawDebugInfo( GameWorld *gw );
static void drawGameOver( GameWorld *gw );
static void drawHelp( void );
static void drawTrajectory( GameWorld *gw );
static void playBallHitSound( void );
static void playBallCushionHitSound( void );

/**
 * @brief Creates a dinamically allocated GameWorld struct instance.
 */
GameWorld* createGameWorld( void ) {

    GameWorld *gw = (GameWorld*) malloc( sizeof( GameWorld ) );
    
    setupEBP( gw );
    if ( BG_MUSIC_ENABLED ) {
        PlayMusicStream( rm.backgroundMusic );
    }

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

    if ( bgMusicEnabled ) {
        UpdateMusicStream( rm.backgroundMusic );
    }

    if ( IsKeyPressed( KEY_F2 ) ) {
        showHelp = !showHelp;
    }

    if ( showHelp ) {
        return;
    }

    if ( IsKeyPressed( KEY_R ) ) {
        setupEBP( gw );
        return;
    }

    if ( IsKeyPressed( KEY_M ) ) {
        StopMusicStream( rm.backgroundMusic );
        bgMusicEnabled = !bgMusicEnabled;
        if ( bgMusicEnabled ) {
            PlayMusicStream( rm.backgroundMusic );
        }
    }

    if ( IsKeyPressed( KEY_S ) ) {
        for ( int i = 0; i <= BALL_COUNT; i++ ) {
            gw->balls[i].vel.x = 0;
            gw->balls[i].vel.y = 0;
        }
    }

    // prev positions here (needed for cushion collision)
    for ( int i = 0; i <= BALL_COUNT; i++ ) {
        gw->balls[i].prevPos = gw->balls[i].center;
    }

    if ( gw->ballsState == GAME_STATE_BALLS_STOPPED ) {

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
            gw->currentCueStick->state = CUE_STICK_STATE_HITING;
        }

        updateCueStick( gw->currentCueStick, delta );

        if ( gw->currentCueStick->state == CUE_STICK_STATE_HIT ) {

            if ( gw->currentCueStick->power != 0 ) {
                PlaySound( rm.cueStickHitSound );
            }

            CueStick *cc = gw->currentCueStick;
            gw->cueBall->vel.x = cc->power * cosf( DEG2RAD * cc->angle );
            gw->cueBall->vel.y = cc->power * sinf( DEG2RAD * cc->angle );

            // applies the spin based on the point of impact
            gw->cueBall->spin.x = cc->hitPoint.x * 2.0f; // side spin
            gw->cueBall->spin.y = cc->hitPoint.y * 2.0f; // top/back spin

            cc->state = CUE_STICK_STATE_READY;
            gw->applyRules = true;

        }

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

                // spin on reflection
                if ( b == gw->cueBall && Vector2Length( b->spin ) > 0.01f ) {

                    bool isVertical = fabs( collision.normal.x ) > fabs( collision.normal.y );

                    if ( isVertical ) {
                        // vertical cushion, spin.x affects angle
                        float spinEffect = b->spin.x * 0.3f; // influence factor
                        b->vel.y += spinEffect * fabs( b->vel.x );
                    } else {
                        // horizontal cushion, spin.y affects angle
                        float spinEffect = b->spin.y * 0.3f; // influence factor
                        b->vel.x += spinEffect * fabs( b->vel.y );
                    }

                    // decrease spin after collision
                    b->spin = Vector2Scale( b->spin, 0.7f );

                }

                // applies elasticity
                b->vel = Vector2Scale( b->vel, b->elasticity );

                // apply some offset to prevent continuous collision
                b->center = Vector2Add( b->center, Vector2Scale( collision.normal, 0.1f ) );

                if ( gw->statistics.cueBallHits > 0 || gw->state != GAME_STATE_BREAKING ) {
                    gw->statistics.ballsTouchedCushion[b->number] = true;
                }

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
                    if ( b == gw->cueBall ) {
                        if ( gw->statistics.cueBallHits == 0 ) {
                            gw->statistics.cueBallFirstHitNumber = bt->number;
                        }
                        gw->statistics.cueBallHits++;
                    }
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

                if ( b == gw->cueBall ) {
                    gw->statistics.cueBallPocketed = true;
                    resetCueBallPosition( gw );
                } else {

                    if ( gw->state != GAME_STATE_BREAKING ) {

                        if ( gw->currentCueStick->group == BALL_GROUP_UNDEFINED ) {
                            if ( gw->currentCueStick == &gw->cueStickP1 ) {
                                gw->cueStickP1.pocketedBalls[gw->cueStickP1.pocketedCount++] = b->number;
                            } else {
                                gw->cueStickP2.pocketedBalls[gw->cueStickP2.pocketedCount++] = b->number;
                            }
                        } else if ( gw->currentCueStick->group == BALL_GROUP_SOLID ) {
                            if ( gw->currentCueStick == &gw->cueStickP1 ) {
                                if ( b->number < 8 ) {
                                    gw->cueStickP1.pocketedBalls[gw->cueStickP1.pocketedCount++] = b->number;
                                } else if ( b->number > 8 ) {
                                    gw->cueStickP2.pocketedBalls[gw->cueStickP2.pocketedCount++] = b->number;
                                }
                            } else {
                                if ( b->number < 8 ) {
                                    gw->cueStickP2.pocketedBalls[gw->cueStickP2.pocketedCount++] = b->number;
                                } else if ( b->number > 8 ) {
                                    gw->cueStickP1.pocketedBalls[gw->cueStickP1.pocketedCount++] = b->number;
                                }
                            }
                        } else if ( gw->currentCueStick->group == BALL_GROUP_STRIPED ) {
                            if ( gw->currentCueStick == &gw->cueStickP1 ) {
                                if ( b->number > 8 ) {
                                    gw->cueStickP1.pocketedBalls[gw->cueStickP1.pocketedCount++] = b->number;
                                } else if ( b->number < 8 ) {
                                    gw->cueStickP2.pocketedBalls[gw->cueStickP2.pocketedCount++] = b->number;
                                }
                            } else {
                                if ( b->number > 8 ) {
                                    gw->cueStickP2.pocketedBalls[gw->cueStickP2.pocketedCount++] = b->number;
                                } else if ( b->number < 8 ) {
                                    gw->cueStickP1.pocketedBalls[gw->cueStickP1.pocketedCount++] = b->number;
                                }
                            }
                        }

                    }

                    gw->statistics.pocketedBalls[gw->statistics.pocketedCount++] = b->number;
                    gw->pocketedBalls[gw->pocketedCount++] = b->number;

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
        gw->ballsState = GAME_STATE_BALLS_MOVING;
    } else {

        gw->ballsState = GAME_STATE_BALLS_STOPPED;

        if ( gw->applyRules ) {

            gw->lastCueStick = gw->currentCueStick;

            if ( gw->currentCueStick == &gw->cueStickP1 ) {
                gw->currentCueStick = &gw->cueStickP2;
            } else {
                gw->currentCueStick = &gw->cueStickP1;
            }

            applyRulesEBP( gw );

            gw->applyRules = false;

        }

    }

    highlighCurrentPlayerCounter += delta;
    if ( highlighCurrentPlayerCounter > highlighCurrentPlayerTime ) {
        highlighCurrentPlayerCounter = 0.0f;
    }

}

/**
 * @brief Draws the state of the game.
 */
void drawGameWorld( GameWorld *gw ) {

    BeginDrawing();
    ClearBackground( BG_COLOR );

    int pocketedBallsSupportWidth = BALL_RADIUS * 16 * 2;

    DrawRectangleRounded( 
        (Rectangle) {
            gw->boundarie.x + gw->boundarie.width / 2 - pocketedBallsSupportWidth / 2,
            gw->boundarie.y + TABLE_MARGIN + gw->boundarie.height - 20,
            pocketedBallsSupportWidth,
            60
        },
        0.5f,
        10,
        TABLE_POCKETS_BALLS_SUPPORT_COLOR
    );

    DrawRectangleRoundedLines( 
        (Rectangle) {
            gw->boundarie.x + gw->boundarie.width / 2 - pocketedBallsSupportWidth / 2,
            gw->boundarie.y + TABLE_MARGIN + gw->boundarie.height - 20,
            pocketedBallsSupportWidth,
            60
        },
        0.5f,
        10,
        BLACK
    );

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
                gw->boundarie.x + gw->marksSpacing * i, 
                gw->boundarie.y - TABLE_MARGIN / 3 * 2, 
                3, WHITE
            );
            DrawCircle( 
                gw->boundarie.x + gw->marksSpacing * i, 
                gw->boundarie.y + gw->boundarie.height + TABLE_MARGIN / 3 * 2,
                3, WHITE
            );
        }
    }

    // bottom marks
    for ( int i = 1; i <= 3; i++ ) {
        DrawCircle( 
            gw->boundarie.x - TABLE_MARGIN / 3 * 2, 
            gw->boundarie.y + gw->marksSpacing * i, 
            3, WHITE
        );
        DrawCircle( 
            gw->boundarie.x + gw->boundarie.width + TABLE_MARGIN / 3 * 2, 
            gw->boundarie.y + gw->marksSpacing * i, 
            3, WHITE
        );
    }

    DrawLine( 
        gw->boundarie.x + gw->marksSpacing * 2, 
        gw->boundarie.y,
        gw->boundarie.x + gw->marksSpacing * 2, 
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

    if ( gw->ballsState == GAME_STATE_BALLS_STOPPED && selectedBall == NULL ) {
        drawCueStick( gw->currentCueStick );
    }

    if ( gw->ballsState == GAME_STATE_BALLS_STOPPED && selectedBall == NULL ) {
        drawTrajectory( gw );
        drawCueStick( gw->currentCueStick );
    }

    drawHud( gw );

    if ( gw->state == GAME_STATE_GAME_OVER ) {
        drawGameOver( gw );
    }

    if ( showHelp ) {
        drawHelp();
    }

    if ( SHOW_DEBUG_INFO ) {
        drawDebugInfo( gw );
    }

    EndDrawing();

}

static void drawHud( GameWorld *gw ) {
    
    int cueStickAngleX = GetScreenWidth() - 29;
    int cueStickAngleY = 105;
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

    DrawRing( 
        (Vector2) { 
            cueStickAngleX, 
            cueStickAngleY
        },
        5,
        cueStickAngleRadius / 1.5,
        -cueStickAngleAntiClock,
        0,
        30,
        gw->currentCueStick->color
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


    int cueStickHitPointX = GetScreenWidth() - 29;
    int cueStickHitPointY = 175;
    int cueStickHitPointRadius = 21;

    DrawCircle( cueStickHitPointX, cueStickHitPointY, cueStickHitPointRadius, RAYWHITE );
    DrawCircleLines( cueStickHitPointX, cueStickHitPointY, cueStickHitPointRadius, BLACK );

    float hitPointDistance = sqrtf( 
        gw->currentCueStick->hitPoint.x * gw->currentCueStick->hitPoint.x +
        gw->currentCueStick->hitPoint.y * gw->currentCueStick->hitPoint.y
    );

    // normalize if the distance if greater than 1
    float nX = gw->currentCueStick->hitPoint.x;
    float nY = gw->currentCueStick->hitPoint.y;

    if ( hitPointDistance > 1.0f ) {
        nX /= hitPointDistance;
        nY /= hitPointDistance;
    }

    // mapping to the circle with some margin
    float maxRadius = cueStickHitPointRadius * 0.85f; // margin of 10%

    Vector2 hitPointP = {
        cueStickHitPointX + nX * maxRadius,
        cueStickHitPointY + nY * maxRadius
    };

    DrawCircleV( hitPointP, 4, RED );

    DrawLine( 
        cueStickHitPointX - cueStickAngleRadius, 
        cueStickHitPointY, 
        cueStickHitPointX + cueStickAngleRadius,
        cueStickHitPointY, 
        Fade( BLACK, 0.2f )
    );

    DrawLine( 
        cueStickHitPointX, 
        cueStickHitPointY - cueStickAngleRadius, 
        cueStickHitPointX, 
        cueStickHitPointY + cueStickAngleRadius,
        Fade( BLACK, 0.2f )
    );

    DrawCircleLines( cueStickHitPointX, cueStickHitPointY, cueStickHitPointRadius / 2 + 1, Fade( BLACK, 0.2f ) );



    int powerBarWidth = 16;
    int powerBarHeight = 200;

    int powerBarX = GetScreenWidth() - powerBarWidth - 21;
    int powerBarY = GetScreenHeight() / 2 - powerBarHeight / 2 + 35;

    float powerP = getCueStickPowerPercentage( gw->currentCueStick );
    int powerHeight = (int) ( ( powerBarHeight - 6 ) * powerP );
    float powerHue = 120.0f - 120.0f * powerP;

    DrawRectangle( powerBarX, powerBarY, powerBarWidth, powerBarHeight, BLACK );
    DrawRectangle( powerBarX + 3, powerBarY + 3, powerBarWidth - 6, powerBarHeight - 6, RAYWHITE );
    DrawRectangle( powerBarX + 3, powerBarY + 3 + powerBarHeight - 6 - powerHeight, powerBarWidth - 6, powerHeight, ColorFromHSV( powerHue, 1.0f, 1.0f ) );

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

    // TODO: refactor?
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

    int startPocketed = GetScreenWidth() / 2 - radius * 14;

    for ( int i = 0; i < 15; i++ ) {
        int x = startPocketed + ( radius * 2 ) * i;
        int y = gw->boundarie.y + gw->boundarie.height + TABLE_MARGIN + radius * 2;
        DrawCircle( x, y, radius, ColorBrightness( TABLE_POCKETS_BALLS_SUPPORT_COLOR, -0.5f ) );
        DrawCircleLines( x, y, radius, BLACK );
        if ( i < gw->pocketedCount ) {
            int number = gw->pocketedBalls[i];
            DrawTexturePro( 
                rm.ballsTexture, 
                (Rectangle) { 64 * number, 0, 64, 64 }, 
                (Rectangle) { x - radius, y - radius, radius * 2, radius * 2 },
                (Vector2) { 0 },
                0.0f,
                WHITE
            );
            DrawCircleLines( x, y, radius, BLACK );
        }
    }

    int fs = 30;
    int w = MeasureText( gameStateNames[gw->state], fs );

    DrawRectangleRounded( 
        (Rectangle) {
            GetScreenWidth() / 2 - w / 2 - 15, 10, w + 30, 40
        },
        0.4f,
        10,
        gw->currentCueStick->color
    );

    DrawRectangleRoundedLines( 
        (Rectangle) {
            GetScreenWidth() / 2 - w / 2 - 15, 10, w + 30, 40
        },
        0.4f,
        10,
        RAYWHITE
    );

    DrawText( gameStateNames[gw->state], GetScreenWidth() / 2 - w / 2 + 3, 18, fs, BLACK );
    DrawText( gameStateNames[gw->state], GetScreenWidth() / 2 - w / 2, 15, fs, RAYWHITE );

    int musicIconS = bgMusicEnabled ? 0 : 64;

    DrawTexturePro( 
        rm.musicIconsTexture, 
        (Rectangle) { musicIconS, 0, 64, 64 }, 
        (Rectangle) { GetScreenWidth() - 46, GetScreenHeight() - 110, 32, 32 }, 
        (Vector2) { 0 }, 
        0.0f,
        Fade( WHITE, 0.5f )
    );

}

static void drawDebugInfo( GameWorld *gw ) {

    int y = GetScreenHeight() - 200;
    
    DrawRectangle( 0, y, 300, 200, Fade( WHITE, 0.5f ) );
    DrawText( TextFormat( "cue x ball hits: %d", gw->statistics.cueBallHits ), 5, y + 5, 20, BLACK );
    DrawText( TextFormat( "cue first hit number: %d", gw->statistics.cueBallFirstHitNumber ), 5, y + 25, 20, BLACK );
    DrawText( TextFormat( "cue pocketed: %s", gw->statistics.cueBallPocketed ? "yes" : "no"  ), 5, y + 45, 20, BLACK );
    DrawText( TextFormat( "balls touched cushion:", gw->statistics.pocketedCount ), 5, y + 65, 20, BLACK );

    for ( int i = 0; i < 16; i++ ) {
        DrawText( TextFormat( "%s", gw->statistics.ballsTouchedCushion[i] ? "y" : "n" ), 15 + 15 * i, y + 85, 20, BLACK );
    }

    DrawText( TextFormat( "balls pocketed: %d", gw->statistics.pocketedCount ), 5, y + 105, 20, BLACK );

    int xStart = 15;
    for ( int i = 0; i < gw->statistics.pocketedCount; i++ ) {
        const char *t = TextFormat( "%d", gw->statistics.pocketedBalls[i] );
        int w = MeasureText( t, 20 );
        DrawText( t, xStart, y + 125, 20, BLACK );
        xStart += w + 10;
    }

    DrawText( TextFormat( "group: %d", gw->cueStickP1.group ), 20, 50, 20, WHITE );
    DrawText( TextFormat( "group: %d", gw->cueStickP2.group ), 800, 50, 20, WHITE );

}

static void drawGameOver( GameWorld *gw ) {

    DrawRectangle( 0, 0, GetScreenWidth(), GetScreenHeight(), Fade( BLACK, 0.7f ) );
    
    int boxWidth = 400;
    int boxHeight = 200;
    int boxX = GetScreenWidth() / 2 - boxWidth / 2;
    int boxY = GetScreenHeight() / 2 - boxHeight / 2;

    DrawRectangleRounded( 
        (Rectangle) { boxX, boxY, boxWidth, boxHeight },
        0.2f,
        10,
        gw->winnerCueStick->color
    );

    DrawRectangleRoundedLines( 
        (Rectangle) { boxX, boxY, boxWidth, boxHeight },
        0.2f,
        10,
        RAYWHITE
    );

    const char *title = "GAME OVER";
    int titleSize = 40;
    int titleWidth = MeasureText( title, titleSize );

    DrawText( 
        title, 
        GetScreenWidth() / 2 - titleWidth / 2 + 2, 
        boxY + 32, 
        titleSize, 
        BLACK 
    );

    DrawText( 
        title, 
        GetScreenWidth() / 2 - titleWidth / 2, 
        boxY + 30, 
        titleSize, 
        RAYWHITE 
    );

    const char *winnerText = gw->winnerCueStick == &gw->cueStickP1 ? "Player 1 Wins!" : "Player 2 Wins!";
    int winnerSize = 30;
    int winnerWidth = MeasureText( winnerText, winnerSize );

    DrawText( 
        winnerText, 
        GetScreenWidth() / 2 - winnerWidth / 2 + 2, 
        boxY + 92, 
        winnerSize, 
        BLACK 
    );

    DrawText( 
        winnerText, 
        GetScreenWidth() / 2 - winnerWidth / 2, 
        boxY + 90, 
        winnerSize, 
        GOLD 
    );

    const char *restartText = "Press R to restart";
    int restartSize = 20;
    int restartWidth = MeasureText( restartText, restartSize );
    DrawText( 
        restartText, 
        GetScreenWidth() / 2 - restartWidth / 2, 
        boxY + 150, 
        restartSize, 
        Fade( RAYWHITE, 0.7f ) 
    );

}

static void drawHelp( void ) {

    DrawRectangle( 0, 0, GetScreenWidth(), GetScreenHeight(), Fade( BLACK, 0.85f ) );

    int boxWidth = 500;
    int boxHeight = 510;
    int boxX = GetScreenWidth() / 2 - boxWidth / 2;
    int boxY = GetScreenHeight() / 2 - boxHeight / 2;

    DrawRectangleRounded( 
        (Rectangle) { boxX, boxY, boxWidth, boxHeight },
        0.1f,
        10,
        (Color) { 20, 25, 35, 255 }
    );

    DrawRectangleRoundedLines( 
        (Rectangle) { boxX, boxY, boxWidth, boxHeight },
        0.1f,
        10,
        RAYWHITE
    );

    int currentY = boxY + 15;
    int leftMargin = boxX + 25;
    int lineHeight = 22;

    const char *title = "HELP - CONTROLS & RULES";
    int titleSize = 24;
    int titleWidth = MeasureText( title, titleSize );
    DrawText( 
        title, 
        GetScreenWidth() / 2 - titleWidth / 2 + 2, 
        currentY + 2, 
        titleSize, 
        BLACK 
    );
    DrawText( 
        title, 
        GetScreenWidth() / 2 - titleWidth / 2, 
        currentY, 
        titleSize, 
        GOLD 
    );

    currentY += 35;

    DrawLineEx( 
        (Vector2) { boxX + 20, currentY }, 
        (Vector2) { boxX + boxWidth - 20, currentY }, 
        2, 
        Fade( RAYWHITE, 0.3f ) 
    );

    currentY += 12;

    DrawText( "CONTROLS:", leftMargin, currentY, 18, SKYBLUE );
    currentY += lineHeight;

    DrawText( "Mouse Movement", leftMargin + 15, currentY, 14, RAYWHITE );
    DrawText( "Aim cue stick", leftMargin + 220, currentY, 14, GRAY );
    currentY += lineHeight;

    DrawText( "Mouse Wheel", leftMargin + 15, currentY, 14, RAYWHITE );
    DrawText( "Adjust shot power", leftMargin + 220, currentY, 14, GRAY );
    currentY += lineHeight;

    DrawText( "Left Click", leftMargin + 15, currentY, 14, RAYWHITE );
    DrawText( "Shoot", leftMargin + 220, currentY, 14, GRAY );
    currentY += lineHeight;

    DrawText( "Right Click + Drag", leftMargin + 15, currentY, 14, RAYWHITE );
    DrawText( "Move balls (free positioning)", leftMargin + 220, currentY, 14, GRAY );
    currentY += lineHeight;

    DrawText( "Arrow Keys", leftMargin + 15, currentY, 14, RAYWHITE );
    DrawText( "Adjust hit point (spin)", leftMargin + 220, currentY, 14, GRAY );
    currentY += lineHeight;

    DrawText( "Space", leftMargin + 15, currentY, 14, RAYWHITE );
    DrawText( "Reset hit point to center", leftMargin + 220, currentY, 14, GRAY );
    currentY += lineHeight;

    DrawText( "R", leftMargin + 15, currentY, 14, RAYWHITE );
    DrawText( "Restart game", leftMargin + 220, currentY, 14, GRAY );
    currentY += lineHeight;

    DrawText( "M", leftMargin + 15, currentY, 14, RAYWHITE );
    DrawText( "Toggle background music", leftMargin + 220, currentY, 14, GRAY );
    currentY += lineHeight;

    DrawText( "S", leftMargin + 15, currentY, 14, RAYWHITE );
    DrawText( "Stop all balls immediately", leftMargin + 220, currentY, 14, GRAY );
    currentY += lineHeight;

    DrawText( "F2", leftMargin + 15, currentY, 14, RAYWHITE );
    DrawText( "Toggle this help screen", leftMargin + 220, currentY, 14, GRAY );
    currentY += lineHeight + 8;

    DrawLineEx( 
        (Vector2) { boxX + 20, currentY }, 
        (Vector2) { boxX + boxWidth - 20, currentY }, 
        2, 
        Fade( RAYWHITE, 0.3f ) 
    );

    currentY += 12;

    // Rules section
    DrawText( "8 BALL POOL RULES:", leftMargin, currentY, 18, SKYBLUE );
    currentY += lineHeight;

    DrawText( "- First player to pocket all their balls (solid or striped) and then", leftMargin + 15, currentY, 13, RAYWHITE );
    currentY += 18;
    DrawText( "  legally pocket the 8-ball wins;", leftMargin + 15, currentY, 13, RAYWHITE );
    currentY += 18;

    DrawText( "- Groups are assigned after the break based on first ball pocketed;", leftMargin + 15, currentY, 13, RAYWHITE );
    currentY += 18;

    DrawText( "- You must hit your group first, or it's a foul (ball in hand);", leftMargin + 15, currentY, 13, RAYWHITE );
    currentY += 18;

    DrawText( "- Pocketing the 8-ball before clearing your group = instant loss;", leftMargin + 15, currentY, 13, RAYWHITE );
    currentY += 18;

    DrawText( "- Continue playing if you pocket a ball legally.", leftMargin + 15, currentY, 13, RAYWHITE );
    currentY += 22;

    DrawLineEx( 
        (Vector2) { boxX + 20, currentY }, 
        (Vector2) { boxX + boxWidth - 20, currentY }, 
        2, 
        Fade( RAYWHITE, 0.3f ) 
    );

    currentY += 8;

    const char *author = "Author: Prof. Dr. David Buzatto";
    int authorSize = 14;
    int authorWidth = MeasureText( author, authorSize );
    DrawText( 
        author, 
        GetScreenWidth() / 2 - authorWidth / 2, 
        currentY, 
        authorSize, 
        Fade( RAYWHITE, 0.7f ) 
    );

    currentY += 20;

    const char *closeText = "Press F2 to close";
    int closeSize = 16;
    int closeWidth = MeasureText( closeText, closeSize );
    DrawText( 
        closeText, 
        GetScreenWidth() / 2 - closeWidth / 2 + 1, 
        currentY + 1, 
        closeSize, 
        BLACK 
    );
    DrawText( 
        closeText, 
        GetScreenWidth() / 2 - closeWidth / 2, 
        currentY, 
        closeSize, 
        GOLD 
    );

}

// calculate the predicted trajectory (better)
static TrajectoryPrediction calculateTrajectory( GameWorld *gw ) {

    TrajectoryPrediction pred = { 0 };

    CueStick *cs = gw->currentCueStick;

    // shot direction
    float angle = DEG2RAD * cs->angle;
    Vector2 direction = { cosf( angle ), sinf( angle ) };

    // starting point
    Vector2 rayStart = gw->cueBall->center;

    // maximum ray distance (crosses the entire table)
    float maxDistance = 2000.0f;

    float closestDistance = maxDistance;
    int closestBallIndex = -1;
    Vector2 closestHitPoint = { 0 };
    Vector2 closestCollisionPoint = { 0 };

    // check collision with each ball
    for ( int i = 1; i <= BALL_COUNT; i++ ) {

        Ball *b = &gw->balls[i];

        if ( b->pocketed ) {
            continue;
        }

        // vector from cue ball center to target ball center
        Vector2 toTarget = Vector2Subtract( b->center, rayStart );

        // projection of vector onto ray direction
        float projection = Vector2DotProduct( toTarget, direction );

        // if projection is negative, ball is behind
        if ( projection <= 0 ) {
            continue;
        }

        // closest point on the ray to the ball center
        Vector2 closestPointOnRay = Vector2Add( rayStart, Vector2Scale( direction, projection ) );

        // distance from ball center to the ray
        float distanceToRay = Vector2Distance( b->center, closestPointOnRay );

        // sum of radii (collision threshold)
        float sumRadii = gw->cueBall->radius + b->radius;

        // if distance is less than sum of radii, there's a collision
        if ( distanceToRay <= sumRadii ) {

            // use Pythagorean theorem to find exact collision distance
            // we need to find where the cue ball's edge first touches the target ball's edge
            float distanceSquared = sumRadii * sumRadii - distanceToRay * distanceToRay;

            if ( distanceSquared < 0 ) {
                distanceSquared = 0; // numerical safety
            }

            float offsetDistance = sqrtf( distanceSquared );
            float collisionDistance = projection - offsetDistance;

            // only consider forward collisions
            if ( collisionDistance > 0.01f && collisionDistance < closestDistance ) {
                closestDistance = collisionDistance;
                closestBallIndex = i;

                // exact collision point (center of cue ball at moment of impact)
                closestCollisionPoint = Vector2Add( rayStart, Vector2Scale( direction, collisionDistance ) );

                // contact point on target ball surface
                // this is the point where the two balls touch
                Vector2 centerToCenterDir = Vector2Normalize( Vector2Subtract( b->center, closestCollisionPoint ) );
                closestHitPoint = Vector2Subtract( b->center, Vector2Scale( centerToCenterDir, b->radius ) );

            }

        }

    }

    if ( closestBallIndex != -1 ) {

        pred.willHitBall = true;
        pred.ballIndex = closestBallIndex;
        pred.hitPoint = closestHitPoint;
        pred.cueBallStopPoint = closestCollisionPoint;

        // direction target ball will take
        // this is the line from cue ball center to target ball center at impact
        Ball *targetBall = &gw->balls[closestBallIndex];
        Vector2 impactDirection = Vector2Normalize( 
            Vector2Subtract( targetBall->center, closestCollisionPoint ) 
        );

        pred.targetBallDirection = impactDirection;

        // estimated speed (based on power)
        float powerPercent = getCueStickPowerPercentage( cs );
        pred.targetBallSpeed = powerPercent * 300.0f; // adjust as needed

    } else {

        // won't hit any ball, trajectory goes to the end
        pred.willHitBall = false;
        pred.cueBallStopPoint = Vector2Add( rayStart, Vector2Scale( direction, maxDistance ) );

    }

    return pred;

}

// calculate the predicted trajectory
__attribute__((unused)) static TrajectoryPrediction calculateTrajectoryOld( GameWorld *gw ) {

    TrajectoryPrediction pred = { 0 };

    CueStick *cs = gw->currentCueStick;

    // shot direction
    float angle = DEG2RAD * cs->angle;
    Vector2 direction = { cosf( angle ), sinf( angle ) };

    // starting point
    Vector2 rayStart = gw->cueBall->center;

    // maximum ray distance (crosses the entire table)
    float maxDistance = 2000.0f;

    float closestDistance = maxDistance;
    int closestBallIndex = -1;
    Vector2 closestHitPoint = { 0 };

    // check collision with each ball
    for ( int i = 1; i <= BALL_COUNT; i++ ) {

        Ball *b = &gw->balls[i];

        if ( b->pocketed ) {
            continue;
        }

        // vector from cue ball center to target ball center
        Vector2 toTarget = Vector2Subtract( b->center, rayStart );

        // projection of vector onto ray
        float projection = Vector2DotProduct( toTarget, direction );

        // if projection is negative, ball is behind
        if ( projection < 0 ) {
            continue;
        }

        // closest point on ray
        Vector2 closestPoint = Vector2Add( rayStart, Vector2Scale( direction, projection ) );

        // distance from ball center to closest point
        float distanceToRay = Vector2Distance( b->center, closestPoint );

        // sum of radii
        float sumRadii = gw->cueBall->radius + b->radius;

        // if distance is less than sum of radii, there's a collision
        if ( distanceToRay < sumRadii ) {

            // calculate distance to collision point
            float offsetDistance = sqrtf( sumRadii * sumRadii - distanceToRay * distanceToRay );
            float collisionDistance = projection - offsetDistance;

            if ( collisionDistance > 0 && collisionDistance < closestDistance ) {

                closestDistance = collisionDistance;
                closestBallIndex = i;

                // collision point
                Vector2 collisionPoint = Vector2Add( rayStart, Vector2Scale( direction, collisionDistance ) );

                // contact point on target ball surface
                Vector2 toBall = Vector2Normalize( Vector2Subtract( b->center, collisionPoint ) );
                closestHitPoint = Vector2Add( b->center, Vector2Scale( toBall, -b->radius ) );

            }

        }

    }

    if ( closestBallIndex != -1 ) {

        pred.willHitBall = true;
        pred.ballIndex = closestBallIndex;
        pred.hitPoint = closestHitPoint;

        // point where cue ball stops (approximate)
        pred.cueBallStopPoint = Vector2Add( rayStart, Vector2Scale( direction, closestDistance ) );

        // direction target ball will take
        Ball *targetBall = &gw->balls[closestBallIndex];
        Vector2 impactDirection = Vector2Normalize( 
            Vector2Subtract( targetBall->center, pred.cueBallStopPoint ) 
        );

        pred.targetBallDirection = impactDirection;

        // estimated speed (based on power)
        float powerPercent = getCueStickPowerPercentage( cs );
        pred.targetBallSpeed = powerPercent * 300.0f; // todo: adjust if needed

    } else {

        // won't hit any ball, trajectory goes to the end
        pred.willHitBall = false;
        pred.cueBallStopPoint = Vector2Add( rayStart, Vector2Scale( direction, maxDistance ) );

    }

    return pred;

}

// draw the predicted trajectory
static void drawTrajectory( GameWorld *gw ) {

    //TrajectoryPrediction pred = calculateTrajectoryOld( gw );
    TrajectoryPrediction pred = calculateTrajectory( gw );
    Vector2 rayStart = gw->cueBall->center;

    if ( pred.willHitBall ) {

        DrawTexturePro( 
            rm.ballsTexture, 
            (Rectangle) { 0, 0, 64, 64 }, 
            (Rectangle) { pred.cueBallStopPoint.x - gw->cueBall->radius, pred.cueBallStopPoint.y - gw->cueBall->radius, gw->cueBall->radius * 2, gw->cueBall->radius * 2 },
            (Vector2) { 0 },
            0.0f,
            Fade( WHITE, 0.3f ) 
        );

        DrawCircleLines( 
            pred.cueBallStopPoint.x, 
            pred.cueBallStopPoint.y, 
            gw->cueBall->radius, 
            Fade( BLACK, 0.3f ) 
        );

        // line from cue ball to impact point
        DrawLineEx( 
            rayStart, 
            pred.cueBallStopPoint, 
            2.0f, 
            Fade( WHITE, 0.6f ) 
        );

        // dashed line to indicate continuation
        Vector2 direction = Vector2Normalize( Vector2Subtract( pred.cueBallStopPoint, rayStart ) );
        Vector2 extendedPoint = Vector2Add( pred.cueBallStopPoint, Vector2Scale( direction, 50.0f ) );

        // draw dashed line
        float dashLength = 5.0f;
        float gapLength = 5.0f;
        Vector2 dashDir = Vector2Normalize( Vector2Subtract( extendedPoint, pred.cueBallStopPoint ) );
        float totalLength = Vector2Distance( pred.cueBallStopPoint, extendedPoint );

        for ( float d = 0; d < totalLength; d += dashLength + gapLength ) {
            Vector2 start = Vector2Add( pred.cueBallStopPoint, Vector2Scale( dashDir, d ) );
            Vector2 end = Vector2Add( start, Vector2Scale( dashDir, dashLength ) );
            DrawLineEx( start, end, 2.0f, Fade( WHITE, 0.4f ) );
        }

        // impact point on target ball
        Ball *targetBall = &gw->balls[pred.ballIndex];

        // circle at contact point
        DrawCircleV( pred.hitPoint, 4.0f, WHITE );
        DrawCircleV( pred.hitPoint, 6.0f, Fade( WHITE, 0.3f ) );

        // highlight on ball that will be hit
        DrawCircleLines( targetBall->center.x, targetBall->center.y, targetBall->radius + 3, Fade( WHITE, 0.5f ) );
        DrawCircleLines( targetBall->center.x, targetBall->center.y, targetBall->radius + 5, Fade( WHITE, 0.3f ) );

        // predicted trajectory of hit ball
        Vector2 targetEndPoint = Vector2Add( 
            targetBall->center, 
            Vector2Scale( pred.targetBallDirection, pred.targetBallSpeed ) 
        );

        // target ball trajectory line
        DrawLineEx( 
            targetBall->center, 
            targetEndPoint, 
            2.0f, 
            Fade( WHITE, 0.5f ) 
        );

        // arrow at the tip
        float arrowSize = 8.0f;
        Vector2 arrowDir = Vector2Normalize( pred.targetBallDirection );
        Vector2 arrowPerp = { -arrowDir.y, arrowDir.x };

        Vector2 arrowTip = targetEndPoint;
        Vector2 arrowLeft = Vector2Add( 
            Vector2Add( arrowTip, Vector2Scale( arrowDir, -arrowSize ) ),
            Vector2Scale( arrowPerp, arrowSize / 2 )
        );
        Vector2 arrowRight = Vector2Add( 
            Vector2Add( arrowTip, Vector2Scale( arrowDir, -arrowSize ) ),
            Vector2Scale( arrowPerp, -arrowSize / 2 )
        );

        DrawTriangle( arrowTip, arrowRight, arrowLeft, Fade( WHITE, 0.5f ) );

    } else {

        // won't hit any ball, just draw the line
        DrawLineEx( 
            rayStart, 
            pred.cueBallStopPoint, 
            2.0f, 
            Fade( WHITE, 0.4f ) 
        );

    }

}

static void playBallHitSound( void ) {
    PlaySound( rm.ballHitSounds[(rm.ballHitIndex++) % rm.ballHitCount] );
}

static void playBallCushionHitSound( void ) {
    PlaySound( rm.ballCushionHitSounds[(rm.ballCushionHitIndex++) % rm.ballCushionHitCount] );
}
