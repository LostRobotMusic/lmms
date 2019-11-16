/*
 * Sampler.cpp
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


#include <QDomElement>

#include "Sampler.h"
#include "base64.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "Knob.h"
#include "lmms_math.h"
#include "Mixer.h"
#include "NotePlayHandle.h"
#include "Oscillator.h"
#include "ToolTip.h"
#include "Song.h"
#include "SampleBuffer.h"

#include "embed.h"

#include "plugin_export.h"










#include <iostream>
using namespace std;










extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT sampler_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Sampler",
	QT_TRANSLATE_NOOP( "pluginBrowser", "A native sampler" ),
	"Lost Robot [r94231/at/gmail/dot/com]",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;

}


sSynth::sSynth( NotePlayHandle * _nph, const sample_rate_t _sample_rate, std::vector<std::vector<float>> (&soundSamples)[2], QVector<FloatModel *> (&startPoint) ) :
	nph( _nph ),
	sample_rate( _sample_rate )
{
	for( int a = 0; a < soundSamples[0].size(); ++a )
	{
		sample_index.push_back(startPoint[a]->value());
		playDirection.push_back(1);
	}
}


sSynth::~sSynth()
{
}


void sSynth::nextStringSample( sampleFrame &outputSample, QVector<FloatModel *> (&volume), std::vector<std::vector<float>> (&soundSamples)[2], QVector<FloatModel *> (&startNote), QVector<FloatModel *> (&endNote), QVector<FloatModel *> (&velLeft), QVector<FloatModel *> (&velRight), float detuneRatio, QVector<FloatModel *> (&detune), QVector<FloatModel *> (&startPoint), QVector<FloatModel *> (&loopPoint1), QVector<FloatModel *> (&loopPoint2), QVector<FloatModel *> (&endPoint), QVector<FloatModel *> (&loopMode), QVector<FloatModel *> (&panning), QVector<FloatModel *> (&crossfade), QVector<FloatModel *> (&timestretchAmount) )
{
	++noteDuration;

	const sample_rate_t sample_rate = Engine::mixer()->processingSampleRate();

	for( int a = 0; a < soundSamples[0].size(); ++a )
	{
		float index_jump = ((((nph->frequency() * exp2(detune[a]->value() / 1200.f)) - 440) * detuneRatio + 440) / 440.f) * (44100.f / sample_rate) * playDirection[a];
		sample_index[a] += index_jump;

		if( timestretchAmount[a]->value() != 1 && fmod( noteDuration, 1000 ) == 0 && noteDuration )
		{
			sample_index[a] += timestretchAmount[a]->value() * 1000 * index_jump - 1000 * index_jump;
		}

		if( loopMode[a]->value() == 2 )
		{
			if( sample_index[a] > loopPoint2[a]->value() )
			{
				sample_index[a] = loopPoint1[a]->value();
			}
		}
		if( loopMode[a]->value() == 3 )
		{
			if( sample_index[a] >= loopPoint2[a]->value() && playDirection[a] == 1 )
			{
				playDirection[a] = -1;
			}
			else if( sample_index[a] <= loopPoint1[a]->value() && playDirection[a] == -1 )
			{
				playDirection[a] = 1;
			}
		}

		if( sample_index[a] <= endPoint[a]->value() && nph->key() >= startNote[a]->value() && nph->key() <= endNote[a]->value() )
		{

			float velVol;
			if( nph->key() < velLeft[a]->value() )
			{
				velVol = ( nph->key() - startNote[a]->value() ) / ( velLeft[a]->value() - startNote[a]->value() );
			}
			else if( nph->key() <= velRight[a]->value() )
			{
				velVol = 1;
			}
			else
			{
				velVol = 1 - ( ( nph->key() - velRight[a]->value() ) / ( endNote[a]->value() - velRight[a]->value() ) );
			}

			sampleFrame oneSampleOutput = {0, 0};

			oneSampleOutput[0] = soundSamples[0][a][sample_index[a]];
			oneSampleOutput[1] = soundSamples[1][a][sample_index[a]];

			if( fmod( noteDuration, 1000.f ) / 1000 > 0.9 && timestretchAmount[a]->value() != 1 )
			{
				const int timestretchCrossfade = sample_index[a] + (timestretchAmount[a]->value() * 1000 * index_jump - 1000 * index_jump);
				if( timestretchCrossfade >= 0 )
				{
					const float timestretchCrossfadeAmount =  1 - ( (fmod( noteDuration, 1000.f ) / 1000 - 0.9) * 10 );
					oneSampleOutput[0] *= timestretchCrossfadeAmount;
					oneSampleOutput[1] *= timestretchCrossfadeAmount;
					oneSampleOutput[0] += soundSamples[0][a][timestretchCrossfade] * (1 - timestretchCrossfadeAmount);
					oneSampleOutput[1] += soundSamples[1][a][timestretchCrossfade] * (1 - timestretchCrossfadeAmount);
				}
			}

			if( loopMode[a]->value() == 2 && sample_index[a] > loopPoint2[a]->value() - crossfade[a]->value() && crossfade[a]->value() )
			{
				const int otherCrossfade = sample_index[a] - ( loopPoint2[a]->value() - loopPoint1[a]->value() );
				if( otherCrossfade >= 0 )
				{
					const float crossfadeAmount = (loopPoint2[a]->value() - sample_index[a]) / (crossfade[a]->value());
					oneSampleOutput[0] *= crossfadeAmount;
					oneSampleOutput[1] *= crossfadeAmount;
					oneSampleOutput[0] += soundSamples[0][a][otherCrossfade] * (1 - crossfadeAmount);
					oneSampleOutput[1] += soundSamples[1][a][otherCrossfade] * (1 - crossfadeAmount);
				}
			}

			const float volMult = volume[a]->value() * 0.01f * velVol;
			outputSample[0] += oneSampleOutput[0] * volMult * (panning[a]->value() > 0 ? 1 - panning[a]->value() / 100.f : 1);
			outputSample[1] += oneSampleOutput[1] * volMult * (panning[a]->value() < 0 ? 1 + panning[a]->value() / 100.f : 1);
		}
	}
}


// Takes input of original Hz and the number of cents to detune it by, and returns the detuned result in Hz.
inline float sSynth::detuneWithCents(float pitchValue, float detuneValue)
{
	return pitchValue * std::exp2(detuneValue / 1200.f); 
}



Sampler::Sampler( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &sampler_plugin_descriptor ),
	sampleNumber(1, 1, 999, this, tr("Sample Number")),
	detuneRatio(1, 0, 2, 0.0001, this, tr("Detune Ratio"))
{
}




Sampler::~Sampler()
{
}




void Sampler::saveSettings( QDomDocument & _doc, QDomElement & _this )
{

	// Save plugin version
	_this.setAttribute( "version", "0.1" );
}




void Sampler::loadSettings( const QDomElement & _this )
{


}




QString Sampler::nodeName() const
{
	return( sampler_plugin_descriptor.name );
}




void Sampler::playNote( NotePlayHandle * _n, sampleFrame * _working_buffer )
{
	if ( _n->totalFramesPlayed() == 0 || _n->m_pluginData == NULL )
	{
		_n->m_pluginData = new sSynth( _n, Engine::mixer()->processingSampleRate(), soundSamples, startPoint );
	}

	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

	sSynth * ps = static_cast<sSynth *>( _n->m_pluginData );
	for( fpp_t frame = offset; frame < frames + offset; ++frame )
	{
		sampleFrame outputSample = {0,0};

		ps->nextStringSample(outputSample, volume, soundSamples, startNote, endNote, velLeft, velRight, detuneRatio.value(), detune, startPoint, loopPoint1, loopPoint2, endPoint, loopMode, panning, crossfade, timestretchAmount);

		for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
		{
			_working_buffer[frame][chnl] = outputSample[chnl];
		}
	}

	applyRelease( _working_buffer, _n );

	instrumentTrack()->processAudioBuffer( _working_buffer, frames + offset, _n );
}




void Sampler::deleteNotePluginData( NotePlayHandle * _n )
{
	delete static_cast<sSynth *>( _n->m_pluginData );
}




PluginView * Sampler::instantiateView( QWidget * _parent )
{
	return( new SamplerView( this, _parent ) );
}



SamplerView::SamplerView( Instrument * _instrument, QWidget * _parent ) :
	InstrumentView( _instrument, _parent )
{
	setAutoFillBackground( true );
	QPalette pal;

	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );

	sampleNumberBox = new LcdSpinBox(3, this, "Sample Number");
	sampleNumberBox->move( 0, 0 );
	sampleNumberBox->setModel( &b->sampleNumber );
	
	volumeKnob = new Knob( knobDark_28, this );
	volumeKnob->move( 6, 201 );
	volumeKnob->setHintText( tr( "Volume" ), "" );

	detuneRatioKnob = new Knob( knobDark_28, this );
	detuneRatioKnob->move( 36, 201 );
	detuneRatioKnob->setHintText( tr( "Detune Ratio" ), "" );
	detuneRatioKnob->setModel( &b->detuneRatio );

	detuneKnob = new Knob( knobDark_28, this );
	detuneKnob->move( 66, 201 );
	detuneKnob->setHintText( tr( "Detune" ), "" );

	startPointKnob = new Knob( knobDark_28, this );
	startPointKnob->move( 6, 171 );
	startPointKnob->setHintText( tr( "Start Point" ), "" );

	loopPoint1Knob = new Knob( knobDark_28, this );
	loopPoint1Knob->move( 36, 171 );
	loopPoint1Knob->setHintText( tr( "Loop Point 1" ), "" );

	loopPoint2Knob = new Knob( knobDark_28, this );
	loopPoint2Knob->move( 66, 171 );
	loopPoint2Knob->setHintText( tr( "Loop Point 2" ), "" );

	endPointKnob = new Knob( knobDark_28, this );
	endPointKnob->move( 96, 171 );
	endPointKnob->setHintText( tr( "End Point" ), "" );

	loopModeKnob = new Knob( knobDark_28, this );
	loopModeKnob->move( 126, 171 );
	loopModeKnob->setHintText( tr( "Loop Mode" ), "" );

	panningKnob = new Knob( knobDark_28, this );
	panningKnob->move( 156, 201 );
	panningKnob->setHintText( tr( "Panning" ), "" );

	crossfadeKnob = new Knob( knobDark_28, this );
	crossfadeKnob->move( 186, 171 );
	crossfadeKnob->setHintText( tr( "Crossfade" ), "" );

	timestretchAmountKnob = new Knob( knobDark_28, this );
	timestretchAmountKnob->move( 216, 171 );
	timestretchAmountKnob->setHintText( tr( "Timestretch Amount" ), "" );

	loadSampleBtn = new PixmapButton( this, tr( "Load sample" ) );
	loadSampleBtn->move( 56, 201 );
	loadSampleBtn->setActiveGraphic( embed::getIconPixmap( "usr_wave_active" ) );
	loadSampleBtn->setInactiveGraphic( embed::getIconPixmap( "usr_wave_inactive" ) );
	ToolTip::add( loadSampleBtn, tr( "Load sample" ) );

	for( int a = 0; a < b->startNote.size(); ++a )
	{
		startNoteKnob.push_back(new Knob( knobDark_28, this ));
		startNoteKnob[a]->move( 300, 30*a+5 );
		startNoteKnob[a]->setHintText( tr( "Start Note" ), "" );
		startNoteKnob[a]->setModel( b->startNote[a] );

		endNoteKnob.push_back(new Knob( knobDark_28, this ));
		endNoteKnob[a]->move( 330, 30*a+5 );
		endNoteKnob[a]->setHintText( tr( "End Note" ), "" );
		endNoteKnob[a]->setModel( b->endNote[a] );

		velLeftKnob.push_back( new Knob( knobDark_28, this ) );
		velLeftKnob[a]->move( 360, 30*a+5 );
		velLeftKnob[a]->setHintText( tr( "Velocity Left" ), "" );
		velLeftKnob[a]->setModel( b->velLeft[a] );

		velRightKnob.push_back( new Knob( knobDark_28, this ) );
		velRightKnob[a]->move( 390, 30*a+5 );
		velRightKnob[a]->setHintText( tr( "Velocity Right" ), "" );
		velRightKnob[a]->setModel( b->velRight[a] );

		editButton.push_back( new PixmapButton(this, tr("Edit")) );
		editButton[a]->move( 420, 30*a+5 );
		editButton[a]->setActiveGraphic(embed::getIconPixmap("usr_wave_active"));
		editButton[a]->setInactiveGraphic(embed::getIconPixmap("usr_wave_inactive"));
		ToolTip::add(editButton[a], tr("Edit"));
	}
	
	connect( loadSampleBtn, SIGNAL ( clicked() ), this, SLOT ( loadSampleClicked() ) );

	connect( &b->sampleNumber, SIGNAL ( dataChanged() ), this, SLOT ( sampleNumberChanged() ) );
	sampleNumberChanged();
}




void SamplerView::sampleNumberChanged()
{
	if( b->volume.size() >= b->sampleNumber.value() )
	{
		volumeKnob->setModel( b->volume[b->sampleNumber.value()-1] );
		detuneKnob->setModel( b->detune[b->sampleNumber.value()-1] );
		startPointKnob->setModel( b->startPoint[b->sampleNumber.value()-1] );
		loopPoint1Knob->setModel( b->loopPoint1[b->sampleNumber.value()-1] );
		loopPoint2Knob->setModel( b->loopPoint2[b->sampleNumber.value()-1] );
		endPointKnob->setModel( b->endPoint[b->sampleNumber.value()-1] );
		loopModeKnob->setModel( b->loopMode[b->sampleNumber.value()-1] );
		panningKnob->setModel( b->panning[b->sampleNumber.value()-1] );
		crossfadeKnob->setModel( b->crossfade[b->sampleNumber.value()-1] );
		timestretchAmountKnob->setModel( b->timestretchAmount[b->sampleNumber.value()-1] );
	}
}





void SamplerView::loadSampleClicked()
{
	const sample_rate_t sample_rate = Engine::mixer()->processingSampleRate();

	SampleBuffer * sampleBuffer = new SampleBuffer;
	QString fileName = sampleBuffer->openAndSetWaveformFile();
	int filelength = sampleBuffer->sampleLength();
	if( fileName.isEmpty() == false )
	{
		sampleBuffer->dataReadLock();
		float origLengthOfSample = ((filelength/1000.f)*sample_rate);//in samples
		float lengthOfSample = origLengthOfSample;

		// Remove end silence
		while( sampleBuffer->userWaveSample(lengthOfSample/origLengthOfSample, 0) == 0 && sampleBuffer->userWaveSample(lengthOfSample/origLengthOfSample, 1) == 0 )
		{
			--lengthOfSample;
		}

		b->startNote.push_back( new FloatModel(0, 0, 130, 1, b, tr("Start Note")) );
		b->endNote.push_back( new FloatModel(130, 0, 130, 1, b, tr("End Note")) );
		b->velLeft.push_back( new FloatModel(0, 0, 130, 1, b, tr("Velocity Left")) );
		b->velRight.push_back( new FloatModel(130, 0, 130, 1, b, tr("Velocity Right")) );
		b->volume.push_back( new FloatModel(100, 0, 200, 0.0001, b, tr("Volume")) );
		b->detune.push_back( new FloatModel(0, -9600, 9600, 0.0001, b, tr("Detune")) );
		b->startPoint.push_back( new FloatModel(0, 0, lengthOfSample, 1, b, tr("Start Point")) );
		b->loopPoint1.push_back( new FloatModel(0, 0, lengthOfSample, 1, b, tr("Loop Point 1")) );
		b->loopPoint2.push_back( new FloatModel(lengthOfSample, 0, lengthOfSample, 1, b, tr("Loop Point 2")) );
		b->endPoint.push_back( new FloatModel(lengthOfSample, 0, lengthOfSample, 1, b, tr("End Point")) );
		b->loopMode.push_back( new FloatModel(1, 1, 3, 1, b, tr("Loop Mode")) );
		b->panning.push_back( new FloatModel(0, -100, 100, 0.0001, b, tr("Panning")) );
		b->crossfade.push_back( new FloatModel(0, 0, lengthOfSample, 1, b, tr("Crossfade")) );
		b->timestretchAmount.push_back( new FloatModel(1, 0, 8, 0.0001, b, tr("Timestretch Amount")) );

		startNoteKnob.push_back( new Knob( knobDark_28, this ) );
		startNoteKnob[(startNoteKnob.size()-1)]->move( 100, 30*(startNoteKnob.size()-1)+5 );
		startNoteKnob[(startNoteKnob.size()-1)]->setHintText( tr( "Start Note" ), "" );
		startNoteKnob[(startNoteKnob.size()-1)]->setModel( b->startNote[(b->startNote.size()-1)] );
		startNoteKnob[(startNoteKnob.size()-1)]->show();

		endNoteKnob.push_back( new Knob( knobDark_28, this ) );
		endNoteKnob[(endNoteKnob.size()-1)]->move( 130, 30*(endNoteKnob.size()-1)+5 );
		endNoteKnob[(endNoteKnob.size()-1)]->setHintText( tr( "End Note" ), "" );
		endNoteKnob[(endNoteKnob.size()-1)]->setModel( b->endNote[(b->endNote.size()-1)] );
		endNoteKnob[(endNoteKnob.size()-1)]->show();

		velLeftKnob.push_back( new Knob( knobDark_28, this ) );
		velLeftKnob[(velLeftKnob.size()-1)]->move( 160, 30*(velLeftKnob.size()-1)+5 );
		velLeftKnob[(velLeftKnob.size()-1)]->setHintText( tr( "Velocity Left" ), "" );
		velLeftKnob[(velLeftKnob.size()-1)]->setModel( b->velLeft[(b->velLeft.size()-1)] );
		velLeftKnob[(velLeftKnob.size()-1)]->show();

		velRightKnob.push_back( new Knob( knobDark_28, this ) );
		velRightKnob[(velRightKnob.size()-1)]->move( 190, 30*(velRightKnob.size()-1)+5 );
		velRightKnob[(velRightKnob.size()-1)]->setHintText( tr( "Velocity Right" ), "" );
		velRightKnob[(velRightKnob.size()-1)]->setModel( b->velRight[(b->velRight.size()-1)] );
		velRightKnob[(velRightKnob.size()-1)]->show();

		vector<float> tempVector;
		b->soundSamples[0].push_back(tempVector);
		b->soundSamples[1].push_back(tempVector);

		b->soundSamples[0][b->soundSamples[0].size()-1].clear();
		b->soundSamples[1][b->soundSamples[0].size()-1].clear();

		for( int i = 0; i < lengthOfSample; ++i )
		{
			b->soundSamples[0][b->soundSamples[0].size()-1].push_back(sampleBuffer->userWaveSample(i/lengthOfSample, 0));
			b->soundSamples[1][b->soundSamples[0].size()-1].push_back(sampleBuffer->userWaveSample(i/lengthOfSample, 1));
		}
		sampleBuffer->dataUnlock();
	}
}





extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main(Model *m, void *)
{
	return new Sampler(static_cast<InstrumentTrack *>(m));
}


}




