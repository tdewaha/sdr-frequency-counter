/**
 *   Searches for and displays the frequency and strength of the strongest signal
 *   within a user specified range.
 *   Uses librtlsdr and kiss fft libraries.

 *   Copyright (C) 2013 - 2015  Tom de Waha

 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.

 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.

 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
**/

#include <rtl-sdr.h>
#include <stdlib.h>
#include <time.h>
#include "kiss_fft.h"
#include <signal.h>
#include <stdbool.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y));
#define FREQUENCY_CORRECTION -50
#define SAMPLE_RATE 2000000
#define FFT_SIZE 1000
#define BUFFER_LENGTH 2048

uint32_t frequency_min, frequency, frequency_max, frequency_peak;
uint8_t *samples;
float power_peak;

static rtlsdr_dev_t *dongle = NULL;
struct timespec loopdlyA, loopdlyB;

bool stop_program = false;

kiss_fft_cfg fft_cfg;
kiss_fft_cpx *fft_in, *fft_out;

void signalHandler()
{
	printf("Stopping...\n");
	stop_program = true;
}

bool parse_arguments(int argc, char *argv[])
{
	if(argc != 3)
        {
                printf("usage: %s MinFreq(MHz) MaxFreq(MHz)\n", argv[0]);
		return false;
        } else {
                frequency_min = atoi(argv[1])*1000000;
                frequency_max = atoi(argv[2])*1000000;
                if(frequency_min < 2.e7 || frequency_max > 2.e9)
                {
                        printf("Invalid argument\n");
                        return false;
                }
        }
	return true;
}

bool init_vars()
{
	frequency = frequency_min;
	frequency_peak = 0;
	samples = malloc(BUFFER_LENGTH * sizeof(uint8_t));
	fft_cfg = kiss_fft_alloc(FFT_SIZE, 0, NULL, NULL);
	fft_in = malloc(FFT_SIZE * sizeof(kiss_fft_cpx));
	fft_out = malloc(FFT_SIZE * sizeof(kiss_fft_cpx));
	return true;
}

bool init_device()
{
	if(!rtlsdr_get_device_count())
	{
		fprintf(stderr, "FATAL: No devices found\n");
		return false;
	}
	
	if(rtlsdr_open(&dongle, 0) < 0)
	{
		fprintf(stderr, "FATAL: Failed to open device\n");
		return false;
	}

	if(rtlsdr_set_sample_rate(dongle, SAMPLE_RATE) < 0)
	{
		fprintf(stderr, "WARN: Failed to set sample rate\n");
	}
	
	if(rtlsdr_set_center_freq(dongle, frequency) < 0)
	{
		fprintf(stderr, "WARN: Failed to set frequency\n");
	}

	if(rtlsdr_set_tuner_gain_mode(dongle, 0) < 0)
	{
		fprintf(stderr, "WARN: Failed to set automatic gain\n");
	}

	if(rtlsdr_set_freq_correction(dongle, FREQUENCY_CORRECTION) < 0)
	{
		fprintf(stderr, "WARN: Failed to set frequency correction\n");
	}

	if(rtlsdr_reset_buffer(dongle) < 0)
	{
		fprintf(stderr, "WARN: Failed to clear buffers\n");
	}
	return true;
}

void print_startup_info()
{
	printf("Scanning from %.3f MHz to %.3f MHz\n", 1.e-6f*(float)frequency_min, 1.e-6f*(float)frequency_max);
	printf("BW: %.1f MHz   RBW: %.2f kHz\n", 1.e-6f*(float)SAMPLE_RATE, 1.e-3f*(float)(SAMPLE_RATE/FFT_SIZE));
	printf("\n");
}

bool read_samples()
{
	int n_read;
	if(rtlsdr_read_sync(dongle, samples, BUFFER_LENGTH, &n_read) < 0) return false;
	if((uint32_t)n_read < BUFFER_LENGTH) return false;
 	return true;	
}

bool doFFT()
{
	kiss_fft_cpx pt;
	float power;
	float maxpower = -1000;
	int maxpoweridx = 0;
	int i;
	for(i=0; i < FFT_SIZE; i++)
	{
        	fft_in[i].r = ((float)samples[2*i])/255.f;
        	fft_in[i].i = ((float)samples[2*i+1])/255.f;
	}

	kiss_fft(fft_cfg, fft_in, fft_out);

	fft_out[0].r = 0;
	fft_out[0].i = 0;

	for(i=0; i < FFT_SIZE; i++)
	{
        	if (i < FFT_SIZE / 2)
        	{
            		pt.r = fft_out[FFT_SIZE/2+i].r / FFT_SIZE;
            		pt.i = fft_out[FFT_SIZE/2+i].i / FFT_SIZE;
        	} else {
            		pt.r = fft_out[i-FFT_SIZE/2].r / FFT_SIZE;
            		pt.i = fft_out[i-FFT_SIZE/2].i / FFT_SIZE;
        	}
	
        	power = pt.r * pt.r + pt.i * pt.i;
		if(power > maxpower)
		{
			maxpower = power;
			maxpoweridx = i;
		}
	}

	frequency_peak = (frequency - SAMPLE_RATE/2) + maxpoweridx * SAMPLE_RATE / FFT_SIZE;
	power_peak = 10.f * log10(maxpower + 1.0e-20f);
	return true;
}

void print_results()
{
	printf("\rSignal: %f @ %.1f dB",(float)frequency_peak/1000000, power_peak);
}

void analyze()
{
	loopdlyA.tv_nsec = 1000;
	while(true)
	{
		if(stop_program == true) break;
		rtlsdr_set_center_freq(dongle, frequency);
		rtlsdr_reset_buffer(dongle);
		if(read_samples() == false)
		{
			fprintf(stderr, "FATAL: Failed to read samples\n");
			break;
		}
		doFFT();
		print_results();
		nanosleep(&loopdlyA, &loopdlyB);
	}
}

int main(int argc, char *argv[])
{
	if(parse_arguments(argc, argv) == false) return 0;
	init_vars();
	init_device();
	print_startup_info();
	signal(SIGINT, signalHandler);	

	analyze();

	return 0;
}

