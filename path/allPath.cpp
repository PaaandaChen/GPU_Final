#include "allPath.h"

#include <iostream>
#include <cmath>

using namespace std;
using namespace cv;
using namespace cv::cuda;

typedef vector<Mat> Path;

#define PI 3.1415926f
#define gaussian(x, mean, var) exp(-((x-mean)/var)*((x-mean)/var)/2.f)/(var*sqrt(2*PI))
#define gaussianD(dis, var) exp(-(dis/var)*(dis/var)/2.f)/(var*sqrt(2*PI))

allPath::allPath(int height, int width, int t)
{
	cout<<"allallpath init...."<<endl;
	this->height = height;
	this->width = width;
	this->time = t;

	vector<vector<Path> > cellPath(width, vector<Path> (height));
	vector<vector<Path> > cellHomo(width, vector<Path> (height));
	vector<vector<Path> > warpHomo(width, vector<Path> (height));
	vector<vector<Path> > optPath(width, vector<Path> (height));
	vector<vector<Path> > tmpPath(width, vector<Path> (height));
	vector<vector<Path> > BPath(width, vector<Path> (height));

	/* Panda's Code
	vector<vector<Path> > BPath(width, vector<Path> (height));
	//*/ 

	for (int i = 0; i < width; i++)
		for (int j = 0; j < height; j++)
		{
			warpHomo[i][j] = vector<Mat>(this->time);
			optPath[i][j] = vector<Mat>(this->time);
			tmpPath[i][j] = vector<Mat>(this->time);
			cellPath[i][j] = vector<Mat>(this->time);
			BPath[i][j] = vector<Mat>(this->time);
			cellHomo[i][j] = vector<Mat>(this->time-1);
			/* Panda's Code
			BPath[i][j] = vector<Mat>(this->time);
			//*/

			for (int t = 0; t < this->time; t++)
			{
				warpHomo[i][j][t] = Mat::eye(3, 3, CV_32FC1);
				cellPath[i][j][t] = Mat::eye(3, 3, CV_32FC1);
				optPath[i][j][t] = Mat::eye(3, 3, CV_32FC1);
				BPath[i][j][t] = Mat::eye(3, 3, CV_32FC1);
				tmpPath[i][j][t] = Mat::eye(3, 3, CV_32FC1);
				/* Panda's Code
				BPath[i][j][t] = Mat::eye(3, 3, CV_32FC1);
				//*/
				if (t < this->time-1)
				{
					cellHomo[i][j][t] = Mat::eye(3, 3, CV_32FC1);	
				}
			}
		}

	this->cellPath = cellPath;
	this->cellHomo = cellHomo;
	this->warpHomo = warpHomo;
	this->optPath = optPath;
	this->tmpPath = tmpPath;
	/* Panda's Code
	this->BPath = BPath;
	//*/
}

allPath::allPath(int height, int width, int t, vector<vector<vector<Point2f>>> allCellPts)
{
	cout << "allPath init...";
	this->height = height;
	this->width = width;
	this->time = t;

	vector<vector<PtsPath> > cellPoints(width, vector<PtsPath> (height));
	vector<vector<PtsPath> > optPoints(width, vector<PtsPath> (height));
	vector<vector<PtsPath> > tmpPoints(width, vector<PtsPath> (height));
	vector<vector<Path> > BPath(width, vector<Path> (height));

	/* Panda's Code
	vector<vector<Path> > BPath(width, vector<Path> (height));
	//*/ 

	for (int i = 0; i < width; i++)
		for (int j = 0; j < height; j++)
		{
			cellPoints[i][j] = PtsPath(this->time);
			optPoints[i][j] = PtsPath(this->time);
			tmpPoints[i][j] = PtsPath(this->time);
			BPath[i][j] = vector<Mat>(this->time);

			for (int t = 0; t < this->time; t++)
			{
				cellPoints[i][j][t] = allCellPts[t][i][j];
				optPoints[i][j][t] = allCellPts[t][i][j];
				tmpPoints[i][j][t] = allCellPts[t][i][j];
				BPath[i][j][t] = Mat::eye(3, 3, CV_32FC1);
			}
		}

	this->cellPoints = cellPoints;
	this->tmpPoints = tmpPoints;
	this->optPoints = optPoints;
	this->BPath = BPath;
	cout << "done" << endl;
}

