/* retro_core.c
 * Copyright (c) 2014, Adrien Plazas, All rights reserved.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
 */

#include "retro_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/* 
 * A thread local global variable used to mimic a closure.
 */
static __thread retro_core_t *thread_global_retro_core;
/*
void retro_core_new       (char *library_path) {
	retro_core_t *this = (retro_core_t *) malloc (sizeof (retro_core_t));
	retro_core_construct (this, library_path);
	
	return this;
}
*/
void retro_core_construct (retro_core_t *this, char *library_path) {
	this->library = retro_library_new (library_path);
}

void retro_core_finalize (retro_core_t *this) {
	retro_core_deinit (this);
	retro_library_free (this->library);
}

/*
 * Callback setters.
 * 
 * this: a pointer to a retro_core_t to modify.
 * cb:   the callback to set.
 * 
 * lambda: a phony closure, getting the value of this from the
 * thread local global variable thread_global_retro_core.
 * 
 * Set lambda as a callback for this->library
 */

void retro_core_set_environment (retro_core_t *this, retro_core_environment_cb_t cb, void *user_data) {
	this->environment_cb = cb;
	this->environment_data = user_data;
	bool lambda (unsigned cmd, void *data) {
		retro_core_t *core = thread_global_retro_core;
		if (core) return core->environment_cb (cmd, data, core->environment_data);
		// TODO g_assert_not_reached
		return false;
	}
	
	retro_library_set_environment (this->library, lambda);
}

void retro_core_set_video_refresh (retro_core_t *this, retro_core_video_refresh_cb_t cb, void *user_data) {
	this->video_refresh_cb = cb;
	this->video_refresh_data = user_data;
	void lambda (const void *data, unsigned width, unsigned height, size_t pitch) {
		retro_core_t *core = thread_global_retro_core;
		if (core) core->video_refresh_cb (data, width, height, pitch, core->video_refresh_data);
		// TODO g_assert_not_reached
	}
	
	retro_library_set_video_refresh (this->library, lambda);
}

void retro_core_set_audio_sample (retro_core_t *this, retro_core_audio_sample_cb_t cb, void *user_data) {
	this->audio_sample_cb = cb;
	this->audio_sample_data = user_data;
	void lambda (int16_t left, int16_t right) {
		retro_core_t *core = thread_global_retro_core;
		if (core) core->audio_sample_cb (left, right, core->audio_sample_data);
		// TODO g_assert_not_reached
	}
	
	retro_library_set_audio_sample (this->library, lambda);
}

void retro_core_set_audio_sample_batch (retro_core_t *this, retro_core_audio_sample_batch_cb_t cb, void *user_data) {
	this->audio_sample_batch_cb = cb;
	this->audio_sample_batch_data = user_data;
	size_t lambda (const int16_t *data, size_t frames) {
		retro_core_t *core = thread_global_retro_core;
		if (core) return core->audio_sample_batch_cb (data, frames, core->audio_sample_batch_data);
		// TODO g_assert_not_reached
		return 0;
	}
	
	retro_library_set_audio_sample_batch (this->library, lambda);
}

void retro_core_set_input_poll (retro_core_t *this, retro_core_input_poll_cb_t cb, void *user_data) {
	this->input_poll_cb = cb;
	this->input_poll_data = user_data;
	void lambda (void) {
		retro_core_t *core = thread_global_retro_core;
		if (core) return core->input_poll_cb (core->input_poll_data);
		// TODO g_assert_not_reached
	}
	
	retro_library_set_input_poll (this->library, lambda);
}

void retro_core_set_input_state (retro_core_t *this, retro_core_input_state_cb_t cb, void *user_data) {
	this->input_state_cb = cb;
	this->input_state_data = user_data;
	int16_t lambda (unsigned port, unsigned device, unsigned index, unsigned id) {
		retro_core_t *core = thread_global_retro_core;
		if (core) return core->input_state_cb (port, device, index, id, core->input_state_data);
		// TODO g_assert_not_reached
		return 0;
	}
	
	retro_library_set_input_state (this->library, lambda);
}

/* 
 * The next functions start a new thread, store "this" in the thread local
 * global variable thread_global_retro_core and call the actual
 * library function from within this thread.
 * 
 * Callbacks called within the library function will live in a thread where
 * thread_global_retro_core as the same value as the calling function's "this".
 * 
 * It allows to mimic a closure where "this" exists without interfering with
 * other instances of retro_core_t and so to isolate libretro libraries and
 * their environment, overriding libretro's design faults.
 */

void retro_core_init (retro_core_t *this) {
	if (thread_global_retro_core == this) {
		return retro_library_init (this->library);
	}
	
	void *lambda (void *arg) {
		thread_global_retro_core = this;
		retro_library_init (this->library);
		pthread_exit (NULL);
	}
	
	pthread_t thread;
	
	if (pthread_create (&thread, NULL, lambda, "1") < 0) {
		fprintf (stderr, "pthread_create error for thread 1\n");
		return;
	}
	
	pthread_join (thread, NULL);
}

