//  interface for supercollider plugins
//  Copyright (C) 2009, 2010 Tim Blechmann
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; see the file COPYING.  If not, write to
//  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//  Boston, MA 02111-1307, USA.

#include <cstdarg>
#include <cstdio>

#include "sndfile.hh"

#include "sc_plugin_interface.hpp"
#include "sc_ugen_factory.hpp"
#include "sc_synth.hpp"

#include "nova-simd/simd_memory.hpp"

#include "../server/server_args.hpp"
#include "../server/memory_pool.hpp"
#include "../server/server.hpp"
#include "../utilities/malloc_aligned.hpp"
#include "../utilities/sized_array.hpp"

#include "SC_Samp.h"
#include "SC_Prototypes.h"
#include "SC_Errors.h"
#include "clz.h"
#include "SC_fftlib.h"

// undefine the shadowed scfft functions
#undef scfft_create
#undef scfft_dofft
#undef scfft_doifft
#undef scfft_destroy

namespace nova
{
namespace
{

void pause_node(Unit * unit)
{
    server_node * node = static_cast<sc_synth*>(unit->mParent);
    sc_factory->add_pause_node(node);
}

void free_node(Unit * unit)
{
    server_node * node = static_cast<sc_synth*>(unit->mParent);
    sc_factory->add_done_node(node);
}

void free_node_and_preceding(Unit * unit)
{
    server_node * node = static_cast<sc_synth*>(unit->mParent);
    sc_factory->add_done_node(node);

    if (node->get_parent()->is_parallel()) {
        std::cerr << "parallel groups have no notion of preceding nodes" << std::endl;
        return;
    }

    server_node * preceding = node->previous_node();
    if (preceding)
        sc_factory->add_done_node(preceding);
}

void free_node_and_pause_preceding(Unit * unit)
{
    server_node * node = static_cast<sc_synth*>(unit->mParent);
    sc_factory->add_done_node(node);

    if (node->get_parent()->is_parallel()) {
        std::cerr << "parallel groups have no notion of preceding nodes" << std::endl;
        return;
    }

    server_node * preceding = node->previous_node();
    if (preceding)
        sc_factory->add_pause_node(preceding);
}

void free_node_and_preceding_children(Unit * unit)
{
    server_node * node = static_cast<sc_synth*>(unit->mParent);
    sc_factory->add_done_node(node);

    if (node->get_parent()->is_parallel()) {
        std::cerr << "parallel groups have no notion of preceding nodes" << std::endl;
        return;
    }

    server_node * preceding = node->previous_node();
    if (!preceding)
        return;
    if (preceding->is_synth())
        sc_factory->add_done_node(preceding);
    else {
        abstract_group * preceding_group = static_cast<abstract_group*>(preceding);
        sc_factory->add_freeAll_node(preceding_group);
    }
}


void free_node_and_preceding_deep(Unit * unit)
{
    server_node * node = static_cast<sc_synth*>(unit->mParent);
    sc_factory->add_done_node(node);

    if (node->get_parent()->is_parallel()) {
        std::cerr << "parallel groups have no notion of preceding nodes" << std::endl;
        return;
    }

    server_node * preceding = node->previous_node();
    if (!preceding)
        return;
    if (preceding->is_synth())
        sc_factory->add_done_node(preceding);
    else {
        abstract_group * preceding_group = static_cast<abstract_group*>(preceding);
        sc_factory->add_freeDeep_node(preceding_group);
    }
}

void free_node_and_all_preceding(Unit * unit)
{
    server_node * node = static_cast<sc_synth*>(unit->mParent);
    sc_factory->add_done_node(node);

    if (node->get_parent()->is_parallel()) {
        std::cerr << "parallel groups have no notion of preceding nodes" << std::endl;
        return;
    }

    for(;;)
    {
        node = node->previous_node();
        if (node)
            sc_factory->add_done_node(node);
        else
            return;
    }
}

void free_node_and_following(Unit * unit)
{
    server_node * node = static_cast<sc_synth*>(unit->mParent);
    sc_factory->add_done_node(node);

    if (node->get_parent()->is_parallel()) {
        std::cerr << "parallel groups have no notion of following nodes" << std::endl;
        return;
    }

    server_node * next = node->next_node();
    if (next)
        sc_factory->add_done_node(next);
}

void free_node_and_pause_following(Unit * unit)
{
    server_node * node = static_cast<sc_synth*>(unit->mParent);
    sc_factory->add_done_node(node);

    if (node->get_parent()->is_parallel()) {
        std::cerr << "parallel groups have no notion of following nodes" << std::endl;
        return;
    }

    server_node * next = node->next_node();
    if (next)
        sc_factory->add_pause_node(next);
}

void free_node_and_following_children(Unit * unit)
{
    server_node * node = static_cast<sc_synth*>(unit->mParent);
    sc_factory->add_done_node(node);

    if (node->get_parent()->is_parallel()) {
        std::cerr << "parallel groups have no notion of following nodes" << std::endl;
        return;
    }

    server_node * following = node->previous_node();
    if (!following)
        return;
    if (following->is_synth())
        sc_factory->add_done_node(following);
    else {
        abstract_group * following_group = static_cast<abstract_group*>(following);
        sc_factory->add_freeAll_node(following_group);
    }
}

void free_node_and_following_deep(Unit * unit)
{
    server_node * node = static_cast<sc_synth*>(unit->mParent);
    sc_factory->add_done_node(node);

    if (node->get_parent()->is_parallel()) {
        std::cerr << "parallel groups have no notion of following nodes" << std::endl;
        return;
    }

    server_node * following = node->previous_node();
    if (!following)
        return;
    if (following->is_synth())
        sc_factory->add_done_node(following);
    else {
        abstract_group * following_group = static_cast<abstract_group*>(following);
        sc_factory->add_freeDeep_node(following_group);
    }
}

void free_node_and_all_following(Unit * unit)
{
    server_node * node = static_cast<sc_synth*>(unit->mParent);
    sc_factory->add_done_node(node);

    if (node->get_parent()->is_parallel()) {
        std::cerr << "parallel groups have no notion of following nodes" << std::endl;
        return;
    }

    for(;;)
    {
        node = node->previous_node();
        if (node)
            sc_factory->add_done_node(node);
        else
            return;
    }
}

void free_group_members(Unit * unit)
{
    server_node * node = static_cast<sc_synth*>(unit->mParent);
    abstract_group * group = const_cast<abstract_group*>(node->get_parent());

    sc_factory->add_freeAll_node(group);
}

void free_parent_group(Unit * unit)
{
    server_node * node = static_cast<sc_synth*>(unit->mParent);
    abstract_group * group = const_cast<abstract_group*>(node->get_parent());
    sc_factory->add_done_node(group);
}


} /* namespace */
} /* namespace nova */

