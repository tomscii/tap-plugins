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

    $Id: tap_reverb.c,v 1.1 2004/01/31 20:53:55 tszilagyi Exp $
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "ladspa.h"
#include "tap_utils.h"
#include "tap_reverb_presets.h"


/* load plugin data from reverb_data[] into an instance */
void
load_plugin_data(LADSPA_Handle Instance) {

	Reverb * ptr = (Reverb *)Instance;
	unsigned long m;
	int i;

	m = *(ptr->mode);

	/* load combs data */
	ptr->num_combs = 2 * reverb_data[m].num_combs;
	for (i = 0; i < reverb_data[m].num_combs; i++) {
		((COMB_FILTER *)(ptr->combs + 2*i))->buflen = 
			reverb_data[m].combs[i].delay * ptr->sample_rate;
		((COMB_FILTER *)(ptr->combs + 2*i))->feedback = 
			reverb_data[m].combs[i].feedback;
		((COMB_FILTER *)(ptr->combs + 2*i))->freq_resp =
			reverb_data[m].combs[i].freq_resp;

		((COMB_FILTER *)(ptr->combs + 2*i+1))->buflen =
			((COMB_FILTER *)(ptr->combs + 2*i))->buflen;
		((COMB_FILTER *)(ptr->combs + 2*i+1))->feedback = 
			((COMB_FILTER *)(ptr->combs + 2*i))->feedback;
		((COMB_FILTER *)(ptr->combs + 2*i+1))->feedback = 
			((COMB_FILTER *)(ptr->combs + 2*i))->freq_resp;


		/* set initial values: */
		*(((COMB_FILTER *)(ptr->combs + 2*i))->buffer_pos) = 0;
		*(((COMB_FILTER *)(ptr->combs + 2*i+1))->buffer_pos) = 0;
		eq_set_params(((COMB_FILTER *)(ptr->combs + 2*i))->filter,
			      10000.0f,
			      reverb_data[m].combs[i].freq_resp * -60.0f,
			      FREQ_RESP_BWIDTH, ptr->sample_rate);
		eq_set_params(((COMB_FILTER *)(ptr->combs + 2*i+1))->filter,
			      10000.0f,
			      reverb_data[m].combs[i].freq_resp * -60.0f,
			      FREQ_RESP_BWIDTH, ptr->sample_rate);
	}

	/* load allps data */
	ptr->num_allps = 2 * reverb_data[m].num_allps;
	for (i = 0; i < reverb_data[m].num_allps; i++) {
		((ALLP_FILTER *)(ptr->allps + 2*i))->buflen = 
			reverb_data[m].allps[i].delay * ptr->sample_rate;
		((ALLP_FILTER *)(ptr->allps + 2*i))->feedback = 
			reverb_data[m].allps[i].feedback;

		((ALLP_FILTER *)(ptr->allps + 2*i+1))->buflen = 
			((ALLP_FILTER *)(ptr->allps + 2*i))->buflen;
		((ALLP_FILTER *)(ptr->allps + 2*i+1))->feedback = 
			((ALLP_FILTER *)(ptr->allps + 2*i))->feedback;

		/* set initial values: */
		*(((ALLP_FILTER *)(ptr->allps + 2*i))->buffer_pos) = 0;
		*(((ALLP_FILTER *)(ptr->allps + 2*i+1))->buffer_pos) = 0;
	}

	/* init bandpass filters */
	lp_set_params((biquad *)(ptr->low_pass), reverb_data[m].bandpass_high,
		      BANDPASS_BWIDTH, ptr->sample_rate);
	hp_set_params((biquad *)(ptr->high_pass), reverb_data[m].bandpass_low,
		      BANDPASS_BWIDTH, ptr->sample_rate);
	lp_set_params((biquad *)(ptr->low_pass + 1), reverb_data[m].bandpass_high,
		      BANDPASS_BWIDTH, ptr->sample_rate);
	hp_set_params((biquad *)(ptr->high_pass + 1), reverb_data[m].bandpass_low,
		      BANDPASS_BWIDTH, ptr->sample_rate);
}




