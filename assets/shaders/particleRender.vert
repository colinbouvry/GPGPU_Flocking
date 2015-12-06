#version 420
#extension GL_ARB_shader_storage_buffer_object : require

layout( location = 0 ) in int particleId;
out vec3 vColor;

struct Particle
{
	vec3	pos;
	vec3	ppos;
	vec3	home;
	vec4	color;
	float	damping;
};

//Particle buffer
layout( std140, binding = 0 ) buffer Part
{
    Particle particles[];
};

//Incoming model view matrix
uniform mat4 ciModelViewProjection;


void main()
{
	// ModelViewMatrix * CurrentParticlePosition = 3-Space Projection
	gl_Position = ciModelViewProjection * vec4( particles[particleId].pos, 1 );
	vColor = particles[particleId].color.rgb; //Pass color to fragment.
}