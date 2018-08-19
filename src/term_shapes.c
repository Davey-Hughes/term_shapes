/*
 *     Copyright (C) 2018  Davey Hughes
 *
 *     This program is free software: you can redistribute it and/or modify it
 *     under the terms of the GNU General Public License as published by the
 *     Free Software Foundation, either version 3 of the License, or (at your
 *     option) any later version.
 *
 *     This program is distributed in the hope that it will be useful, but
 *     WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *     General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License along
 *     with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <ncurses.h>

#if TIMING
#include "timing.h"
#endif

#include "term_shapes.h"

/*
 * initialize an object from a file
 *
 * the first line of the file is two comma separated ints describing the number
 * of vertices n, number of edges m, and number of faces k
 *
 * the next n lines are each 3 comma separated floating point values describing
 * the (x, y, z) locations of the vertex in 3D coordinates
 *
 * the next m lines are 2 comma separated positive integer values
 * correspoinding to the indices of two vertices that describe an edge. Each
 * edge only needs to be described once
 *
 * the next k lines are 3 comma separated positive integer values corresponding
 * to the indices of 3 points that make up a face
 */
int
init_from_file(char *fname, struct shape *s)
{
	int err, num_v, num_e, num_f, e0, e1, f0, f1, f2, i;
	double x, y, z;

	FILE *file;
	file = fopen(fname, "r");
	if (file == NULL) {
		fprintf(stderr, "could not open file\n");
		return -1;
	}

	/* read number of vertices and edges */
	err = fscanf(file, "%i, %i, %i", &num_v, &num_e, &num_f);
	if (err == EOF) {
		fprintf(stderr, "Returned EOF when reading first line of shape file\n");
		goto cleanup_file;
	} else if (err == 0) {
		fprintf(stderr, "Zero bytes read when reading first line of shape file\n");
		goto cleanup_file;
	} else if (num_v > MAX_VERTICES) {
		fprintf(stderr, "Number of vertices exceeds max vertices: %d\n", MAX_VERTICES);
	} else if (num_e > MAX_EDGES) {
		fprintf(stderr, "Number of edges exceeds max edges: %d\n", MAX_VERTICES);
	} else if (num_f > MAX_FACES) {
		fprintf(stderr, "Number of faces exceeds max faces: %d\n", MAX_FACES);
	}

	s->num_v = num_v;
	s->num_e = num_e;
	s->num_f = num_f;

	s->vertices = malloc(sizeof(struct point3) * s->num_v);
	if (s->vertices == NULL) {
		goto cleanup_file;
	}

	s->edges = malloc(sizeof(struct edge) * s->num_e);
	if (s->edges == NULL) {
		goto cleanup_vertices;
	}

	s->faces = malloc(sizeof(struct face) * s->num_f);
	if (s->faces == NULL) {
		goto cleanup_edges;
	}

	/* read 3D coordinates describing every vertex */
	for (i = 0; i < num_v; ++i) {
		err = fscanf(file, "%lf, %lf, %lf", &x, &y, &z);
		if (err == EOF) {
			fprintf(stderr, "Returned EOF when reading vertices in shape file\n");
			goto cleanup_faces;
		} else if (err == 0) {
			fprintf(stderr, "Zero bytes read when reading vertices in shape file\n");
			goto cleanup_faces;
		}

		s->vertices[i].x = x;
		s->vertices[i].y = y;
		s->vertices[i].z = z;
	}

	/* read indices of edges described by two vertices */
	for (i = 0; i < num_e; ++i) {
		err = fscanf(file, "%i, %i", &e0, &e1);
		if (err == EOF) {
			fprintf(stderr, "Returned EOF when reading edges in shape file\n");
			goto cleanup_faces;
		} else if (err == 0) {
			fprintf(stderr, "Zero bytes read when reading edges in shape file\n");
			goto cleanup_faces;
		}

		if (e0 < 0 || e0 > num_v - 1 || e1 < 0 || e1 > num_v -1) {
			fprintf(stderr, "Edge index out of bounds\n");
			goto cleanup_faces;
		}

		s->edges[i].edge[0] = e0;
		s->edges[i].edge[1] = e1;
	}

	for (i = 0; i < num_f; ++i) {
		err = fscanf(file, "%i, %i, %i", &f0, &f1, &f2);
		if (err == EOF) {
			fprintf(stderr, "Returned EOF when reading faces in shape file\n");
			goto cleanup_faces;
		} else if (err == 0) {
			fprintf(stderr, "Zero bytes read when reading faces in shape file\n");
			goto cleanup_faces;
		}

		if (f0 < 0 || f0 > num_v - 1 ||
		    f1 < 0 || f1 > num_v - 1 ||
		    f2 < 0 || f2 > num_v - 1) {
			fprintf(stderr, "Face index out of bounds\n");
			goto cleanup_faces;
		}

		s->faces[i].face[0] = f0;
		s->faces[i].face[1] = f1;
		s->faces[i].face[2] = f2;
	}

	fclose(file);

	s->center = (struct point3) {0.0, 0.0, 0.0};
	s->fname = fname;

	/*
	 * by default, vertices are not drawn directly, but if no edges are
	 * described by the input file, the vertices will be drawn by the
	 * function print_vertices()
	 */
	s->print_vertices = 1;
	if (s->num_e) {
		s->print_vertices = 0;
	}

	s->print_edges = 1;

	s->e_density = E_DENSITY;
	s->occlusion = NONE;
	s->cop = (struct point3) COP;

	return 0;

cleanup_faces:
	free(s->faces);
cleanup_edges:
	free(s->edges);
cleanup_vertices:
	free(s->vertices);
cleanup_file:
	fclose(file);
	return -1;
}

