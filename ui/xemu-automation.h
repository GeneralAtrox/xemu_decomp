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

#ifndef XEMU_AUTOMATION_H
#define XEMU_AUTOMATION_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Set by the -automation option. xemu then runs beside the user without
 * getting in the way: the window opens minimized and never takes the focus,
 * the host's keyboard and gamepads are not bound, a virtual controller on
 * port 1 takes its state from the xemu-input-set QMP command, and the
 * settings are not saved on exit. Pair it with -qmp to send the commands.
 */
extern bool g_xemu_automation;

/* Creates the virtual controller and binds it to port 1. */
void xemu_automation_init_input(void);

/*
 * The path an xemu-screenshot command asked for, or NULL. The caller saves
 * the frame there and frees the path with g_free().
 */
char *xemu_automation_take_screenshot_request(void);

#ifdef __cplusplus
}
#endif

#endif
