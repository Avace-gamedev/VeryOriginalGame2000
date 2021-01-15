#include "common/vector.hpp"
#include "engine/tilemap.h"

// rect/rect overlap
bool overlaps(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2);
bool overlaps(Vec2f pos1, Vec2f size1, Vec2f pos2, Vec2f size2);

// ray/sphere intersection
bool intersects(Vec2f ray_origin, float ray_angle, Vec2f sphere_origin, float sphere_radius);

// ray/sphere intersection with distance
float computeDistance(Vec2f ray_origin, float ray_angle, Vec2f sphere_origin, float sphere_radius);

// ray/map intersection
float computeDistance(Vec2f ray_origin, float ray_angle, float range, const TilemapDesc *map);

float computeOverlapX(Vec2f pos1, Vec2f last_pos1, Vec2f size1, Vec2f pos2, Vec2f last_pos2, Vec2f size2);
float computeOverlapY(Vec2f pos1, Vec2f last_pos1, Vec2f size1, Vec2f pos2, Vec2f last_pos2, Vec2f size2);