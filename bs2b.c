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

#include <math.h>

#include "bs2b.h"

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

/* Single pole IIR filter.
 * O[n] = a0*I[n] + a1*I[n-1] + b1*O[n-1]
 */

/* Lowpass filter */
#define lo_filter(in, out_1) (_a0_lo * in + _b1_lo * out_1)

/* Highboost filter */
#define hi_filter(in, in_1, out_1) (_a0_hi * in + _a1_hi * in_1 + _b1_hi * out_1)

static int _level = BS2B_DEFAULT_CLEVEL;  /* Crossfeed level */
static int _srate = BS2B_DEFAULT_SRATE;   /* Sample rate (Hz) */

/* Default coefficients for High Easy level and 44100 Hz ) */

/* Lowpass IIR filter coefficients */
static double _a0_lo =  0.037788750135521;
static double _b1_lo =  0.905078951270879;

/* Highboost IIR filter coefficients */
static double _a0_hi =  0.973324969589985;
static double _a1_hi = -0.870302905339220;
static double _b1_hi =  0.870302905339220;

/* Global gain against overloading */
static double _gain =  0.838619849405637;

/* Buffer of last filtered sample.
 * [0] - first channel, [1] - second channel
 */
static struct t_last_sample
{
	double asis[2];
	double lo[2];
	double hi[2];
} _last_sample = {{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}};

/* Type for 24bit conversions */
typedef union int_char
{
	long i;
	struct c { char byte[4]; } c;
} t_int_char;

/* Convert 24bit integer to double */
static double
tribyte2double (char *in_24)
{
	t_int_char int_char;

	int_char.c.byte[0] = in_24[0];
	int_char.c.byte[1] = in_24[1];
	int_char.c.byte[2] = in_24[2];
	int_char.c.byte[3] = in_24[2] & 0x80 ? 0xFF : 0x00;

	return (double) int_char.i;
} /* tribyte2double */

/* Convert double to 24bit integer */
static void
double2tribyte (double in_d, char *out_24)
{
	t_int_char int_char;

	int_char.i = (long) in_d;

	out_24[0] = int_char.c.byte[0];
	out_24[1] = int_char.c.byte[1];
	out_24[2] = int_char.c.byte[2];
} /* double2tribyte */

/* Set up all data. */
static void
init (void)
{
	double Fc_lo, Fc_hi;
	double G_lo,  G_hi;
	double x;

	if ((_srate > 192000) || (_srate < 2000))
		_srate = BS2B_DEFAULT_SRATE;
	
	switch (_level)
	{
	case BS2B_LOW_CLEVEL: /* Low crossfeed level */
		Fc_lo = 360.0;
		Fc_hi = 501.0;
		G_lo  = 0.398107170553497;
		G_hi  = 0.205671765275719;
		break;

	case BS2B_MIDDLE_CLEVEL: /* Middle crossfeed level */
		Fc_lo = 500.0;
		Fc_hi = 711.0;
		G_lo  = 0.459726988530872;
		G_hi  = 0.228208484414988;
		break;

	case BS2B_HIGH_CLEVEL: /* High crossfeed level (virtual speakers are closer to itself) */
		Fc_lo = 700.0;
		Fc_hi = 1021.0;
		G_lo  = 0.530884444230988;
		G_hi  = 0.250105790667544;
		break;

	case BS2B_LOW_ECLEVEL: /* Low easy crossfeed level */
		Fc_lo = 360.0;
		Fc_hi = 494.0;
		G_lo  = 0.316227766016838;
		G_hi  = 0.168236228897329;
		break;

	case BS2B_MIDDLE_ECLEVEL: /* Middle easy crossfeed level */
		Fc_lo = 500.0;
		Fc_hi = 689.0;
		G_lo  = 0.354813389233575;
		G_hi  = 0.187169483835901;
		break;

	default: /* High easy crossfeed level */
		_level = BS2B_HIGH_ECLEVEL;

		Fc_lo = 700.0;
		Fc_hi = 975.0;
		G_lo  = 0.398107170553497;
		G_hi  = 0.205671765275719;
		break;
	} /* switch */

	/* $fc = $Fc / $s;
	 * $d  = 1 / 2 / pi / $fc;
	 * $x  = exp(-1 / $d);
	 */

	x      = exp (-2.0 * M_PI * Fc_lo / _srate);
	_b1_lo = x;
	_a0_lo = G_lo * (1.0 - x);
	
	x      = exp (-2.0 * M_PI * Fc_hi / _srate);
	_b1_hi = x;
	_a0_hi = 1.0 - G_hi * (1.0 - x);
	_a1_hi = -x;

	_gain  = 1.0 / (1.0 - G_hi + G_lo);

	bs2b_clear ();
} /* init */

/* Exported functions.
 * See descriptions in "bs2b.h"
 */

void
bs2b_set_level (int level)
{
	if (level == _level)
		return;
	_level = level;
	init ();
} /* bs2b_set_level */

int
bs2b_get_level (void)
{
	return _level;
} /* bs2b_get_level */

void
bs2b_set_srate (int srate)
{
	if (srate == _srate)
		return;
	_srate = srate;
	init ();
} /* bs2b_set_srate */

int
bs2b_get_srate (void)
{
	return _srate;
} /* bs2b_get_srate */

