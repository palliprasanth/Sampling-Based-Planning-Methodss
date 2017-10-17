#include <math.h>
#include "mex.h"
#include <iostream>
#include <vector>
#include <list>
#include <random>
#include <iterator>
#include <ctime>
#include <chrono>
#include "graphheader.hpp"
#include "plannerheader.hpp"

using namespace std;

#define GETMAPINDEX(X, Y, XSIZE, YSIZE) (Y*XSIZE + X)

#if !defined(MAX)
#define	MAX(A, B)	((A) > (B) ? (A) : (B))
#endif

#if !defined(MIN)
#define	MIN(A, B)	((A) < (B) ? (A) : (B))
#endif

#define PI 3.141592654

//the length of each link in the arm (should be the same as the one used in runtest.m)
#define LINKLENGTH_CELLS 10

void ContXY2Cell(double x, double y, short unsigned int* pX, short unsigned int *pY, int x_size, int y_size)
{
	double cellsize = 1.0;
	//take the nearest cell
	*pX = (int)(x/(double)(cellsize));
	if( x < 0) *pX = 0;
	if( *pX >= x_size) *pX = x_size-1;

	*pY = (int)(y/(double)(cellsize));
	if( y < 0) *pY = 0;
	if( *pY >= y_size) *pY = y_size-1;
}


void get_bresenham_parameters(int p1x, int p1y, int p2x, int p2y, bresenham_param_t *params)
{
	params->UsingYIndex = 0;

	if (fabs((double)(p2y-p1y)/(double)(p2x-p1x)) > 1)
		(params->UsingYIndex)++;

	if (params->UsingYIndex)
	{
		params->Y1=p1x;
		params->X1=p1y;
		params->Y2=p2x;
		params->X2=p2y;
	}
	else
	{
		params->X1=p1x;
		params->Y1=p1y;
		params->X2=p2x;
		params->Y2=p2y;
	}

	if ((p2x - p1x) * (p2y - p1y) < 0)
	{
		params->Flipped = 1;
		params->Y1 = -params->Y1;
		params->Y2 = -params->Y2;
	}
	else
		params->Flipped = 0;

	if (params->X2 > params->X1)
		params->Increment = 1;
	else
		params->Increment = -1;

	params->DeltaX=params->X2-params->X1;
	params->DeltaY=params->Y2-params->Y1;

	params->IncrE=2*params->DeltaY*params->Increment;
	params->IncrNE=2*(params->DeltaY-params->DeltaX)*params->Increment;
	params->DTerm=(2*params->DeltaY-params->DeltaX)*params->Increment;

	params->XIndex = params->X1;
	params->YIndex = params->Y1;
}

void get_current_point(bresenham_param_t *params, int *x, int *y)
{
	if (params->UsingYIndex)
	{
		*y = params->XIndex;
		*x = params->YIndex;
		if (params->Flipped)
			*x = -*x;
	}
	else
	{
		*x = params->XIndex;
		*y = params->YIndex;
		if (params->Flipped)
			*y = -*y;
	}
}

int get_next_point(bresenham_param_t *params)
{
	if (params->XIndex == params->X2)
	{
		return 0;
	}
	params->XIndex += params->Increment;
	if (params->DTerm < 0 || (params->Increment < 0 && params->DTerm <= 0))
		params->DTerm += params->IncrE;
	else
	{
		params->DTerm += params->IncrNE;
		params->YIndex += params->Increment;
	}
	return 1;
}

int IsValidLineSegment(double x0, double y0, double x1, double y1, double*	map,
	int x_size,
	int y_size)

{
	bresenham_param_t params;
	int nX, nY; 
	short unsigned int nX0, nY0, nX1, nY1;

    //printf("checking link <%f %f> to <%f %f>\n", x0,y0,x1,y1);

	//make sure the line segment is inside the environment
	if(x0 < 0 || x0 >= x_size ||
		x1 < 0 || x1 >= x_size ||
		y0 < 0 || y0 >= y_size ||
		y1 < 0 || y1 >= y_size)
		return 0;

	ContXY2Cell(x0, y0, &nX0, &nY0, x_size, y_size);
	ContXY2Cell(x1, y1, &nX1, &nY1, x_size, y_size);

    //printf("checking link <%d %d> to <%d %d>\n", nX0,nY0,nX1,nY1);

	//iterate through the points on the segment
	get_bresenham_parameters(nX0, nY0, nX1, nY1, &params);
	do {
		get_current_point(&params, &nX, &nY);
		if(map[GETMAPINDEX(nX,nY,x_size,y_size)] == 1)
			return 0;
	} while (get_next_point(&params));

	return 1;
}


int IsValidArmConfiguration(double* angles, int numofDOFs, double*	map,
	int x_size, int y_size)
{
	double x0,y0,x1,y1;
	int i;

 	//iterate through all the links starting with the base
	x1 = ((double)x_size)/2.0;
	y1 = 0;
	for(i = 0; i < numofDOFs; i++)
	{
		//compute the corresponding line segment
		x0 = x1;
		y0 = y1;
		x1 = x0 + LINKLENGTH_CELLS*cos(2*PI-angles[i]);
		y1 = y0 - LINKLENGTH_CELLS*sin(2*PI-angles[i]);

		//check the validity of the corresponding line segment
		if(!IsValidLineSegment(x0,y0,x1,y1,map,x_size,y_size))
			return 0;
	}    
}


