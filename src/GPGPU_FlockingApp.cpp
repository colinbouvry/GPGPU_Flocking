#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Ssbo.h"
#include "cinder/params/Params.h"
#include "cinder/CameraUi.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include <cstdio>


using namespace ci;
using namespace ci::app;
using namespace std;

/*Particle holds position, previous position, color, and damping data.
  Since the type of ssbo being used is std140, we must have members of 
  struct remain on a 4byte alignment*/
#pragma pack( push, 1 )
struct Particle
{
	vec3	pos;
	float   pad1;
	vec3	ppos;
	float   pad2;
	vec4	pad3;//Extra padding to maintain struct alignment.
	vec4    color;
	float	damping;
	vec3    pad4;
};
#pragma pack(pop)

//Number of particles to create.
//const int NUM_PARTICLES = static_cast<int>(60e3);

class GPGPU_FlockingApp : public App {
  public:
	void setup() override;
	void keyDown(KeyEvent event) override;
	void update() override;
	void draw() override;
private:
	enum { WORK_GROUP_SIZE = 128, };

	/*### SHADERS ###*/
	gl::GlslProgRef mRenderProg;
	gl::GlslProgRef mUpdateProg;
	/*###############*/

	/*### PARTICLE DATA BUFFERS ###*/
	gl::SsboRef mParticleBuffer;
	gl::VboRef mIdsVbo;
	gl::VaoRef mAttributes;
	/*#############################*/

	/*### PARAMS ###*/
	params::InterfaceGlRef mParams;
	float mSeparationRadius, mCohesionRadius, mAlignRadius;
	float mSeparationStrength, mCohesionStrength, mAlignStrength;
	float mTimeStep, mBoidSpeed, mColorRadius, mParticleSize;
	bool mStep;
	/*##############*/

	/*### CAMERA ###*/
	CameraPersp	mCam;
	CameraUi mCamUi;
	/*##############*/

	int NUM_PARTICLES;
};