void
bs2b_clear (void)
{
	int loopv = sizeof(_last_sample);

	while (loopv)
	{
		((char *)&_last_sample)[--loopv] = 0;
	}
} /* bs2b_clear */

int
bs2b_is_clear (void)
{
	int loopv = sizeof(_last_sample);

	while (loopv)
	{
		if (((char *)&_last_sample)[--loopv] != 0)
			return 0;
	}
	return 1;
} /* bs2b_is_clear */

void
bs2b_cross_feed (double *sample)
{
	/* Lowpass filter */
	_last_sample.lo[0] = lo_filter (sample[0], _last_sample.lo[0]);
	_last_sample.lo[1] = lo_filter (sample[1], _last_sample.lo[1]);

	/* Highboost filter */
	_last_sample.hi[0] =
		hi_filter (sample[0], _last_sample.asis[0], _last_sample.hi[0]);
	_last_sample.hi[1] =
		hi_filter (sample[1], _last_sample.asis[1], _last_sample.hi[1]);
	_last_sample.asis[0] = sample[0];
	_last_sample.asis[1] = sample[1];

	/* Crossfeed */
	sample[0] = _last_sample.hi[0] + _last_sample.lo[1];
	sample[1] = _last_sample.hi[1] + _last_sample.lo[0];

	/* Bass boost cause allpass attenuation */
	sample[0] *= _gain;
	sample[1] *= _gain;

	/* Clipping of overloaded samples */
	if (sample[0] > 1.0)
		sample[0] = 1.0;
	if (sample[0] < -1.0)
		sample[0] = -1.0;
	if (sample[1] > 1.0)
		sample[1] = 1.0;
	if (sample[1] < -1.0)
		sample[1] = -1.0;
} /* bs2b_cross_feed */

void
bs2b_cross_feed_f32 (float *sample)
{
	double sample_d[2];

	sample_d[0] = (double) sample[0];
	sample_d[1] = (double) sample[1];

	bs2b_cross_feed (sample_d);

	sample[0] = (float) sample_d[0];
	sample[1] = (float) sample_d[1];
} /* bs2b_cross_feed_f32 */

#define MAX_LONG_VALUE   2147483647.0
#define MAX_SHORT_VALUE       32767.0
#define MAX_CHAR_VALUE          127.0
#define MAX_24BIT_VALUE     8388607.0

void
bs2b_cross_feed_32 (long *sample)
{
	double sample_d[2];

	sample_d[0] = (double) sample[0] / MAX_LONG_VALUE;
	sample_d[1] = (double) sample[1] / MAX_LONG_VALUE;

	bs2b_cross_feed (sample_d);

	sample[0] = (long) (sample_d[0] * MAX_LONG_VALUE);
	sample[1] = (long) (sample_d[1] * MAX_LONG_VALUE);
} /* bs2b_cross_feed_32 */

void
bs2b_cross_feed_16 (short *sample)
{
	double sample_d[2];

	sample_d[0] = (double) sample[0] / MAX_SHORT_VALUE;
	sample_d[1] = (double) sample[1] / MAX_SHORT_VALUE;

	bs2b_cross_feed (sample_d);

	sample[0] = (short) (sample_d[0] * MAX_SHORT_VALUE);
	sample[1] = (short) (sample_d[1] * MAX_SHORT_VALUE);
} /* bs2b_cross_feed_16 */

void
bs2b_cross_feed_s8 (char *sample)
{
	double sample_d[2];

	sample_d[0] = (double) sample[0] / MAX_CHAR_VALUE;
	sample_d[1] = (double) sample[1] / MAX_CHAR_VALUE;

	bs2b_cross_feed (sample_d);

	sample[0] = (char) (sample_d[0] * MAX_CHAR_VALUE);
	sample[1] = (char) (sample_d[1] * MAX_CHAR_VALUE);
} /* bs2b_cross_feed_s8 */

void
bs2b_cross_feed_u8 (unsigned char *sample)
{
	double sample_d[2];

	sample_d[0] = ((double) ((char) (sample[0] ^ 0x80))) / MAX_CHAR_VALUE;
	sample_d[1] = ((double) ((char) (sample[1] ^ 0x80))) / MAX_CHAR_VALUE;

	bs2b_cross_feed (sample_d);

	sample[0] = ((unsigned char) (sample_d[0] * MAX_CHAR_VALUE)) ^ 0x80;
	sample[1] = ((unsigned char) (sample_d[1] * MAX_CHAR_VALUE)) ^ 0x80;
} /* bs2b_cross_feed_u8 */

void
bs2b_cross_feed_24 (void *sample)
{
	double sample_d[2];
	char *sample_1 = sample;
	char *sample_2 = sample_1 + 3;


	sample_d[0] = tribyte2double (sample_1);
	sample_d[1] = tribyte2double (sample_2);

	sample_d[0] /= MAX_24BIT_VALUE;
	sample_d[1] /= MAX_24BIT_VALUE;

	bs2b_cross_feed (sample_d);

	double2tribyte (sample_d[0] * MAX_24BIT_VALUE, sample_1);
	double2tribyte (sample_d[1] * MAX_24BIT_VALUE, sample_2);
} /* bs2b_cross_feed_24 */
