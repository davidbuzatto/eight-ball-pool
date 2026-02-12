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

    rm.ballFallingSound = LoadSound( "resources/sfx/ball-falling.wav" );
    rm.cueBallHitSound = LoadSound( "resources/sfx/cue-ball-hit.wav" );
    rm.cueStickHitSound = LoadSound( "resources/sfx/cue-stick-hit.wav" );

    for ( int i = 0; i < BALL_HIT_COUNT; i++ ) {
        rm.ballHitSounds[i] = LoadSound( "resources/sfx/ball-hit.wav" );
    }
    rm.ballHitCount = BALL_HIT_COUNT;
    rm.ballHitIndex = 0;

    for ( int i = 0; i < BALL_CUSHION_HIT_COUNT; i++ ) {
        rm.ballCushionHitSounds[i] = LoadSound( "resources/sfx/ball-cushion-hit.wav" );
        SetSoundVolume( rm.ballCushionHitSounds[i], 0.2f );
    }
    rm.ballCushionHitCount = BALL_CUSHION_HIT_COUNT;
    rm.ballCushionHitIndex = 0;

}

void unloadResourcesResourceManager( void ) {

    UnloadTexture( rm.ballsTexture );
    
    StopMusicStream( rm.backgroundMusic );
    UnloadMusicStream( rm.backgroundMusic );
    
    UnloadSound( rm.ballFallingSound );
    UnloadSound( rm.cueBallHitSound );
    UnloadSound( rm.cueStickHitSound );

    for ( int i = 0; i < rm.ballHitCount; i++ ) {
        UnloadSound( rm.ballHitSounds[i] );
    }

    for ( int i = 0; i < rm.ballCushionHitCount; i++ ) {
        UnloadSound( rm.ballCushionHitSounds[i] );
    }

}