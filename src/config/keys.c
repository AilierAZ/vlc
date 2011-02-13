/*****************************************************************************
 * keys.c: keys configuration
 *****************************************************************************
 * Copyright (C) 2003-2009 the VideoLAN team
 *
 * Authors: Sigmund Augdal Helberg <dnumgis@videolan.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/**
 * \file
 * This file defines functions and structures for hotkey handling in vlc
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <limits.h>
#include <search.h>

#include <vlc_common.h>
#include <vlc_keys.h>
#include "configuration.h"
#include "libvlc.h"

typedef struct key_descriptor_s
{
    const char psz_key_string[20];
    uint32_t i_key_code;
} key_descriptor_t;

static const struct key_descriptor_s vlc_keys[] =
{   /* Alphabetical order */
    { "Backspace",         KEY_BACKSPACE         },
    { "Browser Back",      KEY_BROWSER_BACK      },
    { "Browser Favorites", KEY_BROWSER_FAVORITES },
    { "Browser Forward",   KEY_BROWSER_FORWARD   },
    { "Browser Home",      KEY_BROWSER_HOME      },
    { "Browser Refresh",   KEY_BROWSER_REFRESH   },
    { "Browser Search",    KEY_BROWSER_SEARCH    },
    { "Browser Stop",      KEY_BROWSER_STOP      },
    { "Delete",            KEY_DELETE            },
    { "Down",              KEY_DOWN              },
    { "End",               KEY_END               },
    { "Enter",             KEY_ENTER             },
    { "Esc",               KEY_ESC               },
    { "F1",                KEY_F1                },
    { "F10",               KEY_F10               },
    { "F11",               KEY_F11               },
    { "F12",               KEY_F12               },
    { "F2",                KEY_F2                },
    { "F3",                KEY_F3                },
    { "F4",                KEY_F4                },
    { "F5",                KEY_F5                },
    { "F6",                KEY_F6                },
    { "F7",                KEY_F7                },
    { "F8",                KEY_F8                },
    { "F9",                KEY_F9                },
    { "Home",              KEY_HOME              },
    { "Insert",            KEY_INSERT            },
    { "Left",              KEY_LEFT              },
    { "Media Next Track",  KEY_MEDIA_NEXT_TRACK  },
    { "Media Play Pause",  KEY_MEDIA_PLAY_PAUSE  },
    { "Media Prev Track",  KEY_MEDIA_PREV_TRACK  },
    { "Media Stop",        KEY_MEDIA_STOP        },
    { "Menu",              KEY_MENU              },
    { "Mouse Wheel Down",  KEY_MOUSEWHEELDOWN    },
    { "Mouse Wheel Left",  KEY_MOUSEWHEELLEFT    },
    { "Mouse Wheel Right", KEY_MOUSEWHEELRIGHT   },
    { "Mouse Wheel Up",    KEY_MOUSEWHEELUP      },
    { "Page Down",         KEY_PAGEDOWN          },
    { "Page Up",           KEY_PAGEUP            },
    { "Right",             KEY_RIGHT             },
    { "Space",             ' '                   },
    { "Tab",               KEY_TAB               },
    { "Unset",             KEY_UNSET             },
    { "Up",                KEY_UP                },
    { "Volume Mute",       KEY_VOLUME_MUTE       },
    { "Volume Down",       KEY_VOLUME_DOWN       },
    { "Volume Up",         KEY_VOLUME_UP         },
};
#define KEYS_COUNT (sizeof(vlc_keys)/sizeof(vlc_keys[0]))

static int keystrcmp (const void *key, const void *elem)
{
    const char *sa = key, *sb = elem;
    return strcmp (sa, sb);
}

