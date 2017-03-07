/**
 * @section License
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2016, Erik Moqvist
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * This file is part of the Simba project.
 */

#include "simba.h"

#define TX_BUFFER_SIZE                                    48

static struct soam_t soam;
static uint8_t txbuf[TX_BUFFER_SIZE];
static uint8_t queuebuf[256];

static struct queue_t chout;
static struct fs_command_t cmd_foo;

static int cmd_foo_cb(int argc,
                      const char *argv[],
                      void *chout_p,
                      void *chin_p,
                      void *arg_p,
                      void *call_arg_p)
{
    /* The formatted string must be longer than the printf output
       buffer to the begin/end functionality. */
    std_fprintf(chout_p,
                OSTR("%d %u %f %s %c\r\n"),
                -33544,
                56342,
                98743.58,
                "this is a bunch of characters that will not fit in "
                "the tx buffer...",
                'q');

    /* Bypass the channel control begin/end functions to write binary
       data. */
    chan_write(chout_p,
               "\xbe\xef\xba\xbe\x00\x01\x02\x03"
               "\xbe\xef\xba\xbe\x00\x01\x02\x03"
               "\xbe\xef\xba\xbe\x00\x01\x02\x03"
               "\xbe\xef\xba\xbe\x00\x01\x02\x03"
               "\xbe\xef\xba\xbe\x00\x01\x02\x03"
               "\xbe\xef\xba\xbe\x00\x01\x02\x03",
               48);

    return (-1);
}

static int test_init(struct harness_t *harness_p)
{
    void *chin_p;

    BTASSERT(fs_command_init(&cmd_foo, CSTR("/foo"), cmd_foo_cb, NULL) == 0);
    BTASSERT(fs_command_register(&cmd_foo) == 0);

    BTASSERT(queue_init(&chout, &queuebuf[0], sizeof(queuebuf)) == 0);

    BTASSERT(soam_init(&soam,
                       &txbuf[0],
                       sizeof(txbuf),
                       &chout) == 0);

    chin_p = soam_get_log_input_channel(&soam);

    BTASSERT(chin_p != NULL);
    BTASSERT(log_set_default_handler_output_channel(chin_p) == 0);

    return (0);
}

static int test_database_request(struct harness_t *harness_p)
{
    uint8_t buf[64];
    size_t size;
    uint16_t crc;

    /* Request the database id. */
    buf[0] = 0x70;
    buf[1] = 0x01;
    buf[2] = 0x00;
    buf[3] = 0x02;
    buf[4] = 0xd1;
    buf[5] = 0xc7;
    BTASSERT(soam_input(&soam, &buf[0], 6) == 0);

    /* Read the database id. */
    BTASSERT(chan_read(&chout, &buf[0], 38) == 38);
    BTASSERT(buf[0] == 0x81);
    BTASSERT(buf[1] == 1);
    size = ((buf[2] << 8) | buf[3]);
    BTASSERT(size == 34);
    crc = ((buf[36] << 8) | buf[37]);
    BTASSERT(crc_ccitt(0xffff, &buf[0], 36) == crc);

    return (0);
}

static int test_log(struct harness_t *harness_p)
{
    uint8_t buf[64];
    size_t size;
    size_t offset;
    uint16_t crc;

    log_object_print(NULL,
                     LOG_INFO,
                     OSTR("one: %d, "
                          "erik"": %s\r\n"),
                     1,
                     "erik");

    /* Read the SOAM packet written to the output channel from the log
       function. */
    BTASSERT(chan_read(&chout, &buf[0], 4) == 4);
    BTASSERT(buf[0] == 0x21);
    BTASSERT(buf[1] == 2);

    size = ((buf[2] << 8) | buf[3]);
    BTASSERT(size >= 31);
    offset = 4;

    /* Skip the log timestamp since its value is unknown. */
    BTASSERT(chan_read(&chout, &buf[offset], size - 30) == size - 30);
    offset += (size - 30);
    BTASSERT(chan_read(&chout, &buf[offset], 30) == 30);

    BTASSERT(memcmp(&buf[offset], ":info:main:default: ", 20) == 0);
    BTASSERT(memcmp(&buf[offset + 22],
                    "1\x1f"
                    "erik",
                    6) == 0);

    crc = ((buf[size + 2] << 8) | buf[size + 3]);
    BTASSERT(crc_ccitt(0xffff, &buf[0], size + 2) == crc);

    /* Another log point. */
    log_object_print(NULL, LOG_ERROR, OSTR("foo\r\n"));

    /* Read the SOAM packet written to the output channel from the log
       function. */
    BTASSERT(chan_read(&chout, &buf[0], 4) == 4);
    BTASSERT(buf[0] == 0x21);
    BTASSERT(buf[1] == 3);

    size = ((buf[2] << 8) | buf[3]);
    BTASSERT(size >= 26);
    offset = 4;

    /* Skip the log timestamp since its value is unknown. */
    BTASSERT(chan_read(&chout, &buf[offset], size - 25) == size - 25);
    offset += (size - 25);
    BTASSERT(chan_read(&chout, &buf[offset], 25) == 25);

    BTASSERT(memcmp(&buf[offset], ":error:main:default: ", 21) == 0);

    crc = ((buf[size + 2] << 8) | buf[size + 3]);
    BTASSERT(crc_ccitt(0xffff, &buf[0], size + 2) == crc);

    return (0);
}

