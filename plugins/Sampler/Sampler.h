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
	
	void nextStringSample( sampleFrame &outputSample, QVector<FloatModel *> (&volume), std::vector<std::vector<float>> (&soundSamples)[2], QVector<FloatModel *> (&startNote), QVector<FloatModel *> (&endNote), QVector<FloatModel *> (&velLeft), QVector<FloatModel *> (&velRight), float detuneRatio, QVector<FloatModel *> (&detune), QVector<FloatModel *> (&startPoint), QVector<FloatModel *> (&loopPoint1), QVector<FloatModel *> (&loopPoint2), QVector<FloatModel *> (&endPoint), QVector<FloatModel *> (&loopMode), QVector<FloatModel *> (&panning), QVector<FloatModel *> (&crossfade), QVector<FloatModel *> (&timestretchAmount), QVector<FloatModel *> (&distortionType), QVector<FloatModel *> (&distortionAmount), QVector<FloatModel *> (&volPredelay), QVector<FloatModel *> (&volAttack), QVector<FloatModel *> (&volHold), QVector<FloatModel *> (&volDecay), QVector<FloatModel *> (&volSustain), QVector<FloatModel *> (&volRelease), int frame, QVector<FloatModel *> (&amFreq), QVector<FloatModel *> (&amAmnt), QVector<FloatModel *> (&fmFreq), QVector<FloatModel *> (&fmAmnt), QVector<FloatModel *> (&filtCutoff), QVector<FloatModel *> (&filtReso), QVector<FloatModel *> (&filtMorph), QVector<FloatModel *> (&grainSize) );

	inline float detuneWithCents(float pitchValue, float detuneValue);

	inline float calcEnvelope(int sampNum, int envNum, int frame, float predelay, float attack, float hold, float decay, float sustain, float release);

	//inline void calcFilter( sample_t &outSamp, sample_t inSamp, int which, float lpCutoff, sample_rate_t Fs, NotePlayHandle * nph, float feedback, float resonance, float combFreq )

private:
	NotePlayHandle* nph;

	std::vector<double> sample_index;
	std::vector<float> playDirection;
	std::vector<float> amOscIndex;
	std::vector<float> fmOscIndex;
	int noteDuration = -1;
	int noteReleaseTime = 0;

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

	virtual PluginView * instantiateView( QWidget * _parent );

	virtual f_cnt_t desiredReleaseFrames() const;

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
	QVector<FloatModel *> distortionType;
	QVector<FloatModel *> distortionAmount;
	QVector<FloatModel *> volPredelay;
	QVector<FloatModel *> volAttack;
	QVector<FloatModel *> volHold;
	QVector<FloatModel *> volDecay;
	QVector<FloatModel *> volSustain;
	QVector<FloatModel *> volRelease;
	QVector<FloatModel *> amFreq;
	QVector<FloatModel *> amAmnt;
	QVector<FloatModel *> fmFreq;
	QVector<FloatModel *> fmAmnt;
	QVector<FloatModel *> filtCutoff;
	QVector<FloatModel *> filtReso;
	QVector<FloatModel *> filtMorph;
	QVector<FloatModel *> grainSize;

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
	std::vector<float> m_filtX[32][2][4] = {{{0}}};// [filter number][channel][samples back in time]
	std::vector<float> m_filtY[32][2][4] = {{{0}}};// [filter number][channel][samples back in time]

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
	Knob * distortionTypeKnob;
	Knob * distortionAmountKnob;
	Knob * volPredelayKnob;
	Knob * volAttackKnob;
	Knob * volHoldKnob;
	Knob * volDecayKnob;
	Knob * volSustainKnob;
	Knob * volReleaseKnob;
	Knob * amFreqKnob;
	Knob * amAmntKnob;
	Knob * fmFreqKnob;
	Knob * fmAmntKnob;
	Knob * filtCutoffKnob;
	Knob * filtResoKnob;
	Knob * filtMorphKnob;
	Knob * grainSizeKnob;

	LcdSpinBox * sampleNumberBox;

	Sampler * b = castModel<Sampler>();

	static QPixmap * s_artwork;
} ;



#endif