/* push a sample into a ringbuffer and return the sample falling out */
LADSPA_Data
push_buffer(LADSPA_Data insample, LADSPA_Data * buffer, 
	    unsigned long buflen, unsigned long * pos) {
	
	LADSPA_Data outsample;

	outsample = buffer[*pos];
	buffer[(*pos)++] = insample;

	if (*pos >= buflen)
		*pos = 0;
	
	return outsample;
}


/* push a sample into a comb filter and return the sample falling out */
LADSPA_Data
comb_run(LADSPA_Data insample, COMB_FILTER * comb) {

	LADSPA_Data outsample;

	outsample = push_buffer(comb->fb_gain * insample + 
				biquad_run(comb->filter, comb->fb_gain * comb->last_out),
				comb->ringbuffer, comb->buflen, comb->buffer_pos);

	comb->last_out = outsample;
	return outsample;
}


/* push a sample into an allpass filter and return the sample falling out */
LADSPA_Data
allp_run(LADSPA_Data insample, ALLP_FILTER * allp) {

	LADSPA_Data outsample;

	outsample = push_buffer(allp->in_gain * insample + allp->fb_gain * allp->last_out,
				allp->ringbuffer, allp->buflen, allp->buffer_pos);

	allp->last_out = outsample;
	return outsample;
}


/* compute user-input-dependent reverberator coefficients */
void
comp_coeffs(LADSPA_Handle Instance) {

	Reverb * ptr = (Reverb *)Instance;
	int i;
	
	if (*(ptr->mode) != ptr->old_mode)
		load_plugin_data(Instance);

	for (i = 0; i < ptr->num_combs / 2; i++) {
		((COMB_FILTER *)(ptr->combs + 2*i))->fb_gain =
			powf(0.001f, /* -60 dB */
			     100000.0f * ((COMB_FILTER *)(ptr->combs + 2*i))->buflen
			     * (1 + FR_R_COMP * ((COMB_FILTER *)(ptr->combs + 2*i))->freq_resp)
			     / ((COMB_FILTER *)(ptr->combs + 2*i))->feedback
			     / *(ptr->decay)
			     / ptr->sample_rate);

		((COMB_FILTER *)(ptr->combs + 2*i+1))->fb_gain = 
			((COMB_FILTER *)(ptr->combs + 2*i))->fb_gain;
		
		if (*(ptr->stereo_enh) > 0.0f) {
			if (i % 2 == 0)
				((COMB_FILTER *)(ptr->combs + 2*i+1))->buflen =
					ENH_STEREO_RATIO * ((COMB_FILTER *)(ptr->combs + 2*i))->buflen;
			else
				((COMB_FILTER *)(ptr->combs + 2*i))->buflen =
					ENH_STEREO_RATIO * ((COMB_FILTER *)(ptr->combs + 2*i+1))->buflen;
		} else {
			if (i % 2 == 0)
				((COMB_FILTER *)(ptr->combs + 2*i+1))->buflen =
					((COMB_FILTER *)(ptr->combs + 2*i))->buflen;
			else
				((COMB_FILTER *)(ptr->combs + 2*i))->buflen =
					((COMB_FILTER *)(ptr->combs + 2*i+1))->buflen;
		}
	}

	for (i = 0; i < ptr->num_allps / 2; i++) {
		((ALLP_FILTER *)(ptr->allps + 2*i))->fb_gain =
			powf(0.001f, /* -60 dB */
			     10000.0f  /* smaller --> longer decay */
			     / *(ptr->decay)
			     / ((ALLP_FILTER *)(ptr->allps + 2*i))->feedback);
		
		((ALLP_FILTER *)(ptr->allps + 2*i+1))->fb_gain = 
			((ALLP_FILTER *)(ptr->allps + 2*i))->fb_gain;

		((ALLP_FILTER *)(ptr->allps + 2*i))->in_gain =
			-10.0f / ((ALLP_FILTER *)(ptr->allps + 2 * i))->feedback;

		((ALLP_FILTER *)(ptr->allps + 2*i+1))->in_gain = 
			((ALLP_FILTER *)(ptr->allps + 2*i))->in_gain;

		if (*(ptr->stereo_enh) > 0.0f) {
			if (i % 2 == 0)
				((ALLP_FILTER *)(ptr->allps + 2*i+1))->buflen =
					ENH_STEREO_RATIO * ((ALLP_FILTER *)(ptr->allps + 2*i))->buflen;
			else
				((ALLP_FILTER *)(ptr->allps + 2*i))->buflen =
					ENH_STEREO_RATIO * ((ALLP_FILTER *)(ptr->allps + 2*i+1))->buflen;
		} else {
			if (i % 2 == 0)
				((ALLP_FILTER *)(ptr->allps + 2*i+1))->buflen =
					((ALLP_FILTER *)(ptr->allps + 2*i))->buflen;
			else
				((ALLP_FILTER *)(ptr->allps + 2*i))->buflen =
					((ALLP_FILTER *)(ptr->allps + 2*i+1))->buflen;
		}
	}
}



