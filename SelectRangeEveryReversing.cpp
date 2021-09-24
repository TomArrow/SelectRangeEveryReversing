// The following code is mostly adapted from the InvertNeg example and avs_core/filters/field.cpp from AviSynth+
// I'm bad at this legal stuff so I'm not sure what other stuff I should note here.
// 
// This is what the original file says at the top:
// Avisynth v2.5.  Copyright 2002 Ben Rudiak-Gould et al.
// http://www.avisynth.org
//
// Also says it uses GNU General Public License version 2 or above.
// Not sure if I have to include the entire block of license stuff here.
// If I do, let me know and I will.
// 

#define _CRT_SECURE_NO_WARNINGS

#include "avs/avs/minmax.h"
#include "avs/avisynth.h"
#include "SelectRangeEveryReversing.h"
#include <string>
#include <fstream>

//#define WRITEDEBUGINFO

SelectRangeEveryReversing::SelectRangeEveryReversing(PClip _child, int _every, int _length, int _offset, bool _audio, IScriptEnvironment* env)
    : GenericVideoFilter(_child), audio(_audio), achild(_child)
{
#ifdef WRITEDEBUGINFO
    if (debugOutputFilename == "") {
        FILE* test;
        int counter = 0;
        while (test = fopen((debugOutputFilename = "SelectRangeEveryReversing_Debug" + std::to_string(counter) + ".txt").c_str(), "r")) {
            fclose(test);
            counter++;
        }
    }
#endif

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

        if (iteration % 2 == 0) {

            // Normal:
            const int iteration_left = length - iteration_into;           // How many frames is left of this iteration.

            const int64_t iteration_left_samples = vi.AudioSamplesFromFrames(iteration_left);
            // This is the number of samples we can get without either having to skip, or being finished.
            const int64_t getsamples = min(iteration_left_samples, count - samples_filled);
            const int64_t start_offset = vi.AudioSamplesFromFrames(iteration * every + iteration_into) + general_offset;

            child->GetAudio(&samples[samples_filled * bps], start_offset, getsamples, env);
            samples_filled += getsamples;
            //startframe = (iteration + 1) * every;
            startframe = (iteration + 1) * length;
            general_offset = 0; // On the following loops, general offset should be 0, as we are either skipping.

        }
        else {

            // 0  1  2  3  | 4  5  6  7  | 8
            // 01 01 01 01 | 01 01 01 01 | 01

            // Reverse:          
            //const int currentFrame = iteration * every + (length - 1 - startframe % length);
            const int64_t length_in_samples = vi.AudioSamplesFromFrames(length);
            //const int64_t offset_from_last_iter = start - vi.AudioSamplesFromFrames(iteration*length);
            const int64_t offset_from_last_iter = general_offset + vi.AudioSamplesFromFrames(iteration_into);
            const int64_t inversed_offset = length_in_samples-1 - offset_from_last_iter;
            const int64_t first_sample_to_fetch = vi.AudioSamplesFromFrames(iteration * every);
            const int64_t last_sample_to_fetch = vi.AudioSamplesFromFrames(iteration * every) + inversed_offset;
            const int64_t count_of_samples_to_fetch = last_sample_to_fetch - first_sample_to_fetch+1;
            const int64_t count_of_samples_to_fetch_clamped = min(count_of_samples_to_fetch, count - samples_filled);
            const int64_t count_of_samples_to_fetch_clamped_difference = count_of_samples_to_fetch- count_of_samples_to_fetch_clamped;

#ifdef WRITEDEBUGINFO
            std::ofstream myfile;
            myfile.open(debugOutputFilename, std::ios::out | std::ios::app);
            myfile << "DebugInfo: \n";
            myfile << "samples_filled: " << samples_filled << "\n";
            myfile << "startframe: " << startframe << "\n";
            myfile << "general_offset: " << general_offset << "\n";
            myfile << "length_in_samples: " << length_in_samples << "\n";
            myfile << "offset_from_last_iter: " << offset_from_last_iter << "\n";
            myfile << "inversed_offset: " << inversed_offset << "\n";
            myfile << "first_sample_to_fetch: " << first_sample_to_fetch << "\n";
            myfile << "last_sample_to_fetch: " << last_sample_to_fetch << "\n";
            myfile << "count_of_samples_to_fetch: " << count_of_samples_to_fetch << "\n";
            myfile << "count_of_samples_to_fetch_clamped: " << count_of_samples_to_fetch_clamped << "\n";
            myfile << "\n\n\n\n";
            myfile.flush();
            myfile.close();
#endif

            BYTE* tmpBuf = new BYTE[count_of_samples_to_fetch_clamped *(int64_t)bps];

            child->GetAudio(tmpBuf, first_sample_to_fetch+ count_of_samples_to_fetch_clamped_difference, count_of_samples_to_fetch_clamped, env);

            BYTE* offsettedSamplePointer = &samples[samples_filled * bps];
            for (int64_t i = 0; i < count_of_samples_to_fetch_clamped; i++) {
                int64_t invertedPosition = count_of_samples_to_fetch_clamped - 1 - i;
                for (int b = 0; b < bps; b++) {
                    offsettedSamplePointer[i * bps + b] = tmpBuf[invertedPosition * bps + b];
                }
            }
            samples_filled += count_of_samples_to_fetch_clamped;

            //startframe = (iteration + 1) * every;
            startframe = (iteration + 1) * length;
            general_offset = 0; // On the following loops, general offset should be 0, as we are either skipping.
            
                                /*
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
            */

            delete[] tmpBuf;
        }
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