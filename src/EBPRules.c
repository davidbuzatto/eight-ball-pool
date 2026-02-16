/**
 * @file Rules.c
 * @author Prof. Dr. David Buzatto
 * @brief 8 Ball Pool (EBP) rules implementation.
 * 
 * @copyright Copyright (c) 2026
 */

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "raylib/raylib.h"

#include "Ball.h"
#include "CommonMacros.h"
#include "CueStick.h"
#include "Cushion.h"
#include "EBPRules.h"
#include "GameWorld.h"
#include "Pocket.h"
#include "ResourceManager.h"
#include "Types.h"

#define TEST_BALL_POSITIONING false

static const Color EBP_YELLOW = { 255, 215, 0,   255 };
static const Color EBP_BLUE   = { 0,   100, 200, 255 };
static const Color EBP_RED    = { 220, 20,  60,  255 };
static const Color EBP_PURPLE = { 75,  0,   130, 255 };
static const Color EBP_ORANGE = { 255, 100, 0,   255 };
static const Color EBP_GREEN  = { 0,   128, 0,   255 };
static const Color EBP_BROWN  = { 139, 69,  19,  255 };

static void applyRulesBreaking( GameWorld *gw );
static void applyRulesOpenTable( GameWorld *gw );
static void applyRulesPlaying( GameWorld *gw );
static void applyRulesBallInHand( GameWorld *gw );
static void isFault( GameWorld *gw );

static void shuffleColorsAndNumbers( Color *colors, int *numbers, int size );
static void prepareBallData( Color *colors, bool *striped, int *numbers, bool suffle );

static int countBallsTouchedCushion( GameWorld *gw );
static void resetStatistics( GameWorld *gw );
static void resetCueBallPosition( GameWorld *gw );

void applyRulesEBP( GameWorld *gw ) {

    trace( "applying rules:" );

    switch ( gw->state ) {
        case GAME_STATE_BREAKING: applyRulesBreaking( gw ); return;
        case GAME_STATE_OPEN_TABLE: applyRulesOpenTable( gw ); return;
        case GAME_STATE_PLAYING: applyRulesPlaying( gw ); return;
        case GAME_STATE_BALL_IN_HAND: applyRulesBallInHand( gw ); return;
        default: return;
    }

}

static void applyRulesBreaking( GameWorld *gw ) {
    
    trace( "  state: breaking" );

    if (
        gw->cueBallHits > 0 &&  
        !gw->cueBallPocketed &&
        ( countBallsTouchedCushion( gw ) >= 4 || gw->ballsPocketed > 0 ) 
    ) {
        trace( "    ok" );
        resetStatistics( gw );
        gw->state = GAME_STATE_OPEN_TABLE;
    } else {
        trace( "    invalid" );
        setupEBP( gw );
    }

}

static void applyRulesOpenTable( GameWorld *gw ) {

    trace( "  state: open table" );

}

static void applyRulesPlaying( GameWorld *gw ) {

    trace( "  state: playing" );

}

static void applyRulesBallInHand( GameWorld *gw ) {

    trace( "  state: ball in hand" );

}

static void isFault( GameWorld *gw ) {

    //Quando ocorre falta, o adversário ganha "bola na mão" (pode posicionar a branca em qualquer lugar da mesa):
    //Não acertar nenhuma bola
    //Acertar primeiro uma bola que não seja do seu grupo
    //Encaçapar a bola branca (scratch)
    //Bola branca pular para fora da mesa
    //Tocar qualquer bola com a mão, roupa ou taco (exceto a branca no momento do tiro)
    //Após o contato, nenhuma bola tocar tabela e nenhuma ser encaçapada
    //Tiro enquanto bolas ainda estão em movimento

    




}

void setupEBP( GameWorld *gw ) {

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

    gw->marksSpacing = gw->boundarie.width / 8;

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
        .spin = { 0, 0 },
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
            .spin = { 0, 0 },
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
        .minPower = 0,
        .maxPower = maxPower,
        .hitPoint = { 0, 0 },
        .color = { 17, 50, 102, 255 },
        .pocketedBalls = { 0 },
        .pocketedCount = 0,
        /*.pocketedBalls = { 1, 2, 3, 4, 5, 6, 7 },
        .pocketedCount = 7,*/
        .type = CUE_STICK_TYPE_P1,
        .state = CUE_STICK_STATE_READY
    };

    gw->cueStickP2 = (CueStick) {
        .target = gw->cueBall->center,
        .distanceFromTarget = BALL_RADIUS,
        .size = 300,
        .angle = 0,
        .powerTick = powerTick,
        .power = initPower,
        .minPower = 0,
        .maxPower = maxPower,
        .hitPoint = { 0, 0 },
        .color = { 102, 17, 37, 255 },
        .pocketedBalls = { 0 },
        .pocketedCount = 0,
        /*.pocketedBalls = { 9, 10, 11, 12, 13, 14, 15 },
        .pocketedCount = 7,*/
        .type = CUE_STICK_TYPE_P2,
        .state = CUE_STICK_STATE_READY
    };

    gw->currentCueStick = &gw->cueStickP1;

    gw->state = GAME_STATE_BREAKING;
    gw->ballsState = GAME_STATE_BALLS_STOPPED;
    gw->pocketedCount = 0;

    resetStatistics( gw );

    gw->changeCurrentPlayer = false;

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

static void prepareBallData( Color *colors, bool *striped, int *numbers, bool suffle ) {

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

    if ( suffle ) {
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

    if ( suffle ) {
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

static int countBallsTouchedCushion( GameWorld *gw ) {

    int cHits = 0 ;

    for ( int i = 0; i < 16; i++ ) {
        if ( gw->ballsTouchedCushion[i] ) {
            cHits++;
        }
    }

    return cHits;

}

static void resetStatistics( GameWorld *gw ) {

    gw->cueBallCushionHits = 0;
    gw->cueBallHits = 0;
    gw->ballCushionHits = 0;
    gw->ballsPocketed = 0;
    gw->cueBallPocketed = false;
    
    memset( gw->ballsTouchedCushion, false, sizeof( gw->ballsTouchedCushion ) );

}

static void resetCueBallPosition( GameWorld *gw ) {
    gw->cueBall->center = (Vector2) { gw->boundarie.x + gw->boundarie.width / 4, GetScreenHeight() / 2 };
    gw->cueBall->pocketed = false;
}
