/*                                                     -*- linux-c -*-
    Copyright (C) 2006 Maarten Maathuis
    
    Thanks to Tom Szilagyi for his plugins which i used as example.

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

    $Id: tap_bs2b.c,v 1.2 2006/11/30 19:34:39 tszilagyi Exp $
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "ladspa.h"
#include "tap_utils.h"
#include "bs2b.h"

/* The Unique ID of the plugin: */

#define ID_STEREO         2160

/* The port numbers for the plugin: */

#define CONTROL_CROSSFEED	0
#define CONTROL_HIGH_BOOST	1
#define INPUT_L         	2
#define INPUT_R         	3
#define OUTPUT_L        	4
#define OUTPUT_R        	5


/* Total number of ports */

#define PORTCOUNT_STEREO   6

/* The structure used to hold port connection information and state */

typedef struct {
	LADSPA_Data * crossfeed;
	LADSPA_Data * high_boost;
	LADSPA_Data * input_L;
	LADSPA_Data * input_R;
	LADSPA_Data * output_L;
	LADSPA_Data * output_R;
	unsigned long SampleRate;
	LADSPA_Data run_adding_gain;
} BS2B;



/* Construct a new plugin instance. */
LADSPA_Handle 
instantiate_BS2B(const LADSPA_Descriptor * Descriptor,
		 unsigned long SampleRate) {
	
	LADSPA_Handle * ptr;
	
	if ((ptr = malloc(sizeof(BS2B))) != NULL) {
		((BS2B *)ptr)->SampleRate = SampleRate;
		return ptr;
	}
	
	return NULL;
}

void
activate_BS2B(LADSPA_Handle Instance) {

	BS2B * ptr;

	ptr = (BS2B *)Instance;
}



/* Connect a port to a data location. */
void 
connect_port_BS2B(LADSPA_Handle Instance,
		  unsigned long Port,
		  LADSPA_Data * DataLocation) {
	
	BS2B * ptr;
	
	ptr = (BS2B *)Instance;
	switch (Port) {
	case CONTROL_CROSSFEED:
		ptr->crossfeed = DataLocation;
		break;
	case CONTROL_HIGH_BOOST:
		ptr->high_boost = DataLocation;
		break;
	case INPUT_L:
		ptr->input_L = DataLocation;
		break;
	case INPUT_R:
		ptr->input_R = DataLocation;
		break;
	case OUTPUT_L:
		ptr->output_L = DataLocation;
		break;
	case OUTPUT_R:
		ptr->output_R = DataLocation;
		break;
	}
}



void 
run_BS2B(LADSPA_Handle Instance,
	 unsigned long SampleCount) {
  
	BS2B * ptr = (BS2B *)Instance;

	LADSPA_Data * input_L = ptr->input_L;
	LADSPA_Data * input_R = ptr->input_R;
	LADSPA_Data * output_L = ptr->output_L;
	LADSPA_Data * output_R = ptr->output_R;
	unsigned long SampleRate = ptr->SampleRate;
	LADSPA_Data crossfeed = *(ptr->crossfeed);
	LADSPA_Data high_boost = *(ptr->high_boost);
	unsigned long sample_index;
	LADSPA_Data samples[SampleCount*2];
	LADSPA_Data * samplepointer = samples;
	
	/* The bs2b library expects an array of both the left and right input samples */		    
	for (sample_index = 0; sample_index < SampleCount; sample_index++) {
		samples[sample_index*2] = *(input_L++);
		samples[sample_index*2+1] = *(input_R++);
	}
	
	/* Levels are from 1 to 6, the normal levels are 1-3 and the boosted ones 4-6 */
	if (high_boost > 0.0f) {
		bs2b_set_level ((int)(crossfeed+BS2B_CLEVELS));
	} else {
		bs2b_set_level ((int)(crossfeed));
	}
	
	bs2b_set_srate (SampleRate);
	for (sample_index = 0; sample_index < SampleCount; sample_index++) {
		bs2b_cross_feed_f32 (samplepointer);
		samplepointer += 2;
	}
	
	for (sample_index = 0; sample_index < SampleCount; sample_index++) {
		*(output_L++) = samples[sample_index*2];
		*(output_R++) = samples[sample_index*2+1];
	}
}

void
set_run_adding_gain_BS2B(LADSPA_Handle Instance, LADSPA_Data gain) {

	BS2B * ptr;

	ptr = (BS2B *)Instance;

	ptr->run_adding_gain = gain;
}

