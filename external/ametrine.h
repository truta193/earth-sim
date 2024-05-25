#ifndef AMETRINE_FW
#define AMETRINE_FW

//----------------------------------------------------------------------------//
//                                  INCLUDES                                  //
//----------------------------------------------------------------------------//

#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include "glad/glad.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#if (defined _WIN32 || defined _WIN64)
#define OEMRESOURCE
#define AM_WINDOWS
#include <windows.h>
#include <GL/glext.h>
#else
#define AM_LINUX
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <GL/glx.h>
#include <sys/stat.h>
#endif

//----------------------------------------------------------------------------//
//                                END INCLUDES                                //
//----------------------------------------------------------------------------//


//----------------------------------------------------------------------------//
//                               TYPES AND DEFS                               //
//----------------------------------------------------------------------------//

#define am_malloc(size) malloc(size)
#define am_free(mem) free((void*)mem)
#define am_realloc(mem, size) realloc(mem, size)
#define am_calloc(mem, size) calloc(mem, size)

#define AM_MAX_NAME_LENGTH 32

typedef signed char am_int8;
typedef unsigned char am_uint8;
typedef signed short am_int16;
typedef unsigned short am_uint16;
typedef signed int  am_int32;
typedef unsigned int  am_uint32;
typedef signed long long int am_int64; //Can be 4 bytes on 32bit OSes
typedef unsigned long long int am_uint64;
typedef float am_float32;
typedef double am_float64;
typedef enum {false, true} am_bool;
typedef am_uint32 am_id;

#if defined(AM_LINUX)
#define AM_CALL *
#define amgl_get_proc_address(str) glXGetProcAddress((unsigned char*)(str))
#else
#define AM_CALL __stdcall*
#define amgl_get_proc_address(str) wglGetProcAddress((str))
#endif

#if defined(AM_LINUX)
typedef void (AM_CALL PFNGLSWAPINTERVALEXTPROC) (Display *dpy, GLXDrawable drawable, int interval);
#else
typedef BOOL (AM_CALL PFNGLSWAPINTERVALEXTPROC) (int interval);
#endif
PFNGLSWAPINTERVALEXTPROC glSwapInterval = NULL;


//----------------------------------------------------------------------------//
//                             END TYPES AND DEFS                             //
//----------------------------------------------------------------------------//


//----------------------------------------------------------------------------//
//                               DYNAMIC  ARRAY                               //
//----------------------------------------------------------------------------//

#define AM_DYN_ARRAY_EMPTY_START_SLOTS 2

typedef struct am_dyn_array_header {
    size_t size; // In bytes
    size_t capacity; // In bytes
} am_dyn_array_header;

#define am_dyn_array(type) type*
#define am_dyn_array_get_header(array) ((am_dyn_array_header*)((size_t*)(array) - 2))
#define am_dyn_array_get_size(array) am_dyn_array_get_header(array)->size
#define am_dyn_array_get_count(array) am_dyn_array_get_size(array)/sizeof((array)[0])
#define am_dyn_array_get_capacity(array) am_dyn_array_get_header(array)->capacity
#define am_dyn_array_push(array, value) (am_dyn_array_resize((void**)&(array), sizeof((array)[0])), (array)[am_dyn_array_get_count(array)] = (value), am_dyn_array_get_header(array)->size += sizeof((array)[0]))
#define am_dyn_array_pop(array) am_dyn_array_get_header(array)->size -= sizeof((array)[0])
#define am_dyn_array_clear(array) (am_dyn_array_get_header(array)->size = 0)

void am_dyn_array_init(void **array, size_t value_size);
void am_dyn_array_resize(void **array, size_t add_size);
void am_dyn_array_replace(void *array, void *values, size_t offset, size_t size);
void am_dyn_array_destroy(void *array);

//----------------------------------------------------------------------------//
//                             END DYNAMIC  ARRAY                             //
//----------------------------------------------------------------------------//


//----------------------------------------------------------------------------//
//                                PACKED ARRAY                                //
//----------------------------------------------------------------------------//

#define AM_PA_INVALID_ID 0xFFFFFFFF
#define AM_PA_INVALID_INDEX 0xFFFFFFFF

#define am_packed_array(type)\
    struct {\
        am_dyn_array(am_uint32) indices;\
        am_dyn_array(type) elements;\
        am_uint32 next_id;\
    }*

#define am_packed_array_get_idx(array, id) ((id) < (array)->next_id ? (array)->indices[(id)] : AM_PA_INVALID_INDEX)
#define am_packed_array_get_val(array, id) ((array)->elements[(array)->indices[(id)]])
#define am_packed_array_get_ptr(array, id) (((id) < (array)->next_id) && ((array)->indices[(id)] != AM_PA_INVALID_INDEX) ? &((array)->elements[(array)->indices[(id)]]) : NULL)
#define am_packed_array_get_count(array) (am_dyn_array_get_count((array)->elements))
#define am_packed_array_has(array, id) (((id) < (array)->next_id) && (am_packed_array_get_idx((array), (id)) != AM_PA_INVALID_INDEX) ? 1 : 0)
#define am_packed_array_init(array, value_size) (am_packed_array_alloc((void**)&(array), sizeof(*(array))), (array)->next_id = 1, am_dyn_array_init((void**)&((array)->indices), sizeof(am_uint32)*2), am_dyn_array_get_header((array)->indices)->size += sizeof(am_uint32), am_dyn_array_init((void**)&((array)->elements), (value_size)))
#define am_packed_array_clear(array) (am_dyn_array_clear((array)->indices), am_dyn_array_clear((array)->elements))
#define am_packed_array_destroy(array) (am_dyn_array_destroy((array)->indices), am_dyn_array_destroy((array)->elements), am_free(array))
#define am_packed_array_add(array, value) (am_dyn_array_push((array)->indices, am_dyn_array_get_count((array)->indices)-1), am_dyn_array_push((array)->elements, value), (array)->next_id++)

//----------------------------------------------------------------------------//
//                              END PACKED ARRAY                              //
//----------------------------------------------------------------------------//


//----------------------------------------------------------------------------//
//                                    MATH                                    //
//----------------------------------------------------------------------------//

// COMMON MATH STUFF

#define AM_PI 3.1415926535897932
#define am_rad2deg(rad) (am_float32)(((rad) * 180.0f) / AM_PI)
#define am_deg2rad(deg) (am_float32)(((deg) * AM_PI) / 180.0f)

// VEC2

typedef struct am_vec2 {
    union {
        am_float32 xy[2];
        struct {
            am_float32 x, y;
        };
    };
} am_vec2;

typedef struct am_vec2u {
    union {
        am_uint32 xy[2];
        struct {
            am_uint32 x, y;
        };
    };
} am_vec2u;

static inline am_vec2 am_vec2_create(am_float32 x, am_float32 y);
static inline am_vec2 am_vec2_add(am_vec2 a, am_vec2 b);
static inline am_vec2 am_vec2_sub(am_vec2 a, am_vec2 b);
static inline am_vec2 am_vec2_mul(am_vec2 a, am_vec2 b);
static inline am_vec2 am_vec2_div(am_vec2 a, am_vec2 b);
static inline am_vec2 am_vec2_scale(am_float32 scalar, am_vec2 a);
static inline am_float32 am_vec2_dot(am_vec2 a, am_vec2 b);
static inline am_float32 am_vec2_len(am_vec2 a);
static inline am_vec2 am_vec2_norm(am_vec2 a);
static inline am_float32 am_vec2_dist(am_vec2 a, am_vec2 b);
static inline am_float32 am_vec2_cross(am_vec2 a, am_vec2 b);
static inline am_float32 am_vec2_angle(am_vec2 a, am_vec2 b);

//VEC3

typedef struct am_vec3 {
    union {
        am_float32 xyz[3];
        struct {
            am_float32 x, y, z;
        };
    };
} am_vec3;

static inline am_vec3 am_vec3_create(am_float32 x, am_float32 y, am_float32 z);
static inline am_vec3 am_vec3_add(am_vec3 a, am_vec3 b);
static inline am_vec3 am_vec3_sub(am_vec3 a, am_vec3 b);
static inline am_vec3 am_vec3_mul(am_vec3 a, am_vec3 b);
static inline am_vec3 am_vec3_div(am_vec3 a, am_vec3 b);
static inline am_vec3 am_vec3_scale(am_float32 scalar, am_vec3 a);
static inline am_float32 am_vec3_dot(am_vec3 a, am_vec3 b);
static inline am_float32 am_vec3_len(am_vec3 a);
static inline am_float32 am_vec3_len_sqr(am_vec3 a);
static inline am_float32 am_vec3_dist(am_vec3 a, am_vec3 b);
static inline am_vec3 am_vec3_norm(am_vec3 a);
static inline am_vec3 am_vec3_cross(am_vec3 a, am_vec3 b);
static inline am_float32 am_vec3_angle_unsigned(am_vec3 a, am_vec3 b);
static inline am_float32 am_vec3_angle_signed(am_vec3 a, am_vec3 b);
static inline am_vec3 am_vec3_triple_cross(am_vec3 a, am_vec3 b, am_vec3 c);

// VEC4

typedef struct am_vec4 {
    union {
        am_float32 xyzw[4];
        struct {
            am_float32 x, y, z, w;
        };
    };
} am_vec4;

static inline am_vec4 am_vec4_create(am_float32 x, am_float32 y, am_float32 z, am_float32 w);
static inline am_vec4 am_vec4_add(am_vec4 a, am_vec4 b);
static inline am_vec4 am_vec4_sub(am_vec4 a, am_vec4 b);
static inline am_vec4 am_vec4_mul(am_vec4 a, am_vec4 b);
static inline am_vec4 am_vec4_div(am_vec4 a, am_vec4 b);
static inline am_vec4 am_vec4_scale(am_float32 scalar, am_vec4 a);
static inline am_float32 am_vec4_dot(am_vec4 a, am_vec4 b);
static inline am_float32 am_vec4_len(am_vec4 a);
static inline am_float32 am_vec4_dist(am_vec4 a, am_vec4 b);
static inline am_vec4 am_vec4_norm(am_vec4 a);

// GENERAL VECTORS

static inline am_vec3 am_vec4_to_vec3(am_vec4 a);
static inline am_vec2 am_vec3_to_vec2(am_vec4 a);

// MAT 3x3

typedef struct am_mat3 {
    am_float32 m[9];
} am_mat3;

static inline am_mat3 am_mat3_create();
static inline am_mat3 am_mat3_diag(am_float32 val);
#define am_mat3_identity() am_mat3_diag(1.0f)
static inline am_mat3 am_mat3_mul(am_mat3 m0, am_mat3 m1);
static inline am_vec3 am_mat3_mul_vec3(am_mat3 m, am_vec3 v);
static inline am_mat3 am_mat3_scale(am_float32 x, am_float32 y, am_float32 z);
static inline am_mat3 am_mat3_rotate(am_float32 radians, am_float32 x, am_float32 y, am_float32 z);
static inline am_mat3 am_mat3_rotatev(am_float32 radians, am_vec3 axis);
static inline am_mat3 am_mat3_rotateq(am_vec4 q);
static inline am_mat3 am_mat3_rsq(am_vec4 q, am_vec3 s);
static inline am_mat3 am_mat3_inverse(am_mat3 m);

// MAT4

typedef struct am_mat4 {
    union {
        am_vec4 rows[4];
        am_float32 elements[16];
    };
} am_mat4;

static inline am_mat4 am_mat4_create();
static inline am_mat4 am_mat4_diag(am_float32 val);
#define am_mat4_identity() am_mat4_diag(1.0f)
static inline am_mat4 am_mat4_elem(const am_float32* elements);
static inline am_mat4 am_mat4_mul(am_mat4 m0, am_mat4 m1);
static inline am_mat4 am_mat4_mul_list(am_uint32 count, ...);
static inline void am_mat4_set_elements(am_mat4* m, const am_float32* elements, am_uint32 count);
static inline am_mat4 am_mat4_transpose(am_mat4 m);
static inline am_mat4 am_mat4_inverse(am_mat4 m);
static inline am_mat4 am_mat4_ortho(am_float32 left, am_float32 right, am_float32 bottom, am_float32 top, am_float32 _near, am_float32 _far);
static inline am_mat4 am_mat4_perspective(am_float32 fov, am_float32 aspect_ratio, am_float32 _near, am_float32 _far);
static inline am_mat4 am_mat4_translatev(am_vec3 v);
static inline am_mat4 am_mat4_translate(am_float32 x, am_float32 y, am_float32 z);
static inline am_mat4 am_mat4_scalev(am_vec3 v);
static inline am_mat4 am_mat4_scale(am_float32 x, am_float32 y, am_float32 z);
static inline am_mat4 am_mat4_rotatev(am_float32 angle, am_vec3 axis);
static inline am_mat4 am_mat4_rotate(am_float32 angle, am_float32 x, am_float32 y, am_float32 z);
static inline am_mat4 am_mat4_look_at(am_vec3 position, am_vec3 target, am_vec3 up);
static inline am_vec4 am_mat4_mul_vec4(am_mat4 m, am_vec4 v);
static inline am_vec3 am_mat4_mul_vec3(am_mat4 m, am_vec3 v);

// Quaternion

typedef struct am_quat{
    union {
        am_vec4 v;
        am_float32 xyzw[4];
        struct {
            am_float32 x, y, z, w;
        };
    };
} am_quat;

static inline am_quat am_quat_default();
static inline am_quat am_quat_create(am_float32 _x, am_float32 _y, am_float32 _z, am_float32 _w);
static inline am_quat am_quat_add(am_quat q0, am_quat q1);
static inline am_quat am_quat_sub(am_quat q0, am_quat q1);
static inline am_quat am_quat_mul(am_quat q0, am_quat q1);
static inline am_quat am_quat_mul_list(am_uint32 count, ...);
static inline am_quat am_quat_mul_quat(am_quat q0, am_quat q1);
static inline am_quat am_quat_scale(am_quat q, am_float32 s);
static inline am_float32 am_quat_dot(am_quat q0, am_quat q1);
static inline am_quat am_quat_conjugate(am_quat q);
static inline am_float32 am_quat_len(am_quat q);
static inline am_quat am_quat_norm(am_quat q);
static inline am_quat am_quat_cross(am_quat q0, am_quat q1);
static inline am_quat am_quat_inverse(am_quat q);
static inline am_vec3 am_quat_rotate(am_quat q, am_vec3 v);
static inline am_quat am_quat_angle_axis(am_float32 rad, am_vec3 axis);
static inline am_quat am_quat_slerp(am_quat a, am_quat b, am_float32 t);
#define quat_axis_angle(axis, angle) am_quat_angle_axis(angle, axis)
static inline am_mat4 am_quat_to_mat4(am_quat _q);
static inline am_quat am_quat_from_euler(am_float32 yaw_deg, am_float32 pitch_deg, am_float32 roll_deg);
static inline am_float32 am_quat_pitch(am_quat* q);
static inline am_float32 am_quat_yaw(am_quat* q);
static inline am_float32 am_quat_roll(am_quat* q);
static inline am_vec3 am_quat_to_euler(am_quat* q);

// TRANSFORM

typedef struct am_vqs {
    union {
        am_vec3 position;
        am_vec3 translation;
    };
    am_quat rotation;
    am_vec3 scale;
} am_vqs;

static inline am_vqs am_vqs_create(am_vec3 translation, am_quat rotation, am_vec3 scale);
static inline am_vqs am_vqs_default();
static inline am_vqs am_vqs_absolute_transform(const am_vqs* local, const am_vqs* parent);
static inline am_vqs am_vqs_relative_transform(const am_vqs* absolute, const am_vqs* parent);
static inline am_mat4 am_vqs_to_mat4(const am_vqs* transform);

//----------------------------------------------------------------------------//
//                                  END MATH                                  //
//----------------------------------------------------------------------------//


//----------------------------------------------------------------------------//
//                                  PLATFORM                                  //
//----------------------------------------------------------------------------//

#define AM_MAX_KEYCODE_COUNT 512
#define AM_ROOT_WIN_CLASS "AM_ROOT"
#define AM_CHILD_WIN_CLASS "AM_CHILD"
#define AM_WINDOW_DEFAULT_X 500
#define AM_WINDOW_DEFAULT_Y 500
#define AM_WINDOW_DEFAULT_WIDTH 500
#define AM_WINDOW_DEFAULT_HEIGHT 500
#define AM_WINDOW_DEFAULT_NAME "Ametrine"

#if defined(AM_LINUX)
#define AM_WINDOW_DEFAULT_PARENT DefaultRootWindow(am_engine_get_subsystem(platform)->display)
#else
#define AM_WINDOW_DEFAULT_PARENT 0
#endif

typedef enum am_key_map {
    AM_KEYCODE_INVALID,
    AM_KEYCODE_ESCAPE,
    AM_KEYCODE_F1,
    AM_KEYCODE_F2,
    AM_KEYCODE_F3,
    AM_KEYCODE_F4,
    AM_KEYCODE_F5,
    AM_KEYCODE_F6,
    AM_KEYCODE_F7,
    AM_KEYCODE_F8,
    AM_KEYCODE_F9,
    AM_KEYCODE_F10,
    AM_KEYCODE_F11,
    AM_KEYCODE_F12,
    AM_KEYCODE_F13,
    AM_KEYCODE_F14,
    AM_KEYCODE_F15,
    AM_KEYCODE_F16,
    AM_KEYCODE_F17,
    AM_KEYCODE_F18,
    AM_KEYCODE_F19,
    AM_KEYCODE_F20,
    AM_KEYCODE_F21,
    AM_KEYCODE_F22,
    AM_KEYCODE_F23,
    AM_KEYCODE_F24,
    AM_KEYCODE_F25,
    AM_KEYCODE_PRINT_SCREEN,
    AM_KEYCODE_SCROLL_LOCK,
    AM_KEYCODE_PAUSE,
    AM_KEYCODE_ACCENT_GRAVE,
    AM_KEYCODE_1,
    AM_KEYCODE_2,
    AM_KEYCODE_3,
    AM_KEYCODE_4,
    AM_KEYCODE_5,
    AM_KEYCODE_6,
    AM_KEYCODE_7,
    AM_KEYCODE_8,
    AM_KEYCODE_9,
    AM_KEYCODE_0,
    AM_KEYCODE_MINUS,
    AM_KEYCODE_EQUAL,
    AM_KEYCODE_BACKSPACE,
    AM_KEYCODE_INSERT,
    AM_KEYCODE_HOME,
    AM_KEYCODE_PAGE_UP,
    AM_KEYCODE_NUMPAD_NUM,
    AM_KEYCODE_NUMPAD_DIVIDE,
    AM_KEYCODE_NUMPAD_MULTIPLY,
    AM_KEYCODE_NUMPAD_SUBTRACT,
    AM_KEYCODE_TAB,
    AM_KEYCODE_Q,
    AM_KEYCODE_W,
    AM_KEYCODE_E,
    AM_KEYCODE_R,
    AM_KEYCODE_T,
    AM_KEYCODE_Y,
    AM_KEYCODE_U,
    AM_KEYCODE_I,
    AM_KEYCODE_O,
    AM_KEYCODE_P,
    AM_KEYCODE_LEFT_SQUARE_BRACKET,
    AM_KEYCODE_RIGHT_SQUARE_BRACKET,
    AM_KEYCODE_BACKSLASH,
    AM_KEYCODE_DELETE,
    AM_KEYCODE_END,
    AM_KEYCODE_PAGE_DOWN,
    AM_KEYCODE_NUMPAD_7,
    AM_KEYCODE_NUMPAD_8,
    AM_KEYCODE_NUMPAD_9,
    AM_KEYCODE_CAPS_LOCK,
    AM_KEYCODE_A,
    AM_KEYCODE_S,
    AM_KEYCODE_D,
    AM_KEYCODE_F,
    AM_KEYCODE_G,
    AM_KEYCODE_H,
    AM_KEYCODE_J,
    AM_KEYCODE_K,
    AM_KEYCODE_L,
    AM_KEYCODE_SEMICOLON,
    AM_KEYCODE_APOSTROPHE,
    AM_KEYCODE_ENTER,
    AM_KEYCODE_NUMPAD_4,
    AM_KEYCODE_NUMPAD_5,
    AM_KEYCODE_NUMPAD_6,
    AM_KEYCODE_NUMPAD_ADD,
    AM_KEYCODE_LEFT_SHIFT,
    AM_KEYCODE_Z,
    AM_KEYCODE_X,
    AM_KEYCODE_C,
    AM_KEYCODE_V,
    AM_KEYCODE_B,
    AM_KEYCODE_N,
    AM_KEYCODE_M,
    AM_KEYCODE_COMMA,
    AM_KEYCODE_PERIOD,
    AM_KEYCODE_SLASH,
    AM_KEYCODE_RIGHT_SHIFT,
    AM_KEYCODE_UP_ARROW,
    AM_KEYCODE_NUMPAD_1,
    AM_KEYCODE_NUMPAD_2,
    AM_KEYCODE_NUMPAD_3,
    AM_KEYCODE_LEFT_CONTROL,
    AM_KEYCODE_LEFT_META,
    AM_KEYCODE_LEFT_ALT,
    AM_KEYCODE_SPACE,
    AM_KEYCODE_RIGHT_ALT,
    AM_KEYCODE_RIGHT_META,
    AM_KEYCODE_MENU,
    AM_KEYCODE_RIGHT_CONTROL,
    AM_KEYCODE_LEFT_ARROW,
    AM_KEYCODE_DOWN_ARROW,
    AM_KEYCODE_RIGHT_ARROW,
    AM_KEYCODE_NUMPAD_0,
    AM_KEYCODE_NUMPAD_DECIMAL,
    AM_KEYCODE_NUMPAD_EQUAL,
    AM_KEYCODE_NUMPAD_ENTER,
    AM_KEYCODE_COUNT
} am_key_map;

typedef enum am_mouse_map {
    AM_MOUSE_BUTTON_INVALID,
    AM_MOUSE_BUTTON_LEFT,
    AM_MOUSE_BUTTON_RIGHT,
    AM_MOUSE_BUTTON_MIDDLE,
    AM_MOUSE_BUTTON_COUNT
} am_mouse_map;

typedef enum am_platform_events {
    AM_EVENT_INVALID,
    AM_EVENT_KEY_PRESS,
    AM_EVENT_KEY_RELEASE,
    AM_EVENT_MOUSE_MOTION,
    AM_EVENT_MOUSE_BUTTON_PRESS,
    AM_EVENT_MOUSE_BUTTON_RELEASE,
    AM_EVENT_MOUSE_SCROLL_UP,
    AM_EVENT_MOUSE_SCROLL_DOWN,
    AM_EVENT_WINDOW_SIZE,
    AM_EVENT_WINDOW_MOTION,
    AM_EVENT_COUNT
} am_platform_events;

typedef struct am_window_info {
    am_uint64 parent;
    char name[AM_MAX_NAME_LENGTH];
    am_uint32 width;
    am_uint32 height;
    am_uint32 x;
    am_uint32 y;
    am_bool is_fullscreen;
} am_window_info;

typedef am_window_info am_window_cache;

typedef struct am_window {
    am_uint64 handle;
    am_id id;
    am_uint64 parent;
    char name[AM_MAX_NAME_LENGTH];
    am_uint32 width;
    am_uint32 height;
    am_uint32 x;
    am_uint32 y;
    am_bool is_fullscreen;
    am_window_cache cache;

#if defined(AM_LINUX)
    Colormap colormap;
    XVisualInfo *visual_info;
    GLXContext context;
#else
    HDC hdc;
    HGLRC context;
#endif
} am_window;

typedef struct am_platform_callbacks {
    void (*am_platform_key_callback)(am_id, am_key_map, am_platform_events);
    void (*am_platform_mouse_button_callback)(am_id, am_mouse_map, am_platform_events);
    void (*am_platform_mouse_motion_callback)(am_id, am_int32, am_int32, am_platform_events);
    void (*am_platform_mouse_scroll_callback)(am_id, am_platform_events);
    void (*am_platform_window_size_callback)(am_id, am_uint32, am_uint32, am_platform_events);
    void (*am_platform_window_motion_callback)(am_id, am_uint32, am_uint32, am_platform_events);
} am_platform_callbacks;

typedef struct am_platform_input {
    struct {
        am_key_map keycodes[AM_MAX_KEYCODE_COUNT]; //LUT
        am_bool map[AM_KEYCODE_COUNT];
        am_bool prev_map[AM_KEYCODE_COUNT];
    } keyboard;
    struct {
        am_bool map[AM_MOUSE_BUTTON_COUNT];
        am_bool prev_map[AM_MOUSE_BUTTON_COUNT];
        am_int32 wheel_delta;
        am_vec2u position;
        am_vec2u cached_position;
        am_vec2 delta;
        am_bool locked;
        am_bool moved;
    } mouse;
} am_platform_input;

typedef struct am_platform_time {
    am_uint64 offset;
    am_uint64 frequency;
    am_float64 current;
    am_float64 update;
    am_float64 previous;
    am_float64 render;
    am_float64 frame;
    am_float64 delta;
    am_float64 target;
} am_platform_time;

typedef struct am_platform {
#if defined(AM_LINUX)
    Display *display;
#endif
    am_packed_array(am_window) windows;
    am_platform_input input;
    am_platform_time time;
    am_platform_callbacks callbacks;
} am_platform;


//Platform
#if defined(AM_LINUX)
am_key_map am_platform_translate_keysym(const KeySym *key_syms, am_int32 width);
#endif
am_mouse_map am_platform_translate_button(am_uint32 button);
am_platform *am_platform_create();
void am_platform_poll_events();
#if defined(AM_WINDOWS)
LRESULT CALLBACK am_platform_event_handler(HWND handle, am_uint32 event, WPARAM wparam, LPARAM lparam);
#else
void am_platform_event_handler(XEvent *xevent);
#endif
void am_platform_update(am_platform *platform);
void am_platform_terminate(am_platform *platform);

//Keyboard input
void am_platform_key_press(am_key_map key);
void am_platform_key_release(am_key_map key);
am_bool am_platform_key_pressed(am_key_map key);
am_bool am_platform_key_down(am_key_map key);
am_bool am_platform_key_released(am_key_map key);
am_bool am_platform_key_up(am_key_map key);

//Mouse input
void am_platform_mouse_button_press(am_mouse_map button);
void am_platform_mouse_button_release(am_mouse_map button);
am_bool am_platform_mouse_button_pressed(am_mouse_map button);
am_bool am_platform_mouse_button_down(am_mouse_map button);
am_bool am_platform_mouse_button_released(am_mouse_map button);
am_bool am_platform_mouse_button_up(am_mouse_map button);
void am_platform_mouse_get_position(am_uint32 *x, am_uint32 *y);
am_vec2 am_platform_mouse_get_positionv();
void am_platform_mouse_set_position(am_uint32 x, am_uint32 y);
am_vec2 am_platform_mouse_get_delta();
am_int32 am_platform_mouse_get_wheel_delta();
am_bool am_platform_mouse_moved();
void am_platform_mouse_lock(am_bool lock);


//Platform default callbacks
void am_platform_key_callback_default(am_id id, am_key_map key, am_platform_events event);
void am_platform_mouse_button_callback_default(am_id id, am_mouse_map button, am_platform_events event);
void am_platform_mouse_motion_callback_default(am_id id, am_int32 x, am_int32 y, am_platform_events event);
void am_platform_mouse_scroll_callback_default(am_id id, am_platform_events event);
void am_platform_window_size_callback_default(am_id id, am_uint32 width, am_uint32 height, am_platform_events event);
void am_platform_window_motion_callback_default(am_id id, am_uint32 x, am_uint32 y, am_platform_events event);

#define am_platform_set_key_callback(platform, callback) platform->callbacks.am_platform_key_callback = callback
#define am_platform_set_mouse_button_callback(platform, callback) platform->callbacks.am_platform_mouse_button_callback = callback
#define am_platform_set_mouse_motion_callback(platform, callback) platform->callbacks.am_platform_mouse_motion_callback = callback
#define am_platform_set_mouse_scroll_callback(platform, callback) platform->callbacks.am_platform_mouse_scroll_callback = callback
#define am_platform_set_window_size_callback(platform, callback) platform->callbacks.am_platform_window_size_callback = callback
#define am_platform_set_window_motion_callback(platform, callback) platform->callbacks.am_platform_window_motion_callback = callback

//Windows
am_id am_platform_window_create(am_window_info window_info);
void am_platform_window_resize(am_id id, am_uint32 width, am_uint32 height);
void am_platform_window_move(am_id id, am_uint32 x, am_uint32 y);
void am_platform_window_fullscreen(am_id id, am_bool state);
am_vec2u am_platform_window_get_size(am_id id);
void am_platform_window_destroy(am_id id);

//Time
void am_platform_timer_create();
void am_platform_timer_sleep(am_float32 ms);
am_uint64 am_platform_timer_value();
am_uint64 am_platform_elapsed_time();

//----------------------------------------------------------------------------//
//                                END PLATFORM                                //
//----------------------------------------------------------------------------//


//----------------------------------------------------------------------------//
//                                  START GL                                  //
//----------------------------------------------------------------------------//

//Shader & program

#define AMGL_SHADER_DEFAULT_NAME "amgl_shader"

#define AMGL_VERTEX_BUFFER_DEFAULT_NAME "amgl_vertex_buffer"
#define AMGL_INDEX_BUFFER_DEFAULT_NAME "amgl_index_buffer"
#define AMGL_UNIFORM_DEFAULT_NAME "amgl_uniform"

#define AMGL_TEXTURE_DEFAULT_NAME "amgl_texture"
#define AMGL_TEXTURE_DEFAULT_WIDTH 500
#define AMGL_TEXTURE_DEFAULT_HEIGHT 500
#define AMGL_TEXTURE_DEFAULT_WRAP AMGL_TEXTURE_WRAP_REPEAT
#define AMGL_TEXTURE_DEFAULT_FORMAT AMGL_TEXTURE_FORMAT_RGBA8

#define AMGL_FRAME_BUFFER_DEFAULT_NAME "amgl_frame_buffer"
#define AMGL_RENDER_PASS_DEFAULT_NAME "amgl_render_pass"
#define AMGL_PIPELINE_DEFAULT_NAME "amgl_pipeline"


typedef enum amgl_shader_type {
    AMGL_SHADER_INVALID,
    AMGL_SHADER_VERTEX,
    AMGL_SHADER_GEOMETRY,
    AMGL_SHADER_COMPUTE,
    AMGL_SHADER_FRAGMENT
} amgl_shader_type;

typedef struct amgl_shader_source_info {
    amgl_shader_type type;
    char *source;
    char *path;
} amgl_shader_source_info;

typedef struct amgl_shader_info {
    char name[AM_MAX_NAME_LENGTH];
    amgl_shader_source_info *sources;
    am_uint32 num_sources;
} amgl_shader_info;

typedef struct amgl_shader {
    char name[AM_MAX_NAME_LENGTH];
    am_uint32 handle;
    am_id id;
} amgl_shader;

typedef enum amgl_buffer_usage {
    AMGL_BUFFER_USAGE_INVALID,
    AMGL_BUFFER_USAGE_STATIC,
    AMGL_BUFFER_USAGE_DYNAMIC,
    AMGL_BUFFER_USAGE_STREAM
} amgl_buffer_usage;

