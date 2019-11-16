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























//LP->BP->HP->Notch->Allpass












#include <iostream>
using namespace std;






const int TIMESTRETCH_LOOP_SIZE = 10000;



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
		amOscIndex.push_back(0);
		fmOscIndex.push_back(0);
	}
}


sSynth::~sSynth()
{
}


void sSynth::nextStringSample( sampleFrame &outputSample, QVector<FloatModel *> (&volume), std::vector<std::vector<float>> (&soundSamples)[2], QVector<FloatModel *> (&startNote), QVector<FloatModel *> (&endNote), QVector<FloatModel *> (&velLeft), QVector<FloatModel *> (&velRight), float detuneRatio, QVector<FloatModel *> (&detune), QVector<FloatModel *> (&startPoint), QVector<FloatModel *> (&loopPoint1), QVector<FloatModel *> (&loopPoint2), QVector<FloatModel *> (&endPoint), QVector<FloatModel *> (&loopMode), QVector<FloatModel *> (&panning), QVector<FloatModel *> (&crossfade), QVector<FloatModel *> (&timestretchAmount), QVector<FloatModel *> (&distortionType), QVector<FloatModel *> (&distortionAmount), QVector<FloatModel *> (&volPredelay), QVector<FloatModel *> (&volAttack), QVector<FloatModel *> (&volHold), QVector<FloatModel *> (&volDecay), QVector<FloatModel *> (&volSustain), QVector<FloatModel *> (&volRelease), int frame, QVector<FloatModel *> (&amFreq), QVector<FloatModel *> (&amAmnt), QVector<FloatModel *> (&fmFreq), QVector<FloatModel *> (&fmAmnt), QVector<FloatModel *> (&filtCutoff), QVector<FloatModel *> (&filtReso), QVector<FloatModel *> (&filtMorph), QVector<FloatModel *> (&grainSize) )
{
	++noteDuration;

	const sample_rate_t sample_rate = Engine::mixer()->processingSampleRate();

	if( nph->isReleased() && frame >= nph->framesBeforeRelease() && !noteReleaseTime )
	{
		noteReleaseTime = noteDuration;
	}

	for( int a = 0; a < soundSamples[0].size(); ++a )
	{
		if( nph->key() >= startNote[a]->value() && nph->key() <= endNote[a]->value() )
		{
			double index_jump = ((((nph->frequency() * exp2(detune[a]->value() / 1200.f)) - 440) * detuneRatio + 440) / 440.f) * (44100.f / sample_rate) * playDirection[a];
			sample_index[a] += index_jump;

			if( timestretchAmount[a]->value() != 1 && fmod( noteDuration, grainSize[a]->value() ) == 0 && noteDuration )
			{
				sample_index[a] += timestretchAmount[a]->value() * grainSize[a]->value() * index_jump - grainSize[a]->value() * index_jump;
			}
			else if( loopMode[a]->value() == 2 )
			{
				if( sample_index[a] > loopPoint2[a]->value() )
				{
					sample_index[a] = loopPoint1[a]->value();
				}
			}
			else if( loopMode[a]->value() == 3 )
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

			int realSampleIndex = sample_index[a];
			if( fmAmnt[a]->value() )
			{
				fmOscIndex[a] += fmFreq[a]->value() / sample_rate;
				while (fmOscIndex[a] > 1)
				{
					--fmOscIndex[a];
				}

				realSampleIndex = qMin(realSampleIndex + sin(fmOscIndex[a] * F_2PI) * fmAmnt[a]->value(), (float)soundSamples[0][a].size()-1);
			}

			if( realSampleIndex <= endPoint[a]->value() )
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

				oneSampleOutput[0] = soundSamples[0][a][realSampleIndex];
				oneSampleOutput[1] = soundSamples[1][a][realSampleIndex];

				if( fmod( noteDuration, grainSize[a]->value() ) / grainSize[a]->value() > 0.9 && timestretchAmount[a]->value() != 1 )
				{
					const int timestretchCrossfade = realSampleIndex + (timestretchAmount[a]->value() * grainSize[a]->value() * index_jump - grainSize[a]->value() * index_jump);
					if( timestretchCrossfade >= 0 )
					{
						const float timestretchCrossfadeAmount =  1 - ( (fmod( noteDuration, grainSize[a]->value() ) / grainSize[a]->value() - 0.9) * 10 );
						oneSampleOutput[0] *= timestretchCrossfadeAmount;
						oneSampleOutput[1] *= timestretchCrossfadeAmount;
						oneSampleOutput[0] += soundSamples[0][a][timestretchCrossfade] * (1 - timestretchCrossfadeAmount);
						oneSampleOutput[1] += soundSamples[1][a][timestretchCrossfade] * (1 - timestretchCrossfadeAmount);
					}
				}

				if( loopMode[a]->value() == 2 && realSampleIndex > loopPoint2[a]->value() - crossfade[a]->value() && crossfade[a]->value() )
				{
					const int otherCrossfade = realSampleIndex - ( loopPoint2[a]->value() - loopPoint1[a]->value() );
					if( otherCrossfade >= 0 )
					{
						const float crossfadeAmount = (loopPoint2[a]->value() - realSampleIndex) / (crossfade[a]->value());
						oneSampleOutput[0] *= crossfadeAmount;
						oneSampleOutput[1] *= crossfadeAmount;
						oneSampleOutput[0] += soundSamples[0][a][otherCrossfade] * (1 - crossfadeAmount);
						oneSampleOutput[1] += soundSamples[1][a][otherCrossfade] * (1 - crossfadeAmount);
					}
				}

				const float volMult = volume[a]->value() * 0.01f * velVol;
				oneSampleOutput[0] *= volMult * (panning[a]->value() > 0 ? 1 - panning[a]->value() / 100.f : 1);
				oneSampleOutput[1] *= volMult * (panning[a]->value() < 0 ? 1 + panning[a]->value() / 100.f : 1);

				switch( (int)distortionType[a]->value() )
				{
					case 0:
					{
						break;
					}
					case 1:// Cubic
					{
						oneSampleOutput[0] -= distortionAmount[a]->value() / 100.f * oneSampleOutput[0] * oneSampleOutput[0] * oneSampleOutput[0];
						oneSampleOutput[1] -= distortionAmount[a]->value() / 100.f * oneSampleOutput[1] * oneSampleOutput[1] * oneSampleOutput[1];
						break;
					}
					case 2:// Atan
					{
						oneSampleOutput[0] = 2.f/F_PI * atan( distortionAmount[a]->value() * 0.175f * oneSampleOutput[0] );
						oneSampleOutput[1] = 2.f/F_PI * atan( distortionAmount[a]->value() * 0.175f * oneSampleOutput[1] );
						break;
					}
					case 3:// Sine
					{
						oneSampleOutput[0] = sin( distortionAmount[a]->value() / 15.f * oneSampleOutput[0] );
						oneSampleOutput[1] = sin( distortionAmount[a]->value() / 15.f * oneSampleOutput[1] );
						break;
					}
					case 4:// Overdrive
					{
						for( int b = 0; b < 2; ++b )
						{
							oneSampleOutput[b] *= distortionAmount[a]->value() * 0.05f;
							if( abs( oneSampleOutput[b] ) < (1.f/3.f) )
							{
								oneSampleOutput[b] *= 2;
							}
							else if( abs( oneSampleOutput[b] ) < (2.f/3.f) )
							{
								const float temp = 2-3*abs(oneSampleOutput[b]);
								oneSampleOutput[b] = ((3-temp*temp)/3.f) * (oneSampleOutput[b] > 0 ? 1 : -1);
							}
							else
							{
								oneSampleOutput[b] = oneSampleOutput[b] > 0 ? 1 : -1;
							}
						}
						break;
					}
					case 5:// Fuzz
					{
						for( int b = 0; b < 2; ++b )
						{
							if( oneSampleOutput[b] > 0 )
							{
								oneSampleOutput[b] = 1 - std::exp(-oneSampleOutput[b]*distortionAmount[a]->value() * 0.25f);
							}
							else
							{
								oneSampleOutput[b] = -1 + std::exp(oneSampleOutput[b]*distortionAmount[a]->value() * 0.25f);
							}
						}
						break;
					}
					case 6:// Tanh
					{
						for( int b = 0; b < 2; ++b )
						{
							oneSampleOutput[b] = std::tanh( oneSampleOutput[b] * distortionAmount[a]->value() * 0.125f );
						}
						break;
					}
					case 7:// Bram de Jong Soft Saturation
					{
						for( int b = 0; b < 2; ++b )
						{
							const int temp = oneSampleOutput[b] < 0 ? -1 : 1;
							oneSampleOutput[b] = abs(oneSampleOutput[b]);
							if( oneSampleOutput[b] > distortionAmount[a]->value() * 0.01f )
							{
								oneSampleOutput[b] = (distortionAmount[a]->value() * 0.01f) + (oneSampleOutput[b]-(distortionAmount[a]->value() * 0.01f))/(1+pow((oneSampleOutput[b]-(distortionAmount[a]->value() * 0.01f))/(1-(distortionAmount[a]->value() * 0.01f)), 2));
							}
							else if( oneSampleOutput[b] > 1 )
							{
								oneSampleOutput[b] = ((distortionAmount[a]->value() * 0.01f) + 1) / 2.f;
							}
							oneSampleOutput[b] *= temp;
						}
						break;
					}
					case 8:// Bram de Jong Soft Waveshaper
					{
						for( int b = 0; b < 2; ++b )
						{
							oneSampleOutput[b] = oneSampleOutput[b]*(abs(oneSampleOutput[b]) + (distortionAmount[a]->value() * 0.1f))/(oneSampleOutput[b]*oneSampleOutput[b] + ((distortionAmount[a]->value() * 0.1f)-1)*abs(oneSampleOutput[b]) + 1);
						}
						break;
					}
				}

				if( amAmnt[a]->value() )
				{
					amOscIndex[a] += amFreq[a]->value() / sample_rate;
					while( amOscIndex[a] > 1)
					{
						--amOscIndex[a];
					}

					oneSampleOutput[0] *= sin(amOscIndex[a] * F_2PI) * amAmnt[a]->value() + 1;
					oneSampleOutput[1] *= sin(amOscIndex[a] * F_2PI) * amAmnt[a]->value() + 1;
				}

				float volEnvResult = calcEnvelope(a, 0, frame,
	volPredelay[a]->value() * sample_rate,
	volAttack[a]->value() * sample_rate,
	volHold[a]->value() * sample_rate,
	volDecay[a]->value() * sample_rate,
	volSustain[a]->value(),
	volRelease[a]->value() * sample_rate
	);

				oneSampleOutput[0] *= volEnvResult;
				oneSampleOutput[1] *= volEnvResult;
				
				outputSample[0] += oneSampleOutput[0];
				outputSample[1] += oneSampleOutput[1];
			}
		}
	}
}


