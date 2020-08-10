## Chapter 2: The vec3 class

对 color、location、directions、offsets 等等都使用同一个类来表示，即 `vec3`。

## Chapter 3: Rays, a simple camera, and background

所有的 ray tracer 都有的一个类就是 ray 类和一个沿着该 ray 将会看到什么颜色的计算过程。

假设一条 ray 为方程 p(t) = A + t*B。这里的 p 是 3D 中沿着一条线的一个 3D position。A 是该 ray 的原点，B 是该 ray 的方向。而 ray 的参数 t 是一个实数（代码中是浮点数）。插入不同的 t，p(t) 就会沿着该 ray 移动该点。加上负的 t，你就能够得到该 3D 线的任意一个位置了。对于正的 t，你只能得到 A 的前一部分，这通常被称为 half-line 或 ray。

Ray tracer 的核心是发送 rays 穿过 pixels，然后计算在这些 rays 的方向上看到的是什么颜色。即计算从 eye 到一个 pixel 的 ray，算出该 ray 与什么东西相交，然后算出该相交点的颜色。

## Chapter 4: Adding a sphere

人们通常在 ray tracer 中使用球体，因为计算一条 ray 和一个球体是否相交是非常直白的。一个球形位于原点且半径为 R 的球体的方程是 x\*x + y\*y + z\*z = R\*R。如果球心位于 (cx, cy, cz)，则方程为

(x-cx)\*(x-cx) + (y-cy)\*(y-cy) + (z-cz)\*(z-cz) = R\*R

在图形学中，你几乎总是想要你的公式以向量术语的形式来表示，这样所有 x/y/z 的东西都会在 vec3 class 的底层。从球心 C=(cx,cy,cz) 到点 p=(x,y,z) 的向量是 (p-C)，而点乘 dot((p-C), (p-C)) = (x-cx)\*(x-cx) + (y-cy)\*(y-cy) + (z-cz)\*(z-cz)。所以球体方程的向量形式是：

dot((p-C),(p-C)) = R\*R

我们可以读成“任意满足这个方程的点 p 都在球体上”。我们想要知道我们的 ray p(t)=A+t\*B 是否相交该球体。如果相交，则存在某些 t 使得 p(t) 满足该球体方程。所以我们对所有的 t 检查下面的方程是否为 true：

dot((p(t)-C),(p(t)-C)) = R\*R

展开 ray p(t) 的每一项，得到

dot((A+t\*B - C),(A+t\*B - C)) = R\*R

如果我们应用向量代数的规则展开该方程，并把所有的项放在左手边，我们就得到：

t\*t\*dot(B,B) + 2\*t\*dot(B,A-C) + dot(A-C,A-C) - R\*R = 0

该方程中的向量和 R 都是常量且已知，未知的是 t，且该方程是二次的。你可以解出 t，且有一个平方根项，可能为正（意味着有两个实解）、负（意味着没有实解）或者为 0（意味着有一个实解）。在图形学中，代数（algebra）几乎总是和几何（geometry）直接相关。