allPath::~allPath(){}

void allPath::setHomo(int i, int j, int t, Vec9f p)
{
	if (i >= width || j >= height || t >= time-1 || i < 0 || j < 0 || t < 0)
		throw runtime_error("allPath::setHomo: index can only inside the cell.\n");
	cellHomo[i][j][t].at<float>(0, 0) = p[0];
	cellHomo[i][j][t].at<float>(0, 1) = p[1];
	cellHomo[i][j][t].at<float>(0, 2) = p[2];
	cellHomo[i][j][t].at<float>(1, 0) = p[3];
	cellHomo[i][j][t].at<float>(1, 1) = p[4];
	cellHomo[i][j][t].at<float>(1, 2) = p[5];
	cellHomo[i][j][t].at<float>(2, 0) = p[6];
	cellHomo[i][j][t].at<float>(2, 1) = p[7];
	cellHomo[i][j][t].at<float>(2, 2) = p[8];
}


void allPath::setHomo(int i, int j, int t, Mat h)
{
	if (i >= width || j >= height || t >= time-1 || i < 0 || j < 0 || t < 0)
		throw runtime_error("allPath::setHomo: index can only inside the cell.\n");
	cellHomo[i][j][t] = h.clone();
}

void allPath::setPath(int i, int j, Path p)
{
	if (i >= width || j >= height || i < 0 || j < 0)
		throw runtime_error("allPath::setPath: index can only inside the cell.\n");
	if (p.size() < time)
		throw runtime_error("allPath::setPath: p time must big enough.\n");
	cellPath[i][j] = p;
}

void allPath::computePath()
{
	for (int i = 0; i < width; i++)
		for (int j = 0; j < height; j++)
		{
			cellPath[i][j][0] = cellHomo[i][j][0];
			// cellPath[i][j][0] = Mat::eye(3, 3, CV_32FC1);
			for (int t = 1; t < time-1; t++)
			{
				cellPath[i][j][t] = cellHomo[i][j][t-1] * cellPath[i][j][t-1];
				optPath[i][j][t] = cellPath[i][j][t].clone();
				tmpPath[i][j][t] = cellPath[i][j][t].clone();
			}
		}

}

void allPath::computePathOnly30Frames()
{
	for (int i = 0; i < width; i++)
		for (int j = 0; j < height; j++)
		{
			cellPath[i][j][0] = Mat::eye(3, 3, CV_32FC1);
			for (int t = 1; t < time; t++)
			{
				int T = max(t-30, 0);
				Mat h = cellHomo[i][j][T].inv();
				cellPath[i][j][t] = cellHomo[i][j][t-1] * cellPath[i][j][t-1] * h;
				optPath[i][j][t] = cellPath[i][j][t].clone();
				tmpPath[i][j][t] = cellPath[i][j][t].clone();
			}
		}

}

void allPath::computePath40FramesWithWeight()
{
	Mat E = Mat::eye(3, 3, CV_32FC1);
	for (int i = 0; i < width; i++)
		for (int j = 0; j < height; j++)
		{			
			for (int t = 0; t < time; t++)
			{
				cellPath[i][j][t] = E.clone();
				for (int T = t-1; T >= max(t-30, 0); T--)
					cellPath[i][j][t] = cellPath[i][j][t] * cellHomo[i][j][T];

				for (int k = t-30-1; k >= max(t-30-10, 0); k--)
				{
					float u = 0.1f*(k-(t-30)+10);
					cellPath[i][j][t] = cellPath[i][j][t] * (u*cellHomo[i][j][k] + (1-u)*E);
				}
				optPath[i][j][t] = cellPath[i][j][t].clone();
				tmpPath[i][j][t] = cellPath[i][j][t].clone();
			}
		}

}

void allPath::computeWarp()
{
	for (int i = 0; i < width; i++)
		for (int j = 0; j < height; j++)
			for (int t = 0; t < time; t++)
				warpHomo[i][j][t] = optPath[i][j][t]*cellPath[i][j][t].inv();
		
}

Mat allPath::getWarpHomo(int i, int j, int t)
{
	if (i >= width || j >= height || t >= time || i < 0 || j < 0 || t < 0)
		throw runtime_error("allPath::getWarpHomo: index can only inside the cell.\n");
	return warpHomo[i][j][t];
}