typedef enum amgl_buffer_access {
    AMGL_BUFFER_ACCESS_INVALID,
    AMGL_BUFFER_ACCESS_READ_ONLY,
    AMGL_BUFFER_ACCESS_WRITE_ONLY,
    AMGL_BUFFER_ACCESS_READ_WRITE
} amgl_buffer_access;

/*
typedef enum amgl_buffer_update_type {
    AMGL_BUFFER_UPDATE_SUBDATA,
    AMGL_BUFFER_UPDATE_RECREATE
} amgl_buffer_update_type;
*/

typedef enum amgl_vertex_buffer_attribute_format {
    AMGL_VERTEX_BUFFER_ATTRIBUTE_INVALID,
    AMGL_VERTEX_BUFFER_ATTRIBUTE_FLOAT,
    AMGL_VERTEX_BUFFER_ATTRIBUTE_FLOAT2,
    AMGL_VERTEX_BUFFER_ATTRIBUTE_FLOAT3,
    AMGL_VERTEX_BUFFER_ATTRIBUTE_FLOAT4,
    AMGL_VERTEX_BUFFER_ATTRIBUTE_UINT,
    AMGL_VERTEX_BUFFER_ATTRIBUTE_UINT2,
    AMGL_VERTEX_BUFFER_ATTRIBUTE_UINT3,
    AMGL_VERTEX_BUFFER_ATTRIBUTE_UINT4,
    AMGL_VERTEX_BUFFER_ATTRIBUTE_BYTE,
    AMGL_VERTEX_BUFFER_ATTRIBUTE_BYTE2,
    AMGL_VERTEX_BUFFER_ATTRIBUTE_BYTE3,
    AMGL_VERTEX_BUFFER_ATTRIBUTE_BYTE4
} amgl_vertex_buffer_attribute_format;

typedef struct amgl_vertex_buffer_attribute {
    size_t stride;
    size_t offset;
    amgl_vertex_buffer_attribute_format format;
    am_uint32 buffer_index;
} amgl_vertex_buffer_attribute;

typedef struct amgl_vertex_buffer_layout {
    amgl_vertex_buffer_attribute *attributes;
    am_uint32 num_attribs;
} amgl_vertex_buffer_layout;

typedef struct amgl_vertex_buffer_update_info {
    void *data;
    size_t size;
    amgl_buffer_usage usage;
    size_t offset;
} amgl_vertex_buffer_update_info;

typedef struct amgl_vertex_buffer_info {
    char name[AM_MAX_NAME_LENGTH];
    void *data;
    size_t size;
    amgl_buffer_usage usage;
} amgl_vertex_buffer_info;

typedef struct amgl_vertex_buffer {
    char name[AM_MAX_NAME_LENGTH];
    am_uint32 handle;
    am_id id;
    struct {
        size_t size;
        amgl_buffer_usage usage;
    } update;
} amgl_vertex_buffer;

typedef struct amgl_index_buffer_update_info {
    void *data;
    size_t size;
    amgl_buffer_usage usage;
    size_t offset;
} amgl_index_buffer_update_info;

typedef struct amgl_index_buffer_info {
    char name[AM_MAX_NAME_LENGTH];
    void *data;
    size_t size;
    size_t offset;
    amgl_buffer_usage usage;
} amgl_index_buffer_info;

typedef struct amgl_index_buffer {
    char name[AM_MAX_NAME_LENGTH];
    am_id id;
    am_uint32 handle;
    struct {
        size_t size;
        amgl_buffer_usage usage;
    } update;
} amgl_index_buffer;

typedef struct amgl_storage_buffer_update_info {
    void *data;
    size_t size;
    amgl_buffer_usage usage;
    size_t offset;
} amgl_storage_buffer_update_info;

typedef struct amgl_storage_buffer_info {
    char name[AM_MAX_NAME_LENGTH];
    void *data;
    size_t size;
    amgl_buffer_usage usage;
} amgl_storage_buffer_info;

typedef struct amgl_storage_buffer {
    char name[AM_MAX_NAME_LENGTH];
    am_id id;
    am_uint32 handle;
    am_uint32 block_index;
    amgl_buffer_access access;
    struct {
        size_t size;
        amgl_buffer_usage usage;
    } update;
} amgl_storage_buffer;


typedef enum amgl_uniform_type {
    AMGL_UNIFORM_TYPE_INVALID,
    AMGL_UNIFORM_TYPE_FLOAT,
    AMGL_UNIFORM_TYPE_INT,
    AMGL_UNIFORM_TYPE_VEC2,
    AMGL_UNIFORM_TYPE_VEC3,
    AMGL_UNIFORM_TYPE_VEC4,
    AMGL_UNIFORM_TYPE_MAT3,
    AMGL_UNIFORM_TYPE_MAT4,
    AMGL_UNIFORM_TYPE_SAMPLER2D
} amgl_uniform_type;

typedef struct amgl_uniform_info {
    char name[AM_MAX_NAME_LENGTH];
    amgl_uniform_type type;
} amgl_uniform_info;

typedef struct amgl_uniform {
    char name[AM_MAX_NAME_LENGTH];
    am_id id;
    am_uint32 location;
    void* data;
    size_t size;
    amgl_uniform_type type;
    am_id shader_id;
} amgl_uniform;

//Textures
typedef enum amgl_texture_format {
    AMGL_TEXTURE_FORMAT_INVALID,
    AMGL_TEXTURE_FORMAT_RGBA8,
    AMGL_TEXTURE_FORMAT_RGB8,
    AMGL_TEXTURE_FORMAT_RGBA16F,
    AMGL_TEXTURE_FORMAT_RGBA32F,
    AMGL_TEXTURE_FORMAT_RGBA,
    AMGL_TEXTURE_FORMAT_A8,
    AMGL_TEXTURE_FORMAT_R8,
    AMGL_TEXTURE_FORMAT_DEPTH8,
    AMGL_TEXTURE_FORMAT_DEPTH16,
    AMGL_TEXTURE_FORMAT_DEPTH24,
    AMGL_TEXTURE_FORMAT_DEPTH32F,
    AMGL_TEXTURE_FORMAT_DEPTH24_STENCIL8,
    AMGL_TEXTURE_FORMAT_DEPTH32F_STENCIL8,
    AMGL_TEXTURE_FORMAT_STENCIL8
} amgl_texture_format;

typedef enum amgl_texture_wrap {
    AMGL_TEXTURE_WRAP_INVALID,
    AMGL_TEXTURE_WRAP_REPEAT,
    AMGL_TEXTURE_WRAP_MIRRORED_REPEAT,
    AMGL_TEXTURE_WRAP_CLAMP_TO_EDGE,
    AMGL_TEXTURE_WRAP_CLAMP_TO_BORDER
} amgl_texture_wrap;

typedef enum amgl_texture_filter {
    AMGL_TEXTURE_FILTER_INVALID,
    AMGL_TEXTURE_FILTER_NEAREST,
    AMGL_TEXTURE_FILTER_LINEAR
} amgl_texture_filter;

typedef struct amgl_texture_info {
    char name[AM_MAX_NAME_LENGTH];
    void *data;
    char *path;
    am_uint32 width;
    am_uint32 height;
    amgl_texture_format format;
    amgl_texture_filter min_filter;
    amgl_texture_filter mag_filter;
    amgl_texture_filter mip_filter;
    am_uint32 mip_num;
    amgl_texture_wrap wrap_s;
    amgl_texture_wrap wrap_t;
    am_bool save_reference;
} amgl_texture_info;

typedef struct amgl_texture {
    char name[AM_MAX_NAME_LENGTH];
    am_uint32 handle;
    am_id id;
    amgl_texture_format format;
    struct {
        am_bool is_saved;
        am_uint32 width;
        am_uint32 height;
        void *image;
    } refference;
} amgl_texture;

typedef struct amgl_frame_buffer_info {
    char name[AM_MAX_NAME_LENGTH];
    am_uint32 width;
    am_uint32 height;
} amgl_frame_buffer_info;

typedef struct amgl_frame_buffer {
    char name[AM_MAX_NAME_LENGTH];
    am_id id;
    am_uint32 handle;
} amgl_frame_buffer;

typedef enum amgl_blend_func {
    AMGL_BLEND_FUNC_INVALID,
    AMGL_BLEND_FUNC_ADD,
    AMGL_BLEND_FUNC_SUBSTRACT,
    AMGL_BLEND_FUNC_REVERSE_SUBSTRACT,
    AMGL_BLEND_FUNC_MIN,
    AMGL_BLEND_FUNC_MAX
} amgl_blend_func;

typedef enum amgl_blend_mode {
    AMGL_BLEND_MODE_INVALID,
    AMGL_BLEND_MODE_ZERO,
    AMGL_BLEND_MODE_ONE,
    AMGL_BLEND_MODE_SRC_COLOR,
    AMGL_BLEND_MODE_ONE_MINUS_SRC_COLOR,
    AMGL_BLEND_MODE_DST_COLOR,
    AMGL_BLEND_MODE_ONE_MINUS_DST_COLOR,
    AMGL_BLEND_MODE_SRC_ALPHA,
    AMGL_BLEND_MODE_ONE_MINUS_SRC_ALPHA,
    AMGL_BLEND_MODE_DST_ALPHA,
    AMGL_BLEND_MODE_ONE_MINUS_DST_ALPHA,
    AMGL_BLEND_MODE_CONSTANT_COLOR,
    AMGL_BLEND_MODE_ONE_MINUS_CONSTANT_COLOR,
    AMGL_BLEND_MODE_CONSTANT_ALPHA,
    AMGL_BLEND_MODE_ONE_MINUS_CONSTANT_ALPHA
} amgl_blend_mode;

typedef struct amgl_blend_info {
    amgl_blend_func func;
    amgl_blend_mode src;
    amgl_blend_mode dst;
} amgl_blend_info;

typedef enum amgl_depth_func {
    AMGL_DEPTH_FUNC_INVALID,
    AMGL_DEPTH_FUNC_NEVER,
    AMGL_DEPTH_FUNC_LESS,
    AMGL_DEPTH_FUNC_EQUAL,
    AMGL_DEPTH_FUNC_LEQUAL,
    AMGL_DEPTH_FUNC_GREATER,
    AMGL_DEPTH_FUNC_NOTEQUAL,
    AMGL_DEPTH_FUNC_GEQUAL,
    AMGL_DEPTH_FUNC_ALWAYS
} amgl_depth_func;

typedef struct amgl_depth_info {
    amgl_depth_func func;
} amgl_depth_info;

typedef enum amgl_stencil_func {
    AMGL_STENCIL_FUNC_INVALID,
    AMGL_STENCIL_FUNC_NEVER,
    AMGL_STENCIL_FUNC_LESS,
    AMGL_STENCIL_FUNC_EQUAL,
    AMGL_STENCIL_FUNC_LEQUAL,
    AMGL_STENCIL_FUNC_GREATER,
    AMGL_STENCIL_FUNC_NOTEQUAL,
    AMGL_STENCIL_FUNC_GEQUAL,
    AMGL_STENCIL_FUNC_ALWAYS
} amgl_stencil_func;

typedef enum amgl_stencil_op {
    AMGL_STENCIL_OP_INVALID,
    AMGL_STENCIL_OP_KEEP,
    AMGL_STENCIL_OP_ZERO,
    AMGL_STENCIL_OP_REPLACE,
    AMGL_STENCIL_OP_INCR,
    AMGL_STENCIL_OP_INCR_WRAP,
    AMGL_STENCIL_OP_DECR,
    AMGL_STENCIL_OP_DECR_WRAP,
    AMGL_STENCIL_OP_INVERT
} amgl_stencil_op;

typedef struct amgl_stencil_info {
    amgl_stencil_func func;
    am_int32 ref;
    am_int32 comp_mask;
    am_int32 write_mask;
    amgl_stencil_op sfail;
    amgl_stencil_op dpfail;
    amgl_stencil_op dppass;
} amgl_stencil_info;

typedef enum amgl_face_cull {
    AMGL_FACE_CULL_INVALID,
    AMGL_FACE_CULL_FRONT,
    AMGL_FACE_CULL_BACK,
    AMGL_FACE_CULL_FRONT_AND_BACK
} amgl_face_cull;

typedef enum amgl_winding_order {
    AMGL_WINDING_ORDER_INVALID,
    AMGL_WINDING_ORDER_CCW,
    AMGL_WINDING_ORDER_CW
} amgl_winding_order;

typedef enum amgl_primitive {
    AMGL_PRIMITIVE_INVALID,
    AMGL_PRIMITIVE_LINES,
    AMGL_PRIMITIVE_TRIANGLES,
    AMGL_PRIMITIVE_QUADS
} amgl_primitive;

typedef struct amgl_raster_info {
    amgl_face_cull face_culling;
    amgl_winding_order winding_order;
    amgl_primitive primitive;
    am_id shader_id;
    am_int32 index_buffer_element_size;
} amgl_raster_info;

typedef struct amgl_compute_info {
    am_id compute_shader;
} amgl_compute_info;

typedef struct amgl_pipeline_info {
    char name[AM_MAX_NAME_LENGTH];
    amgl_blend_info blend;
    amgl_depth_info depth;
    amgl_raster_info raster;
    amgl_stencil_info stencil;
    amgl_compute_info compute;
    amgl_vertex_buffer_layout layout;
} amgl_pipeline_info;

typedef struct amgl_pipeline {
    char name[AM_MAX_NAME_LENGTH];
    am_id id;
    amgl_blend_info blend;
    amgl_depth_info depth;
    amgl_raster_info raster;
    amgl_stencil_info stencil;
    amgl_compute_info compute;
    amgl_vertex_buffer_layout layout;
} amgl_pipeline;

typedef struct amgl_render_pass_info {
    char name[AM_MAX_NAME_LENGTH];
    am_id framebuffer_id;
    am_id *color_texture_ids;
    am_uint32 num_colors;
    am_id depth_texture_id;
    am_id stencil_texture_id;
} amgl_render_pass_info;

typedef struct amgl_render_pass {
    char name[AM_MAX_NAME_LENGTH];
    am_id id;
    am_id framebuffer_id;
    am_id *color_texture_ids;
    am_uint32 num_colors;
    am_id depth_texture_id;
    am_id stencil_texture_id;
} amgl_render_pass;

typedef struct amgl_vertex_buffer_bind_info {
    am_id vertex_buffer_id;
} amgl_vertex_buffer_bind_info;

typedef struct amgl_index_buffer_bind_info {
    am_id index_buffer_id;
} amgl_index_buffer_bind_info;

typedef struct amgl_storage_buffer_bind_info {
    am_id storage_buffer_id;
    am_uint32 binding;
} amgl_storage_buffer_bind_info;

typedef struct amgl_texture_bind_info {
    am_id texture_id;
    am_uint32 binding;
    amgl_buffer_access access;
} amgl_texture_bind_info;

typedef struct amgl_uniform_bind_info {
    am_id uniform_id;
    void *data;
    am_uint32 binding;
} amgl_uniform_bind_info;

typedef struct amgl_bindings_info {
    struct {
        amgl_vertex_buffer_bind_info *info;
        size_t size;
    } vertex_buffers;

    struct {
        amgl_index_buffer_bind_info *info;
        size_t size;
    } index_buffers;

    struct {
        amgl_storage_buffer_bind_info *info;
        size_t size;
    } storage_buffers;

    struct {
        amgl_uniform_bind_info *info;
        size_t size;
    } uniforms;

    struct {
        amgl_texture_bind_info *info;
        size_t size;
    } images;
} amgl_bindings_info;

typedef struct amgl_frame_cache {
    amgl_index_buffer index_buffer;
    size_t index_element_size;
    am_dyn_array(amgl_vertex_buffer) vertex_buffers;
    amgl_pipeline pipeline;
} amgl_frame_cache;

typedef struct amgl_draw_info {
    am_uint32 start;
    am_uint32 count;
} amgl_draw_info;

typedef enum am_projection_type {
    AM_PROJECTION_TYPE_ORTHOGRAPHIC,
    AM_PROJECTION_TYPE_PERSPECTIVE
} am_projection_type;

typedef struct am_camera {
    am_vqs transform;
    am_float32 fov;
    am_float32 aspect_ratio;
    am_float32 near_plane;
    am_float32 far_plane;
    am_float32 ortho_scale;
    am_projection_type proj_type;
} am_camera;

typedef enum amgl_clear_type {
    AMGL_CLEAR_INVALID,
    AMGL_CLEAR_COLOR,
    AMGL_CLEAR_DEPTH,
    AMGL_CLEAR_STENCIL,
    AMGL_CLEAR_ALL
} amgl_clear_type;

typedef struct amgl_clear_desc {
    union {
        am_float32 color[4];
        am_float32 r, g, b, a;
    };
    amgl_clear_type *types;
    am_uint32 num;
} amgl_clear_desc;

//Shaders
am_id amgl_shader_create(amgl_shader_info info);
void amgl_shader_compute_dispatch(am_int32 x, am_int32 y, am_int32 z);
void amgl_shader_destroy(am_id id);

//Vertex buffer
am_id amgl_vertex_buffer_create(amgl_vertex_buffer_info info);
void amgl_vertex_buffer_update(am_id id, amgl_vertex_buffer_update_info update);
void amgl_vertex_buffer_destroy(am_id id);

//Index buffer
am_id amgl_index_buffer_create(amgl_index_buffer_info info);
void amgl_index_buffer_update(am_id id, amgl_index_buffer_update_info update);
void amgl_index_buffer_destroy(am_id id);

//Storage buffer
am_id amgl_storage_buffer_create(amgl_storage_buffer_info info);
void amgl_storage_buffer_update(am_id id, amgl_storage_buffer_update_info update);
void amgl_storage_buffer_destroy(am_id id);


//Uniform
am_id amgl_uniform_create(amgl_uniform_info info);
void amgl_uniform_destroy(am_id id);

//Texture
am_id amgl_texture_create(amgl_texture_info info);
am_int32 amgl_texture_translate_format(amgl_texture_format format);
am_int32 amgl_texture_translate_wrap(amgl_texture_wrap wrap);
GLenum amgl_texture_translate_filter(amgl_texture_filter filter);
void amgl_texture_load_from_file(const char *path, amgl_texture_info *info, am_bool flip);
void amgl_texture_load_from_memory(const void *memory, amgl_texture_info *info, size_t size, am_bool flip);
void amgl_texture_destroy(am_id id);

//Framebuffer
am_id amgl_frame_buffer_create(amgl_frame_buffer_info info);
void amgl_frame_buffer_destroy(am_id id);

//Render pass
am_id amgl_render_pass_create(amgl_render_pass_info info);
void amgl_render_pass_destroy(am_id id);

//Pipeline
am_id amgl_pipeline_create(amgl_pipeline_info info);
am_int32 amgl_blend_translate_func(amgl_blend_func func);
am_int32 amgl_blend_translate_mode(amgl_blend_mode mode);
am_int32 amgl_depth_translate_func(amgl_depth_func func);
am_int32 amgl_stencil_translate_func(amgl_stencil_func func);
am_int32 amgl_stencil_translate_op(amgl_stencil_op op);
am_int32 amgl_face_cull_translate(amgl_face_cull face_cull);
am_int32 amgl_primitive_translate(amgl_primitive prim);
am_int32 amgl_index_buffer_size_translate(size_t size);
void amgl_pipeline_destroy(am_id id);

//Various OGL
void amgl_clear(amgl_clear_desc info);
void amgl_terminate();
void amgl_set_viewport(am_int32 x, am_int32 y, am_int32 width, am_int32 height);
void amgl_vsync(am_id window_id, am_bool state);
void amgl_start_render_pass(am_id render_pass_id);
void amgl_end_render_pass(am_id render_pass_id);
void amgl_bind_pipeline(am_id pipeline_id);
void amgl_set_bindings(amgl_bindings_info *info);
void amgl_draw(amgl_draw_info *info);

//Camera
am_camera am_camera_default();
am_camera am_camera_perspective();
am_mat4 am_camera_get_view(am_camera* cam);
am_mat4 am_camera_get_proj(am_camera* cam, am_int32 view_width, am_int32 view_height);
am_mat4 am_camera_get_view_projection(am_camera* cam, am_int32 view_width, am_int32 view_height);
am_vec3 am_camera_forward(am_camera* cam);
am_vec3 am_camera_backward(am_camera* cam);
am_vec3 am_camera_up(am_camera* cam);
am_vec3 am_camera_down(am_camera* cam);
am_vec3 am_camera_right(am_camera* cam);
am_vec3 am_camera_left(am_camera* cam);
am_vec3 am_camera_screen_to_world(am_camera* cam, am_vec3 coords, am_int32 view_width, am_int32 view_height);
void am_camera_offset_orientation(am_camera* cam, am_float32 yaw, am_float32 pitch);


//----------------------------------------------------------------------------//
//                                   END GL                                   //
//----------------------------------------------------------------------------//


//----------------------------------------------------------------------------//
//                                   ENGINE                                   //
//----------------------------------------------------------------------------//

typedef struct am_engine_info {
    void (*init)();
    void (*update)();
    void (*shutdown)();
    am_bool is_running;
    am_bool vsync_enabled;
    float desired_fps;
    char win_name[AM_MAX_NAME_LENGTH];
    am_bool win_fullscreen;
    am_uint32 win_width;
    am_uint32 win_height;
    am_uint32 win_x;
    am_uint32 win_y;
} am_engine_info;

typedef struct amgl_ctx_data {
    am_packed_array(amgl_texture) textures;
    am_packed_array(amgl_shader) shaders;
    am_packed_array(amgl_vertex_buffer) vertex_buffers;
    am_packed_array(amgl_index_buffer) index_buffers;
    am_packed_array(amgl_storage_buffer) storage_buffers;
    am_packed_array(amgl_frame_buffer) frame_buffers;
    am_packed_array(amgl_uniform) uniforms;
    am_packed_array(amgl_render_pass) render_passes;
    am_packed_array(amgl_pipeline) pipelines;
    amgl_frame_cache frame_cache;
} amgl_ctx_data;


typedef struct am_engine {
    void (*init)();
    void (*update)();
    void (*shutdown)();
    am_bool is_running;
    am_bool vsync_enabled;
    float desired_fps;
    am_platform *platform;
    amgl_ctx_data ctx_data;
} am_engine;

am_engine *_am_engine_instance;

#define am_engine_get_instance() _am_engine_instance
#define am_engine_get_subsystem(sys) am_engine_get_instance()->sys

void am_engine_create(am_engine_info engine_info);
void am_engine_frame();
void am_engine_quit();
void am_engine_terminate();


//----------------------------------------------------------------------------//
//                                 END ENGINE                                 //
//----------------------------------------------------------------------------//


//----------------------------------------------------------------------------//
//                                    UTIL                                    //
//----------------------------------------------------------------------------//

char* am_util_read_file(const char *path);

#define AM_UTIL_OBJ_MAX_BUFFER_SIZE 65536

typedef struct am_util_obj_vertex {
    am_vec3 position;
    am_int32 texture_index;
    am_int32 normal_index;
    struct am_util_obj_vertex *duplicate_vertex;
    am_int32 index;
} am_util_obj_vertex;

typedef struct am_util_obj {
    am_dyn_array(am_float32) vertices;
    am_dyn_array(am_float32) texture_coords;
    am_dyn_array(am_float32) normals;
    am_dyn_array(am_util_obj_vertex) obj_vertices;
    am_dyn_array(am_int32) indices;
} am_util_obj;

am_util_obj* am_util_obj_create(char *path);
void am_util_obj_handle_duplicate(am_util_obj_vertex *prev_vertex, am_int32 new_tex_idx, am_int32 new_norm_idx, am_int32 **indices, am_util_obj_vertex **vertices);
void am_util_obj_delete_duplicate_chain(am_util_obj_vertex *duplicate);
void am_util_obj_delete(am_util_obj *obj);
void am_util_obj_interpret_buffer(char *start, char *end, am_util_obj *object);

//----------------------------------------------------------------------------//
//                                  END UTIL                                  //
//----------------------------------------------------------------------------//


//----------------------------------------------------------------------------//
//                             DYNAMIC  ARRAY IMPL                            //
//----------------------------------------------------------------------------//

void am_dyn_array_init(void **array, size_t value_size) {
    if (*array != NULL) return;
    am_dyn_array_header *data = (am_dyn_array_header*)malloc(value_size + sizeof(am_dyn_array_header));
    data->capacity = value_size;
    data->size = 0;
    *array = ((size_t*)data + 2);
};

void am_dyn_array_resize(void **array, size_t add_size) {
    am_dyn_array_header *header;
    size_t new_capacity = 0, alloc_size = 0;
    if (am_dyn_array_get_capacity(*array) < am_dyn_array_get_size(*array) + add_size) {
        new_capacity = 2 * am_dyn_array_get_capacity(*array);
        alloc_size = new_capacity + sizeof(am_dyn_array_header);
        header = (am_dyn_array_header*)am_realloc(am_dyn_array_get_header(*array), alloc_size);
        if (!header) {
            printf("[FAIL] am_dyn_array_resize: Failed to allocate memory for array!\n");
            return;
        };
        header->capacity = new_capacity;
        *array = (size_t*)header + 2;
    };
};

void am_dyn_array_replace(void *array, void *values, size_t offset, size_t size) {
    assert(offset + size <= am_dyn_array_get_size(array));
    memcpy(array + offset, values, size);
};

void am_dyn_array_destroy(void *array) {
    am_dyn_array_header *header = am_dyn_array_get_header(array);
    header->capacity = AM_DYN_ARRAY_EMPTY_START_SLOTS;
    header->size = 0;
    am_free(header);
};

//----------------------------------------------------------------------------//
//                           END DYNAMIC  ARRAY IMPL                          //
//----------------------------------------------------------------------------//


//----------------------------------------------------------------------------//
//                             PACKED  ARRAY IMPL                             //
//----------------------------------------------------------------------------//

void am_packed_array_alloc(void **array, size_t size) {
    if (*array == NULL) {
        *array = am_malloc(size);
        memset(*array, 0, size);

    };
};

#define am_packed_array_erase(array, id)\
    do {\
        am_uint32 index = am_packed_array_get_idx(array, id);\
        if (index == AM_PA_INVALID_INDEX) {\
            printf("[WARN] am_packed_array_erase: Element with ID %d does not exist!\n", id);\
            break;\
        };\
        if (!am_packed_array_has((array), (id))) {\
            printf("[WARN] am_packed_array_erase: Invalid id (%d)!\n", id);\
            break;\
        };\
        am_int32 last_element_id = -1;\
        if (am_packed_array_get_idx((array), (id)) == am_dyn_array_get_count((array)->elements) - 1) last_element_id = id;\
        else for (am_int32 i = 0; i < am_dyn_array_get_size(array) / sizeof(am_uint32); i++)\
            if (am_packed_array_get_idx((array), i) == am_dyn_array_get_count((array)->elements) - 1) {\
                last_element_id = i;\
                break;\
            };\
        am_dyn_array_replace((array)->elements, &((array)->elements[am_dyn_array_get_count((array)->elements) - 1]), index*sizeof((array)->elements[0]), sizeof((array)->elements[0]));\
        (array)->indices[last_element_id] = index;\
        (array)->indices[id] = AM_PA_INVALID_INDEX;\
        am_dyn_array_get_size((array)->elements) -= sizeof((array)->elements[0]);\
    } while(0);

//----------------------------------------------------------------------------//
//                           END PACKED  ARRAY IMPL                           //
//----------------------------------------------------------------------------//


//----------------------------------------------------------------------------//
//                                  MATH IMPL                                 //
//----------------------------------------------------------------------------//

// VEC2

static inline am_vec2 am_vec2_create(am_float32 x, am_float32 y) {
    am_vec2 v;
    v.x = x;
    v.y = y;
    return v;
};

static inline am_vec2 am_vec2_add(am_vec2 a, am_vec2 b) {
    return am_vec2_create(a.x + b.x, a.y + b.y);
};

static inline am_vec2 am_vec2_sub(am_vec2 a, am_vec2 b) {
    return am_vec2_create(a.x - b.x, a.y - b.y);
};

static inline am_vec2 am_vec2_mul(am_vec2 a, am_vec2 b) {
    return am_vec2_create(a.x * b.x, a.y * b.y);
};

static inline am_vec2 am_vec2_div(am_vec2 a, am_vec2 b){
    return am_vec2_create(a.x / b.x, a.y / b.y);
};

static inline am_vec2 am_vec2_scale(am_float32 scalar, am_vec2 a){
    return am_vec2_create(a.x * scalar, a.y * scalar);
};

static inline am_float32 am_vec2_dot(am_vec2 a, am_vec2 b) {
    return (am_float32)(a.x * b.x + a.y * b.y);
};

static inline am_float32 am_vec2_len(am_vec2 a) {
    return (am_float32)sqrt(am_vec2_dot(a,a));
};

static inline am_vec2 am_vec2_norm(am_vec2 a) {
    am_float32 len = am_vec2_len(a);
    return am_vec2_scale(len != 0 ? 1.0f / am_vec2_len(a) : 1.0f, a);
};

static inline am_float32 am_vec2_dist(am_vec2 a, am_vec2 b) {
    am_float32 dx = a.x - b.x;
    am_float32  dy = a.y = b.y;
    return (am_float32)sqrt((double)(dx * dx + dy * dy));
};

static inline am_float32 am_vec2_cross(am_vec2 a, am_vec2 b) {
    return a.x * b.y - a.y * b.x;
};

static inline am_float32 am_vec2_angle(am_vec2 a, am_vec2 b) {
    return (am_float32)acos((double)(am_vec2_dot(a, b) / (am_vec2_len(a) * am_vec2_len(b))));
};


//VEC3

static inline am_vec3 am_vec3_create(am_float32 x, am_float32 y, am_float32 z) {
    am_vec3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
};

static inline am_vec3 am_vec3_add(am_vec3 a, am_vec3 b) {
    return am_vec3_create(a.x + b.x, a.y + b.y, a.z + b.z);
};

static inline am_vec3 am_vec3_sub(am_vec3 a, am_vec3 b) {
    return am_vec3_create(a.x - b.x, a.y - b.y, a.z - b.z);
};

static inline am_vec3 am_vec3_mul(am_vec3 a, am_vec3 b) {
    return am_vec3_create(a.x * b.x, a.y * b.y, a.z * b.z);
};

static inline am_vec3 am_vec3_div(am_vec3 a, am_vec3 b) {
    return am_vec3_create(a.x / b.x, a.y / b.y, a.z / b.z);
};

static inline am_vec3 am_vec3_scale(am_float32 scalar, am_vec3 a) {
    return am_vec3_create(a.x * scalar, a.y * scalar, a.z * scalar);
};

static inline am_float32 am_vec3_dot(am_vec3 a, am_vec3 b) {
    return (am_float32)((a.x * b.x) + (a.y * b.y) + a.z * b.z);
};

static inline am_float32 am_vec3_len(am_vec3 a) {
    return (am_float32)sqrt(am_vec3_dot(a,a));
};

static inline am_float32 am_vec3_len_sqr(am_vec3 a) {
    return (am_float32)am_vec3_dot(a,a);
};