/*
 * free memory allocated for shape
 */
void
destroy_shape(struct shape *s)
{
	free(s->vertices);
	free(s->edges);
	free(s->faces);
}

/*
 * default shape if no file is given
 */
int
init_cube(struct shape *c)
{
	c->vertices = malloc(sizeof(struct point3) * 8);
	if (c->vertices == NULL) {
		return -1;
	}

	c->edges = malloc(sizeof(struct edge) * 12);
	if (c->edges == NULL) {
		goto cleanup_verts;
	}

	c->faces = malloc(sizeof(struct edge) * 1);
	if (c->faces == NULL) {
		goto cleanup_edges;
	}

	c->vertices[0] = (struct point3) {1.0, 1.0, 1.0};
	c->vertices[1] = (struct point3) {-1.0, 1.0, 1.0};
	c->vertices[2] = (struct point3) {1.0, -1.0, 1.0};
	c->vertices[3] = (struct point3) {-1.0, -1.0, 1.0};
	c->vertices[4] = (struct point3) {1.0, 1.0, -1.0};
	c->vertices[5] = (struct point3) {-1.0, 1.0, -1.0};
	c->vertices[6] = (struct point3) {1.0, -1.0, -1.0};
	c->vertices[7] = (struct point3) {-1.0, -1.0, -1.0};

	c->edges[0] = (struct edge) {{0, 1}};
	c->edges[1] = (struct edge) {{0, 2}};
	c->edges[2] = (struct edge) {{0, 4}};
	c->edges[3] = (struct edge) {{1, 3}};
	c->edges[4] = (struct edge) {{1, 5}};
	c->edges[5] = (struct edge) {{2, 3}};
	c->edges[6] = (struct edge) {{2, 6}};
	c->edges[7] = (struct edge) {{3, 7}};
	c->edges[8] = (struct edge) {{4, 5}};
	c->edges[9] = (struct edge) {{4, 6}};
	c->edges[10] = (struct edge) {{5, 7}};
	c->edges[11] = (struct edge) {{6, 7}};

	c->num_v = 8;
	c->num_e = 12;
	c->num_f = 0;
	c->center = (struct point3) {0.0, 0.0, 0.0};
	c->fname = NULL;
	c->print_vertices = 0;
	c->print_edges = 1;
	c->e_density = E_DENSITY;
	c->occlusion = NONE;
	c->cop = (struct point3) COP;

	return 0;

cleanup_edges:
	free(c->edges);
cleanup_verts:
	free(c->vertices);
	return -1;
}

/*
 * translate an x and y value based on the window dimensions and some magic
 * numbers so an object described with a "radius" approximately 1 will be
 * centered in the center of the screen and entirely fit on the screen
 */
void
movexy(double *x, double *y)
{
	int winx, winy;
	double movex, movey;

	getmaxyx(stdscr, winy, winx);

	movex = (int) ((*x * SCALE * winy) + (0.5 * winx));
	movey = (int) (-(*y * SCALE * .5 * winy) + (0.5 * winy));

	*x = movex;
	*y = movey;
}

