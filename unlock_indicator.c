/*
 * vim:ts=4:sw=4:expandtab
 *
 * © 2010 Michael Stapelberg
 *
 * See LICENSE for licensing information
 *
 */
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <xcb/xcb.h>
#include <ev.h>
#include <cairo.h>
#include <cairo/cairo-xcb.h>

#include "fancyconf.h"
#include "i3lock.h"
#include "xcb.h"
#include "unlock_indicator.h"
#include "randr.h"
#include "dpi.h"

/*******************************************************************************
 * Variables defined in i3lock.c.
 ******************************************************************************/

extern bool debug_mode;

/* The current position in the input buffer. Useful to determine if any
 * characters of the password have already been entered or not. */
int input_position;

/* The lock window. */
extern xcb_window_t win;

/* The current resolution of the X11 root window. */
extern uint32_t last_resolution[2];

/* Whether the unlock indicator is enabled (defaults to true). */
extern bool unlock_indicator;

/* List of pressed modifiers, or NULL if none are pressed. */
extern char *modifier_string;

/* A Cairo surface containing the specified image (-i), if any. */
extern cairo_surface_t *img;

/* Whether the image should be tiled. */
extern bool tile;

/* Whether the image should be centered */
extern bool img_centered;

/* The background color to use (in hex). */
extern char color[7];

/* Whether the failed attempts should be displayed. */
extern bool show_failed_attempts;
/* Number of failed unlock attempts. */
extern int failed_attempts;

/*******************************************************************************
 * Variables defined in xcb.c.
 ******************************************************************************/

/* The root screen, to determine the DPI. */
extern xcb_screen_t *screen;

/*******************************************************************************
 * Local variables.
 ******************************************************************************/

/* Cache the screen’s visual, necessary for creating a Cairo context. */
static xcb_visualtype_t *vistype;

/* Maintain the current unlock/auth state to draw the appropriate unlock
 * indicator. */
unlock_state_t unlock_state;
auth_state_t auth_state;

/*
 * Draws global image with fill color onto a pixmap with the given
 * resolution and returns it.
 *
 */
