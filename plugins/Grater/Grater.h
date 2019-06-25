/*
 * Grater.h
 *
 * Copyright (c) 2019 Robert Daniel Black AKA Lost Robot <r94231/at/gmail/dot/com>
 * 
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#ifndef GRATER_H
#define GRATER_H

#include "Instrument.h"
#include "InstrumentView.h"
#include "Knob.h"
#include "PixmapButton.h"
#include "LedCheckbox.h"
#include "MemoryManager.h"
#include "SampleBuffer.h"

class oscillator;
class GraterView;
class gSynthVoice;
class gSynth;


class GraterKnob: public Knob
{
	Q_OBJECT
	using Knob::Knob;

public:
	bool hasSlowMovement = false;

protected:
	virtual float getValue( const QPoint & _p );
};


class gSynth
{
	MM_OPERATORS
public:
	gSynth( NotePlayHandle * _nph, const sample_rate_t _sample_rate, float grain, float position, std::vector<float> (&soundSample)[2], float spray, float fmAmount, float fmFreq, float pmAmount, float pmFreq, float ramp, float pitchRand, float stereoGrain, float voiceNum, float posDist );
	virtual ~gSynth();
	
	void nextStringSample( sampleFrame &outputSample, float grain, float position, std::vector<float> (&soundSample)[2], float speed, bool speedEnabled, float spray, float fmAmount, float fmFreq, float pmAmount, float pmFreq, float ramp, float pitchRand, float stereoGrain, float voiceNum, float posDist );

	gSynthVoice * synthVoices[128][2];

	float outputSample1;
	float outputSample2;


private:
	int sample_index;
	NotePlayHandle* nph;
	const sample_rate_t sample_rate;

	float sample_realindex;
} ;


class gSynthVoice
{
	MM_OPERATORS
public:
	gSynthVoice( NotePlayHandle * _nph, const sample_rate_t _sample_rate, float orig_position, std::vector<float> (&soundSample)[2], float spray, float fmAmount, float fmFreq, float pmAmount, float pmFreq, float ramp, float pitchRand, float stereoGrain, float voiceNum, float posDist );
	virtual ~gSynthVoice();
	
	void nextStringSample( float &outputSample, float grain, float orig_position, std::vector<float> (&soundSample)[2], int whichVoice, float speed, bool speedEnabled, float spray, float fmAmount, float fmFreq, float pmAmount, float pmFreq, float ramp, float pitchRand, float stereoGrain, float voiceNum, float posDist );

	inline float detuneWithCents( float pitchValue, float detuneValue );

	float voiceDuration = -1;

	float sprayPos = 0;
	float randomPitchVal = 0;

	float position;

private:
	inline float realfmod( float k, float n );

	int sample_index;
	NotePlayHandle* nph;
	const sample_rate_t sample_rate;

	double speedProgress = 0;

	double sample_realindex;

	double fmIndex = 0;
	double pmIndex = 0;

	double fmDetune;
	double pmShift;

	double sampleRateRatio = 1;
} ;


class Grater : public Instrument
{
	Q_OBJECT
public:
	Grater(InstrumentTrack * _instrument_track );
	virtual ~Grater();

	virtual void playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer );
	virtual void deleteNotePluginData( NotePlayHandle * _n );


	virtual void saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	virtual QString nodeName() const;

	virtual f_cnt_t desiredReleaseFrames() const
	{
		return( 64 );
	}

	virtual PluginView * instantiateView( QWidget * _parent );


private:
	std::vector<float> soundSample[2];

	FloatModel grain;
	FloatModel position;
	FloatModel speed;
	BoolModel speedEnabled;
	FloatModel spray;
	FloatModel fmAmount;
	FloatModel fmFreq;
	FloatModel pmAmount;
	FloatModel pmFreq;
	FloatModel ramp;
	FloatModel pitchRand;
	FloatModel stereoGrain;
	FloatModel voiceNum;
	FloatModel posDist;
	
	friend class GraterView;
} ;



class GraterView : public InstrumentView
{
	Q_OBJECT
public:
	GraterView( Instrument * _instrument,
					QWidget * _parent );

	virtual ~GraterView() {};

protected slots:
	void usrWaveClicked();

private:
	virtual void modelChanged();

	GraterKnob * grainKnob;
	GraterKnob * positionKnob;
	GraterKnob * speedKnob;
	LedCheckBox * speedEnabledToggle;
	GraterKnob * sprayKnob;
	GraterKnob * fmAmountKnob;
	GraterKnob * fmFreqKnob;
	GraterKnob * pmAmountKnob;
	GraterKnob * pmFreqKnob;
	GraterKnob * rampKnob;
	GraterKnob * pitchRandKnob;
	GraterKnob * stereoGrainKnob;
	GraterKnob * voiceNumKnob;
	GraterKnob * posDistKnob;
	PixmapButton * usrWaveBtn;

	static QPixmap * s_artwork;

} ;



#endif
