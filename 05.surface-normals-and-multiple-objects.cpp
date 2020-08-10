#include <iostream>
#include "ray.h"

bool hit_sphere(const vec3& center, float radius, const ray& r) {
    vec3 oc = r.origin() - center;
    float a = dot(r.direction(), r.direction());
    float b = 2.0 * dot(oc, r.direction());
    float c = dot(oc, oc) - radius*radius;
    float discriminant = b*b - 4*a*c;
    return (discriminant > 0);
}

vec3 color(const ray& r) {
    if (hit_sphere(vec3(0,0,-1), 0.5, r)) {  // 检测 ray 是否与位于 [0,0,-1]，半径为 0.5 的球相交
                                             // 如果你把球心 z 设为 +1，那么你会得到相同的图像，因为你现在能够看到你背后的对象，我们之后会修复这个问题。
        return vec3(1.0, 0.0, 0.0);
    }
    vec3 unit_direction = unit_vector(r.direction());
    float t = 0.5*(unit_direction.y() + 1.0);
    return (1.0-t)*vec3(1.0, 1.0, 1.0) + t*vec3(0.5, 0.7, 1.0);
}

int main() {
    int nx = 200;
    int ny = 100;
    std::cout << "P3\n" << nx << " " << ny << "\n255\n";
    vec3 lower_left_corner(-2.0, -1.0, -1.0);  // 屏幕左下角的位置在 3D 场景内的坐标
    vec3 horizontal(4.0, 0.0, 0.0);            // 屏幕的宽度
    vec3 vertical(0.0, 2.0, 0.0);              // 屏幕的高度
    vec3 origin(0.0, 0.0, 0.0);                // eye 的原点
    for (int j = ny-1; j >= 0; j--) {
        for (int i = 0; i < nx; i++) {
            float u = float(i) / float(nx);
            float v = float(j) / float(ny);
            ray r(origin, lower_left_corner + u*horizontal + v*vertical);
            vec3 col = color(r);
            int ir = int(255.99*col[0]);
            int ig = int(255.99*col[1]);
            int ib = int(255.99*col[2]);

            std::cout << ir << " " << ig << " " << ib << "\n";
        }
    }
}
