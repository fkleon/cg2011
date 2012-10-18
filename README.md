# MiniRT (Mini Ray Tracer)

A RayTracer developed within the scope of the lecture "Computer Graphics" in WS 2011/12 at Saarland University.
Please visit the [course website](http://graphics.cg.uni-saarland.de/courses/winter-term-201112/ws1112-cg1000/) for more information.

The ray tracer is based on a skeleton provided by the [Computer Graphics Chair](http://graphics.cg.uni-saarland.de) of the Saarland University.  
The website design is based on a free design by [Bryant Smith](http://bryantsmith.com).

### Collaborators

* Michael Frey
* Frederik Leonhardt

### Content

* Source Code
* Website Presentation
* World Model (see Download section)

### Implemented Topics

For more details on the implementation also have a look at the presentation on the project website provided in the repository.

* __Depth of Field__ (*Advanced Camera Properties*):  
Depth of field using a thin lens camera model.  
You can find the implementation in *impl/perspective_camera.h*.

* __Reflective and Refractive Transparency__ (*Surface Shading*):  
Using Snell's law we can calculate the refraction ray from an incoming ray and refraction index of the material, and Fresnel's term tells us how much light is being refracted and reflected. If the contribution of the reflection is smaller than 0,5%, only refractance is being considered. Also the case of total internal reflection is handled correctly. To simplify rendering, we assume that any material is surrounded by air at sea level (refraction index of 1,00029).  
You can find the implementation in *impl/phong_shaders.h*, *core/ray.h* and *impl/integrator.h*.

* __Procedural Shading__ (*Surface Shading*):  
Different noise functions can be used to generate a base for procedural 3d textures. A very convenient way is the Perlin Noise, described by Ken Perlin. We use his reference implementation in our project. This noise can be processed in many different ways, for example for a simple water texture we just use a stripe function, which is basically a sin-function on the perlin noise.  
You can find the implementation in *rt/texture.h* and *impl/phong_shaders.h*.

* __Bump Mapping__ (*Texturing*):  
Bump mapping is the pertubation of the normal vector based on a height map (bump texture). The rendered image looks geometrically more detailed.  
You can find the implementation in *impl/phong_shaders.h*, *rt/texture.h* and *impl/lwobject_reader.cpp*.

* __SAH construction of BVH__ (*Optimization Techniques*):  
sing the Surface Area Heuristic to determine the split position of a BVH node instead of splitting in the middle or at the median leads to a faster traversal, because ideally large and almost empty cells will be favoured by the algorithm.  
You can find the implementation in *rt/bvh.h*, *rt/bvhsah.cpp*, *rt/geometry_group.h*, *core/bbox.h* and *core/defs.h*.

* __SAH KD Tree__ (*Optimization Techniques*):  
A kD-tree allows faster traversal, because split axis and split position are saved in the nodes. Also the Surface Area Heuristic can be used with kD-trees. In contrast to a BVH in a kD-tree primitives can belong to several nodes, if the bounding box does overlap them.  
You can find the implementation in *rt/kdtree.h* and *rt/kdtree.cpp*.

* __Photon Mapping__ (*Advanced Topics*):  
A photon map is one way to implement global illumination into a raytracer. The Idea is to shoot a specified number of "Photons" into the Scene and save them in a map for later lookup during rendering. Because it is not possible to implement caustics (light that was refracted by a refractiv surface) in a raytracer via shadow rays (the possibility that the shadow rays will hit the light source is very low, because they will be refracted) you can approximate them by using a second photon map named "caustic map". Each photon that has intersected a refractive surface will be flagged and if it hits a diffuse surface later it won't be stored in the normal photon map but in the caustic map.  
You can find the implementation in *rt/myphotonmap.h*, *rt/myphotonmap.cpp*, *impl/photonMapping_Integrator.h* and *impl/phong_shaders.h*.

* __Tile Rendering__ (*misc*/*not listed*):  
The renderer accepts a tile size and will not render the picture in scanlines, but in tiles. We hoped to gain a little performance increase through caching, but it didn't impact the rendering time. But now we can easily save tiles and start rendering at a given tile (very useful for long renders).

* __Orthographic camera__ (*misc*/*not listed*):  
A camera implementation which shoots parallel rays for an orthographic image.

### Usage

1. Download source code
2. Download model files and extract in source code root
3. Compile ray tracer (use parameter *_NO_ACCELERATION=1* to disable acceleration structures)  
<pre><code>> make all</pre></code>
4. Execute ray tracer  
<pre><code>> ./minirt</pre></code>
The ray tracer will carry out the following steps:
    * Initialization (world, shaders, camera)
    * Build acceleration structure
    * Build photon map
    * Render image

Rendering the image with the given world model might take a while (6 hours and 8 minutes on a Intel Core i3-2310M CPU @ 2.10GHz). Console output provides further information about the current status of the rendering process.
