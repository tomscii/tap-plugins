/*                                                     -*- linux-c -*-
    Copyright (C) 2004 Tom Szilagyi
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    $Id: tap_reverb.h,v 1.8 2004/06/09 20:10:10 tszilagyi Exp $
*/



/* The Unique ID of the plugin: */

#define ID_STEREO       2142

/* The port numbers for the plugin: */

#define DECAY       0
#define DRYLEVEL    1
#define WETLEVEL    2
#define COMBS_EN    3  /* comb filters on/off */
#define ALLPS_EN    4  /* allpass filters on/off */
#define BANDPASS_EN 5  /* bandpass filters on/off */
#define STEREO_ENH  6  /* stereo enhanced mode on/off */
#define MODE        7

#define INPUT_L     8
#define OUTPUT_L    9
#define INPUT_R     10
#define OUTPUT_R    11

/* Total number of ports */

#define PORTCOUNT_STEREO 12

/* Global constants (times in ms, bwidth in octaves) */

#define MAX_COMBS         20
#define MAX_ALLPS         20
#define MAX_DECAY         10000.0f
#define MAX_COMB_DELAY    250.0f
#define MAX_ALLP_DELAY    20.0f
#define BANDPASS_BWIDTH   1.5f
#define FREQ_RESP_BWIDTH  3.0f
#define ENH_STEREO_RATIO  0.998f

/* compensation ratio of freq_resp in fb_gain calc */
#define FR_R_COMP         0.75f


typedef struct {
	LADSPA_Data feedback;
	LADSPA_Data fb_gain;
	LADSPA_Data freq_resp;
	LADSPA_Data * ringbuffer;
	unsigned long buflen;
	unsigned long * buffer_pos;
	biquad * filter;
	LADSPA_Data last_out;
} COMB_FILTER;

typedef struct {
	LADSPA_Data feedback;
	LADSPA_Data fb_gain;
	LADSPA_Data in_gain;
	LADSPA_Data * ringbuffer;
	unsigned long buflen;
	unsigned long * buffer_pos;
	LADSPA_Data last_out;
} ALLP_FILTER;


/* The structure used to hold port connection information and state */

typedef struct {
	unsigned long num_combs; /* total number of comb filters */
	unsigned long num_allps; /* total number of allpass filters */
	COMB_FILTER * combs;
	ALLP_FILTER * allps;
	biquad * low_pass; /* ptr to 2 low-pass filters */
	biquad * high_pass; /* ptr to 2 high-pass filters */
	unsigned long sample_rate;

	LADSPA_Data * decay;
	LADSPA_Data * drylevel;
	LADSPA_Data * wetlevel;
	LADSPA_Data * combs_en; /* on/off */
	LADSPA_Data * allps_en; /* on/off */
        LADSPA_Data * bandpass_en; /* on/off */
	LADSPA_Data * stereo_enh; /* on/off */
	LADSPA_Data * mode;

	LADSPA_Data * input_L;
	LADSPA_Data * output_L;
	LADSPA_Data * input_R;
	LADSPA_Data * output_R;

	LADSPA_Data old_decay;
	LADSPA_Data old_stereo_enh;
	LADSPA_Data old_mode;

	LADSPA_Data run_adding_gain;
} Reverb;

typedef struct {
	LADSPA_Data delay;
	LADSPA_Data feedback;
	LADSPA_Data freq_resp;
} COMB_DATA;

typedef struct {
	LADSPA_Data delay;
	LADSPA_Data feedback;
} ALLP_DATA;

typedef struct {
	char name[50];
	unsigned long num_combs;
	unsigned long num_allps;
	COMB_DATA combs[MAX_COMBS];
	ALLP_DATA allps[MAX_ALLPS];
	LADSPA_Data bandpass_low;
	LADSPA_Data bandpass_high;
} REVERB_DATA;
