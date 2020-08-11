#include <iostream>
#include <random>
#include "sphere.h"
#include "hitable_list.h"
#include "camera.h"
#include "float.h"

vec3 random_in_unit_sphere() {
    vec3 p;
    do {
        // [0, +2] => [-1, +1]
        p = 2.0*vec3(drand48(),drand48(),drand48()) - vec3(1.0,1.0,1.0);
    } while (p.squared_length() >= 1.0);
    return p;
}

vec3 color(const ray& r, hitable *world) {
    hit_record rec;
    if (world->hit(r, 0.001, MAXFLOAT, rec)) {
        vec3 target = rec.p + rec.normal + random_in_unit_sphere();
        // 从相交点再发射出一条光线，方向为从相交点指向相交点正切的单位球内的点
        // 光线的颜色衰减为原来的 0.5 倍
        return 0.5*color( ray(rec.p, target-rec.p), world);
    } else {
        vec3 unit_direction = unit_vector(r.direction());
        float t = 0.5*(unit_direction.y() + 1.0);
        return (1.0-t)*vec3(1.0, 1.0, 1.0) + t*vec3(0.5, 0.7, 1.0);
    }
}

int main() {
    int nx = 200;
    int ny = 100;
    int ns = 100;  // 每个像素的采样次数
    std::cout << "P3\n" << nx << " " << ny << "\n255\n";

    hitable *list[2];
    list[0] = new sphere(vec3(0,0,-1), 0.5);
    list[1] = new sphere(vec3(0,-100.5,-1), 100);
    hitable *world = new hitable_list(list, 2);

    camera cam;

    for (int j = ny-1; j >= 0; j--) {
        std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;
        for (int i = 0; i < nx; i++) {
            vec3 col(0, 0, 0);
            for (int s = 0; s < ns; s++) {
                float u = float(i + drand48()) / float(nx);
                float v = float(j + drand48()) / float(ny);
                ray r = cam.get_ray(u, v);
                col += color(r, world);
            }
            col /= float(ns);
            // gamma corrected
            col = vec3( sqrt(col[0]), sqrt(col[1]), sqrt(col[2]) );
            int ir = int(255.99*col[0]);
            int ig = int(255.99*col[1]);
            int ib = int(255.99*col[2]);

            std::cout << ir << " " << ig << " " << ib << "\n";
        }
    }

    std::cerr << "\nDone.\n";
}
