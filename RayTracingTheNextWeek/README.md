# Ray Tracing: the Next Week

在 Ray Tracing In One Weekend 中，你构建了一个简单的、蛮力的 path tracer。本期我们会添加 textures、volumes（例如雾）、rectangles、instances、lights，以及支持对许多物体使用BVH。完成后，你就有一个 “真正的” ray tracer 了。

本书最难的两部分是 BVH 和 the Perlin textures。顺序对于本书介绍的概念不是很重要，即使没有 BVH 和 Perlin texture，你也可以得到一个 Cornell Box！

## Chapter 1: Motion Blur 动态模糊

当你决定进行光纤追踪时，你会认为只要增加了视觉质量即使增加了运行时间也是值得的。在之前的 fuzzy reflection 和 defocus blur 中，你需要对每一个像素进行多次采样。一旦你采用了这种方法，好处就是几乎所有的效果都可以通过这种蛮力的方式来实现。动态模糊（motion blur）就是其中一个可以蛮力实现的效果。在真实相机中，快门会打开并在一定的时间内保持打开的状态，然后快门打开时，相机或者物体可以会移动，我们想要的是相机在这段时间内看到的东西的平局值。我们可以通过当快门打开时，在一些随机的时间点发送每条 ray 来得到一个随机的结果。只要物体在这个时间点处于正确的位置，我们就能够用一条处于单个时间点的 ray 得到正确的平均值。这就是为什么随机的 ray tracing 趋向简单的原因。

基本思想是当快门打开时，生成多条处于随机时间点的 rays，然后在这个时间点和模型相交。通常这样做的方法是让相机移动或让物体移动，但是需要准确地知道每条 ray 处于哪个时间点上。用这种方法的话，ray tracer 的“引擎”就可以确保物体确实是处于该 ray 所需要的位置，这样相交检测的那部分代码就不需要作出太大改动。

因此，我们首先需要一条存储了该 ray 存在时间的 ray：

```diff
class ray {
     public:
         ray() {}
-        ray(const vec3 &a, const vec3 &b) { A = a; B = b; }
+        ray(const vec3 &a, const vec3 &b, float ti = 0.0) { A = a; B = b; _time = ti; }

         vec3 origin() const     { return A; }
         vec3 direction() const  { return B; }
+        float time() const   { return _time; }
         vec3 point_at_parameter(float t) const { return A + t*B; }

         vec3 A;
         vec3 B;
+        float _time;
 };
```

然后，我们需要修改 camera 的代码，使其可以随机生成在 *time1* 和 *time2* 之间随机时间点的 rays。camera 应该保留对 *time1* 和 *time2* 的记录还是应该让 camera 的使用者在创建 ray 时来决定？当有分歧时，我倾向于使用可以让调用更加简单的方法，即使这样做会使构造函数复杂一点，因此我会让 camera 保存这两个值，当然，这只是个人的偏好。因为现在还并不能移动相机，所以并不需要对 camera 作出太大的改动，它只会在这个时间段发射 rays。

```diff
 class camera {
     public:
-        camera(vec3 lookfrom, vec3 lookat, vec3 vup, float vfov, float aspect, float aperture, float focus_dist) {
+        // new: add t0 and t1
+        camera(vec3 lookfrom, vec3 lookto, vec3 up, float vfov, float aspect, float aperture, float focus_dist, float t0, float t1) {
+            time0 = t0;
+            time1 = t1;
             lens_radius = aperture / 2.0;
             float theta = vfov*M_PI/180.0;
             float half_height = tan(theta/2.0);
             float half_width = aspect * half_height;
             origin = lookfrom;
             w = unit_vector(lookfrom - lookat);
             u = unit_vector(cross(vup, w));
             v = cross(w, u);
             lower_left_corner = origin - half_width*focus_dist*u - half_height*focus_dist*v - focus_dist*w;
             horizontal = 2*half_width*focus_dist*u;
             vertical = 2*half_height*focus_dist*v;
         }
         // 新增 time 来构造 ray
         ray get_ray(float s, float t) {
             vec3 rd =  lens_radius*random_in_unit_ disk();
             vec3 offset = u * rd.x() +  v * rd.y();
+            float time = time0 +  drand48()*(time1-time0);
-            return ray(origin + offset, lower_left_corner + s*horizontal + t*vertical - origin - offset);
+            return ray(origin + offset, lower_left_corner + s*horizontal + t*vertical - origin - offset, time);
         }
        
         vec3 origin;
         vec3 lower_left_corner;
         vec3 horizontal;
         vec3 vertical;
         vec3 u, v, w;
+        float time0, time1;  // 表示 shutter 开启和关闭时间的变量
         float lens_radius;
}
```

我们还需要一个移动的物体。我会创建一个 sphere class，该 sphere 的球心在 *time0* 时间处于 *center0*，在 *time1* 时间处于 *center1*，并线性移动。而在这时间段之外的时间，该球体会继续移动，因为这些之外的时间并不需要和相机光圈的开闭相匹配。

