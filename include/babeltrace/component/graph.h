#ifndef BABELTRACE_COMPONENT_GRAPH_H
#define BABELTRACE_COMPONENT_GRAPH_H

/*
 * BabelTrace - Babeltrace Graph Interface
 *
 * Copyright 2017 Jérémie Galarneau <jeremie.galarneau@efficios.com>
 *
 * Author: Jérémie Galarneau <jeremie.galarneau@efficios.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <babeltrace/component/component.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_port;
struct bt_connection;

enum bt_graph_status {
	/** Downstream component does not support multiple inputs. */
	BT_GRAPH_STATUS_END = 1,
	BT_GRAPH_STATUS_OK = 0,
	/** Downstream component does not support multiple inputs. */
	BT_GRAPH_STATUS_MULTIPLE_INPUTS_UNSUPPORTED = -1,
	/** Component is already part of another graph. */
	BT_GRAPH_STATUS_ALREADY_IN_A_GRAPH = -2,
	/** Invalid arguments. */
	BT_GRAPH_STATUS_INVALID = -3,
	/** No sink in graph. */
	BT_GRAPH_STATUS_NO_SINK = -4,
	/** General error. */
	BT_GRAPH_STATUS_ERROR = -5,
	/** No sink can consume at the moment. */
	BT_GRAPH_STATUS_AGAIN = -6,
};

extern struct bt_graph *bt_graph_create(void);

/**
 * Creates a connection between two components using the two ports specified
 * and adds the connection and components (if not already added) to the graph.
 */
extern struct bt_connection *bt_graph_connect(struct bt_graph *graph,
		struct bt_port *upstream,
		struct bt_port *downstream);

/**
 * Add a component as a "sibling" of the origin component. Sibling share
 * connections equivalent to each other at the time of connection (same
 * upstream and downstream ports).
 */
extern enum bt_graph_status bt_graph_add_component_as_sibling(
		struct bt_graph *graph, struct bt_component *origin,
		struct bt_component *new_component);

/**
 * Run graph to completion or until a single sink is left and "AGAIN" is received.
 *
 * Runs "bt_component_sink_consume()" on all sinks in round-robin until they all
 * indicate that the end is reached or that an error occured.
 */
extern enum bt_graph_status bt_graph_run(struct bt_graph *graph,
		enum bt_component_status *component_status);

/**
 * Runs "bt_component_sink_consume()" on the graph's sinks. Each invokation will
 * invoke "bt_component_sink_consume()" on the next sink, in round-robin, until
 * they all indicated that the end is reached.
 */
extern enum bt_component_status bt_graph_consume(struct bt_graph *graph);

#ifdef __cplusplus
}
#endif

#endif /* BABELTRACE_COMPONENT_GRAPH_H */
