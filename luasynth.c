#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>

#include <alloca.h>

#define _POSIX_C_SOURCE
#define ALSA_PCM_NEW_HW_PARAMS_API
#include <sys/time.h>
#include <alsa/asoundlib.h>

#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/inotify.h>

#include </usr/include/lua5.1/lua.h>
#include </usr/include/lua5.1/lauxlib.h>
#include </usr/include/lua5.1/lualib.h>

lua_State *L = NULL;
lua_State *last_L = NULL;
int silence;
const char *_filename;

int inotify_fd;
size_t bufsiz;
struct inotify_event *event;
struct timeval timeouts= { .tv_sec = 0, .tv_usec = 0 };

int16_t* buffer;

int reload_script(const char *filename);
void setup_reload(const char *filename);
double get_sample(double time, double angle);

/* Thread functions and data*/
typedef struct sample_data {
    unsigned int period;
	snd_pcm_uframes_t frames;
    int rc;
	snd_pcm_t *handle;
} sample_data;


void play_sample(sample_data* sd);

int main(int argc, char **argv) {
	//snd_pcm_t *handle;
	int dir;

	if (argc == 2) {
		setup_reload(argv[1]);
	}
	else {
		fprintf(stderr, "Please specify a Lua synth file to play.\n");
		return 1;
	}

    sample_data* sd = (sample_data*) malloc(sizeof(sample_data));

	//int rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	//int rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
    sd->rc = snd_pcm_open(&(sd->handle), "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);

	if (sd->rc < 0) {
		fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(sd->rc));
		abort();
	}

	snd_pcm_hw_params_t *params;
	snd_pcm_hw_params_alloca(&params);
	snd_pcm_hw_params_any(sd->handle, params);

	snd_pcm_hw_params_set_access(sd->handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(sd->handle, params, SND_PCM_FORMAT_S16_LE);
	snd_pcm_hw_params_set_channels(sd->handle, params, 2);

	unsigned int rate = 44100;
	snd_pcm_hw_params_set_rate_near(sd->handle, params, &rate, &dir);

	sd->frames = 32;
	snd_pcm_hw_params_set_period_size_near(sd->handle, params, &(sd->frames), &dir);

	sd->rc = snd_pcm_hw_params(sd->handle, params);
	if (sd->rc < 0) {
		fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(sd->rc));
		abort();
	}

	snd_pcm_hw_params_get_period_size(params, &(sd->frames), &dir);
	//int16_t *buffer = malloc(frames * 2 * sizeof(uint16_t));
	buffer = malloc((sd->frames) * 2 * sizeof(uint16_t));

	//unsigned int period;
	snd_pcm_hw_params_get_period_time(params, &(sd->period), &dir);

    pthread_t sample_th;
    pthread_create(&sample_th, NULL, play_sample, sd);
    pthread_join(sample_th, NULL);

	snd_pcm_drain(sd->handle);
	snd_pcm_close(sd->handle);
	free(buffer);

	return 0;
}

//period, frames, rc, handle,
void play_sample(sample_data* sd) {
	for (uint64_t time = 0;; time += sd->period) {
		for (size_t j = 0; j < sd->frames * 2; j++) {

			double sample;
			if (j & 0x1) {
				sample = get_sample((time + ((j >> 1) * sd->period / sd->frames)) / 1000000.0, 0.0);
			}
			else {
				sample = get_sample((time + ((j >> 1) * sd->period / sd->frames)) / 1000000.0, 3.14159265);
			}

			sample = (sample * 32768.0f);
			int32_t sample32 = sample;
			if (sample32 > 0x7FFF)  sample32 = 0x7FFF;
			if (sample32 < -0x8000) sample32 = -0x8000;

			buffer[j] = sample32;
		}

		sd->rc = snd_pcm_writei(sd->handle, buffer, sd->frames);

		if (sd->rc == -EPIPE) {
			snd_pcm_prepare(sd->handle);
		}
	}
}

int reload_script(const char *filename) {
	silence = 0;

	printf("(re)loading script %s\n", filename);

	if (last_L) {
		lua_close(last_L);
	}

	last_L = L;

	L = lua_open();
	luaL_openlibs(L);

	if (luaL_dofile(L, filename)) {
		fprintf(stderr, "%s\n", lua_tostring(L, 1));
		lua_close(L);
		L = last_L;
		last_L = NULL;
		return 1;
	}

	return 0;
}

void setup_reload(const char *filename) {
	_filename = filename;
	
	inotify_fd = inotify_init();

	if (inotify_fd < 0) {
		perror("inotify_init");
		abort();
	}

	inotify_add_watch(inotify_fd, filename, IN_MODIFY);

	bufsiz = sizeof(struct inotify_event) + 16;
	event = malloc(bufsiz);

	reload_script(_filename);
}

double get_sample(double time, double angle) {
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(inotify_fd, &rfds);

	if (select(inotify_fd + 1, &rfds, NULL, NULL, &timeouts)) {
		read(inotify_fd, event, bufsiz);
		inotify_add_watch(inotify_fd, _filename, IN_MODIFY);
		reload_script(_filename);
	}

	if (!L) return 0.0;

	lua_getfield(L, LUA_GLOBALSINDEX, "main_sound");
	lua_pushnumber(L, time);
	lua_pushnumber(L, angle);
	if (lua_pcall(L, 2, 1, 0)) {
		if (!silence) fprintf(stderr, "%s\n", lua_tostring(L, 1));
		lua_pop(L, 1);
		if (last_L) {
			lua_close(L);
			L = last_L;
			last_L = NULL;
		}
		else {
			silence = 1;
		}
		return 0.0;
	}

	double sample = lua_tonumber(L, -1);
	lua_pop(L, 1);

	return sample;
}
