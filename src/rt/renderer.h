#ifndef __INCLUDE_GUARD_0FEF5793_A843_48DE_8050_1137AEAD3804
#define __INCLUDE_GUARD_0FEF5793_A843_48DE_8050_1137AEAD3804
#ifdef _MSC_VER
	#pragma once
#endif

#include "../core/image.h"
#include "basic_definitions.h"

//A sampler telling how to sample a pixel
struct Sampler : public RefCntBase
{
	//A sample is a poisition [0..1]x[0..1] within the pixel
	//	as well as a weight, telling how much this sample
	//	contributes to the final value of the pixel
	struct Sample
	{
		float2 position;
		float weight;
	};

	//Pushes all samples to _result
	virtual void getSamples(uint _x, uint _y, std::vector<Sample> &_result) = 0;
};

//A renderer class
class Renderer
{
public:
	SmartPtr<Sampler> sampler;
	SmartPtr<Camera> camera;
	SmartPtr<Integrator> integrator;
	SmartPtr<Image> target;

    // renders the whole image
    // line-wise
	void render()
	{
        const clock_t begin_time = clock(); // for building time measurement

		//Loop through all pixels in the scene and determine their color
		//	from the integrator
//#pragma omp parallel
		{
			std::vector<Sampler::Sample> samples;
            int progress;
//#pragma omp for schedule(dynamic, 10)
			for(int y = 0; y < (int)target->height(); y++) {
				for(int x = 0; x < (int)target->width(); x++)
				{
					float4 color = float4::rep(0.f);

					samples.clear();
					sampler->getSamples((uint)x, (uint)y, samples);

					//Accumulate the samples
					for(size_t i = 0; i < samples.size(); i++)
					{

					    std::vector<Ray> rays = camera->getPrimaryRays(samples[i].position.x + x, samples[i].position.y + y);

					    float4 tempColor = float4::rep(0.f);

					    for (size_t j = 0; j < rays.size(); j++)
					    {
					        tempColor += integrator->getRadiance(rays[j]);
                            //std::cout <<"Sent sample ray #" << i << ": "<< sampleRay.o[0] << "," <<sampleRay.o[1] << "," <<sampleRay.o[2]
                            //<< " to: " << sampleRay.d[0] << "," <<sampleRay.d[1] << "," << sampleRay.d[2]  << std::endl;
					        //std::cout << "sample " << j << ": " << tempColor[0] << "," << tempColor[1] << "," << tempColor[2] << "," << tempColor[3] << std::endl;
					    }

                        float rayWeight = 1.f/rays.size();
					    tempColor *= float4::rep(rayWeight);

                        //std::cout << "color: " << tempColor[0] << "," << tempColor[1] << "," << tempColor[2] << "," << tempColor[3] << std::endl;


						//Ray r = camera->getPrimaryRay(samples[i].position.x + x, samples[i].position.y + y);
                        //color += integrator->getRadiance(r) * float4::rep(samples[i].weight);

                        color += tempColor * float4::rep(samples[i].weight);
					}

					(*target)(x, y) = color;
				}

				progress = y*100 / (int)target->height() +0.5;
				std::cout << "Line: " << y << " Progress: " << progress << " % " << "\r";
			}
		}

        std::cout << "Time needed to render: " << float(clock()-begin_time)/CLOCKS_PER_SEC << " s."<< std::endl;
	}

    // will render the image in tiles
    // last Y- and X-tile will be cropped so that it fits, if necessary
    void render(int tileSize, int yStart)
    {
        if (tileSize<=0)
            render();

        const clock_t begin_time = clock(); // for building time measurement
        int progress; // # of tile being rendered right now

        int height = (int)target->height(); // image height
        int width = (int)target->width(); // image width

        int numYTiles = height / tileSize; // number of tiles in Y-direction
        int numXTiles = width / tileSize; // number of tiles in X_direction

        int lastYTile = height % tileSize; // do we need another small tile at the end?
        int lastXTile = width % tileSize; // same.

        int tileXSize = tileSize; // size of a x-tile
        int tileYSize = tileSize; // size of a y-tile

        // check if last tiles need to be cropped
        if (lastYTile > 0)
            numYTiles++;

        if (lastXTile > 0)
            numXTiles++;

        // iteratre through all tiles
        for (int yTile = yStart; yTile < numYTiles; yTile++)
        {
            // if we are in the last iteration,
            // check if we need to use another height
            if (yTile == numYTiles-1 && lastYTile > 0)
                tileYSize = lastYTile;
            else
                tileYSize = tileSize;

            for(int xTile = 0; xTile < numXTiles; xTile++)
            {
                // if we are in lsat iteration,
                // check if we need to use another width
                if (xTile == numXTiles-1 && lastXTile > 0)
                    tileXSize = lastXTile;
                else
                    tileXSize = tileSize;

                // render the tile
                renderTile(xTile*tileSize, xTile*tileSize+tileXSize,
                           yTile*tileSize, yTile*tileSize+tileYSize);
            }

            // progress information output
            progress = ((yTile)*100) / (numYTiles);
            std::cout << "Rendered Y-tile " << yTile << " of " << numYTiles << ". Progress: " << progress << " % " << std::endl;

            std::ostringstream osstream;
            osstream << yTile << ".png";
            target->writePNG(osstream.str());
        }

        // time information output
        std::cout << std::endl << "Time needed to render: " << float(clock()-begin_time)/CLOCKS_PER_SEC << " s."<< std::endl;
    }

private:

    // renders a given tile specified by xStart, xEnd
    // and yStart, yEnd
    void renderTile(int xStart, int xEnd, int yStart, int yEnd)
	{
		//Loop through all pixels in the tile and determine their color
		//	from the integrator
//#pragma omp parallel
		{
			std::vector<Sampler::Sample> samples;
//#pragma omp for schedule(dynamic, 10)
			for(int y = yStart; y < yEnd; y++) {
				for(int x = xStart; x < xEnd; x++)
				{
					float4 color = float4::rep(0.f);

                    // clear samples vector
					samples.clear();
					// get new samples from Sampler
					sampler->getSamples((uint)x, (uint)y, samples);

					//Accumulate the samples
					for(size_t i = 0; i < samples.size(); i++)
					{
					    // can be more than a ray, i.e. when using lens camera model
					    std::vector<Ray> rays = camera->getPrimaryRays(samples[i].position.x + x, samples[i].position.y + y);

					    float4 tempColor = float4::rep(0.f);

                        // go through all rays the camera gave us
					    for (size_t j = 0; j < rays.size(); j++)
					    {
					        tempColor += integrator->getRadiance(rays[j]);
                        }

                        // weight those rays averaged
                        float rayWeight = 1.f/rays.size();
					    tempColor *= float4::rep(rayWeight);

					    // add weighted color to overall color
                        color += tempColor * float4::rep(samples[i].weight);
					}

                    // write color to image
					(*target)(x, y) = color;
				}
			}
		}
	}
};

#endif //__INCLUDE_GUARD_0FEF5793_A843_48DE_8050_1137AEAD3804