extern "C"
{

bool define_unit(const char *inUnitClassName, size_t inAllocSize,
                 UnitCtorFunc inCtor, UnitDtorFunc inDtor, uint32 inFlags)
{
    try {
        nova::sc_factory->register_ugen(inUnitClassName, inAllocSize, inCtor, inDtor, inFlags);
        return true;
    }
    catch(...)
    {
        return false;
    }
}

bool define_bufgen(const char * name, BufGenFunc func)
{
    try {
        nova::sc_factory->register_bufgen(name, func);
        return true;
    }
    catch(...)
    {
        return false;
    }
}

bool define_unitcmd(const char * unitClassName, const char * cmdName, UnitCmdFunc inFunc)
{
    return nova::sc_factory->register_ugen_command_function(unitClassName, cmdName, inFunc);
}


bool define_plugincmd(const char * name, PlugInCmdFunc func, void * user_data)
{
    std::cerr << "plugin commands not implemented: " << name << std::endl;
    return false;
}

void * rt_alloc(World * dummy, size_t size)
{
    if (size)
        return nova::rt_pool.malloc(size);
    else
        return NULL;
}

void * rt_realloc(World * dummy, void * ptr, size_t size)
{
    return nova::rt_pool.realloc(ptr, size);
}

void rt_free(World * dummy, void * ptr)
{
    if (ptr)
        nova::rt_pool.free(ptr);
}

void * nrt_alloc(size_t size)
{
    return malloc(size);
}

void * nrt_realloc(void * ptr, size_t size)
{
    return realloc(ptr, size);
}

void nrt_free(void * ptr)
{
    free(ptr);
}

void clear_outputs(Unit *unit, int samples)
{
    size_t outputs = unit->mNumOutputs;

    if ((samples & 15) == 0)
        for (size_t i=0; i!=outputs; ++i)
            nova::zerovec_simd(OUT(i), samples);
    else
        for (size_t i=0; i!=outputs; ++i)
            nova::zerovec(OUT(i), samples);
}

void node_end(struct Node * node)
{
    nova::server_node * s = nova::instance->find_node(node->mID);
    nova::sc_factory->add_done_node(s);
}

int print(const char *fmt, ...)
{
    va_list vargs;
    va_start(vargs, fmt);

    char data[1024];

    vsprintf(data, fmt, vargs);

    std::cout << data << std::endl;

    va_end(vargs);
    return 0;
}

/* todo: we need to implement most of the done actions */
void done_action(int done_action, struct Unit *unit)
{
    using namespace nova;
    switch(done_action)
    {
    case 0:
        // do nothing when the UGen is finished
        return;

    case 1:
        // pause the enclosing synth, but do not free it
        nova::pause_node(unit);
        return;
    case 2:
        // free the enclosing synth
        nova::free_node(unit);
        return;

    case 3:
        // free both this synth and the preceding node
        nova::free_node_and_preceding(unit);
        return;

    case 4:
        // free both this synth and the following node
        nova::free_node_and_following(unit);
        return;

    case 5:
        // free this synth; if the preceding node is a group then do g_freeAll on it, else free it
        nova::free_node_and_preceding_children(unit);
        return;

    case 6:
        nova::free_node_and_following_children(unit);
        // free this synth; if the following node is a group then do g_freeAll on it, else free it
        return;

    case 7:
        //free this synth and all preceding nodes in this group
        nova::free_node_and_all_preceding(unit);
        return;

    case 8:
        //free this synth and all following nodes in this group
        nova::free_node_and_all_following(unit);
        return;

    case 9:
        // free this synth and pause the preceding node
        nova::free_node_and_pause_preceding(unit);
        return;

    case 10:
        // free this synth and pause the following node
        nova::free_node_and_pause_following(unit);
        return;

    case 11:
        // free this synth and if the preceding node is a group then do g_deepFree on it, else free it
        nova::free_node_and_preceding_deep(unit);
        return;

    case 12:
        // free this synth and if the following node is a group then do g_deepFree on it, else free it
        nova::free_node_and_following_deep(unit);
        return;

    case 13:
        // free this synth and all other nodes in this group (before and after)
        nova::free_group_members(unit);
        return;

    case 14:
        // free the enclosing group and all nodes within it (including this synth)
        nova::free_parent_group(unit);
        return;

    default:
        return;
    }
}

int buf_alloc(SndBuf * buf, int channels, int frames, double samplerate)
{
    return nova::sc_factory->allocate_buffer(buf, channels, frames, samplerate);
}

void send_trigger(Node * unit, int trigger_id, float value)
{
    nova::instance->send_trigger(unit->mID, trigger_id, value);
}

void world_lock(World *world)
{
    world->mNRTLock->Lock();
}

void world_unlock(World *world)
{
    world->mNRTLock->Unlock();
}

void send_node_reply(Node* node, int reply_id, const char* command_name, int argument_count, const float* values)
{
    if (!nova::sc_factory->world.mRealTime)
        return;

    nova::instance->send_node_reply(node->mID, reply_id, command_name, argument_count, values);
}


} /* extern "C" */

