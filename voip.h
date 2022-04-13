/*
 * voip.h
 *
 *  Created on: Dec 5, 2020
 *      Author: ugurbuyukdurak
 */

#ifndef VOIP_H_
#define VOIP_H_

#define SOUND_DEVICE "hw:0,0"

#define RATE 48000
#define CHANNELS 2
#define PERIOD_TIME 20000
#define PERIODS 10

extern snd_pcm_uframes_t period_size;

void set_hw_params(snd_pcm_t *handle, int verbose);
void set_sw_params(snd_pcm_t *handle, int verbose);

#endif /* VOIP_H_ */
