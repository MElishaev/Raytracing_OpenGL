#version 130 

uniform vec4 eye;
uniform vec4 ambient;
uniform vec4[20] objects;
uniform vec4[20] objColors;
uniform vec4[10] lightsDirection;
uniform vec4[10] lightsIntensity;
uniform vec4[10] lightPosition;
uniform ivec4 sizes; 

uniform float zoom = 0;
uniform float xoff = 0;
uniform float yoff = 0;

in vec3 position1;

struct Intersection
{
	int id;
	vec3 point;
};

struct Ray
{
	vec3 source;
	vec3 dir;
	int sourceId;
};

bool isEqualf(float a, float b)
{
	const float epsilon = 0.001;
	return abs(a-b) < epsilon;
}

bool isEqualv3(vec3 a, vec3 b)
{
	return isEqualf(a.x,b.x) && isEqualf(a.y,b.y) && isEqualf(a.z,b.z);
}

bool isEyeFacingPlane(int planeId)
{
	float d = objects[planeId].w;
	vec3 N = objects[planeId].xyz;;
	vec3 pointOnPlane;

	if(objects[planeId].x != 0)
		pointOnPlane = vec3(objects[planeId].x/-d, 0, 0);
	else if(objects[planeId].y != 0)
		pointOnPlane = vec3(0, objects[planeId].y/-d, 0);
	else
		pointOnPlane = vec3(0, 0, objects[planeId].z/-d);

	if(dot(eye.xyz - pointOnPlane, N) > 0)
		return true;
	return false;
}

Intersection sphereIntersection(Ray ray, int id)
{
	vec3 L = objects[id].xyz - ray.source;
	float tm = dot(L, ray.dir);
	float d2 = pow(distance(objects[id].xyz,ray.source),2) - pow(tm, 2);
	float r = objects[id].w;
	Intersection inter = Intersection(-1, vec3(0,0,0));
	if(d2 <= pow(r,2))
	{
		float th = sqrt(pow(r,2) - d2);
		float t = tm-th > 0 ? tm-th : tm+th;
		if(t > 0)
			inter = Intersection(id, ray.source + ray.dir*t);
	}
	return inter;
}

Intersection planeIntersection(Ray ray, int id)
{
	vec3 N = objects[id].xyz;
	float d = objects[id].w;
	vec3 P0 = ray.source;
	float VdotN = dot(ray.dir, N);
	float t = -(dot(P0, N) + d)/VdotN;
	if(t < 0 || isEqualf(t,0) || isEqualf(VdotN, 0))
		return Intersection(-1, vec3(0,0,0));
	else 
		return Intersection(id, ray.source + ray.dir*t);
}

vec3 normalAt(vec3 point, int objId)
{
	vec3 normal;

	if(objects[objId].w > 0)
		normal = normalize(point - objects[objId].xyz);
	else
	{
		if(isEyeFacingPlane(objId))
			normal = normalize(objects[objId].xyz);
		else
			normal = normalize(-objects[objId].xyz);
	}

	return normal;
}

Intersection findIntersection(Ray ray)
{
	Intersection min_inter = Intersection(-1, vec3(0,0,0));
	Intersection temp_inter;
    for(int i=0; i < sizes.x; i++)
	{
		if(objects[i].w > 0)
		{
			temp_inter = sphereIntersection(ray, i);
			if(ray.sourceId != -1 && isEqualv3(temp_inter.point, ray.source) && dot(ray.dir, normalAt(ray.source, temp_inter.id)) > 0)
				continue;
		}
		else
		{
			temp_inter = planeIntersection(ray, i);
			if(ray.sourceId != -1  && temp_inter.id == ray.sourceId)
				continue;
		}
		if(temp_inter.id != -1)
			if(min_inter.id == -1 || distance(ray.source, temp_inter.point) < distance(ray.source, min_inter.point))
				min_inter = temp_inter;
	}
    return min_inter;
}

bool isShadowed(Intersection hit, int light)
{
	Ray rayToLight;

	if(isEqualf(lightsDirection[light].w, 1.0)) // spotlight
	{
		rayToLight = Ray(hit.point, normalize(lightPosition[light].xyz - hit.point), hit.id);
		Intersection k = findIntersection(rayToLight);

		if(k.id == -1 || objects[k.id].w < 0)
			return false;
	}
	else // directional light
	{
		rayToLight = Ray(hit.point, normalize(-lightsDirection[light].xyz), hit.id);
		Intersection k = findIntersection(rayToLight);

		if(k.id == -1 || objects[k.id].w < 0)
			return false;
	}

	return true;
}

