/**
 * @file Types.h
 * @author Prof. Dr. David Buzatto
 * @brief Type structs for the game.
 * 
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "raylib/raylib.h"

typedef enum GameState {
    GAME_STATE_BALLS_STOPPED,
    GAME_STATE_BALLS_MOVING
} GameState;

typedef struct Ball {
    Vector2 center;
    Vector2 prevPos;
    int radius;
    Vector2 vel;
    float friction;
    float elasticity;
    bool moving;
    Color color;
    bool striped;
    int number;
    bool pocketed;
} Ball;

typedef struct CueStick {
    Vector2 target;
    float distanceFromTarget;
    float size;
    float angle;
    int power;
    int minPower;
    int maxPower;
} CueStick;

typedef struct Cushion {
    Vector2 vertices[4];
} Cushion;

typedef struct Pocket {
    Vector2 center;
    int radius;
} Pocket;

typedef struct GameWorld {
    Rectangle boundarie;
    Cushion cushions[6];
    Pocket pockets[6];
    Ball *cueBall;
    Ball balls[16];
    CueStick cueStick;
    GameState state;
} GameWorld;

typedef struct CollisionResult {
    bool hasCollision;
    float t;              // collision time (0 to 1)
    Vector2 point;        // point of contact
    Vector2 normal;       // collision normal
} CollisionResult;
