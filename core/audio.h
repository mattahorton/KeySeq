//-----------------------------------------------------------------------------
// name: audio.h
// desc: audio stuff
//
// author: Matt Horton (mattah@ccrma.stanford.edu), after Ge Wang
//   date: 2014
//-----------------------------------------------------------------------------
#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <vector>


// init audio
bool audio_init( unsigned int srate, unsigned int frameSize, unsigned channels );
// start audio
bool audio_start();

// play some notes
void play(std::vector<int> &channels, std::vector<float> &pitches, std::vector<float> &vels );



#endif
