/*
 * test-ctf-writer.c
 *
 * CTF Writer test
 *
 * Copyright 2013 - Jérémie Galarneau <jeremie.galarneau@efficios.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; under version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#define _GNU_SOURCE
#include <babeltrace/ctf-writer/writer.h>
#include <babeltrace/ctf-writer/clock.h>
#include <babeltrace/ctf-writer/stream.h>
#include <babeltrace/ctf-writer/event.h>
#include <babeltrace/ctf-writer/event-types.h>
#include <babeltrace/ctf-writer/event-fields.h>
#include <babeltrace/ctf/events.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/utsname.h>
#include <limits.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>

#include "tap.h"

#define METADATA_LINE_SIZE 512

void validate_metadata(char *parser_path, char *metadata_path)
{
	int ret = 0;
	char parser_output_path[] = "/tmp/parser_output_XXXXXX";
	int parser_output_fd = -1, metadata_fd = -1;

	if (!metadata_path) {
		goto result;
	}

	parser_output_fd = mkstemp(parser_output_path);
	metadata_fd = open(metadata_path, O_RDONLY);

	unlink(parser_output_path);

	if (parser_output_fd == -1 || metadata_fd == -1) {
		printf("# Failed create temporary files for metadata parsing.\n");
		ret = -1;
		goto result;
	}

	pid_t pid = fork();
	if (pid) {
		int status = 0;
		waitpid(pid, &status, 0);
		ret = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
	} else {
		/* ctf-parser-test expects a metadata string on stdin. */
		ret = dup2(metadata_fd, STDIN_FILENO);
		if (ret < 0) {
			perror("# dup2 metadata_fd to STDIN");
			goto result;
		}

		ret = dup2(parser_output_fd, STDOUT_FILENO);
		if (ret < 0) {
			perror("# dup2 parser_output_fd to STDOUT");
			goto result;
		}

		ret = dup2(parser_output_fd, STDERR_FILENO);
		if (ret < 0) {
			perror("# dup2 parser_output_fd to STDERR");
			goto result;
		}

		execl(parser_path, "ctf-parser-test", NULL);
		perror("# Could not launch the ctf metadata parser process");
		exit(-1);
	}
result:
	ok(ret == 0, "Metadata string is valid");

	if (ret && metadata_fd > 0 && parser_output_fd > 0) {
		char *line;
		size_t len = METADATA_LINE_SIZE;
		FILE *metadata_fp = NULL, *parser_output_fp = NULL;

		metadata_fp = fdopen(metadata_fd, "r");
		if (!metadata_fp) {
			perror("fdopen on metadata_fd");
			goto close_fp;
		}

		parser_output_fp = fdopen(parser_output_fd, "r");
		if (!parser_output_fp) {
			perror("fdopen on parser_output_fd");
			goto close_fp;
		}

		line = malloc(len);
		rewind(metadata_fp);

		/* Output the metadata and parser output as diagnostic */
		while (getline(&line, &len, metadata_fp) > 0) {
			printf("# %s", line);
		}

		rewind(parser_output_fp);
		while (getline(&line, &len, parser_output_fp) > 0) {
			printf("# %s", line);
		}

		free(line);
	close_fp:
		fclose(metadata_fp);
		fclose(parser_output_fp);
	}

	close(parser_output_fd);
	close(metadata_fd);
}

void packet_resize_test(struct bt_ctf_stream_class *stream_class,
		struct bt_ctf_stream *stream)
{
	/*
	 * Push enough events to force the underlying packet to be resized.
	 * Also tests that a new event can be declared after a stream has been
	 * instanciated and used/flushed.
	 */
	int ret = 0;
	struct bt_ctf_event_class *event_class = bt_ctf_event_class_create(
		"Spammy_Event");
	struct bt_ctf_field_type *integer_type =
		bt_ctf_field_type_integer_create(17);

	ret = bt_ctf_event_class_add_field(event_class, integer_type,
		"field_1");
	ret = bt_ctf_stream_class_add_event_class(stream_class, event_class);
	ok(ret == 0, "Add a new event class to a stream class after writing an event");
	if (ret) {
		goto end;
	}