/*
 * first approximation of occluding points
 *
 * operates by finding the angle between the vectors given by the point to the
 * center of the object and the point to the center of projection. This is only
 * an approximation as the angle given by this calculation doesn't determine
 * entirely whether a point should be occluded. The threshold is given by pi/3,
 * but as this is approximate it can be adjusted.
 *
 * returns 0 if point should be rendered, else 1
 */
int
occlude_point_approx(struct shape s, struct point3 point)
{
	double x0, y0, z0, x1, y1, z1, dprod, mag0, mag1, theta;

	/* vector from point to the center of solid */
	x0 = s.center.x - point.x;
	y0 = s.center.y - point.y;
	z0 = s.center.z - point.z;

	/* vector from point to the center of projection */
	x1 = s.cop.x - point.x;
	y1 = s.cop.y - point.y;
	z1 = s.cop.z - point.z;

	/*
	 * angle between two vectors is given by the arc cosine of the dot
	 * product of the two vectors divided by the product of magnitudes of
	 * the vectors
	 */
	dprod = (x0 * x1) + (y0 * y1) + (z0 * z1);
	mag0 = sqrt((x0 * x0) + (y0 * y0) + (z0 * z0));
	mag1 = sqrt((x1 * x1) + (y1 * y1) + (z1 * z1));

	theta = acos(dprod / (mag0 * mag1));

	if (theta < M_PI / 3) {
		return 1;
	}

	return 0;
}

/*
 * occlusion method that works for convex shapes
 *
 * returns 0 if point should be rendered, else 1
 */
int
occlude_point_convex(struct shape s, struct point3 point)
{
	int i;
	double x0, y0, z0, x1, y1, z1, x2, y2, z2, d, t, dist0, dist1;
	struct point3 u, v0, v1, n, inter;

	/*
	 * iterate through every face in the shape and determine whether the
	 * ray between the center of projection and input point intersects a
	 * face
	 *
	 * note: precomputing the equations for the faces would speed this up
	 */
	for (i = 0; i < s.num_f; ++i) {
		/*
		 * two vectors from the points that define the plane-face of
		 * the object
		 */
		u = s.vertices[s.faces[i].face[0]];
		/* u = (struct point3) {1, -2, 0}; */
		x0 = u.x;
		y0 = u.y;
		z0 = u.z;

		u = s.vertices[s.faces[i].face[1]];
		/* u = (struct point3) {3, 1, 4}; */
		x1 = u.x;
		y1 = u.y;
		z1 = u.z;

		u = s.vertices[s.faces[i].face[2]];
		/* u = (struct point3) {0, -1, 2}; */
		x2 = u.x;
		y2 = u.y;
		z2 = u.z;

		v0.x = x0 - x1;
		v0.y = y0 - y1;
		v0.z = z0 - z1;

		v1.x = x0 - x2;
		v1.y = y0 - y2;
		v1.z = z0 - z2;

		/* n is the cross product of the two vectors */
		n.x = (v0.y * v1.z) - (v0.z * v1.y);
		n.y = (v0.z * v1.x) - (v0.x * v1.z);
		n.z = (v0.x * v1.y) - (v0.y * v1.x);

		/*
		 * with the parameterized equation of the plane given as:
		 * 	ax + by + cz = d
		 * a is the value n.x, b is the value n.y, z is the value n.z
		 * and d is given by solving ax + by + cz = 0, where x, y, and
		 * z are the x, y, and z from any one of the intial points
		 */
		d = (n.x * x0) + (n.y * y0) + (n.z * z0);

		/* TODO: determine if line is parallel to plane or not */

		/*
		 * the intersection of the line between the point we're
		 * evaluating and the center of projection with the face-plane
		 * is given by finding the parametric form of the line (where p
		 * is the point we're evaluating and cop is the center of
		 * projection point:
		 * 	r(t) = <x_p, y_p, z_p> + t<x_cop - x_p, y_cop - y_p, z_cop - z_p>
		 * After substituting these points in, we can then solve for z
		 * by plugging in the parametric form of the line to our
		 * equation of the plane.
		 */

		t = (d - (n.x * point.x + n.y * point.y + n.z * point.z)) /
			(n.x * (s.cop.x - point.x) +
			 n.y * (s.cop.y - point.y) +
			 n.z * (s.cop.z - point.z));

		/* inter is the intersection point */
		inter.x = (point.x + (t * (s.cop.x - point.x)));
		inter.y = (point.y + (t * (s.cop.y - point.y)));
		inter.z = (point.z + (t * (s.cop.z - point.z)));

		/*
 		 * APPROXIMATION
 		 *
		 * if the intersection point is far away from the shape, don't
		 * consider it.
		 *
		 * Far away is defined as being further away from center of the
		 * shape than a point on the shape. (Should be from the
		 * furthest away point, but this is currently not implemeted)
		 */

		/* distance between center and intersection */
		x0 = s.center.x - inter.x;
		y0 = s.center.y - inter.y;
		z0 = s.center.z - inter.z;
		dist0 = sqrt((x0 * x0) + (y0 * y0) + (z0 * z0));

		/* distance between center and a point on the shape */
		x1 = s.center.x - s.vertices[0].x;
		y1 = s.center.y - s.vertices[0].y;
		z1 = s.center.z - s.vertices[0].z;
		dist1 = sqrt((x1 * x1) + (y1 * y1) + (z1 * z1));

		/* fprintf(stderr, "%lf, %lf, %lf %lf, %lf\n", point.x, point.y, point.z, dist0, dist1); */

		if (dist0 > dist1) {
			continue;
		}

		/*
		 * if the distance between the intersection point and the cop
		 * is smaller than the distance between the point we're
		 * evaluating and the cop, the point should be occluded (not
		 * rendered)
		 */

		/* distance between cop and intersection */
		x0 = s.cop.x - inter.x;
		y0 = s.cop.y - inter.y;
		z0 = s.cop.z - inter.z;
		dist0 = sqrt((x0 * x0) + (y0 * y0) + (z0 * z0));

		/* distance between cop and point we're evaluating */
		x1 = s.cop.x - point.x;
		y1 = s.cop.y - point.y;
		z1 = s.cop.z - point.z;
		dist1 = sqrt((x1 * x1) + (y1 * y1) + (z1 * z1));

		if (dist0 < dist1) {
			return 1;
		}
	}

	return 0;
}

