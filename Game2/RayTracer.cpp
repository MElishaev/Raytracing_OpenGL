#include "RayTracer.h"
#include <iostream>
#include <math.h>
#include <vector>
#include "GL/glew.h"

static void printMat(const glm::mat4 mat)
{
	std::cout<<" matrix:"<<std::endl;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
			std::cout<< mat[j][i]<<" ";
		std::cout<<std::endl;
	}
}

RayTracer::RayTracer() : Scene()
{
	counter = 1;
}

void RayTracer::Init()
{
	// takes scene from the res folder in build folder
	SceneParser::SceneParser("../res/scenes/scene1.txt", &data);
	std::cout <<	"# objs: " << data.sizes.x <<
					"\n# lights: " << data.sizes.y <<
					"\n# mirrors: " << data.sizes.z <<
					"\nheight: " << data.sizes.w << std::endl;
	std::cout << "Loading shaders\n";
	AddShader("../res/shaders/pickingShader");
	AddShader("../res/shaders/myShader");
	
	AddShape(Plane, -1, TRIANGLES);
	SetShapeShader(0, 1);

	UploadFragmentData();
}

void RayTracer::Update(const glm::mat4 &MVP,const glm::mat4 &Model,const int  shaderIndx)
{	
	if(counter)
		counter++;
	Shader *s = shaders[shaderIndx];
	int r = ((pickedShape+1) & 0x000000FF) >>  0;
	int g = ((pickedShape+1) & 0x0000FF00) >>  8;
	int b = ((pickedShape+1) & 0x00FF0000) >> 16;
	if (shapes[pickedShape]->GetMaterial() >= 0 && !materials.empty())
		BindMaterial(s, shapes[pickedShape]->GetMaterial());
	//textures[0]->Bind(0);
	s->Bind();
	if (shaderIndx != 1)
	{
		s->SetUniformMat4f("MVP", MVP);
		s->SetUniformMat4f("Normal", Model);
	}
	else
	{
		s->SetUniformMat4f("MVP", glm::mat4(1));
		s->SetUniformMat4f("Normal", glm::mat4(1));
	}
	s->Unbind();
}

void RayTracer::UpdatePosition(float xpos,  float ypos)
{
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	x = xpos / viewport[2];
	y = 1- ypos / viewport[3];
}

void RayTracer::UploadFragmentData()
{
	Shader* s = shaders[1];
	s->Bind();
	s->SetUniform4f("eye", data.eye.x, data.eye.y, data.eye.z, data.eye.w); //position + shine
	s->SetUniform4f("ambient", data.ambient.x, data.ambient.y, data.ambient.z, data.ambient.w);
	s->SetUniform4i("sizes", data.sizes.x, data.sizes.y, data.sizes.z, data.sizes.w); // number of (objects, light) width, height
	s->SetUniform4fv("objects", &data.objects[0], data.sizes.x); //center coordinates + radius / normal + d
	s->SetUniform4fv("objColors", &data.colors[0], data.sizes.x);
	s->SetUniform4fv("lightsDirection", &data.directions[0], data.sizes.y); //direction +  is directional 0.0/1.0
	s->SetUniform4fv("lightsIntensity", &data.intensities[0], data.sizes.y); //light intensity
	s->SetUniform4fv("lightPosition", &data.lights[0], data.sizes.y); //position + cos(angle)
	s->Unbind();
}

void RayTracer::PanScreen(float xpos, float ypos)
{
	Shader* s = shaders[1];
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	xpos = xpos / viewport[2];
	ypos = 1 - ypos / viewport[3];
	float dx = xpos - xold;
	float dy = ypos - yold;
	xold = xpos;
	yold = ypos;
	xoffset += dx;
	yoffset += dy;
	data.eye.x += dx;
	data.eye.y += dy;
	// update the shader uniforms with panned offset
	s->Bind();
	s->SetUniform1f("xoff", xoffset);
	s->SetUniform1f("yoff", yoffset);
	s->SetUniform4f("eye", data.eye.x, data.eye.y, data.eye.z, data.eye.w);
	s->Unbind();
}

