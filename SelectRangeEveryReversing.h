
#include "avs/avisynth.h"
#include "avs/avisynth.h"
#include <string>

//#define WRITEDEBUGINFO
class SelectRangeEveryReversing : public GenericVideoFilter {
    int every, length;
    bool audio;
    PClip achild; 

#ifdef WRITEDEBUGINFO
    std::string debugOutputFilename ="";
#endif

public:
    SelectRangeEveryReversing   (PClip _child, int _every, int _length, int _offset, bool _audio, IScriptEnvironment* env);
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
    bool __stdcall GetParity(int n);
    static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
    void __stdcall GetAudio(void* buf, int64_t start, int64_t count, IScriptEnvironment* env);

};