static inline am_float32 am_vec3_dist(am_vec3 a, am_vec3 b) {
    am_float32 dx = (a.x - b.x);
    am_float32 dy = (a.y - b.y);
    am_float32 dz = (a.z - b.z);
    return (am_float32)(sqrt((am_float64)(dx * dx + dy * dy + dz * dz)));
};

static inline am_vec3 am_vec3_norm(am_vec3 a) {
    am_float32 len = am_vec3_len(a);
    return len == 0.f ? a : am_vec3_scale(1.f / len, a);
};

static inline am_vec3 am_vec3_cross(am_vec3 a, am_vec3 b) {
    return am_vec3_create(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
};

static inline am_float32 am_vec3_angle_unsigned(am_vec3 a, am_vec3 b) {
    return acosf(am_vec3_dot(a,b));
};

static inline am_float32 am_vec3_angle_signed(am_vec3 a, am_vec3 b) {
    return asinf(am_vec3_len(am_vec3_cross(a, b)));
};

static inline am_vec3 am_vec3_triple_cross(am_vec3 a, am_vec3 b, am_vec3 c) {
    return am_vec3_sub((am_vec3_scale( am_vec3_dot(c, a), b)), (am_vec3_scale(am_vec3_dot(c, b), a)));

};

// VEC4

static inline am_vec4 am_vec4_create(am_float32 x, am_float32 y, am_float32 z, am_float32 w) {
    am_vec4 v;
    v.x = x;
    v.y = y;
    v.z = z;
    v.w = w;
    return v;
};

static inline am_vec4 am_vec4_add(am_vec4 a, am_vec4 b) {
    return am_vec4_create(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
};

static inline am_vec4 am_vec4_sub(am_vec4 a, am_vec4 b) {
    return am_vec4_create(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
};

static inline am_vec4 am_vec4_mul(am_vec4 a, am_vec4 b) {
    return am_vec4_create(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
};

static inline am_vec4 am_vec4_div(am_vec4 a, am_vec4 b) {
    return am_vec4_create(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);
};

static inline am_vec4 am_vec4_scale(am_float32 scalar, am_vec4 a) {
    return am_vec4_create(a.x * scalar, a.y * scalar, a.z * scalar, a.w * scalar);
};

static inline am_float32 am_vec4_dot(am_vec4 a, am_vec4 b) {
    return (am_float32)(a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w);
};

static inline am_float32 am_vec4_len(am_vec4 a) {
    return (am_float32)sqrt(am_vec4_dot(a, a));
};

static inline am_float32 am_vec4_dist(am_vec4 a, am_vec4 b) {
    am_float32 dx = (a.x - b.x);
    am_float32 dy = (a.y - b.y);
    am_float32 dz = (a.z - b.z);
    am_float32 dw = (a.w - b.w);
    return (am_float32)(sqrt((double)(dx * dx + dy * dy + dz * dz + dw * dw)));
};

static inline am_vec4 am_vec4_norm(am_vec4 a) {
    return am_vec4_scale(1.0f / am_vec4_len(a), a);
};

// GENERAL VECTORS

static inline am_vec3 am_vec4_to_vec3(am_vec4 a) {
    return am_vec3_create(a.x, a.y, a.z);
};

static inline am_vec2 am_vec3_to_vec2(am_vec4 a) {
    return am_vec2_create(a.x, a.y);
};

// MAT 3x3

static inline am_mat3 am_mat3_create() {
    am_mat3 mat = {0};
    return mat;
};

static inline am_mat3 am_mat3_diag(am_float32 val) {
    am_mat3 m = {0};
    m.m[0 + 0 * 3] = val;
    m.m[1 + 1 * 3] = val;
    m.m[2 + 2 * 3] = val;
    return m;
};

static inline am_mat3 am_mat3_mul(am_mat3 m0, am_mat3 m1) {
    am_mat3 m = {0};
    for (am_uint32 y = 0; y < 3; ++y) {
        for (am_uint32 x = 0; x < 3; ++x) {
            am_float32 sum = 0.0f;
            for (am_uint32 e = 0; e < 3; ++e) {
                sum += m0.m[x + e * 3] * m1.m[e + y * 3];
            };
            m.m[x + y * 3] = sum;
        };
    };
    return m;
};

static inline am_vec3 am_mat3_mul_vec3(am_mat3 m, am_vec3 v) {
    return am_vec3_create(
        m.m[0] * v.x + m.m[1] * v.y + m.m[2] * v.z,
        m.m[3] * v.x + m.m[4] * v.y + m.m[5] * v.z,
        m.m[6] * v.x + m.m[7] * v.y + m.m[8] * v.z
    );
};

static inline am_mat3 am_mat3_scale(am_float32 x, am_float32 y, am_float32 z) {
    am_mat3 m = {0};
    m.m[0] = x;
    m.m[4] = y;
    m.m[8] = z;
    return m;
}

static inline am_mat3 am_mat3_rotate(am_float32 radians, am_float32 x, am_float32 y, am_float32 z) {
    am_mat3 m = {0};
    am_float32 s = sinf(radians), c = cosf(radians), c1 = 1.f - c;
    am_float32 xy = x * y;
    am_float32 yz = y * z;
    am_float32 zx = z * x;
    am_float32 xs = x * s;
    am_float32 ys = y * s;
    am_float32 zs = z * s;
    m.m[0] = c1 * x * x + c; m.m[1] = c1 * xy - zs;   m.m[2] = c1 * zx + ys;
    m.m[3] = c1 * xy + zs;   m.m[4] = c1 * y * y + c; m.m[5] = c1 * yz - xs;
    m.m[6] = c1 * zx - ys;   m.m[7] = c1 * yz + xs;   m.m[8] = c1 * z * z + c;
    return m;
};

static inline am_mat3 am_mat3_rotatev(am_float32 radians, am_vec3 axis) {
    return am_mat3_rotate(radians, axis.x, axis.y, axis.z);
};

// Turn quaternion into mat3
static inline am_mat3 am_mat3_rotateq(am_vec4 q) {
    am_mat3 m = {0};
    am_float32 x2 = q.x * q.x, y2 = q.y * q.y, z2 = q.z * q.z, w2 = q.w * q.w;
    am_float32 xz = q.x  *q.z, xy = q.x * q.y, yz = q.y * q.z, wz = q.w * q.z, wy = q.w * q.y, wx = q.w * q.x;
    m.m[0] = 1 - 2 * (y2 + z2); m.m[1] = 2 * (xy + wz);     m.m[2] = 2 * (xz - wy);
    m.m[3] = 2 * (xy - wz);     m.m[4] = 1 - 2 * (x2 + z2); m.m[5] = 2 * (yz + wx);
    m.m[6] = 2 * (xz + wy);     m.m[7] = 2 * (yz - wx);     m.m[8] = 1 - 2 * (x2 + y2);
    return m;
};

static inline am_mat3 am_mat3_rsq(am_vec4 q, am_vec3 s) {
    am_mat3 mr = am_mat3_rotateq(q);
    am_mat3 ms = am_mat3_scale(s.x, s.y, s.z);
    return am_mat3_mul(mr, ms);
};

static inline am_mat3 am_mat3_inverse(am_mat3 m) {
    am_mat3 r = {0};

    am_float64 det = (am_float64)(m.m[0 * 3 + 0] * (m.m[1 * 3 + 1] * m.m[2 * 3 + 2] - m.m[2 * 3 + 1] * m.m[1 * 3 + 2]) -
                                  m.m[0 * 3 + 1] * (m.m[1 * 3 + 0] * m.m[2 * 3 + 2] - m.m[1 * 3 + 2] * m.m[2 * 3 + 0]) +
                                  m.m[0 * 3 + 2] * (m.m[1 * 3 + 0] * m.m[2 * 3 + 1] - m.m[1 * 3 + 1] * m.m[2 * 3 + 0])
    );

    am_float64 inv_det = det ? 1.0 / det : 0.0;

    r.m[0 * 3 + 0] = (am_float32)((m.m[1 * 3 + 1] * m.m[2 * 3 + 2] - m.m[2 * 3 + 1] * m.m[1 * 3 + 2]) * inv_det);
    r.m[0 * 3 + 1] = (am_float32)((m.m[0 * 3 + 2] * m.m[2 * 3 + 1] - m.m[0 * 3 + 1] * m.m[2 * 3 + 2]) * inv_det);
    r.m[0 * 3 + 2] = (am_float32)((m.m[0 * 3 + 1] * m.m[1 * 3 + 2] - m.m[0 * 3 + 2] * m.m[1 * 3 + 1]) * inv_det);
    r.m[1 * 3 + 0] = (am_float32)((m.m[1 * 3 + 2] * m.m[2 * 3 + 0] - m.m[1 * 3 + 0] * m.m[2 * 3 + 2]) * inv_det);
    r.m[1 * 3 + 1] = (am_float32)((m.m[0 * 3 + 0] * m.m[2 * 3 + 2] - m.m[0 * 3 + 2] * m.m[2 * 3 + 0]) * inv_det);
    r.m[1 * 3 + 2] = (am_float32)((m.m[1 * 3 + 0] * m.m[0 * 3 + 2] - m.m[0 * 3 + 0] * m.m[1 * 3 + 2]) * inv_det);
    r.m[2 * 3 + 0] = (am_float32)((m.m[1 * 3 + 0] * m.m[2 * 3 + 1] - m.m[2 * 3 + 0] * m.m[1 * 3 + 1]) * inv_det);
    r.m[2 * 3 + 1] = (am_float32)((m.m[2 * 3 + 0] * m.m[0 * 3 + 1] - m.m[0 * 3 + 0] * m.m[2 * 3 + 1]) * inv_det);
    r.m[2 * 3 + 2] = (am_float32)((m.m[0 * 3 + 0] * m.m[1 * 3 + 1] - m.m[1 * 3 + 0] * m.m[0 * 3 + 1]) * inv_det);

    return r;
};

// MAT4

static inline am_mat4 am_mat4_create() {
    am_mat4 mat = {0};
    return mat;
};

static inline am_mat4 am_mat4_diag(am_float32 val) {
    am_mat4 m;
    memset(m.elements, 0, sizeof(m.elements));
    m.elements[0 + 0 * 4] = val;
    m.elements[1 + 1 * 4] = val;
    m.elements[2 + 2 * 4] = val;
    m.elements[3 + 3 * 4] = val;
    return m;
};

static inline am_mat4 am_mat4_elem(const am_float32* elements) {
    am_mat4 mat = am_mat4_create();
    memcpy(mat.elements, elements, sizeof(am_float32) * 16);
    return mat;
};

static inline am_mat4 am_mat4_mul(am_mat4 m0, am_mat4 m1) {
    am_mat4 m_res = am_mat4_create();
    for (am_uint32 y = 0; y < 4; ++y) {
        for (am_uint32 x = 0; x < 4; ++x) {
            am_float32 sum = 0.0f;
            for (am_uint32 e = 0; e < 4; ++e) sum += m0.elements[x + e * 4] * m1.elements[e + y * 4];
            m_res.elements[x + y * 4] = sum;
        };
    };
    return m_res;
};

static inline am_mat4 am_mat4_mul_list(am_uint32 count, ...) {
    va_list ap;
    am_mat4 m = am_mat4_identity();
    va_start(ap, count);
    for (am_uint32 i = 0; i < count; ++i) m = am_mat4_mul(m, va_arg(ap, am_mat4));
    va_end(ap);
    return m;
}

static inline void am_mat4_set_elements(am_mat4* m, const am_float32* elements, am_uint32 count) {
    for (am_uint32 i = 0; i < count; ++i) m->elements[i] = elements[i];
};

static inline am_mat4 am_mat4_transpose(am_mat4 m) {
    am_mat4 t = am_mat4_identity();

    // First row
    t.elements[0 * 4 + 0] = m.elements[0 * 4 + 0];
    t.elements[1 * 4 + 0] = m.elements[0 * 4 + 1];
    t.elements[2 * 4 + 0] = m.elements[0 * 4 + 2];
    t.elements[3 * 4 + 0] = m.elements[0 * 4 + 3];

    // Second row
    t.elements[0 * 4 + 1] = m.elements[1 * 4 + 0];
    t.elements[1 * 4 + 1] = m.elements[1 * 4 + 1];
    t.elements[2 * 4 + 1] = m.elements[1 * 4 + 2];
    t.elements[3 * 4 + 1] = m.elements[1 * 4 + 3];

    // Third row
    t.elements[0 * 4 + 2] = m.elements[2 * 4 + 0];
    t.elements[1 * 4 + 2] = m.elements[2 * 4 + 1];
    t.elements[2 * 4 + 2] = m.elements[2 * 4 + 2];
    t.elements[3 * 4 + 2] = m.elements[2 * 4 + 3];

    // Fourth row
    t.elements[0 * 4 + 3] = m.elements[3 * 4 + 0];
    t.elements[1 * 4 + 3] = m.elements[3 * 4 + 1];
    t.elements[2 * 4 + 3] = m.elements[3 * 4 + 2];
    t.elements[3 * 4 + 3] = m.elements[3 * 4 + 3];

    return t;
};

static inline am_mat4 am_mat4_inverse(am_mat4 m) {
    am_mat4 res = am_mat4_identity();

    am_float32 temp[16];

    temp[0] = m.elements[5] * m.elements[10] * m.elements[15] -
              m.elements[5] * m.elements[11] * m.elements[14] -
              m.elements[9] * m.elements[6] * m.elements[15] +
              m.elements[9] * m.elements[7] * m.elements[14] +
              m.elements[13] * m.elements[6] * m.elements[11] -
              m.elements[13] * m.elements[7] * m.elements[10];

    temp[4] = -m.elements[4] * m.elements[10] * m.elements[15] +
              m.elements[4] * m.elements[11] * m.elements[14] +
              m.elements[8] * m.elements[6] * m.elements[15] -
              m.elements[8] * m.elements[7] * m.elements[14] -
              m.elements[12] * m.elements[6] * m.elements[11] +
              m.elements[12] * m.elements[7] * m.elements[10];

    temp[8] = m.elements[4] * m.elements[9] * m.elements[15] -
              m.elements[4] * m.elements[11] * m.elements[13] -
              m.elements[8] * m.elements[5] * m.elements[15] +
              m.elements[8] * m.elements[7] * m.elements[13] +
              m.elements[12] * m.elements[5] * m.elements[11] -
              m.elements[12] * m.elements[7] * m.elements[9];

    temp[12] = -m.elements[4] * m.elements[9] * m.elements[14] +
               m.elements[4] * m.elements[10] * m.elements[13] +
               m.elements[8] * m.elements[5] * m.elements[14] -
               m.elements[8] * m.elements[6] * m.elements[13] -
               m.elements[12] * m.elements[5] * m.elements[10] +
               m.elements[12] * m.elements[6] * m.elements[9];

    temp[1] = -m.elements[1] * m.elements[10] * m.elements[15] +
              m.elements[1] * m.elements[11] * m.elements[14] +
              m.elements[9] * m.elements[2] * m.elements[15] -
              m.elements[9] * m.elements[3] * m.elements[14] -
              m.elements[13] * m.elements[2] * m.elements[11] +
              m.elements[13] * m.elements[3] * m.elements[10];

    temp[5] = m.elements[0] * m.elements[10] * m.elements[15] -
              m.elements[0] * m.elements[11] * m.elements[14] -
              m.elements[8] * m.elements[2] * m.elements[15] +
              m.elements[8] * m.elements[3] * m.elements[14] +
              m.elements[12] * m.elements[2] * m.elements[11] -
              m.elements[12] * m.elements[3] * m.elements[10];

    temp[9] = -m.elements[0] * m.elements[9] * m.elements[15] +
              m.elements[0] * m.elements[11] * m.elements[13] +
              m.elements[8] * m.elements[1] * m.elements[15] -
              m.elements[8] * m.elements[3] * m.elements[13] -
              m.elements[12] * m.elements[1] * m.elements[11] +
              m.elements[12] * m.elements[3] * m.elements[9];

    temp[13] = m.elements[0] * m.elements[9] * m.elements[14] -
               m.elements[0] * m.elements[10] * m.elements[13] -
               m.elements[8] * m.elements[1] * m.elements[14] +
               m.elements[8] * m.elements[2] * m.elements[13] +
               m.elements[12] * m.elements[1] * m.elements[10] -
               m.elements[12] * m.elements[2] * m.elements[9];

    temp[2] = m.elements[1] * m.elements[6] * m.elements[15] -
              m.elements[1] * m.elements[7] * m.elements[14] -
              m.elements[5] * m.elements[2] * m.elements[15] +
              m.elements[5] * m.elements[3] * m.elements[14] +
              m.elements[13] * m.elements[2] * m.elements[7] -
              m.elements[13] * m.elements[3] * m.elements[6];

    temp[6] = -m.elements[0] * m.elements[6] * m.elements[15] +
              m.elements[0] * m.elements[7] * m.elements[14] +
              m.elements[4] * m.elements[2] * m.elements[15] -
              m.elements[4] * m.elements[3] * m.elements[14] -
              m.elements[12] * m.elements[2] * m.elements[7] +
              m.elements[12] * m.elements[3] * m.elements[6];

    temp[10] = m.elements[0] * m.elements[5] * m.elements[15] -
               m.elements[0] * m.elements[7] * m.elements[13] -
               m.elements[4] * m.elements[1] * m.elements[15] +
               m.elements[4] * m.elements[3] * m.elements[13] +
               m.elements[12] * m.elements[1] * m.elements[7] -
               m.elements[12] * m.elements[3] * m.elements[5];

    temp[14] = -m.elements[0] * m.elements[5] * m.elements[14] +
               m.elements[0] * m.elements[6] * m.elements[13] +
               m.elements[4] * m.elements[1] * m.elements[14] -
               m.elements[4] * m.elements[2] * m.elements[13] -
               m.elements[12] * m.elements[1] * m.elements[6] +
               m.elements[12] * m.elements[2] * m.elements[5];

    temp[3] = -m.elements[1] * m.elements[6] * m.elements[11] +
              m.elements[1] * m.elements[7] * m.elements[10] +
              m.elements[5] * m.elements[2] * m.elements[11] -
              m.elements[5] * m.elements[3] * m.elements[10] -
              m.elements[9] * m.elements[2] * m.elements[7] +
              m.elements[9] * m.elements[3] * m.elements[6];

    temp[7] = m.elements[0] * m.elements[6] * m.elements[11] -
              m.elements[0] * m.elements[7] * m.elements[10] -
              m.elements[4] * m.elements[2] * m.elements[11] +
              m.elements[4] * m.elements[3] * m.elements[10] +
              m.elements[8] * m.elements[2] * m.elements[7] -
              m.elements[8] * m.elements[3] * m.elements[6];

    temp[11] = -m.elements[0] * m.elements[5] * m.elements[11] +
               m.elements[0] * m.elements[7] * m.elements[9] +
               m.elements[4] * m.elements[1] * m.elements[11] -
               m.elements[4] * m.elements[3] * m.elements[9] -
               m.elements[8] * m.elements[1] * m.elements[7] +
               m.elements[8] * m.elements[3] * m.elements[5];

    temp[15] = m.elements[0] * m.elements[5] * m.elements[10] -
               m.elements[0] * m.elements[6] * m.elements[9] -
               m.elements[4] * m.elements[1] * m.elements[10] +
               m.elements[4] * m.elements[2] * m.elements[9] +
               m.elements[8] * m.elements[1] * m.elements[6] -
               m.elements[8] * m.elements[2] * m.elements[5];

    am_float32 determinant = m.elements[0] * temp[0] + m.elements[1] * temp[4] + m.elements[2] * temp[8] + m.elements[3] * temp[12];
    determinant = 1.0f / determinant;

    for (am_int32 i = 0; i < 4 * 4; i++) res.elements[i] = (am_float32)(temp[i] * (am_float32)determinant);

    return res;
};

static inline am_mat4 am_mat4_ortho(am_float32 left, am_float32 right, am_float32 bottom, am_float32 top, am_float32 _near, am_float32 _far) {
    am_mat4 m_res = am_mat4_identity();

    // Main diagonal
    m_res.elements[0 + 0 * 4] = 2.0f / (right - left);
    m_res.elements[1 + 1 * 4] = 2.0f / (top - bottom);
    m_res.elements[2 + 2 * 4] = -2.0f / (_far - _near);

    // Last column
    m_res.elements[0 + 3 * 4] = -(right + left) / (right - left);
    m_res.elements[1 + 3 * 4] = -(top + bottom) / (top - bottom);
    m_res.elements[2 + 3 * 4] = -(_far + _near) / (_far - _near);

    return m_res;
};

static inline am_mat4 am_mat4_perspective(am_float32 fov, am_float32 aspect_ratio, am_float32 _near, am_float32 _far) {
    // Zero matrix
    am_mat4 m_res = am_mat4_create();

    am_float32 q = 1.0f / (am_float32)tan(am_deg2rad(0.5f * fov));
    am_float32 a = q / aspect_ratio;
    am_float32 b = (_near + _far) / (_near - _far);
    am_float32 c = (2.0f * _near * _far) / (_near - _far);

    m_res.elements[0 + 0 * 4] = a;
    m_res.elements[1 + 1 * 4] = q;
    m_res.elements[2 + 2 * 4] = b;
    m_res.elements[2 + 3 * 4] = c;
    m_res.elements[3 + 2 * 4] = -1.0f;

    return m_res;
};

static inline am_mat4 am_mat4_translatev(const am_vec3 v) {
    am_mat4 m_res = am_mat4_identity();

    m_res.elements[0 + 4 * 3] = v.x;
    m_res.elements[1 + 4 * 3] = v.y;
    m_res.elements[2 + 4 * 3] = v.z;

    return m_res;
};

static inline am_mat4 am_mat4_translate(am_float32 x, am_float32 y, am_float32 z) {
    return am_mat4_translatev(am_vec3_create(x, y, z));
};

static inline am_mat4 am_mat4_scalev(const am_vec3 v) {
    am_mat4 m_res = am_mat4_identity();
    m_res.elements[0 + 0 * 4] = v.x;
    m_res.elements[1 + 1 * 4] = v.y;
    m_res.elements[2 + 2 * 4] = v.z;
    return m_res;
};

static inline am_mat4 am_mat4_scale(am_float32 x, am_float32 y, am_float32 z) {
    return (am_mat4_scalev(am_vec3_create(x, y, z)));
};

// Assumes normalized axis
static inline am_mat4 am_mat4_rotatev(am_float32 angle, am_vec3 axis) {
    am_mat4 m_res = am_mat4_identity();

    am_float32 a = angle;
    am_float32 c = (am_float32)cos(a);
    am_float32 s = (am_float32)sin(a);

    am_vec3 naxis = am_vec3_norm(axis);
    am_float32 x = naxis.x;
    am_float32 y = naxis.y;
    am_float32 z = naxis.z;

    //First column
    m_res.elements[0 + 0 * 4] = x * x * (1 - c) + c;
    m_res.elements[1 + 0 * 4] = x * y * (1 - c) + z * s;
    m_res.elements[2 + 0 * 4] = x * z * (1 - c) - y * s;

    //Second column
    m_res.elements[0 + 1 * 4] = x * y * (1 - c) - z * s;
    m_res.elements[1 + 1 * 4] = y * y * (1 - c) + c;
    m_res.elements[2 + 1 * 4] = y * z * (1 - c) + x * s;

    //Third column
    m_res.elements[0 + 2 * 4] = x * z * (1 - c) + y * s;
    m_res.elements[1 + 2 * 4] = y * z * (1 - c) - x * s;
    m_res.elements[2 + 2 * 4] = z * z * (1 - c) + c;

    return m_res;
};

static inline am_mat4 am_mat4_rotate(am_float32 angle, am_float32 x, am_float32 y, am_float32 z) {
    return am_mat4_rotatev(angle, am_vec3_create(x, y, z));
};

static inline am_mat4 am_mat4_look_at(am_vec3 position, am_vec3 target, am_vec3 up) {
    am_vec3 f = am_vec3_norm(am_vec3_sub(target, position));
    am_vec3 s = am_vec3_norm(am_vec3_cross(f, up));
    am_vec3 u = am_vec3_cross(s, f);

    am_mat4 m_res = am_mat4_identity();
    m_res.elements[0 * 4 + 0] = s.x;
    m_res.elements[1 * 4 + 0] = s.y;
    m_res.elements[2 * 4 + 0] = s.z;

    m_res.elements[0 * 4 + 1] = u.x;
    m_res.elements[1 * 4 + 1] = u.y;
    m_res.elements[2 * 4 + 1] = u.z;

    m_res.elements[0 * 4 + 2] = -f.x;
    m_res.elements[1 * 4 + 2] = -f.y;
    m_res.elements[2 * 4 + 2] = -f.z;

    m_res.elements[3 * 4 + 0] = -am_vec3_dot(s, position);;
    m_res.elements[3 * 4 + 1] = -am_vec3_dot(u, position);
    m_res.elements[3 * 4 + 2] = am_vec3_dot(f, position);

    return m_res;
};

static inline am_vec4 am_mat4_mul_vec4(am_mat4 m, am_vec4 v) {
    return am_vec4_create(m.elements[0 + 4 * 0] * v.x + m.elements[0 + 4 * 1] * v.y + m.elements[0 + 4 * 2] * v.z + m.elements[0 + 4 * 3] * v.w,
                          m.elements[1 + 4 * 0] * v.x + m.elements[1 + 4 * 1] * v.y + m.elements[1 + 4 * 2] * v.z + m.elements[1 + 4 * 3] * v.w,
                          m.elements[2 + 4 * 0] * v.x + m.elements[2 + 4 * 1] * v.y + m.elements[2 + 4 * 2] * v.z + m.elements[2 + 4 * 3] * v.w,
                          m.elements[3 + 4 * 0] * v.x + m.elements[3 + 4 * 1] * v.y + m.elements[3 + 4 * 2] * v.z + m.elements[3 + 4 * 3] * v.w
    );
};

static inline am_vec3 am_mat4_mul_vec3(am_mat4 m, am_vec3 v) {
    return am_vec4_to_vec3(am_mat4_mul_vec4(m, am_vec4_create(v.x, v.y, v.z, 1.0f)));
};

// Quaternion

static inline am_quat am_quat_default() {
    am_quat q;
    q.x = 0.f;
    q.y = 0.f;
    q.z = 0.f;
    q.w = 1.f;
    return q;
};

static inline am_quat am_quat_create(am_float32 _x, am_float32 _y, am_float32 _z, am_float32 _w) {
    am_quat q;
    q.x = _x;
    q.y = _y;
    q.z = _z;
    q.w = _w;
    return q;
};

static inline am_quat am_quat_add(am_quat q0, am_quat q1) {
    return am_quat_create(q0.x + q1.x, q0.y + q1.y, q0.z + q1.z, q0.w + q1.w);
};

static inline am_quat am_quat_sub(am_quat q0, am_quat q1) {
    return am_quat_create(q0.x - q1.x, q0.y - q1.y, q0.z - q1.z, q0.w - q1.w);
};

static inline am_quat am_quat_mul(am_quat q0, am_quat q1) {
    return am_quat_create(q0.w * q1.x + q1.w * q0.x + q0.y * q1.z - q1.y * q0.z,
                          q0.w * q1.y + q1.w * q0.y + q0.z * q1.x - q1.z * q0.x,
                          q0.w * q1.z + q1.w * q0.z + q0.x * q1.y - q1.x * q0.y,
                          q0.w * q1.w - q0.x * q1.x - q0.y * q1.y - q0.z * q1.z
    );
};

static inline am_quat am_quat_mul_list(am_uint32 count, ...) {
    va_list ap;
    am_quat q = am_quat_default();
    va_start(ap, count);
    for (am_uint32 i = 0; i < count; ++i) q = am_quat_mul(q, va_arg(ap, am_quat));
    va_end(ap);
    return q;
}

static inline am_quat am_quat_mul_quat(am_quat q0, am_quat q1) {
    return am_quat_create(q0.w * q1.x + q1.w * q0.x + q0.y * q1.z - q1.y * q0.z,
                          q0.w * q1.y + q1.w * q0.y + q0.z * q1.x - q1.z * q0.x,
                          q0.w * q1.z + q1.w * q0.z + q0.x * q1.y - q1.x * q0.y,
                          q0.w * q1.w - q0.x * q1.x - q0.y * q1.y - q0.z * q1.z
    );
};

static inline am_quat am_quat_scale(am_quat q, am_float32 s) {
    return am_quat_create(q.x * s, q.y * s, q.z * s, q.w * s);
};

static inline am_float32 am_quat_dot(am_quat q0, am_quat q1) {
    return (am_float32)(q0.x * q1.x + q0.y * q1.y + q0.z * q1.z + q0.w * q1.w);
};

static inline am_quat am_quat_conjugate(am_quat q) {
    return (am_quat_create(-q.x, -q.y, -q.z, q.w));
};

static inline am_float32 am_quat_len(am_quat q) {
    return (am_float32)sqrt(am_quat_dot(q, q));
};

static inline am_quat am_quat_norm(am_quat q) {
    return am_quat_scale(q, 1.0f / am_quat_len(q));
};

static inline am_quat am_quat_cross(am_quat q0, am_quat q1) {
    return am_quat_create (q0.x * q1.x + q0.x * q1.w + q0.y * q1.z - q0.z * q1.y,
                           q0.w * q1.y + q0.y * q1.w + q0.z * q1.x - q0.x * q1.z,
                           q0.w * q1.z + q0.z * q1.w + q0.x * q1.y - q0.y * q1.x,
                           q0.w * q1.w - q0.x * q1.x - q0.y * q1.y - q0.z * q1.z
    );
};

// Inverse := Conjugate / Dot;
static inline am_quat am_quat_inverse(am_quat q) {
    return (am_quat_scale(am_quat_conjugate(q), 1.0f / am_quat_dot(q, q)));
};

static inline am_vec3 am_quat_rotate(am_quat q, am_vec3 v) {
    am_vec3 qvec = am_vec3_create(q.x, q.y, q.z);
    am_vec3 uv = am_vec3_cross(qvec, v);
    am_vec3 uuv = am_vec3_cross(qvec, uv);
    uv = am_vec3_scale(2.f * q.w, uv);
    uuv = am_vec3_scale(2.f, uuv);
    return (am_vec3_add(v, am_vec3_add(uv, uuv)));
};

static inline am_quat am_quat_angle_axis(am_float32 rad, am_vec3 axis) {
    // Normalize axis
    am_vec3 a = am_vec3_norm(axis);
    // Get scalar
    am_float32 half_angle = 0.5f * rad;
    am_float32 s = (am_float32)sin(half_angle);

    return am_quat_create(a.x * s, a.y * s, a.z * s, (am_float32)cos(half_angle));
};

static inline am_quat am_quat_slerp(am_quat a, am_quat b, am_float32 t) {
    am_float32 c = am_quat_dot(a, b);
    am_quat end = b;

    if (c < 0.0f) {
        // Reverse all signs
        c *= -1.0f;
        end.x *= -1.0f;
        end.y *= -1.0f;
        end.z *= -1.0f;
        end.w *= -1.0f;
    };

    // Calculate coefficients
    am_float32 sclp, sclq;
    if ((1.0f - c) > 0.0001f) {
        am_float32 omega = (float)acosf(c);
        am_float32 s = (float)sinf(omega);
        sclp = (float)sinf((1.0f - t) * omega) / s;
        sclq = (float)sinf(t * omega) / s;
    } else {
        sclp = 1.0f - t;
        sclq = t;
    };

    am_quat q;
    q.x = sclp * a.x + sclq * end.x;
    q.y = sclp * a.y + sclq * end.y;
    q.z = sclp * a.z + sclq * end.z;
    q.w = sclp * a.w + sclq * end.w;

    return q;
};

static inline am_mat4 am_quat_to_mat4(am_quat _q) {
    am_mat4 mat = am_mat4_identity();
    am_quat q = am_quat_norm(_q);

    am_float32 xx = q.x * q.x;
    am_float32 yy = q.y * q.y;
    am_float32 zz = q.z * q.z;
    am_float32 xy = q.x * q.y;
    am_float32 xz = q.x * q.z;
    am_float32 yz = q.y * q.z;
    am_float32 wx = q.w * q.x;
    am_float32 wy = q.w * q.y;
    am_float32 wz = q.w * q.z;

    mat.elements[0 * 4 + 0] = 1.0f - 2.0f * (yy + zz);
    mat.elements[1 * 4 + 0] = 2.0f * (xy - wz);
    mat.elements[2 * 4 + 0] = 2.0f * (xz + wy);

    mat.elements[0 * 4 + 1] = 2.0f * (xy + wz);
    mat.elements[1 * 4 + 1] = 1.0f - 2.0f * (xx + zz);
    mat.elements[2 * 4 + 1] = 2.0f * (yz - wx);

    mat.elements[0 * 4 + 2] = 2.0f * (xz - wy);
    mat.elements[1 * 4 + 2] = 2.0f * (yz + wx);
    mat.elements[2 * 4 + 2] = 1.0f - 2.0f * (xx + yy);

    return mat;
};

static inline am_quat am_quat_from_euler(am_float32 yaw_deg, am_float32 pitch_deg, am_float32 roll_deg) {
    am_float32 yaw = am_deg2rad(yaw_deg);
    am_float32 pitch = am_deg2rad(pitch_deg);
    am_float32 roll = am_deg2rad(roll_deg);

    am_quat q;
    am_float32 cy = (am_float32)cos((double)(yaw * 0.5f));
    am_float32 sy = (am_float32)sin((double)(yaw * 0.5f));
    am_float32 cr = (am_float32)cos((double)(roll * 0.5f));
    am_float32 sr = (am_float32)sin((double)(roll * 0.5f));
    am_float32 cp = (am_float32)cos((double)(pitch * 0.5f));
    am_float32 sp = (am_float32)sin((double)(pitch * 0.5f));

    q.x = cy * sr * cp - sy * cr * sp;
    q.y = cy * cr * sp + sy * sr * cp;
    q.z = sy * cr * cp - cy * sr * sp;
    q.w = cy * cr * cp + sy * sr * sp;

    return q;
};

static inline am_float32 am_quat_pitch(am_quat* q) {
    return (am_float32)atan2((double)(2.0f * q->y * q->z + q->w * q->x), (double)(q->w * q->w - q->x * q->x - q->y * q->y + q->z * q->z));
};

static inline am_float32 am_quat_yaw(am_quat* q) {
    return (am_float32)asin((double)(-2.0f * (q->x * q->z - q->w * q->y)));
};

static inline am_float32 am_quat_roll(am_quat* q) {
    return (am_float32)atan2((double)(2.0f * q->x * q->y +  q->z * q->w),  (double)(q->x * q->x + q->w * q->w - q->y * q->y - q->z * q->z));
};

static inline am_vec3 am_quat_to_euler(am_quat* q) {
    return am_vec3_create(am_quat_yaw(q), am_quat_pitch(q), am_quat_roll(q));
};

// TRANSFORMS

static inline am_vqs am_vqs_create(am_vec3 translation, am_quat rotation, am_vec3 scale) {
    am_vqs t;
    t.position = translation;
    t.rotation = rotation;
    t.scale = scale;
    return t;
};

static inline am_vqs am_vqs_default() {
    am_vqs t = am_vqs_create(am_vec3_create(0.0f, 0.0f, 0.0f),
                             am_quat_create(0.0f, 0.0f, 0.0f, 1.0f),
                             am_vec3_create(1.0f, 1.0f, 1.0f)
    );
    return t;
};

// AbsScale = ParentScale * LocalScale
// AbsRot   = LocalRot * ParentRot
// AbsTrans = ParentPos + [ParentRot * (ParentScale * LocalPos)]
static inline am_vqs am_vqs_absolute_transform(const am_vqs* local, const am_vqs* parent) {
    if (!local || !parent) return am_vqs_default();

    // Normalized rotations
    am_quat p_rot_norm = am_quat_norm(parent->rotation);
    am_quat l_rot_norm = am_quat_norm(local->rotation);

    // Scale
    am_vec3 scl = am_vec3_mul(local->scale, parent->scale);
    // Rotation
    am_quat rot = am_quat_norm(am_quat_mul(p_rot_norm, l_rot_norm));
    // position
    am_vec3 tns = am_vec3_add(parent->position, am_quat_rotate(p_rot_norm, am_vec3_mul(parent->scale, local->position)));

    return am_vqs_create(tns, rot, scl);
};

// RelScale = AbsScale / ParentScale
// RelRot   = Inverse(ParentRot) * AbsRot
// RelTrans = [Inverse(ParentRot) * (AbsPos - ParentPosition)] / ParentScale;
static inline am_vqs am_vqs_relative_transform(const am_vqs* absolute, const am_vqs* parent) {
    if (!absolute || !parent) return am_vqs_default();

    // Get inverse rotation normalized
    am_quat p_rot_inv = am_quat_norm(am_quat_inverse(parent->rotation));
    // Normalized abs rotation
    am_quat a_rot_norm = am_quat_norm(absolute->rotation);

    // Scale
    am_vec3 scl = am_vec3_div(absolute->scale, parent->scale);
    // Rotation
    am_quat rot = am_quat_norm(am_quat_mul(p_rot_inv, a_rot_norm));
    // position
    am_vec3 tns = am_vec3_div(am_quat_rotate(p_rot_inv, am_vec3_sub(absolute->position, parent->position)), parent->scale);

    return am_vqs_create(tns, rot, scl);
};

static inline am_mat4 am_vqs_to_mat4(const am_vqs* transform) {
    am_mat4 mat = am_mat4_identity();
    am_mat4 trans = am_mat4_translatev(transform->position);
    am_mat4 rot = am_quat_to_mat4(transform->rotation);
    am_mat4 scl = am_mat4_scalev(transform->scale);
    mat = am_mat4_mul(mat, trans);
    mat = am_mat4_mul(mat, rot);
    mat = am_mat4_mul(mat, scl);
    return mat;
};

//----------------------------------------------------------------------------//
//                               END MATH IMPL                                //
//----------------------------------------------------------------------------//


//----------------------------------------------------------------------------//
//                                PLATFORM IMPL                               //
//----------------------------------------------------------------------------//


#if defined(AM_LINUX)
am_key_map am_platform_translate_keysym(const KeySym *key_syms, am_int32 width) {
    if (width > 1) {
        switch (key_syms[1]) {
            case XK_KP_0:           return AM_KEYCODE_NUMPAD_0;
            case XK_KP_1:           return AM_KEYCODE_NUMPAD_1;
            case XK_KP_2:           return AM_KEYCODE_NUMPAD_2;
            case XK_KP_3:           return AM_KEYCODE_NUMPAD_3;
            case XK_KP_4:           return AM_KEYCODE_NUMPAD_4;
            case XK_KP_5:           return AM_KEYCODE_NUMPAD_5;
            case XK_KP_6:           return AM_KEYCODE_NUMPAD_6;
            case XK_KP_7:           return AM_KEYCODE_NUMPAD_7;
            case XK_KP_8:           return AM_KEYCODE_NUMPAD_8;
            case XK_KP_9:           return AM_KEYCODE_NUMPAD_9;
            case XK_KP_Separator:
            case XK_KP_Decimal:     return AM_KEYCODE_NUMPAD_DECIMAL;
            case XK_KP_Equal:       return AM_KEYCODE_NUMPAD_EQUAL;
            case XK_KP_Enter:       return AM_KEYCODE_NUMPAD_ENTER;
            default:                break;
        };
    };

    switch (key_syms[0]) {
        case XK_Escape:         return AM_KEYCODE_ESCAPE;
        case XK_Tab:            return AM_KEYCODE_TAB;
        case XK_Shift_L:        return AM_KEYCODE_LEFT_SHIFT;
        case XK_Shift_R:        return AM_KEYCODE_RIGHT_SHIFT;
        case XK_Control_L:      return AM_KEYCODE_LEFT_CONTROL;
        case XK_Control_R:      return AM_KEYCODE_RIGHT_CONTROL;
        case XK_Alt_L:          return AM_KEYCODE_LEFT_ALT;
        case XK_Alt_R:          return AM_KEYCODE_RIGHT_ALT;
        case XK_Super_L:        return AM_KEYCODE_LEFT_META;
        case XK_Super_R:        return AM_KEYCODE_RIGHT_META;
        case XK_Menu:           return AM_KEYCODE_MENU;
        case XK_Num_Lock:       return AM_KEYCODE_NUMPAD_NUM;
        case XK_Caps_Lock:      return AM_KEYCODE_CAPS_LOCK;
        case XK_Print:          return AM_KEYCODE_PRINT_SCREEN;
        case XK_Scroll_Lock:    return AM_KEYCODE_SCROLL_LOCK;
        case XK_Pause:          return AM_KEYCODE_PAUSE;
        case XK_Delete:         return AM_KEYCODE_DELETE;
        case XK_BackSpace:      return AM_KEYCODE_BACKSPACE;
        case XK_Return:         return AM_KEYCODE_ENTER;
        case XK_Home:           return AM_KEYCODE_HOME;
        case XK_End:            return AM_KEYCODE_END;
        case XK_Page_Up:        return AM_KEYCODE_PAGE_UP;
        case XK_Page_Down:      return AM_KEYCODE_PAGE_DOWN;
        case XK_Insert:         return AM_KEYCODE_INSERT;
        case XK_Left:           return AM_KEYCODE_LEFT_ARROW;
        case XK_Right:          return AM_KEYCODE_RIGHT_ARROW;
        case XK_Down:           return AM_KEYCODE_DOWN_ARROW;
        case XK_Up:             return AM_KEYCODE_UP_ARROW;
        case XK_F1:             return AM_KEYCODE_F1;
        case XK_F2:             return AM_KEYCODE_F2;
        case XK_F3:             return AM_KEYCODE_F3;
        case XK_F4:             return AM_KEYCODE_F4;
        case XK_F5:             return AM_KEYCODE_F5;
        case XK_F6:             return AM_KEYCODE_F6;
        case XK_F7:             return AM_KEYCODE_F7;
        case XK_F8:             return AM_KEYCODE_F8;
        case XK_F9:             return AM_KEYCODE_F9;
        case XK_F10:            return AM_KEYCODE_F10;
        case XK_F11:            return AM_KEYCODE_F11;
        case XK_F12:            return AM_KEYCODE_F12;
        case XK_F13:            return AM_KEYCODE_F13;
        case XK_F14:            return AM_KEYCODE_F14;
        case XK_F15:            return AM_KEYCODE_F15;
        case XK_F16:            return AM_KEYCODE_F16;
        case XK_F17:            return AM_KEYCODE_F17;
        case XK_F18:            return AM_KEYCODE_F18;
        case XK_F19:            return AM_KEYCODE_F19;
        case XK_F20:            return AM_KEYCODE_F20;
        case XK_F21:            return AM_KEYCODE_F21;
        case XK_F22:            return AM_KEYCODE_F22;
        case XK_F23:            return AM_KEYCODE_F23;
        case XK_F24:            return AM_KEYCODE_F24;
        case XK_F25:            return AM_KEYCODE_F25;
        case XK_KP_Divide:      return AM_KEYCODE_NUMPAD_DIVIDE;
        case XK_KP_Multiply:    return AM_KEYCODE_NUMPAD_MULTIPLY;
        case XK_KP_Subtract:    return AM_KEYCODE_NUMPAD_SUBTRACT;
        case XK_KP_Add:         return AM_KEYCODE_NUMPAD_ADD;
        case XK_KP_Insert:      return AM_KEYCODE_NUMPAD_0;
        case XK_KP_End:         return AM_KEYCODE_NUMPAD_1;
        case XK_KP_Down:        return AM_KEYCODE_NUMPAD_2;
        case XK_KP_Page_Down:   return AM_KEYCODE_NUMPAD_3;
        case XK_KP_Left:        return AM_KEYCODE_NUMPAD_4;
        case XK_KP_Right:       return AM_KEYCODE_NUMPAD_6;
        case XK_KP_Home:        return AM_KEYCODE_NUMPAD_7;
        case XK_KP_Up:          return AM_KEYCODE_NUMPAD_8;
        case XK_KP_Page_Up:     return AM_KEYCODE_NUMPAD_9;
        case XK_KP_Delete:      return AM_KEYCODE_NUMPAD_DECIMAL;
        case XK_KP_Equal:       return AM_KEYCODE_NUMPAD_EQUAL;
        case XK_KP_Enter:       return AM_KEYCODE_NUMPAD_ENTER;
        case XK_a:              return AM_KEYCODE_A;
        case XK_b:              return AM_KEYCODE_B;
        case XK_c:              return AM_KEYCODE_C;
        case XK_d:              return AM_KEYCODE_D;
        case XK_e:              return AM_KEYCODE_E;
        case XK_f:              return AM_KEYCODE_F;
        case XK_g:              return AM_KEYCODE_G;
        case XK_h:              return AM_KEYCODE_H;
        case XK_i:              return AM_KEYCODE_I;
        case XK_j:              return AM_KEYCODE_J;
        case XK_k:              return AM_KEYCODE_K;
        case XK_l:              return AM_KEYCODE_L;
        case XK_m:              return AM_KEYCODE_M;
        case XK_n:              return AM_KEYCODE_N;
        case XK_o:              return AM_KEYCODE_O;
        case XK_p:              return AM_KEYCODE_P;
        case XK_q:              return AM_KEYCODE_Q;
        case XK_r:              return AM_KEYCODE_R;
        case XK_s:              return AM_KEYCODE_S;
        case XK_t:              return AM_KEYCODE_T;
        case XK_u:              return AM_KEYCODE_U;
        case XK_v:              return AM_KEYCODE_V;
        case XK_w:              return AM_KEYCODE_W;
        case XK_x:              return AM_KEYCODE_X;
        case XK_y:              return AM_KEYCODE_Y;
        case XK_z:              return AM_KEYCODE_Z;
        case XK_1:              return AM_KEYCODE_1;
        case XK_2:              return AM_KEYCODE_2;
        case XK_3:              return AM_KEYCODE_3;
        case XK_4:              return AM_KEYCODE_4;
        case XK_5:              return AM_KEYCODE_5;
        case XK_6:              return AM_KEYCODE_6;
        case XK_7:              return AM_KEYCODE_7;
        case XK_8:              return AM_KEYCODE_8;
        case XK_9:              return AM_KEYCODE_9;
        case XK_0:              return AM_KEYCODE_0;
        case XK_space:          return AM_KEYCODE_SPACE;
        case XK_minus:          return AM_KEYCODE_MINUS;
        case XK_equal:          return AM_KEYCODE_EQUAL;
        case XK_bracketleft:    return AM_KEYCODE_LEFT_SQUARE_BRACKET;
        case XK_bracketright:   return AM_KEYCODE_RIGHT_SQUARE_BRACKET;
        case XK_backslash:      return AM_KEYCODE_BACKSLASH;
        case XK_semicolon:      return AM_KEYCODE_SEMICOLON;
        case XK_apostrophe:     return AM_KEYCODE_APOSTROPHE;
        case XK_grave:          return AM_KEYCODE_ACCENT_GRAVE;
        case XK_comma:          return AM_KEYCODE_COMMA;
        case XK_period:         return AM_KEYCODE_PERIOD;
        case XK_slash:          return AM_KEYCODE_SLASH;
        default:                return AM_KEYCODE_INVALID;
    };
};
#endif

am_mouse_map am_platform_translate_button(am_uint32 button) {
    switch (button) {
        case 1: return AM_MOUSE_BUTTON_LEFT;
        case 2: return AM_MOUSE_BUTTON_MIDDLE;
        case 3: return AM_MOUSE_BUTTON_RIGHT;
        default: return AM_MOUSE_BUTTON_INVALID;
    };
};

am_platform *am_platform_create() {
    am_platform *platform = (am_platform*)am_malloc(sizeof(am_platform));
    if (platform == NULL) printf("[FAIL] am_platform_create: Could not allocate memory!\n");
    assert(platform != NULL);
    platform->windows = NULL;
    am_packed_array_init(platform->windows, sizeof(am_window));

    memset(&platform->time, 0, sizeof(platform->time));

#if defined(AM_LINUX)
    platform->display = XOpenDisplay(NULL);
    memset(platform->input.keyboard.keycodes, -1, sizeof(platform->input.keyboard.keycodes));
    am_int32 min, max;
    XDisplayKeycodes(platform->display, &min, &max);
    am_int32 width;
    KeySym *key_syms = XGetKeyboardMapping(platform->display, min, max - min + 1, &width);
    for (am_int32 i = min; i < max; i++) platform->input.keyboard.keycodes[i] = am_platform_translate_keysym(&key_syms[(i-min)*width], width);
    XFree(key_syms);
#else
    platform->input.keyboard.keycodes[0x00B] = AM_KEYCODE_0;
    platform->input.keyboard.keycodes[0x002] = AM_KEYCODE_1;
    platform->input.keyboard.keycodes[0x003] = AM_KEYCODE_2;
    platform->input.keyboard.keycodes[0x004] = AM_KEYCODE_3;
    platform->input.keyboard.keycodes[0x005] = AM_KEYCODE_4;
    platform->input.keyboard.keycodes[0x006] = AM_KEYCODE_5;
    platform->input.keyboard.keycodes[0x007] = AM_KEYCODE_6;
    platform->input.keyboard.keycodes[0x008] = AM_KEYCODE_7;
    platform->input.keyboard.keycodes[0x009] = AM_KEYCODE_8;
    platform->input.keyboard.keycodes[0x00A] = AM_KEYCODE_9;
    platform->input.keyboard.keycodes[0x01E] = AM_KEYCODE_A;
    platform->input.keyboard.keycodes[0x030] = AM_KEYCODE_B;
    platform->input.keyboard.keycodes[0x02E] = AM_KEYCODE_C;
    platform->input.keyboard.keycodes[0x020] = AM_KEYCODE_D;
    platform->input.keyboard.keycodes[0x012] = AM_KEYCODE_E;
    platform->input.keyboard.keycodes[0x021] = AM_KEYCODE_F;
    platform->input.keyboard.keycodes[0x022] = AM_KEYCODE_G;
    platform->input.keyboard.keycodes[0x023] = AM_KEYCODE_H;
    platform->input.keyboard.keycodes[0x017] = AM_KEYCODE_I;
    platform->input.keyboard.keycodes[0x024] = AM_KEYCODE_J;
    platform->input.keyboard.keycodes[0x025] = AM_KEYCODE_K;
    platform->input.keyboard.keycodes[0x026] = AM_KEYCODE_L;
    platform->input.keyboard.keycodes[0x032] = AM_KEYCODE_M;
    platform->input.keyboard.keycodes[0x031] = AM_KEYCODE_N;
    platform->input.keyboard.keycodes[0x018] = AM_KEYCODE_O;
    platform->input.keyboard.keycodes[0x019] = AM_KEYCODE_P;
    platform->input.keyboard.keycodes[0x010] = AM_KEYCODE_Q;
    platform->input.keyboard.keycodes[0x013] = AM_KEYCODE_R;
    platform->input.keyboard.keycodes[0x01F] = AM_KEYCODE_S;
    platform->input.keyboard.keycodes[0x014] = AM_KEYCODE_T;
    platform->input.keyboard.keycodes[0x016] = AM_KEYCODE_U;
    platform->input.keyboard.keycodes[0x02F] = AM_KEYCODE_V;
    platform->input.keyboard.keycodes[0x011] = AM_KEYCODE_W;
    platform->input.keyboard.keycodes[0x02D] = AM_KEYCODE_X;
    platform->input.keyboard.keycodes[0x015] = AM_KEYCODE_Y;
    platform->input.keyboard.keycodes[0x02C] = AM_KEYCODE_Z;
    platform->input.keyboard.keycodes[0x028] = AM_KEYCODE_APOSTROPHE;
    platform->input.keyboard.keycodes[0x02B] = AM_KEYCODE_BACKSLASH;
    platform->input.keyboard.keycodes[0x033] = AM_KEYCODE_COMMA;
    platform->input.keyboard.keycodes[0x00D] = AM_KEYCODE_EQUAL;
    platform->input.keyboard.keycodes[0x029] = AM_KEYCODE_ACCENT_GRAVE;
    platform->input.keyboard.keycodes[0x01A] = AM_KEYCODE_LEFT_SQUARE_BRACKET;
    platform->input.keyboard.keycodes[0x00C] = AM_KEYCODE_MINUS;
    platform->input.keyboard.keycodes[0x034] = AM_KEYCODE_PERIOD;
    platform->input.keyboard.keycodes[0x01B] = AM_KEYCODE_RIGHT_SQUARE_BRACKET;
    platform->input.keyboard.keycodes[0x027] = AM_KEYCODE_SEMICOLON;
    platform->input.keyboard.keycodes[0x035] = AM_KEYCODE_SLASH;
    platform->input.keyboard.keycodes[0x00E] = AM_KEYCODE_BACKSPACE;
    platform->input.keyboard.keycodes[0x153] = AM_KEYCODE_DELETE;
    platform->input.keyboard.keycodes[0x14F] = AM_KEYCODE_END;
    platform->input.keyboard.keycodes[0x01C] = AM_KEYCODE_ENTER;
    platform->input.keyboard.keycodes[0x001] = AM_KEYCODE_ESCAPE;
    platform->input.keyboard.keycodes[0x147] = AM_KEYCODE_HOME;
    platform->input.keyboard.keycodes[0x152] = AM_KEYCODE_INSERT;
    platform->input.keyboard.keycodes[0x15D] = AM_KEYCODE_MENU;
    platform->input.keyboard.keycodes[0x151] = AM_KEYCODE_PAGE_DOWN;
    platform->input.keyboard.keycodes[0x149] = AM_KEYCODE_PAGE_UP;
    platform->input.keyboard.keycodes[0x045] = AM_KEYCODE_PAUSE;
    platform->input.keyboard.keycodes[0x146] = AM_KEYCODE_PAUSE;
    platform->input.keyboard.keycodes[0x039] = AM_KEYCODE_SPACE;
    platform->input.keyboard.keycodes[0x00F] = AM_KEYCODE_TAB;
    platform->input.keyboard.keycodes[0x03A] = AM_KEYCODE_CAPS_LOCK;
    platform->input.keyboard.keycodes[0x145] = AM_KEYCODE_NUMPAD_NUM;
    platform->input.keyboard.keycodes[0x046] = AM_KEYCODE_SCROLL_LOCK;
    platform->input.keyboard.keycodes[0x03B] = AM_KEYCODE_F1;
    platform->input.keyboard.keycodes[0x03C] = AM_KEYCODE_F2;
    platform->input.keyboard.keycodes[0x03D] = AM_KEYCODE_F3;
    platform->input.keyboard.keycodes[0x03E] = AM_KEYCODE_F4;
    platform->input.keyboard.keycodes[0x03F] = AM_KEYCODE_F5;
    platform->input.keyboard.keycodes[0x040] = AM_KEYCODE_F6;
    platform->input.keyboard.keycodes[0x041] = AM_KEYCODE_F7;
    platform->input.keyboard.keycodes[0x042] = AM_KEYCODE_F8;
    platform->input.keyboard.keycodes[0x043] = AM_KEYCODE_F9;
    platform->input.keyboard.keycodes[0x044] = AM_KEYCODE_F10;
    platform->input.keyboard.keycodes[0x057] = AM_KEYCODE_F11;
    platform->input.keyboard.keycodes[0x058] = AM_KEYCODE_F12;
    platform->input.keyboard.keycodes[0x064] = AM_KEYCODE_F13;
    platform->input.keyboard.keycodes[0x065] = AM_KEYCODE_F14;
    platform->input.keyboard.keycodes[0x066] = AM_KEYCODE_F15;
    platform->input.keyboard.keycodes[0x067] = AM_KEYCODE_F16;
    platform->input.keyboard.keycodes[0x068] = AM_KEYCODE_F17;
    platform->input.keyboard.keycodes[0x069] = AM_KEYCODE_F18;
    platform->input.keyboard.keycodes[0x06A] = AM_KEYCODE_F19;
    platform->input.keyboard.keycodes[0x06B] = AM_KEYCODE_F20;
    platform->input.keyboard.keycodes[0x06C] = AM_KEYCODE_F21;
    platform->input.keyboard.keycodes[0x06D] = AM_KEYCODE_F22;
    platform->input.keyboard.keycodes[0x06E] = AM_KEYCODE_F23;
    platform->input.keyboard.keycodes[0x076] = AM_KEYCODE_F24;
    platform->input.keyboard.keycodes[0x038] = AM_KEYCODE_LEFT_ALT;
    platform->input.keyboard.keycodes[0x01D] = AM_KEYCODE_LEFT_CONTROL;
    platform->input.keyboard.keycodes[0x02A] = AM_KEYCODE_LEFT_SHIFT;
    platform->input.keyboard.keycodes[0x15B] = AM_KEYCODE_LEFT_META;
    platform->input.keyboard.keycodes[0x137] = AM_KEYCODE_PRINT_SCREEN;
    platform->input.keyboard.keycodes[0x138] = AM_KEYCODE_RIGHT_ALT;
    platform->input.keyboard.keycodes[0x11D] = AM_KEYCODE_RIGHT_CONTROL;
    platform->input.keyboard.keycodes[0x036] = AM_KEYCODE_RIGHT_SHIFT;
    platform->input.keyboard.keycodes[0x15C] = AM_KEYCODE_RIGHT_META;
    platform->input.keyboard.keycodes[0x150] = AM_KEYCODE_DOWN_ARROW;
    platform->input.keyboard.keycodes[0x14B] = AM_KEYCODE_LEFT_ARROW;
    platform->input.keyboard.keycodes[0x14D] = AM_KEYCODE_RIGHT_ARROW;
    platform->input.keyboard.keycodes[0x148] = AM_KEYCODE_UP_ARROW;
    platform->input.keyboard.keycodes[0x052] = AM_KEYCODE_NUMPAD_0;
    platform->input.keyboard.keycodes[0x04F] = AM_KEYCODE_NUMPAD_1;
    platform->input.keyboard.keycodes[0x050] = AM_KEYCODE_NUMPAD_2;
    platform->input.keyboard.keycodes[0x051] = AM_KEYCODE_NUMPAD_3;
    platform->input.keyboard.keycodes[0x04B] = AM_KEYCODE_NUMPAD_4;
    platform->input.keyboard.keycodes[0x04C] = AM_KEYCODE_NUMPAD_5;
    platform->input.keyboard.keycodes[0x04D] = AM_KEYCODE_NUMPAD_6;
    platform->input.keyboard.keycodes[0x047] = AM_KEYCODE_NUMPAD_7;
    platform->input.keyboard.keycodes[0x048] = AM_KEYCODE_NUMPAD_8;
    platform->input.keyboard.keycodes[0x049] = AM_KEYCODE_NUMPAD_9;
    platform->input.keyboard.keycodes[0x04E] = AM_KEYCODE_NUMPAD_ADD;
    platform->input.keyboard.keycodes[0x053] = AM_KEYCODE_NUMPAD_DECIMAL;
    platform->input.keyboard.keycodes[0x135] = AM_KEYCODE_NUMPAD_DIVIDE;
    platform->input.keyboard.keycodes[0x11C] = AM_KEYCODE_NUMPAD_ENTER;
    platform->input.keyboard.keycodes[0x059] = AM_KEYCODE_NUMPAD_EQUAL;
    platform->input.keyboard.keycodes[0x037] = AM_KEYCODE_NUMPAD_MULTIPLY;
    platform->input.keyboard.keycodes[0x04A] = AM_KEYCODE_NUMPAD_SUBTRACT;

    WNDCLASS window_class = {0};
    window_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.hInstance = GetModuleHandle(NULL);
    window_class.lpfnWndProc = am_platform_event_handler;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.lpszMenuName = NULL;
    window_class.hbrBackground = NULL;
    window_class.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
    window_class.lpszClassName = "AM_ROOT";
    if (!RegisterClass(&window_class)) {
        printf("[FAIL] Failed to register root class!\n");
        return NULL;
    };
    window_class.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
    window_class.style = CS_HREDRAW | CS_VREDRAW | CS_PARENTDC;
    window_class.lpszClassName = "AM_CHILD";
    if (!RegisterClass(&window_class)) {
        printf("[FAIL] Failed to register child class!\n");
        return NULL;
    };
#endif

    platform->input.mouse.wheel_delta = 0;
    platform->input.mouse.position.x = 0;
    platform->input.mouse.position.y = 0;

    memset(platform->input.keyboard.map, 0, sizeof(platform->input.keyboard.map));
    memset(platform->input.keyboard.prev_map, 0, sizeof(platform->input.keyboard.prev_map));
    memset(platform->input.mouse.map, 0, sizeof(platform->input.mouse.map));
    memset(platform->input.mouse.map, 0, sizeof(platform->input.mouse.prev_map));

    am_platform_set_key_callback(platform, am_platform_key_callback_default);
    am_platform_set_mouse_button_callback(platform, am_platform_mouse_button_callback_default);
    am_platform_set_mouse_motion_callback(platform, am_platform_mouse_motion_callback_default);
    am_platform_set_mouse_scroll_callback(platform, am_platform_mouse_scroll_callback_default);
    am_platform_set_window_motion_callback(platform, am_platform_window_motion_callback_default);
    am_platform_set_window_size_callback(platform, am_platform_window_size_callback_default);
    printf("[OK] Platform init successful!\n");
    return platform;
};

void am_platform_poll_events() {
    am_platform *platform = am_engine_get_subsystem(platform);
#if defined(AM_LINUX)
    XEvent xevent;
    while (XPending(platform->display)) {
        XNextEvent(platform->display, &xevent);
        am_platform_event_handler(&xevent);
    };
#else
    MSG msg;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    };
#endif

    //HACK: Coords are bound to main window
    if (platform->input.mouse.locked) {
#if defined(AM_LINUX)
        am_platform_mouse_set_position(am_packed_array_get_ptr(platform->windows, 1)->width/2, am_packed_array_get_ptr(platform->windows, 1)->height/2);
#else
        am_vec2u pos = am_platform_window_get_size(1);
        am_platform_mouse_set_position(pos.x/2, pos.y/2);
#endif
    };
};

#if defined(AM_LINUX)
void am_platform_event_handler(XEvent *xevent) {
    am_platform *platform = am_engine_get_subsystem(platform);
    am_uint64 handle = xevent->xany.window;
    am_id id = -1;

    for (am_uint32 i = 0; i < am_packed_array_get_count(platform->windows); i++)
        if (platform->windows->elements[i].handle == handle)
            id = platform->windows->elements[i].id;

    switch (xevent->type) {
        case KeyPress: {
            am_key_map key = platform->input.keyboard.keycodes[xevent->xkey.keycode];
            platform->callbacks.am_platform_key_callback(id, key, AM_EVENT_KEY_PRESS);
            break;
        };
        case KeyRelease: {
            am_key_map key = platform->input.keyboard.keycodes[xevent->xkey.keycode];
            platform->callbacks.am_platform_key_callback(id, key, AM_EVENT_KEY_RELEASE);
            break;
        };
        case ButtonPress: {
            am_mouse_map button = am_platform_translate_button(xevent->xbutton.button);
            if (button == AM_MOUSE_BUTTON_INVALID) {
                if (xevent->xbutton.button == 4) {
                    platform->callbacks.am_platform_mouse_scroll_callback(id, AM_EVENT_MOUSE_SCROLL_UP);
                    break;
                };
                if (xevent->xbutton.button == 5) {
                    platform->callbacks.am_platform_mouse_scroll_callback(id, AM_EVENT_MOUSE_SCROLL_DOWN);
                    break;
                };
            };
            platform->callbacks.am_platform_mouse_button_callback(id, button, AM_EVENT_MOUSE_BUTTON_PRESS);
            break;
        };
        case ButtonRelease: {
            am_mouse_map button = am_platform_translate_button(xevent->xbutton.button);
            platform->callbacks.am_platform_mouse_button_callback(id, button, AM_EVENT_MOUSE_BUTTON_RELEASE);
            break;
        };
        case MotionNotify: {
            platform->callbacks.am_platform_mouse_motion_callback(id, xevent->xbutton.x, xevent->xbutton.y, AM_EVENT_MOUSE_MOTION);
            break;
        };
        case ConfigureNotify: {
            am_window *window = am_packed_array_get_ptr(platform->windows, id);
            if (window->height != xevent->xconfigure.height || window->width != xevent->xconfigure.width) {
                platform->callbacks.am_platform_window_size_callback(id, xevent->xconfigure.width, xevent->xconfigure.height, AM_EVENT_WINDOW_SIZE);
            };
            if (window->x != xevent->xconfigure.x || window->y != xevent->xconfigure.y) {
                platform->callbacks.am_platform_window_motion_callback(id, xevent->xconfigure.x, xevent->xconfigure.y, AM_EVENT_WINDOW_MOTION);
            };
            break;
        };
        case DestroyNotify: {
            printf("Destroying window %d!\n", id);
            am_packed_array_erase(platform->windows, id);

            am_bool check_no_root = true;
            for (am_int32 i = 0; i < am_packed_array_get_count(platform->windows); i++)
                if (platform->windows->elements[i].parent == AM_WINDOW_DEFAULT_PARENT) {
                    printf("check no root false");
                    check_no_root = false;
                    break;
                };

            if (check_no_root) am_engine_get_instance()->is_running = false;
            break;
        };
        case ClientMessage: {
            if (xevent->xclient.data.l[0] == XInternAtom(platform->display, "WM_DELETE_WINDOW", false)) {
                XUnmapWindow(platform->display, handle);
                XDestroyWindow(platform->display, handle);
            };
            break;
        };
        default: break;
    };
};
#else
LRESULT CALLBACK am_platform_event_handler(HWND handle, am_uint32 event, WPARAM wparam, LPARAM lparam) {
    am_platform *platform = am_engine_get_subsystem(platform);
    am_uint64 window_handle = (am_uint64) handle;
    am_id id = -1;

    for (am_uint32 i = 0; i < am_packed_array_get_count(platform->windows); i++)
        if (platform->windows->elements[i].handle == window_handle)
            id = platform->windows->elements[i].id;

    switch (event) {
        case WM_KEYDOWN: {
            am_key_map key = platform->input.keyboard.keycodes[(HIWORD(lparam) & (KF_EXTENDED | 0xff))];
            platform->callbacks.am_platform_key_callback(id, key, AM_EVENT_KEY_PRESS);
            break;
        };
        case WM_KEYUP: {
            am_key_map key = platform->input.keyboard.keycodes[(HIWORD(lparam) & (KF_EXTENDED | 0xff))];
            platform->callbacks.am_platform_key_callback(id, key, AM_EVENT_KEY_RELEASE);
            break;
        };
        case WM_LBUTTONDOWN: {
            platform->callbacks.am_platform_mouse_button_callback(id, AM_MOUSE_BUTTON_LEFT, AM_EVENT_MOUSE_BUTTON_PRESS);
            break;
        };
        case WM_LBUTTONUP: {
            platform->callbacks.am_platform_mouse_button_callback(id, AM_MOUSE_BUTTON_LEFT, AM_EVENT_MOUSE_BUTTON_RELEASE);
            break;
        };
        case WM_MBUTTONDOWN: {
            platform->callbacks.am_platform_mouse_button_callback(id, AM_MOUSE_BUTTON_MIDDLE, AM_EVENT_MOUSE_BUTTON_PRESS);
            break;
        };
        case WM_MBUTTONUP: {
            platform->callbacks.am_platform_mouse_button_callback(id, AM_MOUSE_BUTTON_MIDDLE, AM_EVENT_MOUSE_BUTTON_RELEASE);
            break;
        };
        case WM_RBUTTONDOWN: {
            platform->callbacks.am_platform_mouse_button_callback(id, AM_MOUSE_BUTTON_RIGHT, AM_EVENT_MOUSE_BUTTON_PRESS);
            break;
        };
        case WM_RBUTTONUP: {
            platform->callbacks.am_platform_mouse_button_callback(id, AM_MOUSE_BUTTON_RIGHT, AM_EVENT_MOUSE_BUTTON_RELEASE);
            break;
        };
        case WM_MOUSEWHEEL: {
            ++platform->input.mouse.wheel_delta;
            break;
        };
        case WM_MOUSEMOVE: {
            platform->callbacks.am_platform_mouse_motion_callback(id, LOWORD(lparam), HIWORD(lparam), AM_EVENT_MOUSE_MOTION);
            break;
        };
        case WM_GETMINMAXINFO: {
            MINMAXINFO *info = (MINMAXINFO*)lparam;
            info->ptMinTrackSize.x = 1;
            info->ptMinTrackSize.y = 1;
            break;
        };
        case WM_SIZE: {
            if (am_packed_array_has(platform->windows, id))
                platform->callbacks.am_platform_window_size_callback(id, LOWORD(lparam), HIWORD(lparam), AM_EVENT_WINDOW_SIZE);
            break;
        };
        case WM_MOVE: {
            if (am_packed_array_has(platform->windows, id))
                platform->callbacks.am_platform_window_motion_callback(id, LOWORD(lparam), HIWORD(lparam), AM_EVENT_WINDOW_MOTION);
            break;
        };
        case WM_DESTROY: {
            printf("Destroying window %d!\n", id);
            am_packed_array_erase(platform->windows, id)

            am_bool check_no_root = true;
            for (am_int32 i = 0; i < am_packed_array_get_count(platform->windows); i++)
                if (platform->windows->elements[i].parent == AM_WINDOW_DEFAULT_PARENT) {
                    printf("check no root false\n");
                    check_no_root = false;
                    break;
                };

            if (check_no_root) am_engine_get_instance()->is_running = false;
            break;
        };
        default: break;
    };
    return DefWindowProc(handle, event, wparam, lparam);
};
#endif

void am_platform_update(am_platform *platform) {
    memcpy(platform->input.mouse.prev_map, platform->input.mouse.map, sizeof(platform->input.mouse.map));
    memcpy(platform->input.keyboard.prev_map, platform->input.keyboard.map, sizeof(platform->input.keyboard.map));
    platform->input.mouse.wheel_delta = 0;
    platform->input.mouse.moved = false;
    platform->input.mouse.delta.x = 0.0f;
    platform->input.mouse.delta.y = 0.0f;
    am_platform_poll_events();
};

void am_platform_terminate(am_platform *platform) {
    if (platform->input.mouse.locked) am_platform_mouse_lock(false);
    for (am_int32 i = 0; i < am_packed_array_get_count(platform->windows); i++) am_platform_window_destroy(platform->windows->elements[i].id);
#if defined(AM_LINUX)
    am_platform_update(am_engine_get_subsystem(platform));
#else
    UnregisterClass(AM_ROOT_WIN_CLASS, GetModuleHandle(NULL));
    UnregisterClass(AM_CHILD_WIN_CLASS, GetModuleHandle(NULL));
#endif

    am_packed_array_destroy(platform->windows);
    am_free(platform);
};

void am_platform_key_press(am_key_map key) {
    if (key >= AM_KEYCODE_COUNT) return;
    am_platform *platform = am_engine_get_subsystem(platform);
    platform->input.keyboard.map[key] = true;
};

void am_platform_key_release(am_key_map key) {
    if (key >= AM_KEYCODE_COUNT) return;
    am_platform *platform = am_engine_get_subsystem(platform);
    platform->input.keyboard.map[key] = false;
};

am_bool am_platform_key_pressed(am_key_map key) {
    if (key >= AM_KEYCODE_COUNT) return false;
    am_platform *platform = am_engine_get_subsystem(platform);
    return platform->input.keyboard.map[key] && !platform->input.keyboard.prev_map[key];
};

am_bool am_platform_key_down(am_key_map key) {
    if (key >= AM_KEYCODE_COUNT) return false;
    am_platform *platform = am_engine_get_subsystem(platform);
    return platform->input.keyboard.map[key] && platform->input.keyboard.prev_map[key];
};

am_bool am_platform_key_released(am_key_map key) {
    if (key >= AM_KEYCODE_COUNT) return false;
    am_platform *platform = am_engine_get_subsystem(platform);
    return !platform->input.keyboard.map[key] && platform->input.keyboard.prev_map[key];
};

am_bool am_platform_key_up(am_key_map key) {
    if (key >= AM_KEYCODE_COUNT) return false;
    am_platform *platform = am_engine_get_subsystem(platform);
    return !platform->input.keyboard.map[key];
};

void am_platform_mouse_button_press(am_mouse_map button) {
    if (button >= AM_MOUSE_BUTTON_COUNT) return;
    am_platform *platform = am_engine_get_subsystem(platform);
    platform->input.mouse.map[button] = true;
};

void am_platform_mouse_button_release(am_mouse_map button) {
    if (button >= AM_MOUSE_BUTTON_COUNT) return;
    am_platform *platform = am_engine_get_subsystem(platform);
    platform->input.mouse.map[button] = false;

};

am_bool am_platform_mouse_button_pressed(am_mouse_map button) {
    if (button >= AM_MOUSE_BUTTON_COUNT) return false;
    am_platform *platform = am_engine_get_subsystem(platform);
    return platform->input.mouse.map[button] && !platform->input.mouse.prev_map[button];
};

am_bool am_platform_mouse_button_down(am_mouse_map button) {
    if (button >= AM_MOUSE_BUTTON_COUNT) return false;
    am_platform *platform = am_engine_get_subsystem(platform);
    return platform->input.mouse.map[button] && platform->input.mouse.prev_map[button];
};

am_bool am_platform_mouse_button_released(am_mouse_map button) {
    if (button >= AM_MOUSE_BUTTON_COUNT) return false;
    am_platform *platform = am_engine_get_subsystem(platform);
    return !platform->input.mouse.map[button] && platform->input.mouse.prev_map[button];
};

am_bool am_platform_mouse_button_up(am_mouse_map button) {
    if (button >= AM_MOUSE_BUTTON_COUNT) return false;
    am_platform *platform = am_engine_get_subsystem(platform);
    return !platform->input.mouse.map[button];
};

void am_platform_mouse_get_position(am_uint32 *x, am_uint32 *y) {
    am_platform *platform = am_engine_get_subsystem(platform);
    *x = platform->input.mouse.position.x;
    *y = platform->input.mouse.position.y;
};

am_vec2 am_platform_mouse_get_positionv() {
    am_platform *platform = am_engine_get_subsystem(platform);
    am_vec2 posv;
    posv.x = (am_float32)platform->input.mouse.position.x;
    posv.y = (am_float32)platform->input.mouse.position.y;
    return posv;
};

void am_platform_mouse_set_position(am_uint32 x, am_uint32 y) {
    am_platform *platform = am_engine_get_subsystem(platform);
    platform->input.mouse.position.x = x;
    platform->input.mouse.position.y = y;

#if defined(AM_LINUX)
    XWarpPointer(platform->display, None, am_packed_array_get_ptr(platform->windows, 1)->handle, 0, 0, 0, 0, (am_int32)x, (am_int32)y);
    XFlush(platform->display);
#else
    POINT pos = { (am_int32)x, (am_int32)y};
    ClientToScreen((HWND)am_packed_array_get_ptr(platform->windows, 1)->handle, &pos);
    SetCursorPos(pos.x, pos.y);
#endif
};

am_vec2 am_platform_mouse_get_delta() {
    return am_engine_get_subsystem(platform)->input.mouse.delta;
};

am_int32 am_platform_mouse_get_wheel_delta() {
    am_platform *platform = am_engine_get_subsystem(platform);
    return platform->input.mouse.wheel_delta;
};

am_bool am_platform_mouse_moved() {
    am_platform *platform = am_engine_get_subsystem(platform);
    return platform->input.mouse.moved;
};

void am_platform_mouse_lock(am_bool lock) {
    am_platform *platform = am_engine_get_subsystem(platform);

    if (platform->input.mouse.locked == lock) return;
    am_window *window_to_lock = am_packed_array_get_ptr(platform->windows, 1);
#if defined(AM_LINUX)
    if (lock) {
        platform->input.mouse.locked = lock;
        am_platform_mouse_get_position(&platform->input.mouse.cached_position.x, &platform->input.mouse.cached_position.y);
        XGrabPointer(platform->display, window_to_lock->handle, true,
                     ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                     GrabModeAsync, GrabModeAsync, window_to_lock->handle, 0, 0
        );
    } else {
        platform->input.mouse.locked = lock;
        XUngrabPointer(platform->display, 0);
        am_platform_mouse_set_position(platform->input.mouse.cached_position.x, platform->input.mouse.cached_position.y);
    };
#else
    if (lock) {
        platform->input.mouse.locked = lock;
        am_platform_mouse_get_position(&platform->input.mouse.cached_position.x, &platform->input.mouse.cached_position.y);
        RECT clipRect;
        GetClientRect((HWND)window_to_lock->handle, &clipRect);
        ClientToScreen((HWND)window_to_lock->handle, (POINT*) &clipRect.left);
        ClientToScreen((HWND)window_to_lock->handle, (POINT*) &clipRect.right);
        ClipCursor(&clipRect);
    } else {
        platform->input.mouse.locked = lock;
        ClipCursor(NULL);
        am_platform_mouse_set_position(platform->input.mouse.cached_position.x, platform->input.mouse.cached_position.y);
    }
#endif

};

void am_platform_key_callback_default(am_id id, am_key_map key, am_platform_events event) {
    am_platform *platform = am_engine_get_subsystem(platform);
    switch (event) {
        case AM_EVENT_KEY_PRESS: {
            platform->input.keyboard.map[key] = true;
            break;
        };
        case AM_EVENT_KEY_RELEASE: {
            platform->input.keyboard.map[key] = false;
            break;
        };
        default: break;
    };
};

void am_platform_mouse_button_callback_default(am_id id, am_mouse_map button, am_platform_events event) {
    am_platform *platform = am_engine_get_subsystem(platform);
    switch (event) {
        case AM_EVENT_MOUSE_BUTTON_PRESS: {
            platform->input.mouse.map[button] = true;
            break;
        };
        case AM_EVENT_MOUSE_BUTTON_RELEASE: {
            platform->input.mouse.map[button] = false;
            break;
        };
        default: break;
    };
};

void am_platform_mouse_motion_callback_default(am_id id, am_int32 x, am_int32 y, am_platform_events event) {
    am_platform *platform = am_engine_get_subsystem(platform);
    if (!platform->input.mouse.locked) {
        platform->input.mouse.moved = true;
        platform->input.mouse.delta.x = (am_float32)(x - platform->input.mouse.position.x);
        platform->input.mouse.delta.y = (am_float32)(y - platform->input.mouse.position.y);
        platform->input.mouse.position.x = x;
        platform->input.mouse.position.y = y;
    } else {
        am_float32 dx = (am_float32)x - (am_float32)platform->input.mouse.position.x;
        am_float32 dy = (am_float32)y - (am_float32)platform->input.mouse.position.y;
        platform->input.mouse.position.x = x;
        platform->input.mouse.position.y = y;
        platform->input.mouse.delta = am_vec2_add(platform->input.mouse.delta, am_vec2_create(dx, dy));
    };
};

void am_platform_mouse_scroll_callback_default(am_id id, am_platform_events event) {
    am_platform *platform = am_engine_get_subsystem(platform);
    switch (event) {
        case AM_EVENT_MOUSE_SCROLL_UP: {
            ++platform->input.mouse.wheel_delta;
            break;
        };
        case AM_EVENT_MOUSE_SCROLL_DOWN: {
            --platform->input.mouse.wheel_delta;
            break;
        };
        default: break;
    };
};

void am_platform_window_size_callback_default(am_id id, am_uint32 width, am_uint32 height, am_platform_events event) {
    am_platform *platform = am_engine_get_subsystem(platform);
    am_window *window = am_packed_array_get_ptr(platform->windows, id);
    if (!window) {
        printf("[FAIL] am_platform_window_size_callback_default: Window not found on size callback!\n");
        return;
    };

    window->cache.width = window->width;
    window->cache.height = window->height;
    window->width = width;
    window->height = height;
    printf("Window size callback: %d %d\n", width, height);
};

void am_platform_window_motion_callback_default(am_id id, am_uint32 x, am_uint32 y, am_platform_events event) {
    am_platform *platform = am_engine_get_subsystem(platform);
    am_window *window = am_packed_array_get_ptr(platform->windows, id);
    if (!window) {
        printf("[FAIL] am_platform_window_motion_callback_default: Window not found on motion callback!\n");
        return;
    };
    window->cache.x = window->x;
    window->cache.y = window->y;
    window->x = x;
    window->y = y;
    printf("Window pos callback: %d %d\n", x, y);
};

am_id am_platform_window_create(am_window_info window_info) {
    am_platform *platform = am_engine_get_subsystem(platform);

    am_window *new_window = (am_window*)am_malloc(sizeof(am_window));
    if (new_window == NULL) {
        printf("[FAIL] am_platform_window_create (id: %u): Could not allocate memory for window!\n", platform->windows->next_id);
        return AM_PA_INVALID_ID;
    };

    am_id ret_id = am_packed_array_add(platform->windows, *new_window);
    am_free(new_window);
    new_window = am_packed_array_get_ptr(platform->windows, ret_id);
    new_window->id = ret_id;

    //Defaults
    if (!window_info.x) {
        window_info.x = AM_WINDOW_DEFAULT_X;
        printf("[WARN] am_platform_window_create (id: %u): info.x is 0, setting to default (%d)!\n", ret_id, AM_WINDOW_DEFAULT_X);
    };
    if (!window_info.y) {
        window_info.y = AM_WINDOW_DEFAULT_Y;
        printf("[WARN] am_platform_window_create (id: %u): info.y is 0, setting to default (%d)!\n", ret_id, AM_WINDOW_DEFAULT_Y);
    }
    if (!window_info.width) {
        window_info.width = AM_WINDOW_DEFAULT_WIDTH;
        printf("[WARN] am_platform_window_create (id: %u): info.width is 0, setting to default (%d)!\n", ret_id, AM_WINDOW_DEFAULT_WIDTH);
    }
    if (!window_info.height) {
        window_info.height = AM_WINDOW_DEFAULT_HEIGHT;
        printf("[WARN] am_platform_window_create (id: %u): info.height is 0, setting to default (%d)!\n", ret_id, AM_WINDOW_DEFAULT_HEIGHT);
    }
    if (!strlen(window_info.name)) {
        snprintf(window_info.name, AM_MAX_NAME_LENGTH, "%s%d", AM_WINDOW_DEFAULT_NAME, ret_id);
        printf("[WARN] am_platform_window_create (id: %u): info.name is empty, setting default name (%s)!\n", ret_id, window_info.name);
    };
    if (!window_info.parent) {
        window_info.parent = AM_WINDOW_DEFAULT_PARENT;
        printf("[WARN] am_platform_window_create (id: %u): info.parent is 0, setting default parent (%lu)!\n", ret_id, AM_WINDOW_DEFAULT_PARENT);
    };
    if (!window_info.is_fullscreen) {
        window_info.is_fullscreen = false;
    };


#if defined(AM_LINUX)
    XSetWindowAttributes window_attributes;
    am_int32 attribs[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
    new_window->visual_info = glXChooseVisual(platform->display, 0, attribs);
    new_window->colormap = XCreateColormap(platform->display, window_info.parent, new_window->visual_info->visual, AllocNone);
    window_attributes.colormap = new_window->colormap;
    window_attributes.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |ButtonPressMask | ButtonReleaseMask | PointerMotionMask | FocusChangeMask | StructureNotifyMask | EnterWindowMask | LeaveWindowMask;
    am_uint64 window = (am_uint64)XCreateWindow(platform->display, window_info.parent,
                                                (am_int32)window_info.x, (am_int32)window_info.y,
                                                window_info.width, window_info.height, 0,
                                                new_window->visual_info->depth, InputOutput,
                                                new_window->visual_info->visual, CWColormap | CWEventMask,
                                                &window_attributes
    );
    if (window == BadAlloc || window == BadColor || window == BadCursor || window == BadMatch || window == BadPixmap || window == BadValue || window == BadWindow) {
        printf("[FAIL] am_platform_window_create (id: %u): Could not create window!\n", ret_id);
        am_packed_array_erase(platform->windows, ret_id);
        return AM_PA_INVALID_ID;
    };
    new_window->handle = window;

    Atom wm_delete = XInternAtom(platform->display, "WM_DELETE_WINDOW", true);
    XSetWMProtocols(platform->display, (Window)new_window->handle, &wm_delete, 1);
    XStoreName(platform->display, (Window)new_window->handle, window_info.name);
    XMapWindow(platform->display, (Window)new_window->handle);

#else
    DWORD dwExStyle = WS_EX_LEFT; // 0
    DWORD dwStyle = WS_OVERLAPPED; // 0
    if (window_info.parent == AM_WINDOW_DEFAULT_PARENT) {
        dwStyle = WS_CAPTION | WS_SYSMENU | WS_VISIBLE | WS_THICKFRAME;
        dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    };

    RECT window_rect = {
            .left = 0,
            .right = (long)window_info.width,
            .top = 0,
            .bottom = (long)window_info.height
    };
    AdjustWindowRectEx(&window_rect, dwStyle, false, dwExStyle);
    am_int32 rect_width = window_rect.right - window_rect.left;
    am_int32 rect_height = window_rect.bottom - window_rect.top;

    new_window->handle = (am_uint64)CreateWindowEx(dwExStyle, window_info.parent == AM_WINDOW_DEFAULT_PARENT ? AM_ROOT_WIN_CLASS:AM_CHILD_WIN_CLASS,
                                                   window_info.name, dwStyle, (am_int32)window_info.x, (am_int32)window_info.y, rect_width, rect_height,
                                                   NULL, NULL, GetModuleHandle(NULL), NULL
    );
    if (new_window->handle == 0) {
        printf("[FAIL] am_platform_window_create (id: %u): Could not create window!\n", new_window->id);
        return AM_PA_INVALID_ID;
    };
    if ((window_info.parent != AM_WINDOW_DEFAULT_PARENT)) {
        SetParent((HWND)new_window->handle, (HWND)window_info.parent);
        SetWindowLong((HWND)new_window->handle, GWL_STYLE, 0);
    };
    ShowWindow((HWND)new_window->handle, 1);
#endif

    new_window->x = window_info.x;
    new_window->y = window_info.y;
    //This is set to false due to how the is_fullscreen toggle works, it will be correctly assigned after am_platform_window_fullscreen
    new_window->is_fullscreen = false;
    new_window->parent = window_info.parent;
    new_window->height = window_info.height;
    new_window->width = window_info.width;
    snprintf(new_window->name, AM_MAX_NAME_LENGTH, "%s", window_info.name);
    am_platform_window_fullscreen(new_window->id, window_info.is_fullscreen);

    am_window *main_window = am_packed_array_get_ptr(platform->windows, 1);
#if defined(AM_LINUX)
    new_window->context = NULL;
    new_window->context = glXCreateContext(platform->display, new_window->visual_info, main_window->context, GL_TRUE);
    glXMakeCurrent(platform->display, new_window->handle, new_window->context);
#else
    new_window->hdc = GetDC((HWND)new_window->handle);
    PIXELFORMATDESCRIPTOR pixel_format_desc = {
            sizeof(PIXELFORMATDESCRIPTOR),1,
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA,
            32,0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0,
            24,8,0, PFD_MAIN_PLANE, 0, 0, 0, 0
    };

    am_int32 suggested_pf_index = ChoosePixelFormat(new_window->hdc, &pixel_format_desc);
    PIXELFORMATDESCRIPTOR suggested_pf;
    DescribePixelFormat(new_window->hdc, suggested_pf_index, sizeof(suggested_pf), &suggested_pf);
    SetPixelFormat(new_window->hdc, suggested_pf_index, &suggested_pf);

    new_window->context = wglCreateContext(new_window->hdc);
    wglMakeCurrent(new_window->hdc, new_window->context);
    wglShareLists(new_window->context, main_window->context);
#endif

    //Have to do this here unfortunately
    if (am_packed_array_get_count(platform->windows) <= 1)
        if (!gladLoadGL()) printf("[FAIL] am_platform_window_create (id: %u): Could not load OpenGL functions!\n", new_window->id);

    amgl_vsync(new_window->id, am_engine_get_instance()->vsync_enabled);

#if defined(AM_LINUX)
    glXMakeCurrent(platform->display, 0, 0);
#else
    wglMakeCurrent(0,0);
#endif
    return ret_id;
};

void am_platform_window_resize(am_id id, am_uint32 width, am_uint32 height) {
    am_platform *platform  = am_engine_get_subsystem(platform);
    am_window *window = am_packed_array_get_ptr(platform->windows, id);
    if (!window) {
        printf("[FAIL] am_platform_window_resize (id: %u): Window id is invalid!\n", id);
        return;
    };
    window->cache.width = window->width;
    window->cache.height = window->height;
#if defined(AM_LINUX)
    XResizeWindow(platform->display, window->handle, width, height);
#else
    RECT rect = {
            .left = 0,
            .top = 0,
            .bottom = (long)window->height,
            .right = (long)window->width
    };
    if (window->parent == AM_WINDOW_DEFAULT_PARENT)
        AdjustWindowRectEx(&rect, WS_CAPTION | WS_SYSMENU | WS_VISIBLE | WS_THICKFRAME, false, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
    else
        AdjustWindowRectEx(&rect, 0, false, 0);

    SetWindowPos((HWND)window->handle, 0, 0, 0 , rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_NOMOVE | SWP_FRAMECHANGED);
#endif
};

void am_platform_window_move(am_id id, am_uint32 x, am_uint32 y) {
    am_platform *platform = am_engine_get_subsystem(platform);
    am_window *window = am_packed_array_get_ptr(platform->windows, id);
    if (!window) {
        printf("[FAIL] am_platform_window_move (id: %u): Window id is invalid!\n", id);
        return;
    };
    window->cache.x = window->x;
    window->cache.y = window->y;

#if defined(AM_LINUX)
    XMoveWindow(platform->display, window->handle, (am_int32)x, (am_int32)y);
#else
    RECT rect = {
            .left = (long)x,
            .top = (long)y,
            .bottom = (long)window->height,
            .right = (long)window->width
    };
    if (window->parent == AM_WINDOW_DEFAULT_PARENT)
        AdjustWindowRectEx(&rect, WS_CAPTION | WS_SYSMENU | WS_VISIBLE | WS_THICKFRAME, false, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
    else
        AdjustWindowRectEx(&rect, 0, false, 0);
    SetWindowPos((HWND)window->handle, 0, rect.left, rect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_FRAMECHANGED);
#endif
};

void am_platform_window_fullscreen(am_id id, am_bool state) {
    am_platform *platform = am_engine_get_subsystem(platform);
    am_window *window = am_packed_array_get_ptr(platform->windows, id);
    printf("at fs %d\n", window->width);
    if (!window) {
        printf("[FAIL] am_platform_window_fullscreen (id: %u): Window id is invalid!\n", id);
        return;
    };
    if (window->is_fullscreen == state || window->parent != AM_WINDOW_DEFAULT_PARENT) return;

    window->cache.is_fullscreen = window->is_fullscreen;
    window->is_fullscreen = state;
    am_window_info temp_info = {
        .width = window->width,
        .height = window->height,
        .x = window->x,
        .y = window->y,
        .is_fullscreen = window->is_fullscreen,
        .parent = window->parent,
    };
    snprintf(temp_info.name, AM_MAX_NAME_LENGTH, "%s", window->name);
#if defined(AM_WINDOWS)
    am_window_cache temp_cache = window->cache;
#endif

#if defined(AM_LINUX)
    Atom wm_state = XInternAtom(platform->display, "_NET_WM_STATE", false);
    Atom wm_fs = XInternAtom(platform->display, "_NET_WM_STATE_FULLSCREEN", false);
    XEvent xevent = {0};
    xevent.type = ClientMessage;
    xevent.xclient.window = window->handle;
    xevent.xclient.message_type = wm_state;
    xevent.xclient.format = 32;
    xevent.xclient.data.l[0] = state ? 1:0;
    xevent.xclient.data.l[1] = (long)wm_fs;
    xevent.xclient.data.l[3] = 0l;
    XSendEvent(platform->display, AM_WINDOW_DEFAULT_PARENT, false, SubstructureRedirectMask | SubstructureNotifyMask, &xevent);
    XFlush(platform->display);
    XWindowAttributes window_attribs = {0};
    XGetWindowAttributes(platform->display, window->handle, &window_attribs);
    printf("Fullscreen toggle\n Pos: %d %d\n Size: %d %d\nFullscreen toggle end\n\n", window_attribs.x, window_attribs.y, window_attribs.width, window_attribs.height);
    memcpy(&window->cache, &temp_info, sizeof(am_window_info));
#else
    DWORD dw_style = GetWindowLong((HWND)window->handle, GWL_STYLE);
    if (window->is_fullscreen) {
        printf("Going fullscreen\n");
        MONITORINFO monitor_info = {sizeof(monitor_info)};
        GetMonitorInfo(MonitorFromWindow((HWND)window->handle, MONITOR_DEFAULTTONEAREST), &monitor_info);
        SetWindowLong((HWND)window->handle, GWL_STYLE, dw_style & ~WS_OVERLAPPEDWINDOW);
        SetWindowPos((HWND)window->handle, HWND_TOP, monitor_info.rcMonitor.left, monitor_info.rcMonitor.top,
                     monitor_info.rcMonitor.right - monitor_info.rcMonitor.left, monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED
        );
        memcpy(&window->cache, &temp_info, sizeof(am_window_info));
    } else {
        printf("Going not is_fullscreen\n");
        SetWindowLong((HWND)window->handle, GWL_STYLE, dw_style | WS_OVERLAPPEDWINDOW);
        am_platform_window_resize(id, temp_cache.width, temp_cache.height);
        am_platform_window_move(id, temp_cache.x, temp_cache.y);
    };
#endif
};

am_vec2u am_platform_window_get_size(am_id id) {
    am_platform *platform = am_engine_get_subsystem(platform);
    am_window *window = am_packed_array_get_ptr(platform->windows, id);
#if defined(AM_LINUX)
#else
    RECT area;
    GetClientRect((HWND)window->handle, &area);
    am_vec2u ret = {area.right, area.bottom};
    return ret;
#endif
};

void am_platform_window_destroy(am_id id) {
    am_platform *platform = am_engine_get_subsystem(platform);
    am_window *window = am_packed_array_get_ptr(platform->windows, id);
    if (!window) {
        printf("[FAIL] am_platform_window_destroy (id: %u): Window id is invalid!\n", id);
        return;
    };
#if defined(AM_LINUX)
    glXDestroyContext(platform->display, window->context);
    XUnmapWindow(platform->display, window->handle);
    XDestroyWindow(platform->display, window->handle);
    XFreeColormap(platform->display, window->colormap);
    XFree(window->visual_info);
    XFlush(platform->display);
#else
    DestroyWindow((HWND)(window->handle));
#endif
};

void am_platform_timer_create() {
    am_platform *platform = am_engine_get_subsystem(platform);
#if defined(AM_LINUX)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    platform->time.offset = (am_uint64)ts.tv_sec * (am_uint64)1000000000 + (am_uint64)ts.tv_nsec;

    platform->time.frequency = 1000000000;
#else
    QueryPerformanceFrequency((LARGE_INTEGER*)&platform->time.frequency);
    QueryPerformanceCounter((LARGE_INTEGER*)&platform->time.offset);
#endif
};

void am_platform_timer_sleep(am_float32 ms) {
#if defined(AM_LINUX)
    usleep((__useconds_t)(ms*1000.0f));
#else
    timeBeginPeriod(1);
    Sleep((uint64_t)ms);
    timeEndPeriod(1);
#endif
};

am_uint64 am_platform_timer_value() {
#if defined(AM_LINUX)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (am_uint64)ts.tv_sec * (am_uint64)1000000000 + (am_uint64)ts.tv_nsec;
#else
    am_uint64 value;
    QueryPerformanceCounter((LARGE_INTEGER*)&value);
    return value;
#endif
};

am_uint64 am_platform_elapsed_time() {
    am_platform *platform = am_engine_get_subsystem(platform);
    return (am_platform_timer_value() - platform->time.offset);
};

//----------------------------------------------------------------------------//
//                              END PLATFORM IMPL                             //
//----------------------------------------------------------------------------//


//----------------------------------------------------------------------------//
//                                START GL IMPL                               //
//----------------------------------------------------------------------------//

am_id amgl_shader_create(amgl_shader_info info) {
    am_engine *engine = am_engine_get_instance();

    if (!info.num_sources || info.sources == NULL) {
        printf("[FAIL] amgl_shader_create (id: %u): No shader sources provided!\n", engine->ctx_data.shaders->next_id);
        return AM_PA_INVALID_ID;
    };

    amgl_shader *new_shader = (amgl_shader*)am_malloc(sizeof(amgl_shader));
    if (new_shader == NULL) {
        printf("[FAIL] amgl_shader_create (id: %u): Could not allocate memory for shader!\n", engine->ctx_data.shaders->next_id);
        return AM_PA_INVALID_ID;
    };

    am_int32 ret_id = am_packed_array_add(engine->ctx_data.shaders, *new_shader);
    am_free(new_shader);
    new_shader = am_packed_array_get_ptr(engine->ctx_data.shaders, ret_id);
    new_shader->id = ret_id;

    am_uint32 main_shader = glCreateProgram();
    am_uint32 shader_list[info.num_sources];
    am_bool is_vertex = false;
    am_bool is_compute = false;
    for (am_int32 i = 0; i < info.num_sources; i++) {
        am_uint32 shader = 0;

        if (!info.sources[i].path) {
            printf("[WARN] amgl_shader_create (id: %u): No path given for shader file for index %u, assuming info.sources[%u].source is given!\n", ret_id, i, i);
        } else {
            if (access(info.sources[i].path, F_OK)) {
                printf("[FAIL] amgl_shader_create (id: %u): Path given for shader file (%s) does not exist for index %u!\n", ret_id, info.sources[i].path, i);
                break;
            };
            info.sources[i].source = am_util_read_file(info.sources[i].path);
        };

        if (!info.sources[i].type) {
            printf("[FAIL] amgl_shader_create (id: %u): No shader type provided for index %d!\n", ret_id, i);
            break;
        }

        switch (info.sources[i].type) {
            case AMGL_SHADER_FRAGMENT: {
                shader = glCreateShader(GL_FRAGMENT_SHADER);
                break;
            };
            case AMGL_SHADER_VERTEX: {
                shader = glCreateShader(GL_VERTEX_SHADER);
                is_vertex = true;
                break;
            };
            case AMGL_SHADER_GEOMETRY: {
                shader = glCreateShader(GL_GEOMETRY_SHADER);
                break;
            };
            case AMGL_SHADER_COMPUTE: {
                shader = glCreateShader(GL_COMPUTE_SHADER);
                is_compute = true;
                break;
            }
            default: break;
        };

        if (is_vertex && is_compute) {
            printf("[FAIL] amgl_shader_create (id: %u): Compute & vertex shaders cannot go into the same program!\n", ret_id);
            glDeleteShader(shader);
            break;
        };

        glShaderSource(shader, 1, (const GLchar**)&info.sources[i].source, NULL);
        glCompileShader(shader);
        shader_list[i] = shader;// For detaching after linking

        am_int32 compile_result = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_result);
        if (!compile_result) {
            am_int32 length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
            char err_log[length];
            glGetShaderInfoLog(shader, length, &length, err_log);
            printf("[FAIL] amgl_shader_create (id: %u): Shader compilation failed for index %u:\n%s\n", ret_id, i, err_log);
            glDeleteShader(shader);
        };
        glAttachShader(main_shader, shader);
    };
    glLinkProgram(main_shader);

    am_int32 is_linked = 0;
    glGetProgramiv(main_shader, GL_LINK_STATUS, &is_linked);
    if (!is_linked) {
        am_int32 length = 0;
        glGetProgramiv(main_shader, GL_INFO_LOG_LENGTH, &length);
        char err_log[length];
        glGetProgramInfoLog(main_shader, length, &length, err_log);
        printf("[FAIL] amgl_shader_create (id: %u): Failed to link shader:\n%s\n", ret_id, err_log);
        am_packed_array_erase(engine->ctx_data.shaders, ret_id);
        glDeleteProgram(main_shader);
        return AM_PA_INVALID_ID;
    };

    for (am_int32 i = 0; i < info.num_sources; i++) {
        glDetachShader(main_shader, shader_list[i]);
        glDeleteShader(shader_list[i]);
        am_free(info.sources[i].source);
    };

    new_shader->handle = main_shader;
    if (!strlen(info.name)) {
        snprintf(info.name, AM_MAX_NAME_LENGTH, "%s%d", AMGL_SHADER_DEFAULT_NAME, ret_id);
        printf("[WARN] amgl_shader_create (id: %u): info.name is empty, choosing default name (%s)!\n", ret_id, info.name);
    };
    snprintf(new_shader->name, AM_MAX_NAME_LENGTH, "%s", info.name);
    return ret_id;
};

void amgl_shader_compute_dispatch(am_int32 x, am_int32 y, am_int32 z) {
    am_engine *engine = am_engine_get_instance();
    if (!am_packed_array_has(engine->ctx_data.pipelines, engine->ctx_data.frame_cache.pipeline.id)) {
        printf("[FAIL] amgl_shader_compute_dispatch (id: %u): Compute pipeline not bound!\n", engine->ctx_data.frame_cache.pipeline.id);
        return;
    };

    amgl_pipeline *pipeline = am_packed_array_get_ptr(engine->ctx_data.pipelines, engine->ctx_data.frame_cache.pipeline.id);
    if (!pipeline->compute.compute_shader) {
        printf("[FAIL] amgl_shader_compute_dispatch (id: %u): Compute shader not bound!\n", engine->ctx_data.frame_cache.pipeline.id);
        return;
    };

    glDispatchCompute(x, y, z);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

};

void amgl_shader_destroy(am_id id) {
    am_engine *engine = am_engine_get_instance();
    amgl_shader *shader = am_packed_array_get_ptr(engine->ctx_data.shaders, id);
    if (!shader) {
        printf("[FAIL] amgl_shader_destroy (id: %u): Invalid shader id!\n", id);
        return;
    };
    glDeleteProgram(shader->handle);
    am_packed_array_erase(engine->ctx_data.shaders, id);
};

am_id amgl_vertex_buffer_create(amgl_vertex_buffer_info info) {
    am_engine *engine = am_engine_get_instance();

    if (info.data == NULL) printf("[WARN] amgl_vertex_buffer_create (id: %u): Data pointer is NULL!\n", engine->ctx_data.vertex_buffers->next_id);
    if (info.size == 0) printf("[WARN] amgl_vertex_buffer_create (id: %u): Size is not specified!\n", engine->ctx_data.vertex_buffers->next_id);

    amgl_vertex_buffer *v_buffer = (amgl_vertex_buffer*)am_malloc(sizeof(amgl_vertex_buffer));
    if (v_buffer == NULL) {
        printf("[FAIL] amgl_vertex_buffer_create (id: %u): Could not allocate memory for vertex buffer!\n", engine->ctx_data.vertex_buffers->next_id);
        return AM_PA_INVALID_ID;
    };

    am_id ret_id = am_packed_array_add(engine->ctx_data.vertex_buffers, *v_buffer);
    am_free(v_buffer);
    v_buffer = am_packed_array_get_ptr(engine->ctx_data.vertex_buffers, ret_id);
    v_buffer->id = ret_id;

    if (!info.usage) {
        printf("[WARN] amgl_vertex_buffer_create (id: %u): No usage provided, choosing default!\n", ret_id);
        info.usage = AMGL_BUFFER_USAGE_STATIC;
    };

    am_int32 usage = 0;
    switch (info.usage) {
        case AMGL_BUFFER_USAGE_STATIC: usage = GL_STATIC_DRAW; break;
        case AMGL_BUFFER_USAGE_STREAM: usage = GL_STREAM_DRAW; break;
        case AMGL_BUFFER_USAGE_DYNAMIC: usage = GL_DYNAMIC_DRAW; break;
        default: {
            printf("[FAIL] amgl_vertex_buffer_create (id: %u): Invalid usage value!\n", ret_id);
            am_packed_array_erase(engine->ctx_data.vertex_buffers, ret_id);
            return AM_PA_INVALID_ID;
        };
    };

    glGenBuffers(1, &v_buffer->handle);
    glBindBuffer(GL_ARRAY_BUFFER, v_buffer->handle);
    glBufferData(GL_ARRAY_BUFFER, (long int)info.size, info.data, usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (!strlen(info.name)) {
        snprintf(info.name, AM_MAX_NAME_LENGTH, "%s%d", AMGL_VERTEX_BUFFER_DEFAULT_NAME, v_buffer->id);
        printf("[WARN] amgl_vertex_buffer_create (id: %u): Choosing default name!\n", v_buffer->id);
    };
    snprintf(v_buffer->name, AM_MAX_NAME_LENGTH, "%s", info.name);

    v_buffer->update.size = info.size;
    v_buffer->update.usage = info.usage;
    return ret_id;
};

void amgl_vertex_buffer_update(am_id id, amgl_vertex_buffer_update_info update) {
    am_engine *engine = am_engine_get_instance();
    if (!am_packed_array_has(engine->ctx_data.vertex_buffers, id)) printf("[FAIL] amgl_vertex_buffer_update (id: %u): No entry found by id!\n", id);
    amgl_vertex_buffer *buffer = am_packed_array_get_ptr(engine->ctx_data.vertex_buffers, id);

    glBindBuffer(GL_ARRAY_BUFFER, buffer->handle);
    if (update.size > buffer->update.size) {
        am_int32 usage = 0;
        switch (update.usage) {
            case AMGL_BUFFER_USAGE_STATIC: usage = GL_STATIC_DRAW; break;
            case AMGL_BUFFER_USAGE_STREAM: usage = GL_STREAM_DRAW; break;
            case AMGL_BUFFER_USAGE_DYNAMIC: usage = GL_DYNAMIC_DRAW; break;
            default: break;
        };
        glBufferData(GL_ARRAY_BUFFER, (long int)update.size, update.data, usage);
        buffer->update.usage = update.usage;
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, (long int)update.offset, (long int)update.size, update.data);
    };
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    buffer->update.size = update.size;
};

void amgl_vertex_buffer_destroy(am_id id) {
    am_engine *engine = am_engine_get_instance();
    amgl_vertex_buffer *vbuffer = am_packed_array_get_ptr(engine->ctx_data.vertex_buffers, id);
    if (!vbuffer) {
        printf("[FAIL] amgl_vertex_buffer_destroy (id: %u): Invalid vertex buffer id!\n", id);
        return;
    }
    glDeleteBuffers(1, &vbuffer->handle);
    am_packed_array_erase(engine->ctx_data.vertex_buffers, id);
};

am_id amgl_index_buffer_create(amgl_index_buffer_info info) {
    am_engine *engine = am_engine_get_instance();

    if (info.data == NULL) printf("[WARN] amgl_index_buffer_create (id: %u): Data pointer is NULL!\n", engine->ctx_data.index_buffers->next_id);
    if (info.size == 0) printf("[WARN] amgl_index_buffer_create (id: %u): Size is not specified!\n", engine->ctx_data.index_buffers->next_id);

    amgl_index_buffer *index_bfr = (amgl_index_buffer*)malloc(sizeof(amgl_index_buffer));
    if (index_bfr == NULL) {
        printf("[FAIL] amgl_vertex_index_create (id: %u): Could not allocate memory for index buffer!\n", engine->ctx_data.index_buffers->next_id);
        return AM_PA_INVALID_ID;
    };

    am_id ret_id = am_packed_array_add(engine->ctx_data.index_buffers, *index_bfr);
    am_free(index_bfr);
    index_bfr = am_packed_array_get_ptr(engine->ctx_data.index_buffers, ret_id);
    index_bfr->id = ret_id;

    glGenBuffers(1, &index_bfr->handle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_bfr->handle);

    if (!info.usage) {
        printf("[WARN] amgl_index_buffer_create (id: %u): No usage provided, choosing default!\n", ret_id);
        info.usage = AMGL_BUFFER_USAGE_STATIC;
    };

    switch (info.usage) {
        case AMGL_BUFFER_USAGE_STATIC: {
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)info.size, info.data, GL_STATIC_DRAW);
            break;
        };
        case AMGL_BUFFER_USAGE_STREAM: {
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)info.size, info.data, GL_STREAM_DRAW);
            break;
        };
        case AMGL_BUFFER_USAGE_DYNAMIC: {
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)info.size, info.data, GL_DYNAMIC_DRAW);
            break;
        };
        default: {
            printf("[FAIL] amgl_index_buffer_create (id: %u): Invalid usage!\n", ret_id);
            am_packed_array_erase(engine->ctx_data.index_buffers, ret_id);
            return AM_PA_INVALID_ID;
        };
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    if (!strlen(info.name)) {
        snprintf(info.name, AM_MAX_NAME_LENGTH, "%s%d", AMGL_INDEX_BUFFER_DEFAULT_NAME, index_bfr->id);
        printf("[WARN] amgl_index_buffer_create  (id: %u): Choosing default name!\n", index_bfr->id);
    };
    snprintf(index_bfr->name, AM_MAX_NAME_LENGTH, "%s", info.name);

    index_bfr->update.size = info.size;
    index_bfr->update.usage = info.usage;
    return ret_id;
};