	for (unsigned int i = 0; i < 100000; i++) {
		struct bt_ctf_event *event = bt_ctf_event_create(event_class);
		struct bt_ctf_field *integer =
			bt_ctf_field_create(integer_type);

		ret |= bt_ctf_field_unsigned_integer_set_value(integer, i);
		ret |= bt_ctf_event_set_payload(event, "field_1",
			integer);
		bt_ctf_field_put(integer);
		ret |= bt_ctf_stream_push_event(stream, event);
		bt_ctf_event_put(event);

		if (ret) {
			break;
		}
	}
end:
	ok(ret == 0, "Push 100 000 events to a stream");
	ok(bt_ctf_stream_flush(stream) == 0, "Flush a stream that forces a packet resize");
	bt_ctf_field_type_put(integer_type);
	bt_ctf_event_class_put(event_class);
}

int main(int argc, char **argv)
{
	char trace_path[] = "/tmp/ctfwriter_XXXXXX";
	char metadata_path[sizeof(trace_path) + 9];

	if (argc < 2) {
		printf("Usage: tests-ctf-writer path_to_ctf_parser_test\n");
		exit(-1);
	}

	plan_no_plan();

	if (!mkdtemp(trace_path)) {
		perror("# perror");
	}
	strcpy(metadata_path, trace_path);
	strcat(metadata_path + sizeof(trace_path) - 1, "/metadata");

	struct bt_ctf_writer *writer =
		bt_ctf_writer_create(trace_path);
	ok(writer, "bt_ctf_create succeeds in creating trace with path");

	/* Add environment context to the trace */
	char hostname[HOST_NAME_MAX];
	gethostname(hostname, HOST_NAME_MAX);
	ok(bt_ctf_writer_add_environment_field(writer, "host", hostname) == 0,
		"Add host (%s) environment field to writer instance",
		hostname);
	ok(bt_ctf_writer_add_environment_field(NULL, "test_field",
		"test_value"),
		"bt_ctf_writer_add_environment_field error with NULL writer");
	ok(bt_ctf_writer_add_environment_field(writer, NULL,
		"test_value"),
		"bt_ctf_writer_add_environment_field error with NULL field name");
	ok(bt_ctf_writer_add_environment_field(writer, "test_field",
		NULL),
		"bt_ctf_writer_add_environment_field error with NULL field value");

	struct utsname *name = malloc(sizeof(struct utsname));
	if (uname(name)) {
		perror("uname");
		return -1;
	}

	ok(bt_ctf_writer_add_environment_field(writer, "sysname", name->sysname)
		== 0, "Add sysname (%s) environment field to writer instance",
		name->sysname);
	ok(bt_ctf_writer_add_environment_field(writer, "nodename",
		name->nodename) == 0,
		"Add nodename (%s) environment field to writer instance",
		name->nodename);
	ok(bt_ctf_writer_add_environment_field(writer, "release", name->release)
		== 0, "Add release (%s) environment field to writer instance",
		name->release);
	ok(bt_ctf_writer_add_environment_field(writer, "version", name->version)
		== 0, "Add version (%s) environment field to writer instance",
		name->version);
	ok(bt_ctf_writer_add_environment_field(writer, "machine", name->machine)
		== 0, "Add machine (%s) environment field to writer istance",
		name->machine);
	free(name);

	/* Define a clock and add it to the trace */
	const char *clock_name = "test_clock";
	const char *clock_description = "This is a test clock";

	ok(bt_ctf_clock_create("signed") == NULL, "Illegal clock name rejected");
	struct bt_ctf_clock *clock = bt_ctf_clock_create(clock_name);
	ok(clock, "Clock created sucessfully");
	bt_ctf_clock_set_description(clock, clock_description);

	const uint64_t frequency = 1000000000;
	const uint64_t offset_s = 1351530929945824323;
	const uint64_t offset = 1234567;
	const uint64_t precision = 10;

	ok(bt_ctf_clock_set_frequency(clock, frequency) == 0,
		"Set clock frequency");
	ok(bt_ctf_clock_set_offset_s(clock, offset_s) == 0,
		"Set clock offset (seconds)");
	ok(bt_ctf_clock_set_offset(clock, offset) == 0, "Set clock offset");
	ok(bt_ctf_clock_set_precision(clock, precision) == 0,
		"Set clock precision");
	ok(bt_ctf_clock_set_is_absolute(clock, 0xFF) == 0,
		"Set clock absolute property");

	ok(bt_ctf_writer_add_clock(writer, clock) == 0,
		"Add clock to writer instance");
	ok(bt_ctf_writer_add_clock(writer, clock),
		"Verify a clock can't be added twice to a writer instance");

	/* Define a stream class and field types */
	struct bt_ctf_stream_class *stream_class = bt_ctf_stream_class_create("test_stream");
	ok(stream_class, "Create stream class");
	ok(bt_ctf_stream_class_set_clock(stream_class, clock) == 0,
		"Set a stream class' clock");

	struct bt_ctf_field_type *uint_12_type =
		bt_ctf_field_type_integer_create(12);
	ok(uint_12_type, "Create an unsigned integer type");
	ok(bt_ctf_field_type_integer_set_base(uint_12_type,
		BT_CTF_INTEGER_BASE_BINARY) == 0,
		"Set integer type's base as binary");
	ok(bt_ctf_field_type_integer_set_base(uint_12_type,
		BT_CTF_INTEGER_BASE_DECIMAL) == 0,
		"Set integer type's base as decimal");
	ok(bt_ctf_field_type_integer_set_base(uint_12_type,
		BT_CTF_INTEGER_BASE_UNKNOWN),
		"Reject integer type's base set as unknown");
	ok(bt_ctf_field_type_integer_set_base(uint_12_type,
		BT_CTF_INTEGER_BASE_OCTAL) == 0,
		"Set integer type's base as octal");
	ok(bt_ctf_field_type_integer_set_base(uint_12_type,
		BT_CTF_INTEGER_BASE_HEXADECIMAL) == 0,
		"Set integer type's base as hexadecimal");
	ok(bt_ctf_field_type_integer_set_base(uint_12_type, 457417),
		"Reject unknown integer base value");
	ok(bt_ctf_field_type_integer_set_signed(uint_12_type, 952835) == 0,
		"Set integer type signedness to signed");
	ok(bt_ctf_field_type_integer_set_signed(uint_12_type, 0) == 0,
		"Set integer type signedness to unsigned");

	struct bt_ctf_field_type *int_16_type =
		bt_ctf_field_type_integer_create(16);
	bt_ctf_field_type_integer_set_signed(int_16_type, 1);
	struct bt_ctf_field_type *uint_8_type =
		bt_ctf_field_type_integer_create(8);
	struct bt_ctf_field_type *sequence_type =
		bt_ctf_field_type_sequence_create(int_16_type, "seq_len");
	ok(sequence_type, "Create a sequence of int16_t type");

	struct bt_ctf_field_type *string_type =
		bt_ctf_field_type_string_create();
	ok(string_type, "Create a string type");
	ok(bt_ctf_field_type_string_set_encoding(string_type,
		CTF_STRING_NONE),
		"Reject invalid \"None\" string encoding");
	ok(bt_ctf_field_type_string_set_encoding(string_type,
		42),
		"Reject invalid string encoding");
	ok(bt_ctf_field_type_string_set_encoding(string_type,
		CTF_STRING_ASCII) == 0,
		"Set string encoding to ASCII");
	struct bt_ctf_field_type *structure_seq_type =
		bt_ctf_field_type_structure_create();
	ok(structure_seq_type, "Create a structure type");
	ok(bt_ctf_field_type_structure_add_field(structure_seq_type,
		uint_8_type, "seq_len") == 0,
		"Add a uint8_t type to a structure");
	ok(bt_ctf_field_type_structure_add_field(structure_seq_type,
		sequence_type, "a_sequence") == 0,
		"Add a sequence type to a structure");
	struct bt_ctf_field_type *composite_structure_type =
		bt_ctf_field_type_structure_create();
	ok(bt_ctf_field_type_structure_add_field(composite_structure_type,
		string_type, "a_string") == 0,
		"Add a string type to a structure");
	ok(bt_ctf_field_type_structure_add_field(composite_structure_type,
		structure_seq_type, "inner_structure") == 0,
		"Add a structure type to a structure");

	ok(bt_ctf_event_class_create("clock") == NULL,
		"Reject creation of an event class with an illegal name");
	struct bt_ctf_event_class *event_class =
		bt_ctf_event_class_create("A Test Event");
	ok(event_class, "Create an event class");
	ok(bt_ctf_event_class_add_field(event_class, uint_12_type, ""),
		"Reject addition of a field with an empty name to an event");
	ok(bt_ctf_event_class_add_field(event_class, NULL, "an_integer"),
		"Reject addition of a field with a NULL type to an event");
	ok(bt_ctf_event_class_add_field(event_class, uint_12_type,
		"int"),
		"Reject addition of a type with an illegal name to an event");
	ok(bt_ctf_event_class_add_field(event_class, uint_12_type,
		"uint_12") == 0,
		"Add field of type unsigned integer to an event");
	ok(bt_ctf_event_class_add_field(event_class, int_16_type,
		"int_16") == 0, "Add field of type signed integer to an event");
	ok(bt_ctf_event_class_add_field(event_class, composite_structure_type,
		"complex_structure") == 0,
		"Add composite structure to an event");

	/* Add event class to the stream class */
	ok(bt_ctf_stream_class_add_event_class(stream_class, NULL),
		"Reject addition of NULL event class to a stream class");
	ok(bt_ctf_stream_class_add_event_class(stream_class,
		event_class) == 0, "Add an event class to stream class");

	/* Create and add a simple event class */
	struct bt_ctf_event_class *simple_event_class =
		bt_ctf_event_class_create("Simple Event");
	bt_ctf_event_class_add_field(simple_event_class, uint_12_type,
		"integer_field");
	bt_ctf_stream_class_add_event_class(stream_class, simple_event_class);

	/* Instanciate a stream and an event */
	struct bt_ctf_stream *stream1 = bt_ctf_writer_create_stream(writer,
		stream_class);
	ok(stream1, "Instanciate a stream class from writer");
	/* Should fail after instanciating a stream (locked)*/
	ok(bt_ctf_stream_class_set_clock(stream_class, clock),
		"Changes to a stream class that was already instanciated fail");

	struct bt_ctf_event *event =
		bt_ctf_event_create(event_class);
	ok(event, "Instanciate an event class");

	struct bt_ctf_field *int_16 = bt_ctf_field_create(int_16_type);
	ok(int_16, "Instanciate a signed 16-bit integer");
	struct bt_ctf_field *uint_12 = bt_ctf_field_create(uint_12_type);
	ok(uint_12, "Instanciate an unsigned 12-bit integer");

	/* Can't modify types after instanciating them */
	ok(bt_ctf_field_type_integer_set_base(uint_12_type,
		BT_CTF_INTEGER_BASE_DECIMAL),
		"Check an integer type' base can't be modified after instanciation");
	ok(bt_ctf_field_type_integer_set_signed(uint_12_type, 0),
		"Check an integer type's signedness can't be modified after instanciation");

	/* Check signed property is checked */
	ok(bt_ctf_field_signed_integer_set_value(uint_12, -52),
		"Check bt_ctf_field_signed_integer_set_value is not allowed on an unsigned integer");
	ok(bt_ctf_field_unsigned_integer_set_value(int_16, 42),
		"Check bt_ctf_field_unsigned_integer_set_value is not allowed on a signed integer");

	/* Check the overflow is properly tested for */
	ok(bt_ctf_field_signed_integer_set_value(int_16, -32768) == 0,
		"Check -32768 is allowed for a signed 16-bit integer");
	ok(bt_ctf_field_signed_integer_set_value(int_16, 32767) == 0,
		"Check 32767 is allowed for a signed 16-bit integer");
	ok(bt_ctf_field_signed_integer_set_value(int_16, 32768),
		"Check 32768 is not allowed for a signed 16-bit integer");
	ok(bt_ctf_field_signed_integer_set_value(int_16, -32769),
		"Check -32769 is not allowed for a signed 16-bit integer");
	ok(bt_ctf_field_signed_integer_set_value(int_16, -42) == 0,
		"Check -42 is allowed for a signed 16-bit integer");

	ok(bt_ctf_field_unsigned_integer_set_value(uint_12, 4095) == 0,
		"Check 4095 is allowed for an unsigned 12-bit integer");
	ok(bt_ctf_field_unsigned_integer_set_value(uint_12, 4096),
		"Check 4096 is not allowed for a unsigned 12-bit integer");
	ok(bt_ctf_field_unsigned_integer_set_value(uint_12, 0) == 0,
		"Check 0 is allowed for an unsigned 12-bit integer");
	bt_ctf_field_unsigned_integer_set_value(uint_12, 1295);

	/* Set event payload */
	ok(bt_ctf_event_set_payload(event, "uint_12", uint_12) == 0,
		"Set an event field payload");
	ok(bt_ctf_event_set_payload(event, "uint_12", uint_12) == 0,
		"Change an event's existing payload");
	ok(bt_ctf_event_set_payload(event, "int_16", uint_12),
		"Reject event payloads of incorrect type");
	bt_ctf_event_set_payload(event, "int_16", int_16);

	/* Push one simple event in the stream and flush */
	struct bt_ctf_event *simple_event =
		bt_ctf_event_create(simple_event_class);
	struct bt_ctf_field *integer_field = bt_ctf_field_create(uint_12_type);
	bt_ctf_field_unsigned_integer_set_value(integer_field, 42);
	bt_ctf_event_set_payload(simple_event, "integer_field", integer_field);
	bt_ctf_field_put(integer_field);
	ok(bt_ctf_stream_push_event(stream1, simple_event) == 0,
		"Push simple event to trace stream");
	bt_ctf_event_put(simple_event);
	ok(bt_ctf_stream_flush(stream1) == 0,
		"Flush trace stream with one event");
	simple_event = NULL;
	integer_field = NULL;

	packet_resize_test(stream_class, stream1);

	char *metadata_string = bt_ctf_writer_get_metadata_string(writer);
	ok(metadata_string, "Get metadata string");

	bt_ctf_writer_flush_metadata(writer);
	validate_metadata(argv[1], metadata_path);

	ok(bt_ctf_stream_push_event(stream1, simple_event) == 0,
		"Push simple event to trace stream");
	ok(bt_ctf_stream_flush(stream1) == 0,
		"Flush trace stream");

	bt_ctf_field_put(uint_12);
	bt_ctf_clock_put(clock);
	bt_ctf_field_put(int_16);
	bt_ctf_stream_class_put(stream_class);
	bt_ctf_event_class_put(event_class);
	bt_ctf_event_class_put(simple_event_class);
	bt_ctf_field_type_put(uint_12_type);
	bt_ctf_field_type_put(uint_8_type);
	bt_ctf_field_type_put(sequence_type);
	bt_ctf_field_type_put(string_type);
	bt_ctf_field_type_put(structure_seq_type);
	bt_ctf_field_type_put(composite_structure_type);
	bt_ctf_field_type_put(int_16_type);
	bt_ctf_event_put(event);
	bt_ctf_event_put(simple_event);
	bt_ctf_writer_put(writer);
	bt_ctf_field_put(integer_field);
	bt_ctf_stream_put(stream1);
	free(metadata_string);

	/* Remove all trace files and delete temporary trace directory */
	DIR *trace_dir = opendir(trace_path);
	if (!trace_dir) {
		perror("# opendir");
		return -1;
	}

	struct dirent *entry;
	while ((entry = readdir(trace_dir))) {
		if (entry->d_type == DT_REG) {
			unlinkat(dirfd(trace_dir), entry->d_name, 0);
		}
	}

	rmdir(trace_path);
	closedir(trace_dir);

	return 0;
}
