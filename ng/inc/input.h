#ifndef VD_INPUT_H
#define VD_INPUT_H
#include "flecs.h"

typedef enum {
	VD_KEY_Q,
	VD_KEY_W,
	VD_KEY_E,
	VD_KEY_R,
	VD_KEY_T,
	VD_KEY_Y,
	VD_KEY_U,
	VD_KEY_I,
	VD_KEY_O,
	VD_KEY_P,
	VD_KEY_A,
	VD_KEY_S,
	VD_KEY_D,
	VD_KEY_F,
	VD_KEY_G,
	VD_KEY_H,
	VD_KEY_J,
	VD_KEY_K,
	VD_KEY_L,
	VD_KEY_Z,
	VD_KEY_X,
	VD_KEY_C,
	VD_KEY_V,
	VD_KEY_B,
	VD_KEY_N,
	VD_KEY_M,
	VD_KEY_ESCAPE,
	VD_KEY_ENTER,
	VD_KEY_SPACE,
} VD_Key;

typedef enum {
	VD_MOUSE_BUTTON_LEFT,
	VD_MOUSE_BUTTON_RIGHT,
	VD_MOUSE_BUTTON_MIDDLE,
} VD_MouseButton;

typedef enum {
	VD_EVENT_KEYBOARD,
	VD_EVENT_MOUSE,
} VD_EventType;

typedef enum {
	VD_INTERACTION_DOWN,
	VD_INTERACTION_UP,
	VD_INTERACTION_MOVE,
} VD_InteractionType;

typedef struct {
	VD_Key				key;
	VD_InteractionType	interaction_type;
} VD_KeyboardEvent;

typedef struct {
	VD_MouseButton		mouse_button;
	VD_InteractionType	interaction_type;
} VD_MouseButtonEvent;

typedef struct {
	float position[2];
	float delta[2];
} VD_MouseMotionEvent;

typedef struct {
	VD_EventType			type;
	union {
		VD_KeyboardEvent	keyboard;
		VD_MouseButtonEvent mouse_button;
		VD_MouseMotionEvent mouse_motion;
	} data;
} VD_InputEvent;

typedef struct {

} InputReceiverComponent;

#endif // !VD_INPUT_H