void amgl_index_buffer_update(am_id id, amgl_index_buffer_update_info update) {
    am_engine *engine = am_engine_get_instance();
    if (!am_packed_array_has(engine->ctx_data.index_buffers, id)) printf("[FAIL] amgl_index_buffer_update (id: %u): No entry found by id!\n", id);
    amgl_index_buffer *buffer = am_packed_array_get_ptr(engine->ctx_data.index_buffers, id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->handle);
    if (update.size > buffer->update.size) {
        am_int32 usage = 0;
        switch (update.usage) {
            case AMGL_BUFFER_USAGE_STATIC: usage = GL_STATIC_DRAW; break;
            case AMGL_BUFFER_USAGE_STREAM: usage = GL_STREAM_DRAW; break;
            case AMGL_BUFFER_USAGE_DYNAMIC: usage = GL_DYNAMIC_DRAW; break;
            default: break;
        };
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (long int)update.size, update.data, usage);
        buffer->update.usage = update.usage;
    } else {
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, (long int)update.offset, (long int)update.size, update.data);
    };
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    buffer->update.size = update.size;
};

void amgl_index_buffer_destroy(am_id id) {
    am_engine *engine = am_engine_get_instance();
    amgl_index_buffer *index_buffer = am_packed_array_get_ptr(engine->ctx_data.index_buffers, id);
    if (!index_buffer) {
        printf("[FAIL] amgl_index_buffer_destroy (id: %u): Invalid index buffer id!\n", id);
        return;
    };
    glDeleteBuffers(1, &index_buffer->handle);
    am_packed_array_erase(engine->ctx_data.index_buffers, id);
};

