/*
Copyright 2013-2015 Rohit Nirmal

This file is part of Clonepoint.

Clonepoint is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Clonepoint is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Clonepoint.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "audio.h"
#include "file.h"
#include "global.h"
#include "stb_vorbis.c"

AudioManager::AudioManager()
{
	_device = alcOpenDevice(NULL);
	_context = alcCreateContext(_device, NULL);
	alcMakeContextCurrent(_context);

	ALCint maxmono = 0, maxstereo = 0;
	alcGetIntegerv(_device, ALC_MONO_SOURCES, 1, &maxmono);
	alcGetIntegerv(_device, ALC_STEREO_SOURCES, 1, &maxstereo);

	ALuint src;
	for (unsigned int i = 0; i < MAX_SOURCES; i++)
	{
		src = 0;
		alGenSources(1, &src);
		LOGF((stdout, "Pushing source %i into buffer.\n", src));
		_loadedSources.push_back(src);
	}

	_musicSource = 0;
	_mapMusicBuffer = 0;
	_mapMusicCrosslinkBuffer = 0;
	alGenSources(1, &_musicSource);

	loadWAV("./data/sounds/alarm.wav");
	loadWAV("./data/sounds/door_open.wav");
	loadWAV("./data/sounds/door_close.wav");
	loadWAV("./data/sounds/circuitbox.wav");
	loadWAV("./data/sounds/pistol.wav");
	loadWAV("./data/sounds/pistol_ready.wav");
	loadWAV("./data/sounds/menu_click.wav");
	loadWAV("./data/sounds/switch.wav");
	loadWAV("./data/sounds/link.wav");
	loadWAV("./data/sounds/jump.wav");
	loadWAV("./data/sounds/enter_crosslink.wav");
	loadWAV("./data/sounds/exit_crosslink.wav");
	loadWAV("./data/sounds/punch1.wav");
	loadWAV("./data/sounds/punch2.wav");
	loadWAV("./data/sounds/glass_break.wav");
	loadWAV("./data/sounds/elevator_arrive.wav");
	loadWAV("./data/sounds/elevator_leave.wav");
	loadWAV("./data/sounds/elevator_decelerate.wav");
	_mainmenuMusicBuffer = bufferFromOGG("./data/music/ontheground.ogg", NULL);
	_playingMenuMusic = false;
}

AudioManager::~AudioManager()
{
	size_t i;

	for (i = 0; i < _loadedSources.size(); i++)
	{
		alDeleteSources(1, &_loadedSources[i]);
	}

	_loadedSources.clear();

	alDeleteSources(1, &_musicSource);

	if (_mapMusicBuffer != 0)
	{
		alDeleteBuffers(1, &_mapMusicBuffer);
	}

	if (_mainmenuMusicBuffer != 0)
	{
		alDeleteBuffers(1, &_mainmenuMusicBuffer);
	}

	if (_mapMusicCrosslinkBuffer != 0)
	{
		alDeleteBuffers(1, &_mapMusicCrosslinkBuffer);
	}

	std::map<std::string, ALuint>::iterator it;

	for (it = _loadedBuffers.begin(); it != _loadedBuffers.end(); it++)
	{
		ALuint buf = it->second;
		LOGF((stdout, "Deleting OpenAL buffer %i.\n", buf));
		alDeleteBuffers(1, &buf);
	}

	alcMakeContextCurrent(NULL);
	if (_context) alcDestroyContext(_context);
	if (_device) alcCloseDevice(_device);
}

void AudioManager::playSound(std::string filename)
{
	ALuint src = getNextAvailableSource();
	ALuint buf = getBuffer(filename);

	alSourcei(src, AL_BUFFER, buf);
	alSource3f(src, AL_POSITION, 0, 0, 0);
	alSourcePlay(src);
}

void AudioManager::playMapMusic(std::string filename)
{
	if (_mapMusicBuffer != 0)
	{
		alDeleteBuffers(1, &_mapMusicBuffer);
		alDeleteBuffers(1, &_mapMusicCrosslinkBuffer);
		_mapMusicBuffer = 0;
		_mapMusicCrosslinkBuffer = 0;
	}

	if (filename == "")
	{
		_playingMenuMusic = false;
		return;
	}

	_mapMusicBuffer = bufferFromOGG("./data/music/" + filename, &_mapMusicCrosslinkBuffer);

	if (_playingMenuMusic)
	{
		alSourceStop(_musicSource);
	}
	_playingMenuMusic = false;
	playMusic(_mapMusicBuffer);
}

void AudioManager::playMenuMusic()
{
	if (_playingMenuMusic)
		return;

	_playingMenuMusic = true;
	playMusic(_mainmenuMusicBuffer);
}

void AudioManager::playMusic(ALuint buf)
{
	if (buf == 0)
	{
		return;
	}

	alSourcei(_musicSource, AL_BUFFER, buf);
	alSource3f(_musicSource, AL_POSITION, 0, 0, 0);
	alSourcei(_musicSource, AL_LOOPING, 1);
	alSourcePlay(_musicSource);
}

void AudioManager::stopMusic()
{
	alDeleteBuffers(1, &_mapMusicBuffer);
	_mapMusicBuffer = 0;
	alSourceStop(_musicSource);
}

void AudioManager::playSound3D(const char* filename, float x, float y, float z)
{
	ALuint src = getNextAvailableSource();
	ALuint buf = getBuffer(filename);

	alSourcei(src, AL_BUFFER, buf);
	alSource3f(src, AL_POSITION, x, y, z);
	alSourcePlay(src);
}

void AudioManager::loadWAV(std::string filename)
{
	SDL_AudioSpec wav_spec;
	Uint32 wav_length = 0;
	Uint8* wav_buffer;

	if (SDL_LoadWAV(filename.c_str(), &wav_spec, &wav_buffer, &wav_length) != NULL)
	{
		ALenum format;
		switch(wav_spec.format)
		{
		case AUDIO_U8:
		case AUDIO_S8:
			format = wav_spec.channels == 2 ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
			break;
		case AUDIO_U16:
		case AUDIO_S16:
			format = wav_spec.channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
			break;
		default:
			LOGF((stderr, "Wave file %s is of unknown format!\n", filename.c_str()));
			SDL_FreeWAV(wav_buffer);
			return;
			break;
		}

		ALuint buf;

		alGenBuffers(1, &buf);
		alBufferData(buf, format, wav_buffer, wav_length, wav_spec.freq);
		SDL_FreeWAV(wav_buffer);

		int slash = filename.find_last_of("/");
		int dot = filename.find_last_of(".");

		std::string bufferString = filename.substr(slash + 1, dot - slash - 1);
		LOGF((stdout, "Storing %s into buffer %i.\n", bufferString.c_str(), buf));
		_loadedBuffers.insert(std::pair<std::string, ALuint>(bufferString, buf));
	}
	else
	{
		LOGF((stderr, "Couldn't load wave file %s\n", filename.c_str()));
		LOGF((stderr, "%s\n", SDL_GetError()));
	}
}

ALuint AudioManager::bufferFromOGG(std::string filename, ALuint* filtered_buf)
{
	long size;
	char* file_buffer = file_read(filename.c_str(), &size);
	if (!file_buffer)
	{
		return 0;
	}

	stb_vorbis* ogg = stb_vorbis_open_memory((const unsigned char*)file_buffer, (int)size, NULL, NULL);
	if (!ogg)
	{
		return 0;
	}

	ALuint ret;

	stb_vorbis_info info = stb_vorbis_get_info( ogg );
	ALenum format = ( ( info.channels == 1 ) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16);

	int const length_samples = stb_vorbis_stream_length_in_samples(ogg) * info.channels;
	std::vector<ALshort> ogg_buffer(length_samples);
	std::vector<ALshort> filtered(length_samples);

	alGenBuffers(1, &ret);
	stb_vorbis_get_samples_short_interleaved(ogg, info.channels, ogg_buffer.data(), length_samples);

	if (filtered_buf != NULL)
	{
		//Apply low pass filter for to another buffer of the music for crosslink
		int i;
		float alpha = 0.05f;
		filtered[0] = ogg_buffer[0];
		for (i = 1; i < length_samples; i++)
		{
			float filter = (alpha * (float)ogg_buffer[i]) + (1.0f - alpha) * (float)filtered[i-1];
			filtered[i] = (ALshort)filter;
		}
		alGenBuffers(1, filtered_buf);
	}

	alBufferData(ret, format, ogg_buffer.data(), (length_samples * sizeof( ALshort)), info.sample_rate);
	if (filtered_buf != NULL)
	{
		alBufferData(*filtered_buf, format, filtered.data(), (length_samples * sizeof( ALshort)), info.sample_rate);
	}
	stb_vorbis_close(ogg);
	delete [] file_buffer;

	return ret;
}

ALuint AudioManager::getBuffer(std::string filename)
{
	std::map<std::string, ALuint>::iterator it = _loadedBuffers.find(filename);
	if (it == _loadedBuffers.end())
	{
		LOGF((stderr, "Failed to find buffer for %s\n", filename.c_str()));
		return 0;
	}

	return it->second;
}

ALuint AudioManager::getNextAvailableSource()
{
	int status;
	size_t i;
	for (i = 0; i < _loadedSources.size(); i++)
	{
		alGetSourcei(_loadedSources[i], AL_SOURCE_STATE, &status);
		if (status != AL_PLAYING)
		{
			return _loadedSources[i];
		}
	}

	//all sounds are currently playing!
	return 0;
}

void AudioManager::setSoundVolume(float volume)
{
	size_t i;
	for (i = 0; i < _loadedSources.size(); i++)
	{
		alSourcef(_loadedSources[i], AL_GAIN, volume);
	}
}

float AudioManager::getMusicTime()
{
	float result;

	alGetSourcef(_musicSource, AL_SEC_OFFSET, &result);

	return result;
}

void AudioManager::changeMusicMode(bool crosslink)
{
	float time = getMusicTime();
	int status;
	alGetSourcei(_musicSource, AL_SOURCE_STATE, &status);
	alSourceStop(_musicSource);
	playMusic(crosslink ? _mapMusicCrosslinkBuffer : _mapMusicBuffer);
	alSourcef(_musicSource, AL_SEC_OFFSET, time);
}

void AudioManager::setMusicVolume(float volume)
{
	alSourcef(_musicSource, AL_GAIN, volume);
}