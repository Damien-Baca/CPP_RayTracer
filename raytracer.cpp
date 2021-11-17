#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>
#include "./Eigen/Core"
#include "./Eigen/Geometry"
using namespace std;

typedef Eigen::Matrix<double,3,1> Vector3f;
typedef Eigen::Matrix<double,4,1> Vector4f;
typedef Eigen::Matrix<double,3,3> Matrix3f;

Vector3f extractVector3f(ifstream& file)
{
	double x,y,z;
	file >> x >> y >> z;
	return Vector3f(x,y,z);
}

Vector4f extractVector4f(ifstream& file)
{
	double w;
	Vector3f part = extractVector3f(file);
	file >> w;
	return Vector4f(part(0,0), part(1,0), part(2,0), w);
}

class Ray
{
public:
	Vector3f point_;
	Vector3f direction_;

	Ray(Vector3f point, Vector3f direction) : point_(point), direction_(direction) {};
};

class RaySphereTestResponse
{
public:
	bool state;
	double t;
	Vector3f pt;

	RaySphereTestResponse(bool state_, double t_, Vector3f pt_) : state(state_), t(t_), pt(pt_) {};
};

class RaySphereColorResponse
{
public:
	bool state;
	Vector3f color;

	RaySphereColorResponse(bool state_, Vector3f color_) : state(state_), color(color_) {};
};

class Camera
{
public:
	int near_;
	int width_;
	int height_;      //  0      1      2     3
	Vector4f bounds_; // left, right, bottom, top
	Vector3f eye_, look_, up_;

	Camera() : near_(0), width_(0), height_(0) {};
	Camera(int width, int height, int near, Vector4f bounds, Vector3f eye, Vector3f look, Vector3f up) :
		   near_(near), width_(width), height_(height), bounds_(bounds), eye_(eye), look_(look), up_(up){};

	friend ostream& operator<<(ostream& os, Camera c) {
		return os << "    Camera ---------------------\n"
			        << "    near   = " << c.near_ << endl
			        << "    res    = " << c.width_ << " x " << c.height_ << endl
			        << "    bounds = " << c.bounds_ << endl
			        << "    eye    = " << c.eye_ << endl
			        << "    look   = " << c.look_ << endl
			        << "    up     = " << c.up_ << endl;
	};
};

class Light
{
	public:
		bool isDiffuse_;
		Vector3f lightColor_;
		Vector4f lightPosition_;

		// Ambient
		Light(Vector3f lightColor)
			: isDiffuse_(false), lightColor_(lightColor), lightPosition_(Vector4f(0.0,0.0,0.0,0.0)) {};

		// Diffuse
		Light(Vector3f lightColor, Vector4f lightPosition)
			: isDiffuse_(true), lightColor_(lightColor), lightPosition_(lightPosition) {};

		friend ostream& operator<<(ostream& os, Light l) {
			return os << "    Light ---------------------\n"
			          << "    type   = " << (l.isDiffuse_ ? "Diffuse" : "Ambient") << endl
			 		  		<< "    color  = " << l.lightColor_ << endl
					  		<< "    pos    = " << l.lightPosition_ << endl;
		};
};

class Sphere
{
public:
	double radius_;
	Vector3f position_;
	Vector3f Ka_;
	Vector3f Kd_;
	Sphere(double radius, Vector3f position, Vector3f Ka, Vector3f Kd)
		: radius_(radius), position_(position), Ka_(Ka), Kd_(Kd) {};

	friend ostream& operator<<(ostream& os, Sphere s) {
		return os << "    Objects ---------------------" << endl
		          << "    Sphere : " << endl
		          << "    radius = " << s.radius_ << endl
		          << "    position = " << s.position_ <<  endl
		          << "    Ka = " << s.Ka_ << endl
		          << "    Kd = " << s.Kd_ << endl;
	};
};

class Scene
{
public:
	Camera         camera_;
	vector<Sphere> objects_;
	vector<Light>  diffuse_lights_;
	vector<Light>  ambient_lights_;

