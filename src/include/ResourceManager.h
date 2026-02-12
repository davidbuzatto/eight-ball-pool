/**
 * @file ResourceManager.h
 * @author Prof. Dr. David Buzatto
 * @brief ResourceManager struct and function declarations.
 * 
 * @copyright Copyright (c) 2026
 */
#pragma once

#include "raylib/raylib.h"

#define BALL_HIT_COUNT 10
#define BALL_CUSHION_HIT_COUNT 10

typedef struct ResourceManager {

    Texture2D ballsTexture;

    Music backgroundMusic;
    
    Sound ballFalling;
    Sound cueBallHit;
    Sound cueStickHit;

    Sound ballHits[BALL_HIT_COUNT];
    int ballHitCount;
    int ballHitIndex;

    Sound ballCushionHits[BALL_CUSHION_HIT_COUNT];
    int ballCushionHitCount;
    int ballCushionHitIndex;

} ResourceManager;

/**
 * @brief Global ResourceManager instance.
 */
extern ResourceManager rm;

/**
 * @brief Load global game resources, linking them in the global instance of
 * ResourceManager called rm.
 */
void loadResourcesResourceManager( void );

/**
 * @brief Unload global game resources.
 */
void unloadResourcesResourceManager( void );