namespace nova
{


inline void initialize_rate(Rate & rate, double sample_rate, int blocksize)
{
    rate.mSampleRate = sample_rate;
    rate.mSampleDur = 1. / sample_rate;
    rate.mRadiansPerSample = 2 * M_PI / sample_rate;

    rate.mBufLength = blocksize;
    rate.mBufDuration = blocksize / sample_rate;
    rate.mBufRate = sample_rate / blocksize;
    rate.mSlopeFactor = 1. / blocksize;

    div_t div_result = std::div(blocksize, 3);
    rate.mFilterLoops = div_result.quot;
    rate.mFilterRemain = div_result.rem;
    if (rate.mFilterLoops == 0.)
        rate.mFilterSlope = 0.;
    else
        rate.mFilterSlope = 1. / rate.mFilterLoops;
}


void sc_plugin_interface::initialize(void)
{
    done_nodes.reserve(64);
    pause_nodes.reserve(16);
    freeAll_nodes.reserve(16);
    freeDeep_nodes.reserve(16);

    server_arguments const & args = server_arguments::instance();

    /* define functions */
    sc_interface.fDefineUnit = &define_unit;
    sc_interface.fDefineBufGen = &define_bufgen;
    sc_interface.fDefinePlugInCmd = &define_plugincmd;
    sc_interface.fDefineUnitCmd = &define_unitcmd;

    /* interface functions */
    sc_interface.fNodeEnd = &node_end;
    sc_interface.fPrint = &print;
    sc_interface.fDoneAction = &done_action;

    /* wave tables */
    sc_interface.mSine = gSine;
    sc_interface.mCosecant = gInvSine;
    sc_interface.mSineSize = kSineSize;
    sc_interface.mSineWavetable = gSineWavetable;

    /* memory allocation */
    sc_interface.fRTAlloc = &rt_alloc;
    sc_interface.fRTRealloc = &rt_realloc;
    sc_interface.fRTFree = &rt_free;

    sc_interface.fNRTAlloc = &nrt_alloc;
    sc_interface.fNRTRealloc = &nrt_realloc;
    sc_interface.fNRTFree = &nrt_free;

    /* ugen functions */
    sc_interface.fClearUnitOutputs = clear_outputs;

    /* buffer functions */
    sc_interface.fBufAlloc = &buf_alloc;

    /* trigger functions */
    sc_interface.fSendTrigger = &send_trigger;
    sc_interface.fSendNodeReply = &send_node_reply;

    /* world locks */
    sc_interface.fNRTLock = &world_lock;
    sc_interface.fNRTUnlock = &world_unlock;
    world.mNRTLock = new SC_Lock();

    /* fft library */
    sc_interface.fSCfftCreate = &scfft_create;
    sc_interface.fSCfftDestroy = &scfft_destroy;
    sc_interface.fSCfftDoFFT = &scfft_dofft;
    sc_interface.fSCfftDoIFFT = &scfft_doifft;


    /* initialize world */
    /* control busses */
    world.mControlBus = new float[args.control_busses];
    world.mNumControlBusChannels = args.control_busses;
    world.mControlBusTouched = new int32[args.control_busses];
    for (size_t i = 0; i != args.control_busses; ++i)
        world.mControlBusTouched[i] = -1;

    /* audio busses */
    audio_busses.initialize(args.audio_busses, args.blocksize);

    world.mAudioBus = audio_busses.buffers;
    world.mNumAudioBusChannels = args.audio_busses;
    world.mAudioBusTouched = new int32[args.audio_busses];
    world.mAudioBusLocks = audio_busses.locks;
    for (size_t i = 0; i != args.audio_busses; ++i)
        world.mAudioBusTouched[i]   = -1;

    /* audio buffers */
    world.mNumSndBufs = args.buffers;
    world.mSndBufs = new SndBuf[world.mNumSndBufs];
    world.mSndBufsNonRealTimeMirror = new SndBuf[world.mNumSndBufs];
    world.mSndBufUpdates = new SndBufUpdates[world.mNumSndBufs];
    memset(world.mSndBufs, 0, world.mNumSndBufs*sizeof(SndBuf));
    memset(world.mSndBufsNonRealTimeMirror, 0, world.mNumSndBufs*sizeof(SndBuf));
    memset(world.mSndBufUpdates, 0, world.mNumSndBufs*sizeof(SndBufUpdates));
    world.mBufCounter = 0;

    async_buffer_guards.reset(new boost::mutex[world.mNumSndBufs]);

    /* audio settings */
    world.mBufLength = args.blocksize;
    world.mSampleRate = args.samplerate;

    initialize_rate(world.mFullRate, args.samplerate, args.blocksize);
    initialize_rate(world.mBufRate, double(args.samplerate)/args.blocksize, 1);

    world.mNumInputs = args.input_channels;
    world.mNumOutputs = args.output_channels;

    world.mRealTime = true; /* todo: for now we just support real-time synthesis */
}

void sc_done_action_handler::update_nodegraph(void)
{
    std::for_each(done_nodes.begin(), done_nodes.end(), boost::bind(&nova_server::free_node, instance, _1));
    done_nodes.clear();

    std::for_each(pause_nodes.begin(), pause_nodes.end(), boost::bind(&nova_server::node_pause, instance, _1));
    pause_nodes.clear();

    std::for_each(freeDeep_nodes.begin(), freeDeep_nodes.end(), boost::bind(&nova_server::group_free_deep, instance, _1));
    freeDeep_nodes.clear();

    std::for_each(freeAll_nodes.begin(), freeAll_nodes.end(), boost::bind(&nova_server::group_free_all, instance, _1));
    freeAll_nodes.clear();
}

sc_plugin_interface::~sc_plugin_interface(void)
{
    delete[] world.mAudioBusTouched;
    delete[] world.mControlBus;
    delete[] world.mControlBusTouched;
    delete[] world.mSndBufs;
    delete[] world.mSndBufsNonRealTimeMirror;
    delete[] world.mSndBufUpdates;
    delete world.mNRTLock;
}

namespace
{

sample * allocate_buffer(size_t samples)
{
    const size_t alloc_size = samples * sizeof(sample);
    sample * ret = (sample*)calloc_aligned(alloc_size);
    if (ret)
        mlock(ret, alloc_size);
    return ret;
}

void free_buffer(sample * chunk)
{
    free_aligned(chunk);
}

inline int32 bufmask(int32 x)
{
    return (1 << (31 - CLZ(x))) - 1;
}

inline void sndbuf_init(SndBuf * buf)
{
    buf->samplerate = 0;
    buf->sampledur = 0;
    buf->data = 0;
    buf->channels = 0;
    buf->samples = 0;
    buf->frames = 0;
    buf->mask = 0;
    buf->mask1 = 0;
    buf->coord = 0;
    buf->sndfile = 0;
}

inline void sndbuf_copy(SndBuf * dest, const SndBuf * src)
{
    dest->samplerate = src->samplerate;
    dest->sampledur = src->sampledur;
    dest->data = src->data;
    dest->channels = src->channels;
    dest->samples = src->samples;
    dest->frames = src->frames;
    dest->mask = src->mask;
    dest->mask1 = src->mask1;
    dest->coord = src->coord;
    dest->sndfile = src->sndfile;
}

void read_channel(SndfileHandle & sf, uint32_t channel_count, const uint32_t * channel_data,
                  uint32 frames, sample * data)
{
    const unsigned int frames_per_read = 1024;
    sized_array<sample> read_frame(sf.channels() * frames_per_read);

    if (channel_count == 1) {
        // fast-path for single-channel read
        for (size_t i = 0; i < frames; i += frames_per_read) {
            size_t read = sf.readf(read_frame.c_array(), frames_per_read);

            size_t channel_mapping = channel_data[0];
            for (size_t frame = 0; frame != read; ++frame) {
                data[0] = read_frame[frame * sf.channels() + channel_mapping];
                data += channel_count;
            }
        }
    } else {
        for (size_t i = 0; i < frames; i += frames_per_read) {
            size_t read = sf.readf(read_frame.c_array(), frames_per_read);

            for (size_t frame = 0; frame != read; ++frame) {
                for (size_t c = 0; c != channel_count; ++c) {
                    size_t channel_mapping = channel_data[c];
                    data[c] = read_frame[frame * sf.channels() + channel_mapping];
                }
                data += channel_count;
            }
        }
    }
}

} /* namespace */

int sc_plugin_interface::allocate_buffer(SndBuf * buf, uint32_t frames, uint32_t channels, double samplerate)
{
    const uint32_t samples = frames * channels;
    if (samples == 0)
        return kSCErr_Failed; /* invalid buffer size */

    sample * data = nova::allocate_buffer(samples);
    if (data == NULL)
        return kSCErr_Failed; /* could not allocate memory */

    buf->data = data;

    buf->channels = channels;
    buf->frames = frames;
    buf->samples = samples;
    buf->mask = bufmask(samples); /* for delay lines */
    buf->mask1 = buf->mask - 1;    /* for oscillators */
    buf->samplerate = samplerate;
    buf->sampledur = 1.0 / samplerate;
    return kSCErr_None;
}

SndBuf * sc_plugin_interface::allocate_buffer(uint32_t index, uint32_t frames, uint32_t channels)
{
    SndBuf * buf = World_GetNRTBuf(&world, index);
    allocate_buffer(buf, frames, channels, world.mFullRate.mSampleRate);
    return buf;
}

int sc_plugin_interface::buffer_read_alloc(uint32_t index, const char * filename, uint32_t start, uint32_t frames)
{
    SndfileHandle f(filename);
    if (!f)
        return -1; /* file cannot be opened */

    const size_t sf_frames = f.frames();

    if (start > sf_frames)
        start = sf_frames;

    if (frames == 0 || frames > sf_frames - start)
        frames = sf_frames - start;

    SndBuf * buf = World_GetNRTBuf(&world, index);
    allocate_buffer(buf, frames, f.channels(), f.samplerate());

    f.seek(start, SEEK_SET);
    f.readf(buf->data, frames);
    return 0;
}


int sc_plugin_interface::buffer_alloc_read_channels(uint32_t index, const char * filename, uint32_t start,
                                                    uint32_t frames, uint32_t channel_count,
                                                    const uint32_t * channel_data)
{
    SndfileHandle f(filename);
    if (!f)
        return -1; /* file cannot be opened */

    uint32_t sf_channels = uint32_t(f.channels());
    const uint32_t * max_chan = std::max_element(channel_data, channel_data + channel_count);
    if (*max_chan >= sf_channels)
        return -2;

    const size_t sf_frames = f.frames();

    if (start > sf_frames)
        start = sf_frames;

    if (frames == 0 || frames > sf_frames - start)
        frames = sf_frames - start;

    SndBuf * buf = World_GetNRTBuf(&world, index);
    allocate_buffer(buf, frames, channel_count, f.samplerate());

    f.seek(start, SEEK_SET);
    read_channel(f, channel_count, channel_data, frames, buf->data);

    return 0;
}

/* directly taken from supercollider sources
   Copyright (c) 2002 James McCartney. All rights reserved.
*/
int sampleFormatFromString(const char* name)
{
    if (!name) return SF_FORMAT_PCM_16;

    size_t len = strlen(name);
    if (len < 1) return 0;

    if (name[0] == 'u') {
        if (len < 5) return 0;
        if (name[4] == '8') return SF_FORMAT_PCM_U8; // uint8
            return 0;
    } else if (name[0] == 'i') {
        if (len < 4) return 0;
        if (name[3] == '8') return SF_FORMAT_PCM_S8;      // int8
            else if (name[3] == '1') return SF_FORMAT_PCM_16; // int16
                else if (name[3] == '2') return SF_FORMAT_PCM_24; // int24
                    else if (name[3] == '3') return SF_FORMAT_PCM_32; // int32
    } else if (name[0] == 'f') {
        return SF_FORMAT_FLOAT; // float
    } else if (name[0] == 'd') {
        return SF_FORMAT_DOUBLE; // double
    } else if (name[0] == 'm' || name[0] == 'u') {
        return SF_FORMAT_ULAW; // mulaw ulaw
    } else if (name[0] == 'a') {
        return SF_FORMAT_ALAW; // alaw
    }
    return 0;
}

int headerFormatFromString(const char *name)
{
    if (!name) return SF_FORMAT_AIFF;
    if (strcasecmp(name, "AIFF")==0) return SF_FORMAT_AIFF;
    if (strcasecmp(name, "AIFC")==0) return SF_FORMAT_AIFF;
    if (strcasecmp(name, "RIFF")==0) return SF_FORMAT_WAV;
    if (strcasecmp(name, "WAVEX")==0) return SF_FORMAT_WAVEX;
    if (strcasecmp(name, "WAVE")==0) return SF_FORMAT_WAV;
    if (strcasecmp(name, "WAV" )==0) return SF_FORMAT_WAV;
    if (strcasecmp(name, "Sun" )==0) return SF_FORMAT_AU;
    if (strcasecmp(name, "IRCAM")==0) return SF_FORMAT_IRCAM;
    if (strcasecmp(name, "NeXT")==0) return SF_FORMAT_AU;
    if (strcasecmp(name, "raw")==0) return SF_FORMAT_RAW;
    if (strcasecmp(name, "MAT4")==0) return SF_FORMAT_MAT4;
    if (strcasecmp(name, "MAT5")==0) return SF_FORMAT_MAT5;
    if (strcasecmp(name, "PAF")==0) return SF_FORMAT_PAF;
    if (strcasecmp(name, "SVX")==0) return SF_FORMAT_SVX;
    if (strcasecmp(name, "NIST")==0) return SF_FORMAT_NIST;
    if (strcasecmp(name, "VOC")==0) return SF_FORMAT_VOC;
    if (strcasecmp(name, "W64")==0) return SF_FORMAT_W64;
    if (strcasecmp(name, "PVF")==0) return SF_FORMAT_PVF;
    if (strcasecmp(name, "XI")==0) return SF_FORMAT_XI;
    if (strcasecmp(name, "HTK")==0) return SF_FORMAT_HTK;
    if (strcasecmp(name, "SDS")==0) return SF_FORMAT_SDS;
    if (strcasecmp(name, "AVR")==0) return SF_FORMAT_AVR;
    if (strcasecmp(name, "SD2")==0) return SF_FORMAT_SD2;
    if (strcasecmp(name, "FLAC")==0) return SF_FORMAT_FLAC;
    if (strcasecmp(name, "vorbis")==0) return SF_FORMAT_VORBIS;
    if (strcasecmp(name, "CAF")==0) return SF_FORMAT_CAF;
    return 0;
}


int sc_plugin_interface::buffer_write(uint32_t index, const char * filename, const char * header_format, const char * sample_format,
                                      uint32_t start, uint32_t frames, bool leave_open)
{
    SndBuf * buf = World_GetNRTBuf(&world, index);
    int format = headerFormatFromString(header_format) | sampleFormatFromString(sample_format);

    SndfileHandle sf(filename, SFM_WRITE, format, buf->channels, buf->samplerate);

    if (!sf)
        return -1;

    if (frames == 0xffffffff)
        frames = buf->frames;

    const uint32_t remain = uint32_t(buf->frames) > start ? buf->frames - start : 0;
    const uint32_t frames_to_write = std::min(remain, frames);

    if (frames_to_write)
        sf.writef(buf->data + start * buf->channels, frames_to_write);

    if (leave_open && !buf->sndfile)
        buf->sndfile = sf.takeOwnership();

    return 0;
}

static int buffer_read_verify(SndfileHandle const & sf, size_t min_length, size_t samplerate)
{
    if (!sf)
        return -1;
    if (sf.frames() < min_length)
        return -2; /* no more frames to read */
    if (sf.samplerate() != samplerate)
        return -3; /* sample rate mismatch */
    return 0;
}

int sc_plugin_interface::buffer_read(uint32_t index, const char * filename, uint32_t start_file, uint32_t frames,
                                     uint32_t start_buffer, bool leave_open)
{
    SndBuf * buf = World_GetNRTBuf(&world, index);

    if (uint32_t(buf->frames) < start_buffer)
        return -2; /* buffer already full */

    SndfileHandle sf(filename, SFM_READ);
    int error = buffer_read_verify(sf, start_file, buf->samplerate);
    if (error)
        return error;

    if (sf.channels() != buf->channels)
        return -3; /* sample rate or channel count mismatch */

    const uint32_t buffer_remain = buf->frames - start_buffer;
    const uint32_t file_remain = sf.frames() - start_file;
    const uint32_t frames_to_read = std::min(frames, std::min(buffer_remain, file_remain));

    sf.seek(start_file, SEEK_SET);
    sf.readf(buf->data + start_buffer*buf->channels, frames_to_read);

    if (leave_open)
        buf->sndfile = sf.takeOwnership();
    return 0;
}

int sc_plugin_interface::buffer_read_channel(uint32_t index, const char * filename, uint32_t start_file, uint32_t frames,
                                             uint32_t start_buffer, bool leave_open, uint32_t channel_count,
                                             const uint32_t * channel_data)
{
    SndBuf * buf = World_GetNRTBuf(&world, index);

    if (channel_count != uint32_t(buf->channels))
        return -2; /* channel count mismatch */

    if (uint32_t(buf->frames) >= start_buffer)
        return -2; /* buffer already full */

    SndfileHandle sf(filename, SFM_READ);
    int error = buffer_read_verify(sf, start_file, buf->samplerate);
    if (error)
        return error;

    uint32_t sf_channels = uint32_t(sf.channels());
    const uint32_t * max_chan = std::max_element(channel_data, channel_data + channel_count);
    if (*max_chan >= sf_channels)
        return -2;
    const uint32_t buffer_remain = buf->frames - start_buffer;
    const uint32_t file_remain = sf.frames() - start_file;

    const uint32_t frames_to_read = std::min(frames, std::min(buffer_remain, file_remain));

    sf.seek(start_file, SEEK_SET);
    read_channel(sf, channel_count, channel_data, frames, buf->data);

    if (leave_open)
        buf->sndfile = sf.takeOwnership();
    return 0;
}

void sc_plugin_interface::buffer_close(uint32_t index)
{
    SndBuf * buf = World_GetNRTBuf(&world, index);

    if (buf->sndfile == NULL)
        return;
    sf_close(buf->sndfile);
    buf->sndfile = NULL;
}


void sc_plugin_interface::buffer_zero(uint32_t index)
{
    SndBuf * buf = World_GetNRTBuf(&world, index);

    uint32_t length = buf->frames * buf->channels;

    uint32_t unrolled = length & ~63;
    uint32_t remain = length & 63;

    zerovec_simd(buf->data, unrolled);
    zerovec(buf->data + unrolled, remain);
}

sample * sc_plugin_interface::buffer_generate(uint32_t index, const char* cmd_name, struct sc_msg_iter & msg)
{
    SndBuf * buf = World_GetNRTBuf(&world, index);
    sample * data = buf->data;

    BufGenFunc bufgen = sc_factory->find_bufgen(cmd_name);
    (bufgen)(&world, buf, &msg);
    if (data == buf->data)
        return NULL;
    else
        return data;
}

void sc_plugin_interface::buffer_sync(uint32_t index)
{
    sndbuf_copy(world.mSndBufs + index, world.mSndBufsNonRealTimeMirror + index);
    increment_write_updates(index);
}

void sc_plugin_interface::free_buffer(uint32_t index)
{
    sndbuf_init(world.mSndBufsNonRealTimeMirror + index);
}

void sc_plugin_interface::initialize_synths_perform(void)
{
    for (std::size_t i = 0; i != uninitialized_synths.size(); ++i)
    {
        sc_synth * synth = static_cast<sc_synth*>(uninitialized_synths[i]);
        synth->prepare();
    }
    synths_to_initialize = false;
    uninitialized_synths.clear();
}

} /* namespace nova */