/*
 * chooses which occlusion method to use based on the s.occlusion enum
 *
 * returns 0 if point should be rendered, else 1
 */
int
occlude_point(struct shape s, struct point3 point)
{
	switch (s.occlusion) {
	case NONE:
		return 1;

	case APPROX:
		return occlude_point_approx(s, point);

	case CONVEX:
		return occlude_point_convex(s, point);

	case EXACT: /* not implemented */
		return 0;
	}

	return 0;
}

/*
 * prints the edges by calculating the normal vector from two given points, and
 * then prints points along the edge by multiplying the normal
 */
void
print_edges(struct shape s)
{
	int i, k;
	double x0, y0, z0, x1, y1, z1, v_len, x, y, z, movex, movey;
	struct point3 v, u;

	/* iterates over the edges */
	for (i = s.num_e - 1; i >= 0; --i) {
		v = s.vertices[s.edges[i].edge[0]];
		x0 = v.x;
		y0 = v.y;
		z0 = v.z;

		v = s.vertices[s.edges[i].edge[1]];
		x1 = v.x;
		y1 = v.y;
		z1 = v.z;

		/* v is the vector given by two points */
		v = (struct point3) {x1 - x0, y1 - y0, z1 - z0};
		v_len = sqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z));

		/* u is the normal vector from v */
		u = (struct point3) {v.x / v_len, v.y / v_len, v.z / v_len};

		/*
		 * prints points along the edge
		 *
		 * e_density is a natural number directly corresponding to the
		 * number of points printed along the edge
		 */
		for (k = 0; k <= s.e_density; ++k) {
			x = x0 + ((k / (float) s.e_density * v_len) * u.x);
			y = y0 + ((k / (float) s.e_density * v_len) * u.y);
			z = z0 + ((k / (float) s.e_density * v_len) * u.z);

			/*
			 * if the occlusion flag is set and a point shouldn't
			 * be occluded, the rest of the loop prints the point
			 */
			if (s.occlusion && occlude_point(s, (struct point3) {x, y, z})) {
				continue;
			}

			movex = x;
			movey = y;
			movexy(&movex, &movey);

			/*
			 * to give a sense of distance and since each object
			 * starts centered around the origin, points that have
			 * a negative z value (therefore are "behind" the
			 * xy-plane) are printed with a "." instead of a "o"
			 */
			if (z < 0) {
				mvprintw(movey, movex, "%s", ".");
			} else {
				mvprintw(movey, movex, "%s", "o");
			}
		}
	}
}