// Takes input of original Hz and the number of cents to detune it by, and returns the detuned result in Hz.
inline float sSynth::detuneWithCents(float pitchValue, float detuneValue)
{
	return pitchValue * std::exp2(detuneValue / 1200.f);
}



inline float sSynth::calcEnvelope(int sampNum, int envNum, int frame, float predelay, float attack, float hold, float decay, float sustain, float release)
{
	float returnVal;
	if( noteDuration < predelay )
	{
		returnVal = 0;
	}
	else if( noteDuration < predelay + attack )
	{
		returnVal = pow((noteDuration - predelay) / attack, 1.f / F_E);
	}
	else if( noteDuration < predelay + attack + hold )
	{
		returnVal = 1;
	}
	else if( noteDuration < predelay + attack + hold + decay )
	{
		if (sustain == 1)// Without this if statement, decay phase is muted if sustain == 1
		{
			returnVal = 1;
		}
		else
		{
			returnVal = (((((sustain - 1) / decay) * (noteDuration - hold - attack - predelay) + 1) - sustain) / (1 - sustain)) * (1 - sustain) + sustain;
		}
	}
	else
	{
		returnVal = sustain;
	}

	if( noteReleaseTime )
	{
		if( noteDuration < release + noteReleaseTime )
		{
			returnVal *= (-1 / release) * (noteDuration - noteReleaseTime) + 1;
		}
		else
		{
			returnVal = 0;
		}
	}
	
	return returnVal;
}



