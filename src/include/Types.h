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
    GAME_STATE_BREAKING,
    GAME_STATE_OPEN_TABLE,
    GAME_STATE_PLAYING,
    GAME_STATE_BALL_IN_HAND,
    GAME_STATE_GAME_OVER
} GameState;

typedef enum GameBallsState {
    GAME_STATE_BALLS_STOPPED,
    GAME_STATE_BALLS_MOVING
} GameBallsState;

typedef enum CueStickType {
    CUE_STICK_TYPE_P1,
    CUE_STICK_TYPE_P2,
} CueStickType;

typedef enum CueStickState {
    CUE_STICK_STATE_READY,
    CUE_STICK_STATE_HITING,
    CUE_STICK_STATE_HIT
} CueStickState;

typedef enum BallGroup {
    BALL_GROUP_UNDEFINED,
    BALL_GROUP_PLAIN,
    BALL_GROUP_STRIPED
} BallGroup;

typedef struct Ball {
    Vector2 center;
    Vector2 prevPos;
    Vector2 spin;      // ball spin, spin.x = side spin, spin.y = top/back spin
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
    int powerTick;
    int power;
    int minPower;
    int maxPower;
    Vector2 hitPoint;    // point of impact, -1 to 1 (0,0 = center)
    Color color;
    int pocketedBalls[7];
    int pocketedCount;
    CueStickType type;
    CueStickState state;
    BallGroup group;
} CueStick;

typedef struct Cushion {
    Vector2 vertices[4];
} Cushion;

typedef struct Pocket {
    Vector2 center;
    int radius;
} Pocket;

typedef struct TurnStatistics {
    // statistics for game flow control
    int cueBallHits;
    int cueBallFirstHitNumber;
    bool cueBallPocketed;
    bool ballsTouchedCushion[16];
    int pocketedBalls[16];
    int pocketedCount;
} TurnStatistics;

typedef struct GameWorld {

    Rectangle boundarie;
    Cushion cushions[6];
    Pocket pockets[6];
    Ball *cueBall;
    Ball balls[16];
    CueStick cueStickP1;
    CueStick cueStickP2;
    CueStick *currentCueStick;
    GameState state;
    GameBallsState ballsState;

    // for HUD and game logic
    int pocketedBalls[15];
    int pocketedCount;

    CueStick *lastCueStick;
    CueStick *winnerCueStick;

    // drawing data
    int marksSpacing;

    // game logic
    bool applyRules;

    TurnStatistics statistics;

} GameWorld;

typedef struct CollisionResult {
    bool hasCollision;
    float t;              // collision time (0 to 1)
    Vector2 point;        // point of contact
    Vector2 normal;       // collision normal
} CollisionResult;

typedef struct TrajectoryPrediction {
    bool willHitBall;
    int ballIndex;
    Vector2 hitPoint;
    Vector2 cueBallStopPoint;
    Vector2 targetBallDirection;
    float targetBallSpeed;
} TrajectoryPrediction;
