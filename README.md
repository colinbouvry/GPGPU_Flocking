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

It doesn't take many Google searches to realize on of the best people in the game for Flocking on the GPU is [Robert Hodgin](http://roberthodgin.com/), a creative coder in Brooklyn. Robert has developed a variety of projects that display flocking on the GPU, such as his "Murmuration" or "Boil Up" 

<img align="left" src = "http://i.imgur.com/diTI9rE.jpg">




What set off to be an AI study gradually turned into a graphics study buy the end of it
