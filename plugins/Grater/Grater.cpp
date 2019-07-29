/*
 * Grater.cpp - granular synthesizer
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


#include <QDomElement>

#include "Grater.h"
#include "base64.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "Knob.h"
#include "LedCheckbox.h"
#include "Mixer.h"
#include "NotePlayHandle.h"
#include "Oscillator.h"
#include "PixmapButton.h"
#include "ToolTip.h"
#include "Song.h"
#include "SampleBuffer.h"
#include "gui_templates.h"
#include "GuiApplication.h"
#include "MainWindow.h"

#include "embed.h"

#include "plugin_export.h"
















#include <iostream>
using namespace std;



















extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT grater_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Grater",
	QT_TRANSLATE_NOOP( "pluginBrowser", "Granular synthesizer" ),
	"Lost Robot <r94231/at/gmail/dot/com>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader( "logo" ),
	NULL,
	NULL
} ;

}


gSynth::gSynth( NotePlayHandle * _nph, const sample_rate_t _sample_rate, float grain, float position, std::vector<float> (&soundSample)[2], float spray, float fmAmount, float fmFreq, float pmAmount, float pmFreq, float ramp, float pitchRand, float stereoGrain, float voiceNum, float posDist, float volRand, float amAmount, float amFreq ) :
	sample_index( 0 ),
	nph( _nph ),
	sample_rate( _sample_rate )
{
	for( int i = 0; i < voiceNum; ++i )
	{
		synthVoices[i][0] = new gSynthVoice( nph, sample_rate, position + ((i/voiceNum)*(sample_rate/grain/soundSample[0].size())), soundSample, spray, fmAmount, fmFreq, pmAmount, pmFreq, ramp, pitchRand, stereoGrain, voiceNum, posDist, volRand, amAmount, amFreq );
		synthVoices[i][1] = new gSynthVoice( nph, sample_rate, position + ((i/voiceNum)*(sample_rate/grain/soundSample[0].size())), soundSample, spray, fmAmount, fmFreq, pmAmount, pmFreq, ramp, pitchRand, stereoGrain, voiceNum, posDist, volRand, amAmount, amFreq );
	}
}


void gSynth::nextStringSample( sampleFrame &outputSample, float grain, float position, std::vector<float> (&soundSample)[2], float speed, bool speedEnabled, float spray, float fmAmount, float fmFreq, float pmAmount, float pmFreq, float ramp, float pitchRand, float stereoGrain, float voiceNum, float posDist, float volRand, float amAmount, float amFreq )
{
	outputSample[0] = 0;
	outputSample[1] = 0;

	for( int i = 0; i < voiceNum; ++i )
	{
		if( !synthVoices[i][0] )
		{
			synthVoices[i][0] = new gSynthVoice( nph, sample_rate, position + ((i/voiceNum)*(sample_rate/grain/soundSample[0].size())), soundSample, spray, fmAmount, fmFreq, pmAmount, pmFreq, ramp, pitchRand, stereoGrain, voiceNum, posDist, volRand, amAmount, amFreq );
			synthVoices[i][1] = new gSynthVoice( nph, sample_rate, position + ((i/voiceNum)*(sample_rate/grain/soundSample[0].size())), soundSample, spray, fmAmount, fmFreq, pmAmount, pmFreq, ramp, pitchRand, stereoGrain, voiceNum, posDist, volRand, amAmount, amFreq );
		}

		synthVoices[i][0]->nextStringSample( outputSample1, qMax( grain * stereoGrain, 0.025f ), position + ((i/voiceNum)*(sample_rate/grain/soundSample[0].size())), soundSample, 0, speed, speedEnabled, spray, fmAmount, fmFreq, pmAmount, pmFreq, ramp, pitchRand, stereoGrain, voiceNum, posDist, volRand, amAmount, amFreq );
		synthVoices[i][1]->nextStringSample( outputSample2, grain, position + ((i/voiceNum)*(sample_rate/grain/soundSample[0].size())), soundSample, 1, speed, speedEnabled, spray, fmAmount, fmFreq, pmAmount, pmFreq, ramp, pitchRand, stereoGrain, voiceNum, posDist, volRand, amAmount, amFreq );
		outputSample[0] += outputSample1;
		outputSample[1] += outputSample2;
	}

	if( voiceNum > 3 )
	{
		outputSample[0] /= voiceNum / 4.f;
		outputSample[1] /= voiceNum / 4.f;
	}
}



gSynth::~gSynth()
{
	for( int i = 0; i < 128; ++i )
	{
		if( synthVoices[i][0] )
		{
			synthVoices[i][0] = NULL;
			synthVoices[i][1] = NULL;
			delete synthVoices[i][0];
			delete synthVoices[i][1];
		}
	}
}



gSynthVoice::gSynthVoice( NotePlayHandle * _nph, const sample_rate_t _sample_rate, float orig_position, std::vector<float> (&soundSample)[2], float spray, float fmAmount, float fmFreq, float pmAmount, float pmFreq, float ramp, float pitchRand, float stereoGrain, float voiceNum, float posDist, float volRand, float amAmount, float amFreq ):
	sample_index( 0 ),
	nph( _nph ),
	sample_rate( _sample_rate )
{
	sample_realindex = 0;

	position = orig_position;
}


gSynthVoice::~gSynthVoice()
{
}


void gSynthVoice::nextStringSample( float &outputSample, float grain, float orig_position, std::vector<float> (&soundSample)[2], int whichVoice, float speed, bool speedEnabled, float spray, float fmAmount, float fmFreq, float pmAmount, float pmFreq, float ramp, float pitchRand, float stereoGrain, float voiceNum, float posDist, float volRand, float amAmount, float amFreq )
{

	++voiceDuration;

	sampleRateRatio = 44100.f / sample_rate;

	fmIndex += ( fmFreq * F_2PI ) / sample_rate;
	fmDetune = sin( fmIndex ) * fmAmount;

	pmIndex += ( pmFreq * F_2PI ) / sample_rate;
	pmShift = sin( pmIndex ) * pmAmount * sampleRateRatio;

	amIndex += ( amFreq * F_2PI ) / sample_rate;
	amChange = sin( amIndex ) * amAmount * sampleRateRatio;

	float sample_step = ( detuneWithCents( nph->frequency(), fmDetune + randomPitchVal * pitchRand ) / 440.f ) * sampleRateRatio;

	float positionProgress = soundSample[0].size() * position;
	float actualGrain = sample_rate / grain;

	if( speedEnabled )
	{
		speedProgress += sampleRateRatio / ( speed * 0.01f );
	}

	// check overflow
	if( voiceDuration >= actualGrain )
	{
		voiceDuration -= actualGrain;
		sample_realindex = 0;
		sprayPos = ( rand() / (float)RAND_MAX ) * 2 - 1;
		position = orig_position + ( speedProgress / soundSample[0].size() );
		randomPitchVal = ( rand() / (float)RAND_MAX ) * 2 - 1;
		randomVolume = ( rand() / (float)RAND_MAX ) * 2 - 1;
	}

	sample_index = int(realfmod( sample_realindex + positionProgress + pmShift + ( spray * sample_rate * sprayPos ), soundSample[0].size() ));

	outputSample = soundSample[whichVoice][sample_index];

	float actualRamp = ramp * actualGrain;
	float lenNoDecay = actualGrain - actualRamp;

	float outputVolume = 1;

	if( voiceDuration < actualRamp )
	{
		outputVolume = voiceDuration / actualRamp;
	}
	else if( voiceDuration > lenNoDecay )
	{
		outputVolume = -( ( voiceDuration - lenNoDecay ) / actualRamp ) + 1;
	}

	outputSample *= outputVolume * ( 1 + randomVolume * volRand ) * ( amChange + 1 );
	
	sample_realindex += sample_step;
}


// Takes input of original Hz and the number of cents to detune it by, and returns the detuned result in Hz.
inline float gSynthVoice::detuneWithCents( float pitchValue, float detuneValue )
{
	if( detuneValue )// Avoids expensive exponentiation if no detuning is necessary
	{
		return pitchValue * std::exp2( detuneValue / 1200.f ); 
	}
	else
	{
		return pitchValue;
	}
}	


// Handles negative values properly, unlike fmod.
inline float gSynthVoice::realfmod( float k, float n )// Handles negative values properly, unlike fmod.
{
	return ((k = fmod(k,n)) < 0) ? k+n : k;
}



Grater::Grater( InstrumentTrack * _instrument_track ) :
	Instrument( _instrument_track, &grater_plugin_descriptor ),
	grain( 44, 0.025, 400, 0.0001, this, tr( "Grain Frequency" ) ),
	position( 0, 0, 1, 0.0001, this, tr( "Position" ) ),
	speed( 100, 25, 20000, 0.0001, this, tr( "Speed" ) ),
	speedEnabled( false, this ),
	spray( 0, 0, 10, 0.0001, this, tr( "Spray" ) ),
	fmAmount( 0, 0, 4800, 0.0001, this, tr( "FM Amount" ) ),
	fmFreq( 200, 0.0001, 20000, 0.0001, this, tr( "FM Frequency" ) ),
	pmAmount( 0, 0, 400, 0.0001, this, tr( "PM Amount" ) ),
	pmFreq( 200, 0.0001, 20000, 0.0001, this, tr( "PM Frequency" ) ),
	ramp( 0.05, 0, 0.5, 0.0001, this, tr( "Ramp" ) ),
	pitchRand( 0, 0, 9600, 0.0001, this, tr( "Pitch Randomness" ) ),
	stereoGrain( 1, 0, 4, 0.0001, this, tr( "L/R Grain Frequency Difference" ) ),
	voiceNum( 1, 1, 32, 1, this, tr( "Number of voices" ) ),
	posDist( 0, 0, 0.5, 0.0001, this, tr( "Position Distribution" ) ),
	volRand( 0, 0, 1, 0.0001, this, tr( "volume Randomness" ) ),
	amAmount( 0, 0, 1, 0.0001, this, tr( "AM Amount" ) ),
	amFreq( 200, 0.0001, 20000, 0.0001, this, tr( "AM Frequency" ) )
{
	grain.setScaleLogarithmic( true );
	speed.setScaleLogarithmic( true );
	fmAmount.setScaleLogarithmic( true );
	fmFreq.setScaleLogarithmic( true );
	pmAmount.setScaleLogarithmic( true );
	pmFreq.setScaleLogarithmic( true );
	pitchRand.setScaleLogarithmic( true );
	posDist.setScaleLogarithmic( true );
	amAmount.setScaleLogarithmic( true );
	amFreq.setScaleLogarithmic( true );

	soundSample[0].push_back( 0 );
	soundSample[1].push_back( 0 );
}




Grater::~Grater()
{
}




void Grater::saveSettings( QDomDocument & _doc, QDomElement & _this )
{

	// Save plugin version
	_this.setAttribute( "version", "0.1" );

	QString saveString;

	grain.saveSettings( _doc, _this, "grainSize" );
	position.saveSettings( _doc, _this, "position" );
	speed.saveSettings( _doc, _this, "speed" );
	speedEnabled.saveSettings( _doc, _this, "speedEnabled" );
	spray.saveSettings( _doc, _this, "spray" );
	fmAmount.saveSettings( _doc, _this, "fmAmount" );
	fmFreq.saveSettings( _doc, _this, "fmFreq" );
	pmAmount.saveSettings( _doc, _this, "pmAmount" );
	pmFreq.saveSettings( _doc, _this, "pmFreq" );
	ramp.saveSettings( _doc, _this, "ramp" );
	pitchRand.saveSettings( _doc, _this, "pitchRand" );
	stereoGrain.saveSettings( _doc, _this, "stereoGrain" );
	voiceNum.saveSettings( _doc, _this, "voiceNum" );
	posDist.saveSettings( _doc, _this, "posDist" );
	volRand.saveSettings( _doc, _this, "volRand" );
	amAmount.saveSettings( _doc, _this, "amAmount" );
	amFreq.saveSettings( _doc, _this, "amFreq" );

	for( int i = 0; i < 2; ++i )
	{
		base64::encode( (const char *)soundSample[i].data(),
			soundSample[i].size() * sizeof(float), saveString );
		_this.setAttribute( "soundSample_"+QString::number(i), saveString );
	}

	_this.setAttribute( "sampleSize", (int)soundSample[0].size() );
}




void Grater::loadSettings( const QDomElement & _this )
{
	grain.loadSettings( _this, "grainSize" );
	position.loadSettings( _this, "position" );
	speed.loadSettings( _this, "speed" );
	speedEnabled.loadSettings( _this, "speedEnabled" );
	spray.loadSettings( _this, "spray" );
	fmAmount.loadSettings( _this, "fmAmount" );
	fmFreq.loadSettings( _this, "fmFreq" );
	pmAmount.loadSettings( _this, "pmAmount" );
	pmFreq.loadSettings( _this, "pmFreq" );
	ramp.loadSettings( _this, "ramp" );
	pitchRand.loadSettings( _this, "pitchRand" );
	stereoGrain.loadSettings( _this, "stereoGrain" );
	voiceNum.loadSettings( _this, "voiceNum" );
	posDist.loadSettings( _this, "posDist" );
	volRand.loadSettings( _this, "volRand" );
	amAmount.loadSettings( _this, "amAmount" );
	amFreq.loadSettings( _this, "amFreq" );

	int sampleSize = _this.attribute( "sampleSize" ).toInt();

	int size = 0;
	char * dst = 0;

	soundSample[0].clear();
	soundSample[1].clear();

	for( int i = 0; i < 2; ++i )
	{
		base64::decode( _this.attribute( "soundSample_"+QString::number(i) ), &dst, &size );
		for( int j = 0; j < sampleSize; ++j )
		{
			soundSample[i].push_back( ( (float*) dst )[j] );
		}
	}

	if( !soundSample[0].size() )
	{
		soundSample[0].push_back( 0 );
		soundSample[1].push_back( 0 );
	}
}



QString Grater::nodeName() const
{
	return( grater_plugin_descriptor.name );
}




void Grater::playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer )
{
	if ( _n->totalFramesPlayed() == 0 || _n->m_pluginData == NULL )
	{

		_n->m_pluginData = new gSynth( _n, Engine::mixer()->processingSampleRate(), grain.value(), position.value(), soundSample, spray.value(), fmAmount.value(), fmFreq.value(), pmAmount.value(), pmFreq.value(), ramp.value(), pitchRand.value(), stereoGrain.value(), voiceNum.value(), posDist.value(), volRand.value(), amAmount.value(), amFreq.value() );
	}

	const fpp_t frames = _n->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _n->noteOffset();

	gSynth * ps = static_cast<gSynth *>( _n->m_pluginData );
	for( fpp_t frame = offset; frame < frames + offset; ++frame )
	{
		sampleFrame outputSample;

		ps->nextStringSample( outputSample, grain.value(), position.value(), soundSample, speed.value(), speedEnabled.value(), spray.value(), fmAmount.value(), fmFreq.value(), pmAmount.value(), pmFreq.value(), ramp.value(), pitchRand.value(), stereoGrain.value(), voiceNum.value(), posDist.value(), volRand.value(), amAmount.value(), amFreq.value() );
		for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
		{
			_working_buffer[frame][chnl] = outputSample[chnl];
		}
	}

	applyRelease( _working_buffer, _n );

	instrumentTrack()->processAudioBuffer( _working_buffer, frames + offset, _n );
}




void Grater::deleteNotePluginData( NotePlayHandle * _n )
{
	delete static_cast<gSynth *>( _n->m_pluginData );
}




PluginView * Grater::instantiateView( QWidget * _parent )
{
	return( new GraterView( this, _parent ) );
}







GraterView::GraterView( Instrument * _instrument,
					QWidget * _parent ) :
	InstrumentView( _instrument, _parent )
{
	setAutoFillBackground( true );
	QPalette pal;

	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	setPalette( pal );
	
	grainKnob = new GraterKnob( knobDark_28, this );
	grainKnob->move( 6, 201 );
	grainKnob->setHintText( tr( "Grain Frequency" ), "" );
	grainKnob->hasSlowMovement = true;

	positionKnob = new GraterKnob( knobDark_28, this );
	positionKnob->move( 46, 201 );
	positionKnob->setHintText( tr( "Position" ), "" );

	speedKnob = new GraterKnob( knobDark_28, this );
	speedKnob->move( 76, 201 );
	speedKnob->setHintText( tr( "Speed" ), "" );
	speedKnob->hasSlowMovement = true;

	speedEnabledToggle = new LedCheckBox( "", this, tr( "Speed Enabled" ), LedCheckBox::Green );

	sprayKnob = new GraterKnob( knobDark_28, this );
	sprayKnob->move( 136, 201 );
	sprayKnob->setHintText( tr( "Spray" ), "" );

	fmAmountKnob = new GraterKnob( knobDark_28, this );
	fmAmountKnob->move( 166, 201 );
	fmAmountKnob->setHintText( tr( "FM Amount" ), "" );
	fmAmountKnob->hasSlowMovement = true;

	fmFreqKnob = new GraterKnob( knobDark_28, this );
	fmFreqKnob->move( 196, 201 );
	fmFreqKnob->setHintText( tr( "FM Frequency" ), "" );
	fmFreqKnob->hasSlowMovement = true;

	pmAmountKnob = new GraterKnob( knobDark_28, this );
	pmAmountKnob->move( 166, 171 );
	pmAmountKnob->setHintText( tr( "PM Amount" ), "" );
	pmAmountKnob->hasSlowMovement = true;

	pmFreqKnob = new GraterKnob( knobDark_28, this );
	pmFreqKnob->move( 196, 171 );
	pmFreqKnob->setHintText( tr( "PM Frequency" ), "" );
	pmFreqKnob->hasSlowMovement = true;

	rampKnob = new GraterKnob( knobDark_28, this );
	rampKnob->move( 136, 171 );
	rampKnob->setHintText( tr( "Ramp" ), "" );

	pitchRandKnob = new GraterKnob( knobDark_28, this );
	pitchRandKnob->move( 106, 171 );
	pitchRandKnob->setHintText( tr( "Pitch Randomness" ), "" );

	stereoGrainKnob = new GraterKnob( knobDark_28, this );
	stereoGrainKnob->move( 76, 171 );
	stereoGrainKnob->setHintText( tr( "L/R Grain Frequency Difference" ), "" );

	voiceNumKnob = new GraterKnob( knobDark_28, this );
	voiceNumKnob->move( 46, 171 );
	voiceNumKnob->setHintText( tr( "Number of voices" ), "" );

	posDistKnob = new GraterKnob( knobDark_28, this );
	posDistKnob->move( 16, 171 );
	posDistKnob->setHintText( tr( "Distribution of voice positions" ), "" );

	volRandKnob = new GraterKnob( knobDark_28, this );
	volRandKnob->move( 16, 141 );
	volRandKnob->setHintText( tr( "Volume Randomness" ), "" );

	amAmountKnob = new GraterKnob( knobDark_28, this );
	amAmountKnob->move( 166, 131 );
	amAmountKnob->setHintText( tr( "AM Amount" ), "" );
	amAmountKnob->hasSlowMovement = true;

	amFreqKnob = new GraterKnob( knobDark_28, this );
	amFreqKnob->move( 196, 131 );
	amFreqKnob->setHintText( tr( "AM Frequency" ), "" );
	amFreqKnob->hasSlowMovement = true;

	usrWaveBtn = new PixmapButton( this, tr( "User-defined wave" ) );
	usrWaveBtn->move( 131 + 14*5, 105 );
	usrWaveBtn->setActiveGraphic( embed::getIconPixmap(
						"usr_wave_active" ) );
	usrWaveBtn->setInactiveGraphic( embed::getIconPixmap(
						"usr_wave_inactive" ) );
	ToolTip::add( usrWaveBtn,
			tr( "User-defined wave" ) );
	

	connect( usrWaveBtn, SIGNAL ( clicked () ),
			this, SLOT ( usrWaveClicked() ) );

}




void GraterView::modelChanged()
{
	Grater * b = castModel<Grater>();

	grainKnob->setModel( &b->grain );
	positionKnob->setModel( &b->position );
	speedKnob->setModel( &b->speed );
	speedEnabledToggle->setModel( &b->speedEnabled );
	sprayKnob->setModel( &b->spray );
	fmAmountKnob->setModel( &b->fmAmount );
	fmFreqKnob->setModel( &b->fmFreq );
	pmAmountKnob->setModel( &b->pmAmount );
	pmFreqKnob->setModel( &b->pmFreq );
	rampKnob->setModel( &b->ramp );
	pitchRandKnob->setModel( &b->pitchRand );
	stereoGrainKnob->setModel( &b->stereoGrain );
	voiceNumKnob->setModel( &b->voiceNum );
	posDistKnob->setModel( &b->posDist );
	volRandKnob->setModel( &b->volRand );
	amAmountKnob->setModel( &b->amAmount );
	amFreqKnob->setModel( &b->amFreq );
}



void GraterView::usrWaveClicked()
{
	Grater * b = castModel<Grater>();

	const sample_rate_t sample_rate = Engine::mixer()->processingSampleRate();

	SampleBuffer * sampleBuffer = new SampleBuffer;
	QString fileName = sampleBuffer->openAndSetWaveformFile();
	int filelength = sampleBuffer->sampleLength();
	if( fileName.isEmpty() == false )
	{
		sampleBuffer->dataReadLock();
		float lengthOfSample = ((filelength/1000.f)*sample_rate);//in samples
		b->soundSample[0].clear();
		b->soundSample[1].clear();

		for( int i = 0; i < lengthOfSample; ++i )
		{
			b->soundSample[0].push_back(sampleBuffer->userWaveSample(i/lengthOfSample, 0));
			b->soundSample[1].push_back(sampleBuffer->userWaveSample(i/lengthOfSample, 1));
		}
		sampleBuffer->dataUnlock();
	}
}



float GraterKnob::getValue( const QPoint & _p )
{
	float value;

	float weight = ( ( model()->value() + model()->minValue() ) / ( pageSize() + model()->minValue() ) ) + 0.5f;

	// arcane mathemagicks for calculating knob movement
	value = ( ( _p.y() + _p.y() * qMin( qAbs( _p.y() / 2.5f ), 6.0f ) ) ) / 12.0f;

	// if shift pressed we want slower movement
	if( gui->mainWindow()->isShiftPressed() && !hasSlowMovement )
	{
		value /= 4.0f;
		value = qBound( -4.0f, value, 4.0f );
	}
	if( hasSlowMovement )
	{
		if( gui->mainWindow()->isShiftPressed() )
		{
			value /= 512.0f / weight;
			value = qBound( -512.0f * weight, value, 512.0f * weight );
		}
		else
		{
			value /= 128.0f / weight;
			value = qBound( -128.0f * weight, value, 128.0f * weight );
		}
	}
	return value * pageSize();
}




extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main( Model * m, void * )
{
	return( new Grater( static_cast<InstrumentTrack *>( m ) ) );
}


}




