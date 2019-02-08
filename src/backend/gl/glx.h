// SPDX-License-Identifier: MPL-2.0
// Copyright (c) Yuxuan Shui <yshuiv7@gmail.com>
#pragma once
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <xcb/xcb.h>
#include <xcb/render.h>

#include "log.h"
#include "compiler.h"
#include "utils.h"
#include "x.h"

struct glx_fbconfig_info {
	GLXFBConfig cfg;
	int texture_tgts;
	int texture_fmt;
	int y_inverted;
};

/// The search criteria for glx_find_fbconfig
struct glx_fbconfig_criteria {
	/// Bit width of the red component
	int red_size;
	/// Bit width of the green component
	int green_size;
	/// Bit width of the blue component
	int blue_size;
	/// Bit width of the alpha component
	int alpha_size;
	/// The depth of X visual
	int visual_depth;
};

struct glx_fbconfig_info *glx_find_fbconfig(Display *, int screen, struct glx_fbconfig_criteria);

/// Generate a search criteria for fbconfig from a X visual.
/// Returns {-1, -1, -1, -1, -1} on failure
static inline struct glx_fbconfig_criteria
x_visual_to_fbconfig_criteria(xcb_connection_t *c, xcb_visualid_t visual) {
	auto pictfmt = x_get_pictform_for_visual(c, visual);
	auto depth = x_get_visual_depth(c, visual);
	if (!pictfmt || depth == -1) {
		log_error("Invalid visual %#03x", visual);
		return (struct glx_fbconfig_criteria){-1, -1, -1, -1, -1};
	}
	if (pictfmt->type != XCB_RENDER_PICT_TYPE_DIRECT) {
		log_error("compton cannot handle non-DirectColor visuals. Report an "
		          "issue if you see this error message.");
		return (struct glx_fbconfig_criteria){-1, -1, -1, -1, -1};
	}

	int red_size = popcountl(pictfmt->direct.red_mask),
	    blue_size = popcountl(pictfmt->direct.blue_mask),
	    green_size = popcountl(pictfmt->direct.green_mask),
	    alpha_size = popcountl(pictfmt->direct.alpha_mask);

	return (struct glx_fbconfig_criteria){
	    .red_size = red_size,
	    .green_size = green_size,
	    .blue_size = blue_size,
	    .alpha_size = alpha_size,
	    .visual_depth = depth,
	};
}

/**
 * Check if a GLX extension exists.
 */
static inline bool glx_has_extension(Display *dpy, int screen, const char *ext) {
	const char *glx_exts = glXQueryExtensionsString(dpy, screen);
	if (!glx_exts) {
		log_error("Failed get GLX extension list.");
		return false;
	}

	long inlen = strlen(ext);
	const char *curr = glx_exts;
	bool match = false;
	while (curr && !match) {
		const char *end = strchr(curr, ' ');
		if (!end) {
			// Last extension string
			match = strcmp(ext, curr) == 0;
		} else if (end - curr == inlen) {
			// Length match, do match string
			match = strncmp(ext, curr, end - curr) == 0;
		}
		curr = end ? end + 1 : NULL;
	}

	if (!match) {
		log_info("Missing GLX extension %s.", ext);
	} else {
		log_info("Found GLX extension %s.", ext);
	}

	return match;
}