void 
run_adding_BS2B(LADSPA_Handle Instance,
		unsigned long SampleCount) {
  
	BS2B * ptr = (BS2B *)Instance;

	LADSPA_Data * input_L = ptr->input_L;
	LADSPA_Data * input_R = ptr->input_R;
	LADSPA_Data * output_L = ptr->output_L;
	LADSPA_Data * output_R = ptr->output_R;
	unsigned long SampleRate = ptr->SampleRate;
	LADSPA_Data crossfeed = *ptr->crossfeed;
	LADSPA_Data high_boost = *ptr->high_boost;
	unsigned long sample_index;
	LADSPA_Data samples[SampleCount*2];
	LADSPA_Data * samplepointer = samples;
	
	/* The bs2b library expects an array of both the left and right input samples */		    
	for (sample_index = 0; sample_index < SampleCount; sample_index++) {
		samples[sample_index*2] = *(input_L++);
		samples[sample_index*2+1] = *(input_R++);
	}
	
	if (high_boost > 0.0f) {
		bs2b_set_level ((int)(crossfeed+BS2B_CLEVELS));
	} else {
		bs2b_set_level ((int)(crossfeed));
	}
	
	bs2b_set_srate (SampleRate);
	for (sample_index = 0; sample_index < SampleCount; sample_index++) {
		bs2b_cross_feed_f32 (samplepointer);
		samplepointer += 2;
	}
	
	for (sample_index = 0; sample_index < SampleCount; sample_index++) {
		*(output_L++) += samples[sample_index*2]*ptr->run_adding_gain;
		*(output_R++) += samples[sample_index*2+1]*ptr->run_adding_gain;
	}
}

/* Throw away an bs2b effect instance. */
void 
cleanup_BS2B(LADSPA_Handle Instance) {
	free(Instance);
}


LADSPA_Descriptor * mono_descriptor = NULL;


/* _init() is called automatically when the plugin library is first
   loaded. */
void 
_init() {
	
	char ** port_names;
	LADSPA_PortDescriptor * port_descriptors;
	LADSPA_PortRangeHint * port_range_hints;
	
	if ((mono_descriptor = 
	     (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor))) == NULL)
		exit(1);

	mono_descriptor->UniqueID = ID_STEREO;
	mono_descriptor->Label = strdup("tap_bs2b");
	mono_descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
	mono_descriptor->Name = strdup("TAP Bauer stereophonic-to-binaural DSP");
	mono_descriptor->Maker = strdup("Maarten Maathuis");
	mono_descriptor->Copyright = strdup("GPL");
	mono_descriptor->PortCount = PORTCOUNT_STEREO;

	if ((port_descriptors =
	     (LADSPA_PortDescriptor *)calloc(PORTCOUNT_STEREO, sizeof(LADSPA_PortDescriptor))) == NULL)
		exit(1);

	mono_descriptor->PortDescriptors = (const LADSPA_PortDescriptor *)port_descriptors;
	port_descriptors[CONTROL_CROSSFEED] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[CONTROL_HIGH_BOOST] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_descriptors[INPUT_L] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[INPUT_R] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT_L] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
	port_descriptors[OUTPUT_R] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;

	if ((port_names = 
	     (char **)calloc(PORTCOUNT_STEREO, sizeof(char *))) == NULL)
		exit(1);

	mono_descriptor->PortNames = (const char **)port_names;
	port_names[CONTROL_CROSSFEED] = strdup("Crossfeed Level");
	port_names[CONTROL_HIGH_BOOST] = strdup("High Boost");
	port_names[INPUT_L] = strdup("Input L");
	port_names[INPUT_R] = strdup("Input R");
	port_names[OUTPUT_L] = strdup("Output L");
	port_names[OUTPUT_R] = strdup("Output R");

	if ((port_range_hints = 
	     ((LADSPA_PortRangeHint *)calloc(PORTCOUNT_STEREO, sizeof(LADSPA_PortRangeHint)))) == NULL)
		exit(1);

	mono_descriptor->PortRangeHints	= (const LADSPA_PortRangeHint *)port_range_hints;
	port_range_hints[CONTROL_CROSSFEED].HintDescriptor = 
		(LADSPA_HINT_BOUNDED_BELOW |
		 LADSPA_HINT_BOUNDED_ABOVE |
		 LADSPA_HINT_INTEGER |
		 LADSPA_HINT_DEFAULT_MAXIMUM);
	port_range_hints[CONTROL_CROSSFEED].LowerBound = 1;
	port_range_hints[CONTROL_CROSSFEED].UpperBound = 3;
	port_range_hints[CONTROL_HIGH_BOOST].HintDescriptor = 
		(LADSPA_HINT_TOGGLED |
		 LADSPA_HINT_DEFAULT_1);
	port_range_hints[INPUT_L].HintDescriptor = 0;
	port_range_hints[INPUT_R].HintDescriptor = 0;
	port_range_hints[OUTPUT_L].HintDescriptor = 0;
	port_range_hints[OUTPUT_R].HintDescriptor = 0;
	mono_descriptor->instantiate = instantiate_BS2B;
	mono_descriptor->connect_port = connect_port_BS2B;
	mono_descriptor->activate = activate_BS2B;
	mono_descriptor->run = run_BS2B;
	mono_descriptor->run_adding = run_adding_BS2B;
	mono_descriptor->set_run_adding_gain = set_run_adding_gain_BS2B;
	mono_descriptor->deactivate = NULL;
	mono_descriptor->cleanup = cleanup_BS2B;
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
	delete_descriptor(mono_descriptor);
}


/* Return a descriptor of the requested plugin type. */
const LADSPA_Descriptor * 
ladspa_descriptor(unsigned long Index) {

	switch (Index) {
	case 0:
		return mono_descriptor;
	default:
		return NULL;
	}
}
