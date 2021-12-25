#include "shape.hh"

#include <iostream>
#include <fstream>
#include <sstream>

#include <Eigen/Dense>
#include <Eigen/StdVector>

namespace TS {
Shape::Shape(const char* fname)
{
	std::ifstream f(fname);
	if (!f) {
		std::cerr << "File \""
			  << fname
			  << "\" could not be opened for reading"
			  << std::endl;
		exit(2);
	}

	/* read over shape size descriptor (for C implementation) */
	read_block<int>(f);

	/* read list of vertices */
	std::vector<std::vector<double>> points = read_block<double>(f);
	for (auto v: points) {
		this->vertices.push_back(Eigen::Map<Eigen::Vector3d>(v.data()));
	}

	/* read list of edges */
	std::vector<std::vector<size_t>> edges = read_block<size_t>(f);
	for (auto e: edges) {
		if (e.size() != 2) {
			std::cerr << "Edges must contain exactly 2 indices" << std::endl;
			exit(3);
		}

		edge cur_edge = {
			&(this->vertices[e[0]]),
			&(this->vertices[e[1]])
		};

		this->edges.push_back(cur_edge);
	}

	/* read list of faces */
	std::vector<std::vector<size_t>> faces = read_block<size_t>(f);
	for (auto face: faces) {
		struct face cur_face;

		for (auto p: face) {
			cur_face.vertices.push_back(&(this->vertices[p]));
		}

		/* TODO: determine whether normal should be calculated here */

	}

	f.close();

	this->fname = fname;

	(void) this->e_density;
}

template <typename T>
std::vector<std::vector<T>>
Shape::read_block(std::ifstream& f)
{
	std::vector<std::vector<T>> vec;

	std::string line;
	while (std::getline(f, line)) {
		if (line.empty())
			break;

		std::string temp;
		std::vector<T> nums;
		std::istringstream iss(line);
		while (std::getline(iss, temp, ',')) {
			nums.push_back(std::stod(temp));
		}

		vec.push_back(nums);
	}

	return vec;
}
}