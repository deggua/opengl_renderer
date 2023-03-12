#pragma once

#include <stdint.h>

#include <glm/glm.hpp>

#include "common.hpp"

void Random_Seed(u64 s1, u64 s2);
void Random_Seed_HighEntropy(void);

f32 Random_Unilateral(void);
f32 Random_Bilateral(void);
f32 Random_InRange(f32 min, f32 max);

glm::vec3 Random_InCube(f32 min, f32 max);

glm::vec3 Random_InSphere(f32 radius);
glm::vec3 Random_OnSphere(f32 radius);

glm::vec3 Random_InHemisphere(glm::vec3 facing, f32 radius);
glm::vec3 Random_OnHemisphere(glm::vec3 facing, f32 radius);

glm::vec2 Random_InDisc(f32 radius);
glm::vec2 Random_OnDisc(f32 radius);
