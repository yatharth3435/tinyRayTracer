#include <cmath>
#include <vector>
#include <fstream>
#include <limits>
#include <iostream>

using namespace std;

struct Vec3f {
    float x, y, z;

    Vec3f() : x(0), y(0), z(0) {}
    Vec3f(float xx, float yy, float zz) : x(xx), y(yy), z(zz) {}

    Vec3f operator + (const Vec3f &v) const {
        return Vec3f(x + v.x, y + v.y, z + v.z);
    }

    Vec3f operator - (const Vec3f &v) const {
        return Vec3f(x - v.x, y - v.y, z - v.z);
    }

    Vec3f operator * (float k) const {
        return Vec3f(x * k, y * k, z * k);
    }

    float dot(const Vec3f &v) const {
        return x*v.x + y*v.y + z*v.z;
    }

    Vec3f normalize() const {
        float mag = sqrt(dot(*this));
        return *this * (1.f / mag);
    }
};

struct Light {
    Vec3f position;
    float intensity;
    Light(const Vec3f &p, float i) : position(p), intensity(i) {}
};

struct Sphere {
    Vec3f center;
    float radius;
    Vec3f color;
    float specular;
    float reflective;

    Sphere(const Vec3f &c, float r, const Vec3f &col, float spec, float refl)
        : center(c), radius(r), color(col), specular(spec), reflective(refl) {}

    bool ray_intersect(const Vec3f &orig, const Vec3f &dir, float &t0) const {
        Vec3f L = center - orig;
        float tca = L.dot(dir);
        float d2 = L.dot(L) - tca * tca;
        if (d2 > radius * radius) return false;
        float thc = sqrt(radius * radius - d2);
        t0 = tca - thc;
        float t1 = tca + thc;
        if (t0 < 0) t0 = t1;
        if (t0 < 0) return false;
        return true;
    }
};

Vec3f reflect(const Vec3f &I, const Vec3f &N) {
    return I - N * 2.f * I.dot(N);
}

bool scene_intersect(
    const Vec3f &orig,
    const Vec3f &dir,
    const vector<Sphere> &spheres,
    Vec3f &hit,
    Vec3f &N,
    Vec3f &color,
    float &spec,
    float &refl
) {
    float dist = numeric_limits<float>::max();
    for (const Sphere &s : spheres) {
        float t;
        if (s.ray_intersect(orig, dir, t) && t < dist) {
            dist = t;
            hit = orig + dir * t;
            N = (hit - s.center).normalize();
            color = s.color;
            spec = s.specular;
            refl = s.reflective;
        }
    }
    return dist < numeric_limits<float>::max();
}

Vec3f cast_ray(
    const Vec3f &orig,
    const Vec3f &dir,
    const vector<Sphere> &spheres,
    const vector<Light> &lights,
    int depth = 0
) {
    Vec3f hit, N, color;
    float spec, refl;

    if (depth > 4 || !scene_intersect(orig, dir, spheres, hit, N, color, spec, refl))
        return Vec3f(0.2, 0.7, 0.8); // background

    float diffuse = 0, specular = 0;

    for (const Light &light : lights) {
        Vec3f light_dir = (light.position - hit).normalize();

        // Shadow check
        Vec3f shadow_hit, shadow_N, tmp;
        float sh_spec, sh_refl;
        if (scene_intersect(hit + N*1e-3, light_dir, spheres,
                             shadow_hit, shadow_N, tmp, sh_spec, sh_refl))
            continue;

        diffuse += light.intensity * max(0.f, light_dir.dot(N));
        Vec3f reflect_dir = reflect(-light_dir, N);
        specular += pow(max(0.f, reflect_dir.dot(dir)), spec) * light.intensity;
    }

    Vec3f reflect_dir = reflect(dir, N).normalize();
    Vec3f reflect_color = cast_ray(hit + N*1e-3, reflect_dir, spheres, lights, depth+1);

    return color * diffuse * (1 - refl)
         + Vec3f(1,1,1) * specular * 0.6
         + reflect_color * refl;
}

int main() {
    const int width = 800;
    const int height = 600;
    const float fov = M_PI / 2.;

    vector<Sphere> spheres = {
        Sphere(Vec3f(-3, 0, -16), 2, Vec3f(0.4, 0.4, 0.3), 50, 0.2),
        Sphere(Vec3f(-1, -1.5, -12), 2, Vec3f(0.3, 0.1, 0.1), 10, 0.4),
        Sphere(Vec3f(1.5, -0.5, -18), 3, Vec3f(0.3, 0.4, 0.3), 100, 0.3),
        Sphere(Vec3f(7, 5, -18), 4, Vec3f(0.1, 0.2, 0.4), 300, 0.1)
    };

    vector<Light> lights = {
        Light(Vec3f(-20, 20, 20), 1.5),
        Light(Vec3f(30, 50, -25), 1.8),
        Light(Vec3f(30, 20, 30), 1.7)
    };

    vector<Vec3f> framebuffer(width * height);

    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            float x =  (2*(i + 0.5)/(float)width  - 1) * tan(fov/2.) * width/(float)height;
            float y = -(2*(j + 0.5)/(float)height - 1) * tan(fov/2.);
            Vec3f dir = Vec3f(x, y, -1).normalize();
            framebuffer[i + j*width] = cast_ray(Vec3f(0,0,0), dir, spheres, lights);
        }
    }

    ofstream ofs("out.ppm", ios::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n";
    for (Vec3f &c : framebuffer) {
        char r = (char)(255 * max(0.f, min(1.f, c.x)));
        char g = (char)(255 * max(0.f, min(1.f, c.y)));
        char b = (char)(255 * max(0.f, min(1.f, c.z)));
        ofs << r << g << b;
    }
    ofs.close();

    cout << "Rendered image saved as out.ppm\n";
    return 0;
}
