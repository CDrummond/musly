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

#ifndef MUSLY_DECODERS_FFMPEG_H_
#define MUSLY_DECODERS_FFMPEG_H_

#include "decoder.h"

namespace musly {
namespace decoders {

class ffmpeg :
    public musly::decoder
{
    MUSLY_DECODER_REGCLASS(ffmpeg);

public:
    ffmpeg();

    virtual std::vector<float>
    decodeto_22050hz_mono_float(
            const std::string& file,
            float excerpt_length,
            float excerpt_start);
};

} /* namespace decoders */
} /* namespace musly */
#endif /* MUSLY_DECODERS_FFMPEG_H_ */
