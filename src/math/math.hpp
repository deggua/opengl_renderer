#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

#include "common.hpp"

static inline glm::quat quat_RotationBetweenVectors(const glm::vec3& start, const glm::vec3& dest)
{
    glm::vec3 n_start = glm::normalize(start);
    glm::vec3 n_dest  = glm::normalize(dest);

    f32       cos_theta = glm::dot(n_start, n_dest);
    glm::vec3 rot_axis;

    if (cos_theta < -0.999f) {
        rot_axis = glm::cross(glm::vec3(0.0f, 0.0f, 1.0f), n_start);
        if (glm::length2(rot_axis) < 0.01f) {
            rot_axis = glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), n_start);
        }

        rot_axis = glm::normalize(rot_axis);
        return glm::angleAxis(glm::radians(180.0f), rot_axis);
    }

    rot_axis = glm::cross(n_start, n_dest);
    f32 s    = glm::sqrt(2 * (1 + cos_theta));
    f32 invs = 1 / s;

    return glm::quat(s * 0.5f, rot_axis.x * invs, rot_axis.y * invs, rot_axis.z * invs);
}

// see: Moller & Hughes 1999 - 'Efficiently Building a Matrix to Rotate One Vector to Another'
static inline glm::mat4 mat4_RotationBetweenVectors(const glm::vec3& start, const glm::vec3& end)
{
    glm::vec3 f = glm::normalize(start);
    glm::vec3 t = glm::normalize(end);

    // construct x
    glm::vec3 x;
    if (f.x <= f.y && f.x <= f.z) {
        x = glm::vec3(1.0f, 0.0f, 0.0f);
    } else if (f.y <= f.x && f.y <= f.z) {
        x = glm::vec3(0.0f, 1.0f, 0.0f);
    } else {
        x = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    // construct rotation matrix
    glm::mat3 rot = glm::mat3(0.0f);
    glm::vec3 u   = x - f;
    glm::vec3 v   = x - t;
    for (usize r = 0; r < 3; r++) {
        for (usize c = 0; c < 3; c++) {
            f32 delta_ij = (r == c) ? 1.0f : 0.0f;
            f32 r_ij     = delta_ij;
            r_ij -= (2.0f / glm::dot(u, u)) * u[r] * u[c];
            r_ij -= (2.0f / glm::dot(v, v)) * v[r] * v[c];
            r_ij += ((4.0f * glm::dot(u, v)) / (glm::dot(u, u) * glm::dot(v, v))) * u[r] * u[c];
            rot[c][r] = r_ij;
        }
    }

    return glm::mat4(rot);
}
