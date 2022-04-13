/*
 * voip.c
 *
 *  Created on: Dec 5, 2020
 *      Author: ugurbuyukdurak
 */

#include <stdio.h>
#include <alsa/asoundlib.h>
#include "voip.h"

snd_pcm_uframes_t period_size;

void set_hw_params(snd_pcm_t *handle, int verbose) {
	snd_pcm_hw_params_t *hw_params;
	int err;

	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
		fprintf(stderr, "cannot allocate hardware parameter structure (%s)\n",
				snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	if ((err = snd_pcm_hw_params_any(handle, hw_params)) < 0) {
		fprintf(stderr, "cannot initialize hardware parameter structure (%s)\n",
				snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	if ((err = snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		fprintf(stderr, "cannot set access type (%s)\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	if ((err = snd_pcm_hw_params_set_format(handle, hw_params,
			SND_PCM_FORMAT_S16_LE)) < 0) {
		fprintf(stderr, "cannot set sample format (%s)\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	unsigned int rate = RATE;
	if ((err = snd_pcm_hw_params_set_rate(handle, hw_params, rate, 0)) < 0) {
		fprintf(stderr, "cannot set sample rate (%s)\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	unsigned int channels = CHANNELS;
	if ((err = snd_pcm_hw_params_set_channels(handle, hw_params, channels))
			< 0) {
		fprintf(stderr, "cannot set channel count (%s)\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	unsigned int period_time = PERIOD_TIME;
	if ((err = snd_pcm_hw_params_set_period_time(handle, hw_params, period_time,
			0)) < 0) {
		fprintf(stderr, "cannot set period time (%s)\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	unsigned int periods = PERIODS;
	if ((err = snd_pcm_hw_params_set_periods(handle, hw_params, periods, 0))
			< 0) {
		fprintf(stderr, "cannot set periods (%s)\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	if ((err = snd_pcm_hw_params(handle, hw_params)) < 0) {
		fprintf(stderr, "cannot set parameters (%s)\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	/* Read hardware parameters */

	int dir;
	unsigned int buffer_time;
	snd_pcm_uframes_t buffer_size;

	if ((err = snd_pcm_hw_params_get_rate(hw_params, &rate, &dir)) < 0) {
		fprintf(stderr, "cannot get rate (%s)\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	if ((err = snd_pcm_hw_params_get_channels(hw_params, &channels)) < 0) {
		fprintf(stderr, "cannot get channel count (%s)\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	if ((err = snd_pcm_hw_params_get_periods(hw_params, &periods, &dir)) < 0) {
		fprintf(stderr, "cannot get periods (%s)\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	if ((err = snd_pcm_hw_params_get_period_size(hw_params, &period_size, &dir))
			< 0) {
		fprintf(stderr, "cannot get period size (%s)\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	if ((err = snd_pcm_hw_params_get_period_time(hw_params, &period_time, &dir))
			< 0) {
		fprintf(stderr, "cannot get period time (%s)\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	if ((err = snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size))
			< 0) {
		fprintf(stderr, "cannot get buffer size (%s)\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	if ((err = snd_pcm_hw_params_get_buffer_time(hw_params, &buffer_time, &dir))
			< 0) {
		fprintf(stderr, "cannot get buffer time (%s)\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	if(verbose){
		fprintf(stderr, "[alsa] Hardware parameters chosen:\n");
		fprintf(stderr, "[alsa]	rate        = %u\n", rate);
		fprintf(stderr, "[alsa]	periods     = %u\n", periods);
		fprintf(stderr, "[alsa]	period_size = %lu\n", period_size);
		fprintf(stderr, "[alsa]	period_time = %u us\n", period_time);
		fprintf(stderr, "[alsa]	buffer_size = %lu\n", buffer_size);
		fprintf(stderr, "[alsa]	buffer_time = %u us\n", buffer_time);
	}

	snd_pcm_hw_params_free(hw_params);
}

void set_sw_params(snd_pcm_t *handle, int verbose) {
	snd_pcm_sw_params_t *sw_params;
	int err;

	if ((err = snd_pcm_sw_params_malloc(&sw_params)) < 0) {
		fprintf(stderr, "cannot allocate software parameter structure (%s)\n",
				snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	if ((err = snd_pcm_sw_params_current(handle, sw_params)) < 0) {
		fprintf(stderr,
				"cannot initialize software parameters structure (%s)\n",
				snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	snd_pcm_uframes_t start_threshold = 2 * period_size;
	if ((err = snd_pcm_sw_params_set_start_threshold(handle, sw_params, start_threshold)) < 0) {
		fprintf(stderr, "cannot set start threshold (%s)\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	if ((err = snd_pcm_sw_params(handle, sw_params)) < 0) {
		fprintf(stderr, "cannot set software parameters (%s)\n",
				snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	if ((err = snd_pcm_sw_params_current(handle, sw_params)) < 0) {
		fprintf(stderr, "cannot initialize software parameters structure (%s)\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	/* Read parameters */
	snd_pcm_uframes_t avail_min;
	if ((err = snd_pcm_sw_params_get_avail_min(sw_params, &avail_min))
			< 0) {
		fprintf(stderr, "cannot get minimum available count (%s)\n",
				snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	if ((err = snd_pcm_sw_params_get_start_threshold(sw_params, &start_threshold)) < 0) {
		fprintf(stderr, "cannot get start threshold (%s)\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	snd_pcm_uframes_t stop_threshold;
	if ((err = snd_pcm_sw_params_get_stop_threshold(sw_params, &stop_threshold)) < 0) {
		fprintf(stderr, "cannot get stop threshold (%s)\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	if(verbose){
		fprintf(stderr, "[alsa] Software parameters chosen:\n");
		fprintf(stderr, "[alsa]	avail_min       = %lu\n", avail_min);
		fprintf(stderr, "[alsa]	start_threshold = %lu\n", start_threshold);
		fprintf(stderr, "[alsa]	stop_threshold  = %lu\n", stop_threshold);
	}

	snd_pcm_sw_params_free(sw_params);
}