Mat allPath::getPath(int i, int j, int t)
{
	if (i >= width || j >= height || t >= time || i < 0 || j < 0 || t < 0)
		throw runtime_error("allPath::getWarpHomo: index can only inside the cell.\n");
	return cellPath[i][j][t];
}

Path allPath::getPath(int i, int j)
{
	if (i >= width || j >= height || i < 0 || j < 0)
		throw runtime_error("allPath::getPath: index can only inside the cell.\n");
	return cellPath[i][j];
}

BundleHomo allPath::getHomo(int t)
{
	if (t >= time || t < 0)
		throw runtime_error("allPath::getPath: index can only inside the cell.\n");

	vector<vector<Mat> > path(width, vector<Mat> (height));
	for (int i = 0; i < width; i++)
		for (int j = 0; j < height; j++)
		{
			if (t > 0)
				path[i][j] = cellHomo[i][j][t-1].clone();
			else
				path[i][j] = Mat::eye(3, 3, cellHomo[0][0][0].type());
		}
	return path;
}

BundleHomo allPath::getbHomo(int t)
{
	if (t >= time || t < 0)
		throw runtime_error("allPath::getPath: index can only inside the cell.\n");

	vector<vector<Mat> > path(width, vector<Mat> (height));
	for (int i = 0; i < width; i++)
		for (int j = 0; j < height; j++)
		{
				path[i][j] = BPath[i][j][t].clone();
		}
	return path;
}

BundleHomo allPath::getPath(int t)
{
	if (t >= time || t < 0)
		throw runtime_error("allPath::getPath: index can only inside the cell.\n");

	vector<vector<Mat> > path(width, vector<Mat> (height));
	for (int i = 0; i < width; i++)
		for (int j = 0; j < height; j++)
			path[i][j] = cellPath[i][j][t].clone();
	return path;
}

BundleHomo allPath::getOptimizedPath(int t)
{
	if (t >= time || t < 0)
		throw runtime_error("allPath::getOptimizedPath: index can only inside the cell.\n");

	vector<vector<Mat> > path(width, vector<Mat> (height));
	for (int i = 0; i < width; i++)
		for (int j = 0; j < height; j++)
			path[i][j] = optPath[i][j][t].clone();
	return path;
}

Path allPath::getOptimizedPath(int i, int j)
{
	if (i >= width || j >= height || i < 0 || j < 0)
		throw runtime_error("allPath::getOptimizedPath: index can only inside the cell.\n");
	return optPath[i][j];
}

void allPath::optimizePath(int iter)
{
	float w = .1f;

	printf("Optimizing path: \n");
	for (int k = 0; k < iter; k++)
	{
		printf("\titer %d \n", k);
		int sta_t, end_t;
		for (int t = 0; t < time; t++)
		{
			sta_t = t-30 < 0 ? 0 : t-30;
			end_t = t+30 > time-1 ? time-1 : t+30;

			int sta_i, end_i, sta_j, end_j;
			for (int i = 0; i < width; i++)
				for (int j = 0; j < height; j++)
				{

					sta_i = i-1 < 0 ? 0 : i-1;
					end_i = i+1 > width-1 ? width-1 : i+1;
					sta_j = j-1 < 0 ? 0 : j-1;
					end_j = j+1 > height-1 ? height-1 : j+1;
					
					int num = 0;
					float weight = 0.f;
					Mat sum = Mat::zeros(3, 3, CV_32FC1);
					for (int T = sta_t; T <= end_t; T++)
						for (int I = sta_i; I <= end_i; I++)
							for (int J = sta_j; J <= end_j; J++)
							{
								// cout << gaussian(T, t, 10)*gaussian(I, i, 1)*gaussian(J, j, 1) << "\n";
								weight += gaussian(float(T), float(t), 10.f)*gaussian(float(I), float(i), 1)*gaussian(float(J), float(j), 1);
								sum += gaussian(float(T), float(t), 10)*gaussian(float(I), float(i), 1)*gaussian(float(J), float(j), 1)*tmpPath[I][J][T];
							}

					sum += w*cellPath[i][j][t];
					weight += w;
					sum /= weight;
					optPath[i][j][t] = sum;
					tmpPath[i][j][t] = optPath[i][j][t].clone();
				}
			
		}
	}
}

