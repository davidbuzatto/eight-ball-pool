#pragma once

#include "raylib/raylib.h"

typedef enum GameState {
    GAME_STATE_BALLS_STOPPED,
    GAME_STATE_BALLS_MOVING,
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
} Ball;

typedef struct CueStick {
    Vector2 target;
    float distanceFromTarget;
    float size;
    float angle;
    int impulse;
} CueStick;

typedef struct GameWorld {
    Rectangle boundarie;
    Ball *cueBall;
    Ball balls[16];
    CueStick cueStick;
    GameState state;
} GameWorld;
