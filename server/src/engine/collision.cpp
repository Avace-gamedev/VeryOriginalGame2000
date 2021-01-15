#include "engine/collision.h"

#include "loguru/loguru.hpp"

#include "engine/game_config.h"

bool overlaps(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2)
{
    return (x1 + w1 > x2) && (x1 < x2 + w2) && (y1 + h1 > y2) && (y1 < y2 + h2);
}

bool overlaps(Vec2f pos1, Vec2f size1, Vec2f pos2, Vec2f size2)
{
    return overlaps(pos1.x, pos1.y, size1.x, size1.y, pos2.x, pos2.y, size2.x, size2.y);
}

bool intersects(Vec2f ray_origin, float ray_angle, Vec2f sphere_origin, float sphere_radius)
{
    Vec2f dir = Vec2f((float)cos(ray_angle), (float)sin(ray_angle));

    Vec2f L = ray_origin - sphere_origin;
    float a = dir * dir;
    float b = 2 * dir * L;
    float c = L * L - sphere_radius * sphere_radius;

    float d = b * b - 4 * a * c;

    return d >= 0;
}

float computeDistance(Vec2f ray_origin, float ray_angle, Vec2f sphere_origin, float sphere_radius)
{
    Vec2f dir = Vec2f((float)cos(ray_angle), (float)sin(ray_angle));

    float t = (sphere_origin - ray_origin) * dir;

    Vec2f p = ray_origin + t * dir;
    float y = (float)sqrt((sphere_origin - p) * (sphere_origin - p));

    if (y > sphere_radius)
        return -1;

    float x = (float)sqrt((double)sphere_radius * (double)sphere_radius - (double)y * (double)y);

    return t - x;
}

float computeDistance(Vec2f ray_origin, float ray_angle, float range, const TilemapDesc *map)
{
    int tile_size = map->tile_size * map->scale;

    Vec2f pos = ray_origin / (float)tile_size;
    Vec2i map_pos = Vec2i((int)pos.x, (int)pos.y);
    Vec2f dir = Vec2f((float)cos(ray_angle), (float)sin(ray_angle));

    float delta_dist_x = dir.y == 0 ? 0 : dir.x == 0 ? 1 : (float)abs(1. / dir.x);
    float delta_dist_y = dir.x == 0 ? 0 : dir.y == 0 ? 1 : (float)abs(1. / dir.y);

    int step_x;
    int step_y;
    float side_dist_x;
    float side_dist_y;

    if (dir.x < 0)
    {
        step_x = -1;
        side_dist_x = (pos.x - map_pos.x) * delta_dist_x;
    }
    else
    {
        step_x = 1;
        side_dist_x = (map_pos.x + 1 - pos.x) * delta_dist_x;
    }

    if (dir.y < 0)
    {
        step_y = -1;
        side_dist_y = (pos.y - map_pos.y) * delta_dist_y;
    }
    else
    {
        step_y = 1;
        side_dist_y = (map_pos.y + 1 - pos.y) * delta_dist_y;
    }

    bool hit = false;
    int side;
    float wall_dist;
    while (!hit)
    {
        if (side_dist_x < side_dist_y)
        {
            side_dist_x += delta_dist_x;
            map_pos.x += step_x;
            side = 0;
        }
        else
        {
            side_dist_y += delta_dist_y;
            map_pos.y += step_y;
            side = 1;
        }

        hit = map->collisions->get(map->indexOfXY(map_pos.x, map_pos.y));

        if (side == 0)
            wall_dist = (map_pos.x - pos.x + (1 - step_x) / 2) / dir.x;
        else
            wall_dist = (map_pos.y - pos.y + (1 - step_y) / 2) / dir.y;

        if (wall_dist > range / tile_size)
            return range;
    }

    return wall_dist * tile_size;
}