// use jacobi solver to optimize path
void allPath::jacobiSolver(int iter)
{
	float lambda = 5.0; // TODO: need to optimize
	float cellsize = height*width;
	cout << "iter time:" << iter << endl;
	for(int it = 0; it < iter; it++)
	{
		for(int t = 0; t < time; t++)
		{
			int sta_t, end_t;
			sta_t = t-30 < 0 ? 0 : t-30;
			end_t = t+30 > time-1 ? time-1 : t+30;
			int num_omega = end_t - sta_t + 1;
			float w[num_omega] = {0.0};
			float w_sum = 0.0;

			// calc weights
			for (int r = sta_t; r <= end_t; r++)
			{
				// if(r == t)
				// {
				// 	w[r-sta_t] = 0.0;
				// 	continue;
				// }
				float trans =0.f;

				for (int i = 0; i < width; i++)
					for (int j = 0; j < height; j++)
					{
						trans += abs(cellPath[i][j][r].at<float>(0,2) - cellPath[i][j][t].at<float>(0,2)) + 
								abs(cellPath[i][j][r].at<float>(1,2) - cellPath[i][j][t].at<float>(1,2));
					}
				trans = trans / cellsize;
				// cout<<"trans = "<<trans<<endl;
				w[r-sta_t] = 10*gaussianD(float(r-t), 10.f)*gaussianD(trans, 10.f);
				// cout<<"w[r] = "<<w[r-sta_t]<<endl;
				w_sum += w[r-sta_t];
			}

			// each cell
			for (int i = 0; i < width; i++)
				for (int j = 0; j < height; j++)
				{
					// 1st cons
					// tmpPath[i][j][t] = cellPath[i][j][t].clone();
					// 2nd cons
					Mat cons2 = Mat::zeros(3,3,CV_32FC1);
					for (int r = sta_t; r <= end_t; r++)
					{
						// tmpPath[i][j][t] += 2*lambda*w[r-sta_t]*optPath[i][j][r];
						cons2 += 2*lambda*w[r-sta_t]*optPath[i][j][r];
					}
					//3rd cons
					Mat cons3 = Mat::zeros(3,3,CV_32FC1);
					int N = 0;
						if(i>0 && j>0)
						{
							N++;
							// tmpPath[i][j][t] += 2*optPath[i-1][j-1][t];
							cons3 += 2*optPath[i-1][j-1][t];
						}
						if(i>0)
						{
							N++;
							// tmpPath[i][j][t] += 2*optPath[i-1][j][t];
							cons3 += 2*optPath[i-1][j][t];
						}
						if(i>0 && j<height-1)
						{
							N++;
							// tmpPath[i][j][t] += 2*optPath[i-1][j+1][t];
							cons3 += 2*optPath[i-1][j+1][t];
						}
						if(j>0)
						{
							N++;
							// tmpPath[i][j][t] += 2*optPath[i][j-1][t];
							cons3 += 2*optPath[i][j-1][t];
						}
						if(j<height-1)
						{
							N++;
							// tmpPath[i][j][t] += 2*optPath[i][j+1][t];
							cons3 += 2*optPath[i][j+1][t];
						}
						if(i<width-1 && j>0)
						{
							N++;
							// tmpPath[i][j][t] += 2*optPath[i+1][j-1][t];
							cons3 += 2*optPath[i+1][j-1][t];
						}
						if(i<width-1)
						{
							N++;
							// tmpPath[i][j][t] += 2*optPath[i+1][j][t];
							cons3 += 2*optPath[i+1][j][t];
						}
						if(i<width-1 && j<height-1)
						{
							N++;
							// tmpPath[i][j][t] += 2*optPath[i+1][j+1][t];
							cons3 += 2*optPath[i+1][j+1][t];
						}
					float gamma = 2*lambda*w_sum+2*N+1;
					// tmpPath[i][j][t] = tmpPath[i][j][t]/gamma;
					// cout<<"cellPath:"<<endl<<cellPath[i][j][t]<<endl
					// <<"cons2:"<<endl<<cons2<<endl
					// <<"cons3:"<<endl<<cons3<<endl
					// <<"gamma:"<<gamma<<endl;
					tmpPath[i][j][t] = (cellPath[i][j][t] + cons2 + cons3)/gamma;
				}
		}
		// need deep copy here again
		for(int t = 0;t<time;t++)
			for(int i = 0;i<width;i++)
				for(int j = 0;j<height;j++)
				{
					optPath[i][j][t] = tmpPath[i][j][t].clone();
				}
		// optPath = tmpPath.clone();
	}
}

