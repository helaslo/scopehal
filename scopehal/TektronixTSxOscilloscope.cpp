/***********************************************************************************************************************
*                                                                                                                      *
* libscopehal v0.1                                                                                                     *
*                                                                                                                      *
* Copyright (c) 2012-2023 Andrew D. Zonenberg and contributors                                                         *
* All rights reserved.                                                                                                 *
*                                                                                                                      *
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the     *
* following conditions are met:                                                                                        *
*                                                                                                                      *
*    * Redistributions of source code must retain the above copyright notice, this list of conditions, and the         *
*      following disclaimer.                                                                                           *
*                                                                                                                      *
*    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the       *
*      following disclaimer in the documentation and/or other materials provided with the distribution.                *
*                                                                                                                      *
*    * Neither the name of the author nor the names of any contributors may be used to endorse or promote products     *
*      derived from this software without specific prior written permission.                                           *
*                                                                                                                      *
* THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   *
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL *
* THE AUTHORS BE HELD LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES        *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR       *
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
* POSSIBILITY OF SUCH DAMAGE.                                                                                          *
*                                                                                                                      *
***********************************************************************************************************************/

#include "scopehal.h"
#include "TektronixTSxOscilloscope.h"
#include "EdgeTrigger.h"

#include <cinttypes>

#ifdef _WIN32
#include <chrono>
#include <thread>
#endif

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Construction / destruction

TektronixTSxOscilloscope::TektronixTSxOscilloscope(SCPITransport* transport)
	: SCPIDevice(transport)
	, SCPIInstrument(transport)
	, m_triggerArmed(false)
	, m_triggerOneShot(false)
	, m_lastAcqCount(0)
{
	//make sure all setup commands finish before we proceed
	m_transport->SendCommandQueued("ACQUIRE:STATE 0");
	m_transport->SendCommandQueued("ACQUIRE:MODE SAMPLE");
	m_transport->SendCommandQueued("WFMPRE:ENC BIN");
	m_transport->SendCommandQueued("DATA:ENC RIB");
	m_transport->SendCommandQueued("HEADER 0");

	m_transport->SendCommandQueued("DATA:SOURCE CH1");
	m_transport->SendCommandQueued("WFMPRE:BYT_NR 1");
	m_transport->SendCommandQueued("DATA:START 1");
	m_transport->FlushCommandQueue();
	auto samplePointCountResp = Trim(m_transport->SendCommandQueuedWithReply("WFMPRE:NR_PT?"));
	size_t samplePointCount = stoi(samplePointCountResp.c_str());
	char tmp[32];
	snprintf(tmp, sizeof(tmp) - 1, "DATA:STOP %lu", samplePointCount);
	m_transport->SendCommandQueued(tmp);

	unsigned channels = 1;

	for(unsigned i = 0; i < channels; ++i)
	{
		char nameBuff[16];
		snprintf(nameBuff, sizeof(nameBuff) - 1, "CH%i", i + 1);
		auto chan = new OscilloscopeChannel(
			this, nameBuff, "#00FF00", Unit(Unit::UNIT_FS), Unit(Unit::UNIT_VOLTS), Stream::STREAM_TYPE_ANALOG, i);
		m_channels.push_back(chan);
		chan->SetDefaultDisplayName();
	}

	m_transport->FlushCommandQueue();
}