am_id amgl_storage_buffer_create(amgl_storage_buffer_info info) {
    am_engine *engine = am_engine_get_instance();

    if (info.data == NULL) printf("[WARN] amgl_storage_buffer_create (id: %u): Data pointer is NULL!\n", engine->ctx_data.storage_buffers->next_id);
    if (info.size == 0) printf("[WARN] amgl_storage_buffer_create (id: %u): Size is not specified!\n", engine->ctx_data.storage_buffers->next_id);

    amgl_storage_buffer *storage_bfr = (amgl_storage_buffer*)malloc(sizeof(amgl_storage_buffer));
    if (storage_bfr == NULL) {
        printf("[FAIL] amgl_vertex_index_create (id: %u): Could not allocate memory for storage buffer!\n", engine->ctx_data.storage_buffers->next_id);
        return AM_PA_INVALID_ID;
    };

    am_id ret_id = am_packed_array_add(engine->ctx_data.storage_buffers, *storage_bfr);
    am_free(storage_bfr);
    storage_bfr = am_packed_array_get_ptr(engine->ctx_data.storage_buffers, ret_id);
    storage_bfr->id = ret_id;

    glGenBuffers(1, &storage_bfr->handle);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, storage_bfr->handle);

    if (!info.usage) {
        printf("[WARN] amgl_storage_buffer_create (id: %u): No usage provided, choosing default!\n", ret_id);
        info.usage = AMGL_BUFFER_USAGE_STATIC;
    };

    switch (info.usage) {
        case AMGL_BUFFER_USAGE_STATIC: {
            glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)info.size, info.data, GL_STATIC_DRAW);
            break;
        };
        case AMGL_BUFFER_USAGE_STREAM: {
            glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)info.size, info.data, GL_STREAM_DRAW);
            break;
        };
        case AMGL_BUFFER_USAGE_DYNAMIC: {
            glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)info.size, info.data, GL_DYNAMIC_DRAW);
            break;
        };
        default: {
            printf("[FAIL] amgl_storage_buffer_create (id: %u): Invalid usage!\n", ret_id);
            am_packed_array_erase(engine->ctx_data.storage_buffers, ret_id);
            return AM_PA_INVALID_ID;
        };
    };
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    if (!strlen(info.name)) {
        snprintf(info.name, AM_MAX_NAME_LENGTH, "%s%d", AMGL_INDEX_BUFFER_DEFAULT_NAME, storage_bfr->id);
        printf("[WARN] amgl_storage_buffer_create  (id: %u): Choosing default name!\n", storage_bfr->id);
    };
    snprintf(storage_bfr->name, AM_MAX_NAME_LENGTH, "%s", info.name);

    storage_bfr->update.size = info.size;
    storage_bfr->update.usage = info.usage;
    storage_bfr->block_index = 0xFFFFFFFF;
    return ret_id;
};