void allPath::jacobiPointSolver(int iter)
{
	//float lambda = 5.0; // TODO: need to optimize
	float *lambda = new float[time];
	for (int t = 0; t < time; t++)
		lambda[t] = 5.0;
	float cellsize = (height)*(width);
	cout << "iter time:" << iter << endl;
	for(int it = 0; it < iter; it++)
	{
		for(int t = 0; t < time; t++)
		{
			// int sta_t, end_t;
			// sta_t = t-30 < 0 ? 0 : t-30;
			// end_t = t+30 > time-1 ? time-1 : t+30;
			// int num_omega = end_t - sta_t + 1;
			// float w[num_omega] = {0.0};
			// float w_sum = 0.0;

			// // calc weights
			// for (int r = sta_t; r <= end_t; r++)
			// {
			// 	// if(r == t)
			// 	// {
			// 	// 	w[r-sta_t] = 0.0;
			// 	// 	continue;
			// 	// }
			// 	float trans =0.f;
			// 	for (int i = 0; i < width; i++)
			// 		for (int j = 0; j < height; j++)
			// 		{
			// 			trans += abs(cellPoints[i][j][r].x - cellPoints[i][j][t].x) + 
			// 					abs(cellPoints[i][j][r].y - cellPoints[i][j][t].y);
			// 		}
			// 	trans = trans / cellsize;
			// 	// cout<<"trans = "<<trans<<endl;
			// 	w[r-sta_t] = gaussianD(float(r-t), 10.f)*gaussianD(trans, 10.f)*10;
			// 	// cout<<"w[r] = "<<w[r-sta_t]<<endl;
			// 	w_sum += w[r-sta_t];
			// }

			// each cell
			for (int i = 0; i < width; i++)
				for (int j = 0; j < height; j++)
				{
					int sta_t, end_t;
					sta_t = t-30 < 0 ? 0 : t-30;
					end_t = t+30 > time-1 ? time-1 : t+30;
					int num_omega = end_t - sta_t + 1;
					float w[num_omega] = {0.0};
					float w_sum = 0.0;

					// calc weights
					for (int r = sta_t; r <= end_t; r++)
					{
						if(r == t)
						{
							w[r-sta_t] = 0.0;
							continue;
						}
						float trans = (abs(cellPoints[i][j][r].x - cellPoints[i][j][t].x) + 
										abs(cellPoints[i][j][r].y - cellPoints[i][j][t].y))/1000.0;
						// cout<<"trans = "<<trans<<endl;
						w[r-sta_t] = gaussianD(float(r-t), 10.f);//*gaussianD(trans, 1.f);
						// cout<<"w[r] = "<<w[r-sta_t]<<endl;
						w_sum += w[r-sta_t];
					}

					// 1st cons
					// tmpPath[i][j][t] = cellPath[i][j][t].clone();
					// 2nd cons
					Point2f cons2(0,0);
					for (int r = sta_t; r <= end_t; r++)
					{
						// tmpPath[i][j][t] += 2*lambda[t]*w[r-sta_t]*optPath[i][j][r];
						cons2 += 2*lambda[t]*w[r-sta_t]*optPoints[i][j][r];
					}

					//3rd cons
					Point2f cons3(0,0);
					int N = 0;
						if(0<i && i<width-1 && 0<j && j<height-1)
						{
							N=8;
							// tmpPath[i][j][t] += 2*optPath[i-1][j-1][t];
							cons3 += 2*optPoints[i-1][j-1][t];
							cons3 += 2*optPoints[i-1][j][t];
							cons3 += 2*optPoints[i-1][j+1][t];
							cons3 += 2*optPoints[i][j-1][t];
							cons3 += 2*optPoints[i][j+1][t];
							cons3 += 2*optPoints[i+1][j-1][t];
							cons3 += 2*optPoints[i+1][j][t];
							cons3 += 2*optPoints[i+1][j+1][t];
						}
						else if(0<i && i<width-1 && j==0)
						{
							N=2;
							cons3 += 2*optPoints[i-1][j][t];
							cons3 += 2*optPoints[i+1][j][t];
						}
						else if(0<i && i<width-1 && j==height-1)
						{
							N=2;
							cons3 += 2*optPoints[i-1][j][t];
							cons3 += 2*optPoints[i+1][j][t];
						}
						else if(i==0 && 0<j && j<height-1)
						{
							N=2;
							cons3 += 2*optPoints[i][j-1][t];
							cons3 += 2*optPoints[i][j+1][t];
						}
						else if(i==width-1 && 0<j && j<height-1)
						{
							N=2;
							cons3 += 2*optPoints[i][j-1][t];
							cons3 += 2*optPoints[i][j+1][t];
						}
						// N=0;cons3.x=0;cons3.y=0;
					float gamma = 2*lambda[t]*w_sum+2*N+1;
					// tmpPath[i][j][t] = tmpPath[i][j][t]/gamma;
					// cout<<"cellPath:"<<endl<<cellPath[i][j][t]<<endl
					// <<"cons2:"<<endl<<cons2<<endl
					// <<"cons3:"<<endl<<cons3<<endl
					// <<"gamma:"<<gamma<<endl;
					tmpPoints[i][j][t] = (cellPoints[i][j][t] + cons2 + cons3)/gamma;
				}

			Point2f diff = tmpPoints[width/2][height/2][t] - cellPoints[width/2][height/2][t];
			float diffx, diffy;
			Point2f pointsdiff = cellPoints[width/2+1][height/2+1][t] - cellPoints[width/2][height/2][t];
			diffx = abs(pointsdiff.x);
			diffy = abs(pointsdiff.y);
			pointsdiff = cellPoints[width/2+1][height/2][t] - cellPoints[width/2][height/2][t];
			diffx = max(diffx, abs(pointsdiff.x));
			diffy = max(diffy, abs(pointsdiff.y));
			pointsdiff = cellPoints[width/2][height/2+1][t] - cellPoints[width/2][height/2][t];
			diffx = max(diffx, abs(pointsdiff.x));
			diffy = max(diffy, abs(pointsdiff.y));
			if (abs(diff.x) > diffx/2 || abs(diff.y) > diffy/2 )
			{
				lambda[t] /= 10;
				//cout << "disconnect t: " << t << endl;
			}
		}
		// need deep copy here again
		// for(int t = 0;t<time;t++)
		// 	for(int i = 0;i<width;i++)
		// 		for(int j = 0;j<height;j++)
		// 		{
		// 			optPoints[i][j][t] = tmpPoints[i][j][t].clone();
		// 		}

		// smooth lambda
		float *tmp = new float[time];
		for (int t = 0; t < time; t++)
		{
			tmp[t] = lambda[t];
			lambda[t] = 0;
		}
		for (int t = 0; t < time; t++)
		{
			int N = 0; 
			for (int T = max(t-5, 0); T <= min(t+5, time-1); T++)
			{
				N++;
				lambda[t] += tmp[T];
			}
			lambda[t] /= N;
			//lambda[t] = 5.0;
		}
		//cout << "what's wrong" << endl;
		optPoints = tmpPoints;
	}

	// calc homography for each cell
	for(int t = 0; t<time;t++)
	{
		for(int i = 0; i < width-1; i++)
			for(int j = 0; j<height-1;j++)
			{
				vector<Point2f> O(4);
				O[0]=cellPoints[i][j][0];
				O[1]=cellPoints[i+1][j][0];
				O[2]=cellPoints[i][j+1][0];
				O[3]=cellPoints[i+1][j+1][0];
				vector<Point2f> V(4);
				V[0]=cellPoints[i][j][t];
				V[1]=cellPoints[i+1][j][t];
				V[2]=cellPoints[i][j+1][t];
				V[3]=cellPoints[i+1][j+1][t];
				vector<Point2f> W(4);
				W[0]=optPoints[i][j][t];
				W[1]=optPoints[i+1][j][t];
				W[2]=optPoints[i][j+1][t];
				W[3]=optPoints[i+1][j+1][t];
				BPath[i][j][t] = findHomography(W,V);//*findHomography(O, V).inv();
				// cout<<BPath[i][j][t]<<endl;
				// cout<<W[0]<<endl;
				// cout<<W[1]<<endl;
				// cout<<W[2]<<endl;
				// cout<<W[3]<<endl;
				// getchar();
			}
	}
}

