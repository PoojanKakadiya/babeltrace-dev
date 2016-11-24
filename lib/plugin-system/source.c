/*
 * source.c
 *
 * Babeltrace Source Component
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

#include <babeltrace/ref.h>
#include <babeltrace/compiler.h>
#include <babeltrace/plugin/source-internal.h>
#include <babeltrace/plugin/component-internal.h>
#include <babeltrace/plugin/notification/iterator.h>
#include <babeltrace/plugin/notification/iterator-internal.h>

BT_HIDDEN
enum bt_component_status bt_component_source_validate(
		struct bt_component *component)
{
	enum bt_component_status ret = BT_COMPONENT_STATUS_OK;
	struct bt_component_source *source;

	if (!component) {
		ret = BT_COMPONENT_STATUS_INVALID;
		goto end;
	}

	if (!component->class) {
		ret = BT_COMPONENT_STATUS_INVALID;
		goto end;
	}

	if (component->class->type != BT_COMPONENT_TYPE_SOURCE) {
		ret = BT_COMPONENT_STATUS_INVALID;
		goto end;
	}

	source = container_of(component, struct bt_component_source, parent);
	if (!source->init_iterator) {
		ret = BT_COMPONENT_STATUS_INVALID;
		goto end;
	}
end:
	return ret;
}

BT_HIDDEN
struct bt_component *bt_component_source_create(
		struct bt_component_class *class, struct bt_value *params)
{
	struct bt_component_source *source = NULL;
	enum bt_component_status ret;

	source = g_new0(struct bt_component_source, 1);
	if (!source) {
		goto end;
	}

	source->parent.class = bt_get(class);
	ret = bt_component_init(&source->parent, NULL);
	if (ret != BT_COMPONENT_STATUS_OK) {
		BT_PUT(source);
		goto end;
	}
end:
	return source ? &source->parent : NULL;
}

enum bt_component_status
bt_component_source_set_iterator_init_cb(struct bt_component *component,
		bt_component_source_init_iterator_cb init_iterator)
{
	struct bt_component_source *source;
	enum bt_component_status ret = BT_COMPONENT_STATUS_OK;

	if (component->class->type != BT_COMPONENT_TYPE_SOURCE ||
			!component->initializing) {
		ret = BT_COMPONENT_STATUS_INVALID;
		goto end;
	}

	source = container_of(component, struct bt_component_source, parent);
	source->init_iterator = init_iterator;
end:
	return ret;
}

struct bt_notification_iterator *bt_component_source_create_iterator(
		struct bt_component *component)
{
	return bt_component_create_iterator(component);
}
