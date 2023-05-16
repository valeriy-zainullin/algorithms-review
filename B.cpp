#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <vector>

#define DEBUG 0

struct Point {
	int32_t x;
	int32_t y;
};

// Алгоритм Джарвиса (заворачивания подарка).
// TODO: написать объяснение, доказать.
std::vector<size_t> MakeConvexHull(const std::vector<Point>& points) {
	assert(!points.empty());
	
	std::vector<size_t> indices(points.size(), 0);
	for (size_t i = 0; i < points.size(); ++i) {
		indices[i] = i;
	}
	
	// Удалим повторяющиеся точки, одну и ту же точку
	//   всё равно в выпуклую оболочку брать не будем.
	std::sort(
		indices.begin(),
		indices.end(),
		[&](size_t lhs, size_t rhs) {
			const Point& p1    = points[lhs];
			const Point& p2    = points[rhs];
			
			if (p1.x != p2.x) {
				return p1.x < p2.x;
			}
			return p1.y < p2.y;
		}
	);
	indices.erase(
		std::unique(
			indices.begin(),
			indices.end(),
			[&](size_t lhs, size_t rhs) {
				const Point& p1 = points[lhs];
				const Point& p2 = points[rhs];
			
				return p1.x == p2.x && p1.y == p2.y;
			}
		),
		indices.end()
	);
	
	// В задаче хотят, чтобы фигуры выводилась с самой левой, а если таких несколько,
	//   с самой нижней самой левой. Выбираем такую.
	size_t selected_index = indices[0];
	for (size_t i = 1; i < indices.size(); ++i) {
		const Point& selected = points[selected_index];
		const Point& current  = points[indices[i]];
		if (current.x < selected.x || (current.x == selected.x && current.y < selected.y)) {
			selected_index = i;
		}
	}
	indices.erase(std::find(indices.begin(), indices.end(), selected_index));
	
	#if DEBUG
		fprintf(stderr, "selected_index = %zu.\n", selected_index);
	#endif
	
	// Все точки в первом и втором квадранте, сортируем их по
	//   возрастанию угла направляющего вектора от выбранной
	//   точки. Или, что то же самое, т.к. они в первых двух
	//   квадрантах, по убыванию контангенса этого угла, можно
	//   посчитать этот котангенс по координатам.
	// Так мы получим обход против часовой, тогда ещё используем
	//   предикат "правая тройка". Обычно пишу так, но в задаче
	//   хотят обход по часовой, потому сортируем по убыванию
	//   тангенса (теперь все точки в первом и четвертом
	//   квадранте), по убыванию угла. И предикат используем не
	//   "правая тройка", а "левая тройка".
	// При совпадении углов нужно сортировать по расстоянию до
	//   выбранной точки, иначе есть пример, когда алгоритм не
	//   работает, он ниже. Ещё можно почитать об алгоритме по
	//   ссылке: https://wcipeg.com/wiki/Convex_hull
	std::sort(
		indices.begin(),
		indices.end(),
		[&](size_t lhs, size_t rhs) {
			const Point& p1    = points[lhs];
			const Point& p2    = points[rhs];
			const Point& start = points[selected_index];
			
			int32_t p1_dx = p1.x - start.x;
			int32_t p1_dy = p1.y - start.y;
			
			int32_t p2_dx = p2.x - start.x;
			int32_t p2_dy = p2.y - start.y;

			// p1_dy / p1_dx -> p1_tg
			// p2_dy / p2_dx -> p2_tg.
			// p1_tg > p2_tg.
			// p1_dy / p1_dx > p2_dy / p2_dx.
			// перенесём знак в p1_dy, p2_dy,
			//   если p1_dx или p2_dx отрицательны.
			//   Затем умножим обе части на p1_dx и p2_dx.
			// (sign1 * p1_dy) p2_dx > (sign2 * p2_dy) p1_dx.
			// Отдельно нужно разобрать случай, когда dx равен нулю.
			// Тогда угол равен pi/2, т.к. выбрали самую нижнюю из самых левых.
			//   Если бы были повторы, могли бы получить ещё одну точку, которая
			//   совпадала бы с выбранной. Угол неизвестен. Мы бы просто всегда
			//   её брали, если границы не входят в фигуру, или не брали бы,
			//   удаляли бы повторы так же, если бы границы входили.
			
			if (p1_dx == 0 && p2_dx != 0) {
				return true;
			}
			if (p1_dx != 0 && p2_dx == 0) {
				return false;
			}
			if (p1_dx == 0 && p2_dx == 0) {
				// Полярный угол с центром в выбранной точке
				//   у точек один: pi/2. Тогда предпочитаем
				//   ближайшую.
				return p1_dy < p2_dy;
			}

		 	if (p1_dx < 0) {
		 		p1_dx *= -1;
		 		p1_dy *= -1;
		 	}
		 	
		 	if (p2_dx < 0) {
		 		p2_dx *= -1;
		 		p2_dy *= -1;
		 	}
		 	
		 	int64_t tan_cmp = static_cast<int64_t>(p1_dy) * p2_dx - static_cast<int64_t>(p2_dy) * p1_dx;
		 	if (tan_cmp != 0) {
		 		return tan_cmp > 0;
		 	}
		 	
		 	// Углы совпадают, выбираем ближайшую.

		  	int64_t p1_distsq = static_cast<int64_t>(p1_dx) * p1_dx + static_cast<int64_t>(p1_dy) * p1_dy;
		  	int64_t p2_distsq = static_cast<int64_t>(p2_dx) * p2_dx + static_cast<int64_t>(p2_dy) * p2_dy;
		  	
		  	return p1_distsq < p2_distsq;
		}
	);
	
	#if DEBUG
		fprintf(stderr, "indices = [");
		for (size_t i = 0; i < indices.size(); ++i) {
			fprintf(stderr, "%zu,", indices[i]);
		}
		fprintf(stderr, "].\n");
	#endif
	
	std::vector<size_t> points_in_hull = {selected_index};
	
	// Чтобы алгоритм создавал правильную оболочку в случае,
	//   когда могут быть две точки с одним углом, нужно
	//   выбирать либо упорядочивать по убыванию расстояния,
	//   либо по возрастанию: если у нас есть две точки с
	//   одним углом, ещё ок, а если три, то выберем среднюю,
	//   затем пусть идет более дальняя, её возьмём, а затем
	//   пусть более ближняя. Тогда она выкинет более дальнюю,
	//   мы потеряем эту дальнюю точку. Потому при совпадении
	//   углов (тангенсов), сортируем по расстоянию.
	for (size_t new_point: indices) {
		const Point& p3 = points[new_point];
		while (points_in_hull.size() >= 2) {
			// Пусть
			//   p1 -- points_in_hull[points_in_hull.size()-2],
			//   p2 -- points_in_hull[points_in_hull.size()-1],
			//   p3 -- new_point.
			// Если предыдущая точка ломает выпуклость,
			//   т.е. угол от вектора (p1, p2) к вектору
			//   (p2, p3) против часовой меньше pi, то
			//   выбрасываем p2, она будет внутри выпуклой
			//   оболочки, т.к. внутри треугольника
			//   (selected_point, p1, p3).
			// Если угол равен pi, тоже выбрасываем, точка на
			//   границе считается внутри фигуры (если было бы
			//   не так, включали бы), 
			// Проверить это можно с помощью "псевдоскалярного"
			//   произведения, ещё его называют предикатом
			//   "правая тройка", а ещё это третья координата
			//   векторного произведения.
			
			const Point& p1 = points[points_in_hull[points_in_hull.size() - 2]];
			const Point& p2 = points[points_in_hull[points_in_hull.size() - 1]];
			
			int32_t p12_dx = p2.x - p1.x;
			int32_t p12_dy = p2.y - p1.y;
			int32_t p23_dx = p3.x - p2.x;
			int32_t p23_dy = p3.y - p2.y;

			int64_t product = 
				static_cast<int64_t>(p12_dx) * p23_dy - 
				static_cast<int64_t>(p12_dy) * p23_dx;

			// a_x * b_y - a_y * b_x = |a| * |b| * sin(a ^ b)
			// Проверяем, что угол больше pi.
			if (product >= 0) {
				points_in_hull.pop_back();
			} else {
				break;
			}
		}

		points_in_hull.push_back(new_point);

		#if DEBUG
			fprintf(stderr, "points_in_hull = [");
			for (size_t i = 0; i < points_in_hull.size(); ++i) {
				fprintf(stderr, "%zu,", points_in_hull[i]);
			}
			fprintf(stderr, "].\n");
		#endif
	}
	
	// Последняя точка могла быть на одной прямой с первой и предпоследней,
	//   т.к. для первой точки мы не проверяли, что угол больше pi.
	// Проверим, т.к. можем и неправильную оболочку получить.
	{
		const Point& p3 = points[selected_index];
		while (points_in_hull.size() >= 2) {
			const Point& p1 = points[points_in_hull[points_in_hull.size() - 2]];
			const Point& p2 = points[points_in_hull[points_in_hull.size() - 1]];
			
			int32_t p12_dx = p2.x - p1.x;
			int32_t p12_dy = p2.y - p1.y;
			int32_t p23_dx = p3.x - p2.x;
			int32_t p23_dy = p3.y - p2.y;

			int64_t product = 
				static_cast<int64_t>(p12_dx) * p23_dy - 
				static_cast<int64_t>(p12_dy) * p23_dx;

			// a_x * b_y - a_y * b_x = |a| * |b| * sin(a ^ b)
			// Проверяем, что угол больше pi.
			if (product >= 0) {
				points_in_hull.pop_back();
			} else {
				break;
			}
		}
	}	

	return points_in_hull;
}

