#include <stdio.h>
#include <unistd.h>
#include <alsa/asoundlib.h>
#include <opus/opus.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include "voip.h"
#include "xor-feed.h"

#define COMPLEXITY 10
#define PORT 10000
#define HOLE_PUNCHING_SERVER "XXX"

#define DEBUG
#undef DEBUG

int main(int argc, char *argv[]) {

	// Variables
	int err;
	int opt;
	int verbose = 0;
	unsigned short revents;
	ssize_t bytesNsent = 0;
	ssize_t bytesNread = 0;

	// PCM devices
	snd_pcm_t *capture;
	snd_pcm_t *playback;
	const char *audiodevice = "hw:0,0";
	snd_pcm_sframes_t pcmnread;
	snd_pcm_sframes_t pcmnwrite;

	// Opus Encoder Settings
	OpusEncoder *enc;
	opus_int16 buffer_in[960 * 2];
	uint8_t opus_enc_packet[4000];
	opus_int32 nBytesEncoded = 0;
	int bitrate = 16000;

	// Opus Decoder Settings
	OpusDecoder *dec;
	opus_int16 buffer_out[960 * 6 * 2];
	opus_int32 nBytesDecoded = 0;

	// Socket Settings
	struct sockaddr_in server;
	struct sockaddr_in remote_addr;
	socklen_t remote_addr_len = sizeof(remote_addr);
	int sockfd;

	// Poll
	struct pollfd fds[6];

	// Parse command line arguments
	while ((opt = getopt(argc, argv, "hvb:")) != -1) {
		switch (opt) {
		case 'h':
			exit(EXIT_SUCCESS);
			break;
		case 'v':
			verbose = 1;
			break;
		case 'b':
			bitrate = atoi(optarg);
			break;
		}
	}

	// Open PCM devices and set
	if ((err = snd_pcm_open(&playback, audiodevice, SND_PCM_STREAM_PLAYBACK,
	SND_PCM_NONBLOCK)) < 0) {
		fprintf(stderr, "cannot open audio device %s (%s)\n", audiodevice,
				snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	set_hw_params(playback, verbose);
	set_sw_params(playback, verbose);

	if ((err = snd_pcm_open(&capture, audiodevice, SND_PCM_STREAM_CAPTURE,
	SND_PCM_NONBLOCK)) < 0) {
		fprintf(stderr, "cannot open audio device %s (%s)\n", audiodevice,
				snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	set_hw_params(capture, verbose);
	set_sw_params(capture, verbose);

	int nfds_playback = snd_pcm_poll_descriptors_count(playback);
	if ((err = snd_pcm_poll_descriptors(playback, fds + 1, nfds_playback))
			< 0) {
		fprintf(stderr, "Error snd_pcm_poll_descriptor (%s)",
				snd_strerror(err));
		exit(1);
	}

	int nfds_capture = snd_pcm_poll_descriptors_count(capture);
	if ((err = snd_pcm_poll_descriptors(capture, fds + 1 + nfds_playback,
			nfds_capture)) < 0) {
		fprintf(stderr, "Error snd_pcm_poll_descriptor (%s)",
				snd_strerror(err));
		exit(1);
	}

	// Create socket and socket polling setup
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Socket");
		exit(EXIT_FAILURE);
	}
	fds[0].fd = sockfd;
	fds[0].events = POLLIN;

	// Create Opus encoder and decoder
	enc = opus_encoder_create(RATE, CHANNELS, OPUS_APPLICATION_VOIP, &err);
	if (err < 0) {
		fprintf(stderr, "opus encoder fail (%s)\n", opus_strerror(err));
		exit(EXIT_FAILURE);
	}

	err = opus_encoder_ctl(enc, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
	if(err < 0){
		fprintf(stderr, "opus encoder_ctl fail (%s)\n", opus_strerror(err));
		exit(EXIT_FAILURE);
	}

	err = opus_encoder_ctl(enc, OPUS_SET_BITRATE(bitrate));
	if(err < 0){
		fprintf(stderr, "opus encoder_ctl fail (%s)\n", opus_strerror(err));
		exit(EXIT_FAILURE);
	}

	err = opus_encoder_ctl(enc, OPUS_SET_COMPLEXITY(COMPLEXITY));
	if(err < 0){
		fprintf(stderr, "opus encoder_ctl fail (%s)\n", opus_strerror(err));
		exit(EXIT_FAILURE);
	}

	dec = opus_decoder_create(RATE, CHANNELS, &err);
	if (err < 0) {
		fprintf(stderr, "opus decoder fail (%s)\n", opus_strerror(err));
		exit(EXIT_FAILURE);
	}


	// Send message server.
	memset(&remote_addr, 0, sizeof(remote_addr));
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(PORT);
	remote_addr.sin_addr.s_addr = inet_addr(HOLE_PUNCHING_SERVER);

	if ((bytesNsent = sendto(sockfd, NULL, 0, 0,
					(struct sockaddr*) &remote_addr, remote_addr_len)) < 0) {
		perror("sendto()");
		exit(EXIT_FAILURE);
	}

	socklen_t server_addr_len;
	if ((bytesNread = recvfrom(sockfd, &remote_addr,
					sizeof(remote_addr), 0, (struct sockaddr*) &server,
					&server_addr_len)) < 0) {
		perror("recvfrom()");
		exit(EXIT_FAILURE);
	}

	if ((err = snd_pcm_start(capture)) < 0) {
		fprintf(stderr, "Problem starting audio device %s\n",
				snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	while (1) {
		int pollret = poll(fds, 3, -1);
		if (pollret == -1) {
			perror("poll()");
			exit(EXIT_FAILURE);
		}

		if (fds[0].revents & POLLERR) {
			fprintf(stderr, "Socket pollerr\n");
			exit(EXIT_FAILURE);
		}

		if (fds[0].revents & POLLIN) {
			if ((bytesNread = recvfrom(sockfd, opus_enc_packet,
					sizeof(opus_enc_packet), 0, (struct sockaddr*) &remote_addr,
					&remote_addr_len)) < 0) {
				perror("recvfrom()");
				exit(EXIT_FAILURE);
			}

			for(int i = 0; i < bytesNread; i++)
				opus_enc_packet[i] ^= (xor_cipher_feed_1[i] ^ xor_cipher_feed_2[i]);

			nBytesDecoded = opus_decode(dec, opus_enc_packet, bytesNread,
					buffer_out, 960 * 6, 0);
			if (nBytesDecoded < 0) {
				fprintf(stderr, "Error opus_decode\n");
				exit(EXIT_FAILURE);
			}

			snd_pcm_poll_descriptors_revents(playback, fds + 1, nfds_playback,
					&revents);
			if (revents & POLLOUT) {
				if ((pcmnwrite = snd_pcm_writei(playback, buffer_out,
						nBytesDecoded)) < 0) {
#ifdef DEBUG
					fprintf(stderr, "write to audio interface failed (%s)\n",
							snd_strerror(pcmnwrite));
#endif
					snd_pcm_prepare(playback);
					if ((pcmnwrite = snd_pcm_writei(playback, buffer_out,
							nBytesDecoded)) < 0) {
						exit(EXIT_FAILURE);
					}
				}
			}
		}

		snd_pcm_poll_descriptors_revents(capture, fds + 1 + nfds_playback,
				nfds_capture, &revents);
		if (revents & POLLIN) {
			if ((pcmnread = snd_pcm_readi(capture, buffer_in, 960)) != 960) {
				if (pcmnread < 0) {
#ifdef DEBUG
					fprintf(stderr, "read from audio interface failed (%s)\n",
							snd_strerror(pcmnread));
#endif
					snd_pcm_prepare(capture);
					if ((pcmnread = snd_pcm_readi(capture, buffer_in, 960))
							!= 960) {
						if (pcmnread < 0) {
							exit(EXIT_FAILURE);
						}
					}
				}
			}

			nBytesEncoded = opus_encode(enc, buffer_in, 960, opus_enc_packet,
					4000);
			if (nBytesEncoded < 0) {
				fprintf(stderr, "Error opus_encode\n");
				exit(EXIT_FAILURE);
			}

			for(int i = 0; i < nBytesEncoded; ++i)
				opus_enc_packet[i] ^= (xor_cipher_feed_1[i] ^ xor_cipher_feed_2[i]);

			if ((bytesNsent = sendto(sockfd, opus_enc_packet, nBytesEncoded, 0,
					(struct sockaddr*) &remote_addr, remote_addr_len)) < 0) {
				perror("sendto()");
				exit(EXIT_FAILURE);
			}
		}

		fprintf(stderr, "\r[Encoded bytes: %d, Bytes Sent: %ld] [Bytes Read: %ld, Bytes Decoded: %d]", nBytesEncoded, bytesNsent, bytesNread, nBytesDecoded);
	}

	snd_pcm_drain(capture);
	snd_pcm_close(capture);
	opus_encoder_destroy(enc);

	snd_pcm_drain(playback);
	snd_pcm_close(playback);
	opus_decoder_destroy(dec);

	close(sockfd);

	return 0;
}
