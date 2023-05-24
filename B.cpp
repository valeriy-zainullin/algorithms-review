#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <vector>

#define DEBUG 0

#if defined(NDEBUG)
#define assert_with_node(expr, msg)
#else
#define assert_with_note(expr, msg) if (!(expr)) { puts(msg); assert(false); }
#endif

struct Point {
	int32_t x;
	int32_t y;
};

/**
 * \brief   Compares points by polar angle from the selected point.
 *
 * \details Does so by comparing tangents.
 *
 * \note    Points must not be above the selected point.
 */
struct PolarAngleComparator {
	PolarAngleComparator(size_t selected_index, const std::vector<Point>& points)
	  : selected_index_(selected_index), points_(points) {}

	bool operator()(size_t lhs, size_t rhs) const {
		const Point& p1    = points_[lhs];
		const Point& p2    = points_[rhs];
		const Point& start = points_[selected_index_];
			
		int32_t p1_dx = p1.x - start.x;
		int32_t p1_dy = p1.y - start.y;
			
		int32_t p2_dx = p2.x - start.x;
		int32_t p2_dy = p2.y - start.y;
			
		// Сравнение в сортировке по полярному углу. Детали в B_notes.cpp. Прочитайте,
		//   буду рад ответить на вопросы.
			
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
	
	private:
		size_t selected_index_ = 0;
		const std::vector<Point>& points_;
};

// Алгоритм Джарвиса (заворачивания подарка).
// TODO: написать объяснение, доказать.
/**
 * \brief   Calculates convex hull with Jarvis' algorithm.
 *
 * \note    This algorithms is also often called
 *          "Gift wrapping".
 *
 * \param[in]     points    Set of Points
 *
 * \return        Doubled area (area, multiplied by two).
 */
std::vector<size_t> MakeConvexHull(const std::vector<Point>& points) {
	assert_with_note(!points.empty(), "Points mustn't be empty. Cannot make convex hull of an empty set.");
	
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

	// Сортировка по полярному углу. Детали в B_notes.cpp. Прочитайте, буду рад ответить на вопросы.
	std::sort(
		indices.begin(),
		indices.end(),
		PolarAngleComparator{selected_index, points}
	);
	
	#if DEBUG
		fprintf(stderr, "indices = [");
		for (size_t i = 0; i < indices.size(); ++i) {
			fprintf(stderr, "%zu,", indices[i]);
		}
		fprintf(stderr, "].\n");
	#endif
	
	std::vector<size_t> points_in_hull = {selected_index};
	
	// Замечание 1 из B_notes.cpp. Прочитайте, буду рад ответить на вопросы.
	for (size_t new_point: indices) {
		const Point& p3 = points[new_point];
		while (points_in_hull.size() >= 2) {
			// Замечание 2 из B_notes.cpp. Прочитайте, буду рад ответить на вопросы.
			
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

/**
 * \brief   Calculates oriented area with trapezoidal method.
 *
 * \details Can accept both convex and non-convex polygons.
 *
 * \note    The area is positive for a convex polygon, if
 *          the polygon is enumerated clock-wise.
 *          And negative for a convex-polgon, if it is
 *          enumerated counter clock-wise.
 *
 * \param[in]     a    Polygon vertices.
 *
 * \return        Doubled area (area, multiplied by two).
 */
int64_t CalculateDoubledArea(const std::vector<Point>& polygon) {
	int64_t result = 0;
	for (size_t i = 0; i + 1 < polygon.size(); ++i) {
		const Point& p1 = polygon[i];
		const Point& p2 = polygon[i + 1];
	
		int32_t y1 = p1.y;
		int32_t y2 = p2.y;
		int32_t dx = p2.x - p1.x;
		
		result += static_cast<int64_t>(y1 + y2) * dx;
	}
	{
		const Point& p1 = polygon.back();
		const Point& p2 = polygon.front();
	
		int32_t y1 = p1.y;
		int32_t y2 = p2.y;
		int32_t dx = p2.x - p1.x;
		
		result += static_cast<int64_t>(y1 + y2) * dx;
	}
	
	return result;
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
	
	std::vector<Point> convex_hull_points(convex_hull.size());
	for (size_t i = 0; i < convex_hull.size(); ++i) {
		convex_hull_points[i] = points[convex_hull[i]];
	}
	int64_t doubled_area = CalculateDoubledArea(convex_hull_points);
		
	// Это ориентированная площадь, она положительна при
	//   обходе по часовой (можно представить пример).
	// Потому модуль брать не надо, т.к. функция
	//   построения выпуклой оболочки выдаёт её в
	//   обходе по часовой (так нужно в задаче).
	// Если бы обход был против часовой, нужно было
	//   бы брать.
	/*
	if (result < 0) {
		doubled_area *= -1;
	}
	*/
	assert_with_note(doubled_area > 0, "Area is expected to be positive");
	
	// Доказать, что точности в один знак достаточно можно
	//   с помощью формулы Пика: для многоугольника с
	//   целочисленными координатами вершин есть формула
	//   через количество точек сетки на границах и внутри
	//   фигуры, там есть только деление на два.
	// Потому ответа оканчивается на .0 или .5.
	if (doubled_area % 2 == 0) {
		printf("%" PRId64 ".0\n", doubled_area / 2);
	} else {
		printf("%" PRId64 ".5\n", doubled_area / 2);
	}
	
	return 0;
}