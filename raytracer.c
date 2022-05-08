#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef unsigned int  uint;
typedef unsigned char uchar;
typedef uchar         bool;
typedef float         mat4[4];
typedef float         vec2[2];
typedef float         vec3[3];
typedef char         *string;

static const uchar true  = 1;
static const uchar false = 0;

static const float pi           = 3.1415926535897932385;

#define VEC_X(v)       (v[0])
#define VEC_Y(v)       (v[1])
#define VEC_Z(v)       (v[2])
#define VEC_FILL(v, t) (v[0] = v[1] = v[2] = t)

// degress to radians
#define DTOR(d)        (d * pi / 180.0)

enum ray_object_types {
    RAY_OBJECT_SPHERE
};

typedef struct t_List
{
    int len;
    int size;
    void **buff;
} List;

typedef struct t_Hit
{
    float t;
    bool frontface;
    vec3 p;
    vec3 normal;
} Hit;

typedef struct t_Sphere
{
    float r;
    vec3 center;
} Sphere;

typedef struct t_Object
{
    uint type;
    void *st;
} Object;

typedef struct t_RScreen
{
    float aspect_ratio;
    vec2 size;
} RScreen;

typedef struct t_RCamera
{
    float focal_len;
    vec2 vp;
    vec3 orig;
    vec3 hor;
    vec3 ver;
    vec3 llc; // lower left corner
} RCamera;

typedef struct t_Ray
{
    vec3 orig;
    vec3 dir;
} Ray;

static RScreen scr;
static RCamera cam;
static List    world;

/*
 * *************************
 * * LIST FUNCTIONS        *
 * *************************
 */
void lpush(List *list, void *item)
{
    if (list->len > 0)
        list->buff = (void*)realloc(list->buff, list->size * (list->len + 1));

    list->buff[list->len] = item;
    list->len += 1;
}

void*
lat(List *list, int i)
{
    if (i > list->len) return NULL;
    return list->buff[i];
}

void lfree(List *list)
{
    int i;

    for (i = 0; i < list->len; i++)
        free(list->buff[i]);

    free(list->buff);
}

void linit(List *l)
{
    l->size = sizeof(void*);
    l->len = 0;
    l->buff = (void**)malloc(sizeof(void*));
}

/*
 * **************************
 * * VECTOR FUNCTIONS       *
 * **************************
 */
void
vec3_set(vec3 v, float x, float y, float z)
{
    v[0] = x;
    v[1] = y;
    v[2] = z;
}

void
vec3_add(vec3 v1, int n, ...)
{
    va_list vl;
    int i = 0;
    float* v2;

    va_start(vl, n);
    for (;i < n; i++) {
        v2 = (float*)va_arg(vl, float*);
        (v1[0]) += v2[0];
        (v1[1]) += v2[1];
        (v1[2]) += v2[2];
    }
    va_end(vl);
}

void
vec3_sub(vec3 v1, int n, ...)
{
    va_list vl;
    int i = 0;
    float* v2;

    va_start(vl, n);
    for (;i++ < n;) {
        v2 = (float*)va_arg(vl, float*);
        (v1[0]) -= v2[0];
        (v1[1]) -= v2[1];
        (v1[2]) -= v2[2];
    }
    va_end(vl);
}

void
vec3_mul(vec3 v1, int n, ...)
{
    va_list vl;
    int i = 0;
    float* v2;

    va_start(vl, n);
    for (;i++ < n;) {
        v2 = (float*)va_arg(vl, float*);
        (v1[0]) *= v2[0];
        (v1[1]) *= v2[1];
        (v1[2]) *= v2[2];
    }
    va_end(vl);
}

void
vec3_div(vec3 v1, int n, ...)
{
    va_list vl;
    int i = 0;
    float* v2;

    va_start(vl, n);
    for (;i++ < n;) {
        v2 = (float*)va_arg(vl, float*);
        (v1[0]) /= v2[0];
        (v1[1]) /= v2[1];
        (v1[2]) /= v2[2];
    }
    va_end(vl);
}

void
vec3_mul_t(vec3 v1, float t)
{
    vec3 v2;
    v2[0] = v2[1] = v2[2] = t;
    vec3_mul(v1, 1, v2);
}

void
vec3_div_t(vec3 v1, float t)
{
    vec3 v2;
    v2[0] = v2[1] = v2[2] = t;
    vec3_div(v1, 1, v2);
}

void
vec3_copy(vec3 v1, vec3 v2)
{
    v1[0] = v2[0];
    v1[1] = v2[1];
    v1[2] = v2[2];
}

