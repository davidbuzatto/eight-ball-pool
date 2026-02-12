/**
 * @file Ball.c
 * @author Prof. Dr. David Buzatto
 * @brief Ball implementation.
 * 
 * @copyright Copyright (c) 2026
 */

#include <math.h>

#include "raylib/raylib.h"
#include "raylib/raymath.h"

#include "Ball.h"
#include "ResourceManager.h"
#include "Types.h"

void updateBall( Ball *b, float delta ) {

    b->center.x += b->vel.x * delta;
    b->center.y += b->vel.y * delta;

    b->vel.x *= b->friction;
    b->vel.y *= b->friction;

    float speed = sqrtf( b->vel.x * b->vel.x + b->vel.y * b->vel.y );
    if ( speed < 0.5f ) {  // less than 0.5 pixels/second
        b->vel.x = 0;
        b->vel.y = 0;
        b->moving = false;
    } else {
        b->moving = true;
    }

}

void drawBall( Ball *b ) {

    if ( b->pocketed ) {
        return;
    }

    DrawTexturePro( 
        rm.ballsTexture, 
        (Rectangle) { 64 * b->number, 0, 64, 64 }, 
        (Rectangle) { b->center.x - b->radius, b->center.y - b->radius, b->radius * 2, b->radius * 2 },
        (Vector2) { 0 },
        0.0f,
        WHITE
    );

    DrawCircleLinesV( b->center, b->radius, BLACK );

    /*if ( b->striped ) {
        DrawCircleV( b->center, b->radius, WHITE );
        DrawLineEx( 
            (Vector2) { b->center.x - b->radius, b->center.y}, 
            (Vector2) { b->center.x + b->radius, b->center.y}, 
            10, 
            b->color
        );
    } else {
        DrawCircleV( b->center, b->radius, b->color );
    }
    
    DrawCircleLinesV( b->center, b->radius, BLACK );

    if ( b->number != 0 ) {
        const char *n = TextFormat( "%d", b->number );
        int w = MeasureText( n, 14 );
        DrawText( 
            n, 
            b->center.x - w / 2, 
            b->center.y - 6, 
            14, 
            b->number == 8 ? WHITE : BLACK
        );
    }*/

    /*if ( b->moving ) {
        DrawCircleLinesV( b->center, b->radius, WHITE );
    } else {
        DrawCircleLinesV( b->center, b->radius, BLACK );
    }*/

}

void resolveCollisionBallBall( Ball *b1, Ball *b2 ) {

    // vector between centers
    Vector2 d = {
        b2->center.x - b1->center.x,
        b2->center.y - b1->center.y
    };

    float distance = Vector2Distance( b1->center, b2->center );
    float minDistance = b1->radius + b2->radius;

    // the balls are not colliding
    if ( distance >= minDistance ) {
        return;
    }

    // normalizing
    Vector2 norm = {
        d.x / distance,
        d.y / distance
    };

    // important: separate the balls
    float overlap = minDistance - distance;
    float separation = overlap / 2.0f;

    b1->center.x -= separation * norm.x;
    b1->center.y -= separation * norm.y;
    b2->center.x += separation * norm.x;
    b2->center.y += separation * norm.y;

    // relative velocity
    Vector2 v = {
        b1->vel.x - b2->vel.x,
        b1->vel.y - b2->vel.y
    };

    // velocity on normal
    float dotProduct = Vector2DotProduct( v, norm );

    // do not collide if the balls are moving away
    if ( dotProduct <= 0 ) {
        return;
    }

    // transfer momentum (equal mass)
    b1->vel.x -= dotProduct * norm.x;
    b1->vel.y -= dotProduct * norm.y;
    b2->vel.x += dotProduct * norm.x;
    b2->vel.y += dotProduct * norm.y;

}