static int test_command(struct harness_t *harness_p)
{
    uint8_t buf[64];
    size_t size;
    uint16_t crc;

    /* Command 'foo'. */
    buf[0] = 0x30;
    buf[1] = 0x01;
    buf[2] = 0x00;
    buf[3] = 0x05;
    buf[4] = CSTR("/foo")[1];
    buf[5] = CSTR("/foo")[2];
    buf[6] = 0x00;
    buf[7] = 0x95;
    buf[8] = 0x09;
    BTASSERT(soam_input(&soam, &buf[0], 9) == 0);

    /* Read the command response printf data packet #1, created by
       std_fprintf(...). */
    BTASSERT(chan_read(&chout, &buf[0], 4) == 4);
    BTASSERT(buf[0] == 0x40);
    BTASSERT(buf[1] == 4);

    size = ((buf[2] << 8) | buf[3]);
    BTASSERT(size == 44);

    BTASSERT(chan_read(&chout, &buf[4], size) == size);
    BTASSERT(memcmp(&buf[4],
                    "\x80\x01"
                    "-33544""\x1f"
                    "56342""\x1f"
                    "98743.580000""\x1f"
                    "this is a bunc",
                    42) == 0);
    crc = ((buf[46] << 8) | buf[47]);
    BTASSERT(crc_ccitt(0xffff, &buf[0], 46) == crc);

    /* Read the command response printf data packet #2, created by
       std_fprintf(...). */
    BTASSERT(chan_read(&chout, &buf[0], 4) == 4);
    BTASSERT(buf[0] == 0x42);
    BTASSERT(buf[1] == 5);

    size = ((buf[2] << 8) | buf[3]);
    BTASSERT(size == 44);

    BTASSERT(chan_read(&chout, &buf[4], size) == size);
    BTASSERT(memcmp(&buf[4],
                    "h of characters that will not fit in the t",
                    42) == 0);
    crc = ((buf[46] << 8) | buf[47]);
    BTASSERT(crc_ccitt(0xffff, &buf[0], 46) == crc);

    /* Read the command response printf data packet #3, created by
       std_fprintf(...). */
    BTASSERT(chan_read(&chout, &buf[0], 4) == 4);
    BTASSERT(buf[0] == 0x43);
    BTASSERT(buf[1] == 6);

    size = ((buf[2] << 8) | buf[3]);
    BTASSERT(size == 15);

    BTASSERT(chan_read(&chout, &buf[4], size) == size);
    BTASSERT(memcmp(&buf[4],
                    "x buffer..."
                    "\x1f"
                    "q",
                    13) == 0);
    crc = ((buf[17] << 8) | buf[18]);
    BTASSERT(crc_ccitt(0xffff, &buf[0], 17) == crc);

    /* Read the command response binary data packet #1, created by
       chan_write(...). */
    BTASSERT(chan_read(&chout, &buf[0], 4) == 4);
    BTASSERT(buf[0] == 0x50);
    BTASSERT(buf[1] == 7);

    size = ((buf[2] << 8) | buf[3]);
    BTASSERT(size == 44);

    BTASSERT(chan_read(&chout, &buf[4], size) == size);
    BTASSERT(memcmp(&buf[4],
                    "\xbe\xef\xba\xbe\x00\x01\x02\x03"
                    "\xbe\xef\xba\xbe\x00\x01\x02\x03"
                    "\xbe\xef\xba\xbe\x00\x01\x02\x03"
                    "\xbe\xef\xba\xbe\x00\x01\x02\x03"
                    "\xbe\xef\xba\xbe\x00\x01\x02\x03"
                    "\xbe\xef",
                    42) == 0);
    crc = ((buf[46] << 8) | buf[47]);
    BTASSERT(crc_ccitt(0xffff, &buf[0], 46) == crc);

    /* Read the command response binary data packet #2, created by
       chan_write(...). */
    BTASSERT(chan_read(&chout, &buf[0], 4) == 4);
    BTASSERT(buf[0] == 0x53);
    BTASSERT(buf[1] == 8);

    size = ((buf[2] << 8) | buf[3]);
    BTASSERT(size == 8);

    BTASSERT(chan_read(&chout, &buf[4], size) == size);
    BTASSERT(memcmp(&buf[4], "\xba\xbe\x00\x01\x02\x03", 6) == 0);
    crc = ((buf[10] << 8) | buf[11]);
    BTASSERT(crc_ccitt(0xffff, &buf[0], 10) == crc);

    /* Read the command response packet, containing the command exit
       code. */
    BTASSERT(chan_read(&chout, &buf[0], 4) == 4);
    BTASSERT(buf[0] == 0x61);
    BTASSERT(buf[1] == 9);

    size = ((buf[2] << 8) | buf[3]);
    BTASSERT(size == 6);

    BTASSERT(chan_read(&chout, &buf[4], size) == size);
    BTASSERT(buf[4] == 0xff);
    BTASSERT(buf[5] == 0xff);
    BTASSERT(buf[6] == 0xff);
    BTASSERT(buf[7] == 0xff);

    crc = ((buf[8] << 8) | buf[9]);
    BTASSERT(crc_ccitt(0xffff, &buf[0], 8) == crc);

    return (0);
}

