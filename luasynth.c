#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <alloca.h>

#define _POSIX_C_SOURCE
#define ALSA_PCM_NEW_HW_PARAMS_API
#include <sys/time.h>
#include <alsa/asoundlib.h>

#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/inotify.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

lua_State *L = NULL;
lua_State *last_L = NULL;
int silence;
const char *_filename;

int inotify_fd;
size_t bufsiz;
struct inotify_event *event;
struct timeval timeout = { .tv_sec = 0, .tv_usec = 0 };

int reload_script(const char *filename);
void setup_reload(const char *filename);
double get_sample(double time, double angle);

int main(int argc, char **argv) {
	snd_pcm_t *handle;
	int dir;

	if (argc == 2) {
		setup_reload(argv[1]);
	}
	else {
		fprintf(stderr, "Please specify a Lua synth file to play.\n");
		return 1;
	}

	int rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if (rc < 0) {
		fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(rc));
		abort();
	}

	snd_pcm_hw_params_t *params;
	snd_pcm_hw_params_alloca(&params);
	snd_pcm_hw_params_any(handle, params);

	snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
	snd_pcm_hw_params_set_channels(handle, params, 2);

	unsigned int rate = 44100;
	snd_pcm_hw_params_set_rate_near(handle, params, &rate, &dir);

	snd_pcm_uframes_t frames = 32;
	snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

	rc = snd_pcm_hw_params(handle, params);
	if (rc < 0) {
		fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));
		abort();
	}

	snd_pcm_hw_params_get_period_size(params, &frames, &dir);
	int16_t *buffer = malloc(frames * 2 * sizeof(uint16_t));

	unsigned int period;
	snd_pcm_hw_params_get_period_time(params, &period, &dir);

	for (uint64_t time = 0;; time += period) {

		for (size_t j = 0; j < frames * 2; j++) {
			double sample;
			if (j & 0x1) {
				sample = get_sample((time + ((j >> 1) * period / frames)) / 1000000.0, 0.0);
			}
			else {
				sample = get_sample((time + ((j >> 1) * period / frames)) / 1000000.0, 3.14159265);
			}

			sample = (sample * 32768.0f);
			int32_t sample32 = sample;
			if (sample32 > 0x7FFF)  sample32 = 0x7FFF;
			if (sample32 < -0x8000) sample32 = -0x8000;

			buffer[j] = sample32;
		}

		rc = snd_pcm_writei(handle, buffer, frames);

		if (rc == -EPIPE) {
			snd_pcm_prepare(handle);
		}
	}

	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	free(buffer);

	return 0;
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

	if (select(inotify_fd + 1, &rfds, NULL, NULL, &timeout)) {
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
