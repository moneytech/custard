#include "handlers.h"

#include "config.h"
#include "custard.h"
#include "grid.h"
#include "window.h"
#include "workspace.h"
#include "xcb.h"
#include "ewmh.h"
#include "utilities.h"

#include <string.h>
#include <xcb/xcb.h>

void
handlers_handle_event(xcb_generic_event_t *event)
{
    unsigned int event_type = event->response_type & ~0x80;

    debug_output("Event received");

    switch (event_type) {
        case (XCB_MAP_REQUEST):
            handlers_map_request(event);
            break;
        case (XCB_DESTROY_NOTIFY):
            handlers_window_destroyed(event);
            break;
        case (XCB_BUTTON_PRESS):
            handlers_button_press(event);
            break;
        case (XCB_CLIENT_MESSAGE):
            handlers_window_message_received(event);
            break;
        default:
            debug_output("Unhandled event %d\n", event_type);
            break;
    }

    /* TODO: create setting for sloppy focus */
}

void
handlers_map_request(xcb_generic_event_t *generic_event)
{
    debug_output("Called");

    xcb_map_request_event_t *event = (xcb_map_request_event_t *)generic_event;
    xcb_window_t window_id = event->window;

    if (window_id == ewmh_window_id) {
        return;
    }

    unsigned short int managed = manage_window(window_id);

    if (managed) {
        change_window_geometry_grid_coordinate(window_id,
            grid_window_default_x, grid_window_default_y,
            grid_window_default_height, grid_window_default_width);

        attach_window_to_workspace(window_id, focused_workspace);

        map_window(window_id);

        /* xcb: regex test the title */
        struct LinkedListElement *element = geometry_rules_list_head;

        if (element)
        {

            xcb_atom_t prop = XCB_ATOM_WM_NAME;
            xcb_atom_t type = XCB_GET_PROPERTY_TYPE_ANY;

            xcb_get_property_reply_t *reply = xcb_get_property_reply(
                xcb_connection,
                    xcb_get_property(xcb_connection, 0, window_id, prop,
                    type, 0, 256),
                NULL);

            char *expression = (char *)malloc(sizeof(char));
            char *window_title = (char *)xcb_get_property_value(reply);
            GeometryRule *rule;

            struct LinkedListElement *geometry_element = geometry_list_head;

            while (element)
            {
                rule = (GeometryRule *)element->data;

                strcpy(expression, rule->match);

                if (regex_match(window_title, expression))
                {
                    debug_output("Regex title match: %s %s\n", window_title, expression);

                    while (geometry_element)
                    {
                        if (strcmp(((Geometry *)geometry_element->data)->name,
                            rule->geometry) == 0)
                        {
                            change_window_geometry_grid_coordinate(window_id,
                                ((Geometry *)geometry_element->data)->x,
                                ((Geometry *)geometry_element->data)->y,
                                ((Geometry *)geometry_element->data)->height,
                                ((Geometry *)geometry_element->data)->width);
                            geometry_element = NULL;
                            continue;
                        }

                        geometry_element = geometry_element->next;
                    }
                }

                element = element->next;
            }

            free(reply);
        }

        /* end xcb test */

        focus_on_window(window_id);
        raise_window(window_id);
    } else {
        map_window(window_id);
        raise_window(window_id);
    }

    commit();
}

void
handlers_window_destroyed(xcb_generic_event_t *generic_event)
{
    debug_output("Called");

    xcb_destroy_notify_event_t *event;
    event = (xcb_destroy_notify_event_t *)generic_event;

    xcb_window_t window_id = event->window;

    if(window_list_get_window(window_id)) {
        unmanage_window(window_id);
        commit();
    }
}

void
handlers_button_press(xcb_generic_event_t *generic_event)
{
    debug_output("Called");

    xcb_button_press_event_t *event;
    event = (xcb_button_press_event_t *)generic_event;

    xcb_window_t window_id = event->event;

    focus_on_window(window_id);
    raise_window(window_id);
    commit();
}

void
handlers_window_message_received(xcb_generic_event_t *generic_event)
{
    debug_output("Called");

    xcb_client_message_event_t *event;
    event = (xcb_client_message_event_t *)generic_event;

    xcb_window_t window_id = event->window;

    if (event->type == ewmh_connection->_NET_WM_STATE) {
        xcb_atom_t atom = event->data.data32[1];
        unsigned int action = event->data.data32[0];

        if (atom == ewmh_connection->_NET_WM_STATE_FULLSCREEN) {

            if (action == XCB_EWMH_WM_STATE_ADD) {
                fullscreen(window_id);
            } else if (action == XCB_EWMH_WM_STATE_REMOVE) {
                window(window_id);
            } else if (action == XCB_EWMH_WM_STATE_TOGGLE) {
            }

        }

    } else if (event->type == ewmh_connection->_NET_CLOSE_WINDOW) {
        Window *window = window_list_get_window(window_id);

        if (window) {
            unmanage_window(window_id);
        }
    }

    commit();
}
