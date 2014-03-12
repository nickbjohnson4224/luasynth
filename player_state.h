#ifndef STATE_H
#define STATE_H

#include <pthread.h>

int paused;
int pause_flag;
int quit;

pthread_mutex_t pause_tex;

void player();
#endif