/* Convert Unicode code point to UTF-8 */
static char *utf8_cp (uint_fast32_t cp, char *buf)
{
    if (cp < (1 << 7))
    {
        buf[1] = 0;
        buf[0] = cp;
    }
    else if (cp < (1 << 11))
    {
        buf[2] = 0;
        buf[1] = 0x80 | (cp & 0x3F);
        cp >>= 6;
        buf[0] = 0xC0 | cp;
    }
    else if (cp < (1 << 16))
    {
        buf[3] = 0;
        buf[2] = 0x80 | (cp & 0x3F);
        cp >>= 6;
        buf[1] = 0x80 | (cp & 0x3F);
        cp >>= 6;
        buf[0] = 0xE0 | cp;
    }
    else if (cp < (1 << 21))
    {
        buf[4] = 0;
        buf[3] = 0x80 | (cp & 0x3F);
        cp >>= 6;
        buf[2] = 0x80 | (cp & 0x3F);
        cp >>= 6;
        buf[1] = 0x80 | (cp & 0x3F);
        cp >>= 6;
        buf[0] = 0xE0 | cp;
    }
    else
        return NULL;
    return buf;
}

/**
 * Parse a human-readable string representation of a VLC key code.
 * @return a VLC key code, or KEY_UNSET on failure.
 */
static
uint_fast32_t vlc_str2keycode (const char *name)
{
    uint_fast32_t mods = 0;
    uint32_t code;

    for (;;)
    {
        size_t len = strcspn (name, "-+");
        if (len == 0 || name[len] == '\0')
            break;

        if (len == 4 && !strncasecmp (name, "Ctrl", 4))
            mods |= KEY_MODIFIER_CTRL;
        if (len == 3 && !strncasecmp (name, "Alt", 3))
            mods |= KEY_MODIFIER_ALT;
        if (len == 5 && !strncasecmp (name, "Shift", 5))
            mods |= KEY_MODIFIER_SHIFT;
        if (len == 4 && !strncasecmp (name, "Meta", 4))
            mods |= KEY_MODIFIER_META;
        if (len == 7 && !strncasecmp (name, "Command", 7))
            mods |= KEY_MODIFIER_COMMAND;

        name += len + 1;
    }

    key_descriptor_t *d = bsearch (name, vlc_keys, KEYS_COUNT,
                                   sizeof (vlc_keys[0]), keystrcmp);
    if (d != NULL)
        code = d->i_key_code;
    else
    if (vlc_towc (name, &code) <= 0)
        code = KEY_UNSET;

    if (code != KEY_UNSET)
        code |= mods;
    return code;
}

/**
 * Format a human-readable and unique representation of a VLC key code
 * (including modifiers).
 * @return a heap-allocated string, or NULL on error.
 */
char *vlc_keycode2str (uint_fast32_t code)
{
    const char *name;
    char *str, buf[5];
    uintptr_t key = code & ~KEY_MODIFIER;

    for (size_t i = 0; i < KEYS_COUNT; i++)
        if (vlc_keys[i].i_key_code == key)
        {
            name = vlc_keys[i].psz_key_string;
            goto found;
        }

    if (utf8_cp (key, buf) == NULL)
        return NULL;
    name = buf;

found:
    if (asprintf (&str, "%s%s%s%s%s%s",
                  (code & KEY_MODIFIER_CTRL) ? "Ctrl+" : "",
                  (code & KEY_MODIFIER_ALT) ? "Alt+" : "",
                  (code & KEY_MODIFIER_SHIFT) ? "Shift+" : "",
                  (code & KEY_MODIFIER_META) ? "Meta+" : "",
                  (code & KEY_MODIFIER_COMMAND) ? "Command+" : "", name) == -1)
        return NULL;
    return str;
}


/*** VLC key map ***/

struct action
{
    char name[24];
    int  value;
};