// collisions taken from HaxeFlixel engine
float computeOverlapX(Vec2f pos1, Vec2f last_pos1, Vec2f size1, Vec2f pos2, Vec2f last_pos2, Vec2f size2)
{
    float overlap = 0;
    float delta1 = pos1.x - last_pos1.x;
    float delta2 = pos2.x - last_pos2.x;

    if (delta1 != delta2)
    {
        float delta1_abs = (delta1 > 0) ? delta1 : -delta1;
        float delta2_abs = (delta2 > 0) ? delta2 : -delta2;

        // did movements of 1 and 2 overlap ?
        // compute movement rects
        float x1 = pos1.x - ((delta1 > 0) ? delta1 : 0);
        float y1 = last_pos1.y;
        float w1 = size1.x + delta1_abs;
        float h1 = size1.y;

        float x2 = pos2.x - ((delta2 > 0) ? delta2 : 0);
        float y2 = last_pos2.y;
        float w2 = size2.x + delta2_abs;
        float h2 = size2.y;

        // do movement hulls overlap ?
        if (overlaps(x1, y1, w1, h1, x2, y2, w2, h2))
        {
            float maxOverlap = delta1_abs + delta2_abs + SEPARATE_BIAS;

            // amazing:
            // if objects are moving in opposite directions, the one on left is the one moving toward the right, thus delta1 > delta2
            // if objects are moving in the same direction, one of them is faster:
            //    - if they are moving right, the faster one has to be the one on left, hence delta1 > delta2
            //    - if they are moving left, the faster one has to be the one at right, but moving up means delta < 0, hence delta1 > delta2
            // all in all, if delta1 > delta2, object 1 is the one on left
            if (delta1 > delta2)
            {
                overlap = pos1.x + size1.x - pos2.x;

                if (overlap > maxOverlap)
                    overlap = 0;
            }
            else
            {
                overlap = pos1.x - size2.x - pos2.x;

                if (-overlap > maxOverlap)
                    overlap = 0;
            }
        }
    }
    return overlap;
}

float computeOverlapY(Vec2f pos1, Vec2f last_pos1, Vec2f size1, Vec2f pos2, Vec2f last_pos2, Vec2f size2)
{
    float overlap = 0;
    float delta1 = pos1.y - last_pos1.y;
    float delta2 = pos2.y - last_pos2.y;

    if (delta1 != delta2)
    {
        float delta1_abs = (delta1 > 0) ? delta1 : -delta1;
        float delta2_abs = (delta2 > 0) ? delta2 : -delta2;

        // did movements of 1 and 2 overlap ?
        // compute movement rects
        float x1 = pos1.x;
        float y1 = pos1.y - ((delta1 > 0) ? delta1 : 0);
        float w1 = size1.x;
        float h1 = size1.y + delta1_abs;

        float x2 = pos2.x;
        float y2 = pos2.y - ((delta2 > 0) ? delta2 : 0);
        float w2 = size2.x;
        float h2 = size2.y + delta2_abs;

        // do movement hulls overlap ?
        if (overlaps(x1, y1, w1, h1, x2, y2, w2, h2))
        {
            float maxOverlap = delta1_abs + delta2_abs + SEPARATE_BIAS;

            // amazing:
            // if objects are moving in opposite directions, the one on left is the one moving toward the right, thus delta1 > delta2
            // if objects are moving in the same direction, one of them is faster:
            //    - if they are moving right, the faster one has to be the one on left, hence delta1 > delta2
            //    - if they are moving left, the faster one has to be the one at right, but moving up means delta < 0, hence delta1 > delta2
            // all in all, if delta1 > delta2, object 1 is the one on left
            if (delta1 > delta2)
            {
                overlap = pos1.y + size1.y - pos2.y;

                if (overlap > maxOverlap)
                    overlap = 0;
            }
            else
            {
                overlap = pos1.y - size2.y - pos2.y;

                if (-overlap > maxOverlap)
                    overlap = 0;
            }
        }
    }
    return overlap;
}