/*inline void sSynth::calcFilter( sample_t &outSamp, sample_t inSamp, int which, float cutoff, sample_rate_t Fs, NotePlayHandle * nph, float feedback, float resonance, float combFreq )
{
	m_temp1 = combFreq;

	if (m_filtDelayBuf[which].size() < m_temp1)
	{
		m_filtDelayBuf[which].resize(m_temp1);
	}

	++m_filtFeedbackLoc[which];
	if (m_filtFeedbackLoc[which] > m_temp1 - 1)
	{
		m_filtFeedbackLoc[which] = 0;
	}

	inSamp += m_filtDelayBuf[which][m_filtFeedbackLoc[which]] * feedback;

	m_w0 = D_2PI * cutoff / Fs;

	m_temp2 = cos(m_w0);
	m_alpha = sin(m_w0) / ( resonance / 2.f );
	m_b0 = (1 - m_temp2) * 0.5f;
	m_b1 = 1 - m_temp2;
	m_b2 = m_b0;
	m_a0 = 1 + m_alpha;
	m_a1 = -2 * m_temp2;
	m_a2 = 1 - m_alpha;

	m_temp1 = m_b0/m_a0;
	m_temp2 = m_b1/m_a0;
	m_temp3 = m_b2/m_a0;
	m_temp4 = m_a1/m_a0;
	m_temp5 = m_a2/m_a0;
	filtY[which][0] = m_temp1*inSamp + m_temp2*filtX[which][1] + m_temp3*filtX[which][2] - m_temp4*filtY[which][1] - m_temp5*filtY[which][2];

	filtX[which][2] = filtX[which][1];
	filtX[which][1] = inSamp;
	filtY[which][2] = filtY[which][1];
	filtY[which][1] = filtY[which][0];

	outSamp = filtY[which][0];

	m_filtDelayBuf[which][m_filtFeedbackLoc[which]] = outSamp;
}*/




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

		ps->nextStringSample(outputSample, volume, soundSamples, startNote, endNote, velLeft, velRight, detuneRatio.value(), detune, startPoint, loopPoint1, loopPoint2, endPoint, loopMode, panning, crossfade, timestretchAmount, distortionType, distortionAmount, volPredelay, volAttack, volHold, volDecay, volSustain, volRelease, frame, amFreq, amAmnt, fmFreq, fmAmnt, filtCutoff, filtReso, filtMorph, grainSize);

		for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
		{
			_working_buffer[frame][chnl] = outputSample[chnl];
		}
	}

	// applyRelease( _working_buffer, _n ); // Nope!  Release is handled within the Sampler.

	instrumentTrack()->processAudioBuffer( _working_buffer, frames + offset, _n );
}


