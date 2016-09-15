/*
    Copyright 2005-2016 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks. Threading Building Blocks is free software;
    you can redistribute it and/or modify it under the terms of the GNU General Public License
    version 2  as  published  by  the  Free Software Foundation.  Threading Building Blocks is
    distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See  the GNU General Public License for more details.   You should have received a copy of
    the  GNU General Public License along with Threading Building Blocks; if not, write to the
    Free Software Foundation, Inc.,  51 Franklin St,  Fifth Floor,  Boston,  MA 02110-1301 USA

    As a special exception,  you may use this file  as part of a free software library without
    restriction.  Specifically,  if other files instantiate templates  or use macros or inline
    functions from this file, or you compile this file and link it with other files to produce
    an executable,  this file does not by itself cause the resulting executable to be covered
    by the GNU General Public License. This exception does not however invalidate any other
    reasons why the executable file might be covered by the GNU General Public License.
*/

#ifndef UNIVERSE_H_
#define UNIVERSE_H_

#ifndef UNIVERSE_WIDTH
#define UNIVERSE_WIDTH 1024
#endif
#ifndef UNIVERSE_HEIGHT
#define UNIVERSE_HEIGHT 512
#endif

#include "../../common/gui/video.h"
#include "tbb/partitioner.h"

class Universe {
public:
    enum {
        UniverseWidth  = UNIVERSE_WIDTH,
        UniverseHeight = UNIVERSE_HEIGHT
    };
private:
    //in order to avoid performance degradation due to cache aliasing issue
    //some padding is needed after each row in array, and between array themselves.
    //the padding is achieved by adjusting number of rows and columns.
    //as the compiler is forced to place class members of the same clause in order of the
    //declaration this seems to be the right way of padding.

    //magic constants added below are chosen experimentally for 1024x512.
    enum {
        MaxWidth = UniverseWidth+1,
        MaxHeight = UniverseHeight+3
    };

    typedef float ValueType;

    //! Horizontal stress
    ValueType S[MaxHeight][MaxWidth];

    //! Velocity at each grid point
    ValueType V[MaxHeight][MaxWidth];

    //! Vertical stress
    ValueType T[MaxHeight][MaxWidth];

    //! Coefficient related to modulus
    ValueType M[MaxHeight][MaxWidth];

    //! Damping coefficients
    ValueType D[MaxHeight][MaxWidth];

    //! Coefficient related to lightness
    ValueType L[MaxHeight][MaxWidth];

    enum { ColorMapSize = 1024};
    color_t ColorMap[4][ColorMapSize];

    enum MaterialType {
        WATER=0,
        SANDSTONE=1,
        SHALE=2
    };

    //! Values are MaterialType, cast to an unsigned char to save space.
    unsigned char material[MaxHeight][MaxWidth];

private:
    enum { DamperSize = 32};

    int pulseTime;
    int pulseCounter;
    int pulseX;
    int pulseY;

    drawing_memory drawingMemory;

public:
    void InitializeUniverse(video const& colorizer);

    void SerialUpdateUniverse();
    void ParallelUpdateUniverse();
    bool TryPutNewPulseSource(int x, int y);
    void SetDrawingMemory(const drawing_memory &dmem);
private:
    struct Rectangle;
    void UpdatePulse();
    void UpdateStress(Rectangle const& r );

    void SerialUpdateStress() ;
    friend struct UpdateStressBody;
    friend struct UpdateVelocityBody;
    void ParallelUpdateStress(tbb::affinity_partitioner &affinity);

    void UpdateVelocity(Rectangle const& r);

    void SerialUpdateVelocity() ;
    void ParallelUpdateVelocity(tbb::affinity_partitioner &affinity);
};

#endif /* UNIVERSE_H_ */
