/**
 * @file ResourceManager.c
 * @author Prof. Dr. David Buzatto
 * @brief ResourceManager implementation.
 * 
 * @copyright Copyright (c) 2026
 */

#include <stdio.h>
#include <stdlib.h>

#include "raylib/raylib.h"

#include "ResourceManager.h"

ResourceManager rm = { 0 };

void loadResourcesResourceManager( void ) {

    rm.ballsTexture = LoadTexture( "resources/images/balls3.png" );
    
    rm.backgroundMusic = LoadMusicStream( "resources/musics/jazz-background-music.mp3" );
    rm.backgroundMusic.looping = true;
    SetMusicVolume( rm.backgroundMusic, 0.3f );

    rm.ballFalling = LoadSound( "resources/sfx/ball-falling.wav" );
    rm.cueBallHit = LoadSound( "resources/sfx/cue-ball-hit.wav" );
    rm.cueStickHit = LoadSound( "resources/sfx/cue-stick-hit.wav" );

    for ( int i = 0; i < BALL_HIT_COUNT; i++ ) {
        rm.ballHits[i] = LoadSound( "resources/sfx/ball-hit.wav" );
    }
    rm.ballHitCount = BALL_HIT_COUNT;
    rm.ballHitIndex = 0;

    for ( int i = 0; i < BALL_CUSHION_HIT_COUNT; i++ ) {
        rm.ballCushionHits[i] = LoadSound( "resources/sfx/ball-cushion-hit.wav" );
        SetSoundVolume( rm.ballCushionHits[i], 0.2f );
    }
    rm.ballCushionHitCount = BALL_CUSHION_HIT_COUNT;
    rm.ballCushionHitIndex = 0;

}

void unloadResourcesResourceManager( void ) {

    UnloadTexture( rm.ballsTexture );
    
    StopMusicStream( rm.backgroundMusic );
    UnloadMusicStream( rm.backgroundMusic );
    
    UnloadSound( rm.ballFalling );
    UnloadSound( rm.cueBallHit );
    UnloadSound( rm.cueStickHit );

    for ( int i = 0; i < rm.ballHitCount; i++ ) {
        UnloadSound( rm.ballHits[i] );
    }

    for ( int i = 0; i < rm.ballCushionHitCount; i++ ) {
        UnloadSound( rm.ballCushionHits[i] );
    }

}