void allPath::computeBPath()
{
	for(int t = 0;t<time;t++)
		for(int i = 0;i<width;i++)
			for(int j = 0;j<height;j++)
				BPath[i][j][t] = optPath[i][j][t]*cellPath[i][j][t].inv();
}

vector<Path> allPath::getcellPath(int t)
{
	if (t < 0 || t > time-1)
		throw runtime_error("allPath::getcellHomo: index can only inside the cell.\n");
	vector<Path> P(width, Path (height));
	for (int i = 0; i<width; i++)
		for(int j = 0; j<height;j++)
		{
			P[i][j] = cellPath[i][j][t].clone();
		}
	return P;
}

vector<Path> allPath::gethomoPath(int t)
{
	if (t < 0 || t > time-1)
		throw runtime_error("allPath::getcellHomo: index can only inside the cell.\n");
	vector<Path> P(width, Path (height));
	for (int i = 0; i<width; i++)
		for(int j = 0; j<height;j++)
		{
			P[i][j] = cellHomo[i][j][t].clone();
		}
	return P;
}

vector<Path> allPath::getoptPath(int t)
{
	if (t < 0 || t > time-1)
		throw runtime_error("allPath::getoptHomo: index can only inside the cell.\n");
	vector<Path> P(width, Path (height));
	for (int i = 0; i<width; i++)
		for(int j = 0; j<height;j++)
		{
			P[i][j] = optPath[i][j][t].clone();
		}
	return P;
}