/*
 * prints the vertices with their index (to make connecting edges easier)
 *
 * vertices are printed backwards so that lower numbered indices are printed in
 * front of the higher number indices if they overlap
 */
void
print_vertices(struct shape s)
{
	int i;
	double x, y, z;

	for (i = s.num_v - 1; i >= 0; --i) {
		x = s.vertices[i].x;
		y = s.vertices[i].y;
		z = s.vertices[i].z;

		if (s.occlusion && occlude_point(s, (struct point3) {x, y, z})) {
			continue;
		}

		movexy(&x, &y);
		mvprintw(y, x, "%i", i);
	}
}

/*
 * only prints edges if edges are stored in the shape struct
 */
void
print_shape(struct shape s)
{
	if (s.print_edges && s.num_e) {
		print_edges(s);
	}

	if (s.print_vertices) {
		print_vertices(s);
	}
}

/*
 * rotates each point in one of 6 directions, given the angle (can be positive
 * or negative) and the axis around which to rotate
 */
void
rotate_shape(double theta, char axis, struct shape *s)
{
	int i;
	double *fst, *snd;
	double presin, precos, cfst, csnd, tempfst, tempsnd;

	/* get center values and test whether axis in 'xyz' */
	switch (axis) {
	case 'x':
		cfst = s->center.y;
		csnd = s->center.z;
		break;
	case 'y':
		cfst = s->center.z;
		csnd = s->center.x;
		break;
	case 'z':
		cfst = s->center.x;
		csnd = s->center.y;
		break;
	default:
		return;
	}

	/* precompute sin theta and cos theta */
	presin = sin(theta);
	precos = cos(theta);

	/* change every point */
	for (i = 0; i < s->num_v; ++i) {
		switch (axis) {
		case 'x':
			fst = &(s->vertices[i].y);
			snd = &(s->vertices[i].z);
			break;
		case 'y':
			fst = &(s->vertices[i].z);
			snd = &(s->vertices[i].x);
			break;
		case 'z':
			fst = &(s->vertices[i].x);
			snd = &(s->vertices[i].y);
			break;
		default: /* shouldn't get here */
			exit(1);
		}

		tempfst = *fst - cfst;
		tempsnd = *snd - csnd;

		*fst = tempfst * precos - tempsnd * presin;
		*snd = tempsnd * precos + tempfst * presin;

		*fst += cfst;
		*snd += csnd;
	}
}

/*
 * scale the object by a given magnitude
 */
void
scale_shape(double mag, struct shape *s)
{
	int i;

	for (i = 0; i < s->num_v; ++i) {
		s->vertices[i].x -= s->center.x;
		s->vertices[i].y -= s->center.y;
		s->vertices[i].z -= s->center.z;

		s->vertices[i].x *= mag;
		s->vertices[i].y *= mag;
		s->vertices[i].z *= mag;

		s->vertices[i].x += s->center.x;
		s->vertices[i].y += s->center.y;
		s->vertices[i].z += s->center.z;
	}
}

/*
 * translates each vertex given a distance and the axis which to translate
 * along
 */
void
translate_shape(double dist, char axis, struct shape *s)
{
	int i;

	switch (axis) {
	case 'x':
		s->center.x += dist;
		break;
	case 'y':
		s->center.y += dist;
		break;
	case 'z':
		s->center.z += dist;
		break;
	default:
		return;
	}

	for (i = 0; i < s->num_v; ++i) {
		switch (axis) {
		case 'x':
			s->vertices[i].x += dist;
			printw("%f\n", s->vertices[i].y);
			break;
		case 'y':
			s->vertices[i].y += dist;
			break;
		case 'z':
			s->vertices[i].z += dist;
			break;
		default: /* shouldn't get here */
			return;
		}
	}
}

/*
 * completely resets the shape, either by the filename originally given or the
 * default, hardcoded cube
 */
int
reset_shape(struct shape *s)
{
	destroy_shape(s);
	if (s->fname != NULL) {
		return init_from_file(s->fname, s);
	}

	return init_cube(s);
}

/*
 * loop which re-prints the shape with every keypress, and checks for certain
 * keyboard input to determine functions to run on the shape
 */
