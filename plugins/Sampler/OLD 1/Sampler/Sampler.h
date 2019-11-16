/*
 * Sampler.h
 *
 * Copyright (c) 2019 Lost Robot [r94231/at/gmail/dot/com]
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


#ifndef SAMPLER_H
#define SAMPLER_H

#include "Instrument.h"
#include "InstrumentView.h"
#include "Knob.h"
#include "MemoryManager.h"
#include "PixmapButton.h"
#include "LcdSpinBox.h"

class oscillator;
class SamplerView;

class sSynth
{
	MM_OPERATORS
public:
	sSynth( NotePlayHandle * _nph, const sample_rate_t _sample_rate, std::vector<std::vector<float>> (&soundSamples)[2], QVector<FloatModel *> (&startPoint) );
	virtual ~sSynth();
	
	void nextStringSample( sampleFrame &outputSample, QVector<FloatModel *> (&volume), std::vector<std::vector<float>> (&soundSamples)[2], QVector<FloatModel *> (&startNote), QVector<FloatModel *> (&endNote), QVector<FloatModel *> (&velLeft), QVector<FloatModel *> (&velRight), float detuneRatio, QVector<FloatModel *> (&detune), QVector<FloatModel *> (&startPoint), QVector<FloatModel *> (&loopPoint1), QVector<FloatModel *> (&loopPoint2), QVector<FloatModel *> (&endPoint), QVector<FloatModel *> (&loopMode), QVector<FloatModel *> (&panning), QVector<FloatModel *> (&crossfade), QVector<FloatModel *> (&timestretchAmount) );

	inline float detuneWithCents(float pitchValue, float detuneValue);

private:
	NotePlayHandle* nph;

	std::vector<float> sample_index;
	std::vector<float> playDirection;
	int noteDuration = -1;

	const sample_rate_t sample_rate;
} ;



class Sampler : public Instrument
{
	Q_OBJECT
public:
	Sampler(InstrumentTrack * _instrument_track );
	virtual ~Sampler();

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
	IntModel sampleNumber;
	FloatModel detuneRatio;

	QVector<FloatModel *> startNote;
	QVector<FloatModel *> endNote;
	QVector<FloatModel *> velLeft;
	QVector<FloatModel *> velRight;

	QVector<FloatModel *> volume;
	QVector<FloatModel *> detune;
	QVector<FloatModel *> startPoint;
	QVector<FloatModel *> loopPoint1;
	QVector<FloatModel *> loopPoint2;
	QVector<FloatModel *> endPoint;
	QVector<FloatModel *> loopMode;
	QVector<FloatModel *> panning;
	QVector<FloatModel *> crossfade;
	QVector<FloatModel *> timestretchAmount;

	std::vector<std::vector<float>> soundSamples[2];
	
	friend class SamplerView;
} ;



class SamplerView : public InstrumentView
{
	Q_OBJECT
public:
	SamplerView( Instrument * _instrument, QWidget * _parent );

	QSize sizeHint() const override { return QSize(800, 250); }

	virtual ~SamplerView() {};

protected slots:
	void loadSampleClicked();
	void sampleNumberChanged();

private:
	PixmapButton * loadSampleBtn;

	QVector<Knob *> startNoteKnob;
	QVector<Knob *> endNoteKnob;
	QVector<Knob *> velLeftKnob;
	QVector<Knob *> velRightKnob;
	QVector<PixmapButton *> editButton;

	Knob * volumeKnob;
	Knob * detuneRatioKnob;
	Knob * detuneKnob;
	Knob * startPointKnob;
	Knob * loopPoint1Knob;
	Knob * loopPoint2Knob;
	Knob * endPointKnob;
	Knob * loopModeKnob;
	Knob * panningKnob;
	Knob * crossfadeKnob;
	Knob * timestretchAmountKnob;

	LcdSpinBox * sampleNumberBox;

	Sampler * b = castModel<Sampler>();

	static QPixmap * s_artwork;
} ;



#endif
