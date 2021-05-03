#pragma once
#include "scene.h"
#include "sceneParser.h"

class RayTracer : public Scene
{
public:
	
	RayTracer();
	void Init();
	void Update(const glm::mat4 &MVP,const glm::mat4 &Model,const int  shaderIndx);
	
	void WhenRotate();
	void WhenTranslate();
	void Motion();
	
	unsigned int TextureDesine(int width, int height);
	~RayTracer(void);
	inline void ResetCounter() { tmp = counter; counter = 0; }
	inline void SetCounter() { counter = tmp; }

	void UpdatePosition( float xpos, float ypos);
	void UploadFragmentData();
	void PanScreen(float xpos, float ypos);
	void TranslateSphere(float xpos, float ypos);
	void SetPanStart(float xstart, float ystart);
	void SetTranslateStart(float xstart, float ystart);
	bool IsPanning() { return isPanningScreen; }
	bool IsTranslating() { return isTranslatingSphere; }
	void TogglePanning();
	void ToggleTranslating();
	void zoomin();
	void zoomout();
	int isHitSphere(int x, int y);
	SceneData* getSceneData() { return &data; }
private:
	SceneData data;
	unsigned int counter;
	unsigned int tmp;
	bool isPanningScreen = false;
	bool isTranslatingSphere = false;
	float x, y, xoffset = 0, yoffset = 0;
	float xold, yold, sphere_xold, sphere_yold;
	float zoom = 0;
	int sphereId = -1;
};