	Scene(){};
	Scene(string fileName){parseScene(fileName);};

	Ray pixelRay(double i, double j);
	void parseScene(string fileName);
	RaySphereColorResponse raySphereRGB(Ray ray, Sphere sph);
	RaySphereTestResponse  raySphereTest(Ray ray, Sphere sph);

	void unpack();
	void sortByDepth();
	void rendertoFile(string fileName);

	friend ostream& operator<<(ostream& os, Scene s) {
		os << "Scene --------------------------" << endl
		   << s.camera_ << endl;

		for(Light l : s.diffuse_lights_)
			os << l << endl;

		for(Light l : s.ambient_lights_)
			os << l << endl;

		for(Sphere sp : s.objects_)
			os << sp << endl;

		return os;
	}

private:
	Vector3f CWv;
	Vector3f CUv;
	Vector3f CVv;
};

void Scene::sortByDepth()
{
	// Index BEGIN (0)                = Farthest Object
	// Index END   (objects.size - 1) = Closest  Object
	sort(objects_.begin(), objects_.end(),
		[this](const Sphere& a, const Sphere& b) -> bool // '<' operator
		{
			double dist_to_a = (a.position_ - camera_.eye_).norm();
			double dist_to_b = (b.position_ - camera_.eye_).norm();
			return dist_to_a > dist_to_b;
		});
}

void Scene::rendertoFile(string fileName)
{
	ofstream ppm(fileName.c_str());
	vector<vector<Vector3f>> pixels;

	if(!ppm.is_open()) {
		cerr << "File Open Error" << endl;
		throw exception();
	}

	int width = camera_.width_;
	int height = camera_.height_;

	unpack();
	sortByDepth();

	ppm << "P3" << endl;
	ppm << width << " " << height << " 255" << endl;

	for(unsigned k = 0; k < objects_.size(); k++)
	{
		for(int i = 0; i < width; i++)
		{
			pixels.push_back(vector<Vector3f>(height));
			for(int j = 0; j < height; j++)
			{
				RaySphereColorResponse result = raySphereRGB(pixelRay(j,i), objects_[k]);
				if(result.state)
				{
					double R = max(0.0,min(255.0,round(result.color(0,0) * 255.0)));
					double G = max(0.0,min(255.0,round(result.color(1,0) * 255.0)));
					double B = max(0.0,min(255.0,round(result.color(2,0) * 255.0)));
					pixels[i][j] = Vector3f(R,G,B);
				}
				else if(k == 0) // If k == 0 then drawing farthest element. Draw with black background.
				{
					pixels[i][j] = Vector3f(0,0,0);
				}
			}
		}
	}

	for(vector<Vector3f> v_outer : pixels)
		for(Vector3f v_inner : v_outer)
			ppm << v_inner(0,0) << " " << v_inner(1,0) << " " << v_inner(2,0) << " ";

	ppm.close();
}

RaySphereTestResponse Scene::raySphereTest(Ray ray, Sphere sph)
{
	double r     = sph.radius_;
	Vector3f Cv  = sph.position_;
	Vector3f Lv  = ray.point_;
	Vector3f Uv  = ray.direction_;
	Vector3f Tv  = Cv - Lv;
	double v   = Tv.dot(Uv);
	double csq = Tv.dot(Tv);
	double disc = pow(r,2) - (csq - pow(v,2));

	if(disc < 0) {
		return RaySphereTestResponse(false, 0, Vector3f(0,0,0));
	} else {
		double d = sqrt(disc);
		double t = v - d;
		Vector3f pt = Lv + t * Uv;
		return RaySphereTestResponse(true, t, pt);
	}
}

