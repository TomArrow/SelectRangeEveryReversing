
#include "avs/avisynth.h"

class SelectRangeEveryReversing : public GenericVideoFilter {
public:
	SelectRangeEveryReversing(PClip _child, IScriptEnvironment* env);
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

SelectRangeEveryReversing::SelectRangeEveryReversing(PClip _child, IScriptEnvironment* env) :
	GenericVideoFilter(_child) {
	if (!vi.IsPlanar() || !vi.IsYUV()) {
		env->ThrowError("InvertNeg: planar YUV data only!");
	}
}

PVideoFrame __stdcall SelectRangeEveryReversing::GetFrame(int n, IScriptEnvironment* env) {

    PVideoFrame src = child->GetFrame(n, env);
    PVideoFrame dst = env->NewVideoFrame(vi);

    const unsigned char* srcp;
    unsigned char* dstp;
    int src_pitch, dst_pitch, row_size, height;
    int p, x, y;

    int planes[] = { PLANAR_Y, PLANAR_V, PLANAR_U };

    for (p = 0; p < 3; p++) {
        srcp = src->GetReadPtr(planes[p]);
        dstp = dst->GetWritePtr(planes[p]);

        src_pitch = src->GetPitch(planes[p]);
        dst_pitch = dst->GetPitch(planes[p]);
        row_size = dst->GetRowSize(planes[p]);
        height = dst->GetHeight(planes[p]);

        for (y = 0; y < height; y++) {
            for (x = 0; x < row_size; x++) {
                dstp[x] = srcp[x] ^ 255;
            }
            srcp += src_pitch; // or srcp = srcp + src_pitch;
            dstp += dst_pitch; // or dstp = dstp + dst_pitch;
        }
    }
    return dst;
}

AVSValue __cdecl Create_SelectRangeEveryReversing(AVSValue args, void* user_data, IScriptEnvironment* env) {
    return new SelectRangeEveryReversing(args[0].AsClip(), env);
}


const AVS_Linkage* AVS_linkage = 0;

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment * env, const AVS_Linkage* const vectors) {
	AVS_linkage = vectors;
	env->AddFunction("SelectRangeEveryReversing", "c[every]i[length]i[offset]i[audio]b",
        Create_SelectRangeEveryReversing, 0);
	return "SelectRangeEvery, but every second range is reversed";
}