void retro_core_deinit (retro_core_t *this) {
	if (thread_global_retro_core == this) {
		return retro_library_deinit (this->library);
	}
	
	void *lambda (void *arg) {
		thread_global_retro_core = this;
		retro_library_deinit (this->library);
		pthread_exit (NULL);
	}
	
	pthread_t thread;
	
	if (pthread_create (&thread, NULL, lambda, "1") < 0) {
		fprintf (stderr, "pthread_create error for thread 1\n");
		return;
	}
	
	pthread_join (thread, NULL);
}

unsigned retro_core_api_version (retro_core_t *this) {
	if (thread_global_retro_core == this) {
		return retro_library_api_version (this->library);
	}
	
	unsigned result;
	
	void *lambda (void *arg) {
		thread_global_retro_core = this;
		result = retro_library_api_version (this->library);
		pthread_exit (NULL);
	}
	
	pthread_t thread;
	
	if (pthread_create (&thread, NULL, lambda, "1") < 0) {
		fprintf (stderr, "pthread_create error for thread 1\n");
		return;
	}
	
	pthread_join (thread, NULL);
	
	return result;
}

void retro_core_get_system_info (retro_core_t *this, struct retro_system_info *info) {
	if (thread_global_retro_core == this) {
		return retro_library_get_system_info (this->library, info);
	}
	
	void *lambda (void *arg) {
		thread_global_retro_core = this;
		retro_library_get_system_info (this->library, info);
		pthread_exit (NULL);
	}
	
	pthread_t thread;
	
	if (pthread_create (&thread, NULL, lambda, "1") < 0) {
		fprintf (stderr, "pthread_create error for thread 1\n");
		return;
	}
	
	pthread_join (thread, NULL);
}

void retro_core_get_system_av_info (retro_core_t *this, struct retro_system_av_info *info) {
	if (thread_global_retro_core == this) {
		return retro_library_get_system_av_info (this->library, info);
	}
	
	void *lambda (void *arg) {
		thread_global_retro_core = this;
		retro_library_get_system_av_info (this->library, info);
		pthread_exit (NULL);
	}
	
	pthread_t thread;
	
	if (pthread_create (&thread, NULL, lambda, "1") < 0) {
		fprintf (stderr, "pthread_create error for thread 1\n");
		return;
	}
	
	pthread_join (thread, NULL);
}

//void retro_core_set_controller_port_device (unsigned port, unsigned device);

//void retro_core_reset (retro_core_t *this);

void retro_core_run (retro_core_t *this) {
	if (thread_global_retro_core == this) {
		return retro_library_run (this->library);
	}
	
	void *lambda (void *arg) {
		thread_global_retro_core = this;
		retro_library_run (this->library);
		pthread_exit (NULL);
	}
	
	pthread_t thread;
	
	if (pthread_create (&thread, NULL, lambda, "1") < 0) {
		fprintf (stderr, "pthread_create error for thread 1\n");
		return;
	}
	
	pthread_join (thread, NULL);
}
/*
size_t retro_core_serialize_size (retro_core_t *this);

bool retro_core_serialize (retro_core_t *this, void *data, size_t size);
bool retro_core_unserialize (retro_core_t *this, const void *data, size_t size);

void retro_core_cheat_reset (retro_core_t *this);
void retro_core_cheat_set (retro_core_t *this, unsigned index, bool enabled, const char *code);
*/
bool retro_core_load_game (retro_core_t *this, const struct retro_game_info *game) {
	if (thread_global_retro_core == this) {
		return retro_library_load_game (this->library, game);
	}
	
	bool result;
	
	void *lambda (void *arg) {
		thread_global_retro_core = this;
		result = retro_library_load_game(this->library, game);
		pthread_exit (NULL);
	}
	
	pthread_t thread;
	
	if (pthread_create (&thread, NULL, lambda, "1") < 0) {
		fprintf (stderr, "pthread_create error for thread 1\n");
		return;
	}
	
	pthread_join (thread, NULL);
	
	return result;
}
/*
bool retro_core_load_game_special (retro_core_t *this, unsigned game_type, const struct retro_game_info *info, size_t num_info) {
	if (thread_global_retro_core == this) {
		return retro_library_load_game_special (this->library, game_type, info, num_info);
	}
	
	bool result;
	
	void *lambda (void *arg) {
		thread_global_retro_core = this;
		result = retro_library_load_game_special(this->library, game_type, info, num_info);
		pthread_exit (NULL);
	}
	
	pthread_t thread;
	
	if (pthread_create (&thread, NULL, lambda, "1") < 0) {
		fprintf (stderr, "pthread_create error for thread 1\n");
		return;
	}
	
	pthread_join (thread, NULL);
	
	return result;
}
/*
void retro_core_unload_game (retro_core_t *this);

unsigned retro_core_get_region (retro_core_t *this);

void *retro_core_get_memory_data (retro_core_t *this, unsigned id);
size_t retro_core_get_memory_size (retro_core_t *this, unsigned id);
*/
