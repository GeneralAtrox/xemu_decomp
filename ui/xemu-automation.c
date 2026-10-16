/*
 * xemu automation: driving xemu from a script over QMP
 *
 * Copyright (c) 2026 xemu_decomp contributors
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qapi/qapi-commands-ui.h"
#include "xemu-automation.h"
#include "xemu-input.h"

bool g_xemu_automation;

static ControllerState *automation_controller;

static GMutex screenshot_lock;
static char *screenshot_request;

static const struct {
    const char *name;
    uint16_t mask;
} button_names[] = {
    { "a", CONTROLLER_BUTTON_A },
    { "b", CONTROLLER_BUTTON_B },
    { "x", CONTROLLER_BUTTON_X },
    { "y", CONTROLLER_BUTTON_Y },
    { "black", CONTROLLER_BUTTON_BLACK },
    { "white", CONTROLLER_BUTTON_WHITE },
    { "back", CONTROLLER_BUTTON_BACK },
    { "start", CONTROLLER_BUTTON_START },
    { "dpad-up", CONTROLLER_BUTTON_DPAD_UP },
    { "dpad-down", CONTROLLER_BUTTON_DPAD_DOWN },
    { "dpad-left", CONTROLLER_BUTTON_DPAD_LEFT },
    { "dpad-right", CONTROLLER_BUTTON_DPAD_RIGHT },
    { "left-stick", CONTROLLER_BUTTON_LSTICK },
    { "right-stick", CONTROLLER_BUTTON_RSTICK },
};

void xemu_automation_init_input(void)
{
    ControllerState *state = g_new0(ControllerState, 1);
    state->type = INPUT_DEVICE_AUTOMATION;
    state->name = "Automation";
    state->bound = -1;
    state->peripheral_types[0] = PERIPHERAL_NONE;
    state->peripheral_types[1] = PERIPHERAL_NONE;

    xemu_input_bind(0, state, 0);
    QTAILQ_INSERT_TAIL(&available_controllers, state, entry);
    automation_controller = state;
}

/* -1 to 1 onto the controller's axis range, as the SDL gamepad path fills
 * it. */
static int16_t stick_value(bool has, double value)
{
    if (!has) {
        return 0;
    }
    value = MAX(-1.0, MIN(1.0, value));
    return value < 0 ? (int16_t)(value * 32768.0) : (int16_t)(value * 32767.0);
}

static int16_t trigger_value(bool has, double value)
{
    if (!has) {
        return 0;
    }
    return (int16_t)(MAX(0.0, MIN(1.0, value)) * 32767.0);
}

void qmp_xemu_input_set(strList *buttons, bool has_left_x, double left_x,
                        bool has_left_y, double left_y, bool has_right_x,
                        double right_x, bool has_right_y, double right_y,
                        bool has_left_trigger, double left_trigger,
                        bool has_right_trigger, double right_trigger,
                        Error **errp)
{
    if (!automation_controller) {
        error_setg(errp, "xemu-input-set needs xemu started with -automation");
        return;
    }

    uint16_t held = 0;
    for (strList *button = buttons; button; button = button->next) {
        size_t i = 0;
        while (i < ARRAY_SIZE(button_names) &&
               strcmp(button_names[i].name, button->value) != 0) {
            i++;
        }
        if (i == ARRAY_SIZE(button_names)) {
            error_setg(errp, "unknown button '%s'", button->value);
            return;
        }
        held |= button_names[i].mask;
    }

    ControllerState *state = automation_controller;
    state->axis[CONTROLLER_AXIS_LSTICK_X] = stick_value(has_left_x, left_x);
    state->axis[CONTROLLER_AXIS_LSTICK_Y] = stick_value(has_left_y, left_y);
    state->axis[CONTROLLER_AXIS_RSTICK_X] = stick_value(has_right_x, right_x);
    state->axis[CONTROLLER_AXIS_RSTICK_Y] = stick_value(has_right_y, right_y);
    state->axis[CONTROLLER_AXIS_LTRIG] =
        trigger_value(has_left_trigger, left_trigger);
    state->axis[CONTROLLER_AXIS_RTRIG] =
        trigger_value(has_right_trigger, right_trigger);
    state->buttons = held;
}

void qmp_xemu_screenshot(const char *filename, Error **errp)
{
    if (!filename[0]) {
        error_setg(errp, "xemu-screenshot needs a filename");
        return;
    }
    g_mutex_lock(&screenshot_lock);
    g_free(screenshot_request);
    screenshot_request = g_strdup(filename);
    g_mutex_unlock(&screenshot_lock);
}

char *xemu_automation_take_screenshot_request(void)
{
    g_mutex_lock(&screenshot_lock);
    char *path = screenshot_request;
    screenshot_request = NULL;
    g_mutex_unlock(&screenshot_lock);
    return path;
}