void amgl_storage_buffer_update(am_id id, amgl_storage_buffer_update_info update) {
    am_engine *engine = am_engine_get_instance();
    if (!am_packed_array_has(engine->ctx_data.storage_buffers, id)) printf("[FAIL] amgl_storage_buffer_update (id: %u): No entry found by id!\n", id);
    amgl_storage_buffer *buffer = am_packed_array_get_ptr(engine->ctx_data.storage_buffers, id);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer->handle);
    if (update.size > buffer->update.size) {
        am_int32 usage = 0;
        switch (update.usage) {
            case AMGL_BUFFER_USAGE_STATIC: usage = GL_STATIC_DRAW; break;
            case AMGL_BUFFER_USAGE_STREAM: usage = GL_STREAM_DRAW; break;
            case AMGL_BUFFER_USAGE_DYNAMIC: usage = GL_DYNAMIC_DRAW; break;
            default: break;
        };
        glBufferData(GL_SHADER_STORAGE_BUFFER, (long int)update.size, update.data, usage);
        buffer->update.usage = update.usage;

    } else {
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, (long int)update.offset, (long int)update.size, update.data);
    };
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    buffer->update.size = update.size;

};

void amgl_storage_buffer_destroy(am_id id) {
    am_engine *engine = am_engine_get_instance();
    amgl_storage_buffer *storage_buffer = am_packed_array_get_ptr(engine->ctx_data.storage_buffers, id);
    if (!storage_buffer) {
        printf("[FAIL] amgl_storage_buffer_destroy (id: %u): Invalid storage buffer id!\n", id);
        return;
    };
    glDeleteBuffers(1, &storage_buffer->handle);
    am_packed_array_erase(engine->ctx_data.storage_buffers, id);
};


am_id amgl_uniform_create(amgl_uniform_info info) {
    am_engine *engine = am_engine_get_instance();

    amgl_uniform *uniform = (amgl_uniform*)malloc(sizeof(amgl_uniform));
    if (uniform == NULL) {
        printf("[FAIL] amgl_uniform_create (id: %u): Could not allocate memory for uniform!\n", engine->ctx_data.uniforms->next_id);
        return AM_PA_INVALID_ID;
    };

    uniform->location = 0xFFFFFFFF;
    uniform->id = engine->ctx_data.uniforms->next_id;

    if (!info.type) {
        printf("[WARN] amgl_uniform_create (id: %u): No type given, choosing default!\n", uniform->id);
        info.type = AMGL_UNIFORM_TYPE_FLOAT;
    };

    uniform->type = info.type;
    if (!strlen(info.name)) {
        snprintf(info.name, AM_MAX_NAME_LENGTH, "%s%d", AMGL_UNIFORM_DEFAULT_NAME, uniform->id);
        printf("[WARN] amgl_uniform_create (id: %u): Choosing default name!\n", uniform->id);
    };
    snprintf(uniform->name, AM_MAX_NAME_LENGTH, "%s", info.name);
    am_int32 ret_id = am_packed_array_add(engine->ctx_data.uniforms, *uniform);
    am_free(uniform);
    return ret_id;
};

void amgl_uniform_destroy(am_id id) {
    am_engine *engine = am_engine_get_instance();
    am_packed_array_erase(engine->ctx_data.uniforms, id);
};

am_id amgl_texture_create(amgl_texture_info info) {
    am_engine *engine = am_engine_get_instance();

    amgl_texture *texture = (amgl_texture*)am_malloc(sizeof(amgl_texture));
    if (texture == NULL) {
        printf("[FAIL] amgl_texture_create (id: %u): Could not allocate memory for texture!\n", engine->ctx_data.textures->next_id);
        return AM_PA_INVALID_ID;
    };

    am_id ret_id = am_packed_array_add(engine->ctx_data.textures, *texture);
    am_free(texture);
    texture = am_packed_array_get_ptr(engine->ctx_data.textures, ret_id);
    texture->id = ret_id;

    if (!info.path) {
        printf("[WARN] amgl_texture_create: No image path given, assuming info.data is already filled!\n");
    } else {
        if (access(info.path, F_OK)) {
            printf("[FAIL] amgl_texture_create (id: %u): Path given (%s) does not exist!\n", engine->ctx_data.textures->next_id, info.path);
            return AM_PA_INVALID_ID;
        };
        amgl_texture_load_from_file(info.path, &info, true);
    };
    if (!info.wrap_s) {
        info.wrap_s = AMGL_TEXTURE_DEFAULT_WRAP;
        printf("[WARN] amgl_texture_create (id: %u): Choosing default wrap_s value!\n", ret_id);
    };
    if (!info.wrap_t) {
        info.wrap_t = AMGL_TEXTURE_DEFAULT_WRAP;
        printf("[WARN] amgl_texture_create (id: %u): Choosing default wrap_t value!\n", ret_id);
    };
    if (!info.height) {
        info.height = AMGL_TEXTURE_DEFAULT_HEIGHT;
        printf("[WARN] amgl_texture_create (id: %u): Choosing default height value!\n", ret_id);
    };
    if (!info.width) {
        info.width = AMGL_TEXTURE_DEFAULT_WIDTH;
        printf("[WARN] amgl_texture_create (id: %u): Choosing default width value!\n", ret_id);
    };
    if (!info.format) {
        info.format = AMGL_TEXTURE_DEFAULT_FORMAT;
        printf("[WARN] amgl_texture_create (id: %u): Choosing default format value!\n", ret_id);
    };

    glGenTextures(1, &texture->handle);
    glBindTexture(GL_TEXTURE_2D, texture->handle);

    switch (info.format) {
        case AMGL_TEXTURE_FORMAT_A8: glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, (am_int32)info.width, (am_int32)info.height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, info.data); break;
        case AMGL_TEXTURE_FORMAT_R8: glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, (am_int32)info.width, (am_int32)info.height, 0, GL_RED, GL_UNSIGNED_BYTE, info.data); break;
        case AMGL_TEXTURE_FORMAT_RGB8: glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, (am_int32)info.width, (am_int32)info.height, 0, GL_RGB8, GL_UNSIGNED_BYTE, info.data); break;
        case AMGL_TEXTURE_FORMAT_RGBA8: glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (am_int32)info.width, (am_int32)info.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, info.data); break;
        case AMGL_TEXTURE_FORMAT_RGBA16F: glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, (am_int32)info.width, (am_int32)info.height, 0, GL_RGBA, GL_FLOAT, info.data); break;
        case AMGL_TEXTURE_FORMAT_RGBA32F: glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, (am_int32)info.width, (am_int32)info.height, 0, GL_RGBA, GL_FLOAT, info.data); break;
        case AMGL_TEXTURE_FORMAT_RGBA: glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (am_int32)info.width, (am_int32)info.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, info.data); break;
        case AMGL_TEXTURE_FORMAT_DEPTH8: glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, (am_int32)info.width, (am_int32)info.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, info.data); break;
        case AMGL_TEXTURE_FORMAT_DEPTH16: glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, (am_int32)info.width, (am_int32)info.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, info.data); break;
        case AMGL_TEXTURE_FORMAT_DEPTH24: glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, (am_int32)info.width, (am_int32)info.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, info.data); break;
        case AMGL_TEXTURE_FORMAT_DEPTH32F: glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, (am_int32)info.width, (am_int32)info.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, info.data); break;
        case AMGL_TEXTURE_FORMAT_DEPTH24_STENCIL8: glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, (am_int32)info.width, (am_int32)info.height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, info.data); break;
        case AMGL_TEXTURE_FORMAT_DEPTH32F_STENCIL8: glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH32F_STENCIL8, (am_int32)info.width, (am_int32)info.height, 0, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, info.data); break;
        default: {
            printf("[FAIL] amgl_texture_create (id: %u): Invalid texture format!\n", ret_id);
            am_packed_array_erase(engine->ctx_data.textures, ret_id);
            return AM_PA_INVALID_ID;
        };
    };
    texture->format = info.format;

    am_int32 min_filter = info.min_filter == AMGL_TEXTURE_FILTER_NEAREST ? GL_NEAREST:GL_LINEAR;
    am_int32 mag_filter = info.mag_filter == AMGL_TEXTURE_FILTER_NEAREST ? GL_NEAREST:GL_LINEAR;

    if (info.mip_num) {
        if (info.min_filter == AMGL_TEXTURE_FILTER_NEAREST)
            min_filter = info.mip_filter == AMGL_TEXTURE_FILTER_NEAREST ? GL_NEAREST_MIPMAP_NEAREST:GL_NEAREST_MIPMAP_LINEAR;
        else
            min_filter = info.mip_filter == AMGL_TEXTURE_FILTER_NEAREST ? GL_LINEAR_MIPMAP_NEAREST:GL_LINEAR_MIPMAP_LINEAR;

        glGenerateMipmap(GL_TEXTURE_2D);
    };

    am_int32 wrap_s = amgl_texture_translate_wrap(info.wrap_s);
    am_int32 wrap_t = amgl_texture_translate_wrap(info.wrap_t);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (!strlen(info.name)) {
        snprintf(info.name, AM_MAX_NAME_LENGTH, "%s%d", AMGL_TEXTURE_DEFAULT_NAME, texture->id);
        printf("[WARN] amgl_texture_create (id: %u): Choosing default name!\n", texture->id);
    };
    snprintf(texture->name, AM_MAX_NAME_LENGTH, "%s", info.name);

    if (info.save_reference) {
        printf("[OK] amgl_texture_create (id: %u): Saving reference in memory!\n", texture->id);
        texture->refference.image = info.data;
        texture->refference.is_saved = true;
        texture->refference.width = info.width;
        texture->refference.height = info.height;
    } else {
        am_free(info.data);
    };
    return ret_id;
};

am_int32 amgl_texture_translate_format(amgl_texture_format format) {
    switch (format) {
        case AMGL_TEXTURE_FORMAT_RGBA8: return GL_RGBA8;
        case AMGL_TEXTURE_FORMAT_RGB8: return  GL_RGB8;
        case AMGL_TEXTURE_FORMAT_RGBA16F: return GL_RGBA16F;
        case AMGL_TEXTURE_FORMAT_RGBA32F: return  GL_RGBA32F;
        case AMGL_TEXTURE_FORMAT_RGBA: return GL_RGBA;
        case AMGL_TEXTURE_FORMAT_A8: return GL_ALPHA;
        case AMGL_TEXTURE_FORMAT_R8: return GL_RED;
        case AMGL_TEXTURE_FORMAT_DEPTH8: return GL_DEPTH_COMPONENT;
        case AMGL_TEXTURE_FORMAT_DEPTH16: return GL_DEPTH_COMPONENT16;
        case AMGL_TEXTURE_FORMAT_DEPTH24: return  GL_DEPTH_COMPONENT24;
        case AMGL_TEXTURE_FORMAT_DEPTH32F: return  GL_DEPTH_COMPONENT32F;
        case AMGL_TEXTURE_FORMAT_DEPTH24_STENCIL8: return GL_DEPTH24_STENCIL8;
        case AMGL_TEXTURE_FORMAT_DEPTH32F_STENCIL8: return GL_DEPTH32F_STENCIL8;
        default: {
            printf("[WARN] amgl_texture_translate_format (id: ?): Unknown texture format!\n");
            return GL_UNSIGNED_BYTE;
        };
    };
};

