## GPGPU_Flocking
![test](https://i.gyazo.com/32b8b3bfb0cb93186a7352105e4110e3.gif)

#Description
This README will serve doubly as a 'blog post' for my Artificial Intelligence class final research assignment, in which this project is associated.

##Introduction
<img align="right" src = "http://i.imgur.com/yErDZxJ.png">
Since implementing the classic [flocking behavior on the CPU](https://github.com/parsaiej/AISteeringBehaviors#flocking), I have become enamored in the idea of working a system that can easily handle not 100, not 1000, but upwards of hundreds of thousands of flocking boids. Moreover, I wanted to do all of this in 3-space as well. Anyone familiar with the flocking algorithm knows all too well that something like this is nigh impossible to achieve over today's average CPU, due to the algorithm's taxing computational demands.

To those unfamiliar with the behavior and why it is so demanding, allow me to explain. 

##Initial Research