void GPGPU_FlockingApp::setup()
{
	if (getCommandLineArgs().size() > 1)
		NUM_PARTICLES = std::atoi(getCommandLineArgs().at(1).c_str());
	else
		NUM_PARTICLES = 60000;

	// CREATE TEMPORARY 
	vector<Particle> particles;
	particles.assign(NUM_PARTICLES, Particle());
	const float azimuth = 256.0f * static_cast<float>(M_PI) / particles.size();
	const float inclination = static_cast<float>(M_PI) / particles.size();
	const float radius = 180.0f;
	vec3 center = vec3(getWindowCenter() + vec2(0.0f, 40.0f), 0.0f);
	for (unsigned int i = 0; i < particles.size(); ++i)
	{	// assign starting values to particles.
		float x = radius * math<float>::sin(inclination * i) * math<float>::cos(azimuth * i);
		float y = radius * math<float>::cos(inclination * i);
		float z = radius * math<float>::sin(inclination * i) * math<float>::sin(azimuth * i);

		auto &p = particles.at(i);
		p.pos = center + vec3(x, y, z);
		p.ppos = p.pos + Rand::randVec3() * 10.0f; // random initial velocity
		p.damping = Rand::randFloat(0.965f, 0.985f);
		p.color = vec4(0.2, 0.2, 0.2, 1.0f);
	}

	//
	ivec3 count = gl::getMaxComputeWorkGroupCount();
	CI_ASSERT(count.x >= (NUM_PARTICLES / WORK_GROUP_SIZE));

	// Create particle buffers on GPU and copy data into the first buffer.
	// Mark as static since we only write from the CPU once.
	mParticleBuffer = gl::Ssbo::create(particles.size() * sizeof(Particle), particles.data(), GL_STATIC_DRAW);
	gl::ScopedBuffer scopedParticleSsbo(mParticleBuffer);
	mParticleBuffer->bindBase(0);

	/*### CREATE COLOR PROGRAM */
	try {
		//Load vert and frag pass-thru shaders.
		mRenderProg = gl::GlslProg::create(gl::GlslProg::Format().vertex(loadFile("shaders/particleRender.vert")) //change loadFile() to loadAsset() if running debugger.
			.fragment(loadFile("shaders/particleRender.frag"))
															     .attribLocation("particleId", 0));
	}
	catch (gl::GlslProgCompileExc e) {
		//Catch any shader compilation errors and throw to console.
		ci::app::console() << e.what() << std::endl;
	}
	/*##################################*/

	std::vector<GLuint> ids(NUM_PARTICLES);
	GLuint currId = 0;
	std::generate(ids.begin(), ids.end(), [&currId]() -> GLuint { return currId++; });

	mIdsVbo = gl::Vbo::create<GLuint>(GL_ARRAY_BUFFER, ids, GL_STATIC_DRAW);
	mAttributes = gl::Vao::create();
	gl::ScopedVao vao(mAttributes);
	gl::ScopedBuffer scopedIds(mIdsVbo);
	gl::enableVertexAttribArray(0);
	gl::vertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(GLuint), 0);

	/*### CREATE UPDATE PROGRAM ###*/
	try {
		//Load our compute shader.
		mUpdateProg = gl::GlslProg::create(gl::GlslProg::Format().compute(loadFile("shaders/particleUpdate.comp")));
	}
	catch (gl::GlslProgCompileExc e) {
		//Catch any shader compilation errors and throw to console.
		ci::app::console() << e.what() << std::endl;
	}
	/*#############################*/

	/*### ASSIGN DEFAULT PARAMS ###*/
	mSeparationRadius = 3.0;
	mCohesionRadius = 322.0;
	mAlignRadius = 250.0;
	mSeparationStrength = 0.24;
	mCohesionStrength = 0.08;
	mAlignStrength = 0.30;
	mTimeStep = 1.0;
	mBoidSpeed = 1.0;//0.5;
	mColorRadius = 2.8;
	mParticleSize = 1.0;
	mStep = false;
	/*##############################*/

	/*### CREATE PARAMS -> ADD VARIABLES ###*/
	mParams = params::InterfaceGl::create("Params", vec2(200, 250));
	mParams->addParam("Separation Radius", &mSeparationRadius, "min=0.1 max=2000.0 step=0.1");
	mParams->addParam("Cohesion Radius", &mCohesionRadius, "min=0.1 max=2000.0 step=0.1");
	mParams->addParam("Align Radius", &mAlignRadius, "min=0.1 max=2000.0 step=0.1");
	mParams->addSeparator();
	mParams->addParam("Separation Strength", &mSeparationStrength, "min=0.01 max=5.0 step=0.01");
	mParams->addParam("Cohesion Strength", &mCohesionStrength, "min=0.01 max=5.0 step=0.01");
	mParams->addParam("Align Strength", &mAlignStrength, "min=0.01 max=5.0 step=0.01");
	mParams->addSeparator();
	mParams->addParam("Time Step", &mTimeStep, "min=0.01 max=1.0 step=0.05");
	mParams->addParam("Boid Max Speed", &mBoidSpeed, "min=0.01 max=10.0 step=0.05");
	mParams->addParam("Color Radius", &mColorRadius, "min=0.01 max=10.0 step=0.01");
	mParams->addParam("Boid Size", &mParticleSize, "min=1.0 max=5.0 step=0.1");
	mParams->minimize();
	/*######################################*/
	
	/*### CREATE CAMERA UI ###*/
	mCam = CameraPersp(getWindowWidth(), getWindowHeight(), 65.0f, 0.1f, 6000.0f);
	mCam.lookAt(vec3(getWindowCenter(), -600.f), vec3(getWindowCenter(), 0));
	mCamUi = CameraUi(&mCam, getWindow());
	/*########################*/
}