vec3 BaseColorCalc(Intersection hit, Ray r)
{
	if(hit.id == -1) return vec3(0,0,0);
	// constants per object
	vec3 Ka = objColors[hit.id].xyz;
	float n = objColors[hit.id].w;
	vec3 Kd = Ka;
	if(objects[hit.id].w < 0 && (mod(floor(1.5*hit.point.x),2) == mod(floor(1.5*hit.point.y),2)))
		Kd = 0.5*Kd;
	vec3 Ks = vec3(0.7, 0.7, 0.7);
	vec3 color = Ka * ambient.xyz;
	for(int i = 0; i < sizes[1]; i++)
	{
		vec3 nRayToLight;
		if(!isShadowed(hit, i))
		{
			vec3 nLightDirection = normalize(lightsDirection[i].xyz);
			if(isEqualf(lightsDirection[i].w, 1)) // spotlight
			{
				nRayToLight = normalize(hit.point - lightPosition[i].xyz);
				float cosAngle = dot(nLightDirection, nRayToLight);
				if(cosAngle < lightPosition[i].w)
					continue;
				nLightDirection = nRayToLight;
			}
			vec3 N = normalAt(hit.point, hit.id);
			vec3 L = -nLightDirection;
			vec3 V = normalize(r.source - hit.point);
			vec3 Ri = reflect(L, N);
			color += ((Kd * dot(N, L) + Ks * pow(dot(V, Ri),n)))*lightsIntensity[i].xyz;
		}
	}
    
	return clamp(color,0.0, 1.0);
}

vec3 mirror5(Ray ray)
{	
	Intersection hit = findIntersection(ray);
	return BaseColorCalc(hit, ray);
}

vec3 mirror4(Ray ray)
{
	Intersection hit = findIntersection(ray);
	if(hit.id >= 0 && hit.id < sizes.z) // if mirror
	{
		Ray new_ray = Ray(hit.point, reflect(ray.dir,normalAt(hit.point, hit.id)), hit.id);
		return mirror5(new_ray);
	}
	return BaseColorCalc(hit, ray);
}

vec3 mirror3(Ray ray)
{
	Intersection hit = findIntersection(ray);
	if(hit.id >= 0 && hit.id < sizes.z) // if mirror
	{
		Ray new_ray = Ray(hit.point, reflect(ray.dir,normalAt(hit.point, hit.id)), hit.id);
		return mirror4(new_ray);
	}
	return BaseColorCalc(hit, ray);
}

vec3 mirror2(Ray ray)
{
	Intersection hit = findIntersection(ray);
	if(hit.id >= 0 && hit.id < sizes.z) // if mirror
	{
		Ray new_ray = Ray(hit.point, reflect(ray.dir,normalAt(hit.point, hit.id)), hit.id);
		return mirror3(new_ray);
	}
	return BaseColorCalc(hit, ray);
}

vec3 mirror1(Ray ray)
{
	Intersection hit = findIntersection(ray);
	if(hit.id >= 0 && hit.id < sizes.z) // if mirror
	{
		Ray new_ray = Ray(hit.point, reflect(ray.dir,normalAt(hit.point, hit.id)), hit.id);
		return mirror2(new_ray);
	}
	return BaseColorCalc(hit, ray);
}

vec3 colorCalc(Ray ray)
{	
	Intersection hit = findIntersection(ray);
	if(hit.id >= 0 && hit.id < sizes.z) // if mirror
	{
		Ray new_ray = Ray(hit.point, reflect(ray.dir,normalAt(hit.point, hit.id)), hit.id);
		return mirror1(new_ray);
	}
	return BaseColorCalc(hit, ray);
}

void main()
{
	// for current fragment pixel:
	float minx = -1 + xoff, miny = -1 + yoff;
	float maxx = 1 + xoff, maxy = 1 + yoff;
	vec2 scaledPixel = mix(vec2(minx, miny), vec2(maxx,maxy), gl_FragCoord.xy/800);

	// 1.	calculate the ray from the eye to the current pixel and normalize it
	vec3 pixelIn3D = vec3(scaledPixel.x, scaledPixel.y, zoom);
	vec3 vecToPixel = pixelIn3D - eye.xyz;
	Ray ray = Ray(eye.xyz, normalize(vecToPixel), -1); 

	// 2.	calculate the intersected objects with the calculated ray and return
	//		the closest object color (data)
	gl_FragColor = vec4(colorCalc(ray),1.0);
}
 