/* Construct a new plugin instance. */
LADSPA_Handle 
instantiate_Reverb(const LADSPA_Descriptor * Descriptor,
		   unsigned long             SampleRate) {
	
	LADSPA_Handle * ptr;
	
	if ((ptr = malloc(sizeof(Reverb))) != NULL) {
		((Reverb *)ptr)->sample_rate = SampleRate;
		((Reverb *)ptr)->run_adding_gain = 1.0f;
		return ptr;
	}
	
	return NULL;
}


/* activate a plugin instance */
void
activate_Reverb(LADSPA_Handle Instance) {

	Reverb * ptr = (Reverb *)Instance;
	int i;

	/* allocate memory for comb/allpass filters and other dynamic vars */

	if ((ptr->combs = calloc(2 * MAX_COMBS, sizeof(COMB_FILTER))) == NULL)
		exit(1);
	for (i = 0; i < 2 * MAX_COMBS; i++) {
		if ((((COMB_FILTER *)(ptr->combs + i))->ringbuffer =
		     calloc(MAX_COMB_DELAY * ptr->sample_rate / 1000,
			    sizeof(LADSPA_Data))) == NULL)
			exit(1);
		if ((((COMB_FILTER *)(ptr->combs + i))->buffer_pos =
		     calloc(1, sizeof(unsigned long))) == NULL)
			exit(1);
		if ((((COMB_FILTER *)(ptr->combs + i))->filter =
		     calloc(1, sizeof(biquad))) == NULL)
			exit(1);
	}

	if ((ptr->allps = calloc(2 * MAX_ALLPS, sizeof(ALLP_FILTER))) == NULL)
		exit(1);
	for (i = 0; i < 2* MAX_ALLPS; i++) {
		if ((((ALLP_FILTER *)(ptr->allps + i))->ringbuffer =
		     calloc(MAX_ALLP_DELAY * ptr->sample_rate / 1000,
			    sizeof(LADSPA_Data))) == NULL)
			exit(1);
		if ((((ALLP_FILTER *)(ptr->allps + i))->buffer_pos =
		     calloc(1, sizeof(unsigned long))) == NULL)
			exit(1);
	}

	if ((ptr->low_pass = calloc(2, sizeof(biquad))) == NULL)
		exit(1);
	if ((ptr->high_pass = calloc(2, sizeof(biquad))) == NULL)
		exit(1);

	load_plugin_data(Instance);
	comp_coeffs(Instance);
}