am_int32 amgl_texture_translate_wrap(amgl_texture_wrap wrap) {
    switch (wrap) {
        case AMGL_TEXTURE_WRAP_REPEAT: return GL_REPEAT;
        case AMGL_TEXTURE_WRAP_MIRRORED_REPEAT: return GL_MIRRORED_REPEAT;
        case AMGL_TEXTURE_WRAP_CLAMP_TO_EDGE: return GL_CLAMP_TO_EDGE;
        case AMGL_TEXTURE_WRAP_CLAMP_TO_BORDER: return GL_CLAMP_TO_BORDER;
        default: {
            printf("[WARN] amgl_texture_translate_wrap (id: ?): Unknown texture wrap!\n");
            return GL_REPEAT;
        };
    };
};

void amgl_texture_load_from_file(const char *path, amgl_texture_info *info, am_bool flip) {
    FILE *file = fopen(path, "rb");
    char* buffer = NULL;
    size_t rd_size = 0;

    if (file) {
#if defined(AM_LINUX)
        struct stat st;
        stat(path, &st);
        rd_size = st.st_size;
#else
        HANDLE file_hwnd = CreateFile(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        LARGE_INTEGER size;
        GetFileSizeEx(file_hwnd, &size);
        rd_size = (am_int32) size.QuadPart;
        CloseHandle(file_hwnd);
#endif
        buffer = (char*)am_malloc(rd_size+1);
        if (buffer) fread(buffer, 1, rd_size, file);
        buffer[rd_size] = '\0';
    };
    fclose(file);
    amgl_texture_load_from_memory(buffer, info, rd_size, flip);
};

void amgl_texture_load_from_memory(const void *memory, amgl_texture_info *info, size_t size, am_bool flip) {
    am_int32 num_comps = 0;
    stbi_set_flip_vertically_on_load(flip);
    info->data = NULL;
    info->data = (void*)stbi_load_from_memory((const stbi_uc*)memory, (am_int32)size, (am_int32*)&info->width, (am_int32*)&info->height, &num_comps, 4);
    am_free(memory);
    if (info->data == NULL) {
        printf("[FAIL] amgl_texture_load_from_memory (id: ?): Unable to load texture!\n");
        return;
    };
};

void amgl_texture_destroy(am_id id) {
    am_engine *engine = am_engine_get_instance();
    amgl_texture *texture = am_packed_array_get_ptr(engine->ctx_data.textures, id);
    if (!texture) {
        printf("[FAIL] amgl_texture_destroy (id: %u): Invalid texture id!\n", id);
        return;
    }
    glDeleteTextures(1, &texture->handle);
    if (texture->refference.is_saved) am_free(texture->refference.image);
    am_packed_array_erase(engine->ctx_data.textures, id);
};


am_id amgl_frame_buffer_create(amgl_frame_buffer_info info) {
    am_engine *engine = am_engine_get_instance();

    amgl_frame_buffer *framebuffer = (amgl_frame_buffer*)am_malloc(sizeof(amgl_frame_buffer));
    if (framebuffer == NULL) {
        printf("[FAIL] amgl_frame_buffer_create (id: %u): Could not allocate memory for framebuffer!\n", engine->ctx_data.frame_buffers->next_id);
        return AM_PA_INVALID_ID;
    };

    glGenFramebuffers(1, &framebuffer->handle);
    framebuffer->id = engine->ctx_data.frame_buffers->next_id;
    if (!strlen(info.name)) {
        snprintf(info.name, AM_MAX_NAME_LENGTH, "%s%d", AMGL_FRAME_BUFFER_DEFAULT_NAME, framebuffer->id);
        printf("[WARN] amgl_frame_buffer_create (id: %u): Choosing default name!\n", framebuffer->id);
    };
    snprintf(framebuffer->name, AM_MAX_NAME_LENGTH, "%s", info.name);
    am_id ret_id = am_packed_array_add(engine->ctx_data.frame_buffers, *framebuffer);
    am_free(framebuffer);
    return ret_id;
};

void amgl_frame_buffer_destroy(am_id id) {
    am_engine *engine = am_engine_get_instance();
    amgl_frame_buffer *framebuffer = am_packed_array_get_ptr(engine->ctx_data.frame_buffers, id);
    if (!framebuffer) {
        printf("[FAIL] amgl_frame_buffer_destroy (id: %u): Invalid frame buffer id!\n", framebuffer->id);
        return;
    };
    glDeleteFramebuffers(1, &framebuffer->handle);
    am_packed_array_erase(engine->ctx_data.frame_buffers, id);
};

am_id amgl_render_pass_create(amgl_render_pass_info info) {
    am_engine *engine = am_engine_get_instance();

    amgl_render_pass *render_pass = (amgl_render_pass*)am_malloc(sizeof(amgl_render_pass));
    if (render_pass == NULL) {
        printf("[FAIL] amgl_render_pass_create (id: %u): Could not allocate memory for render pass!\n", engine->ctx_data.render_passes->next_id);
        return AM_PA_INVALID_ID;
    };

    am_id ret_id = am_packed_array_add(engine->ctx_data.render_passes, *render_pass);
    am_free(render_pass);
    render_pass = am_packed_array_get_ptr(engine->ctx_data.render_passes, ret_id);
    render_pass->id = ret_id;

    if (!info.depth_texture_id) printf("[WARN] amgl_render_pass_create (id: %u): No depth texture specified!\n", ret_id);
    render_pass->depth_texture_id = info.depth_texture_id;
    if (!info.framebuffer_id) {
        printf("[WARN] amgl_render_pass_create (id: %u): No framebuffer id specified, choosing id 1!\n", ret_id);
        render_pass->framebuffer_id = 1;
    } else {
        render_pass->framebuffer_id = info.framebuffer_id;
    };

    if (!info.stencil_texture_id) printf("[WARN] amgl_render_pass_create (id: %u): No stencil texture id specified!\n", ret_id);
    render_pass->stencil_texture_id = info.stencil_texture_id;
    if ((!info.num_colors) || info.color_texture_ids == NULL) printf("[WARN] amgl_render_pass_create (id: %u): No color attachments passed!\n", ret_id);
    render_pass->num_colors = info.num_colors;
    render_pass->color_texture_ids = am_malloc(sizeof(am_id)*info.num_colors);
    memcpy(render_pass->color_texture_ids, info.color_texture_ids, sizeof(am_id)*info.num_colors);


    if (!strlen(info.name)) {
        snprintf(info.name, AM_MAX_NAME_LENGTH, "%s%d", AMGL_RENDER_PASS_DEFAULT_NAME, render_pass->id);
        printf("[WARN] amgl_render_pass_create (id: %u): Choosing default name!\n", render_pass->id);
    };
    snprintf(render_pass->name, AM_MAX_NAME_LENGTH, "%s", info.name);
    return ret_id;
};

void amgl_render_pass_destroy(am_id id) {
    am_engine *engine = am_engine_get_instance();
    am_packed_array_erase(engine->ctx_data.render_passes, id);
};

am_id amgl_pipeline_create(amgl_pipeline_info info) {
    am_engine *engine = am_engine_get_instance();

    amgl_pipeline *pipeline = (amgl_pipeline*)am_malloc(sizeof(amgl_pipeline));
    if (pipeline == NULL) {
        printf("[FAIL] amgl_pipeline_create (id: %u): Could not allocate memory for pipeline!\n", engine->ctx_data.pipelines->next_id);
        return AM_PA_INVALID_ID;
    };

    if (info.blend.func) {
        if (!info.blend.src) {
            printf("[WARN] amgl_pipeline_create (id: %u): Blend func is defined, src is not!\n", engine->ctx_data.pipelines->next_id);
            info.blend.src = AMGL_BLEND_MODE_ONE;
        };
        if (!info.blend.dst) {
            printf("[WARN] amgl_pipeline_create (id: %u): Blend func is defined, dst is not!\n", engine->ctx_data.pipelines->next_id);
            info.blend.dst = AMGL_BLEND_MODE_ZERO;
        };
    };
    pipeline->blend = info.blend;

    pipeline->depth = info.depth;

    if (info.stencil.func) {
        if (!info.stencil.dppass) {
            printf("[WARN] amgl_pipeline_create (id: %u): Stencil func is defined, dppass is not. Choosing default!\n", engine->ctx_data.pipelines->next_id);
            info.stencil.dppass = AMGL_STENCIL_OP_KEEP;
        };
        if (!info.stencil.dpfail) {
            printf("[WARN] amgl_pipeline_create (id: %u): Stencil func is defined, dpfail is not. Choosing default!\n", engine->ctx_data.pipelines->next_id);
            info.stencil.dpfail = AMGL_STENCIL_OP_KEEP;
        };
        if (!info.stencil.sfail) {
            printf("[WARN] amgl_pipeline_create (id: %u): Stencil func is defined, sfail is not. Choosing default!\n", engine->ctx_data.pipelines->next_id);
            info.stencil.sfail = AMGL_STENCIL_OP_KEEP;
        };
    };
    pipeline->stencil = info.stencil;
    if (!info.raster.winding_order) {
        printf("[WARN] amgl_pipeline_create (id: %u): Winding order not defined, choosing default!\n", engine->ctx_data.pipelines->next_id);
        info.raster.winding_order = AMGL_WINDING_ORDER_CCW;
    };
    if (!info.raster.primitive) {
        printf("[WARN] amgl_pipeline_create (id: %u): Primitive not defined, choosing default!\n", engine->ctx_data.pipelines->next_id);
        info.raster.primitive = AMGL_PRIMITIVE_TRIANGLES;
    };
    pipeline->raster = info.raster;

    pipeline->layout.attributes = am_malloc(info.layout.num_attribs * sizeof(amgl_vertex_buffer_attribute));
    memcpy(pipeline->layout.attributes, info.layout.attributes, info.layout.num_attribs * sizeof(amgl_vertex_buffer_attribute));
    pipeline->layout.num_attribs = info.layout.num_attribs;

    pipeline->compute = info.compute;

    pipeline->id = engine->ctx_data.pipelines->next_id;
    if (!strlen(info.name)) {
        snprintf(info.name, AM_MAX_NAME_LENGTH, "%s%d", AMGL_PIPELINE_DEFAULT_NAME, pipeline->id);
        printf("[WARN] amgl_pipeline_create (id: %u): Choosing default name!\n", pipeline->id);
    };
    snprintf(pipeline->name, AM_MAX_NAME_LENGTH, "%s", info.name);
    am_int32 ret_id = am_packed_array_add(engine->ctx_data.pipelines, *pipeline);
    am_free(pipeline);
    return ret_id;
};

am_int32 amgl_blend_translate_func(amgl_blend_func func) {
    switch(func) {
        case AMGL_BLEND_FUNC_ADD: return GL_FUNC_ADD;
        case AMGL_BLEND_FUNC_SUBSTRACT: return GL_FUNC_SUBTRACT;
        case AMGL_BLEND_FUNC_REVERSE_SUBSTRACT: return GL_FUNC_REVERSE_SUBTRACT;
        case AMGL_BLEND_FUNC_MIN: return GL_MIN;
        case AMGL_BLEND_FUNC_MAX: return GL_MAX;
        default: {
            printf("[WARN] amgl_blend_translate_func (id: ?): Unknown blend func!\n");
            return GL_FUNC_ADD;
        };
    };
};

am_int32 amgl_blend_translate_mode(amgl_blend_mode mode) {
    switch (mode) {
        case AMGL_BLEND_MODE_ZERO: return GL_ZERO;
        case AMGL_BLEND_MODE_ONE: return GL_ONE;
        case AMGL_BLEND_MODE_SRC_COLOR: return GL_SRC_COLOR;
        case AMGL_BLEND_MODE_ONE_MINUS_SRC_COLOR: return GL_ONE_MINUS_SRC_COLOR;
        case AMGL_BLEND_MODE_DST_COLOR: return GL_DST_COLOR;
        case AMGL_BLEND_MODE_ONE_MINUS_DST_COLOR: return GL_ONE_MINUS_DST_COLOR;
        case AMGL_BLEND_MODE_SRC_ALPHA: return GL_SRC_ALPHA;
        case AMGL_BLEND_MODE_ONE_MINUS_SRC_ALPHA: return GL_ONE_MINUS_SRC_ALPHA;
        case AMGL_BLEND_MODE_DST_ALPHA: return GL_DST_ALPHA;
        case AMGL_BLEND_MODE_ONE_MINUS_DST_ALPHA: return GL_ONE_MINUS_DST_ALPHA;
        case AMGL_BLEND_MODE_CONSTANT_COLOR: return GL_CONSTANT_COLOR;
        case AMGL_BLEND_MODE_ONE_MINUS_CONSTANT_COLOR: return GL_ONE_MINUS_CONSTANT_COLOR;
        case AMGL_BLEND_MODE_CONSTANT_ALPHA: return GL_CONSTANT_ALPHA;
        case AMGL_BLEND_MODE_ONE_MINUS_CONSTANT_ALPHA: return GL_ONE_MINUS_CONSTANT_ALPHA;
        default: {
            printf("[WARN] amgl_blend_translate_mode (id: ?): Unknown blend mode!\n");
            return GL_ZERO;
        };
    };
};

am_int32 amgl_depth_translate_func(amgl_depth_func func) {
    switch (func) {
        case AMGL_DEPTH_FUNC_LESS: return GL_LESS;
        case AMGL_DEPTH_FUNC_NEVER: return GL_NEVER;
        case AMGL_DEPTH_FUNC_EQUAL: return GL_EQUAL;
        case AMGL_DEPTH_FUNC_LEQUAL: return GL_LEQUAL;
        case AMGL_DEPTH_FUNC_GREATER: return GL_GREATER;
        case AMGL_DEPTH_FUNC_NOTEQUAL: return GL_NOTEQUAL;
        case AMGL_DEPTH_FUNC_GEQUAL: return GL_GEQUAL;
        case AMGL_DEPTH_FUNC_ALWAYS: return GL_ALWAYS;
        default: {
            printf("[WARN] amgl_depth_translate_func (id: ?): Unknown depth func!\n");
            return GL_LESS;
        };
    };
};

am_int32 amgl_stencil_translate_func(amgl_stencil_func func) {
    switch (func) {
        case AMGL_STENCIL_FUNC_LESS: return GL_LESS;
        case AMGL_STENCIL_FUNC_NEVER: return GL_NEVER;
        case AMGL_STENCIL_FUNC_EQUAL: return GL_EQUAL;
        case AMGL_STENCIL_FUNC_LEQUAL: return GL_LEQUAL;
        case AMGL_STENCIL_FUNC_GREATER: return GL_GREATER;
        case AMGL_STENCIL_FUNC_NOTEQUAL: return GL_NOTEQUAL;
        case AMGL_STENCIL_FUNC_GEQUAL: return GL_GEQUAL;
        case AMGL_STENCIL_FUNC_ALWAYS: return GL_ALWAYS;
        default: {
            printf("[WARN] amgl_stencil_translate_func (id: ?): Unknown stencil func!\n");
            return GL_ALWAYS;
        };
    };
};

am_int32 amgl_stencil_translate_op(amgl_stencil_op op) {
    switch (op) {
        case AMGL_STENCIL_OP_KEEP: return GL_KEEP;
        case AMGL_STENCIL_OP_ZERO: return GL_ZERO;
        case AMGL_STENCIL_OP_REPLACE: return GL_REPLACE;
        case AMGL_STENCIL_OP_INCR: return GL_INCR;
        case AMGL_STENCIL_OP_INCR_WRAP: return GL_INCR_WRAP;
        case AMGL_STENCIL_OP_DECR: return GL_DECR;
        case AMGL_STENCIL_OP_DECR_WRAP: return GL_DECR_WRAP;
        case AMGL_STENCIL_OP_INVERT: return GL_INVERT;
        default: {
            printf("[WARN] amgl_stencil_translate_op (id: ?): Unknown stencil op!\n");
            return GL_KEEP;
        };
    };
};

am_int32 amgl_face_cull_translate(amgl_face_cull face_cull) {
    switch (face_cull) {
        case AMGL_FACE_CULL_BACK: return GL_BACK;
        case AMGL_FACE_CULL_FRONT: return GL_FRONT;
        case AMGL_FACE_CULL_FRONT_AND_BACK: return GL_FRONT_AND_BACK;
        default: {
            printf("[WARN] amgl_face_cull_translate (id: ?): Unknown face culling type!\n");
            return GL_BACK;
        };
    };
};

am_int32 amgl_winding_order_translate(amgl_winding_order winding_order) {
    switch (winding_order) {
        case AMGL_WINDING_ORDER_CCW: return GL_CCW;
        case AMGL_WINDING_ORDER_CW: return GL_CW;
        default: {
            printf("[WARN] amgl_winding_order_translate (id: ?): Unknown winding_order type!\n");
            return GL_CCW;
        };
    };
};

am_int32 amgl_primitive_translate(amgl_primitive prim) {
    switch (prim) {
        case AMGL_PRIMITIVE_TRIANGLES: return GL_TRIANGLES;
        case AMGL_PRIMITIVE_LINES: return GL_LINES;
        case AMGL_PRIMITIVE_QUADS: return GL_QUADS;
        default: {
            printf("[WARN] amgl_primitive_translate (id: ?): Unknown primitive type!\n");
            return GL_TRIANGLES;
        };
    };
};

am_int32 amgl_index_buffer_size_translate(size_t size) {
    switch (size) {
        case 4: return GL_UNSIGNED_INT;
        case 2: return GL_UNSIGNED_SHORT;
        case 1: return GL_UNSIGNED_BYTE;
        default: {
            printf("[WARN] index_buffer_size (id: ?): Unknown size!\n");
            return GL_UNSIGNED_INT;
        };
    };
};

void amgl_pipeline_destroy(am_id id) {
    am_engine *engine = am_engine_get_instance();
    am_packed_array_erase(engine->ctx_data.pipelines, id);
};



void amgl_clear(amgl_clear_desc info) {
    for (am_int32 i = 0; i < info.num; i++) {
        switch (info.types[i]) {
            case AMGL_CLEAR_COLOR: {
                glClearColor(info.r, info.g, info.b, info.a);
                glClear(GL_COLOR_BUFFER_BIT);
                break;
            };
            case AMGL_CLEAR_DEPTH: {
                glClear(GL_DEPTH_BUFFER_BIT);
                break;
            };
            case AMGL_CLEAR_STENCIL: {
                glClear(GL_STENCIL_BUFFER_BIT);
                break;
            };
            case AMGL_CLEAR_ALL: {
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
                break;
            };
            default: break;
        }
    }
};

void amgl_terminate() {

};

void amgl_set_viewport(am_int32 x, am_int32 y, am_int32 width, am_int32 height) {
    glViewport(x, y, width, height);
};

void amgl_vsync(am_id window_id, am_bool state) {
    glFlush();
#if defined(AM_LINUX)
    am_platform *platform = am_engine_get_subsystem(platform);
    am_window *window = am_packed_array_get_ptr(platform->windows, window_id);
    if (glSwapInterval == NULL) glSwapInterval = (PFNGLSWAPINTERVALEXTPROC)amgl_get_proc_address("glXSwapIntervalEXT");
    glSwapInterval(platform->display, window->handle, state);
#else
    if (glSwapInterval == NULL) glSwapInterval = (PFNGLSWAPINTERVALEXTPROC)amgl_get_proc_address("wglSwapIntervalEXT");
    glSwapInterval(state == true ? 1:0);
#endif
};

void amgl_start_render_pass(am_id render_pass_id) {
    am_engine *engine = am_engine_get_instance();
    if (am_packed_array_has(engine->ctx_data.render_passes, render_pass_id)) {
        amgl_render_pass *render_pass = am_packed_array_get_ptr(engine->ctx_data.render_passes, render_pass_id);
        if (am_packed_array_has(engine->ctx_data.frame_buffers, render_pass->framebuffer_id)) {
            glBindFramebuffer(GL_FRAMEBUFFER, am_packed_array_get_val(engine->ctx_data.frame_buffers, render_pass->framebuffer_id).handle);
            for (am_int32 i = 0; i < render_pass->num_colors; i++) {
                if (am_packed_array_has(engine->ctx_data.textures, render_pass->color_texture_ids[i])) {
                    amgl_texture *color = am_packed_array_get_ptr(engine->ctx_data.textures, render_pass->color_texture_ids[i]);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, color->handle, 0);
                };
            };
        };
    };
};

void amgl_end_render_pass(am_id render_pass_id) {
    am_engine* engine = am_engine_get_instance();

    engine->ctx_data.frame_cache.index_buffer = (amgl_index_buffer){.id = AM_PA_INVALID_ID};
    engine->ctx_data.frame_cache.index_element_size = 0;
    engine->ctx_data.frame_cache.pipeline = (amgl_pipeline){.id = AM_PA_INVALID_ID};
    am_dyn_array_clear(engine->ctx_data.frame_cache.vertex_buffers);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //glDisable(GL_SCISSOR_TEST); //Read more on this
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
};

void amgl_bind_pipeline(am_id pipeline_id) {
    am_engine* engine = am_engine_get_instance();
    if (am_packed_array_has(engine->ctx_data.pipelines, pipeline_id)) {

        engine->ctx_data.frame_cache.index_buffer = (amgl_index_buffer){.id = AM_PA_INVALID_ID};
        engine->ctx_data.frame_cache.index_element_size = 0;
        engine->ctx_data.frame_cache.pipeline = (amgl_pipeline){.id = AM_PA_INVALID_ID};
        am_dyn_array_clear(engine->ctx_data.frame_cache.vertex_buffers);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);


        amgl_pipeline *pipeline = am_packed_array_get_ptr(engine->ctx_data.pipelines, pipeline_id);
        engine->ctx_data.frame_cache.pipeline = *pipeline;

        if (pipeline->compute.compute_shader) glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

        if (pipeline->compute.compute_shader) {
            if (pipeline->compute.compute_shader && am_packed_array_has(engine->ctx_data.shaders, pipeline->compute.compute_shader)) {
                glUseProgram(am_packed_array_get_ptr(engine->ctx_data.shaders, pipeline->compute.compute_shader)->handle);
            };
            return;
        };

        if (!pipeline->depth.func) {
            glDisable(GL_DEPTH_TEST);
        } else {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(amgl_depth_translate_func(pipeline->depth.func));
        };

        if (!pipeline->stencil.func) {
            glDisable(GL_STENCIL_TEST);
        } else {
            glEnable(GL_STENCIL_TEST);
            am_uint32 func = amgl_stencil_translate_func(pipeline->stencil.func);
            am_uint32 sfail = amgl_stencil_translate_op(pipeline->stencil.sfail);
            am_uint32 dpfail = amgl_stencil_translate_op(pipeline->stencil.dpfail);
            am_uint32 dppass = amgl_stencil_translate_op(pipeline->stencil.dppass);
            glStencilFunc(func, pipeline->stencil.ref, pipeline->stencil.comp_mask);
            glStencilMask(pipeline->stencil.write_mask);
            glStencilOp(sfail, dpfail, dppass);
        };

        if (!pipeline->blend.func) {
            glDisable(GL_BLEND);
        } else {
            glEnable(GL_BLEND);
            glBlendEquation(amgl_blend_translate_func(pipeline->blend.func));
            glBlendFunc(amgl_blend_translate_mode(pipeline->blend.src), amgl_blend_translate_mode(pipeline->blend.dst));
        };

        if (!pipeline->raster.face_culling) {
            glDisable(GL_CULL_FACE);
        } else {
            glEnable(GL_CULL_FACE);
            glCullFace(amgl_face_cull_translate(pipeline->raster.face_culling));
        };

        glFrontFace(amgl_winding_order_translate(pipeline->raster.winding_order));

        if (am_packed_array_has(engine->ctx_data.shaders, pipeline->raster.shader_id)){
            glUseProgram(am_packed_array_get_ptr(engine->ctx_data.shaders, pipeline->raster.shader_id)->handle);
        };
    };
};

void amgl_set_bindings(amgl_bindings_info *info) {
    am_engine *engine = am_engine_get_instance();
    am_uint32 vertex_count = info->vertex_buffers.info ? info->vertex_buffers.size ? info->vertex_buffers.size / sizeof(amgl_vertex_buffer_bind_info) : 1 : 0;
    am_uint32 index_count = info->index_buffers.info ? info->index_buffers.size ? info->index_buffers.size / sizeof(amgl_index_buffer_bind_info) : 1 : 0;
    am_uint32 storage_count = info->storage_buffers.info ? info->storage_buffers.size ? info->storage_buffers.size / sizeof(amgl_storage_buffer_bind_info) : 1 : 0;
    am_uint32 uniform_count = info->uniforms.info ? info->uniforms.size ? info->uniforms.size / sizeof(amgl_uniform_bind_info) : 1 : 0;
    am_uint32 texture_count = info->images.info ? info->images.size ? info->images.size / sizeof(amgl_texture_bind_info) : 1 : 0;

    if (vertex_count) {
        am_dyn_array_clear(engine->ctx_data.frame_cache.vertex_buffers);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    };

    for (am_uint32 i = 0; i < vertex_count; i++) {
        am_id id = info->vertex_buffers.info[i].vertex_buffer_id;
        if (!am_packed_array_has(engine->ctx_data.vertex_buffers, id)) {
            printf("[FAIL] amgl_set_bindings (id: %u): Vertex buffer could not be found!\n", id);
            break;
        };
        amgl_vertex_buffer vertex_buffer = am_packed_array_get_val(engine->ctx_data.vertex_buffers, id);
        am_dyn_array_push(engine->ctx_data.frame_cache.vertex_buffers, vertex_buffer);
    };

    for (am_uint32 i = 0; i < index_count; i++) {
        am_id id = info->index_buffers.info[i].index_buffer_id;
        if (!am_packed_array_has(engine->ctx_data.index_buffers, id)) {
            printf("[FAIL] amgl_set_bindings (id: %u): Index buffer could not be found!\n", id);
            break;
        };
        amgl_index_buffer index_buffer = am_packed_array_get_val(engine->ctx_data.index_buffers, id);
        engine->ctx_data.frame_cache.index_buffer = index_buffer;
    };

    for (am_uint32 i = 0; i < uniform_count; i++) {
        am_id id = info->uniforms.info[i].uniform_id;
        size_t size = am_packed_array_get_ptr(engine->ctx_data.uniforms, id)->size;
        am_uint32 binding = info->uniforms.info[i].binding;

        if (!am_packed_array_has(engine->ctx_data.uniforms, id)) {
            printf("[FAIL] amgl_set_bindings (id: %u): Uniform could not be found!\n", id);
            break;
        };

        if (!am_packed_array_has(engine->ctx_data.pipelines, engine->ctx_data.frame_cache.pipeline.id)) {
            printf("[FAIL] amgl_set_bindings (id: (1) %u, (2) %u): Cannot bind uniform (1) since pipeline (2) could not be found!\n", id, engine->ctx_data.frame_cache.pipeline.id);
            break;
        }
        amgl_pipeline *pipeline = am_packed_array_get_ptr(engine->ctx_data.pipelines, engine->ctx_data.frame_cache.pipeline.id);
        amgl_uniform *uniform = am_packed_array_get_ptr(engine->ctx_data.uniforms, id);

        am_id shader_id = pipeline->compute.compute_shader ? pipeline->compute.compute_shader : pipeline->raster.shader_id;

        if (uniform->location == 0xFFFFFFFF || uniform->shader_id != pipeline->raster.shader_id) {
            if (!am_packed_array_has(engine->ctx_data.shaders, shader_id)) {
                printf("[FAIL] amgl_set_bindings (id: (1) %u, (2) %u): Cannot bind uniform (1) since shader (2) could not be found!\n", id, shader_id);
                break;
            };
            amgl_shader shader = am_packed_array_get_val(engine->ctx_data.shaders, shader_id);
            uniform->location = glGetUniformLocation(shader.handle, strlen(uniform->name) ? uniform->name : "AM_UNIFORM_EMPTY_NAME");
            if (uniform->location >= 0xFFFFFFFF) {
                printf("[FAIL] amgl_set_bindings (id: %u): Uniform not found by name: %s!\n", uniform->id, uniform->name);
                break;
            };
        }

        uniform->shader_id = shader_id;
        uniform->data = info->uniforms.info[i].data;

        switch (uniform->type) {
            case AMGL_UNIFORM_TYPE_FLOAT: {
                glUniform1f((am_int32)uniform->location, *((float*)uniform->data));
                break;
            };
            case AMGL_UNIFORM_TYPE_INT: {
                glUniform1i((am_int32)uniform->location, *((int*)uniform->data));
                break;
            };
            case AMGL_UNIFORM_TYPE_VEC2: {
                glUniform2f((am_int32)uniform->location, ((float*)(uniform->data))[0], ((float*)(uniform->data))[1]);
                break;
            };
            case AMGL_UNIFORM_TYPE_VEC3: {
                glUniform3f((am_int32)uniform->location, ((float*)(uniform->data))[0], ((float*)(uniform->data))[1], ((float*)(uniform->data))[2]);
                break;
            };
            case AMGL_UNIFORM_TYPE_VEC4: {
                glUniform4f((am_int32)uniform->location, ((float*)(uniform->data))[0], ((float*)(uniform->data))[1], ((float*)(uniform->data))[2], ((float*)(uniform->data))[3]);
                break;
            }
            case AMGL_UNIFORM_TYPE_MAT3: {
                glUniformMatrix3fv((am_int32)uniform->location, 1, false, (float*)(uniform->data));
                break;
            };
            case AMGL_UNIFORM_TYPE_MAT4: {
                glUniformMatrix4fv((am_int32)uniform->location, 1, false, (float*)(uniform->data));
                break;
            };
            case AMGL_UNIFORM_TYPE_SAMPLER2D: {
                amgl_texture *texture = am_packed_array_get_ptr(engine->ctx_data.textures, *((am_int32*)(uniform->data)));
                glActiveTexture(GL_TEXTURE0 + binding);
                glBindTexture(GL_TEXTURE_2D, texture->handle);
                glUniform1i((am_int32)uniform->location, (am_int32)binding);
                break;
            };
            default: {
                printf("[FAIL] amgl_set_bindings (id: %u): Invalid uniform type!\n", uniform->id);
                exit(1);
            };
        };
    };

    for (am_uint32 i = 0; i < storage_count; i++) {
        am_id id = info->storage_buffers.info[i].storage_buffer_id;
        am_int32 binding = info->storage_buffers.info[i].binding;
        if (!am_packed_array_has(engine->ctx_data.storage_buffers, id)) {
            printf("[FAIL] amgl_set_bindings (id: %u): Storage buffer could not be found!\n", id);
            break;
        };
        amgl_storage_buffer *storage_buffer = am_packed_array_get_ptr(engine->ctx_data.storage_buffers, id);
        if (!am_packed_array_has(engine->ctx_data.pipelines, engine->ctx_data.frame_cache.pipeline.id)) {
            printf("[FAIL] amgl_set_bindings (id: (1) %u, (2) %u): Cannot bind storage buffer (1) since pipeline (2) could not be found!\n", id, engine->ctx_data.frame_cache.pipeline.id);
            break;
        };
        amgl_pipeline *pipeline = am_packed_array_get_ptr(engine->ctx_data.pipelines, engine->ctx_data.frame_cache.pipeline.id);

        am_id shader_id = pipeline->compute.compute_shader ? pipeline->compute.compute_shader : pipeline->raster.shader_id;
        if (!shader_id || !am_packed_array_has(engine->ctx_data.shaders, shader_id)) {
            printf("[FAIL] amgl_set_bindings (id: (1) %u, (2) %u): Cannot bind storage buffer (1) since shader (2) could not be found!\n", id, shader_id);
            break;
        };

        amgl_shader *shader = am_packed_array_get_ptr(engine->ctx_data.shaders, shader_id);
        am_uint32 location = 0xFFFFFFFF;

        if (storage_buffer->id == AM_PA_INVALID_ID) {
            storage_buffer->block_index = glGetProgramResourceIndex(shader->handle, GL_SHADER_STORAGE_BLOCK, storage_buffer->name);
            am_int32 params[1];
            GLenum props[1] = {GL_BUFFER_BINDING};
            glGetProgramResourceiv(shader->handle, GL_SHADER_STORAGE_BLOCK, storage_buffer->block_index, 1, props, 1, NULL, params);
            location = (am_uint32)params[0];
        };

        if (storage_buffer->block_index < 0xFFFFFFFF - 1) glShaderStorageBlockBinding(shader->handle, storage_buffer->block_index, location);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, storage_buffer->handle);
    };

    for (am_uint32 i = 0; i < texture_count; i++) {
        am_id id = info->images.info[i].texture_id;
        am_uint32 binding = info->images.info[i].binding;

        if (!am_packed_array_has(engine->ctx_data.textures, id)) {
            printf("[FAIL] amgl_set_bindings (id: %u): Texture could not be found!\n", id);
            break;
        };
        amgl_texture *texture = am_packed_array_get_ptr(engine->ctx_data.textures, id);
        am_int32 format = amgl_texture_translate_format(texture->format);

        am_int32 access = 0;
        switch (info->images.info[i].access) {
            case AMGL_BUFFER_ACCESS_READ_ONLY: {
                access = GL_READ_ONLY;
                break;
            };
            case AMGL_BUFFER_ACCESS_WRITE_ONLY: {
                access = GL_WRITE_ONLY;
                break;
            };
            case AMGL_BUFFER_ACCESS_READ_WRITE: {
                access = GL_READ_WRITE;
                break;
            };
            default: break;
        };
        glBindImageTexture(binding, texture->handle, 0, GL_FALSE, 0, access, format);

    };
};