static const struct action actions[] =
{
    /* *MUST* be sorted (ASCII order) */
    { "key-aspect-ratio", ACTIONID_ASPECT_RATIO, },
    { "key-audio-track", ACTIONID_AUDIO_TRACK, },
    { "key-audiodelay-down", ACTIONID_AUDIODELAY_DOWN, },
    { "key-audiodelay-up", ACTIONID_AUDIODELAY_UP, },
    { "key-audiodevice-cycle", ACTIONID_AUDIODEVICE_CYCLE, },
    { "key-chapter-next", ACTIONID_CHAPTER_NEXT, },
    { "key-chapter-prev", ACTIONID_CHAPTER_PREV, },
    { "key-crop", ACTIONID_CROP, },
    { "key-crop-bottom", ACTIONID_CROP_BOTTOM, },
    { "key-crop-left", ACTIONID_CROP_LEFT, },
    { "key-crop-right", ACTIONID_CROP_RIGHT, },
    { "key-crop-top", ACTIONID_CROP_TOP, },
    { "key-decr-scalefactor", ACTIONID_SCALE_DOWN, },
    { "key-deinterlace", ACTIONID_DEINTERLACE, },
    { "key-disc-menu", ACTIONID_DISC_MENU, },
    { "key-faster", ACTIONID_FASTER, },
    { "key-frame-next", ACTIONID_FRAME_NEXT, },
    { "key-incr-scalefactor", ACTIONID_SCALE_UP, },
    { "key-intf-hide", ACTIONID_INTF_HIDE, },
    { "key-intf-show", ACTIONID_INTF_SHOW, },
    { "key-jump+extrashort", ACTIONID_JUMP_FORWARD_EXTRASHORT, },
    { "key-jump+long", ACTIONID_JUMP_FORWARD_LONG, },
    { "key-jump+medium", ACTIONID_JUMP_FORWARD_MEDIUM, },
    { "key-jump+short", ACTIONID_JUMP_FORWARD_SHORT, },
    { "key-jump-extrashort", ACTIONID_JUMP_BACKWARD_EXTRASHORT, },
    { "key-jump-long", ACTIONID_JUMP_BACKWARD_LONG, },
    { "key-jump-medium", ACTIONID_JUMP_BACKWARD_MEDIUM, },
    { "key-jump-short", ACTIONID_JUMP_BACKWARD_SHORT, },
    { "key-leave-fullscreen", ACTIONID_LEAVE_FULLSCREEN, },
    { "key-loop", ACTIONID_LOOP, },
    { "key-menu-down", ACTIONID_MENU_DOWN, },
    { "key-menu-left", ACTIONID_MENU_LEFT, },
    { "key-menu-off", ACTIONID_MENU_OFF, },
    { "key-menu-on", ACTIONID_MENU_ON, },
    { "key-menu-right", ACTIONID_MENU_RIGHT, },
    { "key-menu-select", ACTIONID_MENU_SELECT, },
    { "key-menu-up", ACTIONID_MENU_UP, },
    { "key-nav-activate", ACTIONID_NAV_ACTIVATE, },
    { "key-nav-down", ACTIONID_NAV_DOWN, },
    { "key-nav-left", ACTIONID_NAV_LEFT, },
    { "key-nav-right", ACTIONID_NAV_RIGHT, },
    { "key-nav-up", ACTIONID_NAV_UP, },
    { "key-next", ACTIONID_NEXT, },
    { "key-pause", ACTIONID_PAUSE, },
    { "key-play", ACTIONID_PLAY, },
    { "key-play-bookmark1", ACTIONID_PLAY_BOOKMARK1, },
    { "key-play-bookmark10", ACTIONID_PLAY_BOOKMARK10, },
    { "key-play-bookmark2", ACTIONID_PLAY_BOOKMARK2, },
    { "key-play-bookmark3", ACTIONID_PLAY_BOOKMARK3, },
    { "key-play-bookmark4", ACTIONID_PLAY_BOOKMARK4, },
    { "key-play-bookmark5", ACTIONID_PLAY_BOOKMARK5, },
    { "key-play-bookmark6", ACTIONID_PLAY_BOOKMARK6, },
    { "key-play-bookmark7", ACTIONID_PLAY_BOOKMARK7, },
    { "key-play-bookmark8", ACTIONID_PLAY_BOOKMARK8, },
    { "key-play-bookmark9", ACTIONID_PLAY_BOOKMARK9, },
    { "key-play-pause", ACTIONID_PLAY_PAUSE, },
    { "key-position", ACTIONID_POSITION, },
    { "key-prev", ACTIONID_PREV, },
    { "key-quit", ACTIONID_QUIT, },
    { "key-random", ACTIONID_RANDOM, },
    { "key-rate-faster-fine", ACTIONID_RATE_FASTER_FINE, },
    { "key-rate-normal", ACTIONID_RATE_NORMAL, },
    { "key-rate-slower-fine", ACTIONID_RATE_SLOWER_FINE, },
    { "key-record", ACTIONID_RECORD, },
    { "key-set-bookmark1", ACTIONID_SET_BOOKMARK1, },
    { "key-set-bookmark10", ACTIONID_SET_BOOKMARK10, },
    { "key-set-bookmark2", ACTIONID_SET_BOOKMARK2, },
    { "key-set-bookmark3", ACTIONID_SET_BOOKMARK3, },
    { "key-set-bookmark4", ACTIONID_SET_BOOKMARK4, },
    { "key-set-bookmark5", ACTIONID_SET_BOOKMARK5, },
    { "key-set-bookmark6", ACTIONID_SET_BOOKMARK6, },
    { "key-set-bookmark7", ACTIONID_SET_BOOKMARK7, },
    { "key-set-bookmark8", ACTIONID_SET_BOOKMARK8, },
    { "key-set-bookmark9", ACTIONID_SET_BOOKMARK9, },
    { "key-slower", ACTIONID_SLOWER, },
    { "key-snapshot", ACTIONID_SNAPSHOT, },
    { "key-stop", ACTIONID_STOP, },
    { "key-subdelay-down", ACTIONID_SUBDELAY_DOWN, },
    { "key-subdelay-up", ACTIONID_SUBDELAY_UP, },
    { "key-subpos-down", ACTIONID_SUBPOS_DOWN, },
    { "key-subpos-up", ACTIONID_SUBPOS_UP, },
    { "key-subtitle-track", ACTIONID_SUBTITLE_TRACK, },
    { "key-title-next", ACTIONID_TITLE_NEXT, },
    { "key-title-prev", ACTIONID_TITLE_PREV, },
    { "key-toggle-autoscale", ACTIONID_TOGGLE_AUTOSCALE, },
    { "key-toggle-fullscreen", ACTIONID_TOGGLE_FULLSCREEN, },
    { "key-uncrop-bottom", ACTIONID_UNCROP_BOTTOM, },
    { "key-uncrop-left", ACTIONID_UNCROP_LEFT, },
    { "key-uncrop-right", ACTIONID_UNCROP_RIGHT, },
    { "key-uncrop-top", ACTIONID_UNCROP_TOP, },
    { "key-unzoom", ACTIONID_UNZOOM, },
    { "key-vol-down", ACTIONID_VOL_DOWN, },
    { "key-vol-mute", ACTIONID_VOL_MUTE, },
    { "key-vol-up", ACTIONID_VOL_UP, },
    { "key-wallpaper", ACTIONID_WALLPAPER, },
    { "key-zoom", ACTIONID_ZOOM, },
    { "key-zoom-double", ACTIONID_ZOOM_DOUBLE, },
    { "key-zoom-half", ACTIONID_ZOOM_HALF, },
    { "key-zoom-original", ACTIONID_ZOOM_ORIGINAL, },
    { "key-zoom-quarter", ACTIONID_ZOOM_QUARTER, },
};

