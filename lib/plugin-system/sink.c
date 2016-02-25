/*
 * sink.c
 *
 * Babeltrace Sink Component
 *
 * Copyright 2015 Jérémie Galarneau <jeremie.galarneau@efficios.com>
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

#include <babeltrace/compiler.h>
#include <babeltrace/plugin/sink-internal.h>
#include <babeltrace/plugin/component-internal.h>
#include <babeltrace/plugin/notification/notification.h>

BT_HIDDEN
enum bt_component_status bt_component_sink_validate(
		struct bt_component *component)
{
	enum bt_component_status ret = BT_COMPONENT_STATUS_OK;
	struct bt_component_sink *sink;

	sink = container_of(component, struct bt_component_sink, parent);
	if (sink->registered_notifications_mask == 0) {
		/*
		 * A sink must be registered to at least one notification type.
		 */
		printf_error("Invalid sink component; not registered to any notification");
		ret = BT_COMPONENT_STATUS_INVALID;
		goto end;
	}

	if (!sink->handle_notification) {
		printf_error("Invalid sink component; no notification handling callback defined.");
		ret = BT_COMPONENT_STATUS_INVALID;
		goto end;
	}
end:
	return ret;
}

BT_HIDDEN
struct bt_component *bt_component_sink_create(
		struct bt_component_class *class, struct bt_value *params)
{
	struct bt_component_sink *sink = NULL;
	enum bt_component_status ret;

	sink = g_new0(struct bt_component_sink, 1);
	if (!sink) {
		goto end;
	}

	sink->parent.class = bt_get(class);
	ret = bt_component_init(&sink->parent, NULL);
	if (ret != BT_COMPONENT_STATUS_OK) {
		goto error;
	}

	ret = bt_component_sink_register_notification_type(&sink->parent,
		BT_NOTIFICATION_TYPE_EVENT);
	if (ret != BT_COMPONENT_STATUS_OK) {
		goto error;
	}
end:
	return sink ? &sink->parent : NULL;
error:
	BT_PUT(sink);
	return NULL;
}

enum bt_component_status bt_component_sink_handle_notification(
		struct bt_component *component,
		struct bt_notification *notification)
{
	enum bt_component_status ret = BT_COMPONENT_STATUS_OK;
	struct bt_component_sink *sink = NULL;

	if (!component || !notification) {
		ret = BT_COMPONENT_STATUS_INVALID;
		goto end;
	}

	if (bt_component_get_type(component) != BT_COMPONENT_TYPE_SINK) {
		ret = BT_COMPONENT_STATUS_UNSUPPORTED;
		goto end;
	}

	sink = container_of(component, struct bt_component_sink, parent);
	assert(sink->handle_notification);
	ret = sink->handle_notification(component, notification);
end:
	return ret;
}

enum bt_component_status bt_component_sink_register_notification_type(
		struct bt_component *component, enum bt_notification_type type)
{
	enum bt_component_status ret = BT_COMPONENT_STATUS_OK;
	struct bt_component_sink *sink = NULL;

	if (!component) {
		ret = BT_COMPONENT_STATUS_INVALID;
		goto end;
	}

	if (bt_component_get_type(component) != BT_COMPONENT_TYPE_SINK) {
		ret = BT_COMPONENT_STATUS_UNSUPPORTED;
		goto end;
	}

	if (type <= BT_NOTIFICATION_TYPE_UNKNOWN ||
		type >= BT_NOTIFICATION_TYPE_NR) {
		ret = BT_COMPONENT_STATUS_INVALID;
		goto end;
	}
	sink = container_of(component, struct bt_component_sink, parent);
	if (type == BT_NOTIFICATION_TYPE_ALL) {
		sink->registered_notifications_mask = ~(notification_mask_t) 0;
	} else {
		sink->registered_notifications_mask |=
			(notification_mask_t) 1 << type;
	}
end:
	return ret;
}
