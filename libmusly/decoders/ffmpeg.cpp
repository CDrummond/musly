/**
 * Copyright 2021, Craig Drummond <craig.p.drummond@gmail.com>
 *
 * This file is part of Musly, a program for high performance music
 * similarity computation: http://www.musly.org/.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#define __STDC_CONSTANT_MACROS
#define __STDC_FORMAT_MACROS

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "ffmpeg.h"
#include "minilog.h"

namespace musly {
namespace decoders {

MUSLY_DECODER_REGIMPL(ffmpeg, 1);

static const size_t constMaxLineLen = 180;
static const std::string constDuration("Duration: ");
static const size_t constReadBuffer = 50000;

static int get_duration(const std::string &path) {
    int length = -1;
    std::string cmd = "ffprobe -hide_banner \"" + path + "\" 2>&1";
    auto pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        MINILOG(logDEBUG) << "Failed to start ffprobe";
        return -1;
    }

    MINILOG(logDEBUG) << "Running command:" << cmd;
    char line[constMaxLineLen+1];
    while (!feof(pipe)) {
        if (fgets(line, constMaxLineLen, pipe) != nullptr) {
            line[constMaxLineLen]='\0';
            size_t len = strlen(line);
            char *dur = strstr(line, constDuration.c_str());
            if (nullptr == dur || ((dur+constDuration.size())-line)>=(long)(len-20)) {
                continue;
            }
            dur += constDuration.size();
            char *start = strstr(dur, "start:");
            if (nullptr == start) {
                continue;
            }
            char *comma = strstr(dur, ",");
            if (nullptr == comma || comma > start) {
                continue;
            }
            *comma = '\0';
            
            int hrs, mins;
            float secs;
            if (3==sscanf(dur, "%d:%d:%f", &hrs, &mins, &secs)) {
                length = (hrs * 60 * 60) + (mins * 60) + (int)(secs);
            }
            break;
        }
    };
    pclose(pipe);
    MINILOG(logDEBUG) << "File duration:" << length;
    return length;
}

static std::string format_time(float time) {
    char str[34];
    int t = (int)time;
    int h = t /3600;
    time = t %3600;
    int m = t /60;
    time = t %60;
    int s = (int)time; ;
    sprintf(str, "%02d:%02d:%02d.00", h, m, s);
    return std::string(str);
}


ffmpeg::ffmpeg() {
}

std::vector<float> ffmpeg::decodeto_22050hz_mono_float(
        const std::string& file,
        float excerpt_length,
        float excerpt_start) {
    MINILOG(logDEBUG) << "Decoding: " << file << " started.";

    std::vector<float> pcm;
    int duration = get_duration(file);
    if (duration<10) {
        return pcm;
    }

    // adjust excerpt boundaries
    if ((excerpt_length <= 0) || (excerpt_length > duration)) { // use full file
        excerpt_length = 0;
        excerpt_start = 0;
    } else if (excerpt_start < 0) { // center in file, but start at -excerpt_start the latest
        excerpt_start = std::min(-excerpt_start, (duration - excerpt_length) / 2);
    } else if (excerpt_start + excerpt_length > duration) { // right-align excerpt
        excerpt_start = duration - excerpt_length;
    }

    std::string cmd = "ffmpeg -hide_banner -loglevel panic";
    if (excerpt_length>0) {
        std::string start = format_time(excerpt_start);
        std::string end = format_time(excerpt_length);
        cmd+=" -ss " + start + " -t " + end;
    }
    cmd += " -i \"" + file + "\" -f f32le -ar 22050 -ac 1 pipe:1";
    MINILOG(logDEBUG) << "Running command:" << cmd;
    auto pipe = popen(cmd.c_str(), "rb");
    if (!pipe) {
        MINILOG(logDEBUG) << "Failed to start ffmpeg";
        return pcm;
    }

    float buffer[constReadBuffer];
    while (!feof(pipe)) {
        int numRead = fread(buffer, sizeof(float), constReadBuffer, pipe);
        if (numRead>0) {
            pcm.insert(pcm.end(), buffer, buffer+numRead);
        }
    }
    pclose(pipe);

    MINILOG(logDEBUG) << "Decoding: " << file << " finalized, samples:" << pcm.size();
    return pcm;
}

} /* namespace decoders */
} /* namespace musly */