Ray Scene::pixelRay(double i, double j)
{
	double left   = camera_.bounds_(0,0); // left
	double right  = camera_.bounds_(1,0); // right
	double bottom = camera_.bounds_(2,0); // bottom
	double top    = camera_.bounds_(3,0); // top

	double px = (i/(camera_.width_ -  1.0)) * (right - left) + left;
	double py = (j/(camera_.height_ - 1.0)) * (bottom - top) + top;

	Vector3f Lv = camera_.eye_ + (camera_.near_ * CWv) + (px * CUv) + (py * CVv);
	Vector3f Uv = (Lv - camera_.eye_).normalized();

	return Ray(Lv, Uv);
}

RaySphereColorResponse Scene::raySphereRGB(Ray ray, Sphere sph)
{
	RaySphereTestResponse hitp = raySphereTest(ray, sph);

	if(hitp.state)
	{
		Vector3f snrm = (hitp.pt - sph.position_).normalized();

		Vector3f color(0,0,0);

		// Ambient Color Calculation
		for(Light lt : ambient_lights_)
			for(int i = 0; i < 3; i++)
				color(i,0) += lt.lightColor_(i,0) * sph.Ka_(i,0);

		// Diffuse Color Calculation
		for(Light lt : diffuse_lights_)
		{
			Vector3f ptL(lt.lightPosition_(0,0), lt.lightPosition_(1,0), lt.lightPosition_(2,0));
			Vector3f toL = (ptL - hitp.pt).normalized();
			if(snrm.dot(toL) > 0.0)
			{
				for(int i = 0; i < 3; i++)
					color(i,0) += lt.lightColor_(i,0) * sph.Kd_(i,0) * snrm.dot(toL);
			}
		}

		return RaySphereColorResponse(true, color);

	} else {
		return RaySphereColorResponse(false, Vector3f(0,0,0));
	}
}

void Scene::unpack()
{
	CWv = (camera_.eye_ - camera_.look_).normalized();
	CUv = ((camera_.up_).cross(CWv)).normalized();
	CVv = CWv.cross(CUv);
}

void Scene::parseScene(string fileName)
{
	ifstream file(fileName.c_str(), fstream::in);

	if(!file.is_open()) {
		cerr << "File Open Error\n";
		throw exception();
	}

	string value;
	while(file >> value)
	{
		if(value.compare("#") == 0)
			file.ignore(100, '\n');

		else if(value.compare("eye") == 0)
			this->camera_.eye_ = extractVector3f(file);

		else if(value.compare("look") == 0)
			this->camera_.look_ = extractVector3f(file);

		else if(value.compare("up") == 0)
			this->camera_.up_ = extractVector3f(file);

		else if(value.compare("bounds") == 0)
			this->camera_.bounds_ = extractVector4f(file);

		else if(value.compare("res") == 0)
		{
			file >> value; this->camera_.width_  = stoi(value);
			file >> value; this->camera_.height_ = stoi(value);
		}

		else if(value.compare("ambient") == 0)
		{
			Vector3f v = extractVector3f(file);
			this->ambient_lights_.push_back(Light(v));
		}

		else if(value.compare("d") == 0) {
			file >> value; this->camera_.near_ = stoi(value);
		}

		else if(value.compare("sphere") == 0)
		{
			double radius = 0.0;
			Vector3f pos = extractVector3f(file); // 1
			file >> radius; // 2
			Vector3f Ka = extractVector3f(file); // 3
			Vector3f Kd = extractVector3f(file); // 4
			this->objects_.push_back(Sphere(radius, pos, Ka, Kd));
		}

		else if(value.compare("light") == 0)
		{
			Vector4f pos   = extractVector4f(file);
			Vector3f color = extractVector3f(file);
			this->diffuse_lights_.push_back(Light(color, pos));
		}
	}
	file.close();
}

int main(int argc, char const *argv[])
{
	if(argc != 3) {
		cout << "Error - Too many or too few arguments. (Correct #: 2) (Recieved #: "
		     << argc - 1 << ")" << endl;
		return 0;
	}

	Scene newScene(argv[1]);
	newScene.rendertoFile(argv[2]);

	return 0;
}