static
void
loop(struct shape *s)
{
	int c;
	double theta, dist, scale;
	char *occlusion_type = "";

#if TIMING
	struct timespec start, end, diff, avg_op, avg_print;

	avg_op.tv_sec = -1;
	avg_op.tv_nsec = -1;

	avg_print.tv_sec = -1;
	avg_print.tv_nsec = -1;
#endif

	/* start ncurses mode */
	initscr();
	noecho();
	cbreak();
	keypad(stdscr, TRUE);
	curs_set(0);

	theta = M_PI / 200;
	dist = 0.1;
	scale = 1.1;

#if TIMING
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
#endif

	while(1) {
		wclear(stdscr);

		switch (s->occlusion) {
		case NONE:
			occlusion_type = "none";
			break;

		case APPROX:
			occlusion_type = "approximate";
			break;

		case CONVEX:
			occlusion_type = "convex";
			break;

		case EXACT:
			occlusion_type = "exact not implemented";
			break;
		}

		mvprintw(1, 1, "Occlusion type: %s", occlusion_type);


#if TIMING
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
		timespec_diff(&start, &end, &diff);
		mvprintw(3, 1, "Operation time: %ld.%06ld seconds\n",
			diff.tv_sec, diff.tv_nsec / 1000);

		timespec_avg(&avg_op, &diff, &avg_op);

		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
#endif

		print_shape(*s);

#if TIMING
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);

		timespec_diff(&start, &end, &diff);
		mvprintw(2, 1, "Print time: %ld.%06ld seconds\n",
			diff.tv_sec, diff.tv_nsec / 1000);

		timespec_avg(&avg_print, &diff, &avg_print);
#endif

		c = getch();

#if TIMING
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
#endif

		switch(c) {
		case 'q':
			/* end ncurses mode */
			endwin();

#if TIMING
			printf("Average operation time: %ld.%06ld seconds\n",
				avg_op.tv_sec, avg_op.tv_nsec / 1000);

			printf("Average print time: %ld.%06ld seconds\n",
				avg_print.tv_sec, avg_print.tv_nsec / 1000);
#endif

			return;

		/* ** ROTATIONS ** */
		/* rotate around z axis */
		case 't':
			rotate_shape(theta, 'z', s);
			break;
		case 'y':
			rotate_shape(-theta, 'z', s);
			break;

		/* rotate around x axis */
		case 'u':
			rotate_shape(theta, 'x', s);
			break;
		case 'i':
			rotate_shape(-theta, 'x', s);
			break;

		/* rotate around y axis */
		case 'o':
			rotate_shape(-theta, 'y', s);
			break;
		case 'p':
			rotate_shape(theta, 'y', s);
			break;

		/* ** SCALE ** */
		case '=':
			scale_shape(scale, s);
			break;

		case '-':
			scale_shape(1.0 / scale, s);
			break;


		/* ** TRANSLATIONS ** */
		/* translate along x axis */
		case 'h':
			translate_shape(-dist, 'x', s);
			break;
		case 'l':
			translate_shape(dist, 'x', s);
			break;


		/* translate along y axis */
		case 'j':
			translate_shape(-dist, 'y', s);
			break;

		case 'k':
			translate_shape(dist, 'y', s);
			break;

		/* translate along z axis */
		case 'f':
			translate_shape(-dist, 'z', s);
			break;
		case 'g':
			translate_shape(dist, 'z', s);
			break;


		/* RESET */
		case 'r':
			reset_shape(s);
			break;

		/* flip printing of vertices */
		case '1':
			s->print_vertices = !(s->print_vertices);
			break;

		/* flip printing of edges */
		case '2':
			s->print_edges = !(s->print_edges);
			break;

		/* turn occlusion on or off */
		case '3':
			s->occlusion++;
			if (s->occlusion > EXACT) {
				s->occlusion = NONE;
			}

			break;

		/* **CHANGE EDGE DENSITY** */
		/* increase edge density */
		case '0':
			s->e_density++;
			break;

		/* decrease edge density */
		case '9':
			if (s->e_density > 0) {
				s->e_density--;
			}
			break;

		default:
			continue;
		}
	}
}

int
main(int argc, char **argv)
{
	int err;
	struct shape s;

	if (argc > 1) {
		err = init_from_file(argv[1], &s);
	} else {
		err = init_cube(&s);
	}
	if (err != 0) {
		printf("error allocating cube\n");
		exit(1);
	}

	loop(&s);

	destroy_shape(&s);

	return 0;
}