// Verifica colisão círculo vs segmento de linha
CollisionResult ballSegmentCollision( Ball *b, Vector2 segStart, Vector2 segEnd ) {
    CollisionResult result = { 0 };

    Vector2 movement = Vector2Subtract( b->center, b->prevPos );

    // se não há movimento, não há colisão sweep
    if ( Vector2Length( movement ) < 0.001f ) {
        return result;
    }

    Vector2 segDir = Vector2Subtract( segEnd, segStart );
    float segLen = Vector2Length( segDir );
    Vector2 segNorm = Vector2Normalize( segDir );

    // normal do segmento (perpendicular, aponta para "fora")
    Vector2 normal = { segNorm.y, -segNorm.x };

    // distância do centro atual à linha
    Vector2 toLineCurr = Vector2Subtract( b->center, segStart );
    float distCurr = Vector2DotProduct( toLineCurr, normal );

    // distância do centro anterior à linha
    Vector2 toLinePrev = Vector2Subtract( b->prevPos, segStart );
    float distPrev = Vector2DotProduct( toLinePrev, normal );

    // verifica se está se aproximando da linha
    if ( distCurr >= distPrev ) {
        return result; // Afastando ou paralelo
    }

    // verifica se vai colidir neste frame
    if ( distCurr > b->radius || distPrev < -b->radius ) {
        return result; // muito longe
    }

    // calcula t (quando o círculo toca a linha)
    float distChange = distCurr - distPrev;
    float t = ( distPrev - b->radius ) / -distChange;

    // clamp t entre 0 e 1
    if ( t < 0.0f ) {
        t = 0.0f;
    }
    if ( t > 1.0f ) {
        t = 1.0f;
    }

    // posição do centro no momento da colisão
    Vector2 collisionCenter = Vector2Add( b->prevPos, Vector2Scale( movement, t ) );

    // projeção no segmento
    Vector2 toCollision = Vector2Subtract( collisionCenter, segStart );
    float projection = Vector2DotProduct( toCollision, segNorm );

    // verifica se está dentro do segmento (com margem)
    if ( projection >= -0.1f && projection <= segLen + 0.1f ) {
        result.hasCollision = true;
        result.t = t;
        result.point = Vector2Add( collisionCenter, Vector2Scale( normal, -b->radius ) );
        result.normal = normal;
    }

    return result;

}

// colisão do círculo em movimento com um ponto (vértice)
CollisionResult ballPointSweep( Ball *b, Vector2 point ) {

    CollisionResult result = { 0 };

    Vector2 movement = Vector2Subtract( b->center, b->prevPos );
    Vector2 toPoint = Vector2Subtract( point, b->prevPos );

    float aC = Vector2DotProduct( movement, movement );
    float bC = -2.0f * Vector2DotProduct( movement, toPoint );
    float cC = Vector2DotProduct( toPoint, toPoint ) - b->radius * b->radius;

    float discriminant = bC * bC - 4 * aC * cC;

    if ( discriminant < 0 || aC == 0 ) {
        return result;
    }

    float t = ( -bC - sqrtf( discriminant ) ) / ( 2.0f * aC );

    if ( t >= 0 && t <= 1 ) {

        Vector2 collisionCenter = Vector2Add( b->prevPos, Vector2Scale( movement, t ) );
        Vector2 normal = Vector2Normalize( Vector2Subtract( collisionCenter, point ) );

        result.hasCollision = true;
        result.t = t;
        result.point = Vector2Add( point, Vector2Scale( normal, b->radius ) );
        result.normal = normal;

    }

    return result;

}

// calcula a colisão entre uma bola e um polígono convexo
CollisionResult ballConvexCollision( Ball *b, Vector2* vertices, int numVertices ) {

    CollisionResult earliestCollision = { 0 };
    float minT = INFINITY;

    // verifica cada aresta do polígono convexo
    for ( int i = 0; i < numVertices; i++ ) {

        Vector2 start = vertices[i];
        Vector2 end = vertices[(i + 1) % numVertices];

        CollisionResult collision = ballSegmentCollision( b, start, end );

        if ( collision.hasCollision && collision.t < minT ) {
            minT = collision.t;
            earliestCollision = collision;
        }

    }

    // verifica colisão com os vértices
    if ( !earliestCollision.hasCollision ) {
        for ( int i = 0; i < numVertices; i++ ) {

            CollisionResult collision = ballPointSweep( b, vertices[i] );

            if ( collision.hasCollision && collision.t < minT ) {
                minT = collision.t;
                earliestCollision = collision;
            }

        }

    }

    return earliestCollision;

}

CollisionResult ballCushionCollision( Ball *b, Cushion *c ) {
    return ballConvexCollision( b, c->vertices, 4 );
}
