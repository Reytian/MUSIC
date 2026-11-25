/* SPDX-License-Identifier: MPL-2.0 */
#pragma once
#include "plat.h"

#define APP_NAME "CADENCE-1"
#define APP_SPLASH_RGB RGB(125, 225, 194)
#define APP_BLOCK 256u
#define APP_CANVAS_PX (240u * 152u)
#define APP_HOST_EXPECT
#define APP_HOST_REBOOT
#define APP_HOST_CMD

/* An object keeps its number from one release to the next: new ones go on the end. */
enum { OBJ_PANEL, OBJ_PREFS, OBJ_PROGRESS, OBJ_NOBJ };
#define APP_STORE_MAP {0xDC000u, 0xDA000u, 0xD8000u}

void app_init(void);
void app_ui_init(void);
void app_frame(void);
void app_input_only(void);
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12);
void app_panic(void);
void app_say(const char *a, const char *b);
int app_expect(const char *what, int *got);
void app_host_reboot(void);
int app_host_cmd(const char *cmd, const char *a, const char *b, const char *c, const char *d);
