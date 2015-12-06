#version 420

in vec3 vColor; //Incoming particle color
out vec4 outColor;

void main()
{
	//Set Particle color.
    outColor = vec4(vColor, 1.0);
}