/* deactivate a plugin instance */
void
deactivate_Reverb(LADSPA_Handle Instance) {

	Reverb * ptr = (Reverb *)Instance;
	int i;

	/* free memory allocated for comb/allpass filters & co. in activate_Reverb() */
	
	for (i = 0; i < 2 * MAX_COMBS; i++) {
		free(((COMB_FILTER *)(ptr->combs + i))->ringbuffer);
		free(((COMB_FILTER *)(ptr->combs + i))->buffer_pos);
		free(((COMB_FILTER *)(ptr->combs + i))->filter);
	}
	for (i = 0; i < 2 * MAX_ALLPS; i++) {
		free(((ALLP_FILTER *)(ptr->allps + i))->ringbuffer);
		free(((ALLP_FILTER *)(ptr->allps + i))->buffer_pos);
	}

	free(ptr->combs);
	free(ptr->allps);
	free(ptr->low_pass);
	free(ptr->high_pass);
}


/* Connect a port to a data location. */
void 
connect_port_Reverb(LADSPA_Handle Instance,
		    unsigned long Port,
		    LADSPA_Data * DataLocation) {
	
	Reverb * ptr;
	
	ptr = (Reverb *)Instance;
	switch (Port) {
	case DECAY:
		ptr->decay = DataLocation;
		break;
	case DRYLEVEL:
		ptr->drylevel = DataLocation;
		break;
	case WETLEVEL:
		ptr->wetlevel = DataLocation;
		break;
	case COMBS_EN:
		ptr->combs_en = DataLocation;
		break;
	case ALLPS_EN:
		ptr->allps_en = DataLocation;
		break;
	case BANDPASS_EN:
		ptr->bandpass_en = DataLocation;
		break;
	case STEREO_ENH:
		ptr->stereo_enh = DataLocation;
		break;
	case MODE:
		ptr->mode = DataLocation;
		break;
	case INPUT_L:
		ptr->input_L = DataLocation;
		break;
	case OUTPUT_L:
		ptr->output_L = DataLocation;
		break;
	case INPUT_R:
		ptr->input_R = DataLocation;
		break;
	case OUTPUT_R:
		ptr->output_R = DataLocation;
		break;
	}
}



void 
run_Reverb(LADSPA_Handle Instance,
	   unsigned long SampleCount) {
	
	Reverb * ptr = (Reverb *)Instance;

	unsigned long sample_index;
	int i;

	LADSPA_Data decay = *(ptr->decay);
	LADSPA_Data drylevel = db2lin(*(ptr->drylevel));
	LADSPA_Data wetlevel = db2lin(*(ptr->wetlevel));
	LADSPA_Data combs_en = *(ptr->combs_en);
	LADSPA_Data allps_en = *(ptr->allps_en);
	LADSPA_Data bandpass_en = *(ptr->bandpass_en);
	LADSPA_Data stereo_enh = *(ptr->stereo_enh);
      	LADSPA_Data mode = *(ptr->mode);

	LADSPA_Data * input_L = ptr->input_L;
	LADSPA_Data * output_L = ptr->output_L;
	LADSPA_Data * input_R = ptr->input_R;
	LADSPA_Data * output_R = ptr->output_R;

	LADSPA_Data out_L = 0;
	LADSPA_Data out_R = 0;
	LADSPA_Data in_L = 0;
	LADSPA_Data in_R = 0;
	LADSPA_Data combs_out_L = 0;
	LADSPA_Data combs_out_R = 0;


	/* see if the user changed any control since last run */
	if ((ptr->old_decay != decay) ||
	    (ptr->old_stereo_enh != stereo_enh) ||
	    (ptr->old_mode != mode)) {

		/* re-compute reverberator coefficients */
		comp_coeffs(Instance);

		/* save new values */
		ptr->old_decay = decay;
		ptr->old_stereo_enh = stereo_enh;
		ptr->old_mode = mode;
	}

	for (sample_index = 0; sample_index < SampleCount; sample_index++) {
		
		in_L = *(input_L++);
		in_R = *(input_R++);

		combs_out_L = in_L;
		combs_out_R = in_R;

		/* process comb filters */
		if (combs_en > 0.0f) {
			for (i = 0; i < ptr->num_combs / 2; i++) {
				combs_out_L += 
					comb_run(in_L, ((COMB_FILTER *)(ptr->combs + 2*i)));
				combs_out_R += 
					comb_run(in_R, ((COMB_FILTER *)(ptr->combs + 2*i+1)));
			}
		}

		/* process allpass filters */
		if (allps_en > 0.0f) {
			for (i = 0; i < ptr->num_allps / 2; i++) {
				combs_out_L += allp_run(combs_out_L,
						       ((ALLP_FILTER *)(ptr->allps + 2*i)));
				combs_out_R += allp_run(combs_out_R,
						       ((ALLP_FILTER *)(ptr->allps + 2*i+1)));
			}
		}

		/* process bandpass filters */
		if (bandpass_en > 0.0f) {
			combs_out_L = 
				biquad_run(((biquad *)(ptr->low_pass)), combs_out_L);
			combs_out_L = 
				biquad_run(((biquad *)(ptr->high_pass)), combs_out_L);
			combs_out_R = 
				biquad_run(((biquad *)(ptr->low_pass + 1)), combs_out_R);
			combs_out_R = 
				biquad_run(((biquad *)(ptr->high_pass + 1)), combs_out_R);
		}

		out_L = in_L * drylevel + combs_out_L * wetlevel;
		out_R = in_R * drylevel + combs_out_R * wetlevel;

		*(output_L++) = out_L;
		*(output_R++) = out_R;
	}
}





