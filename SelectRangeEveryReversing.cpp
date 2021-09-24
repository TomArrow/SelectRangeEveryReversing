
#include "avs/avs/minmax.h"
#include "avs/avisynth.h"
#include "SelectRangeEveryReversing.h"

SelectRangeEveryReversing::SelectRangeEveryReversing(PClip _child, int _every, int _length, int _offset, bool _audio, IScriptEnvironment* env)
    : GenericVideoFilter(_child), audio(_audio), achild(_child)
{
    const int64_t num_audio_samples = vi.num_audio_samples;

    AVSValue trimargs[3] = { _child, _offset, 0 };
    PClip c = env->Invoke("Trim", AVSValue(trimargs, 3)).AsClip();
    child = c;
    vi = c->GetVideoInfo();

    every = clamp(_every, 1, vi.num_frames);
    length = clamp(_length, 1, every);

    const int n = vi.num_frames;
    vi.num_frames = (n / every) * length + (n % every < length ? n % every : length);

    if (audio && vi.HasAudio()) {
        vi.num_audio_samples = vi.AudioSamplesFromFrames(vi.num_frames);
    }
    else {
        vi.num_audio_samples = num_audio_samples; // Undo Trim's work!
    }
}


PVideoFrame __stdcall SelectRangeEveryReversing::GetFrame(int n, IScriptEnvironment* env)
{

    //Original: long frameNumber = (n / length) * every + (n % length);
    int rangeIndex = n / length;
    int frameNumber = rangeIndex  * every + (rangeIndex % 2==0?(n % length): (length- 1-n % length));
    return child->GetFrame(frameNumber, env);
}


bool __stdcall SelectRangeEveryReversing::GetParity(int n)
{
    return child->GetParity((n / length) * every + (n % length));
}


void __stdcall SelectRangeEveryReversing::GetAudio(void* buf, int64_t start, int64_t count, IScriptEnvironment* env)
{
    if (!audio) {
        // Use original unTrim'd child
        achild->GetAudio(buf, start, count, env);
        return;
    }

    int64_t samples_filled = 0;
    BYTE* samples = (BYTE*)buf;
    const int bps = vi.BytesPerAudioSample();
    int startframe = vi.FramesFromAudioSamples(start);
    int64_t general_offset = start - vi.AudioSamplesFromFrames(startframe);  // General compensation for startframe rounding.

    while (samples_filled < count) {
        const int iteration = startframe / length;                    // Which iteration is this.
        const int iteration_into = startframe % length;               // How far, in frames are we into this iteration.
        const int iteration_left = length - iteration_into;           // How many frames is left of this iteration.

        const int64_t iteration_left_samples = vi.AudioSamplesFromFrames(iteration_left);
        // This is the number of samples we can get without either having to skip, or being finished.
        const int64_t getsamples = min(iteration_left_samples, count - samples_filled);
        const int64_t start_offset = vi.AudioSamplesFromFrames(iteration * every + iteration_into) + general_offset;

        child->GetAudio(&samples[samples_filled * bps], start_offset, getsamples, env);
        samples_filled += getsamples;
        startframe = (iteration + 1) * every;
        general_offset = 0; // On the following loops, general offset should be 0, as we are either skipping.
    }
}

AVSValue __cdecl SelectRangeEveryReversing::Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
    //AVS_UNUSED(user_data);
    return new SelectRangeEveryReversing(args[0].AsClip(), args[1].AsInt(1500), args[2].AsInt(50), args[3].AsInt(0), args[4].AsBool(true), env);
}



const AVS_Linkage* AVS_linkage = 0;

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment * env, const AVS_Linkage* const vectors) {
	AVS_linkage = vectors;
	env->AddFunction("SelectRangeEveryReversing", "c[every]i[length]i[offset]i[audio]b",
        SelectRangeEveryReversing::Create, 0);
	return "SelectRangeEvery, but every second range is reversed";
}