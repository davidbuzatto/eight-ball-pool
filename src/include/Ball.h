/**
 * @file Ball.h
 * @author Prof. Dr. David Buzatto
 * @brief Ball function declarations.
 * 
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "Types.h"

void updateBall( Ball *b, float delta );
void drawBall( Ball *b );

void resolveCollisionBallBall( Ball *b1, Ball *b2 );
CollisionResult ballSegmentCollision( Ball *b, Vector2 segStart, Vector2 segEnd );
CollisionResult ballPointSweep( Ball *b, Vector2 point );
CollisionResult ballConvexCollision( Ball *b, Vector2* vertices, int numVertices );
CollisionResult ballCushionCollision( Ball *b, Cushion *c );