double deg2rad(double input){
	return (input*PI)/180.0;
}

double rad2deg(double input){
	return (input*180.0)/PI;
}

double angle_between(double angle1, double angle2, int* move_sign){
	double difference1 = angle1 - angle2;
	double difference2 = angle2 - angle1;
	*move_sign = 1;
	if (difference1 < 0)
		difference1 = difference1 + 2*PI;

	if (difference2 < 0)
		difference2 = difference2 + 2*PI;

	if (difference1 <= difference2){
		*move_sign = -1;
		return difference1;
	}
	else
		return difference2;
}

double angle_between(double angle1, double angle2){
	double difference1 = angle1 - angle2;
	double difference2 = angle2 - angle1;
	if (difference1 < 0)
		difference1 = difference1 + 2*PI;

	if (difference2 < 0)
		difference2 = difference2 + 2*PI;

	if (difference1 <= difference2)
		return difference1;
	else
		return difference2;
}

double get_distance_angular(Node* node1, Node* node2){
	double distance = 0;

	distance = pow(angle_between(node1->theta_1,node2->theta_1),2.0) + pow(angle_between(node1->theta_2,node2->theta_2),2.0) + pow(angle_between(node1->theta_3,node2->theta_3),2.0) + pow(angle_between(node1->theta_4,node2->theta_4),2.0) + pow(angle_between(node1->theta_5,node2->theta_5),2.0);
	distance = sqrt(distance);
	return distance;

}


double get_distance_angular(Node* node1, double* node2_angle, int* move_sign){
	double distance = 0;

	distance = pow(angle_between(node1->theta_1,node2_angle[0], &move_sign[0]),2.0) + pow(angle_between(node1->theta_2,node2_angle[1],&move_sign[1]),2.0) + pow(angle_between(node1->theta_3,node2_angle[2],&move_sign[2]),2.0) + pow(angle_between(node1->theta_4,node2_angle[3],&move_sign[3]),2.0) + pow(angle_between(node1->theta_5,node2_angle[4],&move_sign[4]),2.0);
	distance = sqrt(distance);
	return distance;

}

void build_rrt(double* map, int x_size, int y_size, double* armstart_anglesV_rad, double* armgoal_anglesV_rad,
	int numofDOFs, double*** plan, int* planlength, int* planner_id){
	
	const int max_iter = 10;
	const double epsilon = 0.1;
	double bias_random;
	int cur_node_id, nearest_node_id;
	double nearest_dist;
	double current_sample_angles[5];
	int arm_movement_sign[5]; // To keep track of nearest motion to be clockwise or anti-clockwise
	//no plan by default
	*plan = NULL;
	*planlength = 0;

    //for now just do straight interpolation between start and goal checking for the validity of samples

	double distance = 0;
	int i,j;

	Graph rrt;
	rrt.add_Vertex(armstart_anglesV_rad[0], armstart_anglesV_rad[1], armstart_anglesV_rad[2], armstart_anglesV_rad[3], armstart_anglesV_rad[4]);
	//.add_Vertex(armgoal_anglesV_rad[0], armgoal_anglesV_rad[1], armgoal_anglesV_rad[2], armgoal_anglesV_rad[3], armgoal_anglesV_rad[4]);
	
	rrt.print_Vertices();
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);
	std::uniform_real_distribution<double> bias_distribution(0.0,1.5);
	std::uniform_real_distribution<double> uni_distribution(0.0,1.0);

	for (int i=0; i<max_iter; ++i){
		bias_random = bias_distribution(generator);
		//mexPrintf("bias: %f\n",bias_random);
		if (bias_random > 1){
			nearest_node_id = rrt.getNearestNeighbour(armgoal_anglesV_rad, &nearest_dist, arm_movement_sign);
			if (nearest_dist < epsilon){
				// collision check
				cur_node_id = rrt.add_Vertex_ret_id(armgoal_anglesV_rad[0], armgoal_anglesV_rad[1], armgoal_anglesV_rad[2], armgoal_anglesV_rad[3], armgoal_anglesV_rad[4], nearest_node_id);
				return;
			}

			else{

			}

			mexPrintf("Nearest Node G: %d\n",nearest_node_id);
			mexPrintf("Distance G: %f\n",nearest_dist);
		}
		else{
			for (int j =0; j<numofDOFs; ++j){
				current_sample_angles[j] = 2*PI*uni_distribution(generator);
				//mexPrintf("Uni of %d: %f\n",j+1,current_sample_angles[j]);
			}
			nearest_node_id = rrt.getNearestNeighbour(current_sample_angles, &nearest_dist, arm_movement_sign);
			mexPrintf("Nearest Node : %d\n",nearest_node_id);
			mexPrintf("Distance: %f\n",nearest_dist);
		}
	}
	return;
}