/*
 * clock.c
 *
 * Babeltrace CTF Writer
 *
 * Copyright 2013 EfficiOS Inc. and Linux Foundation
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

#include <babeltrace/ctf-writer/clock.h>
#include <babeltrace/ctf-writer/clock-internal.h>
#include <babeltrace/compiler.h>

static void bt_ctf_clock_destroy(struct bt_ctf_ref *ref);

struct bt_ctf_clock *bt_ctf_clock_create(const char *name)
{
	struct bt_ctf_clock *clock = NULL;
	if (!name || !name[0]) {
		/* A name is mandatory and "\0" is not a valid name */
		goto error;
	}
	clock = g_new0(struct bt_ctf_clock, 1);
	if (!clock) {
		goto error;
	}
	clock->name = g_string_new(name);
	if (!clock->name) {
		goto error_destroy;
	}
	clock->precision = 1;
	bt_ctf_ref_init(&clock->ref_count, bt_ctf_clock_destroy);
	return clock;

error_destroy:
	bt_ctf_clock_destroy(&clock->ref_count);
error:
	clock = NULL;
	return clock;
}

const char *bt_ctf_clock_get_name(struct bt_ctf_clock *clock)
{
	assert(clock && clock->name);
	return clock->name->str;
}

const char *bt_ctf_clock_get_description(struct bt_ctf_clock *clock)
{
	assert(clock && clock->description);
	return clock->description->str;
}

int bt_ctf_clock_set_description(struct bt_ctf_clock *clock, const char *desc)
{
	int ret = -1;
	if (!clock || !desc) {
		goto end;
	}
	clock->description = g_string_new(desc);
	ret = clock->description ? 0 : ret;
end:
	return ret;
}

uint64_t bt_ctf_clock_get_frequency(struct bt_ctf_clock *clock)
{
	return clock ? clock->frequency : 0;
}

int bt_ctf_clock_set_frequency(struct bt_ctf_clock *clock, uint64_t freq)
{
	int ret = 0;
	if (!clock) {
		return -1;
	}
	clock->frequency = freq;
	return ret;
}

uint64_t bt_ctf_clock_get_precision(struct bt_ctf_clock *clock)
{
	return clock ? clock->precision : 0;
}

int bt_ctf_clock_set_precision(struct bt_ctf_clock *clock, uint64_t precision)
{
	int ret = 0;
	if (!clock) {
		return -1;
	}
	clock->precision = precision;
	return ret;
}

uint64_t bt_ctf_clock_get_offset_s(struct bt_ctf_clock *clock)
{
	return clock ? clock->offset_s : 0;
}

int bt_ctf_clock_set_offset_s(struct bt_ctf_clock *clock, uint64_t offset_s)
{
	int ret = 0;
	if (!clock) {
		return -1;
	}
	clock->offset_s = offset_s;
	return ret;
}

uint64_t bt_ctf_clock_get_offset(struct bt_ctf_clock *clock)
{
	return clock ? clock->offset : 0;
}

int bt_ctf_clock_set_offset(struct bt_ctf_clock *clock, uint64_t offset)
{
	int ret = 0;
	if (!clock) {
		return -1;
	}
	clock->offset = offset;
	return ret;
}

int bt_ctf_clock_is_absolute(struct bt_ctf_clock *clock)
{
	return clock ? clock->is_absolute : -1;
}

int bt_ctf_clock_set_is_absolute(struct bt_ctf_clock *clock, int is_absolute)
{
	int ret = 0;
	if (!clock) {
		return -1;
	}
	clock->is_absolute = is_absolute ? 1 : 0;
	return ret;
}

int bt_ctf_clock_set_time(struct bt_ctf_clock *clock, uint64_t time)
{
	int ret = 0;
	if (!clock || time < clock->time) {
		return -1;
	}
	clock->time = time;
	return ret;
}

void bt_ctf_clock_get(struct bt_ctf_clock *clock)
{
	if (!clock) {
		return;
	}
	bt_ctf_ref_get(&clock->ref_count);
}

void bt_ctf_clock_put(struct bt_ctf_clock *clock)
{
	if (!clock) {
		return;
	}
	bt_ctf_ref_put(&clock->ref_count);
}

static void bt_ctf_clock_destroy(struct bt_ctf_ref *ref)
{
	if (!ref) {
		return;
	}
	struct bt_ctf_clock *clock = container_of(ref, struct bt_ctf_clock,
		ref_count);
	if (clock->name) {
		g_string_free(clock->name, TRUE);
	}
	if (clock->description) {
		g_string_free(clock->description, TRUE);
	}
	g_free(clock);
}
