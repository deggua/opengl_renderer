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

// Project U onto V
static inline glm::vec3 Project(const glm::vec3& u, const glm::vec3& v)
{
    return v * glm::dot(u, v) / glm::dot(v, v);
}

static inline glm::mat3
Orthonormal_GramSchmidt(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3)
{
    glm::vec3 u1 = v1;
    glm::vec3 u2 = v2 - Project(v2, u1);
    glm::vec3 u3 = v3 - Project(v3, u1) - Project(v3, u2);

    return glm::mat3(glm::normalize(u1), glm::normalize(u2), glm::normalize(u3));
}