static int test_bad_input(struct harness_t *harness_p)
{
    /* Short packet. */
    BTASSERT(soam_input(&soam, (uint8_t *)"1234", 4) == -1);

    /* CRC error. */
    BTASSERT(soam_input(&soam,
                        (uint8_t *)"\x10"
                        "\x01"
                        "\x00\x05"
                        "\x80\x03\x00"
                        "\xa5\xc1",
                        9) == -1);

    /* Bad length in the packet. */
    BTASSERT(soam_input(&soam,
                        (uint8_t *)"\x10"
                        "\x01"
                        "\x00\x06"
                        "\x80\x03\x00"
                        "\x02\x20",
                        9) == -1);

    return (0);
}

static int test_invalid_type(struct harness_t *harness_p)
{
    uint8_t buf[64];
    size_t size;
    uint16_t crc;

    /* Invalid type identifier. */
    buf[0] = 0xf0;
    buf[1] = 0x01;
    buf[2] = 0x00;
    buf[3] = 0x02;
    buf[4] = 0x0c;
    buf[5] = 0xff;
    BTASSERT(soam_input(&soam, &buf[0], 6) == -1);

    /* Read the invalid type response. */
    BTASSERT(chan_read(&chout, &buf[0], 12) == 12);
    BTASSERT(buf[0] == 0x91);
    BTASSERT(buf[1] == 10);
    size = ((buf[2] << 8) | buf[3]);
    BTASSERT(size == 8);
    BTASSERT(memcmp(&buf[4], "\xf0\x01\x00\x02\x0c\xff", 6) == 0);
    crc = ((buf[10] << 8) | buf[11]);
    BTASSERT(crc_ccitt(0xffff, &buf[0], 10) == crc);

    return (0);
}

static int test_stdout(struct harness_t *harness_p)
{
    void *stdout_p;
    uint8_t buf[64];
    size_t size;
    uint16_t crc;

    stdout_p = sys_get_stdout();

    sys_set_stdout(soam_get_stdout_input_channel(&soam));
    std_printf(OSTR("hej\r\n"));

    sys_set_stdout(stdout_p);

    /* Read the SOAM packet written to the output channel from the
       printf function. */
    BTASSERT(chan_read(&chout, &buf[0], 4) == 4);
    BTASSERT(buf[0] == 0x11);
    BTASSERT(buf[1] == 11);

    size = ((buf[2] << 8) | buf[3]);
    BTASSERT(size == 4);

    BTASSERT(chan_read(&chout, &buf[4], size) == size);

    crc = ((buf[6] << 8) | buf[7]);
    BTASSERT(crc_ccitt(0xffff, &buf[0], size + 2) == crc);

    return (0);
}

int main()
{
    struct harness_t harness;
    struct harness_testcase_t harness_testcases[] = {
        { test_init, "test_init" },
        { test_database_request, "test_database_request" },
        { test_log, "test_log" },
        { test_command, "test_command" },
        { test_bad_input, "test_bad_input" },
        { test_invalid_type, "test_invalid_type" },
        { test_stdout, "test_stdout" },
        { NULL, NULL }
    };

    sys_start();

    harness_init(&harness);
    harness_run(&harness, harness_testcases);

    return (0);
}