void
set_run_adding_gain(LADSPA_Handle Instance, LADSPA_Data gain){

	Reverb * ptr;

	ptr = (Reverb *)Instance;

	ptr->run_adding_gain = gain;
}


void 
run_adding_gain_Reverb(LADSPA_Handle Instance,
		       unsigned long SampleCount) {
	
	Reverb * ptr = (Reverb *)Instance;

	unsigned long sample_index;
	int i;

	LADSPA_Data decay = *(ptr->decay);
	LADSPA_Data drylevel = db2lin(*(ptr->drylevel));
	LADSPA_Data wetlevel = db2lin(*(ptr->wetlevel));
	LADSPA_Data combs_en = *(ptr->combs_en);
	LADSPA_Data allps_en = *(ptr->allps_en);
	LADSPA_Data bandpass_en = *(ptr->bandpass_en);
	LADSPA_Data stereo_enh = *(ptr->stereo_enh);
      	LADSPA_Data mode = *(ptr->mode);

	LADSPA_Data * input_L = ptr->input_L;
	LADSPA_Data * output_L = ptr->output_L;
	LADSPA_Data * input_R = ptr->input_R;
	LADSPA_Data * output_R = ptr->output_R;

	LADSPA_Data out_L = 0;
	LADSPA_Data out_R = 0;
	LADSPA_Data in_L = 0;
	LADSPA_Data in_R = 0;
	LADSPA_Data combs_out_L = 0;
	LADSPA_Data combs_out_R = 0;


	/* see if the user changed any control since last run */
	if ((ptr->old_decay != decay) ||
	    (ptr->old_stereo_enh != stereo_enh) ||
	    (ptr->old_mode != mode)) {

		/* re-compute reverberator coefficients */
		comp_coeffs(Instance);

		/* save new values */
		ptr->old_decay = decay;
		ptr->old_stereo_enh = stereo_enh;
		ptr->old_mode = mode;
	}

	for (sample_index = 0; sample_index < SampleCount; sample_index++) {
		
		in_L = *(input_L++);
		in_R = *(input_R++);

		combs_out_L = in_L;
		combs_out_R = in_R;

		/* process comb filters */
		if (combs_en > 0.0f) {
			for (i = 0; i < ptr->num_combs / 2; i++) {
				combs_out_L += 
					comb_run(in_L, ((COMB_FILTER *)(ptr->combs + 2*i)));
				combs_out_R += 
					comb_run(in_R, ((COMB_FILTER *)(ptr->combs + 2*i+1)));
			}
		}

		/* process allpass filters */
		if (allps_en > 0.0f) {
			for (i = 0; i < ptr->num_allps / 2; i++) {
				combs_out_L += allp_run(combs_out_L,
						       ((ALLP_FILTER *)(ptr->allps + 2*i)));
				combs_out_R += allp_run(combs_out_R,
						       ((ALLP_FILTER *)(ptr->allps + 2*i+1)));
			}
		}

		/* process bandpass filters */
		if (bandpass_en > 0.0f) {
			combs_out_L = 
				biquad_run(((biquad *)(ptr->low_pass)), combs_out_L);
			combs_out_L = 
				biquad_run(((biquad *)(ptr->high_pass)), combs_out_L);
			combs_out_R = 
				biquad_run(((biquad *)(ptr->low_pass + 1)), combs_out_R);
			combs_out_R = 
				biquad_run(((biquad *)(ptr->high_pass + 1)), combs_out_R);
		}

		out_L = in_L * drylevel + combs_out_L * wetlevel;
		out_R = in_R * drylevel + combs_out_R * wetlevel;

		*(output_L++) += out_L * ptr->run_adding_gain;
		*(output_R++) += out_R * ptr->run_adding_gain;
	}
}