#define ACTIONS_COUNT (sizeof (actions) / sizeof (actions[0]))

struct mapping
{
    uint32_t  key; ///< Key code
    vlc_key_t action; ///< Action ID
};

static int keycmp (const void *a, const void *b)
{
    const struct mapping *ka = a, *kb = b;

#if (INT_MAX >= 0x7fffffff)
    return ka->key - kb->key;
#else
    return (ka->key < kb->key) ? -1 : (ka->key > kb->key) ? +1 : 0;
#endif
}

struct vlc_actions
{
    void *map; /* Key map */
    struct hotkey keys[0];
};

static int vlc_key_to_action (vlc_object_t *libvlc, const char *varname,
                              vlc_value_t prevkey, vlc_value_t curkey, void *d)
{
    const struct vlc_actions *as = d;
    const struct mapping **pent;
    uint32_t keycode = curkey.i_int;

    pent = tfind (&keycode, &as->map, keycmp);
    if (pent == NULL)
        return VLC_SUCCESS;

    (void) varname;
    (void) prevkey;
    return var_SetInteger (libvlc, "key-action", (*pent)->action);
}


/**
 * Initializes the key map from configuration.
 */
struct vlc_actions *vlc_InitActions (libvlc_int_t *libvlc)
{
    vlc_object_t *obj = VLC_OBJECT(libvlc);
    struct hotkey *keys;
    struct vlc_actions *as = malloc (sizeof (*as) + (ACTIONS_COUNT + 1) * sizeof (*keys));

