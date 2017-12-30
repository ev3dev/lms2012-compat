/*
 * LEGO® MINDSTORMS EV3
 *
 * Copyright (C) 2010-2013 The LEGO Group
 * Copyright (C) 2016 David Lechner <david@lechnology.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <glib-unix.h>
#include <grx-3.0.h>

#include "lms2012.h"
#include "c_ui.h"
#include "led.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define LED_TRIGGER_ON      "default-on"
#define LED_TRIGGER_OFF     "none"
#define LED_TRIGGER_FLASH   "heartbeat"
#define LED_TRIGGER_PULSE   "heartbeat"
// TODO: need a different trigger for flash/pulse

// TODO: this #define and struct is in a kernel header starting with v4.10,
// but we need to run on 4.9 for now.
 #define LED_MAX_NAME_SIZE 64

struct uleds_user_dev {
    char name[LED_MAX_NAME_SIZE];
    int max_brightness;
};

static cUiLedFlags led_state;

/**
 * @brief Tries to open the trigger attribute of a Linux leds subsystem device
 *
 * Use udev to search for a Linux leds class device that will serve as one of
 * the 4 LEDs used on the EV3 platform.
 *
 * @param name  The name of the leds class device
 * @return      The file descriptor or -1 on error.
 */
int cUiLedOpenTriggerFile(const char *name)
{
    struct udev_enumerate *enumerate;
    struct udev_list_entry *list;
    int fd = -1;

    enumerate = udev_enumerate_new(VMInstance.udev);
    udev_enumerate_add_match_subsystem(enumerate, "leds");
    udev_enumerate_add_match_sysname(enumerate, name);
    udev_enumerate_scan_devices(enumerate);
    list = udev_enumerate_get_list_entry(enumerate);
    if (list) {
        // just taking the first match in the list
        const char *path = udev_list_entry_get_name(list);
        char trigger_path[255];

        snprintf(trigger_path, 255, "%s/trigger", path);
        fd = open(trigger_path, O_WRONLY);
        if (fd == -1) {
            fprintf(stderr, "Could not open '%s': %s\n", trigger_path,
                    strerror(errno));
        }
    }
    udev_enumerate_unref(enumerate);

    return fd;
}

typedef struct {
    int fd;
    cUiLedFlags flags;
} UserLed;

static gboolean cUILedUpdateUserLed(gint fd, GIOCondition condition,
                                    gpointer user_data)
{
    UserLed *user_led = user_data;
    GrxColor color;
    int brightness;

    read(fd, &brightness, sizeof(brightness));

    if (brightness) {
        led_state |= user_led->flags;
    }
    else {
        led_state &= ~user_led->flags;
    }

    // just using one of the LEDs since both are always the same
    if (user_led->flags & USER_LED_LEFT) {
        switch (led_state & (USER_LED_RED | USER_LED_GREEN)) {
        case USER_LED_RED:
            color = grx_color_get(255, 0, 0);
            break;
        case USER_LED_GREEN:
            color = grx_color_get(0, 192, 0);
            break;
        case USER_LED_RED | USER_LED_GREEN:
            color = grx_color_get(255, 192, 0);
            break;
        default:
            color = GRX_COLOR_BLACK;
            break;
        }
        grx_draw_filled_box(10, 138, grx_get_max_x() - 10, grx_get_max_y() - 10, color);
    }

    return G_SOURCE_CONTINUE;
}

static void cUiLedFreeUserLed(gpointer user_data)
{
    UserLed *user_led = user_data;

    close(user_led->fd);
    g_free(user_led);
}

/**
 * @brief Tries to create a user-defined LED and returns the file handle
 *
 * @param name  The name of the leds class device
 * @param mask  The color of the LED (red or green)
 * @return      The GSource id. It should be removed with g_source_remove()
 *              when no longer needed.
 */