vector<Path> allPath::getbPath(int t)
{
	if (t < 0 || t > time-1)
		throw runtime_error("allPath::getoptHomo: index can only inside the cell.\n");
	vector<Path> P(width, Path (height));
	for (int i = 0; i<width; i++)
		for(int j = 0; j<height;j++)
		{
			P[i][j] = BPath[i][j][t].clone();
		}
	return P;
}

// vector<PtsPath> allPath::getcellPoints(int t)
// {
// 	if (t < 0 || t > time-1)
// 		throw runtime_error("allPath::getoptPoints: index can only inside the cell.\n");
// 	vector<PtsPath> P(width, PtsPath (height));
// 	for (int i = 0; i<width; i++)
// 		for(int j = 0; j<height;j++)
// 		{
// 			P[i][j] = cellPoints[i][j][t];
// 		}
// 	return P;
// }

// vector<PtsPath> allPath::getoptPoints(int t)
// {
// 	if (t < 0 || t > time-1)
// 		throw runtime_error("allPath::getoptPoints: index can only inside the cell.\n");
// 	vector<PtsPath> P(width, PtsPath (height));
// 	for (int i = 0; i<width; i++)
// 		for(int j = 0; j<height;j++)
// 		{
// 			P[i][j] = optPoints[i][j][t];
// 		}
// 	return P;
// }

vector<Point2f> allPath::getcellPoints(int t)
{
	if (t < 0 || t > time-1)
		throw runtime_error("allPath::getoptPoints: index can only inside the cell.\n");
	vector<Point2f> P(width*height);
	for (int i = 0; i<width; i++)
		for(int j = 0; j<height;j++)
		{
			P[j*width+i] = cellPoints[i][j][t];
		}
	return P;
}

vector<Point2f> allPath::getoptPoints(int t)
{
	if (t < 0 || t > time-1)
		throw runtime_error("allPath::getoptPoints: index can only inside the cell.\n");
	vector<Point2f> P(width*height);
	for (int i = 0; i<width; i++)
		for(int j = 0; j<height;j++)
		{
			P[j*width+i] = optPoints[i][j][t];
		}
	return P;
}

void allPath::printcellPoints(int t)
{
	if (t < 0 || t > time-1)
		throw runtime_error("allPath::getcellPoints: index can only inside the cell.\n");
	for(int j = 0; j<height;j++)
	{
		for (int i = 0; i<width; i++)
		{
			cout<<cellPoints[i][j][t];
		}
		cout<<endl;
	}
}

void allPath::printoptPoints(int t)
{
	if (t < 0 || t > time-1)
		throw runtime_error("allPath::getoptPoints: index can only inside the cell.\n");
	for(int j = 0; j<height;j++)
	{
		for (int i = 0; i<width; i++)
		{
			cout<<optPoints[i][j][t];
		}
		cout<<endl;
	}
}