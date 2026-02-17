/**
 * @file CueStick.h
 * @author Prof. Dr. David Buzatto
 * @brief Common macros for the entire game.
 * 
 * @copyright Copyright (c) 2026
 */

#pragma once

#define MARGIN 100
#define TABLE_MARGIN 40

#define RELEASE

#ifdef RELEASE
    #define TEST_BALL_POSITIONING false
    #define SHUFFLE_BALLS true
    #define SHOW_DEBUG_INFO false
    #define SHOW_HELP false
    #define BG_MUSIC_ENABLED true
    #define trace( ... )
#else
    #define TEST_BALL_POSITIONING false
    #define SHUFFLE_BALLS true
    #define SHOW_DEBUG_INFO true
    #define SHOW_HELP false
    #define BG_MUSIC_ENABLED false
    #define trace( ... ) TraceLog( LOG_INFO, __VA_ARGS__ );
#endif

#define BALL_COUNT 15
#define BALL_RADIUS 10
#define BALL_FRICTION 0.99f
#define BALL_ELASTICITY 0.9f