    if (unlikely(as == NULL))
        return NULL;
    as->map = NULL;
    keys = as->keys;

    var_Create (libvlc, "key-pressed", VLC_VAR_INTEGER);
    var_Create (libvlc, "key-action", VLC_VAR_INTEGER);

    /* Initialize from configuration */
    for (size_t i = 0; i < ACTIONS_COUNT; i++)
    {
#ifndef NDEBUG
        if (i > 0
         && strcmp (actions[i-1].name, actions[i].name) >= 0)
        {
            msg_Err (libvlc, "%s and %s are not ordered properly",
                     actions[i-1].name, actions[i].name);
            abort ();
        }
#endif
        keys->psz_action = actions[i].name;
        keys->i_action = actions[i].value;
        keys++;

        char *str = var_InheritString (obj, actions[i].name);
        if (str == NULL)
            continue;

        for (char *buf, *key = strtok_r (str, "\t", &buf);
             key != NULL;
             key = strtok_r (NULL, "\t", &buf))
        {
            uint32_t code = vlc_str2keycode (key);

            if (code == KEY_UNSET)
            {
                msg_Warn (obj, "Key \"%s\" unrecognized", key);
                continue;
            }

            struct mapping *entry = malloc (sizeof (*entry));
            if (entry == NULL)
                continue;
            entry->key = code;
            entry->action = actions[i].value;

            struct mapping **pent = tsearch (entry, &as->map, keycmp);
            if (unlikely(pent == NULL))
                continue;
            if (*pent != entry)
            {
                free (entry);
                msg_Warn (obj, "Key \"%s\" bound to multiple actions", key);
            }
        }
        free (str);
    }

    keys->psz_action = NULL;
    keys->i_action = 0;

    libvlc->p_hotkeys = as->keys;
    var_AddCallback (libvlc, "key-pressed", vlc_key_to_action, as);
    return as;
}

/**
 * Destroys the key map.
 */
void vlc_DeinitActions (libvlc_int_t *libvlc, struct vlc_actions *as)
{
    if (unlikely(as == NULL))
        return;

    var_DelCallback (libvlc, "key-pressed", vlc_key_to_action, as);
    tdestroy (as->map, free);
    free (as);
    libvlc->p_hotkeys = NULL;
}


static int actcmp(const void *key, const void *ent)
{
    const struct action *act = ent;
    return strcmp(key, act->name);
}

/**
 * Get the action ID from the action name in the configuration subsystem.
 * @return the action ID or ACTIONID_NONE on error.
 */
vlc_key_t vlc_GetActionId(const char *name)
{
    const struct action *act;

    act = bsearch(name, actions, ACTIONS_COUNT, sizeof(*act), actcmp);
    return (act != NULL) ? act->value : ACTIONID_NONE;
}