f_cnt_t Sampler::desiredReleaseFrames() const
{
	float releaseFrameNumber = 0;
	for( int a = 0; a < volRelease.size(); ++a )
	{
		releaseFrameNumber = qMax( releaseFrameNumber, volRelease[a]->value() );
	}
	return qMax( 64, int(releaseFrameNumber * Engine::mixer()->processingSampleRate()) );
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

	distortionTypeKnob = new Knob( knobDark_28, this );
	distortionTypeKnob->move( 246, 171 );
	distortionTypeKnob->setHintText( tr( "Distortion Type" ), "" );

	distortionAmountKnob = new Knob( knobDark_28, this );
	distortionAmountKnob->move( 276, 171 );
	distortionAmountKnob->setHintText( tr( "Distortion Amount" ), "" );

	volPredelayKnob = new Knob( knobDark_28, this );
	volPredelayKnob->move( 6, 141 );
	volPredelayKnob->setHintText( tr( "Volume Predelay" ), "" );

	volAttackKnob = new Knob( knobDark_28, this );
	volAttackKnob->move( 36, 141 );
	volAttackKnob->setHintText( tr( "Volume Attack" ), "" );

	volHoldKnob = new Knob( knobDark_28, this );
	volHoldKnob->move( 66, 141 );
	volHoldKnob->setHintText( tr( "Volume Hold" ), "" );

	volDecayKnob = new Knob( knobDark_28, this );
	volDecayKnob->move( 96, 141 );
	volDecayKnob->setHintText( tr( "Volume Decay" ), "" );

	volSustainKnob = new Knob( knobDark_28, this );
	volSustainKnob->move( 126, 141 );
	volSustainKnob->setHintText( tr( "Volume Sustain" ), "" );

	volReleaseKnob = new Knob( knobDark_28, this );
	volReleaseKnob->move( 156, 141 );
	volReleaseKnob->setHintText( tr( "Volume Release" ), "" );

	amFreqKnob = new Knob( knobDark_28, this );
	amFreqKnob->move( 256, 141 );
	amFreqKnob->setHintText( tr( "AM Frequency" ), "" );

	amAmntKnob = new Knob( knobDark_28, this );
	amAmntKnob->move( 286, 141 );
	amAmntKnob->setHintText( tr( "AM Amount" ), "" );

	fmFreqKnob = new Knob( knobDark_28, this );
	fmFreqKnob->move( 316, 141 );
	fmFreqKnob->setHintText( tr( "FM Frequency" ), "" );

	fmAmntKnob = new Knob( knobDark_28, this );
	fmAmntKnob->move( 346, 141 );
	fmAmntKnob->setHintText( tr( "FM Amount" ), "" );

	filtCutoffKnob = new Knob( knobDark_28, this );
	filtCutoffKnob->move( 316, 111 );
	filtCutoffKnob->setHintText( tr( "Filter Cutoff Frequency" ), "" );

	filtResoKnob = new Knob( knobDark_28, this );
	filtResoKnob->move( 346, 111 );
	filtResoKnob->setHintText( tr( "Filter Resonance" ), "" );

	filtMorphKnob = new Knob( knobDark_28, this );
	filtMorphKnob->move( 376, 111 );
	filtMorphKnob->setHintText( tr( "Filter Morph" ), "" );

	grainSizeKnob = new Knob( knobDark_28, this );
	grainSizeKnob->move( 216, 201 );
	grainSizeKnob->setHintText( tr( "Grain Size" ), "" );

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
		distortionTypeKnob->setModel( b->distortionType[b->sampleNumber.value()-1] );
		distortionAmountKnob->setModel( b->distortionAmount[b->sampleNumber.value()-1] );
		volPredelayKnob->setModel( b->volPredelay[b->sampleNumber.value()-1] );
		volAttackKnob->setModel( b->volAttack[b->sampleNumber.value()-1] );
		volHoldKnob->setModel( b->volHold[b->sampleNumber.value()-1] );
		volDecayKnob->setModel( b->volDecay[b->sampleNumber.value()-1] );
		volSustainKnob->setModel( b->volSustain[b->sampleNumber.value()-1] );
		volReleaseKnob->setModel( b->volRelease[b->sampleNumber.value()-1] );
		amFreqKnob->setModel( b->amFreq[b->sampleNumber.value()-1] );
		amAmntKnob->setModel( b->amAmnt[b->sampleNumber.value()-1] );
		fmFreqKnob->setModel( b->fmFreq[b->sampleNumber.value()-1] );
		fmAmntKnob->setModel( b->fmAmnt[b->sampleNumber.value()-1] );
		filtCutoffKnob->setModel( b->filtCutoff[b->sampleNumber.value()-1] );
		filtResoKnob->setModel( b->filtReso[b->sampleNumber.value()-1] );
		filtMorphKnob->setModel( b->filtMorph[b->sampleNumber.value()-1] );
		grainSizeKnob->setModel( b->grainSize[b->sampleNumber.value()-1] );
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
		float origLengthOfSample = ((filelength / 1000.f) * sample_rate);//in samples
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
		b->distortionType.push_back( new FloatModel(0, 0, 9, 1, b, tr("Distortion Type")) );
		b->distortionAmount.push_back( new FloatModel(0, 0, 100, 0.0001, b, tr("Distortion Amount")) );
		b->volPredelay.push_back( new FloatModel(0, 0, 2, 0.0001, b, tr("Volume Predelay")) );
		b->volAttack.push_back( new FloatModel(0, 0, 2, 0.0001, b, tr("Volume Attack")) );
		b->volHold.push_back( new FloatModel(0, 0, 4, 0.0001, b, tr("Volume Hold")) );
		b->volDecay.push_back( new FloatModel(0, 0, 4, 0.0001, b, tr("Volume Decay")) );
		b->volSustain.push_back( new FloatModel(1, 0, 1, 0.0001, b, tr("Volume Sustain")) );
		b->volRelease.push_back( new FloatModel(0, 0, 2, 0.0001, b, tr("Volume Release")) );
		b->amFreq.push_back( new FloatModel(100, 0, 21050, 0.0001, b, tr("AM Frequency")) );
		b->amAmnt.push_back( new FloatModel(0, 0, 1, 0.0001, b, tr("AM Amount")) );
		b->fmFreq.push_back( new FloatModel(100, 0, 21050, 0.0001, b, tr("FM Frequency")) );
		b->fmAmnt.push_back( new FloatModel(0, 0, 500, 0.0001, b, tr("FM Amount")) );
		b->filtCutoff.push_back( new FloatModel(20000, 20, 20000, 0.0001, b, tr("Filter Cutoff")) );
		b->filtReso.push_back( new FloatModel(0.707, 0.01, 10, 0.0001, b, tr("Filter Resonance")) );
		b->filtMorph.push_back( new FloatModel(1, 1, 5, 0.0001, b, tr("Filter Morph")) );
		b->grainSize.push_back( new FloatModel(1000, 2, 10000, 1, b, tr("Grain Size")) );

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




