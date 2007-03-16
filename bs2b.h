/*-
 * Copyright (c) 2005 Boris Mikhaylov
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef BS2B_C
#define BS2B_C

#include "bs2b.c"

#endif	/* BS2B_C */

#ifndef BS2B_H
#define BS2B_H

/* Number of crossfeed levels */
#define BS2B_CLEVELS           3

/* Normal crossfeed levels */
#define BS2B_HIGH_CLEVEL       3
#define BS2B_MIDDLE_CLEVEL     2
#define BS2B_LOW_CLEVEL        1

/* Easy crossfeed levels */
#define BS2B_HIGH_ECLEVEL      BS2B_HIGH_CLEVEL    + BS2B_CLEVELS
#define BS2B_MIDDLE_ECLEVEL    BS2B_MIDDLE_CLEVEL  + BS2B_CLEVELS
#define BS2B_LOW_ECLEVEL       BS2B_LOW_CLEVEL     + BS2B_CLEVELS

/* Default crossfeed levels */
#define BS2B_DEFAULT_CLEVEL    BS2B_HIGH_ECLEVEL
/* Default sample rate (Hz) */
#define BS2B_DEFAULT_SRATE     44100

#define EPS                    0.00000001

#ifdef __cplusplus
extern "C"
{
#endif	/* __cplusplus */

/* Clear buffers and set new coefficients with new crossfeed level value.
 * level - crossfeed level of *LEVEL values.
 */
void
bs2b_set_level (int level);

/* Return current crossfeed level value */
int
bs2b_get_level (void);

/* Clear buffers and set new coefficients with new sample rate value.
 * srate - sample rate by Hz.
 */
void
bs2b_set_srate (int srate);

/* Return current sample rate value */
int
bs2b_get_srate (void);

/* Clear buffer */
void
bs2b_clear (void);

/* Return 1 if buffer is clear */
int
bs2b_is_clear (void);

/* Crossfeeds one stereo sample that are pointed by sample.
 * [0] - first channel, [1] - second channel.
 * Returns crossfided samle by sample pointer.
 */

/* sample poits to double floats */
void
bs2b_cross_feed (double *sample);

/* sample poits to floats */
void
bs2b_cross_feed_f32 (float *sample);

/* sample poits to 32bit integers */
void
bs2b_cross_feed_32 (long *sample);

/* sample poits to 16bit integers */
void
bs2b_cross_feed_16 (short *sample);

/* sample poits to 8bit integers */
void
bs2b_cross_feed_s8 (char *sample);

/* sample poits to 8bit unsigned integers */
void
bs2b_cross_feed_u8 (unsigned char *sample);

/* sample poits to 24bit integers */
void
bs2b_cross_feed_24 (void *sample);

#ifdef __cplusplus
}	/* extern "C" */
#endif /* __cplusplus */

#endif	/* BS2B_H */
