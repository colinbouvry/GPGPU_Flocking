## GPGPU_Flocking
![test](https://i.gyazo.com/32b8b3bfb0cb93186a7352105e4110e3.gif)

#Description
This README will serve doubly as a 'blog post' for my Artificial Intelligence class final research assignment, in which this project is associated.

##Introduction
<img align="right" src = "http://i.imgur.com/yErDZxJ.png">
Since implementing the classic [flocking behavior on the CPU](https://github.com/parsaiej/AISteeringBehaviors#flocking), I have become enamored in the idea of working a system that can easily handle not 100, not 1000, but upwards of hundreds of thousands of flocking boids. Moreover, I wanted to do all of this in 3-space as well. Anyone familiar with the flocking algorithm knows all too well that something like this is nigh impossible to achieve over today's average CPU, due to the algorithm's taxing computational demands.

To those unfamiliar with the behavior and why it is so demanding, allow me to explain. Achieving the flocking algorithm requires the synthesis of 3 base steering behaviors; separation, cohesion, and alignment. **Separation** is the steering responsible for avoiding other crowding local boids, **Cohesion** is responsible for drawing a boid *toward* the average location of other local boids, and **Alignment** is responsible for steering toward the average heading of other local boids.

<img align="left" src = "http://www.red3d.com/cwr/boids/images/separation.gif">
<img align="left" src = "http://www.red3d.com/cwr/boids/images/cohesion.gif">
<img align="left" src = "http://www.red3d.com/cwr/boids/images/alignment.gif">

Simple in concept, but not so efficient in practice. Performing this behavior comes in at **O(n^2)**. 

As you can imagine, things slow up very quickly for every new boid that enters the system. With this in mind, I knew that accomplishing my goal on the CPU was out of the question. Fortunately, previous inquires into parallel computing on the GPU left a door open to a plethora of posibilities to compute these flocking boids on a massive scale.

##Initial Research

It doesn't take many Google searches to realize on of the best people in the game for Flocking on the GPU is [Robert Hodgin](http://roberthodgin.com/), a creative coder in Brooklyn. Robert has developed a variety of projects that display flocking on the GPU, such as his "Murmuration" or "Boil Up".

<img align="left" src = "http://i.imgur.com/diTI9rE.jpg">
<img align="left" src = "http://i.imgur.com/3v2dbJL.jpg">


After listening to one of Robert's [talks at NVScene](http://www.ustream.tv/recorded/45396322#to00:24:54), I quickly came to understand that my boids would essentially need to be based within a particle system computed on the GPU, ideally through OpenGL and GLSL. This lead me on a search for some of the best ways to handle GPU based particle systems. In my search, I uncovered three main methodologies.

##FBO Ping-Pong

Before even trying to implement any of the flocking sub-behaviors, I wanted to make sure I could work with a particle system that could easily and effortlessly give me the ability to have any particle in question query every other particle in the system. I would perform a simple neighbor count lookup and alter the particle's RGB in respect to this neighbor count to visualize the proper proximity and particle lookups.

The first method I discovered [on the cinder forums](https://forum.libcinder.org/topic/on-my-way-towards-the-million) was the frame buffer object (FBO) ping-pong. This method worked with OpenGL's FBO, taking advantage of its ability to create user-defined framebuffers. Rather than interpreting texture pixel data as RGBA, this particle system used instead interpreted that data as XYZW, representing raw particle data. 

<img align="right" src = "http://i.imgur.com/AxfrnrW.jpg">

The best way to understand how an FBO works is from real-life example. To shamelessly quote Wikipedia: 

>"It is a nice sunny day and you have decided to get the paddling pool out, only you can not find your garden hose. You'll have to >fill the pool with buckets. So you fill one bucket (or buffer) from the tap, turn the tap off, walk over to the pool, pour the water >in, walk back to the tap to repeat the exercise. This is analogous to single buffering. The tap has to be turned off while you >"process" the bucket of water.
>
>Now consider how you would do it if you had two buckets. You would fill the first bucket and then swap the second in under the >running tap. You then have the length of time it takes for the second bucket to fill in order to empty the first into the paddling >pool. When you return you can simply swap the buckets so that the first is now filling again, during which time you can empty the >second into the pool. This can be repeated until the pool is full. It is clear to see that this technique will fill the pool far >faster as there is much less time spent waiting, doing nothing, while buckets fill. This is analogous to double buffering. The tap >can be on all the time and does not have to wait while the processing is done."

I quickly came to learn the downside of this system. As clever as it is, altering the pixel values on an incoming FBO in a fragment shader is not at all scalable. In a 1024 x 1024 FBO, one frame would equate to around 1 trillion texture lookups. Yikes. On top of this, calculating neighboring particles in general was extremely unsuccessful:
[![ScreenShot](http://i.imgur.com/Oy2u00V.png)](https://www.youtube.com/watch?v=QksTr9o-lcc)






What set off to be an AI study gradually turned into a graphics study buy the end of it