void GPGPU_FlockingApp::update()
{
	if (!mStep)
		return;

	/*### UPDATE GPU PARTICLES ###*/
	gl::ScopedGlslProg prog(mUpdateProg);

	//---INJECT CURRENT PARAMS TO COMPUTE SHADER UNIFORMS
	mUpdateProg->uniform("uSeparationRadius", mSeparationRadius);
	mUpdateProg->uniform("uCohesionRadius", mCohesionRadius);
	mUpdateProg->uniform("uAlignRadius", mAlignRadius);
	mUpdateProg->uniform("uSeparationStrength", mSeparationStrength);
	mUpdateProg->uniform("uCohesionStrength", mCohesionStrength);
	mUpdateProg->uniform("uAlignStrength", mAlignStrength);
	mUpdateProg->uniform("uTimeStep", mTimeStep);
	mUpdateProg->uniform("uBoidSpeed", mBoidSpeed);
	mUpdateProg->uniform("uWindowCenter", getWindowCenter());
	mUpdateProg->uniform("uColorRadius", mColorRadius);

	gl::ScopedBuffer scopedParticleSsbo(mParticleBuffer);
	gl::dispatchCompute(NUM_PARTICLES / WORK_GROUP_SIZE, 1, 1); //Only dispatching in the X dimension
	gl::memoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void GPGPU_FlockingApp::draw()
{
	/*### BOILER PLATE DRAW INIT ###*/
	gl::clear(Color(0.0, 0.0, 0.0));
	gl::setMatrices(mCam);
	gl::enableDepthRead();
	gl::enableDepthWrite();
	/*##############################*/

	//DRAW SOME AXES
	glLineWidth(2.0);
	gl::drawLine(vec3(-500.0 + getWindowCenter().x, getWindowCenter().y, 0.0), vec3(500.0 + getWindowCenter().x, getWindowCenter().y, 0.0));
	gl::drawLine(vec3(getWindowCenter().x, getWindowCenter().y - 500.0, 0.0), vec3(getWindowCenter().x, getWindowCenter().y + 500.0, 0.0));
	gl::drawLine(vec3(getWindowCenter().x, getWindowCenter().y, -500.0), vec3(getWindowCenter().x, getWindowCenter().y, 500.0));

	//PREPARE SHADER
	gl::ScopedGlslProg render(mRenderProg);
	gl::ScopedBuffer scopedParticleSsbo(mParticleBuffer);
	gl::ScopedVao vao(mAttributes);
	gl::context()->setDefaultShaderVars();

	//ACTUALLY DRAW PARTICLES
	glPointSize(mParticleSize);
	gl::drawArrays(GL_POINTS, 0, NUM_PARTICLES);

	/*### DRAW MISCELLANEOUS ###*/
	mParams->draw();
	gl::setMatricesWindow(app::getWindowSize());
	gl::drawString( toString( static_cast<int>( getAverageFps() ) ) + " fps", vec2( 32.0f, 52.0f ), ColorA(1.0, 0.0, 0.0, 1.0), Font().getDefault() );
	gl::drawString(toString(static_cast<int>(NUM_PARTICLES)) + " Boids.", vec2(32.0f, 72.0f) ,ColorA(1.0, 0.0, 0.0, 1.0), Font().getDefault());
	gl::drawString("John Parsaie", vec2(32.0f, getWindowHeight() -20.0f), ColorA(1.0, 0.0, 0.0, 1.0), Font().getDefault());
	/*##########################*/
}

CINDER_APP(GPGPU_FlockingApp, RendererGl, [](App::Settings *settings) {
	settings->setWindowSize(1920, 1080);
	settings->setMultiTouchEnabled(false);
	settings->disableFrameRate();
	settings->setBorderless(true);//settings->setFullScreen(true);
	//settings->setConsoleWindowEnabled(true);
})

void GPGPU_FlockingApp::keyDown(KeyEvent event)
{
	//RESET DEFAULT PARAMS
	if (event.getCode() == KeyEvent::KEY_r)
	{
		mSeparationRadius = 3.0;
		mCohesionRadius = 322.0;
		mAlignRadius = 250.0;
		mSeparationStrength = 0.24;
		mCohesionStrength = 0.08;
		mAlignStrength = 0.30;
		mTimeStep = 1.0;
		mBoidSpeed = 1.0;//0.5;
		mColorRadius = 2.8;
		mParticleSize = 1.0;
	}

	//PAUSE
	if (event.getCode() == KeyEvent::KEY_SPACE)
	{
		mStep = !mStep;
	}
}