void RayTracer::TranslateSphere(float xpos, float ypos)
{
	Shader* s = shaders[1];
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	xpos = glm::mix(-1.f, 1.f, xpos / viewport[2]);
	ypos = glm::mix(-1.f, 1.f, 1 - ypos / viewport[3]);
	float dx = xpos - sphere_xold;
	float dy = ypos - sphere_yold;
	// based on similarity of triangles, translate the movement on screen to movement in scene
	glm::vec3 oldSpherePosition = glm::vec3(data.objects[sphereId].x, data.objects[sphereId].y, data.objects[sphereId].z);
	float a = glm::distance(glm::vec3(data.eye.x, data.eye.y, data.eye.z), glm::vec3(sphere_xold, sphere_yold, zoom));
	float b = glm::distance(glm::vec3(data.eye.x, data.eye.y, data.eye.z), oldSpherePosition);
	float dxInScene = dx * b / a;
	float dyInScene = dy * b / a;

	sphere_xold = xpos;
	sphere_yold = ypos;
	data.objects[sphereId].x += dxInScene;
	data.objects[sphereId].y += dyInScene;
	// update the shader uniforms with panned offset
	s->Bind();
	s->SetUniform4fv("objects", &data.objects[0], data.sizes.x);
	s->Unbind();
}

void RayTracer::SetPanStart(float xstart, float ystart)
{
	TogglePanning();
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	xold = xstart / viewport[2];
	yold = 1 - ystart / viewport[3];
}

void RayTracer::SetTranslateStart(float xstart, float ystart)
{
	ToggleTranslating();
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	sphere_xold = glm::mix(-1.f, 1.f, xstart / viewport[2]);
	sphere_yold = glm::mix(-1.f, 1.f, 1 - ystart / viewport[3]);
}

void RayTracer::TogglePanning()
{
	isPanningScreen = !isPanningScreen;
}

void RayTracer::ToggleTranslating()
{
	isTranslatingSphere = !isTranslatingSphere;
}

void RayTracer::zoomin()
{
	Shader *s = shaders[1];
	zoom -= 0.1;
	data.eye.z -= 0.1;
	s->Bind();
	s->SetUniform1f("zoom", zoom);
	s->SetUniform4f("eye", data.eye.x, data.eye.y, data.eye.z, data.eye.w);
	s->Unbind();
}

void RayTracer::zoomout()
{
	Shader *s = shaders[1];
	zoom += 0.1;
	data.eye.z += 0.1;
	s->Bind();
	s->SetUniform1f("zoom", zoom);
	s->SetUniform4f("eye", data.eye.x, data.eye.y, data.eye.z, data.eye.w);
	s->Unbind();
}

int RayTracer::isHitSphere(int x, int y)
{
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	float scaled_x = float(x) / viewport[2];
	float scaled_y = 1 - float(y) / viewport[3];
	glm::vec2 screenCoords = glm::mix(glm::vec2(-1, -1), glm::vec2(1, 1), glm::vec2(scaled_x, scaled_y));
	std::cout << screenCoords.x << " " << screenCoords.y << std::endl;
	glm::vec3 eyePosition = glm::vec3(data.eye.x, data.eye.y, data.eye.z);
	glm::vec3 InSceneScreenSpace = glm::vec3(screenCoords + glm::vec2(xoffset, yoffset), zoom);
	glm::vec3 rayThroughScreen = InSceneScreenSpace - eyePosition;
	// check if ray intersects sphere in scene - use the same function as in shader
	for (int i = 0; i < data.sizes.x; i++)
	{
		float r = data.objects[i].w;
		if (r > 0)
		{
			glm::vec3 objPos = glm::vec3(data.objects[i].x, data.objects[i].y, data.objects[i].z);
			glm::vec3 L = glm::vec3(data.objects[i].x,data.objects[i].y,data.objects[i].z) - eyePosition;
			float tm = glm::dot(L, glm::normalize(rayThroughScreen));
			float d2 = pow(glm::distance(objPos,eyePosition),2) - pow(tm, 2);
			if(d2 <= pow(r,2))
			{
				std::cout << "intersects with sphere\n";
				sphereId = i;
				return i;
			}
		}
	}
	sphereId = -1;
	return -1;
}

void RayTracer::WhenRotate()
{
	std::cout << "x "<<x<<", y "<<y<<std::endl;
}

void RayTracer::WhenTranslate()
{
}

void RayTracer::Motion()
{
	if(isActive)
	{
	}
}

unsigned int RayTracer::TextureDesine(int width, int height)
{
	unsigned char* data = new unsigned char[width * height * 4];
	for (size_t i = 0; i < width; i++)
	{
		for (size_t j = 0; j < height; j++)
		{
			data[(i * height + j) * 4] = (i + j) % 256;
			data[(i * height + j) * 4 + 1] = (i + j * 2) % 256;
			data[(i * height + j) * 4 + 2] = (i * 2 + j) % 256;
			data[(i * height + j) * 4 + 3] = (i * 3 + j) % 256;
		}
	}
	textures.push_back(new Texture(width, height));
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data); //note GL_RED internal format, to save 
	glBindTexture(GL_TEXTURE_2D, 0);
	delete[] data;
	return(textures.size() - 1);
}

RayTracer::~RayTracer(void)
{

}
