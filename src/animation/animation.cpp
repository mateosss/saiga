#include "saiga/animation/animation.h"
#include "saiga/util/assert.h"


int Animation::getActiveFrames()
{
    return skipLastFrame ? frameCount-1 : frameCount;
}

const AnimationFrame &Animation::getKeyFrame(int frameIndex)
{
    assert(frameIndex>=0 && frameIndex<frameCount);
    return keyFrames[frameIndex];
}


void Animation::getFrame(float time, AnimationFrame &out){

    //get frame index before and after
    int frame = floor(time);
    int nextFrame = frame+1;
    float t = time - frame;

    int modulo = getActiveFrames();

    frame = frame%modulo;
    nextFrame = nextFrame%modulo;


    getFrame(frame,nextFrame,t,out);

}

void Animation::getFrameNormalized(float time, AnimationFrame &out)
{
    getFrame(time*getActiveFrames(),out);
}

void Animation::getFrame(int frame0, int frame1, float alpha, AnimationFrame &out)
{
    assert(frame0>=0 && frame0<frameCount);
    assert(frame1>=0 && frame1<frameCount);

    AnimationFrame &k0 = keyFrames[frame0];
    AnimationFrame &k1 = keyFrames[frame1];
    AnimationFrame::interpolate(k0,k1,out,alpha);
}