void amgl_draw(amgl_draw_info *info) {
    am_engine *engine = am_engine_get_instance();
    amgl_pipeline *pipeline = am_packed_array_get_ptr(engine->ctx_data.pipelines, engine->ctx_data.frame_cache.pipeline.id);

    if (am_dyn_array_get_count(engine->ctx_data.frame_cache.vertex_buffers) == 0) {
        printf("[FAIL] amgl_draw (id: ?): No vertex buffer bound at draw time!\n");
        exit(0);
    };

    for (am_uint32 i = 0; i < pipeline->layout.num_attribs; i++) {
        am_uint32 vertex_idx = pipeline->layout.attributes[i].buffer_index;
        amgl_vertex_buffer vertex_buffer = (vertex_idx < am_dyn_array_get_count(engine->ctx_data.frame_cache.vertex_buffers)) ? engine->ctx_data.frame_cache.vertex_buffers[vertex_idx] : engine->ctx_data.frame_cache.vertex_buffers[0];

        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer.handle);
        size_t stride = pipeline->layout.attributes[i].stride;
        size_t offset = pipeline->layout.attributes[i].offset;
        glEnableVertexAttribArray(i);

        if (!pipeline->layout.attributes[i].format) {
            printf("[WARN] amgl_draw (id: ?): No vertex attribute format given, choosing default!\n");
            pipeline->layout.attributes[i].format = AMGL_VERTEX_BUFFER_ATTRIBUTE_FLOAT;
        };

        switch (pipeline->layout.attributes[i].format) {
            case AMGL_VERTEX_BUFFER_ATTRIBUTE_FLOAT4: glVertexAttribPointer(i, 4, GL_FLOAT, GL_FALSE, (am_int32)stride, (void*)(uintptr_t)offset); break;
            case AMGL_VERTEX_BUFFER_ATTRIBUTE_FLOAT3: glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, (am_int32)stride, (void*)(uintptr_t)offset); break;
            case AMGL_VERTEX_BUFFER_ATTRIBUTE_FLOAT2: glVertexAttribPointer(i, 2, GL_FLOAT, GL_FALSE, (am_int32)stride, (void*)(uintptr_t)offset); break;
            case AMGL_VERTEX_BUFFER_ATTRIBUTE_FLOAT:  glVertexAttribPointer(i, 1, GL_FLOAT, GL_FALSE, (am_int32)stride, (void*)(uintptr_t)offset); break;
            case AMGL_VERTEX_BUFFER_ATTRIBUTE_UINT4:  glVertexAttribIPointer(i, 4, GL_UNSIGNED_INT, (am_int32)stride, (void*)(uintptr_t)offset); break;
            case AMGL_VERTEX_BUFFER_ATTRIBUTE_UINT3:  glVertexAttribIPointer(i, 3, GL_UNSIGNED_INT, (am_int32)stride, (void*)(uintptr_t)offset); break;
            case AMGL_VERTEX_BUFFER_ATTRIBUTE_UINT2:  glVertexAttribIPointer(i, 2, GL_UNSIGNED_INT, (am_int32)stride, (void*)(uintptr_t)offset); break;
            case AMGL_VERTEX_BUFFER_ATTRIBUTE_UINT:   glVertexAttribIPointer(i, 1, GL_UNSIGNED_INT, (am_int32)stride, (void*)(uintptr_t)offset); break;
            case AMGL_VERTEX_BUFFER_ATTRIBUTE_BYTE:   glVertexAttribPointer(i, 1, GL_UNSIGNED_BYTE, GL_TRUE, (am_int32)stride, (void*)(uintptr_t)offset); break;
            case AMGL_VERTEX_BUFFER_ATTRIBUTE_BYTE2:  glVertexAttribPointer(i, 2, GL_UNSIGNED_BYTE, GL_TRUE, (am_int32)stride, (void*)(uintptr_t)offset); break;
            case AMGL_VERTEX_BUFFER_ATTRIBUTE_BYTE3:  glVertexAttribPointer(i, 3, GL_UNSIGNED_BYTE, GL_TRUE, (am_int32)stride, (void*)(uintptr_t)offset); break;
            case AMGL_VERTEX_BUFFER_ATTRIBUTE_BYTE4:  glVertexAttribPointer(i, 4, GL_UNSIGNED_BYTE, GL_TRUE, (am_int32)stride, (void*)(uintptr_t)offset); break;
            default: {
                printf("[FAIL] amgl_draw (id: ?): Invalid layout attribute format!\n");
                exit(0);
            };
        };
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    };

    if (am_packed_array_has(engine->ctx_data.index_buffers, engine->ctx_data.frame_cache.index_buffer.id)) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, engine->ctx_data.frame_cache.index_buffer.handle);
    };

    am_uint32 primitive = amgl_primitive_translate(pipeline->raster.primitive);

    if (am_packed_array_has(engine->ctx_data.index_buffers, engine->ctx_data.frame_cache.index_buffer.id)) {
        glDrawElements(primitive, (am_int32)info->count, GL_UNSIGNED_INT, (void*)(intptr_t)info->start);
    } else {
        glDrawArrays(primitive, (am_int32)info->start, (am_int32)info->count);
    };

};

//Camera
am_camera am_camera_default() {
    am_camera cam = {0};
    cam.transform = am_vqs_default();
    cam.transform.position.z = 1.f;
    cam.fov = 60.f;
    cam.near_plane = 0.01f;
    cam.far_plane = 1000.f;
    cam.ortho_scale = 1.f;
    cam.proj_type = AM_PROJECTION_TYPE_ORTHOGRAPHIC;
    return cam;
};

am_camera am_camera_perspective() {
    am_camera cam = am_camera_default();
    cam.proj_type = AM_PROJECTION_TYPE_PERSPECTIVE;
    cam.transform.position.z = 1.f;
    return cam;
};

am_vec3 am_camera_forward(am_camera* cam) {
    return (am_quat_rotate(cam->transform.rotation, am_vec3_create(0.0f, 0.0f, -1.0f)));
};

am_vec3 am_camera_backward(am_camera* cam) {
    return (am_quat_rotate(cam->transform.rotation, am_vec3_create(0.0f, 0.0f, 1.0f)));
};

am_vec3 am_camera_up(am_camera* cam) {
    return (am_quat_rotate(cam->transform.rotation, am_vec3_create(0.0f, 1.0f, 0.0f)));
};

am_vec3 am_camera_down(am_camera* cam) {
    return (am_quat_rotate(cam->transform.rotation, am_vec3_create(0.0f, -1.0f, 0.0f)));
};

am_vec3 am_camera_right(am_camera* cam) {
    return (am_quat_rotate(cam->transform.rotation, am_vec3_create(1.0f, 0.0f, 0.0f)));
};

am_vec3 am_camera_left(am_camera* cam) {
    return (am_quat_rotate(cam->transform.rotation, am_vec3_create(-1.0f, 0.0f, 0.0f)));
};

am_vec3 am_camera_screen_to_world(am_camera* cam, am_vec3 coords, am_int32 view_width, am_int32 view_height) {
    am_vec3 wc = {0};

    // Get inverse of view projection from camera
    am_mat4 inverse_vp = am_mat4_inverse(am_camera_get_view_projection(cam, view_width, view_height));

    am_float32 w_x = (am_float32)coords.x;
    am_float32 w_y = (am_float32)coords.y;
    am_float32 w_z = (am_float32)coords.z;

    // Transform from ndc
    am_vec4 in;
    in.x = (w_x / (am_float32)view_width) * 2.f - 1.f;
    in.y = 1.f - (w_y / (am_float32)view_height) * 2.f;
    in.z = 2.f * w_z - 1.f;
    in.w = 1.f;

    // To world coords
    am_vec4 out = am_mat4_mul_vec4(inverse_vp, in);
    if (out.w == 0.f) return wc;

    out.w = 1.f / out.w;
    wc = am_vec3_create(
        out.x * out.w,
        out.y * out.w,
        out.z * out.w
    );

    return wc;
};

am_mat4 am_camera_get_view_projection(am_camera* cam, am_int32 view_width, am_int32 view_height) {
    am_mat4 view = am_camera_get_view(cam);
    am_mat4 proj = am_camera_get_proj(cam, view_width, view_height);
    return am_mat4_mul(proj, view);
};

am_mat4 am_camera_get_view(am_camera* cam) {
    am_vec3 up = am_camera_up(cam);
    am_vec3 forward = am_camera_forward(cam);
    am_vec3 target = am_vec3_add(forward, cam->transform.position);
    return am_mat4_look_at(cam->transform.position, target, up);
};

am_mat4 am_camera_get_proj(am_camera* cam, am_int32 view_width, am_int32 view_height) {
    am_mat4 proj_mat = am_mat4_identity();

    switch(cam->proj_type)
    {
        case AM_PROJECTION_TYPE_PERSPECTIVE: {
            proj_mat = am_mat4_perspective(cam->fov, (am_float32) view_width / (am_float32) view_height,
                                           cam->near_plane, cam->far_plane);
        } break;

        case AM_PROJECTION_TYPE_ORTHOGRAPHIC: {
            am_float32 _ar = (am_float32)view_width / (am_float32)view_height;
            am_float32 distance = 0.5f * (cam->far_plane - cam->near_plane);
            const am_float32 ortho_scale = cam->ortho_scale;
            const am_float32 aspect_ratio = _ar;
            proj_mat = am_mat4_ortho(
                -ortho_scale * aspect_ratio,
                ortho_scale * aspect_ratio,
                -ortho_scale,
                ortho_scale,
                -distance,
                distance
            );
        } break;
    };
    return proj_mat;
};

void am_camera_offset_orientation(am_camera* cam, am_float32 yaw, am_float32 pitch) {
    am_quat x = am_quat_angle_axis(am_deg2rad(yaw), am_vec3_create(0.f, 1.f, 0.f));   // Absolute up
    am_quat y = am_quat_angle_axis(am_deg2rad(pitch), am_camera_right(cam));                    // Relative right
    cam->transform.rotation = am_quat_mul(am_quat_mul(x, y), cam->transform.rotation);
};


//----------------------------------------------------------------------------//
//                                 END GL IMPL                                //
//----------------------------------------------------------------------------//



//----------------------------------------------------------------------------//
//                              START ENGINE IMPL                             //
//----------------------------------------------------------------------------//

void am_engine_create(am_engine_info engine_info) {
    am_engine_get_instance() = (am_engine*)am_malloc(sizeof(am_engine));
    am_engine *engine = am_engine_get_instance();
    engine->init = engine_info.init;
    engine->update = engine_info.update;
    engine->shutdown = engine_info.shutdown;
    engine->vsync_enabled = engine_info.vsync_enabled;
    if (!engine_info.desired_fps && !engine_info.vsync_enabled) {
        printf("[WARN] am_engine_create (id: ?): No desired fps stated, choosing 60.0f!\n");
        engine_info.desired_fps = 60.0f;
    };
    if (engine_info.desired_fps && engine_info.vsync_enabled) {
        printf("[WARN] am_engine_create (id: ?): Desired fps specified but vsync is on, choosing 60.0f!\n");
        engine_info.desired_fps = 60.0f;
    };
    engine->desired_fps = engine_info.desired_fps;

    engine->platform = am_platform_create();
    am_platform_timer_create();

    engine->ctx_data.textures = NULL;
    am_packed_array_init(engine->ctx_data.textures, sizeof(amgl_texture)*AM_DYN_ARRAY_EMPTY_START_SLOTS);
    engine->ctx_data.shaders = NULL;
    am_packed_array_init(engine->ctx_data.shaders, sizeof(amgl_shader)*AM_DYN_ARRAY_EMPTY_START_SLOTS);
    engine->ctx_data.vertex_buffers = NULL;
    am_packed_array_init(engine->ctx_data.vertex_buffers, sizeof(amgl_vertex_buffer)*AM_DYN_ARRAY_EMPTY_START_SLOTS);
    engine->ctx_data.index_buffers = NULL;
    am_packed_array_init(engine->ctx_data.index_buffers, sizeof(amgl_index_buffer)*AM_DYN_ARRAY_EMPTY_START_SLOTS);
    engine->ctx_data.storage_buffers = NULL;
    am_packed_array_init(engine->ctx_data.storage_buffers, sizeof(amgl_storage_buffer)*AM_DYN_ARRAY_EMPTY_START_SLOTS);
    engine->ctx_data.frame_buffers = NULL;
    am_packed_array_init(engine->ctx_data.frame_buffers, sizeof(amgl_frame_buffer)*AM_DYN_ARRAY_EMPTY_START_SLOTS);
    engine->ctx_data.uniforms = NULL;
    am_packed_array_init(engine->ctx_data.uniforms, sizeof(amgl_uniform)*AM_DYN_ARRAY_EMPTY_START_SLOTS);
    engine->ctx_data.render_passes = NULL;
    am_packed_array_init(engine->ctx_data.render_passes, sizeof(amgl_render_pass)*AM_DYN_ARRAY_EMPTY_START_SLOTS);
    engine->ctx_data.pipelines = NULL;
    am_packed_array_init(engine->ctx_data.pipelines, sizeof(amgl_pipeline)*AM_DYN_ARRAY_EMPTY_START_SLOTS);
    engine->ctx_data.frame_cache.vertex_buffers = NULL;
    am_dyn_array_init((void**)&(engine->ctx_data.frame_cache.vertex_buffers), sizeof(amgl_vertex_buffer));

    am_window_info main = {
        .height = engine_info.win_height,
        .width = engine_info.win_width,
        .x = engine_info.win_x,
        .y = engine_info.win_y,
        .parent = AM_WINDOW_DEFAULT_PARENT,
        .is_fullscreen = engine_info.win_fullscreen
    };
    printf("sl ec %lu\n", strlen(engine_info.win_name));
    if (!strlen(engine_info.win_name)) snprintf(main.name, AM_MAX_NAME_LENGTH, "%s", engine_info.win_name);

    printf("b win\n");
    am_platform_window_create(main);
    printf("a win\n");

    //Base framebuffer
    amgl_frame_buffer base_fbo = {
        .handle = 0, //OpenGL default
        .id = 1
    };
    am_packed_array_add(engine->ctx_data.frame_buffers, base_fbo);

    amgl_set_viewport(0,0, (am_int32)main.width, (am_int32)main.height);

    am_platform *platform = am_engine_get_subsystem(platform);
#if defined(AM_LINUX)
    glXMakeCurrent(am_engine_get_subsystem(platform)->display, am_packed_array_get_ptr(platform->windows, 1)->handle, am_packed_array_get_ptr(platform->windows, 1)->context);
#else
    wglMakeCurrent(am_packed_array_get_ptr(platform->windows, 1)->hdc, am_packed_array_get_ptr(platform->windows, 1)->context);
#endif
    engine->init();
    engine->is_running = true;
};

void am_engine_frame() {
    am_platform *platform = am_engine_get_subsystem(platform);
    am_engine *engine = am_engine_get_instance();

#if defined (AM_LINUX)
    const am_float64 coef = 1000000000.0f;
#else
    const am_float64 coef = 10000000.0f;

#endif
    platform->time.current = (am_float64)am_platform_elapsed_time() / coef;
    platform->time.update = platform->time.current - platform->time.previous;
    platform->time.previous = platform->time.current;

    am_platform_update(platform);
    if (!engine->is_running) {
        engine->shutdown();
        return;
    };

    engine->update();
    if (!engine->is_running) {
        engine->shutdown();
        return;
    };

    for (am_int32 i =  1; i <= am_packed_array_get_count(platform->windows); i++) {
#if defined(AM_LINUX)
        glXSwapBuffers(platform->display, am_packed_array_get_ptr(platform->windows, i)->handle);
#else
        SwapBuffers(am_packed_array_get_ptr(platform->windows, i)->hdc);
#endif
    };

    platform->time.current  = (am_float64)am_platform_elapsed_time() / coef;
    platform->time.render   = platform->time.current - platform->time.previous;
    platform->time.previous = platform->time.current;
    platform->time.frame    = platform->time.update + platform->time.render;            // Total frame time
    platform->time.delta    = platform->time.frame / 1000.f;

    float target = (1000.f / engine->desired_fps);

    if (platform->time.frame < target)
    {
        am_platform_timer_sleep((float)(target - platform->time.frame));

        platform->time.current = (am_float64)am_platform_elapsed_time() / coef;;
        double wait_time = platform->time.current - platform->time.previous;
        platform->time.previous = platform->time.current;
        platform->time.frame += wait_time;
        platform->time.delta = platform->time.frame / 1000.f;
    }
};

void am_engine_quit() {
    am_engine_get_instance()->is_running = false;
};

void am_engine_terminate(){
    am_engine *engine = am_engine_get_instance();
#if defined(AM_LINUX)
    XFlush(am_engine_get_subsystem(platform)->display);
#endif
    am_platform_terminate(am_engine_get_subsystem(platform));

    for (am_int32 i = 0; i < am_packed_array_get_count(engine->ctx_data.pipelines); i++) amgl_pipeline_destroy(engine->ctx_data.pipelines->elements[i].id);
    am_packed_array_destroy(engine->ctx_data.pipelines);
    for (am_int32 i = 0; i < am_packed_array_get_count(engine->ctx_data.textures); i++) amgl_texture_destroy(engine->ctx_data.textures->elements[i].id);
    am_packed_array_destroy(engine->ctx_data.textures);
    for (am_int32 i = 0; i < am_packed_array_get_count(engine->ctx_data.vertex_buffers); i++) amgl_vertex_buffer_destroy(engine->ctx_data.vertex_buffers->elements[i].id);
    am_packed_array_destroy(engine->ctx_data.vertex_buffers);
    for (am_int32 i = 0; i < am_packed_array_get_count(engine->ctx_data.index_buffers); i++) amgl_index_buffer_destroy(engine->ctx_data.index_buffers->elements[i].id);
    am_packed_array_destroy(engine->ctx_data.index_buffers);

    for (am_int32 i = 0; i < am_packed_array_get_count(engine->ctx_data.storage_buffers); i++) amgl_storage_buffer_destroy(engine->ctx_data.storage_buffers->elements[i].id);
    am_packed_array_destroy(engine->ctx_data.storage_buffers);

    for (am_int32 i = 0; i < am_packed_array_get_count(engine->ctx_data.frame_buffers); i++) amgl_frame_buffer_destroy(engine->ctx_data.frame_buffers->elements[i].id);
    am_packed_array_destroy(engine->ctx_data.frame_buffers);
    for (am_int32 i = 0; i < am_packed_array_get_count(engine->ctx_data.uniforms); i++) amgl_uniform_destroy(engine->ctx_data.uniforms->elements[i].id);
    am_packed_array_destroy(engine->ctx_data.uniforms);
    for (am_int32 i = 0; i < am_packed_array_get_count(engine->ctx_data.render_passes); i++) amgl_render_pass_destroy(engine->ctx_data.render_passes->elements[i].id);
    am_packed_array_destroy(engine->ctx_data.render_passes);

    am_dyn_array_destroy(engine->ctx_data.frame_cache.vertex_buffers);

    engine->is_running = false;
    am_free(engine);
};


//----------------------------------------------------------------------------//
//                               END ENGINE IMPL                              //
//----------------------------------------------------------------------------//


//----------------------------------------------------------------------------//
//                               START UTIL IMPL                              //
//----------------------------------------------------------------------------//

char* am_util_read_file(const char *path) {
    FILE *source = fopen(path, "rb");
    if (!source) printf("am_util_read_file (id: ?): Could not open file, errno %d!\n", errno);
    am_int32 rd_size = 0;
    char* buffer = NULL;
    if (source) {
#if defined(AM_LINUX)
        struct stat st;
        stat(path, &st);
        rd_size = (am_int32)st.st_size;
#else
        HANDLE file_hwnd = CreateFile(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        LARGE_INTEGER size;
        GetFileSizeEx(file_hwnd, &size);
        rd_size = (am_int32) size.QuadPart;
        CloseHandle(file_hwnd);
#endif
        buffer = (char*)am_malloc(rd_size+1);
        if (buffer) fread(buffer, 1, rd_size, source);
        buffer[rd_size] = '\0';
    };
    fclose(source);
    return buffer;
};

am_util_obj* am_util_obj_create(char* path) {
    am_util_obj *object = (am_util_obj*)malloc(sizeof(am_util_obj));
    object->vertices = NULL;
    object->normals = NULL;
    object->texture_coords = NULL;
    object->obj_vertices = NULL;
    object->indices = NULL;
    am_dyn_array_init((void**)&object->vertices, sizeof(am_float32));
    am_dyn_array_init((void**)&object->normals, sizeof(am_float32));
    am_dyn_array_init((void**)&object->texture_coords, sizeof(am_float32));
    am_dyn_array_init((void**)&object->obj_vertices, sizeof(am_util_obj_vertex));
    am_dyn_array_init((void**)&object->indices, sizeof(am_int32));

    am_uint32 read = 0;
    char *buffer;
    char *start;
    char *end;
    char *last_endl;
    size_t diff = 0;

    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        printf("[FAIL] am_util_obj_create (id: ?): Could not open file %s!\n", path);
        return NULL;
    };

    buffer = (char*)malloc(2*sizeof(char)*AM_UTIL_OBJ_MAX_BUFFER_SIZE);
    start = buffer;
    while(true) {
        read = fread(start, sizeof(char), AM_UTIL_OBJ_MAX_BUFFER_SIZE, f);
        if (read == 0 && start == buffer) break;

        end = start + read;
        if (end == buffer) break;
        last_endl = end;

        while (last_endl > buffer && *last_endl != '\n') last_endl--;

        if (*last_endl != '\n') break;
        last_endl++;

        am_util_obj_interpret_buffer(buffer, last_endl, object);

        diff = (am_int32)(end - last_endl) < 0 ? 0 : end - last_endl;
        memmove(buffer, last_endl, diff);
        start = buffer + diff;
    };

    fclose(f);
    am_free(buffer);

    am_float32 *v_copy = (am_float32*)malloc(am_dyn_array_get_size(object->vertices));
    memcpy(v_copy, object->vertices, am_dyn_array_get_size(object->vertices));
    am_float32 *t_copy = (am_float32*)malloc(am_dyn_array_get_size(object->texture_coords));
    memcpy(t_copy, object->texture_coords, am_dyn_array_get_size(object->texture_coords));
    am_float32 *n_copy = (am_float32*)malloc(am_dyn_array_get_size(object->normals));
    memcpy(n_copy, object->normals, am_dyn_array_get_size(object->normals));
    am_dyn_array_clear(object->vertices);
    am_dyn_array_clear(object->texture_coords);
    am_dyn_array_clear(object->normals);

    for (am_int32 i = 0; i < am_dyn_array_get_count(object->obj_vertices); i++) {
        am_util_obj_vertex current_v = object->obj_vertices[i];
        am_dyn_array_push(object->vertices, current_v.position.x);
        am_dyn_array_push(object->vertices, current_v.position.y);
        am_dyn_array_push(object->vertices, current_v.position.z);
        am_dyn_array_push(object->texture_coords, t_copy[2*current_v.texture_index]);
        am_dyn_array_push(object->texture_coords, t_copy[2*current_v.texture_index + 1]);
        am_dyn_array_push(object->normals, n_copy[3*current_v.normal_index]);
        am_dyn_array_push(object->normals, n_copy[3*current_v.normal_index + 1]);
        am_dyn_array_push(object->normals, n_copy[3*current_v.normal_index + 2]);
    };

    am_free(v_copy);
    am_free(t_copy);
    am_free(n_copy);

    return object;
};

void am_util_obj_handle_duplicate(am_util_obj_vertex *prev_vertex, am_int32 new_tex_idx, am_int32 new_norm_idx, am_int32 **indices, am_util_obj_vertex **vertices) {
    if (prev_vertex->texture_index == new_tex_idx && prev_vertex->normal_index == new_norm_idx) {
        am_dyn_array_push(*indices, prev_vertex->index);
    } else {
        am_util_obj_vertex *new_vertex = prev_vertex->duplicate_vertex;
        if (new_vertex != NULL) {
            am_util_obj_handle_duplicate(new_vertex, new_tex_idx, new_norm_idx, indices, vertices);
        } else {
            am_util_obj_vertex *duplicate_vertex = (am_util_obj_vertex*)malloc(sizeof(am_util_obj_vertex));
            memset(duplicate_vertex, 0, sizeof(am_util_obj_vertex));
            duplicate_vertex->index = am_dyn_array_get_count(*vertices);
            duplicate_vertex->position = prev_vertex->position;
            duplicate_vertex->texture_index = new_tex_idx;
            duplicate_vertex->normal_index = new_norm_idx;
            prev_vertex->duplicate_vertex = duplicate_vertex;
            am_dyn_array_push(*vertices, *duplicate_vertex);
            am_dyn_array_push(*indices, duplicate_vertex->index);
        };
    };
};

void am_util_obj_delete_duplicate_chain(am_util_obj_vertex *duplicate) {
    if (duplicate == NULL) return;
    am_util_obj_delete_duplicate_chain(duplicate->duplicate_vertex);
    free(duplicate);
};

void am_util_obj_delete(am_util_obj *obj) {
    for (am_int32 i = 0; i < am_dyn_array_get_count(obj->obj_vertices); i++) {
        if (obj->obj_vertices[i].duplicate_vertex != NULL) am_util_obj_delete_duplicate_chain(obj->obj_vertices[i].duplicate_vertex);
    };
    am_dyn_array_destroy(obj->vertices);
    am_dyn_array_destroy(obj->indices);
    am_dyn_array_destroy(obj->normals);
    am_dyn_array_destroy(obj->texture_coords);
    am_dyn_array_destroy(obj->obj_vertices);
};

void am_util_obj_interpret_buffer(char *start, char *end, am_util_obj *object) {
    char *ptr = start;
    char *ptr_end = NULL;
    am_float32 value = 0.0f;
    while (ptr < end) {
        switch (*ptr) {
            case 'v': {
                ptr++;
                switch (*ptr) {
                    case ' ':
                    case '\t': {
                        ptr++;
                        am_util_obj_vertex vertex = {0};
                        value = strtof(ptr, &ptr_end);
                        vertex.position.x = value;
                        am_dyn_array_push(object->vertices, value);
                        ptr = ptr_end;
                        value = strtof(ptr, &ptr_end);
                        vertex.position.y = value;
                        am_dyn_array_push(object->vertices, value);
                        ptr = ptr_end;
                        value = strtof(ptr, &ptr_end);
                        vertex.position.z = value;
                        am_dyn_array_push(object->vertices, value);
                        ptr = ptr_end;
                        vertex.index = am_dyn_array_get_count(object->obj_vertices);
                        vertex.normal_index = -1;
                        vertex.texture_index = -1;
                        am_dyn_array_push(object->obj_vertices, vertex);
                        break;
                    };
                    case 't': {
                        ptr++;
                        value = strtof(ptr, &ptr_end);
                        am_dyn_array_push(object->texture_coords, value);
                        ptr = ptr_end;
                        value = strtof(ptr, &ptr_end);
                        am_dyn_array_push(object->texture_coords, value);
                        ptr = ptr_end;
                        break;
                    }
                    case 'n': {
                        ptr++;
                        value = strtof(ptr, &ptr_end);
                        am_dyn_array_push(object->normals, value);
                        ptr = ptr_end;
                        value = strtof(ptr, &ptr_end);
                        am_dyn_array_push(object->normals, value);
                        ptr = ptr_end;
                        value = strtof(ptr, &ptr_end);
                        am_dyn_array_push(object->normals, value);
                        ptr = ptr_end;
                        break;
                    };
                };
                break;
            };
            case 'f': {
                if (*(ptr + 1) != ' ') break;
                ptr++;
                int v = 0, n = 0, t = 0;
                while (*ptr != '\n' && *ptr != '\r') {
                    value = strtof(ptr, &ptr_end);
                    ptr = ptr_end;
                    v = (am_int32) value;
                    if (*ptr == '/') {
                        ptr++;
                        if (*ptr != '/') {
                            value = strtof(ptr, &ptr_end);
                            ptr = ptr_end;
                            t = (am_int32) value;
                        };
                        if (*ptr == '/') {
                            ptr++;
                            value = strtof(ptr, &ptr_end);
                            ptr = ptr_end;
                            n = (am_int32) value;
                        };
                    };
                    am_int32 index = v - 1;
                    am_util_obj_vertex *current_vertex = &object->obj_vertices[index];
                    int texture_index = t - 1;
                    int normal_index = n - 1;
                    if (!(current_vertex->texture_index != -1 && current_vertex->normal_index != -1)) {
                        current_vertex->texture_index = texture_index;
                        current_vertex->normal_index = normal_index;
                        am_dyn_array_push(object->indices, index);
                    } else {
                        am_util_obj_handle_duplicate(current_vertex, texture_index, normal_index, &object->indices,
                                                     &object->obj_vertices);
                    }
                };
                break;
            };
        };
        ptr++;
    };
};

//----------------------------------------------------------------------------//
//                               END  UTIL IMPL                               //
//----------------------------------------------------------------------------//
#endif