#include "ipc.h"

#include "config.h"
#include "grid.h"
#include "utilities.h"
#include "window.h"
#include "workspace.h"
#include "xcb.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
process_command(char *input)
{
    debug_output("beginning of received input >>\n %s <<end of received input", input);

    unsigned int target = 0;
    unsigned int action = 0;
    char *arguments[8];

    char *token;
    unsigned int index = 0;

    token = strtok(input, ";");

    while (token) {
        if (index == 0) {
            target = parse_unsigned_integer(token);
        } else if (index == 1) {
            action = parse_unsigned_integer(token);
        } else {
            arguments[index - 2] = token;
        }

        token = NULL;
        if (++index < 10) {
            token = strtok(NULL, ";");
        }
    }

    if (!target || !action) {
        return;
    }

    debug_output("Input split");

    if (target == 0xEAC4D763) {
        /* target: custard */

        switch (action) {
            case 0x7C79369D: /* halt */
                wm_running = 0;
                break;

            case 0x3D2C748D: /* configure */

                if (strcmp("grid_rows", arguments[0]) == 0) {
                    grid_rows = parse_unsigned_integer(arguments[1]);
                } else if (strcmp("grid_columns", arguments[0]) == 0) {
                    grid_columns = parse_unsigned_integer(arguments[1]);
                } else if (strcmp("grid_gap", arguments[0]) == 0) {
                    grid_gap = parse_unsigned_integer(arguments[1]);
                } else if (strcmp("grid_margin_top", arguments[0]) == 0) {
                    grid_margin_top = parse_unsigned_integer(arguments[1]);
                } else if (strcmp("grid_margin_bottom", arguments[0]) == 0) {
                    grid_margin_bottom = parse_unsigned_integer(arguments[1]);
                } else if (strcmp("grid_margin_left", arguments[0]) == 0) {
                    grid_margin_left = parse_unsigned_integer(arguments[1]);
                } else if (strcmp("grid_margin_right", arguments[0]) == 0) {
                    grid_margin_right = parse_unsigned_integer(arguments[1]);
                } else if (strcmp("border_type", arguments[0]) == 0) {
                    border_type = parse_unsigned_integer(arguments[1]);

                    if (border_type > 3) {
                        border_type = 3;
                    }
                } else if (strcmp("border_inner_size", arguments[0]) == 0) {
                    border_inner_size = parse_unsigned_integer(arguments[1]);

                    if (border_type == 0) {
                        border_total_size = 0;
                    } else {
                        border_total_size = border_inner_size +
                            ((border_type - 1) * border_outer_size);
                    }
                } else if (strcmp("border_outer_size", arguments[0]) == 0) {
                    border_outer_size = parse_unsigned_integer(arguments[1]);

                    if (border_type == 0) {
                        border_total_size = 0;
                    } else {
                        border_total_size = border_inner_size +
                            ((border_type - 1) * border_outer_size);
                    }

                } else if (strcmp("border_focused_color", arguments[0]) == 0) {
                    border_focused_color = parse_rgba_color(arguments[1]);
                } else if (
                    strcmp("border_unfocused_color", arguments[0]) == 0) {
                    border_unfocused_color = parse_rgba_color(arguments[1]);
                } else if (
                    strcmp("border_background_color", arguments[0]) == 0) {
                    border_background_color = parse_rgba_color(arguments[1]);
                }

                /* Missing: border_invert_colors, workspaces */

                grid_apply_configuration();

                return; /* Don't unnecessarily commit to X */
                break;

            case 0xEC16043C: /* new_geometry */

                new_geometry(arguments[0],
                    parse_unsigned_integer(arguments[1]),
                    parse_unsigned_integer(arguments[2]),
                    parse_unsigned_integer(arguments[3]),
                    parse_unsigned_integer(arguments[4]));

                return;
                break;

            case 0xCD252BAD: /* new_geometry_rule */
                new_geometry_rule(
                    (window_attribute_t)parse_unsigned_integer(arguments[0]),
                    arguments[1],
                    arguments[2]);

                return;
                break;

            default:
                return;
        }

    } else if (target == 0x782DC389) {
        /* target: window */

        if (!focused_window) {
            return;
        }

        xcb_window_t window_id = focused_window->id;

        switch (action) {
            case 0xA8B6733:
                close_window(window_id);
                break;

            case 0xB9A8149:
                raise_window(window_id);
                break;

            case 0xA53CBC6:
                lower_window(window_id);
                break;

            case 0x5769B7BF: {
                    struct LinkedListElement *element = geometry_list_head;

                    if (!element) {
                        return;
                    }

                    while (element) {
                        debug_output("Testing geometry %s to %s.",
                            ((Geometry *)element->data)->name, arguments[0]);
                        if (strcmp(((Geometry *)element->data)->name,
                            arguments[0]) == 0) {

                            change_window_geometry_grid_coordinate(window_id,
                                ((Geometry *)element->data)->x,
                                ((Geometry *)element->data)->y,
                                ((Geometry *)element->data)->height,
                                ((Geometry *)element->data)->width);

                            border_update(window_id);

                            commit();
                            return;
                        }

                        element = element->next;
                    }
                }
                break;

            default:
                return;
        }

    } else if (target == 0x72FE3CE0) {
        /* target: workspace */

        unsigned int workspace = parse_unsigned_integer(arguments[0]);

        switch (action) {
            case 0xA33ED49:
                focus_on_workspace(workspace);
                break;

            default:
                return;
        }
    }

    commit();
}