float
vec3_len(vec3 v)
{
    return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}

float
vec3_dot(vec3 v1, vec3 v2)
{
    return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

void
vec3_cross(vec3 o, vec3 v1, vec3 v2)
{
    o[0] = v1[1] * v2[2] - v1[2] * v2[1];
    o[1] = v1[2] * v2[0] - v1[0] * v2[2];
    o[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

void
vec3_unit(vec3 v1, vec3 v2)
{
    vec3_copy(v1, v2);
    vec3_div_t(v1, sqrt(vec3_len(v1)));
}

/*
 * ***********************
 * * OBJECT FUNCTIONS    *
 * ***********************
 */
void
free_obj_list(List *l)
{
    int i;
    Object *ptr;

    for (; i < l->len; i++) {
        ptr = (Object*)l->buff[i];
        free(ptr->st);
   }
}

void*
new_sphere(vec3 pos, float r)
{
    Object *o = malloc(sizeof(Object));
    Sphere *s = malloc(sizeof(Sphere));

    o->type = RAY_OBJECT_SPHERE;
    o->st   = s;

    s->r = r;
    vec3_copy(s->center, pos);

    return o;
}

/*
 * ***********************
 * * RAY FUNCTIONS       *
 * ***********************
 */
void
rayatpos(Ray *r, float t, vec3 v)
{
    vec3 orig, dir;

    vec3_copy(orig, r->orig);
    vec3_copy(dir, r->dir);
    vec3_mul_t(dir, t);
    vec3_add(orig, 1, dir);
    vec3_copy(v, orig);
}

void
setfacenormal(Hit *hit, Ray *r, vec3 on)
{
    hit->frontface = vec3_dot(r->dir, on) < 0;
    if (!hit->frontface) {
        vec3_set(on, -VEC_X(on), -VEC_Y(on), -VEC_Z(on));
    }
    vec3_copy(hit->normal, on);
}

float
hitsphere(Sphere *s, Ray *r, Hit* hit)
{
    vec3 oc;
    float a, b, c, d;

    vec3_copy(oc, r->orig);
    vec3_sub(oc, 1, s->center);

    a = vec3_len(r->dir);
    b = vec3_dot(oc, r->dir);
    c = vec3_len(oc) - s->r * s->r;
    d = b * b - a * c;

    if (d < 0)
        return -1.0;

    return (-b - sqrt(d)) / a;
}

/*
bool
hitsphere(Sphere *s, Ray *r, float t_min, float t_max, Hit* hit)
{
    vec3 oc, rpos, normal, on;
    float a, b, c, d, root, sqrtd;

    vec3_copy(oc, r->orig);
    vec3_sub(oc, 1, s->center);

    a = vec3_len(r->dir);
    b = vec3_dot(oc, r->dir);
    c = vec3_len(oc) - s->r * s->r;
    d = b * b - a * c;

    if (d < 0)
        return false;

    root = (-b - sqrtd) / a;
    if (root < t_min || t_max < root) {
        root = (-b + sqrtd) / a;
        if (root < t_min || t_max > root)
            return false;
    }

    hit->t = root;

    rayatpos(r, hit->t, rpos);
    vec3_copy(hit->p, rpos);
    vec3_copy(normal, hit->p);
    vec3_sub(normal, 1, s->center);
    vec3_div_t(normal, s->r);
    setfacenormal(hit, r, normal);

    return true;
}
*/

/*
bool
checkhitlist(List *list, Ray *r, float t_min, float t_max, Hit *hit)
{
    Hit htemp;
    int listi;
    bool hitany = false;
    float csf = t_max;
    Object *ptr;

    for (listi = list->len - 1; listi >= 0; listi--) {
        ptr = (Object*)list->buff[listi];

        switch (ptr->type) {
            case RAY_OBJECT_SPHERE:
                Sphere *s = (Sphere*)ptr->st;

                if (hitsphere(s, r, t_min, csf, &htemp)) {
                    hitany = true;
                    csf = hit->t;
                    memcpy(hit, &htemp, sizeof(Hit));
                }
                break;
        }
    }

    return hitany;
}
*/

void
raycolor(Ray *r, vec3 color)
{
    vec3 unit, rpos, c1, c2;
    Sphere s;
    Hit h;
    float t;

    s.r = 0.5;
    vec3_set(s.center, 0.0, 0.0, -1.0);
    t = hitsphere(&s, r, &h);
    if (t > 0.0) {
        rayatpos(r, t, rpos);
        vec3_sub(rpos, 1, s.center);
        vec3_unit(unit, rpos);
        vec3_set(color, VEC_X(unit) + 1, VEC_Y(unit) + 1, VEC_Z(unit) + 1);
        vec3_mul_t(color, 0.5);
        return;
    }

    vec3_unit(unit, r->dir);
    t = 0.5 * (VEC_Y(unit) + 1.0);
    vec3_set(c1, 1.0, 1.0, 1.0);
    vec3_set(c2, 0.5, 0.7, 1.0);
    vec3_mul_t(c1, 1.0 - t);
    vec3_mul_t(c2, t);
    vec3_add(c1, 1, c2);
    vec3_copy(color, c1);
}

/*
void
raycolor(Ray *r, vec3 color)
{
    vec3 unit, sp, rpos, c1, c2, a, b;
    Hit hit;
    float t;

    if (checkhitlist(&world, r, 0, INFINITY, &hit)) {
        vec3_set(color, 1.0, 1.0, 1.0);
        vec3_add(color, 1, hit.normal);
        vec3_mul_t(color, 0.5);
        return;
    }

    vec3_unit(unit, r->dir);
    t = 0.5 * (VEC_Y(unit) + 1.0);

    vec3_set(c1, 1.0, 1.0, 1.0);
    vec3_set(c2, 0.5, 0.7, 1.0);

    vec3_mul_t(c1, 1.0 - t);
    vec3_mul_t(c2, t);
    vec3_add(c1, 1, c2);
    vec3_copy(color, c1);
}
*/

/*
 * ***********************
 * * RENDER FUNCTIONS    *
 * ***********************
 */
void
printcolor(vec3 rgb)
{
    printf("%i %i %i\n",
           (int)(255.999 * rgb[0]),
           (int)(255.999 * rgb[1]),
           (int)(255.999 * rgb[2]));
}

void
genimage()
{
    int j, i;

    // imprimimos la cabezera del archivo
    printf("P3\n%i %i\n255\n", (int)VEC_X(scr.size), (int)VEC_Y(scr.size));

    // imprimimos la escala de colores
    for (j = VEC_Y(scr.size) - 1; j >= 0; --j) {
        fprintf(stderr, "\rScanlines remaining: %i ", j);
        fflush(stderr);

        for (i = 0; i < VEC_X(scr.size); ++i) {
            Ray r;
            vec3 a, b, c, color;
            float u, v;

            u = (float)i / (VEC_X(scr.size) - 1);
            v = (float)j / (VEC_Y(scr.size) - 1);

            vec3_copy(r.orig, cam.orig);
            vec3_copy(a, cam.llc);
            vec3_copy(b, cam.hor);
            vec3_copy(c, cam.ver);

            vec3_mul_t(b, u);
            vec3_mul_t(c, v);

            vec3_add(a, 2, b, c);
            vec3_sub(a, 1, cam.orig);

            vec3_copy(r.dir, a);

            raycolor(&r, color);
            printcolor(color);
        }
    }

    fprintf(stderr, "\nFinish\n");
}

void
clean()
{
    free_obj_list(&world);
    lfree(&world);
}

void
init()
{
    vec3 a, b, c, d, pos;

    // initialize screen
    scr.aspect_ratio = 16.0 / 9.0;
    VEC_X(scr.size) = 480;
    VEC_Y(scr.size) = (int)(VEC_X(scr.size) / scr.aspect_ratio);

    // initialize camera
    VEC_Y(cam.vp) = 2.0;
    VEC_X(cam.vp) = scr.aspect_ratio * VEC_Y(cam.vp);
    cam.focal_len = 1.0;

    VEC_FILL(cam.orig, 0);
    vec3_set(cam.hor, VEC_X(cam.vp), 0, 0);
    vec3_set(cam.ver, 0, VEC_Y(cam.vp), 0);

    vec3_copy(a, cam.orig);
    vec3_copy(b, cam.hor);
    vec3_copy(c, cam.ver);
    vec3_set(d, 0, 0, cam.focal_len);

    vec3_div_t(b, 2);
    vec3_div_t(c, 2);

    vec3_sub(a, 3, b, c, d);

    vec3_copy(cam.llc, a);

    linit(&world);
    vec3_set(pos, 0.0, 0.0, -1.0);
    lpush(&world, new_sphere(pos, 0.5));
    vec3_set(pos, 0.0, -100.5, -1.0);
    lpush(&world, new_sphere(pos, 100));
}

int
main(int argc, char **argv)
{
    init();
    genimage();
    clean();

    return 0;
}
