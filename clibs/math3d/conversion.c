#include "util.h"
#include "vector3.h"
#include "euler.h"
#include "matrix44.h"
#include "conversion.h"


struct vector3 X_v3 = {1, 0, 0};
struct vector3 Y_v3 = {0, 1, 0};
struct vector3 Z_v3 = {0, 0, 1};

struct vector4 X_v4 = {1, 0, 0, 0};
struct vector4 Y_v4 = {0, 1, 0, 0};
struct vector4 Z_v4 = {0, 0, 1, 0};
struct vector4 W_v4 = {0, 0, 0, 1};

#define C m->c
union matrix44*
euler_to_matrix44(const struct euler *e, union matrix44 *m) {
	// the actually order is : 
	// roll -> pitch -> yaw	
	const float p = TO_RADIAN(e->pitch);
	const float y = TO_RADIAN(e->yaw);
	const float r = TO_RADIAN(e->roll);

	const float cp = cosf(p);
	const float sp = sinf(p);

	const float cy = cosf(y);
	const float sy = sinf(y);	

	const float cr = cosf(r);
	const float sr = sinf(r);	

	C[0][0] = cy*cr + sy*sp*sr;
	C[0][1] = sr*cp;
	C[0][2] = -sy*cr + cy*sp*sr;
	C[0][3] = 0.0f;

	C[1][0] = -cy*sr + sy*sp*cr;
	C[1][1] = cr*cp;
	C[1][2] = sr*sy + cy*sp*cr;
	C[1][3] = 0.0f;

	C[2][0] = sy*cp;
	C[2][1] = -sp;
	C[2][2] = cy*cp;
	C[2][3] = 0.0f;

	C[3][0] = C[3][1] = C[3][2] = 0.f;
	C[3][3] = 1.f;
	return m;
}

struct euler*
matrix44_to_euler(const union matrix44 *m, struct euler *e) {
    struct vector3 row0 = *((const struct vector3*)m->c[0]);
    vector3_normalize(&row0);

    struct vector3 row2 = *((const struct vector3*)m->c[2]);
    vector3_normalize(&row2);

    // Detect negative scale with determinant and flip one arbitrary axis
    if (matrix44_determinant(m) < 0){
        row0.x = -row0.x;
    }

    // see euler_to_matrix44
	// cy = cos(yaw), sy = sin(yaw)
    // cp = cos(pitch), sp = sin(pitch)
    // cr = cos(roll), sr = sin(roll)
	// cy*cr+sp*sy*sr   cr*sp*sy-cy*sr  cp*sy	
	// cp*sr            cp*cr           -sp
	// -cr*sy+cy*sp*sr  cy*cr*sp+sy*sr  cp*cy
    
	//float sp = -C[2][1];
    float sp = -row2.y;
	if (sp <= -1.f)
		e->pitch = -(float)(M_PI_2);
	else if (sp >= 1.f)
		e->pitch = (float)(M_PI_2);
	else
		e->pitch = asinf(sp);

    // Special case: cp == 0 (when sp is +/-1)
	if (fabs(sp) > 0.9999f){
        // Pin arbitrarily one of yaw or roll to zero(selet roll to zero is a better chioce, because we only use row0)
	 	// Mathematical equivalent of gimbal lock

		// e->roll = 0.f;
		// e->yaw = atan2f(-C[0][2], C[0][0]);
        // e->yaw = 0.f;
        // e->roll = atan2f(-C[1][0], C[0][0]);

	 	// Now: cp = 0, sp = +/-1, cy = 1, sy = 0
	 	// => m[0][0] = cr and m[1][0] = sr
        e->roll = 0.f;
        e->yaw = atan2f(-row0.z, row0.x);
	} else {
        struct vector3 row1 = *((const struct vector3*)m->c[1]);

        vector3_normalize(&row1);
        

		// e->yaw = atan2f(C[2][0], C[2][2]);
		// e->roll = atan2f(C[0][1], C[1][1]);
        e->yaw = atan2f(row2.x, row2.z);
        e->roll = atan2f(row0.y, row1.y);
	}

	euler_to_degree(e);
    return e;
}

struct quaternion*
euler_to_quaternion(const struct euler *e, struct quaternion *q) {
	// same with euler_to_matrix, the order is : 
	// roll -> pitch -> yaw
	const float p = TO_RADIAN(e->pitch) * 0.5f;
	const float y = TO_RADIAN(e->yaw) * 0.5f;
	const float r = TO_RADIAN(e->roll) * 0.5f;

	const float cr = cosf(r), sr = sinf(r);
	const float cy = cosf(y), sy = sinf(y);
	const float cp = cosf(p), sp = sinf(p);

	q->x = cy * cp * cr + sy * sp * sr;
	q->y = cy * sp * cr + sy * cp * sr;
	q->z = sy * cp * cr - cy * sp * cr;
	q->w = cy * cp * sr - sy * sp * cr;

    return q;
}

 extern struct euler*
 quaternion_to_euler(const struct quaternion *q, struct euler *e){
	float sp = -2.f * (q->y * q->z - q->w * q->x);
	if (fabs(sp) > 0.9999f){
		e->pitch = M_PI_2 * sp;
		e->yaw = atan2f(-q->x * q->z + q->w * q->y, 0.5f - q->y * q->y - q->z * q->z);
		e->roll = 0.f;
	} else {
		e->pitch 	= asinf(sp);
		e->yaw 		= atan2f(q->x * q->z + q->w * q->y, 0.5f - q->x * q->x - q->y * q->y);
		e->roll 	= atan2f(q->x * q->y + q->w * q->z, 0.5f - q->x * q->x - q->z * q->z);
	}

	return euler_to_degree(e);
	 
 }