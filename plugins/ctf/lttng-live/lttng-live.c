/*
 * lttng-live.c
 *
 * Babeltrace CTF LTTng-live Client Component
 *
 * Copyright 2016 Jérémie Galarneau <jeremie.galarneau@efficios.com>
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

#include "lttng-live-internal.h"
#include <babeltrace/component/component-source.h>
#include <plugins-common.h>

BT_HIDDEN
struct bt_notification *lttng_live_iterator_get(
		struct bt_notification_iterator *iterator)
{
	return NULL;
}

BT_HIDDEN
enum bt_notification_iterator_status lttng_live_iterator_next(
		struct bt_notification_iterator *iterator)
{
	return BT_NOTIFICATION_ITERATOR_STATUS_OK;
}

BT_HIDDEN
enum bt_component_status lttng_live_init(struct bt_component *component,
		struct bt_value *params, UNUSED_VAR void *init_method_data)
{
	return BT_COMPONENT_STATUS_OK;
}