```cpp
class moving_sphere: public hitable {
    public:
        moving_sphere() {}
        moving_sphere(vec3 cen0, vec3 cen1, float t0, float t1, float r, material *m) : center0(cen0), center1(cen1), time0(t0), time1(t1), radius(r), mat_ptr(m) {};
        virtual bool hit(const ray& r, float tmin, float tmax, hit_record& rec) const;
        vec3 center(float time) const;

        vec3 center0, center1;
        float time0, time1;
        float radius;
        material *mat_ptr;
}
```

检测是否相交的代码只需要改动一个地方：*center* 需要改成一个函数 *center(float)*：

```diff
+// 用 "center(rec.time)" 替换 "center"
 bool moving_sphere::hit(const ray& r, float t_min, float t_max, hit_record& rec) const {
-    vec3 oc = r.origin() - center;
+    vec3 oc = r.origin() - center(r.time);
     float a = dot(r.direction(), r.direction());
     float b = dot(oc, r.direction());
     float c = dot(oc, oc) - radius*radius;
     float discriminant = b*b - a*c;
     if (discriminant > 0) {
         float temp = (-b - sqrt(b*b-a*c))/a;
         if (temp < t_max && temp > t_min) {
             rec.t = temp;
             rec.p = r.point_at_parameter(rec.t);
-            rec.normal = (rec.p - center) / radius;
+            rec.normal = (rec.p - center(rec.time)) / radius;
             rec.mat_ptr = mat_ptr;
             return true;
         }
         temp = (-b +sqrt(b*b-a*c))/a;
         if (temp < t_max && temp > t_min) {
             rec.t = temp;
             rec.p = r.point_at_parameter(rec.t);
-            rec.normal = (rec.p - center) / radius;
+            rec.normal = (rec.p - center(rec.time)) / radius;
             rec.mat_ptr = mat_ptr;
             return true;
         }
     }
     return false;
 }
```

并确保 material 的 scattered ray 是处于该时间点的入射光（incident ray）。

```diff
 class lambertian : public material {
     public:
         lambertian(const vec3& a) : albedo(a) {}
         virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation,  ray& scattered) const {
             vec3 target = rec.p + rec.normal + random_in_unit_sphere();
-            scattered = ray(rec.p, target-rec.p);
+            scattered = ray(rec.p, target-rec.p, r_in.time());
             attenuation = albedo;
             return true;
         }
 
         vec3 albedo;
 };
```

我们使用上一本书的最后一个场景，并使球体从它们的中心点（此时 `time=0`）移动到 `center + vec3(0, 0.5*drand48(), 0)`（此时 `time=1`），相机的光圈在该帧内始终是打开状态。

```diff
 hitable *random_scene() {
-    int n = 500;
+    int n = 50000;
     hitable **list = new hitable*[n+1];
     list[0] = new sphere(vec3(0,-1000,0), 1000,  new lambertian(vec3(0.5, 0.5, 0.5)));
 
     int i = 1;
-    for (int a = -11; a < 11; a++) {
+    for (int a = -10; a < 10; a++) {
-        for (int b = -11; b < 11; b++) {
+        for (int b = -10; b < 10; b++) {
             float choose_mat = drand48();
             vec3 center(a+0.9*drand48(), 0.2, b+0. 9*drand48());
             if ((center-vec3(4,0.2,0)).length() >  0.9) {
                 if (choose_mat < 0.8) { // diffuse
-                   list[i++] = new sphere(center, 0.2,
+                   list[i++] = new moving_sphere(center, center+vec3(0,0.5*drand48(),0), 0.0, 1.0, 0.2,
                              new lambertian(vec3 (drand48()*drand48(),  drand48()*drand48(),  drand48()*drand48() )) );
                 } else if (choose_mat < 0.95) { //  metal
                     list[i++] = new sphere(center,  0.2,
                             new metal(vec3(0.5*(1  + drand48()), 0.5*(1 +  drand48()), 0.5*(1 +  drand48())), 0. 5*drand48()));
                 } else { // glass
                     list[i++] = new sphere(center,  0.2, new dielectric(1.5));
                 }
             }
         }
     }
 
     list[i++] = new sphere(vec3(0,1,0), 1.0, new  dielectric(1.5));
     list[i++] = new sphere(vec3(-4,1,0), 1.0, new  lambertian(vec3(0.4, 0.2, 0.1)));
     list[i++] = new sphere(vec3(4,1,0), 1.0, new  metal(vec3(0.7, 0.6, 0.5), 0.0));
 
     return new hitable_list(list, i);
 }
```

然后改变视角参数：

```diff
     vec3 lookfrom(13,2,3);
     vec3 lookat(0,0,0);
     float dist_to_focus = 10.0;
-    float aperture = 0.1;
+    float aperture = 0.0;
-    camera cam(lookfrom, lookat, vec3(0,1,0), 20, float(nx)/float(ny), aperture, dist_to_focus);
+    camera cam(lookfrom, lookat, vec3(0,1,0), 20, float(nx)/float(ny), aperture, dist_to_focus, 0.0, 1.0);
```

得到

![./01.motion_blur_1200x800.jpg](./01.motion_blur_1200x800.jpg)



## Chapter 2: Bounding Volume Hierarchies