xcb_pixmap_t draw_image(uint32_t *resolution) {
    xcb_pixmap_t bg_pixmap = XCB_NONE;
    const double scaling_factor = get_dpi_value() / 96.0;
    int button_diameter_physical = ceil(scaling_factor * BUTTON_DIAMETER);
    DEBUG("scaling_factor is %.f, physical diameter is %d px\n",
          scaling_factor, button_diameter_physical);

    if (!vistype)
        vistype = get_root_visual_type(screen);
    bg_pixmap = create_bg_pixmap(conn, screen, resolution, color);
    /* Initialize cairo: Create one in-memory surface to render the unlock
     * indicator on, create one XCB surface to actually draw (one or more,
     * depending on the amount of screens) unlock indicators on. */
    cairo_surface_t *output = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, button_diameter_physical, button_diameter_physical);
    cairo_t *ctx = cairo_create(output);

    cairo_surface_t *xcb_output = cairo_xcb_surface_create(conn, bg_pixmap, vistype, resolution[0], resolution[1]);
    cairo_t *xcb_ctx = cairo_create(xcb_output);

    if (img) {
        if (!tile) {
            if (img_centered) {
                /* Composite the image in the middle of each screen. */
                for (int screen = 0; screen < xr_screens; screen++) {
                    int x = (xr_resolutions[screen].x + ((xr_resolutions[screen].width / 2) - (cairo_image_surface_get_width(img) / 2)));
                    int y = (xr_resolutions[screen].y + ((xr_resolutions[screen].height / 2) - (cairo_image_surface_get_height(img) / 2)));
                    cairo_set_source_surface(xcb_ctx, img, x, y);
                    cairo_paint(xcb_ctx);
                }
            } else {
                cairo_set_source_surface(xcb_ctx, img, 0, 0);
                cairo_paint(xcb_ctx);
            }
        } else {
            /* create a pattern and fill a rectangle as big as the screen */
            cairo_pattern_t *pattern;
            pattern = cairo_pattern_create_for_surface(img);
            cairo_set_source(xcb_ctx, pattern);
            cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
            cairo_rectangle(xcb_ctx, 0, 0, resolution[0], resolution[1]);
            cairo_fill(xcb_ctx);
            cairo_pattern_destroy(pattern);
        }
    } else {
        char strgroups[3][3] = {{color[0], color[1], '\0'},
                                {color[2], color[3], '\0'},
                                {color[4], color[5], '\0'}};
        uint32_t rgb16[3] = {(strtol(strgroups[0], NULL, 16)),
                             (strtol(strgroups[1], NULL, 16)),
                             (strtol(strgroups[2], NULL, 16))};
        cairo_set_source_rgb(xcb_ctx, rgb16[0] / 255.0, rgb16[1] / 255.0, rgb16[2] / 255.0);
        cairo_rectangle(xcb_ctx, 0, 0, resolution[0], resolution[1]);
        cairo_fill(xcb_ctx);
    }

    if (unlock_indicator &&
        (unlock_state >= STATE_KEY_PRESSED || auth_state > STATE_AUTH_IDLE)) {
        cairo_scale(ctx, scaling_factor, scaling_factor);
        /* Draw a (centered) circle with transparent background. */
        cairo_set_line_width(ctx, 10.0);
        cairo_arc(ctx,
                  BUTTON_CENTER /* x */,
                  BUTTON_CENTER /* y */,
                  BUTTON_RADIUS /* radius */,
                  0 /* start */,
                  2 * M_PI /* end */);

        /* Use the appropriate color for the different auth states
         * (currently verifying, wrong password, or default) */
#ifdef FILL_CIRCLE
        switch (auth_state) {
            case STATE_AUTH_VERIFY:
                cairo_set_source_rgba(ctx, FILL_AUTH_VERIFY_R/255 , FILL_AUTH_VERIFY_G/255, FILL_AUTH_VERIFY_B/255, FILL_AUTH_VERIFY_A);
                break;
            case STATE_AUTH_LOCK:
                cairo_set_source_rgba(ctx, FILL_AUTH_LOCK_R/255 , FILL_AUTH_LOCK_G/255, FILL_AUTH_LOCK_B/255, FILL_AUTH_LOCK_A);
                break;
            case STATE_AUTH_WRONG:
                cairo_set_source_rgba(ctx, FILL_AUTH_WRONG_R/255 , FILL_AUTH_WRONG_G/255, FILL_AUTH_WRONG_B/255, FILL_AUTH_WRONG_A);
                break;
            case STATE_I3LOCK_LOCK_FAILED:
                cairo_set_source_rgba(ctx, FILL_LOCK_FAILED_R/255 , FILL_LOCK_FAILED_G/255, FILL_LOCK_FAILED_B/255, FILL_LOCK_FAILED_A);
                break;
            default:
                if (unlock_state == STATE_NOTHING_TO_DELETE) {
                    cairo_set_source_rgba(ctx, FILL_NOTHING_TO_DELETE_R/255 , FILL_NOTHING_TO_DELETE_G/255, FILL_NOTHING_TO_DELETE_B/255, FILL_NOTHING_TO_DELETE_A);
                    break;
                }
                cairo_set_source_rgba(ctx, FILL_DEFAULT_R/255 , FILL_DEFAULT_G/255, FILL_DEFAULT_B/255, FILL_DEFAULT_A);
                break;
        }
        cairo_fill_preserve(ctx);
#endif

        switch (auth_state) {
            case STATE_AUTH_VERIFY:
                cairo_set_source_rgb(ctx, STROKE_AUTH_VERIFY_R/255, STROKE_AUTH_VERIFY_G/255, STROKE_AUTH_VERIFY_B/255);
                break;
            case STATE_AUTH_LOCK:
                cairo_set_source_rgb(ctx, STROKE_AUTH_LOCK_R/255, STROKE_AUTH_LOCK_G/255, STROKE_AUTH_LOCK_B/255);
                break;
            case STATE_AUTH_WRONG:
                cairo_set_source_rgb(ctx, STROKE_AUTH_WRONG_R/255, STROKE_AUTH_WRONG_G/255, STROKE_AUTH_WRONG_B/255);
                break;
            case STATE_I3LOCK_LOCK_FAILED:
                cairo_set_source_rgb(ctx, STROKE_LOCK_FAILED_R/255, STROKE_LOCK_FAILED_G/255, STROKE_LOCK_FAILED_B/255);
                break;
            case STATE_AUTH_IDLE:
                if (unlock_state == STATE_NOTHING_TO_DELETE) {
                    cairo_set_source_rgb(ctx, STROKE_NOTHING_TO_DELETE_R/255, STROKE_NOTHING_TO_DELETE_G/255, STROKE_NOTHING_TO_DELETE_B/255);
                    break;
                }
                cairo_set_source_rgb(ctx, STROKE_AUTH_IDLE_R/255, STROKE_AUTH_IDLE_G/255, STROKE_AUTH_IDLE_B/255);
                break;
        }
        cairo_stroke(ctx);

        /* Draw an inner seperator line. */
#ifdef DRAW_SEPERATOR
        cairo_set_source_rgb(ctx, FILL_SEPERATOR_R, FILL_SEPERATOR_G, FILL_SEPERATOR_B);
        cairo_set_line_width(ctx, FILL_SEPERATOR_WIDTH);
        cairo_arc(ctx,
                  BUTTON_CENTER /* x */,
                  BUTTON_CENTER /* y */,
                  BUTTON_RADIUS - 5 /* radius */,
                  0,
                  2 * M_PI);
        cairo_stroke(ctx);
#endif

        cairo_set_line_width(ctx, 10.0);

        /* Display a (centered) text of the current auth state. */
        char *text = NULL;
        /* We don't want to show more than a 3-digit number. */
        char buf[4];

        cairo_set_source_rgb(ctx, TEXT_R, TEXT_G, TEXT_B);
        cairo_select_font_face(ctx, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(ctx, TEXT_SIZE);
        switch (auth_state) {
#ifdef DRAW_TEXT_AUTH_VERIFY
            case STATE_AUTH_VERIFY:
                text = "Verifying…";
                break;
#endif
#ifdef DRAW_TEXT_AUTH_LOCK
            case STATE_AUTH_LOCK:
                text = "Locking…";
                break;
#endif
#ifdef DRAW_TEXT_AUTH_WRONG
            case STATE_AUTH_WRONG:
                text = "Wrong!";
                break;
#endif
#ifdef DRAW_TEXT_LOCK_FAILED
            case STATE_I3LOCK_LOCK_FAILED:
                text = "Lock failed!";
                break;
#endif
            default:
#ifdef DRAW_TEXT_NOTHING_TO_DELETE
                if (unlock_state == STATE_NOTHING_TO_DELETE) {
                    text = "No input";
                }
#endif
                if (show_failed_attempts && failed_attempts > 0) {
                    if (failed_attempts > 999) {
                        text = "> 999";
                    } else {
                        snprintf(buf, sizeof(buf), "%d", failed_attempts);
                        text = buf;
                    }
                    cairo_set_source_rgb(ctx, TEXT_ATTEMPTS_R/255, TEXT_ATTEMPTS_G/255, TEXT_ATTEMPTS_B/255);
                    cairo_set_font_size(ctx, TEXT_ATTEMPTS_SIZE);
                }
                break;
        }

        if (text) {
            cairo_text_extents_t extents;
            double x, y;

            cairo_text_extents(ctx, text, &extents);
            x = BUTTON_CENTER - ((extents.width / 2) + extents.x_bearing);
            y = BUTTON_CENTER - ((extents.height / 2) + extents.y_bearing);

            cairo_move_to(ctx, x, y);
            cairo_show_text(ctx, text);
            cairo_close_path(ctx);
        }

        if (auth_state == STATE_AUTH_WRONG && (modifier_string != NULL)) {
            cairo_text_extents_t extents;
            double x, y;

            cairo_set_font_size(ctx, 14.0);

            cairo_text_extents(ctx, modifier_string, &extents);
            x = BUTTON_CENTER - ((extents.width / 2) + extents.x_bearing);
            y = BUTTON_CENTER - ((extents.height / 2) + extents.y_bearing) + 28.0;

            cairo_move_to(ctx, x, y);
            cairo_show_text(ctx, modifier_string);
            cairo_close_path(ctx);
        }

        /* After the user pressed any valid key or the backspace key, we
         * highlight a random part of the unlock indicator to confirm this
         * keypress. */
        if (unlock_state == STATE_KEY_ACTIVE ||
            unlock_state == STATE_BACKSPACE_ACTIVE) {
            cairo_new_sub_path(ctx);
            double highlight_start = (rand() % (int)(2 * M_PI * 100)) / 100.0;
            cairo_arc(ctx,
                      BUTTON_CENTER /* x */,
                      BUTTON_CENTER /* y */,
                      BUTTON_RADIUS /* radius */,
                      highlight_start,
                      highlight_start + (M_PI / 3.0));
            if (unlock_state == STATE_KEY_ACTIVE) {
                /* For normal keys, we use a lighter green. */
                cairo_set_source_rgb(ctx, BUTTON_TYPE_R/255, BUTTON_TYPE_G/255, BUTTON_TYPE_B/255);
            } else {
                /* For backspace, we use red. */
                cairo_set_source_rgb(ctx, BUTTON_DELETE_R/255, BUTTON_DELETE_G/255, BUTTON_DELETE_B/255);
            }
            cairo_stroke(ctx);

            /* Draw two little separators for the highlighted part of the
             * unlock indicator. */
            cairo_set_source_rgb(ctx, BUTTON_SEPERATOR_R, BUTTON_SEPERATOR_G, BUTTON_SEPERATOR_B);
            cairo_arc(ctx,
                      BUTTON_CENTER /* x */,
                      BUTTON_CENTER /* y */,
                      BUTTON_RADIUS /* radius */,
                      highlight_start /* start */,
                      highlight_start + (M_PI / 128.0) /* end */);
            cairo_stroke(ctx);
            cairo_arc(ctx,
                      BUTTON_CENTER /* x */,
                      BUTTON_CENTER /* y */,
                      BUTTON_RADIUS /* radius */,
                      (highlight_start + (M_PI / 3.0)) - (M_PI / 128.0) /* start */,
                      highlight_start + (M_PI / 3.0) /* end */);
            cairo_stroke(ctx);
        }
    }

    if (xr_screens > 0) {
        /* Composite the unlock indicator in the middle of each screen. */
        for (int screen = 0; screen < xr_screens; screen++) {
            int x = (xr_resolutions[screen].x + ((xr_resolutions[screen].width / 2) - (button_diameter_physical / 2)));
            int y = (xr_resolutions[screen].y + ((xr_resolutions[screen].height / 2) - (button_diameter_physical / 2)));
            cairo_set_source_surface(xcb_ctx, output, x, y);
            cairo_rectangle(xcb_ctx, x, y, button_diameter_physical, button_diameter_physical);
            cairo_fill(xcb_ctx);
        }
    } else {
        /* We have no information about the screen sizes/positions, so we just
         * place the unlock indicator in the middle of the X root window and
         * hope for the best. */
        int x = (last_resolution[0] / 2) - (button_diameter_physical / 2);
        int y = (last_resolution[1] / 2) - (button_diameter_physical / 2);
        cairo_set_source_surface(xcb_ctx, output, x, y);
        cairo_rectangle(xcb_ctx, x, y, button_diameter_physical, button_diameter_physical);
        cairo_fill(xcb_ctx);
    }

    cairo_surface_destroy(xcb_output);
    cairo_surface_destroy(output);
    cairo_destroy(ctx);
    cairo_destroy(xcb_ctx);
    return bg_pixmap;
}

/*
 * Calls draw_image on a new pixmap and swaps that with the current pixmap
 *
 */
void redraw_screen(void) {
    DEBUG("redraw_screen(unlock_state = %d, auth_state = %d)\n", unlock_state, auth_state);
    xcb_pixmap_t bg_pixmap = draw_image(last_resolution);
    xcb_change_window_attributes(conn, win, XCB_CW_BACK_PIXMAP, (uint32_t[1]){bg_pixmap});
    /* XXX: Possible optimization: Only update the area in the middle of the
     * screen instead of the whole screen. */
    xcb_clear_area(conn, 0, win, 0, 0, last_resolution[0], last_resolution[1]);
    xcb_free_pixmap(conn, bg_pixmap);
    xcb_flush(conn);
}

/*
 * Hides the unlock indicator completely when there is no content in the
 * password buffer.
 *
 */
void clear_indicator(void) {
    if (input_position == 0) {
        unlock_state = STATE_STARTED;
    } else
        unlock_state = STATE_KEY_PRESSED;
    redraw_screen();
}