/* Throw away a Reverb effect instance. */
void 
cleanup_Reverb(LADSPA_Handle Instance) {

	free(Instance);
}



LADSPA_Descriptor * stereo_descriptor = NULL;



/* _init() is called automatically when the plugin library is first
   loaded. */
void 
_init() {
	
	char ** port_names;
	LADSPA_PortDescriptor * port_descriptors;
	LADSPA_PortRangeHint * port_range_hints;
	char string[2048]; /* currently used: 1025 out of 2048 */

#ifdef _SHOW_PRESET_NAMES_IN_GUI_
	int i;
	char snum[128];

	strcpy(string,"Presets\n");
	for (i = 0; i < NUM_MODES; i++) {
		if (i % 2 == 0) {
			strcat(string, "\n");
			strcat(string, reverb_data[i].name);
			sprintf(snum," :%i  ", i);
			strcat(string, snum);
		} else {
			sprintf(snum," %i: ", i);
			strcat(string, snum);
			strcat(string, reverb_data[i].name);
		}
	}
#else
	strcpy(string,"Preset");
#endif
	
	if ((stereo_descriptor =
	     (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor))) == NULL)
		exit(1);		
	

	/* init the stereo Reverb */
	
	stereo_descriptor->UniqueID = ID_STEREO;
	stereo_descriptor->Label = strdup("tap_reverb");
	stereo_descriptor->Properties = 0;
	stereo_descriptor->Name = strdup("TAP Reverberator");
	stereo_descriptor->Maker = strdup("Tom Szilagyi");
	stereo_descriptor->Copyright = strdup("GPL");
	stereo_descriptor->PortCount = PORTCOUNT_STEREO;

	if ((port_descriptors = 
	     (LADSPA_PortDescriptor *)calloc(PORTCOUNT_STEREO, sizeof(LADSPA_PortDescriptor))) == NULL)
		exit(1);		

	stereo_descriptor->PortDescriptors = (const LADSPA_PortDescriptor *)port_descriptors;
	port_descriptors[DECAY] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[DRYLEVEL] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[WETLEVEL] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[COMBS_EN] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[ALLPS_EN] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[BANDPASS_EN] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[STEREO_ENH] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[MODE] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;

	port_descriptors[INPUT_L] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT_L] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
	port_descriptors[INPUT_R] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT_R] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;

	if ((port_names = 
	     (char **)calloc(PORTCOUNT_STEREO, sizeof(char *))) == NULL)
		exit(1);		

	stereo_descriptor->PortNames = (const char **)port_names;

	port_names[DECAY] = strdup("Decay [ms]");
	port_names[DRYLEVEL] = strdup("Dry Level [dB]");
	port_names[WETLEVEL] = strdup("Wet Level [dB]");
	port_names[COMBS_EN] = strdup("Comb Filters");
	port_names[ALLPS_EN] = strdup("Allpass Filters");
	port_names[BANDPASS_EN] = strdup("Bandpass Filter");
	port_names[STEREO_ENH] = strdup("Enhanced Stereo");
	port_names[MODE] = strdup(string);
	port_names[INPUT_L] = strdup("Input Left");
	port_names[OUTPUT_L] = strdup("Output Left");
	port_names[INPUT_R] = strdup("Input Right");
	port_names[OUTPUT_R] = strdup("Output Right");

	if ((port_range_hints = 
	     ((LADSPA_PortRangeHint *)calloc(PORTCOUNT_STEREO, sizeof(LADSPA_PortRangeHint)))) == NULL)
		exit(1);		

	stereo_descriptor->PortRangeHints = (const LADSPA_PortRangeHint *)port_range_hints;

	port_range_hints[DECAY].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_LOW);
	port_range_hints[DECAY].LowerBound = 0;
	port_range_hints[DECAY].UpperBound = MAX_DECAY;

	port_range_hints[DRYLEVEL].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[DRYLEVEL].LowerBound = -70.0f;
	port_range_hints[DRYLEVEL].UpperBound = +10.0f;

	port_range_hints[WETLEVEL].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[WETLEVEL].LowerBound = -70.0f;
	port_range_hints[WETLEVEL].UpperBound = +10.0f;

	port_range_hints[COMBS_EN].HintDescriptor = 
		(LADSPA_HINT_TOGGLED |
		 LADSPA_HINT_DEFAULT_1);

	port_range_hints[ALLPS_EN].HintDescriptor = 
		(LADSPA_HINT_TOGGLED |
		 LADSPA_HINT_DEFAULT_1);

	port_range_hints[BANDPASS_EN].HintDescriptor = 
		(LADSPA_HINT_TOGGLED |
		 LADSPA_HINT_DEFAULT_1);

	port_range_hints[STEREO_ENH].HintDescriptor = 
		(LADSPA_HINT_TOGGLED |
		 LADSPA_HINT_DEFAULT_1);

	port_range_hints[MODE].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_INTEGER |
		 LADSPA_HINT_DEFAULT_0);
	port_range_hints[MODE].LowerBound = 0;
	port_range_hints[MODE].UpperBound = NUM_MODES - 0.9f;

	port_range_hints[INPUT_L].HintDescriptor = 0;
	port_range_hints[OUTPUT_L].HintDescriptor = 0;
	port_range_hints[INPUT_R].HintDescriptor = 0;
	port_range_hints[OUTPUT_R].HintDescriptor = 0;


	stereo_descriptor->instantiate = instantiate_Reverb;
	stereo_descriptor->connect_port = connect_port_Reverb;
	stereo_descriptor->activate = activate_Reverb;
	stereo_descriptor->run = run_Reverb;
	stereo_descriptor->run_adding = run_adding_gain_Reverb;
	stereo_descriptor->set_run_adding_gain = set_run_adding_gain;
	stereo_descriptor->deactivate = deactivate_Reverb;
	stereo_descriptor->cleanup = cleanup_Reverb;
	
}


void
delete_descriptor(LADSPA_Descriptor * descriptor) {
	unsigned long index;
	if (descriptor) {
		free((char *)descriptor->Label);
		free((char *)descriptor->Name);
		free((char *)descriptor->Maker);
		free((char *)descriptor->Copyright);
		free((LADSPA_PortDescriptor *)descriptor->PortDescriptors);
		for (index = 0; index < descriptor->PortCount; index++)
			free((char *)(descriptor->PortNames[index]));
		free((char **)descriptor->PortNames);
		free((LADSPA_PortRangeHint *)descriptor->PortRangeHints);
		free(descriptor);
	}
}


/* _fini() is called automatically when the library is unloaded. */
void
_fini() {
	delete_descriptor(stereo_descriptor);
}


/* Return a descriptor of the requested plugin type. */
const LADSPA_Descriptor * 
ladspa_descriptor(unsigned long Index) {

	switch (Index) {
	case 0:
		return stereo_descriptor;
	default:
		return NULL;
	}
}