TektronixTSxOscilloscope::~TektronixTSxOscilloscope()
{
}
unsigned int TektronixTSxOscilloscope::GetInstrumentTypes() const
{
	return INST_OSCILLOSCOPE;
}
uint32_t TektronixTSxOscilloscope::GetInstrumentTypesForChannel(size_t i) const
{
	return INST_OSCILLOSCOPE;
}
void TektronixTSxOscilloscope::FlushConfigCache()
{
	SCPIOscilloscope::FlushConfigCache();
}
bool TektronixTSxOscilloscope::IsChannelEnabled(size_t i)
{
	return true;
}
void TektronixTSxOscilloscope::EnableChannel(size_t i)
{
}
void TektronixTSxOscilloscope::DisableChannel(size_t i)
{
}
OscilloscopeChannel::CouplingType TektronixTSxOscilloscope::GetChannelCoupling(size_t i)
{
	return OscilloscopeChannel::COUPLE_DC_1M;
}
void TektronixTSxOscilloscope::SetChannelCoupling(size_t i, OscilloscopeChannel::CouplingType type)
{
}
std::vector<OscilloscopeChannel::CouplingType> TektronixTSxOscilloscope::GetAvailableCouplings(size_t i)
{
	return {};
}
double TektronixTSxOscilloscope::GetChannelAttenuation(size_t i)
{
	return 1;
}
void TektronixTSxOscilloscope::SetChannelAttenuation(size_t i, double atten)
{
}
std::vector<unsigned int> TektronixTSxOscilloscope::GetChannelBandwidthLimiters(size_t i)
{
	return SCPIOscilloscope::GetChannelBandwidthLimiters(i);
}
unsigned int TektronixTSxOscilloscope::GetChannelBandwidthLimit(size_t i)
{
	return 1e6;
}
void TektronixTSxOscilloscope::SetChannelBandwidthLimit(size_t i, unsigned int limit_mhz)
{
}
float TektronixTSxOscilloscope::GetChannelVoltageRange(size_t i, size_t stream)
{
	return 270;
}
void TektronixTSxOscilloscope::SetChannelVoltageRange(size_t i, size_t stream, float range)
{
}
OscilloscopeChannel* TektronixTSxOscilloscope::GetExternalTrigger()
{
	return nullptr;
}
float TektronixTSxOscilloscope::GetChannelOffset(size_t i, size_t stream)
{
	return 0;
}
void TektronixTSxOscilloscope::SetChannelOffset(size_t i, size_t stream, float offset)
{
}
Oscilloscope::TriggerMode TektronixTSxOscilloscope::PollTrigger()
{
	LogWarning("PollTrigger\r\n");
	if(m_triggerArmed)
	{
		auto resp = Trim(m_transport->SendCommandQueuedWithReply("ACQUIRE:STATE?"));
		LogWarning("Resp: %s\r\n", resp.c_str());
		if(resp == "0")
		{
			m_triggerArmed = false;
			auto acq_c_resp = Trim(m_transport->SendCommandQueuedWithReply("ACQUIRE:NUMACQ?"));
			LogWarning("Resp: %s\r\n", acq_c_resp.c_str());
			uint32_t acq_n = stoi(acq_c_resp.c_str());
			if(acq_n != 0 && acq_n != m_lastAcqCount)
			{
				LogWarning("Weeee \r\n");
				m_lastAcqCount = acq_n;
				return TRIGGER_MODE_TRIGGERED;
			}
			m_lastAcqCount = acq_n;
			return TRIGGER_MODE_STOP;
		}
		if(resp == "1")
		{
			return TRIGGER_MODE_RUN;
		}
		if(resp == "3")
		{
			return TRIGGER_MODE_TRIGGERED;
		}
	}
	return TRIGGER_MODE_STOP;
}
bool TektronixTSxOscilloscope::AcquireData()
{
	ChannelsDownloadStarted();
	ChannelsDownloadStatusUpdate(0, InstrumentChannel::DownloadState::DOWNLOAD_IN_PROGRESS, 0.0);
	// m_transport->SendCommandQueued("DATA:SOURCE CH1");
	// m_transport->SendCommandQueued("WFMPRE:BYT_NR 1");
	// m_transport->SendCommandQueued("DATA:START 1");
	auto samplePointCountResp = Trim(m_transport->SendCommandQueuedWithReply("WFMPRE:NR_PT?"));
	// auto timescaleResp=Trim(m_transport->SendCommandQueuedWithReply("WFMPRE:XINCR?"));
	// auto timeOffsetResp=Trim(m_transport->SendCommandQueuedWithReply("WFMPRE:XZERO?"));
	// auto voltsResp=Trim(m_transport->SendCommandQueuedWithReply("WFMPRE:YMULT?"));
	// auto voltsOffsetResp=Trim(m_transport->SendCommandQueuedWithReply("WFMPRE:YOFF?"));
	unsigned samplePointCount = stoi(samplePointCountResp.c_str());
	// char tmp[32];
	// snprintf(tmp, sizeof(tmp)-1, "DATA:STOP %lu", samplePointCount);
	// m_transport->SendCommandQueued(tmp);
	// m_transport->SendCommandQueued("DATA:ENC RIB");
	// m_transport->FlushCommandQueue();

	size_t msglen;
	// unsigned samplePointCount=msglen;
	std::vector<int8_t> sampleBytes;
	unsigned batchSize = 40;
	unsigned totalPoints=2500;
	for(unsigned i = 0; i < totalPoints / batchSize; ++i)
	{
		std::vector<int8_t> respVector(batchSize);
		char tmp[32];
		snprintf(tmp, sizeof(tmp) - 1, "DATA:START %u", ((i)*batchSize) + 1);
		m_transport->SendCommandQueued(tmp);
		snprintf(tmp, sizeof(tmp) - 1, "DATA:STOP %u", (i + 1) * batchSize);
		m_transport->SendCommandQueued(tmp);
		m_transport->FlushCommandQueue();
		int8_t* samples = (int8_t*)m_transport->SendCommandImmediateWithRawBlockReply("CURVE?", msglen);
		for(unsigned i = 0; i < msglen; i++)
		{
			sampleBytes.push_back(samples[i]);
		}
		ChannelsDownloadStatusUpdate(1, InstrumentChannel::DownloadState::DOWNLOAD_IN_PROGRESS, totalPoints*1.0/(i*batchSize));
	}
	auto cap = AllocateAnalogWaveform("CH1");
	cap->Resize(sampleBytes.size());
	cap->PrepareForCpuAccess();
	double now = GetTime();
	cap->m_timescale = FS_PER_MICROSECOND;
	cap->m_triggerPhase = 0;
	cap->m_startTimestamp = floor(now);
	cap->m_startFemtoseconds = (now - floor(now)) * FS_PER_SECOND;
	AddWaveformToAnalogPool(cap);
	for(unsigned i = 0; i < sampleBytes.size(); ++i)
	{
		cap->m_samples[i] = sampleBytes[i];
	}
	cap->MarkModifiedFromCpu();
	ChannelsDownloadFinished();
	m_pendingWaveformsMutex.lock();
	size_t num_pending = 1;	   //TODO: segmented capture support
	for(size_t i = 0; i < num_pending; i++)
	{
		SequenceSet s;
		for(size_t j = 0; j < 1; j++)
		{
			s[GetOscilloscopeChannel(j)] = cap;
		}
		m_pendingWaveforms.push_back(s);
	}
	m_pendingWaveformsMutex.unlock();
	return true;
}
void TektronixTSxOscilloscope::Start()
{
	// m_transport->SendCommand("ACQUIRE:RUN 1");
	// m_triggerArmed = true;
}
void TektronixTSxOscilloscope::StartSingleTrigger()
{
	m_transport->SendCommandQueued("ACQUIRE:STOPAFTER SEQUENCE");
	m_transport->SendCommandQueued("ACQUIRE:STATE RUN");
	m_transport->FlushCommandQueue();
	m_triggerArmed = true;
}
void TektronixTSxOscilloscope::Stop()
{
	m_transport->SendCommand("ACQUIRE:STATE 0");
}
void TektronixTSxOscilloscope::ForceTrigger()
{
}
bool TektronixTSxOscilloscope::IsTriggerArmed()
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);
	return m_triggerArmed;
}
void TektronixTSxOscilloscope::PushTrigger()
{
}
void TektronixTSxOscilloscope::PullTrigger()
{
}
std::vector<uint64_t> TektronixTSxOscilloscope::GetSampleRatesNonInterleaved()
{
	return {};
}
std::vector<uint64_t> TektronixTSxOscilloscope::GetSampleRatesInterleaved()
{
	return {};
}
std::set<Oscilloscope::InterleaveConflict> TektronixTSxOscilloscope::GetInterleaveConflicts()
{
	return {};
}
std::vector<uint64_t> TektronixTSxOscilloscope::GetSampleDepthsNonInterleaved()
{
	return {};
}
std::vector<uint64_t> TektronixTSxOscilloscope::GetSampleDepthsInterleaved()
{
	return {};
}
uint64_t TektronixTSxOscilloscope::GetSampleRate()
{
	return {};
}
uint64_t TektronixTSxOscilloscope::GetSampleDepth()
{
	return {};
}
void TektronixTSxOscilloscope::SetSampleDepth(uint64_t depth)
{
}
void TektronixTSxOscilloscope::SetSampleRate(uint64_t rate)
{
}
void TektronixTSxOscilloscope::SetTriggerOffset(int64_t offset)
{
}
int64_t TektronixTSxOscilloscope::GetTriggerOffset()
{
	return 0;
}
bool TektronixTSxOscilloscope::IsInterleaving()
{
	return false;
}
bool TektronixTSxOscilloscope::SetInterleaving(bool combine)
{
	return false;
}
void TektronixTSxOscilloscope::ForceHDMode(bool mode)
{
}
void TektronixTSxOscilloscope::ResynchronizeSCPI()
{
}
bool TektronixTSxOscilloscope::ReadPreamble(std::string& preamble_in, mso56_preamble& preamble_out)
{
	return false;
}
void TektronixTSxOscilloscope::DetectProbes()
{
}
void TektronixTSxOscilloscope::PullEdgeTrigger()
{
}
void TektronixTSxOscilloscope::PushEdgeTrigger(EdgeTrigger* trig)
{
}
void TektronixTSxOscilloscope::PullPulseWidthTrigger()
{
}
void TektronixTSxOscilloscope::PushPulseWidthTrigger(PulseWidthTrigger* trig)
{
}
void TektronixTSxOscilloscope::PullDropoutTrigger()
{
}
void TektronixTSxOscilloscope::PushDropoutTrigger(DropoutTrigger* trig)
{
}
void TektronixTSxOscilloscope::PullRuntTrigger()
{
}
void TektronixTSxOscilloscope::PushRuntTrigger(RuntTrigger* trig)
{
}
void TektronixTSxOscilloscope::PullSlewRateTrigger()
{
}
void TektronixTSxOscilloscope::PushSlewRateTrigger(SlewRateTrigger* trig)
{
}
void TektronixTSxOscilloscope::PullWindowTrigger()
{
}
void TektronixTSxOscilloscope::PushWindowTrigger(WindowTrigger* trig)
{
}
float TektronixTSxOscilloscope::ReadTriggerLevelMSO56(OscilloscopeChannel* chan)
{
	return 0;
}
void TektronixTSxOscilloscope::SetTriggerLevelMSO56(Trigger* trig)
{
}
bool TektronixTSxOscilloscope::IsEnableStateDirty(size_t chan)
{
	return false;
}
void TektronixTSxOscilloscope::FlushChannelEnableStates()
{
}
std::string TektronixTSxOscilloscope::GetDriverNameInternal()
{
	return "tektronix-tsX";
}
