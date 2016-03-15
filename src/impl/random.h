#ifndef RANDOM_H
#define RANDOM_H

// this class returns pseudo-random numbers in a given interval
// and needs to be initialized before first use with init(seed)
class Random
{
    public:
    Random() {
        srand((unsigned)42); // init seed
    }

    static void init(uint seed) {
        srand(seed); // init seed
    }

    // returns a (pseudo-)random float in the interval [a,b)
    static float getRandomFloat(float a, float b)
    {
        // make sure that b is maximum
        if(a > b){
            std::swap(a,b);
        }

        float scale=RAND_MAX+1.f;
        float randFloat = rand();

        // normalize to [0,1)
        float randNormal = randFloat/scale;

        //std::cout << randFloat << " / " << scale << " = " << randNormal << std::endl;

        // return the random float in given range
        return ((b-a)*(randNormal))+a;
    }
    protected:
    private:
};

#endif // RANDOM_H