int main() {
	size_t num_points = 0;
	scanf("%zu", &num_points);
	
	std::vector<Point> points(num_points);
	for (size_t i = 0; i < num_points; ++i) {
		Point& point = points[i];
		scanf("%" SCNd32 " %" SCNd32, &point.x, &point.y);
	}
	
	std::vector<size_t> convex_hull = MakeConvexHull(points);
	printf("%zu\n", convex_hull.size());
	for (size_t i = 0; i < convex_hull.size(); ++i) {
		const Point& point = points[convex_hull[i]];
		printf("%" PRId32 " %" PRId32 "\n", point.x, point.y);
	}
	
	// Считаем ориентированную площадь методом трапеций.
	int64_t result = 0;
	for (size_t i = 0; i + 1 < convex_hull.size(); ++i) {
		const Point& p1 = points[convex_hull[i]];
		const Point& p2 = points[convex_hull[i + 1]];
	
		int32_t y1 = p1.y;
		int32_t y2 = p2.y;
		int32_t dx = p2.x - p1.x;
		
		result += static_cast<int64_t>(y1 + y2) * dx;
	}
	{
		const Point& p1 = points[convex_hull.back()];
		const Point& p2 = points[convex_hull.front()];
	
		int32_t y1 = p1.y;
		int32_t y2 = p2.y;
		int32_t dx = p2.x - p1.x;
		
		result += static_cast<int64_t>(y1 + y2) * dx;
	}
	
	// Это ориентированная площадь, она положительна при
	//   обходе по часовой (можно представить пример).
	// Потому модуль брать не надо, т.к. функция
	//   построения выпуклой оболочки выдаёт её в
	//   обходе по часовой (так нужно в задаче).
	// Если бы обход был против часовой, нужно было
	//   бы брать.
	/*
	if (result < 0) {
		result *= -1;
	}
	*/
	assert(result > 0);
	
	// Доказать, что точности в один знак достаточно можно
	//   с помощью формулы Пика: для многоугольника с
	//   целочисленными координатами вершин есть формула
	//   через количество точек сетки на границах и внутри
	//   фигуры, там есть только деление на два.
	// Потому ответа оканчивается на .0 или .5.
	if (result % 2 == 0) {
		printf("%" PRId64 ".0\n", result / 2);
	} else {
		printf("%" PRId64 ".5\n", result / 2);
	}
	
	return 0;
}