guint cUiLedCreateUserLed(const char *name, cUiLedFlags flags)
{
    struct uleds_user_dev uled_dev;
    UserLed *user_led;
    int fd;

    fd = open("/dev/uleds", O_RDWR);
    if (fd == -1) {
        g_debug("Failed to open /dev/uleds: %s", strerror(errno));
        return 0;
    }
    strncpy(uled_dev.name, name, LED_MAX_NAME_SIZE);
    uled_dev.max_brightness = 255;
    write(fd, &uled_dev, sizeof(uled_dev));

    user_led = g_malloc(sizeof(*user_led));
    user_led->fd = fd;
    user_led->flags = flags;
    return g_unix_fd_add_full(G_PRIORITY_DEFAULT, fd, G_IO_IN,
        cUILedUpdateUserLed, user_led, cUiLedFreeUserLed);
}

/**
 * @brief Set the state of the brick status LEDs.
 *
 * If UiInstance.Warnlight is active, the color will be set to orange regardless
 * of the state selected.
 *
 * @param State     The new state.
 */
void cUiLedSetState(LEDPATTERN State)
{
    const char *green_trigger;
    const char *red_trigger;

    UiInstance.LedState = State;

    switch (State) {
    case LED_BLACK:
        green_trigger = UiInstance.Warnlight ? LED_TRIGGER_ON : LED_TRIGGER_OFF;
        red_trigger = UiInstance.Warnlight ? LED_TRIGGER_ON : LED_TRIGGER_OFF;
        break;
    case LED_GREEN:
        green_trigger = LED_TRIGGER_ON;
        red_trigger = UiInstance.Warnlight ? LED_TRIGGER_ON : LED_TRIGGER_OFF;
        break;
    case LED_RED:
        green_trigger = UiInstance.Warnlight ? LED_TRIGGER_ON : LED_TRIGGER_OFF;
        red_trigger = LED_TRIGGER_ON;
        break;
    case LED_ORANGE:
        green_trigger = LED_TRIGGER_ON;
        red_trigger = LED_TRIGGER_ON;
        break;
    case LED_GREEN_FLASH:
        green_trigger = LED_TRIGGER_FLASH;
        red_trigger = UiInstance.Warnlight ? LED_TRIGGER_FLASH : LED_TRIGGER_OFF;
        break;
    case LED_RED_FLASH:
        green_trigger = UiInstance.Warnlight ? LED_TRIGGER_FLASH : LED_TRIGGER_OFF;
        red_trigger = LED_TRIGGER_FLASH;
        break;
    case LED_ORANGE_FLASH:
        green_trigger = LED_TRIGGER_FLASH;
        red_trigger = LED_TRIGGER_FLASH;
        break;
    case LED_GREEN_PULSE:
        green_trigger = LED_TRIGGER_PULSE;
        red_trigger = UiInstance.Warnlight ? LED_TRIGGER_PULSE : LED_TRIGGER_OFF;
        break;
    case LED_RED_PULSE:
        green_trigger = UiInstance.Warnlight ? LED_TRIGGER_PULSE : LED_TRIGGER_OFF;
        red_trigger = LED_TRIGGER_PULSE;
        break;
    case LED_ORANGE_PULSE:
        green_trigger = LED_TRIGGER_PULSE;
        red_trigger = LED_TRIGGER_PULSE;
        break;
    default:
        // don't crash if we get bad State
        return;
    }

    if (UiInstance.LedRightRedTriggerFile >= MIN_HANDLE) {
        lseek(UiInstance.LedRightRedTriggerFile, 0, SEEK_SET);
        write(UiInstance.LedRightRedTriggerFile, red_trigger, strlen(red_trigger));
    }
    if (UiInstance.LedLeftRedTriggerFile >= MIN_HANDLE) {
        lseek(UiInstance.LedLeftRedTriggerFile, 0, SEEK_SET);
        write(UiInstance.LedLeftRedTriggerFile, red_trigger, strlen(red_trigger));
    }
    if (UiInstance.LedRightGreenTriggerFile >= MIN_HANDLE) {
        lseek(UiInstance.LedRightGreenTriggerFile, 0, SEEK_SET);
        write(UiInstance.LedRightGreenTriggerFile, green_trigger, strlen(green_trigger));
    }
    if (UiInstance.LedLeftGreenTriggerFile >= MIN_HANDLE) {
        lseek(UiInstance.LedLeftGreenTriggerFile, 0, SEEK_SET);
        write(UiInstance.LedLeftGreenTriggerFile, green_trigger, strlen(green_trigger));
    }
}
