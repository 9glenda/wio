#define _POSIX_C_SOURCE 200112L
#include <stdlib.h>
#include <time.h>
#include <wayland-server.h>
#include <wlr/types/wlr_matrix.h>
#include <wlr/types/wlr_output.h>
#include <wlr/render/wlr_renderer.h>
#include "colors.h"
#include "server.h"
#include "view.h"

struct render_data {
	struct wlr_output *output;
	struct wlr_renderer *renderer;
	struct wio_view *view;
	struct timespec *when;
};

static void render_surface(struct wlr_surface *surface,
		int sx, int sy, void *data) {
	struct render_data *rdata = data;
	struct wio_view *view = rdata->view;
	struct wlr_output *output = rdata->output;
	struct wlr_texture *texture = wlr_surface_get_texture(surface);
	if (texture == NULL) {
		return;
	}
	double ox = 0, oy = 0;
	wlr_output_layout_output_coords(
			view->server->output_layout, output, &ox, &oy);
	ox += view->x + sx, oy += view->y + sy;
	struct wlr_box box = {
		.x = ox * output->scale,
		.y = oy * output->scale,
		.width = surface->current.width * output->scale,
		.height = surface->current.height * output->scale,
	};
	float matrix[9];
	enum wl_output_transform transform =
		wlr_output_transform_invert(surface->current.transform);
	wlr_matrix_project_box(matrix, &box, transform, 0,
		output->transform_matrix);
	wlr_render_texture_with_matrix(rdata->renderer, texture, matrix, 1);
	wlr_surface_send_frame_done(surface, rdata->when);
}

static void output_frame(struct wl_listener *listener, void *data) {
	struct wio_output *output = wl_container_of(listener, output, frame);
	struct wlr_renderer *renderer = output->server->renderer;

	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);

	if (!wlr_output_make_current(output->wlr_output, NULL)) {
		return;
	}

	int width, height;
	wlr_output_effective_resolution(output->wlr_output, &width, &height);
	wlr_renderer_begin(renderer, width, height);

	wlr_renderer_clear(renderer, background);

	struct wio_view *view;
	wl_list_for_each_reverse(view, &output->server->views, link) {
		if (!view->xdg_surface->mapped) {
			continue;
		}
		struct render_data rdata = {
			.output = output->wlr_output,
			.view = view,
			.renderer = renderer,
			.when = &now,
		};
		wlr_xdg_surface_for_each_surface(view->xdg_surface,
				render_surface, &rdata);
	}

	wlr_renderer_end(renderer);
	wlr_output_swap_buffers(output->wlr_output, NULL, NULL);
}

void server_new_output(struct wl_listener *listener, void *data) {
	struct wio_server *server =
		wl_container_of(listener, server, new_output);
	struct wlr_output *wlr_output = data;

	if (!wl_list_empty(&wlr_output->modes)) {
		struct wlr_output_mode *mode =
			wl_container_of(wlr_output->modes.prev, mode, link);
		wlr_output_set_mode(wlr_output, mode);
	}

	struct wio_output *output = calloc(1, sizeof(struct wio_output));
	output->wlr_output = wlr_output;
	output->server = server;
	output->frame.notify = output_frame;
	wl_signal_add(&wlr_output->events.frame, &output->frame);
	wl_list_insert(&server->outputs, &output->link);

	wlr_output_layout_add_auto(server->output_layout, wlr_output);
	wlr_output_create_global(wlr_output);
}
